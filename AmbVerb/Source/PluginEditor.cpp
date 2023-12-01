/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmbVerbAudioProcessorEditor::AmbVerbAudioProcessorEditor (AmbVerbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    printf("Editor Konstruktor");
    getLookAndFeel().setColour (juce::Slider::thumbColourId, juce::Colours::red);
    const auto params = p.getParameters();
    for (int i = 0; i < params.size(); ++i)
    {
        if (auto param = dynamic_cast<juce::AudioParameterFloat*> (params[i]))
        {
            //... create a slider and label for this parameter
            
            juce::Slider* aSlider;
            
            paramSliders.add (aSlider = new juce::Slider (param->name));
            aSlider->setRange (param->range.start, param->range.end);
            aSlider->setSliderStyle (juce::Slider::LinearHorizontal);
            if (i<=4) {
                aSlider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, aSlider->getTextBoxHeight());
                
            }
            aSlider->setValue (*param);
            
            if (i==3) {
                aSlider->setColour(0x1001300, juce::Colours::red);
            }
            else aSlider->setColour(0x1001300, juce::Colours::mediumblue);
            
            aSlider->addListener (this);
            addAndMakeVisible (aSlider);
            
            juce::Label* aLabel;
            paramLabels.add (aLabel = new juce::Label (param->name, param->name));
            addAndMakeVisible (aLabel);
            
        }
    }
    
    noParameterLabel.setJustificationType (juce::Justification::horizontallyCentred | juce::Justification::verticallyCentred);
    noParameterLabel.setFont (noParameterLabel.getFont().withStyle (juce::Font::italic));
    
    setSize (kParamSliderWidth + kParamLabelWidth,
             fmax (1, kParamControlHeight * paramSliders.size()));
    
    if (paramSliders.size() == 0)
        addAndMakeVisible (noParameterLabel);
    else
        startTimer (100);
    
    setSize (600, 280);
    
}

AmbVerbAudioProcessorEditor::~AmbVerbAudioProcessorEditor()
{
}

//==============================================================================
void AmbVerbAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AmbVerbAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    juce::Rectangle<int> r = getLocalBounds();
    noParameterLabel.setBounds (r);
    
    for (int i = 0; i < paramSliders.size(); ++i)
    {
        juce::Rectangle<int> paramBounds = r.removeFromTop (kParamControlHeight);
        juce::Rectangle<int> labelBounds = paramBounds.removeFromLeft (kParamLabelWidth);
        
        paramLabels[i]->setBounds (labelBounds);
        paramSliders[i]->setBounds (paramBounds);
    }
    
   //
}

void AmbVerbAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    
    
    if (auto param = getParameterForSlider (slider)){
        *param = (float) slider->getValue();
        if (param->getParameterIndex() == 0) {
           // processor.earlyref.set_EarlyrefVolume(slider->getValue());
            printf("EarlyrefVolume\n");
            
        }
        else if(param->getParameterIndex() == 1) {
           // processor.fdn.set_Volume(slider->getValue());
            printf("FdnVolume \n");
            
        }
        else if(param->getParameterIndex() == 2) {
            printf("ReverbVolume\n");
            set_ReverbVolume(slider->getValue());
            
        }
        else if(param->getParameterIndex() == 3) {
            set_DirectSoundVolume(slider->getValue());
            printf("Distance\n");
            
        }
        
        else if(param->getParameterIndex() == 4) {
            
            
//            if(slider->getValue() != processor.fdn.getMinDelaytime())
//                
//            {
//                *processor.Roomsize = slider->getValue();
//                
//                processor.earlyref.set_Q((int)(Qmin+(slider->getValue()-MinRoomsize)/(MaxRoomsize-MinRoomsize)*(Qmax-Qmin)));
//
//                processor.fdn.setDelayTimes((int)*processor.Roomsize, (int)(*processor.Roomsize * maxDelayTimeAt_xRoomsize));
//
//                processor.fdn.setWindowBoundries(WindowStartsAt_xRoomsize*(int)(*processor.Roomsize), WindowStartsAt_xRoomsize*(int)(*processor.Roomsize)+processor.earlyref.OnsetLength*
//                                             WindowEndsAt_xRoomsize);
//                processor.earlyref.UnlockRotationMatrixForCalculaion();
//
//                /*processor.fdn.setWindowBoundries(processor.earlyref.getQ()/100*(processor.earlyref.IRsymmetryPoint- processor.earlyref.EarlyrefDelayTime), (int)processor.earlyref.getQ()/100*(processor.earlyref.EndOfIR - processor.earlyref.EarlyrefDelayTime));*/
//
//                printf("Roomsize\n");
//            }
        }
        else if(param->getParameterIndex() == 5) {
//            processor.fdn.setT60(slider->getValue()+(WindowStartsAt_xRoomsize* *processor.Roomsize)/processor.getSampleRate());
//            processor.earlyref.FilterCoeffA = processor.fdn.a0[(int)NumDelaylines-1];
//            processor.earlyref.FilterCoeffB = processor.fdn.p[(int)NumDelaylines-1];
//            printf("T60\n");
            
        }
        else if(param->getParameterIndex() == 6) {
//            processor.fdn.setT60Ratio(slider->getValue());
//            processor.earlyref.FilterCoeffA = processor.fdn.a0[(int)NumDelaylines-1];
//            processor.earlyref.FilterCoeffB = processor.fdn.p[(int)NumDelaylines-1];
//            printf("T60Ratio\n");
        }
    }
}


void AmbVerbAudioProcessorEditor:: set_DirectSoundVolume(float Value){
    if (Value < 0)  {
        printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
    }
    else if( Value > 1){
        printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
        
    }
//    else if (Value == 1)
//    {
//        processor.DirectSoundVolume = 0;
//    }
//    else
//        processor.DirectSoundVolume =  0.01*exp(4.605170*(1.0-Value));
}

void AmbVerbAudioProcessorEditor:: set_ReverbVolume(float Value){
    if (Value < 0)  {
        printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
    }
    else if( Value > 1){
        printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
        
    }
//    else if (Value == 0)
//    {
//        processor.ReverbVol = 0;
//    }
//    else
//        processor.ReverbVol =  0.01*exp(4.605170*Value);
}
