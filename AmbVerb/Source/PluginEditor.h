#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

class AmbVerbAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AmbVerbAudioProcessorEditor(AmbVerbAudioProcessor&);
    ~AmbVerbAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    AmbVerbAudioProcessor& audioProcessor;
    juce::OwnedArray<juce::Slider> parameterSliders;
    juce::OwnedArray<juce::Label> parameterLabels;
    juce::OwnedArray<SliderAttachment> parameterAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmbVerbAudioProcessorEditor)
};
