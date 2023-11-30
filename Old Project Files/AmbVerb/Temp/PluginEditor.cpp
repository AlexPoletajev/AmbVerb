/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FDN.h"


//==============================================================================
TutorialPluginAudioProcessorEditor::TutorialPluginAudioProcessorEditor (TutorialPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{

    //setResizable(0, 0);
    /*

    
    addAndMakeVisible (timeLabel);
    timeLabel.setColour (Label::backgroundColourId, Colours::black);
    timeLabel.setColour (Label::textColourId, Colours::white);
    timeLabel.setJustificationType (Justification::centred);
 
    addAndMakeVisible (testLabel);
    testLabel.setColour (Label::backgroundColourId, Colours::greenyellow);
    testLabel.setColour (Label::textColourId, Colours::black);
    testLabel.setJustificationType (Justification::centred);
 */
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    // these define the parameters of our slider object
    Slider_T60.setSliderStyle (Slider::LinearVertical);
    Slider_T60.setRange(0.0, 20.0, 0.1);
    Slider_T60.setTextBoxStyle (Slider::NoTextBox, false, 50, 0);
    Slider_T60.setPopupDisplayEnabled (true, this);
    Slider_T60.setTextValueSuffix ("T60");
    Slider_T60.setValue(2.0);
    addAndMakeVisible (&Slider_T60);
    Slider_T60.addListener (this);

    
    Slider_T60Ratio.setSliderStyle (Slider::LinearVertical);
    Slider_T60Ratio.setRange(0.0, 1.0, 0.01);
    Slider_T60Ratio.setTextBoxStyle (Slider::NoTextBox, false, 50, 0);
    Slider_T60Ratio.setPopupDisplayEnabled (true, this);
    Slider_T60Ratio.setTextValueSuffix ("T60Ratio");
    Slider_T60Ratio.setValue(0.1);
    addAndMakeVisible (&Slider_T60Ratio);
    Slider_T60Ratio.addListener (this);

    
    Slider_fdnVol.setSliderStyle (Slider::LinearVertical);
    Slider_fdnVol.setRange(0.0, 1.0, 0.01);
    Slider_fdnVol.setTextBoxStyle (Slider::NoTextBox, false, 50, 0);
    Slider_fdnVol.setPopupDisplayEnabled (true, this);
    Slider_fdnVol.setTextValueSuffix ("T60Ratio");
    Slider_fdnVol.setValue(1);
    addAndMakeVisible (&Slider_fdnVol);
    Slider_fdnVol.addListener (this);

    Slider_Q.setSliderStyle (Slider::LinearVertical);
    Slider_Q.setRange(50, 500, 50);
    Slider_Q.setTextBoxStyle (Slider::NoTextBox, false, 50, 0);
    Slider_Q.setPopupDisplayEnabled (true, this);
    Slider_Q.setTextValueSuffix ("T60Ratio");
    Slider_Q.setValue(400);
    addAndMakeVisible (&Slider_Q);
    Slider_Q.addListener (this);
    
    Slider_Phi.setSliderStyle (Slider::LinearVertical);
    Slider_Phi.setRange(0.0, 100.0, 10);
    Slider_Phi.setTextBoxStyle (Slider::NoTextBox, false, 50, 0);
    Slider_Phi.setPopupDisplayEnabled (true, this);
    Slider_Phi.setTextValueSuffix ("T60Ratio");
    Slider_Phi.setValue(80);
    addAndMakeVisible (&Slider_Phi);
    Slider_Phi.addListener (this);
    
    Slider_earlyrefVol.setSliderStyle (Slider::LinearVertical);
    Slider_earlyrefVol.setRange(0.0, 1.0, 0.01);
    Slider_earlyrefVol.setTextBoxStyle (Slider::NoTextBox, false, 50, 0);
    Slider_earlyrefVol.setPopupDisplayEnabled (true, this);
    Slider_earlyrefVol.setTextValueSuffix ("T60Ratio");
    Slider_earlyrefVol.setValue(1);
    addAndMakeVisible (&Slider_earlyrefVol);
    Slider_earlyrefVol.addListener (this);
    
    Slider_DirectSoundVol.setSliderStyle (Slider::LinearVertical);
    Slider_DirectSoundVol.setRange(0.0, 1.0, 0.01);
    Slider_DirectSoundVol.setTextBoxStyle (Slider::NoTextBox, false, 50, 0);
    Slider_DirectSoundVol.setPopupDisplayEnabled (true, this);
    Slider_DirectSoundVol.setTextValueSuffix ("T60Ratio");
    Slider_DirectSoundVol.setValue(1);
    addAndMakeVisible (&Slider_DirectSoundVol);
    Slider_DirectSoundVol.addListener (this);

    addAndMakeVisible (set_DelaytimesMinMaxButton);
    set_DelaytimesMinMaxButton.setButtonText ("set_DT");
    set_DelaytimesMinMaxButton.addListener (this); // [7]
    
    addAndMakeVisible (reset_DelaytimesMinMaxButton);
    reset_DelaytimesMinMaxButton.setButtonText ("reset_DT");
    reset_DelaytimesMinMaxButton.addListener (this); // [7]
    
    // add the listener to the slider
    //levelSlider.addListener(this);
    setSize (600, 400);
}

TutorialPluginAudioProcessorEditor::~TutorialPluginAudioProcessorEditor()
{
}

//==============================================================================
void TutorialPluginAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::white);

    g.setColour (Colours::black);
    g.setFont (15.0f);
 
    g.drawFittedText ("Midi Volume", 0, 0, getWidth(), 30, Justification::centred, 1);
}


void TutorialPluginAudioProcessorEditor::resized()
{
    // sets the position and size of the slider with arguments (x, y, width, height)
    set_DelaytimesMinMaxButton.setBounds (240, 20, 40, 40);
    reset_DelaytimesMinMaxButton.setBounds (240, 80, 40, 40);

    //timeLabel.setBounds (10, 60, getWidth() - 20, 40);
    //testLabel.setBounds (10, 110, getWidth() - 20, 80);
    Slider_T60.setBounds (20, 20, 20, getHeight() - 60);
    Slider_T60Ratio.setBounds (50, 20, 20, getHeight() - 60);
    Slider_Q.setBounds (80, 20, 20, getHeight() - 60);
        Slider_Phi.setBounds (110, 20, 20, getHeight() - 60);
        Slider_fdnVol.setBounds (140, 20, 20, getHeight() - 60);
            Slider_earlyrefVol.setBounds (170, 20, 20, getHeight() - 60);
            Slider_DirectSoundVol.setBounds (200, 20, 20, getHeight() - 60);

    //levelLabel.setBounds (10, 10, 90, 20);
    //levelSlider.setBounds (100, 10, getWidth() - 110, 20);

    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void TutorialPluginAudioProcessorEditor::sliderValueChanged (Slider* slider)
{
    // printf("inputs: %i\n", processor.getTotalNumInputChannels());
    // printf("outputs: %i\n", processor.getTotalNumOutputChannels());
    
    if (slider == &Slider_T60){
        processor.fdn.set_T60(Slider_T60.getValue());
        printf("midVol");
    }
    else if (slider == &Slider_T60Ratio){
        processor.fdn.set_T60Ratio(Slider_T60Ratio.getValue());
        
    }
    else if (slider == &Slider_fdnVol){
        processor.fdn.set_Volume(Slider_fdnVol.getValue());
        
    }
    else if (slider == &Slider_Q){
        processor.earlyref.set_Q(Slider_Q.getValue());
        processor.fdn.IRSubtracionOn(Slider_Q.getValue()/100*(processor.earlyref.EndOfIR - processor.earlyref.DirectSoundDelayTime));//(1.0+Qx+Qy)*3.095);
    }
    else if (slider == &Slider_Phi){
        processor.earlyref.set_RotAngle(Slider_Phi.getValue());
    }
    else if (slider == &Slider_earlyrefVol){
        processor.earlyref.set_EarlyrefVolume(Slider_earlyrefVol.getValue());
    }
    else if (slider == &Slider_DirectSoundVol){
        processor.earlyref.set_DirectSoundVolume(Slider_DirectSoundVol.getValue());
    }
    
}


