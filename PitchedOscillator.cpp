#include "PitchedOscillator.h"

CPitchedOscillator::CPitchedOscillator(void)
{
}

CPitchedOscillator::~CPitchedOscillator(void)
{
}

void CPitchedOscillator::reset()
{
	COscillator::reset();

	// --- saw/tri starts at 0.5
	if(m_uWaveform == SAW || m_uWaveform == TRI)
	{
		m_dModulo = 0.5;
	}
}

void CPitchedOscillator::startOscillator()
{
	reset();
	m_bNoteOn = true;
}

void CPitchedOscillator::stopOscillator()
{
	m_bNoteOn = false;
}
