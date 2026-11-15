#include "GenericScreen.h"

using namespace engine;

// 0x464540 (pizzafrenzy)
GenericScreen::GenericScreen()
{
	mState = STATE_NEW;
	mShowDelay = 0;
	mDelayLeft = 0;
	addOwnFlags(NODE_DRAWABLE | NODE_LISTENERS);
	setListener(this);
}

// 0x464410 (pizzafrenzy)
GenericScreen::~GenericScreen()
{
}

// 0x464080 (pizzafrenzy): the old transitions leave the screen's listeners first
void GenericScreen::setTransitions(Transition* theIn, Transition* theOut)
{
	if (mInTransition != nullptr)
	{
		mInTransition->attach(nullptr);
		removeListener(mInTransition);
	}
	if (mOutTransition != nullptr)
	{
		mOutTransition->attach(nullptr);
		removeListener(mOutTransition);
	}
	mInTransition = theIn;
	theIn->attach(this);
	mOutTransition = theOut;
	theOut->attach(this);
	resetTransitions();
}

// 0x464150 (pizzafrenzy)
void GenericScreen::resetTransitions()
{
	removeListener(mOutTransition);
	addListener(mInTransition);
	mInTransition->setProgress(0);
	disconnect();
}

// 0x464020 (pizzafrenzy)
void GenericScreen::show()
{
	resetTransitions();
	mDelayLeft = mShowDelay;
	mState = STATE_SHOW_WAIT;
	addOwnFlags(NODE_DRAWABLE | NODE_LISTENERS);
}

// 0x464190 (pizzafrenzy)
void GenericScreen::hide()
{
	if (mState != STATE_HIDDEN)
	{
		removeListener(mInTransition);
		addListener(mOutTransition);
		mOutTransition->setProgress(1.0f);
		mDelayLeft = mShowDelay;
		mState = STATE_HIDE_WAIT;
		addOwnFlags(NODE_DRAWABLE | NODE_LISTENERS);
		disconnect();
	}
}

// 0x464050 (pizzafrenzy)
bool GenericScreen::isIdle()
{
	int aState = mState;
	if (aState != STATE_SHOW_WAIT && aState != STATE_SHOWING && aState != STATE_HIDING && aState != STATE_HIDE_WAIT)
		return true;
	return false;
}

// 0x464200 (pizzafrenzy): a step that starts a transition or ends one leaves the rest of the frame's update to the
// fade container
void GenericScreen::update(UpdateInfo& theInfo)
{
	switch (mState)
	{
	case STATE_SHOW_WAIT:
	{
		float aDelayLeft = mDelayLeft - theInfo.mSeconds;
		mDelayLeft = aDelayLeft;
		if (aDelayLeft < 0.0f)
		{
			mState = STATE_SHOWING;
			mInTransition->start(Transition::IN);
			FadeContainer::update(theInfo);
			return;
		}
		break;
	}
	case STATE_SHOWING:
		if (mInTransition->isDone())
		{
			mState = STATE_SHOWN;
			removeListener(mInTransition);
			connect();
			FadeContainer::update(theInfo);
			return;
		}
		break;
	case STATE_HIDE_WAIT:
	{
		float aDelayLeft = mDelayLeft - theInfo.mSeconds;
		mDelayLeft = aDelayLeft;
		if (aDelayLeft < 0.0f)
		{
			mState = STATE_HIDING;
			mOutTransition->start(Transition::OUT);
			FadeContainer::update(theInfo);
			return;
		}
		break;
	}
	case STATE_HIDING:
		if (mOutTransition->isDone())
		{
			removeListener(mOutTransition);
			mState = STATE_HIDDEN;
		}
		break;
	}
	FadeContainer::update(theInfo);
}
