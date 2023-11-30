/*
 ==============================================================================
 
 This file was auto-generated!
 
 It contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "FDN.h"
#include "EarlyRef.h"
#include <Accelerate/Accelerate.h>
#include "CompilationFlags.h"


//==============================================================================
/**
 */
class AmbverbAudioProcessor  : public AudioProcessor, private File
{
public:
    //==============================================================================
    AmbverbAudioProcessor();
    //AmbverbAudioProcessor(int Start);
    ~AmbverbAudioProcessor();
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
    
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;
    
    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    const String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;
    
    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    FDN fdn;
    EarlyRef earlyref;
    float* DirectSoundBuffer[(AmbisonicsOrder+1)*(AmbisonicsOrder+1)];
    float* EarlyrefBuffer[(AmbisonicsOrder+1)*(AmbisonicsOrder+1)];

    //==============================================================================
    float DirectSoundVolume;
    float ReverbVol;
    
    AudioParameterFloat* EarlyrefVolume;
    AudioParameterFloat* FdnVolume;
    AudioParameterFloat* ReverbVolume;
    AudioParameterFloat* Distance;
    AudioParameterFloat* Roomsize;
    AudioParameterFloat* FdnT60;
    AudioParameterFloat* FdnT60Ratio;
    AudioParameterFloat* FdnDelayMax;
    AudioParameterFloat* EarlyrefQ;
    
private:
    
    bool OnOFF;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbverbAudioProcessor)
};


#endif  // PLUGINPROCESSOR_H_INCLUDED
