#pragma once

#include <JuceHeader.h>

#include <atomic>

#include "BufferStorage.hpp"
#include "CompilationFlags.h"
#include "EarlyReflections.hpp"
#include "FeedbackDelayNetwork.hpp"

namespace ParameterIDs
{
inline constexpr auto earlyRefVolume = "EarlyrefVolume";
inline constexpr auto fdnVolume = "FdnVolume";
inline constexpr auto reverbVolume = "ReverbVolume";
inline constexpr auto distance = "Distance";
inline constexpr auto roomSize = "Roomsize";
inline constexpr auto fdnT60 = "FdnT60";
inline constexpr auto fdnT60Ratio = "FdnT60Ratio";
}

class AmbVerbAudioProcessor final : public juce::AudioProcessor,
                                    private juce::AudioProcessorValueTreeState::Listener,
                                    private juce::AsyncUpdater
{
public:
    AmbVerbAudioProcessor();
    ~AmbVerbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    [[nodiscard]] juce::AudioProcessorValueTreeState& getParameterState() noexcept
    {
        return parameterState;
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static float parameterToGain(float value) noexcept;
    static float distanceToGain(float value) noexcept;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void rebuildDspConfiguration(bool activateImmediately);
    void loadRotationMatrices();

    FDN fdn;
    EarlyRef earlyref;
    juce::AudioProcessorValueTreeState parameterState;

    std::atomic<float>* earlyRefVolumeParameter = nullptr;
    std::atomic<float>* fdnVolumeParameter = nullptr;
    std::atomic<float>* reverbVolumeParameter = nullptr;
    std::atomic<float>* distanceParameter = nullptr;
    std::atomic<float>* roomSizeParameter = nullptr;
    std::atomic<float>* fdnT60Parameter = nullptr;
    std::atomic<float>* fdnT60RatioParameter = nullptr;

    FloatBuffer directSoundBuffer[NumAmbisonicsChannels];
    FloatBuffer earlyReflectionBuffer[NumAmbisonicsChannels];
    std::atomic<float> appliedRoomSize { static_cast<float>(MinRoomsize) };
    bool prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmbVerbAudioProcessor)
};
