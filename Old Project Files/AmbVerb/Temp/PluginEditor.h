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
#include "FDN.h"


//==============================================================================
/**
 */
class TutorialPluginAudioProcessorEditor  : public AudioProcessorEditor, private Slider::Listener, public Button::Listener
{
public:
    TutorialPluginAudioProcessorEditor (TutorialPluginAudioProcessor&);
    ~TutorialPluginAudioProcessorEditor();
    
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    
    void buttonClicked (Button* button) override  // [2]
    {
    if (button == &set_DelaytimesMinMaxButton)                                                      // [3]
    {
        processor.fdn.set_DelayTimeMinMax(1000, 2500);
    }
    else if (button == &reset_DelaytimesMinMaxButton)                                                      // [3]
    {
        processor.fdn.reset_DelayTimes();
    }
}


private:

void sliderValueChanged (Slider* slider) override;

// This reference is provided as a quick way for your editor to
// access the processor object that created it.

TutorialPluginAudioProcessor& processor;
TextButton set_DelaytimesMinMaxButton;
TextButton reset_DelaytimesMinMaxButton;

Label timeLabel;
Label testLabel;
Slider Slider_T60;
Slider Slider_T60Ratio;
Slider Slider_Q;
Slider Slider_Phi;
Slider Slider_earlyrefVol;
Slider Slider_DirectSoundVol;




Slider Slider_fdnVol;
Random random;
Label levelLabel;
Slider levelSlider;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TutorialPluginAudioProcessorEditor)
};


#endif  // PLUGINEDITOR_H_INCLUDED
