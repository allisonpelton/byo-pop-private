#include "ButtonBase.h"

#include "Scene.h"
#include "Surface.h"

namespace engine
{

namespace
{

// The seconds a button takes to grow or shrink, until it is given its own (0x3f000000)
const float DEFAULT_SCALE_TIME = 0.5f;

} // namespace

ButtonBase::ButtonBase()
	: mState(BUTTON_NORMAL), mScaling(false), mNormalScale(1.0f, 1.0f), mHighScale(1.0f, 1.0f),
	  mBaseScale(1.0f, 1.0f), mScaleTimer(0.0f), mScaleTime(DEFAULT_SCALE_TIME), mScalePhase(SCALE_NORMAL),
	  mFlashTimer(0.0f), mFlashPeriod(0.0f), mFlashIn(true), mFlashing(false), mPivot(PIVOT_UPPER_LEFT)
{
}

void ButtonBase::disable()
{
	if (mState != BUTTON_NORMAL)
	{
		mState = BUTTON_NORMAL;
		updateLook();
	}
	addOffState(STATE_INPUT);
}

void ButtonBase::update(UpdateContext& theContext)
{
	if (mScaling)
	{
		if (mScalePhase == SCALE_GROWING || mScalePhase == SCALE_SHRINKING)
		{
			double aTimer = (double)mScaleTimer - theContext.mElapsed;
			mScaleTimer = (float)aTimer;
			if (aTimer < 0.0)
				mScaleTimer = 0.0f;
		}
		switch (mScalePhase)
		{
		case SCALE_HIGH:
			setScale(mBaseScale.x * mHighScale.x, mBaseScale.y * mHighScale.y);
			break;
		case SCALE_GROWING:
		{
			float aShare = (float)(1.0 - (double)mScaleTimer / mScaleTime);
			setScale((float)((((double)mHighScale.x - mNormalScale.x) * aShare + mNormalScale.x) * mBaseScale.x),
				(float)((((double)mHighScale.y - mNormalScale.y) * aShare + mNormalScale.y) * mBaseScale.y));
			if (1.0f <= aShare)
				mScalePhase = SCALE_HIGH;
			break;
		}
		case SCALE_SHRINKING:
		{
			float aShare = (float)((double)mScaleTimer / mScaleTime);
			setScale((float)((((double)mHighScale.x - mNormalScale.x) * aShare + mNormalScale.x) * mBaseScale.x),
				(float)((((double)mHighScale.y - mNormalScale.y) * aShare + mNormalScale.y) * mBaseScale.y));
			if (aShare <= 0.0f)
				mScalePhase = SCALE_NORMAL;
			break;
		}
		case SCALE_NORMAL:
			setScale(mBaseScale.x * mNormalScale.x, mBaseScale.y * mNormalScale.y);
			break;
		}
	}
	if (mFlashing)
	{
		double aTimer = (double)mFlashTimer - theContext.mElapsed;
		mFlashTimer = (float)aTimer;
		if (aTimer < 0.0)
			mFlashTimer = 0.0f;
		double aShare = (double)mFlashTimer / mFlashPeriod;
		if (mFlashIn)
		{
			applyFlash((float)aShare);
			if (!(0.0f < mFlashTimer))
			{
				mFlashIn = false;
				mFlashTimer = mFlashPeriod;
			}
		}
		else
		{
			applyFlash((float)(1.0 - aShare));
			if (!(0.0f < mFlashTimer))
			{
				mFlashIn = true;
				mFlashTimer = mFlashPeriod;
			}
		}
	}
	Container::update(theContext);
}

void ButtonBase::mouseDown(MouseEvent& theEvent)
{
	if (mState != BUTTON_OVER)
		return;
	mState = BUTTON_DOWN;
	updateLook();
	notifyDown();
	theEvent.mScene->setMouseCapture(this);
	// The press's sound played (its instance's slot 0): the sounds are not yet translated
}

void ButtonBase::mouseUp(MouseEvent& theEvent)
{
	if (mState == BUTTON_DOWN)
	{
		mState = BUTTON_OVER;
		updateLook();
		notifyUp();
		notifyAction();
	}
	else if (mState == BUTTON_PRESSED_OUT)
	{
		setState(BUTTON_NORMAL);
		startFlash();
		notifyLeave();
	}
	theEvent.mScene->setMouseCapture(nullptr);
}

void ButtonBase::mouseEnter(MouseEvent& theEvent)
{
	if (mState == BUTTON_PRESSED_OUT)
	{
		setState(BUTTON_DOWN);
		notifyDown();
	}
	else if (mState == BUTTON_NORMAL)
	{
		mState = BUTTON_OVER;
		updateLook();
		notifyEnter();
		// The sound of the mouse coming on played (its instance's slot 0): the sounds are not yet translated
	}
	// Grows from where a shrinking left it
	mScalePhase = SCALE_GROWING;
	mScaleTimer = 0.0f < mScaleTimer ? mScaleTime - mScaleTimer : mScaleTime;
}

void ButtonBase::mouseLeave(MouseEvent& theEvent)
{
	if (mState == BUTTON_DOWN)
	{
		setState(BUTTON_PRESSED_OUT);
		notifyUp();
	}
	else if (mState == BUTTON_OVER)
	{
		mState = BUTTON_NORMAL;
		updateLook();
		startFlash();
		notifyLeave();
	}
	mScalePhase = SCALE_SHRINKING;
	mScaleTimer = 0.0f < mScaleTimer ? mScaleTime - mScaleTimer : mScaleTime;
}

void ButtonBase::mouseCancel(MouseEvent& theEvent)
{
	switch (mState)
	{
	case BUTTON_DOWN:
		setState(BUTTON_NORMAL);
		startFlash();
		notifyLeave();
		notifyUp();
		// 0x508344 (feedingfrenzy2)
		theEvent.mScene->setMouseCapture(nullptr);
		break;
	case BUTTON_PRESSED_OUT:
		setState(BUTTON_NORMAL);
		startFlash();
		notifyLeave();
		theEvent.mScene->setMouseCapture(nullptr);
		break;
	case BUTTON_OVER:
		setState(BUTTON_NORMAL);
		startFlash();
		notifyLeave();
		break;
	}
}

void ButtonBase::setBaseScale(float theScaleX, float theScaleY)
{
	mBaseScale.x = theScaleX;
	mBaseScale.y = theScaleY;
	Component::setScale(theScaleX, theScaleY);
}

void ButtonBase::setNormalScale(Vector2f theScale)
{
	mNormalScale = theScale;
	mScaling = !(mHighScale.x == mNormalScale.x && mHighScale.y == mNormalScale.y);
}

void ButtonBase::setHighScale(Vector2f theScale)
{
	mHighScale = theScale;
	mScaling = !(mHighScale.x == mNormalScale.x && mHighScale.y == mNormalScale.y);
}

void ButtonBase::addListener(IButtonListener* theListener, std::string theAction)
{
	Listener aListener;
	aListener.mListener = theListener;
	aListener.mAction = theAction;
	mListeners.push_back(aListener);
}

void ButtonBase::setSounds(const std::string& theEnterSound, const std::string& theDownSound)
{
	// The original loads each named sound (the engine's sounds' slot 0x7c) and makes its instance (slot 0x84)
	if (!theEnterSound.empty())
		mEnterSound = theEnterSound;
	if (!theDownSound.empty())
		mDownSound = theDownSound;
}

void ButtonBase::setFlashPeriod(float thePeriod)
{
	if (thePeriod == mFlashPeriod)
		return;
	mFlashTimer = thePeriod;
	mFlashIn = true;
	mFlashPeriod = thePeriod;
	mFlashing = 0.0f < thePeriod;
}

void ButtonBase::notifyEnter()
{
	for (size_t i = 0; i < mListeners.size(); i++)
		mListeners[i].mListener->buttonEnter(mListeners[i].mAction);
}

void ButtonBase::notifyLeave()
{
	for (size_t i = 0; i < mListeners.size(); i++)
		mListeners[i].mListener->buttonLeave(mListeners[i].mAction);
}

void ButtonBase::notifyDown()
{
	for (size_t i = 0; i < mListeners.size(); i++)
		mListeners[i].mListener->buttonDown(mListeners[i].mAction);
}

void ButtonBase::notifyUp()
{
	for (size_t i = 0; i < mListeners.size(); i++)
		mListeners[i].mListener->buttonUp(mListeners[i].mAction);
}

void ButtonBase::notifyAction()
{
	for (size_t i = 0; i < mListeners.size(); i++)
		mListeners[i].mListener->buttonAction(mListeners[i].mAction);
}

void ButtonBase::setState(int theState)
{
	if (theState == mState)
		return;
	mState = theState;
	updateLook();
}

void ButtonBase::startFlash()
{
	if (mFlashPeriod <= 0.0f)
		return;
	mFlashIn = true;
	mFlashTimer = mFlashPeriod;
	mFlashing = true;
}

} // namespace engine
