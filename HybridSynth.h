#pragma once
#ifndef __HYBRIDSYNTH_H
#define __HYBRIDSYNTH_H


#include "Plugin.h"
#include "HybridSynthVoice.h"

#define MAX_VOICES 8
	
class CHybridSynth : public CPlugIn
{
public:
	CHybridSynth();
	virtual ~CHybridSynth(void);

	virtual bool __stdcall prepareForPlay();

	virtual bool __stdcall processAudioFrame(float* pInputBuffer, float* pOutputBuffer, UINT uNumInputChannels, UINT uNumOutputChannels);

	virtual bool __stdcall userInterfaceChange(int nControlIndex);

	virtual bool __stdcall initialize();

	virtual bool __stdcall joystickControlChange(float fControlA, float fControlB, float fControlC, float fControlD, float fACMix, float fBDMix);

	virtual bool __stdcall processRackAFXAudioBuffer(float* pInputBuffer, float* pOutputBuffer, UINT uNumInputChannels, UINT uNumOutputChannels, UINT uBufferSize);

	virtual bool __stdcall processVSTAudioBuffer(float** inBuffer, float** outBuffer, UINT uNumChannels, int inFramesToProcess);

	virtual bool __stdcall midiNoteOn(UINT uChannel, UINT uMIDINote, UINT uVelocity);

	virtual bool __stdcall midiNoteOff(UINT uChannel, UINT uMIDINote, UINT uVelocity, bool bAllNotesOff);

	virtual bool __stdcall midiModWheel(UINT uChannel, UINT uModValue);

	virtual bool __stdcall midiPitchBend(UINT uChannel, int nActualPitchBendValue, float fNormalizedPitchBendValue);

	virtual bool __stdcall midiClock();

	virtual bool __stdcall midiMessage(unsigned char cChannel, unsigned char cStatus, unsigned char cData1, unsigned char cData2);

	virtual bool __stdcall initUI();

	// array of MAX_VOICES polyphonic voices
	CHybridSynthVoice* m_pVoiceArray[MAX_VOICES];
	
	// modulation matrix
	CModulationMatrix m_GlobalModMatrix;

	// global parameters
	globalSynthParams m_GlobalSynthParams;

	// helper functions for note on/off/voice steal
	void incrementVoiceTimestamps();
	CHybridSynthVoice* getOldestVoice();
	CHybridSynthVoice* getOldestVoiceWithNote(UINT uMIDINote);

	// update all voices
	void update();

	// for portamento
	double m_dLastNoteFrequency;

	// our recieve channel
	UINT m_uMidiRxChannel;

	// ADDED BY RACKAFX -- DO NOT EDIT THIS CODE!!! ----------------------------------- //
	//  **--0x07FD--**

	UINT m_uLFO1Waveform;
	enum{sine,tri,usaw,dsaw,square,rsh};
	double m_dLFO1Rate;
	UINT m_uLFO2Waveform;
	double m_dLFO2Rate;
	double m_dOp1Ratio;
	double m_dEG1Attack_mSec;
	double m_dEG1Decay_mSec;
	double m_dEG1SustainLevel;
	double m_dEG1Release_mSec;
	double m_dOp1OutputLevel;
	double m_dFilterEGAttack_mSec;
	double m_dFilterEGDecay_mSec;
	double m_dFilterEGSustainLevel;
	double m_dFilterEGRelease_mSec;
	double m_dOp2Ratio;
	double m_dEG2Attack_mSec;
	double m_dEG2Decay_mSec;
	double m_dEG2SustainLevel;
	double m_dEG2Release_mSec;
	double m_dOp2OutputLevel;
	double m_dNoiseGenEGAttack_mSec;
	double m_dNoiseGenEGDecay_mSec;
	double m_dNoiseGenEGSustainLevel;
	double m_dNoiseGenEGRelease_mSec;
	double m_dOp3Ratio;
	double m_dEG3Attack_mSec;
	double m_dEG3Decay_mSec;
	double m_dEG3SustainLevel;
	double m_dEG3Release_mSec;
	double m_dOp3OutputLevel;
	double m_dFilterFcControl;
	double m_dFilterQControl;
	double m_dNoiseGenOutputLevel;
	double m_dOp4Feedback;
	double m_dOp4Ratio;
	double m_dEG4Attack_mSec;
	double m_dEG4Decay_mSec;
	double m_dEG4SustainLevel;
	double m_dEG4Release_mSec;
	double m_dOp4OutputLevel;
	UINT m_uVoiceMode;
	enum{ALG1,ALG2,ALG3,ALG4,ALG5,ALG6,ALG7,ALG8};
	double m_dVolume_dB;
	double m_dPortamentoTime_mSec;
	UINT m_uLegatoMode;
	enum{OFF,ON};
	UINT m_uResetToZero;
	int m_nPitchBendRange;
	double m_dLFO1Intensity;
	double m_dLFO2Intensity;
	UINT m_uLFO1ModDest1;
	enum{None,AmpMod,Vibrato};
	UINT m_uLFO1ModDest2;
	UINT m_uLFO1ModDest3;
	UINT m_uLFO1ModDest4;
	UINT m_uLFO2ModDest1;
	UINT m_uLFO2ModDest2;
	UINT m_uLFO2ModDest3;
	UINT m_uLFO2ModDest4;
	UINT m_uNoiseGenWaveform;
	enum{WHITE,PINK};
	UINT m_uLFO1ModDestNoiseAmp;
	UINT m_uLFO2ModDestNoiseAmp;
	double m_dLFO1PanIntensity;
	double m_dLFO2PanIntensity;
	double m_dLFO1AmpIntensity;
	double m_dLFO2AmpIntensity;
	double m_dLFO1FCutIntensity;
	double m_dLFO2FCutIntensity;
	UINT m_uFilterTypeControl;
	enum{LPF2,LPF4,BPF2,BPF4,HPF2,HPF4};
	double m_dFilterEGModIntensityControl;
	double m_dPanControl;
	UINT m_uOp1Waveform;
	enum{SINE,TRI,SAW,SQUARE};
	UINT m_uOp2Waveform;
	UINT m_uOp3Waveform;
	UINT m_uOp4Waveform;

	// **--0x1A7F--**
	// ------------------------------------------------------------------------------- //

};








































































































































































































































































#endif


