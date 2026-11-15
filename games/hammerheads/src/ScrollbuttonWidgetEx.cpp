#include "ScrollbuttonWidgetEx.h"

#include "Graphics.h"
#include "Image.h"

using namespace Sexy;

// 0x405a90
ScrollbuttonWidgetEx::ScrollbuttonWidgetEx(int theId, Image* theImage, ButtonListener* theButtonListener) :
	ScrollbuttonWidget(theId, theButtonListener, 0)
{
	mImage = theImage;
}

// 0x405af0
void ScrollbuttonWidgetEx::Draw(Graphics* g)
{
	if (mImage == nullptr)
	{
		ScrollbuttonWidget::Draw(g);
		return;
	}

	bool aHeld = mIsDown && mIsOver && !mDisabled;
	Rect aSource(0, 0, mImage->GetWidth(), mImage->GetHeight());
	if (mInverted == aHeld)
		g->DrawImage(mImage, Rect(0, 0, mWidth, mHeight), aSource);
	else
		g->DrawImage(mImage, Rect(1, 1, mWidth, mHeight), aSource);
}
