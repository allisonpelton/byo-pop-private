#include "ScreenFader.h"
#include "Font.h"
#include "Graphics.h"

using namespace feedingfrenzy;

// 0x435ca0: opaque at first
ScreenFader::ScreenFader() :
	mCoverColor(1.0f, 1.0f, 1.0f, 1.0f), mFadeInSeconds(0.0f), mFadeOutSeconds(0.0f), mSecondsLeft(0.0f),
	mState(STATE_COVERED)
{
	setOpacity(1.0f);
	setPosition(0.0f, 0.0f);
}

// 0x4359c0
void ScreenFader::fadeFromColor()
{
	mState = STATE_FADING_FROM;
	mSecondsLeft = mFadeInSeconds;
	setVisible(true);
	setOpacity(1.0f);
	addOwnFlags(engine::NODE_DRAWABLE | engine::NODE_LISTENERS);
}

// 0x435a00
void ScreenFader::fadeToColor()
{
	mState = STATE_FADING_TO;
	mSecondsLeft = mFadeOutSeconds;
	setVisible(true);
	setOpacity(0.0f);
	addOwnFlags(engine::NODE_DRAWABLE | engine::NODE_LISTENERS);
}

// 0x435a60: while it fades the opacity follows the time left; when the fade is over it stays as it ended, and the
// node stops being updated
void ScreenFader::update(engine::UpdateInfo& theInfo)
{
	switch (mState)
	{
	case STATE_CLEAR:
		setOpacity(0.0f);
		setVisible(false);
		removeOwnFlags(engine::NODE_LISTENERS);
		break;
	case STATE_COVERED:
		setOpacity(1.0f);
		removeOwnFlags(engine::NODE_LISTENERS);
		break;
	case STATE_FADING_FROM:
	{
		mSecondsLeft -= theInfo.mSeconds;
		if (mSecondsLeft < 0.0f)
			mSecondsLeft = 0.0f;
		float anOpacity = mSecondsLeft / mFadeInSeconds;
		setOpacity(anOpacity);
		if (anOpacity <= 0.0f)
			mState = STATE_CLEAR;
		break;
	}
	case STATE_FADING_TO:
	{
		mSecondsLeft -= theInfo.mSeconds;
		if (mSecondsLeft < 0.0f)
			mSecondsLeft = 0.0f;
		float anOpacity = 1.0f - mSecondsLeft / mFadeOutSeconds;
		setOpacity(anOpacity);
		if (1.0f <= anOpacity)
			mState = STATE_COVERED;
		break;
	}
	}
	Node::update(theInfo);
}

// 0x435bc0: the rectangle's size in the cover's colour
void ScreenFader::draw(engine::Graphics* g)
{
	if (mVisible && mOpacity != 0.0f)
	{
		g->setColor(mCoverColor.r, mCoverColor.g, mCoverColor.b);
		g->fillRect(engine::Rect(0.0f, 0.0f, mRect.right - mRect.left, mRect.bottom - mRect.top));
	}
}

// 0x435c30
void ScreenFader::layout()
{
	mBounds.set(mRect.left, mRect.top, mRect.right, mRect.bottom);
	clearFlags(engine::NODE_LAYOUT);
}
