#include "HybridSynthVoice.h"

CHybridSynthVoice::CHybridSynthVoice(void)
{
	// oscillators
	m_pOsc1 = &m_Op1;
	m_pOsc2 = &m_Op2;
	m_pOsc3 = &m_Op3;
	m_pOsc4 = &m_Op4;
	m_pNoiseGen = &m_NoiseGen;

	// filters
	m_pFilter1 = &m_MoogLadderFilter;
	m_pFilter2 = NULL;
    	
	// for passband gain comp in MOOG; can make user adjustable, the higher m_dAuxControl, the more passband gain 
	//     0 <= m_dAuxControl <= 1
	m_MoogLadderFilter.m_dAuxControl = 0.0; 

	// clear our new variables
	m_dOp1Feedback = 0.0;
	m_dOp2Feedback = 0.0;
	m_dOp3Feedback = 0.0;
	m_dOp4Feedback = 0.0;
}

void CHybridSynthVoice::initializeModMatrix(CModulationMatrix* pMatrix)
{
	CVoice::initializeModMatrix(pMatrix);

	if(!pMatrix->getModMatrixCore()) return;

	//     DX SYNTH SPECIFIC MOD MATRIX - different from the others because only uses a 
	//     singe LFO Intensity control for all LFO mod routings (demonstratig an alternate
	//     way to do this)
	//
	//     these are also OFF by default but you can easily allow the user 
	//     to enable/disable
	modMatrixRow* pRow = NULL;

	// EG1 -> FILTER1 FC
	pRow = createModMatrixRow(SOURCE_BIASED_FILTER_EG,
							  DEST_ALL_FILTER_FC,
							  &m_pGlobalVoiceParams->dFilterEGFilter1ModIntensity,
							  &m_pGlobalVoiceParams->dFilterModRange,
							  TRANSFORM_NONE,
							  true);
	pMatrix->addModMatrixRow(pRow);

	// LFO1 -> DEST_OSC1_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_OSC1_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);
	
	// LFO1 -> DEST_OSC1_FO (Vibrato)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_OSC1_FO,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_pGlobalVoiceParams->dOscFoModRange, // this is used for vibrato
							  TRANSFORM_NONE,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);


	// LFO1 -> DEST_OSC2_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_OSC2_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);
	
	// LFO1 -> DEST_OSC2_FO (Vibrato)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_OSC2_FO,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_pGlobalVoiceParams->dOscFoModRange, // this is used for vibrato
							  TRANSFORM_NONE,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);


	// LFO1 -> DEST_OSC3_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_OSC3_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);
	
	// LFO1 -> DEST_OSC3_FO (Vibrato)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_OSC3_FO,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_pGlobalVoiceParams->dOscFoModRange, // this is used for vibrato
							  TRANSFORM_NONE,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);

	// LFO1 -> DEST_OSC4_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_OSC4_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);
	
	// LFO1 -> DEST_OSC4_FO (Vibrato)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_OSC4_FO,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_pGlobalVoiceParams->dOscFoModRange, // this is used for vibrato
							  TRANSFORM_NONE,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);

	// LFO1 -> DEST_NOISE_GEN_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_NOISE_GEN_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO1OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);

	// LFO1 -> FILTER1 FC
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_ALL_FILTER_FC,
							  &m_pGlobalVoiceParams->dLFO1Filter1ModIntensity,
							  &m_pGlobalVoiceParams->dFilterModRange,
							  TRANSFORM_NONE,
							  true); 
	pMatrix->addModMatrixRow(pRow);

	// LFO1 (-1 -> +1) -> DCA Amp Mod (0->1)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_DCA_AMP,
							  &m_pGlobalVoiceParams->dLFO1DCAAmpModIntensity,
							  &m_pGlobalVoiceParams->dAmpModRange,
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  true); 
	pMatrix->addModMatrixRow(pRow);

	// LFO1 (-1 -> +1) -> DCA Pan Mod (-1->1)
	pRow = createModMatrixRow(SOURCE_LFO1,
							  DEST_DCA_PAN,
							  &m_pGlobalVoiceParams->dLFO1DCAPanModIntensity,
							  &m_dDefaultModRange,
							  TRANSFORM_NONE,
							  true); 
	pMatrix->addModMatrixRow(pRow);

	// LFO2 -> DEST_OSC1_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_OSC1_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);
	
	// LFO2 -> DEST_OSC1_FO (Vibrato)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_OSC1_FO,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_pGlobalVoiceParams->dOscFoModRange, // this is used for vibrato
							  TRANSFORM_NONE,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);


	// LFO2 -> DEST_OSC2_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_OSC2_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);
	
	// LFO2 -> DEST_OSC2_FO (Vibrato)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_OSC2_FO,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_pGlobalVoiceParams->dOscFoModRange, // this is used for vibrato
							  TRANSFORM_NONE,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);


	// LFO2 -> DEST_OSC3_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_OSC3_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);
	
	// LFO2 -> DEST_OSC3_FO (Vibrato)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_OSC3_FO,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_pGlobalVoiceParams->dOscFoModRange, // this is used for vibrato
							  TRANSFORM_NONE,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);

	// LFO2 -> DEST_OSC4_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_OSC4_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);
	
	// LFO2 -> DEST_OSC4_FO (Vibrato)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_OSC4_FO,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_pGlobalVoiceParams->dOscFoModRange, // this is used for vibrato
							  TRANSFORM_NONE,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);

	// LFO2 -> DEST_NOISE_GEN_OUTPUT_AMP (Amplitude Modulation)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_NOISE_GEN_OUTPUT_AMP,
							  &m_pGlobalVoiceParams->dLFO2OscModIntensity,
							  &m_dDefaultModRange, // this is used for AM not tremolo
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  false); /* DISABLED BY DEFAULT */
	pMatrix->addModMatrixRow(pRow);

	// LFO2 -> FILTER1 FC
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_ALL_FILTER_FC,
							  &m_pGlobalVoiceParams->dLFO2Filter1ModIntensity,
							  &m_pGlobalVoiceParams->dFilterModRange,
							  TRANSFORM_NONE,
							  true); 
	pMatrix->addModMatrixRow(pRow);

	// LFO2 (-1 -> +1) -> DCA Amp Mod (0->1)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_DCA_AMP,
							  &m_pGlobalVoiceParams->dLFO2DCAAmpModIntensity,
							  &m_pGlobalVoiceParams->dAmpModRange,
							  TRANSFORM_BIPOLAR_TO_UNIPOLAR,
							  true); 
	pMatrix->addModMatrixRow(pRow);

	// LFO2 (-1 -> +1) -> DCA Pan Mod (-1->1)
	pRow = createModMatrixRow(SOURCE_LFO2,
							  DEST_DCA_PAN,
							  &m_pGlobalVoiceParams->dLFO2DCAPanModIntensity,
							  &m_dDefaultModRange,
							  TRANSFORM_NONE,
							  true); 
	pMatrix->addModMatrixRow(pRow);
}

CHybridSynthVoice::~CHybridSynthVoice(void)
{
}

void CHybridSynthVoice::setSampleRate(double dSampleRate)
{
	CVoice::setSampleRate(dSampleRate);
}

void CHybridSynthVoice::prepareForPlay()
{
	CVoice::prepareForPlay();
	reset();
}

void CHybridSynthVoice::reset()
{
	CVoice::reset();
	m_dPortamentoInc = 0.0;
}

void CHybridSynthVoice::update()
{
	// voice specific updates
	if(!m_pGlobalVoiceParams) return;

	CVoice::update();

	// update feedback
	m_dOp1Feedback = m_pGlobalVoiceParams->dOp1Feedback;
	m_dOp2Feedback = m_pGlobalVoiceParams->dOp2Feedback;		
	m_dOp3Feedback = m_pGlobalVoiceParams->dOp3Feedback;
	m_dOp4Feedback = m_pGlobalVoiceParams->dOp4Feedback;
}