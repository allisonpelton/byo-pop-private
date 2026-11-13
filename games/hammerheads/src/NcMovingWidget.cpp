#include "NcMovingWidget.h"

#include <cmath>

#include "Application.h"
#include "DrawUtil.h"

using namespace Sexy;

namespace
{

// What a page that nobody closed answers (the same value as NcMovingDialog's)
const int NO_CLOSE_RESULT = (int) 0xc4653601;

// How far below the screen a page that is leaving has to get before it counts as gone
const int LEAVE_MARGIN = 100;

}

// 0x468c70
NcMovingWidget::NcMovingWidget(int theHeight)
{
	mRestY = (gApp->mHeight - theHeight) / 2;
	mCloseResult = NO_CLOSE_RESULT;
	mLeaving = false;
	// the page starts one height above the screen: the fall to its resting place is a quarter of the speed squared
	mSlide = -sqrt((double) (mRestY + theHeight) * 4.0);
}

// 0x468ce0: down from above while the speed is negative, and, once it has been told to leave, down again by the square of the
// speed growing the other way
void NcMovingWidget::UpdateF(float theFrac)
{
	Widget::UpdateF(theFrac);

	if (mSlide < 0.0)
	{
		double aSlide = theFrac + mSlide;
		if (!(aSlide < 0.0))
			aSlide = 0.0;
		mSlide = aSlide;
		Move(mX, ToInt((double) mRestY - aSlide * aSlide * 0.25));
		return;
	}

	if (mSlide > 0.0 || mLeaving)
	{
		if (ToInt(mSlide * mSlide + (double) mRestY) >= gApp->mHeight + LEAVE_MARGIN)
		{
			Closed();
			return;
		}
		mSlide = theFrac + mSlide;
		Move(mX, ToInt(mSlide * mSlide + (double) mRestY));
	}
}
