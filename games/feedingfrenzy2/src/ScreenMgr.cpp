#include "ScreenMgr.h"

using namespace engine;

ScreenMgr::ScreenMgr() : mChanging(false)
{
	addOwnState(STATE_VISIBLE | STATE_ANIMATED);
}

void ScreenMgr::showScreen(GenericScreen* theScreen, float theDelay)
{
	if (theScreen != nullptr)
		theScreen->setDelay(theDelay);
	mQueue.push_back(theScreen);
}

void ScreenMgr::finishChange()
{
	if (mNext != nullptr && !mNext->isSettled())
		return;
	if (mCurrent != nullptr && !mCurrent->isSettled())
		return;
	if (mCurrent != nullptr)
		removeChild(mCurrent.get());
	mCurrent = mNext;
	mNext = nullptr;
	mChanging = false;
}

void ScreenMgr::update(UpdateContext& theContext)
{
	if (!mChanging)
	{
		if (!mQueue.empty())
		{
			mNext = mQueue.front();
			mQueue.pop_front();
			if (mCurrent == mNext)
			{
				mNext = nullptr;
				mChanging = false;
			}
			else
			{
				if (mCurrent != nullptr)
				{
					mCurrent->setDelay(0.0f);
					mCurrent->hide();
				}
				if (mNext != nullptr)
				{
					addChild(mNext.get());
					mNext->show();
				}
				mChanging = true;
			}
		}
	}
	else
		finishChange();
	Container::update(theContext);
}
