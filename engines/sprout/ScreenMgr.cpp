#include "ScreenMgr.h"

using namespace engine;

// 0x46b710 (pizzafrenzy)
ScreenMgr::ScreenMgr()
{
	addOwnFlags(NODE_DRAWABLE | NODE_LISTENERS);
	mSwitching = false;
}

// 0x46b390 (pizzafrenzy)
ScreenMgr::~ScreenMgr()
{
}

// 0x46b280 (pizzafrenzy): asking for the screen that is current already does nothing
void ScreenMgr::update(UpdateInfo& theInfo)
{
	if (mSwitching)
	{
		finishSwitch();
		Container::update(theInfo);
		return;
	}
	if (mQueue.size() != 0)
	{
		mNext = mQueue.front();
		mQueue.pop_front();
		if (mCurrent != mNext)
		{
			if (mCurrent != nullptr)
			{
				mCurrent->setShowDelay(0);
				mCurrent->hide();
			}
			if (mNext != nullptr)
			{
				addChild(mNext);
				mNext->show();
			}
			mSwitching = true;
			Container::update(theInfo);
			return;
		}
		mNext = nullptr;
		mSwitching = false;
	}
	Container::update(theInfo);
}

// 0x46b7f0 (pizzafrenzy)
void ScreenMgr::showScreen(GenericScreen* theScreen, float theDelay)
{
	if (theScreen != nullptr)
		theScreen->setShowDelay(theDelay);
	mQueue.push_back(theScreen);
}

// 0x46afd0 (pizzafrenzy)
void ScreenMgr::finishSwitch()
{
	if (mNext != nullptr && !mNext->isIdle())
		return;
	if (mCurrent != nullptr && !mCurrent->isIdle())
		return;
	if (mCurrent != nullptr)
		removeChild(mCurrent);
	mCurrent = mNext;
	mNext = nullptr;
	mSwitching = false;
}
