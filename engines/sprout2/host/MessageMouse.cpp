#include "MessageMouse.h"

#include "SexyAppBase.h"

namespace engine
{

MessageMouse::MessageMouse(int theWidth, int theHeight)
    : mAcquired(false), mPosition(0, 0), mCentre(theWidth / 2, theHeight / 2), mSavedPosition(0, 0)
{
	for (int i = 0; i < BUTTON_COUNT; i++)
		mButtonDown[i] = false;
}

bool MessageMouse::acquire()
{
	if (!mAcquired)
	{
		// ShowCursor(FALSE); port: the framework's cursor hidden
		Sexy::gSexyAppBase->SetCursor(Sexy::CURSOR_NONE);
		mSavedPosition = mPosition;
		mPosition = mCentre;
		Sexy::gSexyAppBase->WarpMouse(mPosition.x, mPosition.y);
		mAcquired = true;
	}
	return mAcquired;
}

void MessageMouse::release()
{
	if (!mAcquired)
		return;
	Sexy::gSexyAppBase->WarpMouse(mSavedPosition.x, mSavedPosition.y);
	Sexy::gSexyAppBase->SetCursor(Sexy::CURSOR_POINTER);
	mAcquired = false;
}

bool MessageMouse::isButtonDown(int theButton)
{
	if (theButton < 0 || theButton >= BUTTON_COUNT)
		return false;
	return mButtonDown[theButton];
}

void MessageMouse::buttonDown(const Vector2<int>& thePosition, int theButton)
{
	mPosition = thePosition;
	sendMotion();
	mButtonDown[theButton] = true;
	mButtonDownSignal.emit(theButton);
}

void MessageMouse::buttonUp(const Vector2<int>& thePosition, int theButton)
{
	mPosition = thePosition;
	sendMotion();
	mButtonDown[theButton] = false;
	mButtonUpSignal.emit(theButton);
}

void MessageMouse::sendMotion()
{
	Vector2<int> aMotion(mPosition.x - mCentre.x, mPosition.y - mCentre.y);
	if (aMotion.x == 0 && aMotion.y == 0)
		return;
	mMotionSignal.emit(aMotion);
	mPosition = mCentre;
	Sexy::gSexyAppBase->WarpMouse(mPosition.x, mPosition.y);
}

} // namespace engine
