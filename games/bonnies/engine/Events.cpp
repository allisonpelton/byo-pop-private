// The event queue (see Events.h).

#include "Events.h"
#include "Graphics.h"
#include "Host.h"
#include "Input.h"
#include "Platform.h"
#include "Sound.h"

namespace bonnies
{

EventQueue* gEvents = nullptr;

namespace
{
int gHadFocus = 0;  // 0x474074: whether the window had the input focus at the last activation event
}

// 0x42e980
EventQueue::EventQueue()
{
	mTail = nullptr;
	mHead = nullptr;
}

// 0x42e990
EventQueue::~EventQueue()
{
	Event* anEvent = mHead;
	while (anEvent != nullptr)
	{
		Event* aNext = anEvent->mNext;
		delete anEvent;
		anEvent = aNext;
	}
}

// 0x42e9b0
Event* newEvent(int theType, int theA, int theB)
{
	Event* anEvent = new Event;
	anEvent->mDone = false;
	anEvent->mNext = nullptr;
	anEvent->mType = theType;
	anEvent->mA = theA;
	anEvent->mB = theB;
	anEvent->mShift = Input::isKeyDown(SDLK_LSHIFT) || Input::isKeyDown(SDLK_RSHIFT);
	anEvent->mCtrl = Input::isKeyDown(SDLK_LCTRL) || Input::isKeyDown(SDLK_RCTRL);
	anEvent->mAlt = Input::isKeyDown(SDLK_LALT) || Input::isKeyDown(SDLK_RALT);
	anEvent->mMeta = Input::isKeyDown(SDLK_LMETA) || Input::isKeyDown(SDLK_RMETA);
	gInput->getMousePos(&anEvent->mX, &anEvent->mY);
	return anEvent;
}

// 0x42eac0
void EventQueue::add(int theType, int theA, int theB)
{
	Event* anEvent = newEvent(theType, theA, theB);
	if (mTail != nullptr)
	{
		mTail->mNext = anEvent;
		mTail = anEvent;
		return;
	}
	mHead = anEvent;
	mTail = anEvent;
}

// 0x42eb00
void EventQueue::addButton(int theType, float theX, float theY)
{
	if (gInput != nullptr && gInput->mSkipMotion != 0 && theType == 1)
	{
		gInput->mSkipMotion = 0;
		return;
	}
	float aX = (float)((double)(int)theX / (double)screenWidth());
	float aY = (float)((double)(int)theY / (double)screenHeight());
	if (gInput != nullptr)
	{
		if (gInput->mPinned == 0)
		{
			gInput->mPinnedX = aX;
			gInput->mPinnedY = aY;
		}
		else
		{
			gInput->mPinnedX = (float)((double)aX + gInput->mPinnedX);
			gInput->mPinnedY = (float)((double)aY + gInput->mPinnedY);
			aX = gInput->mPinnedX;
			aY = gInput->mPinnedY;
		}
	}
	Event* anEvent = newEvent(theType, 0, 0);
	bool aNoted = false;
	if (gGraphics != nullptr && screenWidth() != 0)
	{
		if (theType == 1 && gInput != nullptr && gInput->mPinned == 0)
		{
			gInput->getMousePos(&anEvent->mX, &anEvent->mY);
			aNoted = true;
		}
		else
		{
			anEvent->mX = aX;
			anEvent->mY = aY;
		}
	}
	if (!aNoted && theType != 1 && gInput != nullptr)
	{
		gInput->mDownTime = host()->getTicks();
		gInput->mDownX = anEvent->mX;
		gInput->mDownY = anEvent->mY;
	}
	if (mTail != nullptr)
	{
		mTail->mNext = anEvent;
		mTail = anEvent;
		return;
	}
	mHead = anEvent;
	mTail = anEvent;
}

// 0x42ec70
void EventQueue::push(int theType, int theA, int theB)
{
	Event* anEvent = newEvent(theType, theA, theB);
	if (mHead != nullptr)
	{
		anEvent->mNext = mHead;
		mHead = anEvent;
		return;
	}
	mTail = anEvent;
	mHead = anEvent;
}

// 0x42ecb0
void EventQueue::pump()
{
	Event* aPrev = nullptr;
	Event* anEvent = mHead;
	while (anEvent != nullptr)
	{
		if (!anEvent->mDone)
		{
			aPrev = anEvent;
			anEvent = anEvent->mNext;
			continue;
		}
		if (aPrev != nullptr)
			aPrev->mNext = anEvent->mNext;
		if (anEvent == mHead)
			mHead = anEvent->mNext;
		if (anEvent == mTail)
			mTail = aPrev;
		Event* aNext = anEvent->mNext;
		delete anEvent;
		anEvent = aNext;
	}
	host()->pumpEvents();
	HostEvent aHostEvent;
	while (host()->pollEvent(aHostEvent))
	{
		switch (aHostEvent.mType)
		{
		case SDL_ACTIVEEVENT:
		{
			int aFocus = host()->getAppState() >> 1 & 1;
			if (aFocus != gHadFocus && gSound != nullptr)
			{
				if (aFocus == 0)
					gSound->pause();
				else
					gSound->resume();
			}
			gHadFocus = aFocus;
			break;
		}
		case SDL_KEYDOWN:
			if (aHostEvent.mSym != SDLK_PRINT)
				gEvents->add(8, aHostEvent.mSym, aHostEvent.mUnicode & 0xffff);
			break;
		case SDL_KEYUP:
			if (aHostEvent.mSym != SDLK_PRINT)
				gEvents->add(9, aHostEvent.mSym, aHostEvent.mUnicode & 0xffff);
			break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			float aX;
			float aY;
			if (gInput->mPinned == 0)
			{
				aX = (float)(unsigned)(aHostEvent.mX & 0xffff);
				aY = (float)(unsigned)(aHostEvent.mY & 0xffff);
			}
			else
			{
				aX = (float)(int16_t)aHostEvent.mRelX;
				aY = (float)(int16_t)aHostEvent.mRelY;
			}
			if (aHostEvent.mType == SDL_MOUSEMOTION)
			{
				if (isInactive() != 0)
					break;
				if (gInput != nullptr && gInput->mSkipMotion != 0)
				{
					gInput->mSkipMotion = 0;
					break;
				}
				// The same as addButton's, without noting the time
				float aFracX = (float)((double)(int)aX / (double)screenWidth());
				float aFracY = (float)((double)(int)aY / (double)screenHeight());
				if (gInput != nullptr)
				{
					if (gInput->mPinned == 0)
					{
						gInput->mPinnedX = aFracX;
						gInput->mPinnedY = aFracY;
					}
					else
					{
						gInput->mPinnedX = (float)((double)aFracX + gInput->mPinnedX);
						gInput->mPinnedY = (float)((double)aFracY + gInput->mPinnedY);
						aFracX = gInput->mPinnedX;
						aFracY = gInput->mPinnedY;
					}
				}
				Event* aMotion = newEvent(1, 0, 0);
				if (gGraphics != nullptr && screenWidth() != 0)
				{
					if (gInput == nullptr || gInput->mPinned != 0)
					{
						aMotion->mX = aFracX;
						aMotion->mY = aFracY;
					}
					else
						gInput->getMousePos(&aMotion->mX, &aMotion->mY);
				}
				if (gEvents->mTail == nullptr)
				{
					gEvents->mTail = aMotion;
					gEvents->mHead = aMotion;
				}
				else
				{
					gEvents->mTail->mNext = aMotion;
					gEvents->mTail = aMotion;
				}
			}
			else if (aHostEvent.mType == SDL_MOUSEBUTTONDOWN)
			{
				if (aHostEvent.mButton == 1)
					gEvents->addButton(2, aX, aY);
				else if (aHostEvent.mButton == 2)
					gEvents->addButton(4, aX, aY);
				else if (aHostEvent.mButton == 3)
					gEvents->addButton(6, aX, aY);
			}
			else
			{
				if (aHostEvent.mButton == 1)
					gEvents->addButton(3, aX, aY);
				else if (aHostEvent.mButton == 2)
					gEvents->addButton(5, aX, aY);
				else if (aHostEvent.mButton == 3)
					gEvents->addButton(7, aX, aY);
			}
			break;
		}
		case SDL_QUIT:
			gEvents->add(0xb, 0, 0);
			break;
		case SDL_SYSWMEVENT:
			// 0x420630 handles the window messages SDL passes on; the port's host sends none (see ENGINE.md)
			break;
		}
	}
	// Hands the events to the app one at a time, each time the first not yet handled (events the handler pushes come
	// first)
	anEvent = mHead;
	if (anEvent == nullptr)
		return;
	while (anEvent->mDone)
	{
		anEvent = anEvent->mNext;
		if (anEvent == nullptr)
			return;
	}
	for (;;)
	{
		anEvent->mDone = true;
		gPlatform->handleEvent(anEvent);
		anEvent = mHead;
		for (;;)
		{
			if (anEvent == nullptr)
				return;
			if (!anEvent->mDone)
				break;
			anEvent = anEvent->mNext;
		}
	}
}

} // namespace bonnies
