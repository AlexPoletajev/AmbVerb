/*
 ==============================================================================
 
 This file was auto-generated!
 
 It contains the basic framework code for a JUCE plugin editor.
 
 ==============================================================================
 */ 

#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED


#include "../JuceLibraryCode/JuceHeader.h"

#include "PluginProcessor.h"
//#include "FDN.h"


//==============================================================================
/**
 */
class AmbverbAudioProcessorEditor  : public AudioProcessorEditor, private Slider::Listener,private Timer, public OSCReceiver//, public FileChooser
{
public:
    
    enum
    {
        kParamControlHeight = 40,
        kParamLabelWidth = 80,
        kParamSliderWidth = 300
    };
    
    AmbverbAudioProcessorEditor (AmbverbAudioProcessor&);
    ~AmbverbAudioProcessorEditor();
    
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

void sliderValueChanged (Slider* slider) override;


void sliderDragStarted (Slider* slider) override
{
if (AudioParameterFloat* param = getParameterForSlider (slider))
param->beginChangeGesture();
}

void sliderDragEnded (Slider* slider) override
{
if (AudioParameterFloat* param = getParameterForSlider (slider))
param->endChangeGesture();
}


private:

void timerCallback()
{
const OwnedArray<AudioProcessorParameter>& params = getAudioProcessor()->getParameters();

    for (int i = 0; i < params.size(); ++i)
    {
        if (const AudioParameterFloat* param = dynamic_cast<AudioParameterFloat*> (params[i]))
        {
            if (i < paramSliders.size())
            {
                
                 paramSliders[i]->setValue (*param);
                
            }
        }
        
    }
}

AudioParameterFloat* getParameterForSlider (Slider* slider)
{
    const OwnedArray<AudioProcessorParameter>& params = getAudioProcessor()->getParameters();
    return dynamic_cast<AudioParameterFloat*> (params[paramSliders.indexOf (slider)]);
}

void set_ReverbVolume(float Value);
void set_DirectSoundVolume(float Value);


Label noParameterLabel;
OwnedArray<Slider> paramSliders;
OwnedArray<Label> paramLabels;

AmbverbAudioProcessor& processor;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbverbAudioProcessorEditor)
};


#endif  // PLUGINEDITOR_H_INCLUDED
