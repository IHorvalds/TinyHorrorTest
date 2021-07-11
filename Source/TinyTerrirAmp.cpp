/*
  ==============================================================================

    TinyTerrirAmp.cpp
    Created: 9 Jul 2021 3:04:39pm
    Author:  ihorv

  ==============================================================================
*/

#include <JuceHeader.h>
#include "TinyTerrirAmp.h"

//==============================================================================
TinyTerrirAmp::TinyTerrirAmp()
{}

TinyTerrirAmp::~TinyTerrirAmp()
{}

bool TinyTerrirAmp::reset(double sampleRate)
{
	// --- do any other per-audio-run inits here
	this->sampleRate = sampleRate;

	ecc83[T1].reset(sampleRate);
	ClassAValveParameters triodeParams = ecc83[T1].getParameters();
	triodeParams.lowFrequencyShelf_Hz = 10.0;
	triodeParams.lowFrequencyShelfGain_dB = -10.0;
	triodeParams.integratorFc = 1.0;
	triodeParams.millerHF_Hz = 20000.0;
	triodeParams.dcBlockingLF_Hz = 8.0;
	triodeParams.outputGain = pow(10.0, -3.0 / 20.0);
	triodeParams.dcShiftCoefficient = 1.0;
	ecc83[T1].setParameters(triodeParams);

	ecc83[T2].reset(sampleRate);
	triodeParams = ecc83[T2].getParameters();
	triodeParams.lowFrequencyShelf_Hz = 10.0;
	triodeParams.lowFrequencyShelfGain_dB = -10.0;
	triodeParams.integratorFc = 1.0;
	triodeParams.millerHF_Hz = 9000.0;
	triodeParams.dcBlockingLF_Hz = 32.0;
	triodeParams.outputGain = pow(10.0, +5.0 / 20.0);
	triodeParams.dcShiftCoefficient = 0.20;
	ecc83[T2].setParameters(triodeParams);

	ecc83[T3].reset(sampleRate);
	triodeParams = ecc83[T3].getParameters();
	triodeParams.lowFrequencyShelf_Hz = 10.0;
	triodeParams.lowFrequencyShelfGain_dB = -10.0;
	triodeParams.integratorFc = 1.0;
	triodeParams.millerHF_Hz = 7000.0;
	triodeParams.dcBlockingLF_Hz = 40.0;
	triodeParams.outputGain = pow(10.0, +6.0 / 30.0);
	triodeParams.dcShiftCoefficient = 0.50;
	ecc83[T3].setParameters(triodeParams);

	ecc83[T4].reset(sampleRate);
	triodeParams = ecc83[T4].getParameters();
	triodeParams.lowFrequencyShelf_Hz = 10.0;
	triodeParams.lowFrequencyShelfGain_dB = -10.0;
	triodeParams.integratorFc = 1.0;
	triodeParams.millerHF_Hz = 6400.0;
	triodeParams.dcBlockingLF_Hz = 43.0;
	triodeParams.outputGain = pow(10.0, -20.0 / 30.0);
	triodeParams.dcShiftCoefficient = 0.52;
	ecc83[T4].setParameters(triodeParams);

	pentodes.reset(sampleRate);
	ClassBValveParameters pentodeParams = pentodes.getParameters();
	pentodeParams.algorithm = classBType::pirkle;
	pentodeParams.inputGain = pow(10.0, 20.0 / 20.0);
	pentodeParams.outputGain = pow(10.0, -20.0 / 20.0);
	pentodes.setParameters(pentodeParams);

	inputHPF.reset(sampleRate);
	AudioFilterParameters hpfParams = inputHPF.getParameters();
	hpfParams.algorithm = filterAlgorithm::kHPF2;
	inputHPF.setParameters(hpfParams);

	outputHPF.reset(sampleRate);
	hpfParams = outputHPF.getParameters();
	hpfParams.algorithm = filterAlgorithm::kHPF2;
	hpfParams.fc = 5.0;
	outputHPF.setParameters(hpfParams);

	outputLPF.reset(sampleRate);
	AudioFilterParameters lpfParams = outputLPF.getParameters();
	lpfParams.algorithm = filterAlgorithm::kLPF2;
	lpfParams.fc = 17000.0;
	outputLPF.setParameters(lpfParams);

	midBandPass.reset(sampleRate);
	AudioFilterParameters midPassParams = midBandPass.getParameters();
	midPassParams.algorithm = filterAlgorithm::kBPF2;
	midPassParams.fc = 1200;
	midPassParams.Q = 0.222;
	midPassParams.boostCut_dB = 0.0; // will set the boost/cut on process
	midBandPass.setParameters(midPassParams);

	return true;
}

double TinyTerrirAmp::processAudioSample(double xn)
{
	// --- first triode
	//double t1OutSDS = triodes[0].processAudioSample(0.0);
	//return t1OutSDS;

	// --- remove DC, remove bass 
	double hpfOut = inputHPF.processAudioSample(xn);

	// --- "volume 1" control
	double t1In = hpfOut * inputGain;

	// --- first triode
	double t1Out = ecc83[T1].processAudioSample(t1In);

	// --- add pre-drive

	// Let's try subtractive gain. Max gain => * 1.0, min gain => * 0.2
	t1Out *= (driveGain * 0.5);

	// --- cascade of triodes
	//     NOTE: leaving this verbose so you can experiment, use less or more triodes...
	double t2Out = ecc83[T2].processAudioSample(t1Out);

	// Let's try subtractive gain. Max gain => * 1.0, min gain => * 0.2
	double drive2Out = t2Out * (driveGain * 0.5);

	double t3Out = ecc83[T3].processAudioSample(drive2Out);
	double t4Out = ecc83[T4].processAudioSample(t3Out);

	// --- class B drive gain
	t4Out *= tubeCompress;

	// --- class B model
	double classBOut = pentodes.processAudioSample(t4Out);

	// mid filter
	double filterOut = midBandPass.processAudioSample(classBOut);

	// --- tone stack (note: relocating makes a big difference)
	//double toneStackOut = toneStack.processAudioSample(classBOut);
	double dcBlock = outputHPF.processAudioSample(filterOut);

	dcBlock *= outputGain;

	return dcBlock;
}

TinyTerrirParams TinyTerrirAmp::getParameters()
{
	return ttp;
}

/** set parameters: note use of custom structure for passing param data */
/**
\param ValveEmulatorParameters custom data structure
*/
void TinyTerrirAmp::setParameters(const TinyTerrirParams& params)
{
	ttp = params;

	// --- simulate different gain structures with waveshaper saturation
	double saturation = 1.0;
	if (ttp.ampGainStyle == ampGainStructure::medium)
		saturation = 2.0;
	else if (ttp.ampGainStyle == ampGainStructure::high)
		saturation = 4.0;

	// TODO: experiment with using driveGain as saturation parameter for waveshaper. Quick look: waveshaper is tanh(sat * x)/tanh(sat)

	// --- update
	for (int i = 0; i < 4; i++)
	{
		ClassAValveParameters triodeParams = ecc83[i].getParameters();
		triodeParams.waveshaperSaturation = saturation;
		ecc83[i].setParameters(triodeParams);
	}

	// --- input gain
	if (ttp.volume1_010 == 0.0)
		inputGain = 0.0;
	else
	{
		inputGain = calcMappedVariableOnRange(0.0, 10.0,
												-60.0, +5.0,
												ttp.volume1_010,
												true);
	}

	// --- drive gain
	driveGain = calcMappedVariableOnRange(0.0, 10.0,
										  -2.0, 10.0,
										  ttp.volume2_010,
										  true);

	// --- HPF
	AudioFilterParameters hpfParams = inputHPF.getParameters();
	hpfParams.fc = calcMappedVariableOnRange(0.0, 10.0,
											5.0, 1000.0,
											ttp.inputHPF_010);
	inputHPF.setParameters(hpfParams);

	// --- ToneStack
	AudioFilterParameters midPassParams = midBandPass.getParameters();
	midPassParams.algorithm = filterAlgorithm::kCQParaEQ;
	midPassParams.fc = 1200.0;
	midPassParams.Q = 0.222;
	midPassParams.boostCut_dB = calcMappedVariableOnRange(0.0, 10.0,
														  -14.0, 14.0,
														  ttp.midBandPassGain,
														  true);
	midBandPass.setParameters(midPassParams);

	// --- output gain amount
	outputGain = calcMappedVariableOnRange(0.0, 10.0,
											-60.0, +12.0,
											ttp.masterVolume_010,
											true);

	tubeCompress = calcMappedVariableOnRange(0.0, 10.0,
											-6.0, +10.0,
											ttp.tubeCompression_010,
											true);
}