/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

 From Dr Buisin tutorial:
 https://www.youtube.com/watch?v=iFBtwj23-go&t=5084s&ab_channel=DrBruisin
 
 
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BasicGainPhaseChoiceAudioProcessor::BasicGainPhaseChoiceAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    treeState.addParameterListener("gain", this);
    treeState.addParameterListener("phase", this);
    treeState.addParameterListener("choice", this);
}

BasicGainPhaseChoiceAudioProcessor::~BasicGainPhaseChoiceAudioProcessor()
{
    treeState.removeParameterListener("gain", this);
    treeState.removeParameterListener("phase", this);
    treeState.removeParameterListener("choice", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout BasicGainPhaseChoiceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    juce::StringArray choices = {"Compressor", "EQ", "Reverb"};
    
    auto pGain = std::make_unique<juce::AudioParameterFloat>("gain", "Gain", -24.0, 24.0, 0.0);
    auto pPhase = std::make_unique<juce::AudioParameterBool>("phase", "Phase", false);
    auto pChoice = std::make_unique<juce::AudioParameterChoice>("choice", "Choice", choices, 0);
    
    params.push_back(std::move(pGain));
    params.push_back(std::move(pPhase));
    params.push_back(std::move(pChoice));
    
    return { params.begin(), params.end() };
}

void BasicGainPhaseChoiceAudioProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
    if(parameterID == "gain")
    {
        rawGain = juce::Decibels::decibelsToGain(newValue);
        DBG("Gain is: " << newValue);
    }
    
    if(parameterID == "phase")
    {
        phase = newValue;
        DBG("Phase is: " << newValue);
    }
    
    if(parameterID == "choice")
    {
        choice = newValue;
        DBG("Choice is: " << newValue);
    }
}

//==============================================================================
const juce::String BasicGainPhaseChoiceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BasicGainPhaseChoiceAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BasicGainPhaseChoiceAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BasicGainPhaseChoiceAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BasicGainPhaseChoiceAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BasicGainPhaseChoiceAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BasicGainPhaseChoiceAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BasicGainPhaseChoiceAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BasicGainPhaseChoiceAudioProcessor::getProgramName (int index)
{
    return {};
}

void BasicGainPhaseChoiceAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BasicGainPhaseChoiceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    phase = *treeState.getRawParameterValue("phase");
    rawGain = juce::Decibels::decibelsToGain(static_cast<float>(*treeState.getRawParameterValue("gain")));
}

void BasicGainPhaseChoiceAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BasicGainPhaseChoiceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void BasicGainPhaseChoiceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // My audio block object
    juce::dsp::AudioBlock<float> block (buffer);
    
    // My dsp block
    for(int channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto* channelData = block.getChannelPointer(channel);
        
        for(int sample = 0; sample < block.getNumSamples(); ++sample)
        {
            if (phase)
            {
                channelData[sample] *= rawGain * -1.0;
            }
            else
            {
                channelData[sample] *= rawGain;
            }
        }
    }
}

//==============================================================================
bool BasicGainPhaseChoiceAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BasicGainPhaseChoiceAudioProcessor::createEditor()
{
//    return new BasicGainPhaseChoiceAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void BasicGainPhaseChoiceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save params
    juce::MemoryOutputStream stream(destData, false);
    treeState.state.writeToStream(stream);
}

void BasicGainPhaseChoiceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Recall params
    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    
    if(tree.isValid())
    {
        treeState.state = tree;
        phase = *treeState.getRawParameterValue("phase");
        rawGain = juce::Decibels::decibelsToGain(static_cast<float>(*treeState.getRawParameterValue("gain")));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BasicGainPhaseChoiceAudioProcessor();
}
