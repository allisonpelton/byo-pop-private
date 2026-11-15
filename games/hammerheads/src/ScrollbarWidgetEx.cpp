#include "ScrollbarWidgetEx.h"

#include "Graphics.h"
#include "Image.h"
#include "ScrollListener.h"
#include "ScrollbuttonWidgetEx.h"
#include "SexyAppBase.h"

using namespace Sexy;

// 0x405550: the framework makes the two buttons; they are replaced at once by the game's, which hold the picture
ScrollbarWidgetEx::ScrollbarWidgetEx(int theId, Image* theTrackImage, Image* theThumbImage, Image* theButtonImage,
	ScrollListener* theScrollListener) :
	ScrollbarWidget(theId, theScrollListener)
{
	mThumbImage = theThumbImage;
	mTrackImage = theTrackImage;

	if (mUpButton != nullptr)
	{
		if (mUpButton->mParent != nullptr)
			RemoveWidget(mUpButton);
		gSexyAppBase->SafeDeleteWidget(mUpButton);
	}
	mUpButton = new ScrollbuttonWidgetEx(0, theButtonImage, this);
	mUpButton->mButtonImage = theButtonImage;
	mUpButton->SetDisabled(true);
	AddWidget(mUpButton);

	if (mDownButton != nullptr)
	{
		if (mDownButton->mParent != nullptr)
			RemoveWidget(mDownButton);
		gSexyAppBase->SafeDeleteWidget(mDownButton);
	}
	mDownButton = new ScrollbuttonWidgetEx(1, theButtonImage, this);
	mDownButton->mButtonImage = theButtonImage;
	mDownButton->SetDisabled(true);
	AddWidget(mDownButton);
}

// 0x405730
void ScrollbarWidgetEx::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	ScrollbarWidget::MouseUp(x, y, theBtnNum, theClickCount);
	mScrollListener->ScrollPosition(mId, mValue);
	mDirty = true;
}

// 0x405960
void ScrollbarWidgetEx::DrawThumb(Graphics* g, int theX, int theY, int theWidth, int theHeight)
{
	g->DrawImageBox(Rect(theX, theY, theWidth, theHeight), mThumbImage);
}

// 0x4059d0
void ScrollbarWidgetEx::Draw(Graphics* g)
{
	int aThumbSize = GetThumbSize();
	int aThumbPosition = GetThumbPosition();

	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mTrackImage);
	if (!mHorizontal)
	{
		if (aThumbSize > 0)
			DrawThumb(g, 0, aThumbPosition, mWidth, aThumbSize);
	}
	else if (aThumbSize > 0)
		DrawThumb(g, aThumbPosition, 0, aThumbSize, mHeight);
}
