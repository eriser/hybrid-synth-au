#include "VuMeterWP.h"

namespace VSTGUI {

CVuMeterWP::CVuMeterWP(const CRect& size, CBitmap* onBitmap, CBitmap* offBitmap, int32_t nbLed, bool bInverted, bool bAnalogVU, int32_t style)
: CVuMeter(size, onBitmap, offBitmap, nbLed, style)
{
	m_bInverted = bInverted;
	m_bAnalogVU = bAnalogVU;
	subPixMaps = 80;
	heightOfOneImage = 65;
	m_dZero_dB_Frame = 52;
}

CVuMeterWP::~CVuMeterWP(void)
{
}

void CVuMeterWP::draw(CDrawContext *_pContext)
{
	if (!getOnBitmap())
		return;

	if(!m_bAnalogVU)
	{
		CRect _rectOn (rectOn);
		CRect _rectOff (rectOff);
		CPoint pointOn;
		CPoint pointOff;
		CDrawContext *pContext = _pContext;

		bounceValue ();
		
		float newValue = getOldValue () - decreaseValue;
		if (newValue < value)
			newValue = value;
		setOldValue (newValue);

		//VSTGUI::CBitmap* p = getOnBitmap();

		if (style & kHorizontal) 
		{
			CCoord tmp = (CCoord)(((int32_t)(nbLed * (getMin () + newValue) + 0.5f) / (float)nbLed) * getOnBitmap()->getWidth ());
			pointOff (tmp, 0);

			_rectOff.left += tmp;
			_rectOn.right = tmp + rectOn.left;
		}
		else 
		{
			CCoord tmp = (CCoord)(((int32_t)(nbLed * (getMax () - newValue) + 0.5f) / (float)nbLed) * getOnBitmap()->getHeight ());
			pointOn (0, tmp);

			_rectOff.bottom = tmp + rectOff.top;
			_rectOn.top     += tmp;
		}

		if(m_bInverted)
		{
			//CRect temp(_rectOn);

			int h = _rectOn.height();
			_rectOn.top = _rectOff.top;
			_rectOn.bottom = _rectOn.top  + h;

			h = _rectOff.height();
			_rectOff.top = _rectOn.bottom;
			_rectOff.bottom = _rectOff.top  + h;

			pointOn.v = _rectOff.top;
			pointOn.y = _rectOff.top;
			
			if (getOffBitmap())
			{
				getOffBitmap()->draw (pContext, _rectOff, pointOn);
			}
			
			getOnBitmap()->draw(pContext, _rectOn, pointOff);

		}
		else
		{
			if (getOffBitmap())
			{
				getOffBitmap()->draw (pContext, _rectOff, pointOff);
			}

			getOnBitmap()->draw(pContext, _rectOn, pointOn);
		}
	}
	else
	{
		if(getDrawBackground ())
		{
			CPoint where (0, 0);
			if (value >= 0.f && heightOfOneImage > 0.) 
			{
			//	CCoord tmp = heightOfOneImage * (getNumSubPixmaps() - 1);
				CCoord tmp = heightOfOneImage * (subPixMaps - 1);
				if(m_bInverted)
				{
					double dTop = m_dZero_dB_Frame/subPixMaps;
					where.v = floor ((dTop - value*dTop) * tmp);
				}
				else
					where.v = floor (value * tmp);
				where.v -= (int32_t)where.v % (int32_t)heightOfOneImage;
			}

			getDrawBackground()->draw (_pContext, getViewSize (), where);
		}
	}	

	setDirty (false);
}


}