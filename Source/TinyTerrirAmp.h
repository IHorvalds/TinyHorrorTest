/*
  ==============================================================================

    TinyTerrirAmp.h
    Created: 9 Jul 2021 3:04:39pm
    Author:  ihorv

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VAObjects/valves.h"

//==============================================================================
/*
*/

using Filter = juce::dsp::IIR::Filter<double>;
using Coefficients = Filter::CoefficientsPtr;

struct TinyTerrirParams {
	TinyTerrirParams() {}

	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	TinyTerrirParams& operator=(const TinyTerrirParams& params)	// need this override for collections to work
	{
		// --- it is possible to try to make the object equal to itself
		//     e.g. thisObject = thisObject; so this code catches that
		//     trivial case and just returns this object
		if (this == &params)
			return *this;

		// --- copy from params (argument) INTO our variables
		volume1_010 = params.volume1_010;
		volume2_010 = params.volume2_010;
		inputHPF_010 = params.inputHPF_010;

		masterVolume_010 = params.masterVolume_010;
		tubeCompression_010 = params.tubeCompression_010;

		midBandPassGain = params.midBandPassGain;

		ampGainStyle = params.ampGainStyle;

		for (int i = 0; i < PREAMP_TRIODES; i++)
			dcShift[i] = params.dcShift[i];

		// --- MUST be last
		return *this;
	}

	// --- amp controls are all 0 -> 10
	double volume1_010 = 0.0;
	double volume2_010 = 0.0;
	double inputHPF_010 = 0.0;
	double tubeCompression_010 = 0.0;
	double masterVolume_010 = 0.0;
	double midBandPassGain = 0.0;

	// --- for monitoring preamp tube DC shifts
	double dcShift[PREAMP_TRIODES] = { 0.0, 0.0, 0.0, 0.0 };

	ampGainStructure ampGainStyle = ampGainStructure::medium;
};

class TinyTerrirAmp
{
public:
    TinyTerrirAmp();
    ~TinyTerrirAmp();

    enum ValvePositions {
        T1, T2, T3, T4
    };


    bool reset(double sampleRate);
	double processAudioSample(double xn);

	TinyTerrirParams getParameters();
	void setParameters(const TinyTerrirParams& params);

private:
	TinyTerrirParams ttp;

    ClassAValve ecc83[4];
    ClassBValvePair pentodes;


    double sampleRate = 0.0;

    double inputGain = 1.0;
    double driveGain = 5;
    double volume = 7;
    double tubeCompress = 6;
    double outputGain = 5;

    AudioFilter outputLPF;
    AudioFilter inputHPF;
    AudioFilter outputHPF;
	AudioFilter midBandPass;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TinyTerrirAmp)
};
