#include "NoiseGenerator.h"

CNoiseGenerator::CNoiseGenerator(void)
{
}

CNoiseGenerator::~CNoiseGenerator(void)
{
}

void CNoiseGenerator::reset()
{
	COscillator::reset();
}

void CNoiseGenerator::startOscillator()
{
	reset();
	m_bNoteOn = true;
}

void CNoiseGenerator::stopOscillator()
{
	m_bNoteOn = false;
}