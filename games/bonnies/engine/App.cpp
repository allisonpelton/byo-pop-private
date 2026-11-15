// The engine's app (see App.h).

#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include "Host.h"
#include "Resources.h"
#include "Screen.h"
#include "Widget.h"

namespace bonnies
{

App* gApp = nullptr;

// 0x420da0
App::App()
{
}

// 0x420df0
App::~App()
{
}

// 0x420e50
void App::init(int theWidth, int theHeight, int theFullScreen)
{
	gApp = this;
	mTail = nullptr;
	mHead = nullptr;
	mStatus.clear();
	mStatusUntil = 0;
	mDepth = 0;
	mLastResult = 0;
	Platform::init(theWidth, theHeight, theFullScreen);
	mTail = nullptr;
	mHead = nullptr;
	for (int i = 0; i < 7; i++)
		mDebugRects[i] = 0;
	mStatus.clear();
	mStatusFont = nullptr;
	mStatusUntil = 0;
	mEndPending = 0;
	mPendingResult = 0;
	mFocus = nullptr;
	mStatusX = 0.5f;
	mStatusColor = 0xffbfbfbf;
	mStatusFlags = 0x4c;
	mSounds[0] = nullptr;
	mSounds[1] = nullptr;
	mSounds[2] = nullptr;
}

// 0x420f20
void App::shutdown()
{
	while (mHead != nullptr)
		deleteScreen(mHead);
	if (mStatusFont != nullptr)
		gResources->release(mStatusFont);
	mStatusFont = nullptr;
	Sample** aSound = mSounds;
	int i = 3;
	do
	{
		if (*aSound != nullptr)
			gResources->release(*aSound);
		aSound++;
		i--;
	} while (i != 0);
	Platform::shutdown();
}

// 0x420fa0
void App::setDefaultSound(int theIndex, const Str& theName)
{
	if (mSounds[theIndex] != nullptr)
		gResources->release(mSounds[theIndex]);
	mSounds[theIndex] = gResources->getSound(theName);
}

// 0x421030
void App::setStatusFont(FontDesc* theDesc)
{
	if (mStatusFont != nullptr)
		gResources->release(mStatusFont);
	if (theDesc != nullptr)
		mStatusFont = gResources->getFont(theDesc);
}

// 0x421070
void App::addScreen(Screen* theScreen)
{
	Screen* aScreen = mHead;
	if (aScreen == nullptr)
	{
		mHead = theScreen;
		mTail = theScreen;
		return;
	}
	do
	{
		if (theScreen->mPriority < aScreen->mPriority)
			break;
		aScreen = aScreen->mNext;
	} while (aScreen != nullptr);
	theScreen->mNext = aScreen;
	theScreen->mPrev = aScreen == nullptr ? mTail : aScreen->mPrev;
	if (aScreen != nullptr)
	{
		if (aScreen->mPrev != nullptr)
			aScreen->mPrev->mNext = theScreen;
		aScreen->mPrev = theScreen;
		if (aScreen == mHead)
			mHead = theScreen;
		return;
	}
	mTail->mNext = theScreen;
	mTail = theScreen;
}

// 0x4210f0
void App::removeScreen(Screen* theScreen)
{
	if (mHead == theScreen)
		mHead = theScreen->mNext;
	if (mTail == theScreen)
		mTail = theScreen->mPrev;
	if (theScreen->mNext != nullptr)
		theScreen->mNext->mPrev = theScreen->mPrev;
	if (theScreen->mPrev != nullptr)
		theScreen->mPrev->mNext = theScreen->mNext;
}

// 0x421140
void App::deleteScreen(Screen* theScreen)
{
	removeScreen(theScreen);
	theScreen->shutdown();
	delete theScreen;
}

// 0x421160
int App::handleEvent(Event* theEvent)
{
	int aResult = Platform::handleEvent(theEvent);
	Screen* aScreen = mTail;
	if (aScreen == nullptr)
		return aResult;
	while (aResult != 0)
	{
		aResult = aScreen->handleEvent(theEvent);
		if (mDepth != 0 && mDepth <= aScreen->mLevel)
			return aResult;
		aScreen = aScreen->mPrev;
		if (aScreen == nullptr)
			return aResult;
	}
	return aResult;
}

// 0x4011c0
void App::showStatus(const Str& theText)
{
	setStatus(theText, 0);
}

// 0x4211b0
void App::setStatus(const Str& theText, int theTime)
{
	mStatus = theText;
	if (theTime < 1)
		mStatusUntil = 0;
	else
		mStatusUntil = (int)(host()->getTicks() + theTime);
}

// 0x421230
bool App::inModal()
{
	return 0 < mDepth;
}

// 0x421260
int App::runModal(int theBlocking, Screen* theScreen)
{
	addScreen(theScreen);
	gGraphics->setCursor(gArrowCursor);
	mDepth++;
	beginScreen();
	theScreen->mLevel = mDepth;
	mResults[mDepth] = 0;
	mDone[mDepth] = 0;
	mModal[mDepth] = theScreen;
	mBlocking[mDepth] = theBlocking;
	if (theBlocking == 0)
		return 0;
	while (gQuit == 0 && mDone[mDepth] == 0)
	{
		update();
		draw();
	}
	mDepth--;
	endScreen();
	deleteScreen(theScreen);
	return mResults[mDepth + 1];
}

// 0x421340
void App::endModal(int theResult, int theNow)
{
	if (mBlocking[mDepth] == 0 && theNow == 0)
	{
		mEndPending = 1;
		mPendingResult = theResult;
		return;
	}
	mResults[mDepth] = theResult;
	mDone[mDepth] = 1;
	if (mBlocking[mDepth] == 0)
	{
		deleteScreen(mModal[mDepth]);
		endScreen();
		mDepth--;
	}
	mLastResult = theResult;
}

// 0x4213d0
void App::setFocus(Widget* theWidget)
{
	if (gGraphics == nullptr)
		return;
	if (theWidget == nullptr || theWidget->isActive() == 0 ||
		(theWidget->mType != 1 && theWidget->mType != 3 && theWidget->mType != 4))
		gGraphics->setCursor(gArrowCursor);
	else
		gGraphics->setCursor(gHandCursor);
	mFocus = theWidget;
}

// 0x421810
void App::draw()
{
	Platform::draw();
	for (Screen* aScreen = mHead; aScreen != nullptr; aScreen = aScreen->mNext)
		aScreen->draw();
	if (!mStatus.empty() && mStatusFont != nullptr)
	{
		double aHalf = mStatusX;
		if (!(mStatusX < 1.0 - mStatusX))
			aHalf = 1.0 - aHalf;
		float aWidth = (float)((aHalf + aHalf) - 0.0125);
		mStatusFont->drawText(mStatus, (float)(mStatusX - (double)aWidth * 0.5f), 0.84833336f, aWidth, 0.14666666f,
			mStatusColor, 0x9f000000, mStatusFlags, 1.0f, 0.0f);
	}
}

// 0x4218d0
void App::update()
{
	Platform::update();
	if (0 < mStatusUntil && mStatusUntil < (int)host()->getTicks())
	{
		mStatus.clear();
		mStatusUntil = 0;
	}
	Screen* aScreen = mTail;
	while (aScreen != nullptr)
	{
		Screen* aPrev = aScreen->mPrev;
		aScreen->update();
		aScreen = aPrev;
	}
	if (mEndPending != 0)
	{
		mEndPending = 0;
		endModal(mPendingResult, 1);
		update();
	}
}

} // namespace bonnies
