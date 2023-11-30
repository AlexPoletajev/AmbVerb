/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "time.h"


//==============================================================================
TutorialPluginAudioProcessor::TutorialPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet:: discreteChannel0(36), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet:: discreteChannel0(36), true)
                     #endif
                       )
#endif
{

   // fdn = *new FDN();
    DirectSoundVolume = 1.0;
    earlyrefVolume = 1.0 ;
    fdnVolume = 1.0;
}

TutorialPluginAudioProcessor::~TutorialPluginAudioProcessor()
{
}

//==============================================================================
const String TutorialPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TutorialPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TutorialPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

double TutorialPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TutorialPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TutorialPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TutorialPluginAudioProcessor::setCurrentProgram (int index)
{
}

const String TutorialPluginAudioProcessor::getProgramName (int index)
{
    return String();
}

void TutorialPluginAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void TutorialPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void TutorialPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TutorialPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != discreteChannel0(36)
        && layouts.getMainOutputChannelSet() != discreteChannel0(36)){}
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()){}
        return false;
   #endif

    return true;
  #endif
}
#endif

void TutorialPluginAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{

    float time1= clock();
    
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    
    const float ** inBuffer = buffer.getArrayOfReadPointers();
    float ** outBuffer = buffer.getArrayOfWritePointers();
    
    /*
    for (int i = 0; i < totalNumOutputChannels; ++i) {
        printf("%f\n", *inBuffer[i]);
        
    }

    printf("\n");
*/
    earlyref.processBlock(inBuffer, buffer.getNumSamples(), totalNumOutputChannels, getSampleRate());

    fdn.processBlock(inBuffer[0], buffer.getNumSamples(), totalNumOutputChannels, getSampleRate());


    for (int i = 0; i < totalNumOutputChannels; ++i) {
       
        vDSP_vclr(outBuffer[i], 1,buffer.getNumSamples());

       // vDSP_vsma(inBuffer[i], 1, &DirectSoundVolume, fdn.Output[i], 1, outBuffer[i], 1, buffer.getNumSamples());
       // vDSP_vsma(earlyref.Output[i], 1, &earlyrefVolume, outBuffer[i], 1, outBuffer[i], 1, buffer.getNumSamples());
        //vDSP_vsma(earlyref.Output[i], 1, &earlyrefVolume, fdn.Output[i], 1, outBuffer[i], 1, buffer.getNumSamples());
        //vDSP_vsma(fdn.Output[i], 1, &earlyrefVolume, outBuffer[i], 1, outBuffer[i], 1, buffer.getNumSamples());

        vDSP_vadd(earlyref.Output[i], 1, fdn.Output[i], 1, outBuffer[i], 1, buffer.getNumSamples());


    }
    
    if (fdn.check%200 == 0) {
        float      time2= clock();
        printf("'AmbVerb_DSP():    Time: = %f\n", (time2-time1)/CLOCKS_PER_SEC*44100);
        printf("NumOutputs : = %i\n",totalNumOutputChannels);
        printf("NumInputs : = %i\n",totalNumInputChannels);


    }
    fdn.check++;
}

//==============================================================================
bool TutorialPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* TutorialPluginAudioProcessor::createEditor()
{
    return new TutorialPluginAudioProcessorEditor (*this);
}

//==============================================================================
void TutorialPluginAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void TutorialPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TutorialPluginAudioProcessor();
}
