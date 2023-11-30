/*
 ==============================================================================
 
 This file was auto-generated!
 
 It contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "time.h"
/*
//==============================================================================
AmbverbAudioProcessor::AmbverbAudioProcessor(int Start)
#ifndef JucePlugin_PreferredChannelConfigurations
: AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                  .withInput  ("Input",  AudioChannelSet:: discreteChannel0(NumAmbisonicsChannels), true)
#endif
                  .withOutput ("Output", AudioChannelSet:: discreteChannel0(NumAmbisonicsChannels), true)
#endif
                  )
#endif
{
    
    
}
 */
//==============================================================================
AmbverbAudioProcessor::AmbverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
: AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                  .withInput  ("Input",  AudioChannelSet:: discreteChannel0(NumAmbisonicsChannels), true)
#endif
                  .withOutput ("Output", AudioChannelSet:: discreteChannel0(NumAmbisonicsChannels), true)
#endif
                  )
#endif
{
    //fdn = new FDN();
    printf("Audioprozessor Constructor\n");
    
    
    for (int i = 0; i < ((AmbisonicsOrder+1)*(AmbisonicsOrder+1)); i++) {
        DirectSoundBuffer[i] =  (float*) calloc(earlyref_Buffersize, sizeof(DirectSoundBuffer[i]));
        
        if (DirectSoundBuffer[i] == nullptr) {
            perror("Error Allocating Memory");return;
        }
        EarlyrefBuffer[i] =  (float*) calloc(earlyref_Buffersize, sizeof(EarlyrefBuffer[i]));
        
        if (EarlyrefBuffer[i] == nullptr) {
            perror("Error Allocating Memory");return;
        }
    }
    
    
    // - - - - - Initialize Audio Parameter - - - - -
    
    addParameter (EarlyrefVolume = new AudioParameterFloat ("EarlyrefVolume", "Early Reflections Volume", 0.0f, 1.0f, 0.7f));
    addParameter (FdnVolume = new AudioParameterFloat ("FdnVolume", "Late Reverb Volume", 0.0f, 1.0f, 0.7f));
    addParameter (ReverbVolume = new AudioParameterFloat ("ReverbVolume", "ER/LR Volume", 0.0f, 1.0f, 1.0f));
    addParameter (Distance = new AudioParameterFloat ("Distance", "Distance", 0.0f, 1.0f, 0.1f));
    addParameter (Roomsize = new AudioParameterFloat ("Roomsize", "Roomsize", MinRoomsize, MaxRoomsize, MinRoomsize));
    addParameter (FdnT60 = new AudioParameterFloat ("FdnT60", "T60(0Hz)", 0.0f, 10.0f, 2.0f));
    addParameter (FdnT60Ratio = new AudioParameterFloat ("FdnT60Ratio", "T60(0Hz)/T60(24 kHz)", 0.0f, 1.0f, 0.25f));
    
    

    
    
    
    
    
    File RotationMatrixDirectory = (File::getSpecialLocation(currentExecutableFile).getParentDirectory().getFullPathName()+"/RotationMatrices");
    
    std::cout<<"PATH1:  "<< (File::getSpecialLocation(currentExecutableFile).getParentDirectory().getFullPathName()+"/RotationMatrices")<<"\n";
    
    if (RotationMatrixDirectory.exists()==0) {
        
        FileChooser myChooser ("Please select Location of RotationMatrices...",
                               File::getSpecialLocation (File::userHomeDirectory),
                               "");
        if (myChooser.browseForDirectory())
        {
            File filePath =  myChooser.getResult().getFullPathName();
            filePath.copyDirectoryTo(File::getSpecialLocation(currentExecutableFile).getParentDirectory().getFullPathName()+"/RotationMatrices");
            std::cout<<"PATH2:  "<< myChooser.getResult().getFullPathName()<<"\n";
            
        }
        else std::cout<<"PRoblem\n";
        
    }
    
    this->earlyref.readTransformationMatrix((RotationMatrixDirectory.getFullPathName()).toStdString());
    this->earlyref.set_Q(Qmin+(*Roomsize-MinRoomsize)/(MaxRoomsize-MinRoomsize)*(Qmax-Qmin));
    this->earlyref.UnlockRotationMatrixForCalculaion();
    // - - - - - Activiere IR Subtraction innerhalb der FDN  - - - - -
    this->fdn.setT60(*FdnT60);
    this->fdn.setT60Ratio(*FdnT60Ratio);
    this->fdn.setDelayTimes((int)*Roomsize, (int)(*Roomsize * maxDelayTimeAt_xRoomsize));
    this->fdn.setWindowBoundries(WindowStartsAt_xRoomsize*(int)(*Roomsize), WindowStartsAt_xRoomsize*(int)(*Roomsize)+this->earlyref.OnsetLength*WindowEndsAt_xRoomsize);

}

AmbverbAudioProcessor::~AmbverbAudioProcessor()
{
}

//==============================================================================


const String AmbverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AmbverbAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AmbverbAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

double AmbverbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AmbverbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int AmbverbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AmbverbAudioProcessor::setCurrentProgram (int index)
{
}

const String AmbverbAudioProcessor::getProgramName (int index)
{
    return String();
}

void AmbverbAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void AmbverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void AmbverbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AmbverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != discreteChannel0(NumAmbisonicsChannels)
        && layouts.getMainOutputChannelSet() != discreteChannel0(NumAmbisonicsChannels)){}
    return false;
    
    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()){}
    return false;
#endif
    
    return true;
#endif
}
#endif

void AmbverbAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    
    
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    

    
    const float ** inBuffer = buffer.getArrayOfReadPointers();
    float ** outBuffer = buffer.getArrayOfWritePointers();
    int Blocksize = buffer.getNumSamples();
    int DS_ER_Delay = (int)(WindowStartsAt_xRoomsize* *Roomsize);
    
    earlyref.processBlock(inBuffer, buffer.getNumSamples(), getSampleRate()); //erzeuge Erstreflexionen
    for (int i = 0; i < totalNumOutputChannels; ++i) {
        memmove(EarlyrefBuffer[i], EarlyrefBuffer[i]+Blocksize, (earlyref_Buffersize - Blocksize)*sizeof(float));
        memcpy(EarlyrefBuffer[i]+earlyref_Buffersize-Blocksize, earlyref.Output[i], Blocksize*sizeof(float));
        memmove(DirectSoundBuffer[i], DirectSoundBuffer[i]+Blocksize, (earlyref_Buffersize - Blocksize)*sizeof(float));
        memcpy(DirectSoundBuffer[i]+earlyref_Buffersize-Blocksize, inBuffer[i], Blocksize*sizeof(float));
    }
    
    fdn.processBlock(inBuffer[0], buffer.getNumSamples(), getSampleRate()); //erzeuge Nachhall
    
    
    for (int i = 0; i < totalNumOutputChannels; ++i) { //Crossfade
        
        vDSP_vsmul(DirectSoundBuffer[i]+earlyref_Buffersize-Blocksize-DS_ER_Delay+100, 1, &DirectSoundVolume, outBuffer[i], 1, buffer.getNumSamples()); //Ausgabe des Direktschalls
        
        vDSP_vsma(EarlyrefBuffer[i]+earlyref_Buffersize-Blocksize-DS_ER_Delay, 1, &ReverbVol, outBuffer[i], 1, outBuffer[i], 1, buffer.getNumSamples());//Ausgabe der Erstreflexionen
        vDSP_vsma(fdn.Output[i], 1, &ReverbVol, outBuffer[i], 1, outBuffer[i], 1, buffer.getNumSamples());//Ausgabe des Nachhalls
    }

}

//==============================================================================
bool AmbverbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* AmbverbAudioProcessor::createEditor()
{
    return new AmbverbAudioProcessorEditor (*this);
}

//==============================================================================
void AmbverbAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    
    
    ScopedPointer<XmlElement> xml (new XmlElement ("ParamAmbverb"));

    xml->setAttribute ("EarlyrefVolume", (double) *EarlyrefVolume);
    copyXmlToBinary (*xml, destData);
    xml->setAttribute ("FdnVolume", (double) *FdnVolume);
    copyXmlToBinary (*xml, destData);
    xml->setAttribute ("ReverbVolume", (double) *ReverbVolume);
    copyXmlToBinary (*xml, destData);
    xml->setAttribute ("Distance", (double) *Distance);
    copyXmlToBinary (*xml, destData);
    xml->setAttribute ("Roomsize", (double) *Roomsize);
    copyXmlToBinary (*xml, destData);
    xml->setAttribute ("FdnT60", (double) *FdnT60);
    copyXmlToBinary (*xml, destData);
    xml->setAttribute ("FdnT60Ratio", (double) *FdnT60Ratio);
    copyXmlToBinary (*xml, destData);

    
    
    
}

void AmbverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr){
        
        printf("setStateinformation\n");
        if (xmlState->hasTagName ("ParamAmbverb")){
            
            *EarlyrefVolume = xmlState->getDoubleAttribute ("EarlyrefVolume", 0.7);
            this->earlyref.set_EarlyrefVolume(*EarlyrefVolume);
            
            *FdnVolume = xmlState->getDoubleAttribute ("FdnVolume", 0.7);
            this->fdn.set_Volume(*FdnVolume);
            
            *ReverbVolume = xmlState->getDoubleAttribute ("ReverbVolume", 1.0);
            if (*ReverbVolume == 0){ReverbVol = 0;}
            else {ReverbVol =  0.01*exp(4.605170*(*ReverbVolume));}
            
            *Distance = xmlState->getDoubleAttribute ("Distance", 0.1);
            if (*Distance == 1){DirectSoundVolume = 0;}
            else {DirectSoundVolume =  0.01*exp(4.605170*(1.0-*Distance));}
            
            float Helper= *Roomsize;
            *Roomsize = xmlState->getDoubleAttribute ("Roomsize", MinRoomsize);

            
            *FdnT60 = xmlState->getDoubleAttribute ("FdnT60", 2.0);
            this->fdn.setT60(*FdnT60+(WindowStartsAt_xRoomsize* *Roomsize)/getSampleRate());
            this->earlyref.FilterCoeffA = this->fdn.a0[(int)NumDelaylines-1];
            this->earlyref.FilterCoeffB = this->fdn.p[(int)NumDelaylines-1];
            
            *FdnT60Ratio = xmlState->getDoubleAttribute ("FdnT60Ratio", 0.25);
            this->fdn.setT60Ratio(*FdnT60Ratio);
            this->earlyref.FilterCoeffA = this->fdn.a0[(int)NumDelaylines-1];
            this->earlyref.FilterCoeffB = this->fdn.p[(int)NumDelaylines-1];
            

            if (Helper != *Roomsize) {
                this->earlyref.set_Q(Qmin+(*Roomsize-MinRoomsize)/(MaxRoomsize-MinRoomsize)*(Qmax-Qmin));
                this->earlyref.UnlockRotationMatrixForCalculaion();
                this->fdn.setDelayTimes((int)*Roomsize, (int)(*Roomsize * maxDelayTimeAt_xRoomsize));
                this->fdn.setWindowBoundries(WindowStartsAt_xRoomsize*(int)(*Roomsize),WindowStartsAt_xRoomsize*(int)(*Roomsize)+ earlyref.OnsetLength*
                                         WindowEndsAt_xRoomsize);


                
            }
            
            
            
        }
        
    }
    
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmbverbAudioProcessor();
}
