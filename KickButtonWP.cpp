#include "KickButtonWP.h"

namespace VSTGUI {

CKickButtonWP::CKickButtonWP(const VSTGUI::CRect& size, CControlListener* listener, int32_t tag, CBitmap* background, const CPoint& offset)
: CKickButton(size, listener, tag, background, offset)
{

}
	
//------------------------------------------------------------------------
CMouseEventResult CKickButtonWP::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!(buttons & kLButton))
		return kMouseEventNotHandled;

	value = 1.0;
	fEntryState = value;
	beginEdit ();
	
	if (value)
		valueChanged ();
	//value = getMin ();
	// valueChanged ();
	if (isDirty ())
		invalid ();
	//endEdit ();

	return onMouseMoved (where, buttons);
}

//------------------------------------------------------------------------
CMouseEventResult CKickButtonWP::onMouseUp (CPoint& where, const CButtonState& buttons)
{
	//if (value)
	//	valueChanged ();
	value = getMin ();
	valueChanged ();
	if (isDirty ())
		invalid ();
	endEdit ();
	return kMouseEventHandled;
}



}