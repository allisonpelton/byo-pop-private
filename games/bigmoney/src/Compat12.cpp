#include "Compat12.h"
#include "WidgetManager.h"
#include "SexyAppBase.h"
#include "Image.h"

using namespace Sexy;

// MSVC 7.0 CRT rand.c (_holdrand of the main thread)
static unsigned int gCrtHoldRand = 1;

void Sexy::CrtSRand(unsigned int theSeed)
{
	gCrtHoldRand = theSeed;
}

int Sexy::CrtRand()
{
	gCrtHoldRand = gCrtHoldRand * 214013u + 2531011u;
	return (int) ((gCrtHoldRand >> 16) & 0x7fff);
}

Slider12::Slider12(Image* theTrackImage, Image* theThumbImage, int theId, SliderListener* theListener) :
	Slider(theTrackImage, theThumbImage, theId, theListener)
{
}

// SexyAppFramework 1.2 Slider::SetValue
void Slider12::SetValue(double theValue)
{
	mVal = theValue;
	MarkDirtyFull();
}

// SexyAppFramework 1.2 Slider::MouseDown: only a press on the thumb starts a drag
void Slider12::MouseDown(int x, int y, int theClickCount)
{
	int aThumbX = (int) (mVal * (mWidth - mThumbImage->GetWidth()));

	if ((x >= aThumbX) && (x < aThumbX + mThumbImage->GetWidth()))
	{
		mWidgetManager->mApp->SetCursor(CURSOR_DRAGGING);
		mDragging = true;
		mRelX = x - aThumbX;
	}
}
