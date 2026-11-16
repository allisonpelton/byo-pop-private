#include "Banners.h"
#include "GameText.h"
#include "PizzaFrenzy.h"
#include "RectangleItem.h"

using namespace pizzafrenzy;

namespace
{
const char* const TITLE_FONT = "res\\fonts\\titleFont.xml";
const char* const SWISH_SOUND = "bonus_speedySwish";

// the banners' keys are listed as time, value (a position's two), the part of the stretch before the key the curve
// slows over, its speed there, the part after it the curve speeds up over, and its speed there
const float SLOW_SPEED = 0.05f;

// the message banner: its place is (400, 500), over the kitchen's
const float MESSAGE_RISE_FIRST = -30.0f;
const float MESSAGE_RISE_SECOND = -40.0f;
const float MESSAGE_RISE_LAST = -60.0f;

// the speed bonus banner: the strip across the screen at the banner's height, and where the title flies from, to and
// through
const float STRIP_LEFT = 0.0f;
const float STRIP_TOP = 500.0f;
const float STRIP_RIGHT = 800.0f;
const float STRIP_BOTTOM = 600.0f;
const float STRIP_SHADE = 0.25f;
const float TITLE_X = 400.0f;
const float TITLE_Y = 550.0f;
const float TITLE_START_X = 1000.0f;
const float TITLE_SLOW_X = 500.0f;
const float TITLE_MIDDLE_X = 300.0f;
const float TITLE_END_X = -200.0f;
const float FADE_SECONDS = 0.3f;
const float FADE_TARGET = 1.0f;
const float SWISH_FIRST_TIME = 0.1f;
const float SWISH_SECOND_TIME = 1.5f;
}

// 0x42d320 (via 0x42c190)
void SoundTrack::add(float theTime, const std::string& theName)
{
	mSounds.insert(std::make_pair(theTime, theName));
	mNext = mSounds.begin();
}

// 0x42cb30
void SoundTrack::update(float theTime)
{
	while (mNext != mSounds.end() && mNext->first < theTime)
	{
		PizzaFrenzy::instance()->mSounds->play(mNext->second, 1.0f, 1.0f);
		++mNext;
	}
	while (mNext != mSounds.begin())
	{
		std::map<float, std::string>::iterator aPrevious = mNext;
		--aPrevious;
		if (aPrevious->first <= theTime)
			break;
		mNext = aPrevious;
	}
}

// 0x42c980
InOutEffect::InOutEffect() : mState(STATE_IDLE), mRemoveWhenDone(false)
{
}

// 0x42c3a0
void InOutEffect::readyIn()
{
	float aLength = mScaleIn.getLastTime();
	if (aLength <= mMoveIn.getLastTime())
		aLength = mMoveIn.getLastTime();
	if (aLength < mOpacityIn.getLastTime())
		aLength = mOpacityIn.getLastTime();
	mTime.set(aLength, aLength);
	mTime.reset();
}

// 0x42c430
void InOutEffect::readyOut()
{
	float aLength = mScaleOut.getLastTime();
	if (aLength <= mMoveOut.getLastTime())
		aLength = mMoveOut.getLastTime();
	if (aLength < mOpacityOut.getLastTime())
		aLength = mOpacityOut.getLastTime();
	mTime.set(aLength, aLength);
	mTime.reset();
}

// 0x42c1a0
void InOutEffect::applyIn()
{
	float aTime = mTime.getProgress();
	mScaleIn.modify(aTime, this);
	mOpacityIn.modify(aTime, this);
	mMoveIn.modify(aTime, this);
	mSoundsIn.update(aTime);
}

// 0x42c200
void InOutEffect::applyOut()
{
	float aTime = mTime.getProgress();
	mScaleOut.modify(aTime, this);
	mOpacityOut.modify(aTime, this);
	mMoveOut.modify(aTime, this);
	mSoundsOut.update(aTime);
}

// 0x42c950
void InOutEffect::reset(bool theRemoveWhenDone)
{
	mRemoveWhenDone = theRemoveWhenDone;
	readyIn();
	applyIn();
	mState = STATE_IDLE;
}

// 0x42c690
void InOutEffect::rewind()
{
	readyIn();
	applyIn();
	mState = STATE_IDLE;
}

// 0x42c6b0
void InOutEffect::start()
{
	addOwnFlags(engine::NODE_LISTENERS);
	switch (mState)
	{
	case STATE_IDLE:
	case STATE_IN:
	case STATE_OUT_WAIT:
		mState = STATE_IN;
		break;
	case STATE_HOLD:
	case STATE_OUT:
	case STATE_DONE:
		mState = STATE_IN;
		readyIn();
		break;
	}
}

// 0x42c4c0
void InOutEffect::update(engine::UpdateInfo& theInfo)
{
	switch (mState)
	{
	case STATE_IN:
		mTime.update(theInfo.mSeconds);
		applyIn();
		if (mTime.isFinished())
			mState = STATE_HOLD;
		break;
	case STATE_OUT_WAIT:
		mTime.update(theInfo.mSeconds);
		applyIn();
		if (mTime.isFinished())
		{
			mState = STATE_OUT;
			readyOut();
			engine::Container::update(theInfo);
			return;
		}
		break;
	case STATE_OUT:
		mTime.update(theInfo.mSeconds);
		applyOut();
		if (mTime.isFinished())
		{
			if (mRemoveWhenDone)
				addOwnFlags(engine::NODE_REMOVE);
			mState = STATE_DONE;
			engine::Container::update(theInfo);
			return;
		}
		break;
	}
	engine::Container::update(theInfo);
}

// 0x42bb30
MessageBanner::MessageBanner()
{
	mEffect = new InOutEffect();
	mEffect->mOpacityIn.addKey({0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mOpacityIn.addKey({1.5f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mOpacityIn.addKey({2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mMoveIn.addKey({0.0f, engine::Point(0.0f, 0.0f), 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mMoveIn.addKey({0.5f, engine::Point(0.0f, MESSAGE_RISE_FIRST), 0.5f, SLOW_SPEED, 0.0f, SLOW_SPEED});
	mEffect->mMoveIn.addKey({1.5f, engine::Point(0.0f, MESSAGE_RISE_SECOND), 0.0f, SLOW_SPEED, 0.5f, 1.0f});
	mEffect->mMoveIn.addKey({2.0f, engine::Point(0.0f, MESSAGE_RISE_LAST), 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mScaleIn.addKey({0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mScaleIn.addKey({0.5f, 0.5f, 0.5f, SLOW_SPEED, 0.0f, SLOW_SPEED});
	mEffect->mScaleIn.addKey({1.5f, 0.6f, 0.0f, SLOW_SPEED, 0.5f, 1.0f});
	mEffect->mScaleIn.addKey({2.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	addChild(mEffect);
	mText = new engine::TextItem();
	mText->setSmoothType(1);
	mText->setFont(std::string(TITLE_FONT));
	mText->setHAlign(1);
	mText->setVAlign(1);
	mEffect->addChild(mText);
	mEffect->reset(false);
}

// 0x44fb70
SpeedBonusBanner::SpeedBonusBanner() : mWaiting(false)
{
}

// 0x44fd10
void SpeedBonusBanner::setup()
{
	removeAllChildren();
	engine::RectangleItem* aStrip = new engine::RectangleItem(STRIP_LEFT, STRIP_TOP, STRIP_RIGHT, STRIP_BOTTOM);
	aStrip->setOpacity(STRIP_SHADE);
	aStrip->setColor(0.0f, 0.0f, 0.0f);
	addChild(aStrip);
	mBlankLabel = new engine::TextItem();
	mBlankLabel->setFont(std::string(TITLE_FONT));
	mBlankLabel->setHAlign(1);
	mBlankLabel->setVAlign(1);
	mBlankLabel->setPosition(TITLE_X, TITLE_Y);
	addChild(mBlankLabel);
	mEffect = new InOutEffect();
	mEffect->mOpacityIn.addKey({0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mOpacityOut.addKey({0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mMoveIn.addKey({0.0f, engine::Point(TITLE_START_X, TITLE_Y), 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mMoveIn.addKey({0.5f, engine::Point(TITLE_SLOW_X, TITLE_Y), 0.5f, SLOW_SPEED, 0.0f, SLOW_SPEED});
	mEffect->mMoveIn.addKey({1.5f, engine::Point(TITLE_MIDDLE_X, TITLE_Y), 0.0f, SLOW_SPEED, 0.5f, 1.0f});
	mEffect->mMoveIn.addKey({2.0f, engine::Point(TITLE_END_X, TITLE_Y), 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mMoveOut.addKey({0.0f, engine::Point(TITLE_END_X, TITLE_Y), 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mScaleIn.addKey({0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mScaleOut.addKey({0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f});
	mEffect->mSoundsIn.add(SWISH_FIRST_TIME, SWISH_SOUND);
	mEffect->mSoundsIn.add(SWISH_SECOND_TIME, SWISH_SOUND);
	addChild(mEffect);
	mTitle = new engine::TextItem();
	mTitle->setFont(std::string(TITLE_FONT));
	mTitle->setHAlign(1);
	mTitle->setVAlign(1);
	mTitle->setText(GT(0x4fff8c));
	mEffect->addChild(mTitle);
	mEffect->reset(false);
	setVisible(false);
	mWaiting = false;
}

// 0x44fcd0
void SpeedBonusBanner::updateBanner()
{
	if (mWaiting && mEffect->isDone())
	{
		mWaiting = false;
		fade(false, FADE_SECONDS, FADE_TARGET, false);
	}
}
