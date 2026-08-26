#include "PluginEditor.h"

namespace
{
struct ControlDefinition
{
    const char* parameterID;
    const char* label;
};

constexpr ControlDefinition controls[] {
    { ParameterIDs::earlyRefVolume, "Early reflections" },
    { ParameterIDs::fdnVolume, "Late reverb" },
    { ParameterIDs::reverbVolume, "Reverb mix" },
    { ParameterIDs::distance, "Distance" },
    { ParameterIDs::roomSize, "Room size" },
    { ParameterIDs::fdnT60, "T60" },
    { ParameterIDs::fdnT60Ratio, "HF decay ratio" },
};
}

AmbVerbAudioProcessorEditor::AmbVerbAudioProcessorEditor(AmbVerbAudioProcessor& processor)
    : AudioProcessorEditor(&processor), audioProcessor(processor)
{
    auto& state = audioProcessor.getParameterState();

    for (const auto& definition : controls) {
        auto* slider = parameterSliders.add(new juce::Slider());
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 24);
        slider->setColour(juce::Slider::trackColourId, juce::Colours::cornflowerblue);
        slider->setColour(juce::Slider::thumbColourId, juce::Colours::white);
        addAndMakeVisible(slider);

        if (auto* parameter = state.getParameter(definition.parameterID)) {
            const auto range = parameter->getNormalisableRange();
            slider->setRange(range.start, range.end, range.interval);
        }

        auto* label = parameterLabels.add(new juce::Label());
        label->setText(definition.label, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);

        parameterAttachments.add(new SliderAttachment(state,
                                                       definition.parameterID,
                                                       *slider));
    }

    setResizable(true, true);
    setResizeLimits(520, 310, 960, 640);
    setSize(640, 340);
}

void AmbVerbAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(24, 27, 34));
    graphics.setColour(juce::Colours::white);
    graphics.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    graphics.drawText("AmbVerb", 20, 10, getWidth() - 40, 30, juce::Justification::centredLeft);

    graphics.setColour(juce::Colours::lightgrey);
    graphics.setFont(juce::FontOptions(12.0f));
    graphics.drawText("Third-order Ambisonics reverb",
                      20,
                      38,
                      getWidth() - 40,
                      20,
                      juce::Justification::centredLeft);
}

void AmbVerbAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(55);

    constexpr int rowHeight = 36;
    constexpr int labelWidth = 145;

    for (int index = 0; index < parameterSliders.size(); ++index) {
        auto row = bounds.removeFromTop(rowHeight);
        parameterLabels[index]->setBounds(row.removeFromLeft(labelWidth));
        parameterSliders[index]->setBounds(row);
    }
}
