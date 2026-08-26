#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <BinaryData.h>

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace
{
constexpr auto parameterVersion = 1;

std::string_view resourceView(const char* data, int size)
{
    return { data, static_cast<std::size_t>(size) };
}

void writeRing(FloatBuffer& destination,
               std::size_t position,
               const float* source,
               int numSamples)
{
    const auto firstPart = juce::jmin(static_cast<std::size_t>(numSamples),
                                      destination.size() - position);
    memcpy(destination + position, source, firstPart * sizeof(float));

    if (firstPart < static_cast<std::size_t>(numSamples)) {
        memcpy(destination,
               source + firstPart,
               (static_cast<std::size_t>(numSamples) - firstPart) * sizeof(float));
    }
}

void copyRingWithMultiply(float* destination,
                          const FloatBuffer& source,
                          std::size_t position,
                          float gain,
                          int numSamples)
{
    const auto firstPart = juce::jmin(static_cast<std::size_t>(numSamples),
                                      source.size() - position);
    juce::FloatVectorOperations::copyWithMultiply(destination,
                                                   source + position,
                                                   gain,
                                                   static_cast<int>(firstPart));

    if (firstPart < static_cast<std::size_t>(numSamples)) {
        juce::FloatVectorOperations::copyWithMultiply(
            destination + firstPart,
            source,
            gain,
            numSamples - static_cast<int>(firstPart));
    }
}

void addRingWithMultiply(float* destination,
                         const FloatBuffer& source,
                         std::size_t position,
                         float gain,
                         int numSamples)
{
    const auto firstPart = juce::jmin(static_cast<std::size_t>(numSamples),
                                      source.size() - position);
    juce::FloatVectorOperations::addWithMultiply(destination,
                                                  source + position,
                                                  gain,
                                                  static_cast<int>(firstPart));

    if (firstPart < static_cast<std::size_t>(numSamples)) {
        juce::FloatVectorOperations::addWithMultiply(
            destination + firstPart,
            source,
            gain,
            numSamples - static_cast<int>(firstPart));
    }
}
}

AmbVerbAudioProcessor::AmbVerbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
                     #if !JucePlugin_IsMidiEffect
                      #if !JucePlugin_IsSynth
                       #if defined(AMBVERB_STEREO_COMPATIBILITY)
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                       #else
                     .withInput("Input", juce::AudioChannelSet::ambisonic(AmbisonicsOrder), true)
                       #endif
                      #endif
                      #if defined(AMBVERB_STEREO_COMPATIBILITY)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                      #else
                     .withOutput("Output", juce::AudioChannelSet::ambisonic(AmbisonicsOrder), true)
                      #endif
                     #endif
                     ),
      parameterState(*this, nullptr, "AmbVerbParameters", createParameterLayout())
#else
    : parameterState(*this, nullptr, "AmbVerbParameters", createParameterLayout())
#endif
{
    earlyRefVolumeParameter = parameterState.getRawParameterValue(ParameterIDs::earlyRefVolume);
    fdnVolumeParameter = parameterState.getRawParameterValue(ParameterIDs::fdnVolume);
    reverbVolumeParameter = parameterState.getRawParameterValue(ParameterIDs::reverbVolume);
    distanceParameter = parameterState.getRawParameterValue(ParameterIDs::distance);
    roomSizeParameter = parameterState.getRawParameterValue(ParameterIDs::roomSize);
    fdnT60Parameter = parameterState.getRawParameterValue(ParameterIDs::fdnT60);
    fdnT60RatioParameter = parameterState.getRawParameterValue(ParameterIDs::fdnT60Ratio);

    jassert(earlyRefVolumeParameter != nullptr
            && fdnVolumeParameter != nullptr
            && reverbVolumeParameter != nullptr
            && distanceParameter != nullptr
            && roomSizeParameter != nullptr
            && fdnT60Parameter != nullptr
            && fdnT60RatioParameter != nullptr);

    for (auto* parameterID : { ParameterIDs::roomSize,
                               ParameterIDs::fdnT60,
                               ParameterIDs::fdnT60Ratio })
        parameterState.addParameterListener(parameterID, this);

    for (int channel = 0; channel < NumAmbisonicsChannels; ++channel) {
        directSoundBuffer[channel].allocate(earlyref_Buffersize);
        earlyReflectionBuffer[channel].allocate(earlyref_Buffersize);
    }

    loadRotationMatrices();
    rebuildDspConfiguration(true);
}

AmbVerbAudioProcessor::~AmbVerbAudioProcessor()
{
    cancelPendingUpdate();

    for (auto* parameterID : { ParameterIDs::roomSize,
                               ParameterIDs::fdnT60,
                               ParameterIDs::fdnT60Ratio })
        parameterState.removeParameterListener(parameterID, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout AmbVerbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::earlyRefVolume, parameterVersion },
        "Early Reflections Volume",
        juce::NormalisableRange<float> { 0.0f, 1.0f },
        0.7f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::fdnVolume, parameterVersion },
        "Late Reverb Volume",
        juce::NormalisableRange<float> { 0.0f, 1.0f },
        0.7f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::reverbVolume, parameterVersion },
        "ER/LR Volume",
        juce::NormalisableRange<float> { 0.0f, 1.0f },
        1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::distance, parameterVersion },
        "Distance",
        juce::NormalisableRange<float> { 0.0f, 1.0f },
        0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::roomSize, parameterVersion },
        "Room Size",
        juce::NormalisableRange<float> { static_cast<float>(MinRoomsize),
                                         static_cast<float>(MaxRoomsize),
                                         1.0f },
        static_cast<float>(MinRoomsize)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::fdnT60, parameterVersion },
        "T60 (0 Hz)",
        juce::NormalisableRange<float> { 0.05f, 10.0f, 0.01f },
        2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::fdnT60Ratio, parameterVersion },
        "T60 Ratio",
        juce::NormalisableRange<float> { 0.01f, 1.0f, 0.001f },
        0.25f));

    return layout;
}

void AmbVerbAudioProcessor::loadRotationMatrices()
{
    const bool loaded = earlyref.readTransformationMatrices(
        resourceView(BinaryData::Rxz3_1, BinaryData::Rxz3_1Size),
        resourceView(BinaryData::Rxz3_2, BinaryData::Rxz3_2Size),
        resourceView(BinaryData::Ryz3_1, BinaryData::Ryz3_1Size),
        resourceView(BinaryData::Ryz3_2, BinaryData::Ryz3_2Size));

    if (!loaded)
        throw std::runtime_error("The embedded third-order rotation matrices are invalid");
}

const juce::String AmbVerbAudioProcessor::getName() const { return JucePlugin_Name; }

bool AmbVerbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AmbVerbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AmbVerbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AmbVerbAudioProcessor::getTailLengthSeconds() const { return 10.5; }

int AmbVerbAudioProcessor::getNumPrograms() { return 1; }
int AmbVerbAudioProcessor::getCurrentProgram() { return 0; }
void AmbVerbAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String AmbVerbAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}
void AmbVerbAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void AmbVerbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    prepared = sampleRate >= 8000.0
        && sampleRate <= 96000.0
        && samplesPerBlock > 0
        && static_cast<std::size_t>(samplesPerBlock
                                    + static_cast<int>(WindowStartsAt_xRoomsize * MaxRoomsize))
            <= earlyref_Buffersize;

    if (!prepared) {
        jassertfalse;
        return;
    }

   #if defined(AMBVERB_STEREO_COMPATIBILITY)
    stereoCompatibilityBuffer.setSize(NumAmbisonicsChannels,
                                      samplesPerBlock,
                                      false,
                                      true,
                                      false);
   #endif

    earlyref.reset();
    fdn.prepare(sampleRate, samplesPerBlock);

    for (int channel = 0; channel < NumAmbisonicsChannels; ++channel) {
        directSoundBuffer[channel].clear();
        earlyReflectionBuffer[channel].clear();
    }

    delayWritePosition = 0;

    rebuildDspConfiguration(true);
}

void AmbVerbAudioProcessor::releaseResources()
{
    prepared = false;
    earlyref.reset();
    fdn.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AmbVerbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
   #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
   #else
    #if defined(AMBVERB_STEREO_COMPATIBILITY)
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    #else
    const auto requiredLayout = juce::AudioChannelSet::ambisonic(AmbisonicsOrder);
    return layouts.getMainInputChannelSet() == requiredLayout
        && layouts.getMainOutputChannelSet() == requiredLayout;
    #endif
   #endif
}
#endif

float AmbVerbAudioProcessor::parameterToGain(float value) noexcept
{
    value = juce::jlimit(0.0f, 1.0f, value);
    return value == 0.0f ? 0.0f : 0.01f * std::exp(4.605170f * value);
}

float AmbVerbAudioProcessor::distanceToGain(float value) noexcept
{
    value = juce::jlimit(0.0f, 1.0f, value);
    return value == 1.0f ? 0.0f : 0.01f * std::exp(4.605170f * (1.0f - value));
}

void AmbVerbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int blockSize = buffer.getNumSamples();

   #if defined(AMBVERB_STEREO_COMPATIBILITY)
    if (getTotalNumInputChannels() != 2
        || getTotalNumOutputChannels() != 2
        || blockSize > stereoCompatibilityBuffer.getNumSamples()) {
        buffer.clear();
        jassertfalse;
        return;
    }

    constexpr float inverseSquareRootOfTwo = 0.7071067811865475f;
    stereoCompatibilityBuffer.clear();

    const auto* leftInput = buffer.getReadPointer(0);
    const auto* rightInput = buffer.getReadPointer(1);
    auto* wInput = stereoCompatibilityBuffer.getWritePointer(0);
    auto* yInput = stereoCompatibilityBuffer.getWritePointer(1);

    for (int sample = 0; sample < blockSize; ++sample) {
        wInput[sample] = (leftInput[sample] + rightInput[sample])
            * inverseSquareRootOfTwo;
        yInput[sample] = (leftInput[sample] - rightInput[sample])
            * inverseSquareRootOfTwo;
    }

    auto& processingBuffer = stereoCompatibilityBuffer;
   #else
    auto& processingBuffer = buffer;
   #endif

    const int inputChannels = processingBuffer.getNumChannels();
    const int outputChannels = processingBuffer.getNumChannels();

    if (!prepared
        || inputChannels != NumAmbisonicsChannels
        || outputChannels != NumAmbisonicsChannels
        || blockSize <= 0
        || static_cast<std::size_t>(blockSize) > earlyref_Buffersize) {
        buffer.clear();
        jassertfalse;
        return;
    }

    const int directAndEarlyDelay = static_cast<int>(
        WindowStartsAt_xRoomsize * appliedRoomSize.load(std::memory_order_acquire));

    if (static_cast<std::size_t>(blockSize + directAndEarlyDelay) > earlyref_Buffersize) {
        buffer.clear();
        jassertfalse;
        return;
    }

    const auto input = processingBuffer.getArrayOfReadPointers();
    auto output = processingBuffer.getArrayOfWritePointers();

    earlyref.set_EarlyrefVolume(earlyRefVolumeParameter->load(std::memory_order_relaxed));
    fdn.set_Volume(fdnVolumeParameter->load(std::memory_order_relaxed));
    const float directGain = distanceToGain(distanceParameter->load(std::memory_order_relaxed));
    const float reverbGain = parameterToGain(reverbVolumeParameter->load(std::memory_order_relaxed));

    earlyref.processBlock(input, blockSize);

    for (int channel = 0; channel < NumAmbisonicsChannels; ++channel) {
        writeRing(earlyReflectionBuffer[channel],
                  delayWritePosition,
                  earlyref.Output[channel],
                  blockSize);
        writeRing(directSoundBuffer[channel],
                  delayWritePosition,
                  input[channel],
                  blockSize);
    }

    fdn.processBlock(input[0], blockSize);

    const auto earlyReadPosition = (delayWritePosition + earlyref_Buffersize
                                    - static_cast<std::size_t>(directAndEarlyDelay))
        % earlyref_Buffersize;
    const auto directReadPosition = (earlyReadPosition + 100) % earlyref_Buffersize;

    for (int channel = 0; channel < NumAmbisonicsChannels; ++channel) {
        copyRingWithMultiply(output[channel],
                             directSoundBuffer[channel],
                             directReadPosition,
                             directGain,
                             blockSize);
        addRingWithMultiply(output[channel],
                            earlyReflectionBuffer[channel],
                            earlyReadPosition,
                            reverbGain,
                            blockSize);
        juce::FloatVectorOperations::addWithMultiply(output[channel],
                                                      fdn.Output[channel],
                                                      reverbGain,
                                                      blockSize);
    }

    delayWritePosition = (delayWritePosition + static_cast<std::size_t>(blockSize))
        % earlyref_Buffersize;

   #if defined(AMBVERB_STEREO_COMPATIBILITY)
    const auto* wOutput = processingBuffer.getReadPointer(0);
    const auto* yOutput = processingBuffer.getReadPointer(1);
    auto* leftOutput = buffer.getWritePointer(0);
    auto* rightOutput = buffer.getWritePointer(1);

    for (int sample = 0; sample < blockSize; ++sample) {
        leftOutput[sample] = (wOutput[sample] + yOutput[sample])
            * inverseSquareRootOfTwo;
        rightOutput[sample] = (wOutput[sample] - yOutput[sample])
            * inverseSquareRootOfTwo;
    }
   #endif
}

bool AmbVerbAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AmbVerbAudioProcessor::createEditor()
{
    return new AmbVerbAudioProcessorEditor(*this);
}

void AmbVerbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = parameterState.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void AmbVerbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName("ParamAmbverb")) {
            const auto restoreLegacyParameter = [this, &xml](const char* parameterID) {
                if (auto* parameter = parameterState.getParameter(parameterID)) {
                    const float currentValue = parameter->convertFrom0to1(parameter->getValue());
                    const float restoredValue = static_cast<float>(
                        xml->getDoubleAttribute(parameterID, currentValue));
                    parameter->setValueNotifyingHost(parameter->convertTo0to1(restoredValue));
                }
            };

            for (auto* parameterID : { ParameterIDs::earlyRefVolume,
                                       ParameterIDs::fdnVolume,
                                       ParameterIDs::reverbVolume,
                                       ParameterIDs::distance,
                                       ParameterIDs::roomSize,
                                       ParameterIDs::fdnT60,
                                       ParameterIDs::fdnT60Ratio })
                restoreLegacyParameter(parameterID);

            triggerAsyncUpdate();
            return;
        }

        const auto state = juce::ValueTree::fromXml(*xml);

        if (state.isValid() && state.hasType(parameterState.state.getType())) {
            parameterState.replaceState(state);
            triggerAsyncUpdate();
        }
    }
}

void AmbVerbAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(parameterID, newValue);
    triggerAsyncUpdate();
}

void AmbVerbAudioProcessor::handleAsyncUpdate()
{
    rebuildDspConfiguration(false);
}

void AmbVerbAudioProcessor::rebuildDspConfiguration(bool activateImmediately)
{
    const float roomSize = roomSizeParameter->load(std::memory_order_acquire);
    const float t60 = fdnT60Parameter->load(std::memory_order_acquire);
    const float t60Ratio = fdnT60RatioParameter->load(std::memory_order_acquire);
    const float q = Qmin
        + (roomSize - MinRoomsize) / static_cast<float>(MaxRoomsize - MinRoomsize)
            * (Qmax - Qmin);

    earlyref.set_Q(q);

    if (activateImmediately)
        earlyref.UnlockRotationMatrixForCalculaion();

    const int windowStart = static_cast<int>(WindowStartsAt_xRoomsize * roomSize);
    const int windowEnd = static_cast<int>(windowStart
        + earlyref.OnsetLength * WindowEndsAt_xRoomsize);

    fdn.setParameters(roomSize,
                      roomSize * maxDelayTimeAt_xRoomsize,
                      t60,
                      t60Ratio,
                      windowStart,
                      windowEnd);

    float earlyReflectionFilterGain = 1.0f;
    float earlyReflectionFilterPole = 0.0f;
    fdn.getPendingFilterCoefficients(earlyReflectionFilterGain,
                                     earlyReflectionFilterPole);
    earlyref.FilterCoeffA.store(earlyReflectionFilterGain, std::memory_order_release);
    earlyref.FilterCoeffB.store(earlyReflectionFilterPole, std::memory_order_release);
    appliedRoomSize.store(roomSize, std::memory_order_release);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmbVerbAudioProcessor();
}
