/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "EarlyReflections.hpp"
#include "FeedbackDelayNetwork.hpp"
#include "CompilationFlags.h"

//==============================================================================
/**
*/
class AmbVerbAudioProcessor  : public juce::AudioProcessor //, private juce::File
{
public:
    //==============================================================================
    AmbVerbAudioProcessor();
    ~AmbVerbAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    FDN fdn;
    EarlyRef earlyref;
    float* DirectSoundBuffer[(AmbisonicsOrder+1)*(AmbisonicsOrder+1)];
    float* EarlyrefBuffer[(AmbisonicsOrder+1)*(AmbisonicsOrder+1)];

    //==============================================================================
    float DirectSoundVolume;
    float ReverbVol;
    
    juce::AudioParameterFloat* EarlyrefVolume;
    juce::AudioParameterFloat* FdnVolume;
    juce::AudioParameterFloat* ReverbVolume;
    juce::AudioParameterFloat* Distance;
    juce::AudioParameterFloat* Roomsize;
    juce::AudioParameterFloat* FdnT60;
    juce::AudioParameterFloat* FdnT60Ratio;
    juce::AudioParameterFloat* FdnDelayMax;
    juce::AudioParameterFloat* EarlyrefQ;
    
private:
    
    bool OnOFF;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbVerbAudioProcessor)
};
