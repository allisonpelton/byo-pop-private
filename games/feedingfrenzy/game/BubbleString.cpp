#include "BubbleString.h"
#include "App.h"
#include "Modifiers.h"
#include "Random.h"

#include <math.h>
#include <stdio.h>

using namespace engine;

namespace feedingfrenzy
{

// The letter's pictures and where its letter sits in the bubble
const char* const LETTER_FONT = "resources\\HUD\\frenzyFontWhite.xml";
const char* const BUBBLE_IMAGE = "resources\\fx\\BubbleLetterBubble.png";
const char* const BURST_FILE = "config\\FX\\playerEatBonusFx.xml";
const float LETTER_X = -3.0f;
const float LETTER_Y = 0.0f;
// What the font's white is moved by, to the bubble's blue
const float LETTER_COLOR_RED = -0.965f;
const float LETTER_COLOR_GREEN = -0.9f;
// The seconds a bubble takes to grow
const float GROW_SECONDS = 0.2f;
// The wave each letter bobs on: across by this many cycles, down by a random number of cycles, and how far down
const float WAVE_PERIOD_X = 0.5f;
const float WAVE_PERIOD_Y_LOW = 2.5f;
const float WAVE_PERIOD_Y_HIGH = 3.5f;
const float WAVE_SIZE_X = 0.0f;
const float WAVE_SIZE_Y = 0.2f;
// The draw that no wave uses
const float UNUSED_DRAW_LOW = 0.0f;
const float UNUSED_DRAW_HIGH = 2.0f;

// 0x43e4d9
BubbleLetter::BubbleLetter() :
    mCountdown(0.0f), mGrowSeconds(GROW_SECONDS), mDelay(0.0f), mIsSpace(false), mState(LETTER_HIDDEN)
{
}

// 0x43e260
void BubbleLetter::init(char theLetter, float theDelay)
{
	mText = new TextItem();
	addChild(mText);
	mText->setFont(App::instance()->getFont(LETTER_FONT));
	mText->mColorOffset.set(LETTER_COLOR_RED, LETTER_COLOR_GREEN, 0.0f);
	mText->setHAlign(1);
	char aLetterText[2] = {theLetter, 0};
	mText->setText(aLetterText);
	mText->setVAlign(1);
	mText->setPosition(LETTER_X, LETTER_Y);

	Image* anImage = App::instance()->getImage(BUBBLE_IMAGE);
	anImage->setPivot(1);
	mBubble = new ImageNode(anImage);
	addChild(mBubble);

	mBurstFx = new ParticleSystem();
	mBurstFx->load(BURST_FILE);
	addChild(mBurstFx);
	setVisible(false);
	mDelay = theDelay;
	if (theLetter == ' ')
		mIsSpace = true;
}

// 0x43e030
void BubbleLetter::start(const Point& thePlace)
{
	if (!mIsSpace)
	{
		setPosition(thePlace);
		setFlags(NODE_LISTENERS);
		mText->setVisible(true);
		mBubble->setVisible(true);
		setScale(0.0f, 0.0f);
		setVisible(true);
		if (mDelay <= 0.0f)
		{
			mCountdown = mGrowSeconds;
			mState = LETTER_GROWING;
		}
		else
		{
			mCountdown = mDelay;
			mState = LETTER_WAITING;
		}
	}
}

// 0x43e1f0
void BubbleLetter::stop()
{
	if (mState == LETTER_SHOWN || mState == LETTER_GROWING)
	{
		mBurstFx->stopEmitting();
		mBurstFx->startEmitting();
		mBubble->setVisible(false);
		mText->setVisible(false);
		mState = LETTER_HIDDEN;
	}
}

// 0x43e120
void BubbleLetter::update(UpdateInfo& theInfo)
{
	if (mState == LETTER_GROWING)
	{
		mCountdown = mCountdown - theInfo.mSeconds;
		if (mCountdown < 0.0f)
			mCountdown = 0.0f;
		float aScale = 1.0f - mCountdown / mGrowSeconds;
		setScale(aScale, aScale);
		if (mCountdown == 0.0f)
			mState = LETTER_SHOWN;
	}
	else if (mState == LETTER_WAITING)
	{
		mCountdown = mCountdown - theInfo.mSeconds;
		if (mCountdown < 0.0f)
		{
			mState = LETTER_GROWING;
			mCountdown = mGrowSeconds;
			Container::update(theInfo);
			return;
		}
	}
	Container::update(theInfo);
}

// 0x43ea90
BubbleString::BubbleString() : mSpacing(0.0f)
{
}

// 0x43eb20
void BubbleString::setText(const std::string& theText, float theLetterDelay, float theSpacing, float theStartDelay)
{
	for (unsigned int aLetterIndex = 0; aLetterIndex < theText.size(); aLetterIndex++)
	{
		BubbleLetter* aLetter = new BubbleLetter();
		addChild(aLetter);
		aLetter->init(theText[aLetterIndex], (float)aLetterIndex * theLetterDelay + theStartDelay);

		float aPeriodY = randomFloat(Point(WAVE_PERIOD_Y_LOW, WAVE_PERIOD_Y_HIGH));
		WaveModifier* aWave = new WaveModifier(WAVE_PERIOD_X, aPeriodY, WAVE_SIZE_X, WAVE_SIZE_Y, 0.0f);
		// the draw is made and its value is not used
		randomFloat(Point(UNUSED_DRAW_LOW, UNUSED_DRAW_HIGH));
		aLetter->addListener(aWave);
		mLetters.push_back(aLetter);
	}
	mSpacing = theSpacing;
}

// 0x43e580
float BubbleString::getWidth()
{
	return (float)((int)mLetters.size() - 1) * mSpacing;
}

// 0x43e5f0
void BubbleString::show(const Point& thePlace)
{
	setPosition(thePlace);
	Point aLetterPlace(0.0f, 0.0f);
	for (unsigned int aLetterIndex = 0; aLetterIndex < mLetters.size(); aLetterIndex++)
	{
		mLetters[aLetterIndex]->start(aLetterPlace);
		aLetterPlace.x = aLetterPlace.x + mSpacing;
	}
}

// 0x43e6c0
void BubbleString::hide()
{
	for (unsigned int aLetterIndex = 0; aLetterIndex < mLetters.size(); aLetterIndex++)
		mLetters[aLetterIndex]->stop();
}

} // namespace feedingfrenzy
