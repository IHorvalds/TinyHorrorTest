/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "TinyTerrirAmp.h"
#include "foleys_gui_magic/foleys_gui_magic.h"

//==============================================================================
/**
*/

using Oversampler = juce::dsp::Oversampling<float>;
using Waveshaper = juce::dsp::WaveShaper<float, std::function<float(float)>>;

class TinyTerrorMaybeAudioProcessor  : public foleys::MagicProcessor
{
public:
    //==============================================================================
    TinyTerrorMaybeAudioProcessor();
    ~TinyTerrorMaybeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;


    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void postSetStateInformation() override;

    TinyTerrirParams getSettings();

    static juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout();
    juce::AudioProcessorValueTreeState m_apvts{ *this, nullptr, "Parameters", CreateParameterLayout() };

private:


    Oversampler os { 2, 3, Oversampler::FilterType::filterHalfBandPolyphaseIIR };
    Waveshaper ws;
    TinyTerrirAmp tinyTerror;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TinyTerrorMaybeAudioProcessor)
};
