// A screen (see Screen.h).

#include "Screen.h"
#include "App.h"
#include "Events.h"
#include "Graphics.h"
#include "Input.h"
#include "Platform.h"
#include "Widget.h"

namespace bonnies
{

// 0x429560
void Screen::init()
{
	mX = 0.0f;
	mY = 0.0f;
	mWidth = 0.1f;
	mHeight = 0.1f;
	mPriority = 0;
	mHidden = 0;
	mDraggable = 0;
	mDrawsPanel = 0;
	mColor = 0xff3f3f3f;
	mLevel = 0;
	mNext = nullptr;
	mPrev = nullptr;
	mDragging = 0;
	mTail = nullptr;
	mHead = nullptr;
}

void Screen::shutdown()
{
	removeAll();
}

// 0x4295a0
void Screen::draw()
{
	if (isVisible() == 0)
		return;
	if (mDrawsPanel != 0)
		drawBevel(mX, mY, mWidth, mHeight, mColor, mColor, 1, 2.0f, 0.0f);
	for (Widget* aWidget = mHead; aWidget != nullptr; aWidget = aWidget->mNext)
		aWidget->draw();
	if (gPlatform->mDebugScreens != 0)
		drawRect(mX, mY, mWidth, mHeight, 0xff7fffff, 1);
}

// 0x429630
void Screen::update()
{
	for (Widget* aWidget = mHead; aWidget != nullptr; aWidget = aWidget->mNext)
		aWidget->update();
}

// 0x429650
int Screen::isVisible()
{
	return mHidden == 0;
}

// 0x429660
void Screen::addWidget(Widget* theWidget)
{
	theWidget->mOwner = this;
	Widget* aWidget = mHead;
	if (aWidget == nullptr)
	{
		mHead = theWidget;
		mTail = theWidget;
		return;
	}
	do
	{
		if (theWidget->mPriority < aWidget->mPriority)
			break;
		aWidget = aWidget->mNext;
	} while (aWidget != nullptr);
	theWidget->mNext = aWidget;
	theWidget->mPrev = aWidget == nullptr ? mTail : aWidget->mPrev;
	if (aWidget != nullptr)
	{
		// As the original has it: the widget's next becomes itself and the one before it keeps its next (the game adds
		// its widgets in order of priority, so this never runs)
		aWidget->mPrev = theWidget;
		theWidget->mNext = theWidget;
		if (aWidget == mHead)
			mHead = theWidget;
		return;
	}
	mTail->mNext = theWidget;
	mTail = theWidget;
}

// 0x4296d0
int Screen::contains(float theX, float theY)
{
	if (isVisible() != 0 && mX <= theX && theX <= (double)mWidth + mX && mY <= theY && theY <= (double)mHeight + mY)
		return 1;
	return 0;
}

// 0x429730
void Screen::drag()
{
	float aMouseX;
	float aMouseY;
	gInput->getMousePos(&aMouseX, &aMouseY);
	float aX = (float)(((double)aMouseX - mDragX) + mX);
	mX = aX;
	float aY = (float)(((double)aMouseY - mDragY) + mY);
	mY = aY;
	if (aX < 0.0f)
		mX = 0.0f;
	if (aY < 0.0f)
		mY = 0.0f;
	if (1.0 < (double)mX + mWidth)
		mX = (float)(1.0 - mWidth);
	if (1.0 < (double)mHeight + mY)
		mY = (float)(1.0 - mHeight);
	mDragX = aMouseX;
	mDragY = aMouseY;
}

// 0x4297e0
int Screen::handleEvent(Event* theEvent)
{
	Widget* aWidget = mTail;
	int aResult = 1;
	do
	{
		if (aWidget == nullptr)
		{
			if (aResult == 0)
				return 0;
			switch (theEvent->mType)
			{
			case 1:
				if (mDragging == 0)
					return aResult;
				drag();
				return aResult;
			case 2:
				if (contains(theEvent->mX, theEvent->mY) == 0)
					return aResult;
				if (mDraggable == 0)
					return aResult;
				mDragging = 1;
				mDragX = theEvent->mX;
				mDragY = theEvent->mY;
				drag();
				return 0;
			case 3:
				mDragging = 0;
				return aResult;
			case 0xb:
				gQuit = 1;
				return aResult;
			default:
				return aResult;
			}
		}
		if ((aWidget->mFlags & 8) == 0)
			aResult = aWidget->handleEvent(theEvent);
		aWidget = aWidget->mPrev;
	} while (aResult != 0);
	return 0;
}

// 0x4298d0
Widget* Screen::findWidget(int theId)
{
	Widget* aWidget = mTail;
	for (;;)
	{
		if (aWidget == nullptr)
			return nullptr;
		if (aWidget->mId == theId)
			break;
		aWidget = aWidget->mPrev;
	}
	return aWidget;
}

// 0x429900
int Screen::isTop()
{
	if (0 < mLevel && gApp->mDepth == mLevel)
		return 1;
	return 0;
}

// 0x429920
void Screen::removeWidget(Widget* theWidget)
{
	if (mHead == theWidget)
		mHead = theWidget->mNext;
	if (mTail == theWidget)
		mTail = theWidget->mPrev;
	if (theWidget->mNext != nullptr)
		theWidget->mNext->mPrev = theWidget->mPrev;
	if (theWidget->mPrev != nullptr)
		theWidget->mPrev->mNext = theWidget->mNext;
	theWidget->shutdown();
	// The original deletes it by its type (0-6), the only ones there are
	if (theWidget->mType >= 0 && theWidget->mType <= 6)
		delete theWidget;
}

// 0x429a30
void Screen::removeAll()
{
	Widget* aWidget = mHead;
	while (aWidget != nullptr)
	{
		removeWidget(aWidget);
		aWidget = mHead;
	}
}

} // namespace bonnies
