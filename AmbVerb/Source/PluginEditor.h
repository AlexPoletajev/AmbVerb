/*
   ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

   ==============================================================================
 */

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 */
class AmbVerbAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Slider::Listener, public juce::Timer
{
public:
AmbVerbAudioProcessorEditor (AmbVerbAudioProcessor&);
~AmbVerbAudioProcessorEditor() override;

enum {
    kParamControlHeight = 40,
    kParamLabelWidth    = 80,
    kParamSliderWidth   = 300
};


//==============================================================================
void paint(juce::Graphics&) override;
void resized() override;

void sliderValueChanged(juce::Slider *slider) override;


void sliderDragStarted(juce::Slider *slider) override {
    if (juce::AudioParameterFloat *param = getParameterForSlider(slider)) {
        param->beginChangeGesture();
    }
}

void sliderDragEnded(juce::Slider *slider) override {
    if (juce::AudioParameterFloat *param = getParameterForSlider(slider)) {
        param->endChangeGesture();
    }
}

private:

void timerCallback() override {
    auto params = getAudioProcessor()->getParameters();

    for (int i = 0; i < params.size(); ++i) {
        if (auto param = dynamic_cast<juce::AudioParameterFloat *> (params[i])) {
            if (i < paramSliders.size()) {
                paramSliders[i]->setValue(*param);
            }
        }
    }
}

void set_ReverbVolume(float Value);
void set_DirectSoundVolume(float Value);


juce::Label noParameterLabel;
juce::OwnedArray<juce::Slider> paramSliders;
juce::OwnedArray<juce::Label> paramLabels;

juce::AudioParameterFloat * getParameterForSlider(juce::Slider *slider) {
    auto params = getAudioProcessor()->getParameters();

    return dynamic_cast<juce::AudioParameterFloat *> (params[paramSliders.indexOf(slider)]);
}

private:
// This reference is provided as a quick way for your editor to
// access the processor object that created it.
AmbVerbAudioProcessor& audioProcessor;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmbVerbAudioProcessorEditor)
};
