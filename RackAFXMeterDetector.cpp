#include "RackAFXMeterDetector.h"

CRackAFXMeterDetector::CRackAFXMeterDetector(void)
{
	m_fAttackTime_mSec = 0.0;
	m_fReleaseTime_mSec = 0.0;
	m_fAttackTime = 0.0;
	m_fReleaseTime = 0.0;
	m_fSampleRate = 44100;
	m_fEnvelope = 0.0;
	m_uDetectMode = 0;
	m_nSample = 0;
	m_bAnalogTC = false;
	m_bLogDetector = false;
}

CRackAFXMeterDetector::~CRackAFXMeterDetector(void)
{
}

void CRackAFXMeterDetector::prepareForPlay()
{
	m_fEnvelope = 0.0;
	m_nSample = 0;
}

void CRackAFXMeterDetector::init(float samplerate, float attack_in_ms, float release_in_ms, bool bAnalogTC, UINT uDetect, bool bLogDetector)
{
	m_fEnvelope = 0.0;
	m_fSampleRate = samplerate;
	m_bAnalogTC = bAnalogTC;
	m_fAttackTime_mSec = attack_in_ms;
	m_fReleaseTime_mSec = release_in_ms;
	m_uDetectMode = uDetect;
	m_bLogDetector = bLogDetector;

	// set themm_uDetectMode = uDetect;
	setAttackTime(attack_in_ms);
	setReleaseTime(release_in_ms);
}

void CRackAFXMeterDetector::setAttackTime(float attack_in_ms)
{
	m_fAttackTime_mSec = attack_in_ms;

	if(m_bAnalogTC)
		m_fAttackTime = exp(ANALOG_TC/( attack_in_ms * m_fSampleRate * 0.001));
	else
		m_fAttackTime = exp(DIGITAL_TC/( attack_in_ms * m_fSampleRate * 0.001));
}

void CRackAFXMeterDetector::setReleaseTime(float release_in_ms)
{
	m_fReleaseTime_mSec = release_in_ms;

	if(m_bAnalogTC)
		m_fReleaseTime = exp(ANALOG_TC/( release_in_ms * m_fSampleRate * 0.001));
	else
		m_fReleaseTime = exp(DIGITAL_TC/( release_in_ms * m_fSampleRate * 0.001));
}

void CRackAFXMeterDetector::setTCModeAnalog(bool bAnalogTC)
{
	m_bAnalogTC = bAnalogTC;
	setAttackTime(m_fAttackTime_mSec);
	setReleaseTime(m_fReleaseTime_mSec);
}


float CRackAFXMeterDetector::detect(float fInput)
{
	switch(m_uDetectMode)
	{
	case 0:
		fInput = fabs(fInput);
		break;
	case 1:
		fInput = fabs(fInput) * fabs(fInput);
		break;
	case 2:
		fInput = pow((float)fabs(fInput) * (float)fabs(fInput), (float)0.5);
		break;
	default:
		fInput = (float)fabs(fInput);
		break;
	}

	float fOld = m_fEnvelope;
	if(fInput> m_fEnvelope)
		m_fEnvelope = m_fAttackTime * (m_fEnvelope - fInput) + fInput;
	else
		m_fEnvelope = m_fReleaseTime * (m_fEnvelope - fInput) + fInput;

	if(m_fEnvelope > 0.0 && m_fEnvelope < FLT_MIN_PLUS) m_fEnvelope = 0;
	if(m_fEnvelope < 0.0 && m_fEnvelope > FLT_MIN_MINUS) m_fEnvelope = 0;
	
	// bound them; can happen when using pre-detector gains of more than 1.0
	m_fEnvelope = fmin(m_fEnvelope, 1.0);
	m_fEnvelope = fmax(m_fEnvelope, 0.0);

	// NOTE!!!!!! 16-bit scaling!
	if(m_bLogDetector)
	{
		if(m_fEnvelope <= 0)
			return 0;

		float fdB = 20*log10(m_fEnvelope);
		fmax(METER_MIN_DB, fdB);

		fdB += -METER_MIN_DB;
		return fdB/-METER_MIN_DB;
	}

	return m_fEnvelope;
}
