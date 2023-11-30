

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FDN.h"
#include "JuceHeader.h"


//==============================================================================
AmbverbAudioProcessorEditor::AmbverbAudioProcessorEditor (AmbverbAudioProcessor& p)
: AudioProcessorEditor (&p), processor (p)
{
    printf("Editor Konstruktor");
    getLookAndFeel().setColour (Slider::thumbColourId, Colours::red);
    const OwnedArray<AudioProcessorParameter>& params = p.getParameters();
    for (int i = 0; i < params.size(); ++i)
    {
        if (const AudioParameterFloat* param = dynamic_cast<AudioParameterFloat*> (params[i]))
        {
            //... create a slider and label for this parameter
            
            Slider* aSlider;
            
            paramSliders.add (aSlider = new Slider (param->name));
            aSlider->setRange (param->range.start, param->range.end);
            aSlider->setSliderStyle (Slider::LinearHorizontal);
            if (i<=4) {
                aSlider->setTextBoxStyle (Slider::NoTextBox, false, 0, aSlider->getTextBoxHeight());
                
            }
            aSlider->setValue (*param);
            
            if (i==3) {
                aSlider->setColour(0x1001300, Colours::red);
            }
            else aSlider->setColour(0x1001300, Colours::mediumblue);
            
            aSlider->addListener (this);
            addAndMakeVisible (aSlider);
            
            Label* aLabel;
            paramLabels.add (aLabel = new Label (param->name, param->name));
            addAndMakeVisible (aLabel);
            
        }
    }
    
    noParameterLabel.setJustificationType (Justification::horizontallyCentred | Justification::verticallyCentred);
    noParameterLabel.setFont (noParameterLabel.getFont().withStyle (Font::italic));
    
    setSize (kParamSliderWidth + kParamLabelWidth,
             jmax (1, kParamControlHeight * paramSliders.size()));
    
    if (paramSliders.size() == 0)
        addAndMakeVisible (noParameterLabel);
    else
        startTimer (100);
    
    setSize (600, 280);
    //char Bufffer[100];
    //int cx = snprintf ( Bufffer, 100, "/Users/anonym/Dropbox/HybridReverb/Juce/AmbVerb/RotationMatrices/Rxz");
    
    //bool test = connect(1548);
    //printf("AmbverbAudioProcessorEditor::AmbverbAudioProcessorEditor");
    //std::cout<<"test = \n"<<test<<"\n";
    
    
}

AmbverbAudioProcessorEditor::~AmbverbAudioProcessorEditor()
{
}

//==============================================================================
void AmbverbAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::whitesmoke);
    
    g.setColour (Colours::white);
    g.setFont (15.0f);
    
    //g.drawFittedText ("Midi Volume", 0, 0, getWidth(), 30, Justification::centred, 1);
}


void AmbverbAudioProcessorEditor::resized()
{
    Rectangle<int> r = getLocalBounds();
    noParameterLabel.setBounds (r);
    
    for (int i = 0; i < paramSliders.size(); ++i)
    {
        Rectangle<int> paramBounds = r.removeFromTop (kParamControlHeight);
        Rectangle<int> labelBounds = paramBounds.removeFromLeft (kParamLabelWidth);
        
        paramLabels[i]->setBounds (labelBounds);
        paramSliders[i]->setBounds (paramBounds);
    }
    
    
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void AmbverbAudioProcessorEditor::sliderValueChanged (Slider* slider)
{
    
    
    if (AudioParameterFloat* param = getParameterForSlider (slider)){
        *param = (float) slider->getValue();
        if (param->getParameterIndex() == 0) {
            processor.earlyref.set_EarlyrefVolume(slider->getValue());
            printf("EarlyrefVolume\n");
            
        }
        else if(param->getParameterIndex() == 1) {
            processor.fdn.set_Volume(slider->getValue());
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
            
            
            if(slider->getValue() != processor.fdn.getMinDelaytime())
                
            {
                *processor.Roomsize = slider->getValue();
                
                processor.earlyref.set_Q((int)(Qmin+(slider->getValue()-MinRoomsize)/(MaxRoomsize-MinRoomsize)*(Qmax-Qmin)));

                processor.fdn.setDelayTimes((int)*processor.Roomsize, (int)(*processor.Roomsize * maxDelayTimeAt_xRoomsize));

                processor.fdn.setWindowBoundries(WindowStartsAt_xRoomsize*(int)(*processor.Roomsize), WindowStartsAt_xRoomsize*(int)(*processor.Roomsize)+processor.earlyref.OnsetLength*
                                             WindowEndsAt_xRoomsize);
                processor.earlyref.UnlockRotationMatrixForCalculaion();

                //processor.fdn.setWindowBoundries(processor.earlyref.getQ()/100*(processor.earlyref.IRsymmetryPoint- processor.earlyref.EarlyrefDelayTime), (int)processor.earlyref.getQ()/100*(processor.earlyref.EndOfIR - processor.earlyref.EarlyrefDelayTime));
                
                printf("Roomsize\n");
            }
        }
        else if(param->getParameterIndex() == 5) {
            processor.fdn.setT60(slider->getValue()+(WindowStartsAt_xRoomsize* *processor.Roomsize)/processor.getSampleRate());
            processor.earlyref.FilterCoeffA = processor.fdn.a0[(int)NumDelaylines-1];
            processor.earlyref.FilterCoeffB = processor.fdn.p[(int)NumDelaylines-1];
            printf("T60\n");
            
        }
        else if(param->getParameterIndex() == 6) {
            processor.fdn.setT60Ratio(slider->getValue());
            processor.earlyref.FilterCoeffA = processor.fdn.a0[(int)NumDelaylines-1];
            processor.earlyref.FilterCoeffB = processor.fdn.p[(int)NumDelaylines-1];
            printf("T60Ratio\n");
        }
    }
}

    void AmbverbAudioProcessorEditor:: set_DirectSoundVolume(float Value){
        if (Value < 0)  {
            printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
        }
        else if( Value > 1){
            printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
            
        }
        else if (Value == 1)
        {
            processor.DirectSoundVolume = 0;
        }
        else
            processor.DirectSoundVolume =  0.01*exp(4.605170*(1.0-Value));
    }
    
    void AmbverbAudioProcessorEditor:: set_ReverbVolume(float Value){
        if (Value < 0)  {
            printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
        }
        else if( Value > 1){
            printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
            
        }
        else if (Value == 0)
        {
            processor.ReverbVol = 0;
        }
        else
            processor.ReverbVol =  0.01*exp(4.605170*Value);
    }
