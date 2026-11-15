#include "Scrollbar.h"
#include "ScrollListener.h"
#include "Graphics.h"
#include "Image.h"

using namespace Sexy;

// 0x41cc50
PixelusScrollButton::PixelusScrollButton(int theId, Image* theImage, ButtonListener* theListener) :
	ScrollbuttonWidget(theId, theListener)
{
	mImage = theImage;
	mButtonImage = theImage;
}

// 0x41ccb0: the picture at the button's size, a pixel down and right while it is held
void PixelusScrollButton::Draw(Graphics* g)
{
	if (mImage == NULL)
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

// 0x41c7d0: the framework's bar with its two buttons put right (its own are replaced by the game's)
PixelusScrollbar::PixelusScrollbar(int theId, Image* theTrack, Image* theThumb, Image* theUpArrow, Image* theDownArrow,
	ScrollListener* theListener, ScrollKeys* theKeys) :
	ScrollbarWidget(theId, theListener)
{
	mKeys = theKeys;
	mTrack = theTrack;
	mThumb = theThumb;

	RemoveWidget(mUpButton);
	delete mUpButton;
	mUpButton = new PixelusScrollButton(0, theUpArrow, this);
	mUpButton->SetDisabled(true);
	AddWidget(mUpButton);

	RemoveWidget(mDownButton);
	delete mDownButton;
	mDownButton = new PixelusScrollButton(1, theDownArrow, this);
	mDownButton->SetDisabled(true);
	AddWidget(mDownButton);
}

// 0x41cb90: the track as a box, the thumb on it
void PixelusScrollbar::Draw(Graphics* g)
{
	if (mTrack == NULL || mThumb == NULL)
	{
		ScrollbarWidget::Draw(g);
		return;
	}
	int aThumbSize = GetThumbSize();
	int aThumbPosition = GetThumbPosition();
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mTrack);
	if (!mHorizontal)
	{
		if (aThumbSize > 0)
			DrawThumb(g, 0, aThumbPosition, mWidth, aThumbSize);
	}
	else if (aThumbSize > 0)
		DrawThumb(g, aThumbPosition, 0, aThumbSize, mHeight);
}

// 0x41cb20
void PixelusScrollbar::DrawThumb(Graphics* g, int theX, int theY, int theWidth, int theHeight)
{
	if (mThumb == NULL)
	{
		ScrollbarWidget::DrawThumb(g, theX, theY, theWidth, theHeight);
		return;
	}
	g->DrawImageBox(Rect(theX, theY, theWidth, theHeight), mThumb);
}

// 0x41ca80: an arrow asks the owner first, which may do the scrolling its own way
void PixelusScrollbar::ButtonPress(int theId)
{
	mButtonAcc = 0;
	if (theId == 0)
	{
		if (mKeys != NULL && mKeys->LineUp())
			return;
		SetValue(mValue - 1.0);
	}
	else
	{
		if (mKeys != NULL && mKeys->LineDown())
			return;
		SetValue(mValue + 1.0);
	}
}

// 0x41c970: the track above or below the thumb asks the owner first too (and then does not repeat)
void PixelusScrollbar::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	Widget::MouseDown(x, y, theBtnNum, theClickCount);
	if (!mDisabled)
	{
		switch (ThumbCompare(x, y))
		{
		case -1:
			if (mKeys == NULL || !mKeys->PageUp())
			{
				SetValue(mValue - mPageSize);
				mUpdateMode = UPDATE_MODE_PGUP;
				mUpdateAcc = 0;
			}
			break;
		case 0:
			mPressedOnThumb = true;
			mMouseDownThumbPos = GetThumbPosition();
			mMouseDownX = x;
			mMouseDownY = y;
			break;
		case 1:
			if (mKeys == NULL || !mKeys->PageDown())
			{
				SetValue(mValue + mPageSize);
				mUpdateMode = UPDATE_MODE_PGDN;
				mUpdateAcc = 0;
			}
			break;
		}
	}
	mLastMouseX = x;
	mLastMouseY = y;
}

// 0x41c930: letting go tells the listener once more, now that the thumb is no longer held
void PixelusScrollbar::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	ScrollbarWidget::MouseUp(x, y, theBtnNum, theClickCount);
	mScrollListener->ScrollPosition(mId, mValue);
	MarkDirty();
}
