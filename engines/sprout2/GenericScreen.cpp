#include "GenericScreen.h"

#include "Engine.h"

namespace engine
{

GenericScreen::GenericScreen()
	: mPhase(SCREEN_NEW), mDeleteWhenHidden(false), mDelay(0.0f), mCountdown(0.0f)
{
	addOwnState(STATE_VISIBLE | STATE_ANIMATED);
	setListener(this);
}

void GenericScreen::setTransitions(Transition* theIn, Transition* theOut)
{
	if (mInTransition != nullptr)
	{
		mInTransition->setTarget(nullptr);
		removeModifier(mInTransition.get());
	}
	if (mOutTransition != nullptr)
	{
		mOutTransition->setTarget(nullptr);
		removeModifier(mOutTransition.get());
	}
	mInTransition = theIn;
	if (mInTransition != nullptr)
		mInTransition->setTarget(this);
	mOutTransition = theOut;
	if (mOutTransition != nullptr)
		mOutTransition->setTarget(this);
	reset();
}

void GenericScreen::reset()
{
	if (mOutTransition != nullptr)
		removeModifier(mOutTransition.get());
	if (mInTransition != nullptr)
	{
		Component::addModifier(mInTransition.get());
		mInTransition->setProgress(0.0f);
	}
	disable();
}

void GenericScreen::show()
{
	reset();
	mCountdown = mDelay;
	addOwnState(STATE_VISIBLE | STATE_ANIMATED);
	mPhase = SCREEN_WAITING_TO_SHOW;
}

void GenericScreen::hide()
{
	if (mPhase == SCREEN_HIDDEN)
		return;
	if (mInTransition != nullptr)
		removeModifier(mInTransition.get());
	if (mOutTransition != nullptr)
	{
		Component::addModifier(mOutTransition.get());
		mOutTransition->setProgress(1.0f);
	}
	mCountdown = mDelay;
	mPhase = SCREEN_WAITING_TO_HIDE;
	addOwnState(STATE_VISIBLE | STATE_ANIMATED);
	disable();
}

bool GenericScreen::isSettled()
{
	return mPhase != SCREEN_SHOWING && mPhase != SCREEN_HIDING && mPhase != SCREEN_WAITING_TO_SHOW &&
		mPhase != SCREEN_WAITING_TO_HIDE;
}

void GenericScreen::update(UpdateContext& theContext)
{
	if (mPhase == SCREEN_WAITING_TO_SHOW)
	{
		mCountdown = (float)((double)mCountdown - theContext.mElapsed);
		if (mCountdown < 0.0f)
		{
			mPhase = SCREEN_SHOWING;
			if (mInTransition != nullptr)
				mInTransition->start(TRANSITION_IN);
		}
	}
	else if (mPhase == SCREEN_SHOWING)
	{
		if (mInTransition == nullptr || mInTransition->isDone())
		{
			mPhase = SCREEN_SHOWN;
			if (mInTransition != nullptr)
				removeModifier(mInTransition.get());
			else
				setVisible(true);
			enable();
		}
	}
	else if (mPhase == SCREEN_WAITING_TO_HIDE)
	{
		mCountdown = (float)((double)mCountdown - theContext.mElapsed);
		if (mCountdown < 0.0f)
		{
			mPhase = SCREEN_HIDING;
			if (mOutTransition != nullptr)
				mOutTransition->start(TRANSITION_OUT);
		}
	}
	else if (mPhase == SCREEN_HIDING)
	{
		if (mOutTransition == nullptr || mOutTransition->isDone())
		{
			if (mOutTransition != nullptr)
				removeModifier(mOutTransition.get());
			else
				setVisible(false);
			mPhase = SCREEN_HIDDEN;
			if (mDeleteWhenHidden)
				addOwnState(STATE_DELETE);
		}
	}
	ScreenLayout::update(theContext);
}

} // namespace engine
