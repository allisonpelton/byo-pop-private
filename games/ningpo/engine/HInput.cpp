// HInput (see HInput.h).

#include "HInput.h"

#include <cstring>

namespace pg
{

static HInput* gInput = nullptr;

HInput* GetInput()
{
	return gInput;
}

void SetInput(HInput* theInput)
{
	gInput = theInput;
}

HInput::HInput()
{
	memset(mQueue, 0, sizeof(mQueue));
	mWrite = 0;
	mRead = 0;
	mMouseX = 0;
	mMouseY = 0;
	mLastTime = 0;
	mLastMoveX = 0;
	mLastMoveY = 0;
}

bool HInput::postEvent(const PGEvent& theEvent)
{
	if (theEvent.mType == EV_MOUSE_MOVE)
	{
		if (mLastMoveX == theEvent.mX && mLastMoveY == theEvent.mY)
			return true;
		mLastMoveX = theEvent.mX;
		mLastMoveY = theEvent.mY;
	}
	mQueue[mWrite] = theEvent;
	mQueue[mWrite].mTime = HTimeGetTime();
	int aNext = (mWrite + 1) % 128;
	if (aNext == mRead)
		return false;
	mWrite = aNext;
	return true;
}

bool HInput::peekEvent(PGEvent* theEvent)
{
	if (mWrite == mRead)
	{
		theEvent->mType = EV_NONE;
		return false;
	}
	*theEvent = mQueue[mRead];
	int aType = theEvent->mType;
	if (aType == EV_MOUSE_MOVE || (aType == EV_DOWN && theEvent->mCode == BUTTON_LEFT) ||
		(aType == EV_UP && theEvent->mCode == BUTTON_LEFT))
	{
		mMouseX = theEvent->mX;
		mMouseY = theEvent->mY;
	}
	return true;
}

bool HInput::getEvent(PGEvent* theEvent, bool theWait)
{
	if (peekEvent(theEvent))
	{
		mRead = (mRead + 1) % 128;
		mLastTime = theEvent->mTime;
		return true;
	}
	// the original pumped the window's messages here; port: the host posts them during its frame (HSleep)
	if (peekEvent(theEvent))
	{
		mRead = (mRead + 1) % 128;
		mLastTime = theEvent->mTime;
		return true;
	}
	PyTime aNow = HTimeGetTime();
	if (PyTimeSub(aNow, mLastTime) < 1 || theWait)
		HSleep(1);
	PGEvent aTick;
	memset(&aTick, 0, sizeof(aTick));
	aTick.mType = EV_TICK;
	postEvent(aTick);
	return false;
}

} // namespace pg
