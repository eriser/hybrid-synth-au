#pragma once
#ifndef __NOISEGENERATOR_H
#define __NOISEGENERATOR_H

#include "oscillator.h"

class CNoiseGenerator : public COscillator
{
public:
	CNoiseGenerator(void);
	~CNoiseGenerator(void);

	// init globals
	inline virtual void initGlobalParameters(globalOscillatorParams* pGlobalOscParams)
	{
		COscillator::initGlobalParameters(pGlobalOscParams);
	}

	// virtual overrides
	virtual void reset();
	virtual void startOscillator();
	virtual void stopOscillator();

	// the rendering function
	virtual inline double doOscillate(double* pAuxOutput = NULL)
	{
		if(!m_bNoteOn)
			return 0.0;

		double dOut = 0.0;

		// always first
		bool bWrap = checkWrapModulo();

		// added for PHASE MODULATION
		double dCalcModulo = m_dModulo + m_dPhaseMod;
			checkWrapIndex(dCalcModulo);

		switch(m_uWaveform)
		{
            case WHITE:
			{
				// use helper function
				dOut = doWhiteNoise();

				break;
			}

			case PINK:
			{
				// use helper function
				dOut = doPNSequence(m_uPNRegister);

				break;
			}
			default:
				break;
		}

		// ok to inc modulo now
		incModulo();

		// write to outputs
		if(m_pModulationMatrix)
		{
			// write our outputs into their destinations
			m_pModulationMatrix->m_dSources[m_uModDestOutput1] = dOut*m_dAmplitude*m_dAmpMod;

			// add quad phase/stereo output (QBL is mono)
			m_pModulationMatrix->m_dSources[m_uModDestOutput2] = dOut*m_dAmplitude*m_dAmpMod;
		}

		// m_dAmpMod is set in update()
		if(pAuxOutput)
			*pAuxOutput = dOut*m_dAmplitude*m_dAmpMod;;

		// m_dAmpMod is set in update()
		return dOut*m_dAmplitude*m_dAmpMod;
	}
};
#endif

