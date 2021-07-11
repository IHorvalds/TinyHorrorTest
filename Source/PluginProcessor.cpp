/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TinyTerrorMaybeAudioProcessor::TinyTerrorMaybeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : foleys::MagicProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    magicState.setGuiValueTree(BinaryData::tinyterrorface, BinaryData::tinyterrorfaceSize);
}

TinyTerrorMaybeAudioProcessor::~TinyTerrorMaybeAudioProcessor()
{
}

//==============================================================================
const juce::String TinyTerrorMaybeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TinyTerrorMaybeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TinyTerrorMaybeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TinyTerrorMaybeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TinyTerrorMaybeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TinyTerrorMaybeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TinyTerrorMaybeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TinyTerrorMaybeAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TinyTerrorMaybeAudioProcessor::getProgramName (int index)
{
    return {};
}

void TinyTerrorMaybeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TinyTerrorMaybeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    TinyTerrirParams settings = getSettings();

    os.reset();
    os.initProcessing(samplesPerBlock);
    tinyTerror.reset(os.getOversamplingFactor() * sampleRate);
    tinyTerror.setParameters(settings);

    TinyTerrirAmp *tta = &(this->tinyTerror);
    ws.functionToUse = [tta] (float xn){
        return tta->processAudioSample((double)xn);
    };
}

void TinyTerrorMaybeAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TinyTerrorMaybeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void TinyTerrorMaybeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    auto settings = getSettings();
    tinyTerror.setParameters(settings);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> osBlock;

    osBlock = os.processSamplesUp(block);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        juce::dsp::ProcessContextReplacing<float> procCtx(osBlock.getSingleChannelBlock(ch));
        ws.process(procCtx);
    }

    // downsample
    os.processSamplesDown(block);

}

////==============================================================================
//bool TinyTerrorMaybeAudioProcessor::hasEditor() const
//{
//    return true; // (change this to false if you choose to not supply an editor)
//}
//
//juce::AudioProcessorEditor* TinyTerrorMaybeAudioProcessor::createEditor()
//{
//    //return new TinyTerrorMaybeAudioProcessorEditor (*this);
//    return new foleys::MagicPluginEditor(magicState);
//}

//==============================================================================
void TinyTerrorMaybeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    m_apvts.state.writeToStream(mos);
}

void TinyTerrorMaybeAudioProcessor::postSetStateInformation()
{
    m_apvts.replaceState(magicState.getValueTree());
    TinyTerrirParams settings = getSettings();

    tinyTerror.reset(os.getOversamplingFactor() * getSampleRate());
    tinyTerror.setParameters(settings);
}

//void TinyTerrorMaybeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
//{
//    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
//
//    if (tree.isValid()) {
//        m_apvts.replaceState(tree);
//
//        TinyTerrirParams settings = getSettings();
//
//        tinyTerror.reset(os.getOversamplingFactor() * getSampleRate());
//        tinyTerror.setParameters(settings);
//
//    }
//}

TinyTerrirParams TinyTerrorMaybeAudioProcessor::getSettings()
{
    TinyTerrirParams ttp;

    ttp.ampGainStyle = static_cast<ampGainStructure>(m_apvts.getRawParameterValue("Gain Level")->load());
    ttp.volume1_010 = m_apvts.getRawParameterValue("Input Gain")->load();
    ttp.volume2_010 = m_apvts.getRawParameterValue("Preamp Gain")->load();
    ttp.inputHPF_010 = m_apvts.getRawParameterValue("Input high pass")->load();
    ttp.midBandPassGain = m_apvts.getRawParameterValue("Shape")->load();
    ttp.tubeCompression_010 = m_apvts.getRawParameterValue("Tube Compression")->load();
    ttp.masterVolume_010 = m_apvts.getRawParameterValue("Master volume")->load();

    return ttp;
}

juce::AudioProcessorValueTreeState::ParameterLayout TinyTerrorMaybeAudioProcessor::CreateParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    //double volume1_010 = 0.0;
    //double volume2_010 = 0.0;
    //double inputHPF_010 = 0.0;
    //double tubeCompression_010 = 0.0;
    //double masterVolume_010 = 0.0;
    //double midBandPassGain = 0.0;
    //
    // gain style

    layout.add(std::make_unique<juce::AudioParameterFloat>("Input Gain",
                                                           "Input Gain",
                                                           juce::NormalisableRange<float>(0.0, 10.0, 0.2),
                                                           0.5));

    const juce::StringArray gainStyles {"Low Gain", "Medium Gain", "High Gain"};
    layout.add(std::make_unique<juce::AudioParameterChoice>("Gain Level",
                                                            "Gain Level",
                                                            gainStyles,
                                                            1));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Preamp Gain",
                                                            "Preamp Gain",
                                                            juce::NormalisableRange<float>(0.0, 10.0, 0.2),
                                                            0.5));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Input high pass",
                                                            "Input high pass",
                                                            juce::NormalisableRange<float>(0.0, 10.0, 0.2),
                                                            0.5));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Shape",
                                                            "Shape",
                                                            juce::NormalisableRange<float>(0.0, 10.0, 0.2),
                                                            0.5));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Tube Compression",
                                                            "Tube Compression",
                                                            juce::NormalisableRange<float>(0.0, 10.0, 0.2),
                                                            0.5));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Master volume",
                                                            "Master volume",
                                                            juce::NormalisableRange<float>(0.0, 10.0, 0.2),
                                                            0.5));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TinyTerrorMaybeAudioProcessor();
}
