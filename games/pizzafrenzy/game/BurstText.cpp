#include "BurstText.h"
#include "App.h"
#include "Font.h"
#include "Modifiers.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
const float GROW_SECONDS = 0.2f;  // 0x3e4ccccd
const float TEXT_X = -3.0f;       // the text stands a little left of the letter's point
const int ALIGN_CENTRE = 1;

// the letters bob with a period of two seconds, each a tenth of a second behind the one before
const float WAVE_PERIOD = 2.0f;
const float WAVE_STEP = 0.1f;

// the end titles
const char* const TITLE_FONT = "res\\fonts\\titleFont.xml";
const char* const BURST_EFFECT = "BurstFx";
const float TITLE_DELAY_STEP = 0.1f;
const float TITLE_SPACING = 4.0f;
const float TITLE_FIRST_DELAY = 0.0f;
const float TITLE_WAVE_SIZE = 0.05f;
const float TITLE_X = 400.0f;
const float TITLE_Y = 200.0f;
}

// 0x42b230
BurstLetter::BurstLetter() :
	mTimeLeft(0), mGrowSeconds(GROW_SECONDS), mDelay(0), mChar(0), mIsSpace(false), mStage(STAGE_IDLE)
{
}

// 0x42b020
int BurstLetter::setLetter(char theChar, engine::Font* theFont, engine::ParticleSystemDef* theBurst, float theDelay)
{
	mChar = theChar;
	mText = new engine::TextItem();
	addChild(mText);
	mText->setFont(theFont);
	mText->setHAlign(ALIGN_CENTRE);
	mText->setText(std::string(1, theChar));
	mText->setVAlign(ALIGN_CENTRE);
	mText->setPosition(TEXT_X, 0.0f);
	if (theBurst != nullptr)
	{
		mBurst = new engine::SignalParticleSystem();
		mBurst->setUp(theBurst);
		addChild(mBurst);
	}
	setVisible(false);
	mDelay = theDelay;
	if (theChar == ' ')
		mIsSpace = true;
	// the character's width (0x47e6c0, as TextItem's)
	char aString[2] = {theChar, '\0'};
	return theFont->getStringWidth(aString);
}

// 0x42af20
void BurstLetter::start(const engine::Point& thePosition)
{
	if (mIsSpace)
		return;
	setPosition(thePosition);
	addOwnFlags(engine::NODE_LISTENERS);
	mText->setVisible(true);
	setScale(0.0f, 0.0f);
	setVisible(true);
	if (0.0f < mDelay)
	{
		mTimeLeft = mDelay;
		mStage = STAGE_WAITING;
		return;
	}
	mTimeLeft = mGrowSeconds;
	mStage = STAGE_GROWING;
}

// 0x42ab80
void BurstLetter::update(engine::UpdateInfo& theInfo)
{
	if (mStage == STAGE_GROWING)
	{
		mTimeLeft = mTimeLeft - theInfo.mSeconds;
		if (mTimeLeft < 0.0f)
			mTimeLeft = 0;
		float aScale = 1.0f - mTimeLeft / mGrowSeconds;
		setScale(aScale, aScale);
		if (mTimeLeft == 0.0f)
			mStage = STAGE_SHOWN;
	}
	else if (mStage == STAGE_WAITING)
	{
		mTimeLeft = mTimeLeft - theInfo.mSeconds;
		if (mTimeLeft < 0.0f)
		{
			mTimeLeft = mGrowSeconds;
			mStage = STAGE_GROWING;
		}
	}
	Container::update(theInfo);
}

// 0x42afb0
void BurstLetter::leave()
{
	if (mStage != STAGE_SHOWN && mStage != STAGE_GROWING)
		return;
	if (mBurst != nullptr)
	{
		mBurst->stop();
		mBurst->start();
	}
	mText->setVisible(false);
	mStage = STAGE_IDLE;
}

// 0x42b800
BurstText::BurstText() : mSpacing(0), mWidth(0)
{
	setSmoothType(1);
}

// 0x42b8a0
void BurstText::setText(const std::string& theText, float theDelayStep, float theSpacing, engine::Font* theFont,
	engine::ParticleSystemDef* theBurst, float theFirstDelay, float theWaveSize)
{
	removeAllChildren();
	mLetters.clear();
	setFont(theFont);
	mWidth = 0;
	for (size_t i = 0; i < theText.size(); i++)
	{
		BurstLetter* aLetter = new BurstLetter();
		addChild(aLetter);
		float anIndex = (float)i;
		int aWidth = aLetter->setLetter(theText[i], theFont, theBurst, anIndex * theDelayStep + theFirstDelay);
		mWidth = (float)aWidth + theSpacing + mWidth;
		if (0.0f < theWaveSize)
			aLetter->addListener(new engine::WaveModifier(0.0f, WAVE_PERIOD, 0.0f,
				(float)theFont->getHeight() * theWaveSize, anIndex * WAVE_STEP));
		mLetters.push_back(aLetter);
	}
	mSpacing = theSpacing;
}

// 0x42b350
void BurstText::start(const engine::Point& thePosition)
{
	setPosition(thePosition.x - mWidth * 0.5f, thePosition.y);
	engine::Point aPoint(0.0f, 0.0f);
	for (size_t i = 0; i < mLetters.size(); i++)
	{
		BurstLetter* aLetter = mLetters[i];
		aPoint.x = (float)mFont->getLeadingWidth(aLetter->getChar()) + aPoint.x;
		aLetter->start(aPoint);
		aPoint.x = (float)mFont->getTrailingWidth(aLetter->getChar()) + aPoint.x + mSpacing;
	}
}

// 0x42b460
void BurstText::leave()
{
	for (size_t i = 0; i < mLetters.size(); i++)
		mLetters[i]->leave();
}

BurstText* pizzafrenzy::makeEndTitle(engine::Container* theScreen, int theStringId)
{
	engine::App* anApp = engine::App::instance();
	ParticleEffect* anEffect = PizzaFrenzy::instance()->mTileManifest->getParticleEffect(BURST_EFFECT);
	BurstText* aTitle = new BurstText();
	std::string aText = anApp->loadString(theStringId);
	aTitle->setText(aText, TITLE_DELAY_STEP, TITLE_SPACING, anApp->getFont(TITLE_FONT),
		anEffect != nullptr ? anEffect->mDef.get() : nullptr, TITLE_FIRST_DELAY, TITLE_WAVE_SIZE);
	theScreen->addChild(aTitle);
	aTitle->start(engine::Point(TITLE_X, TITLE_Y));
	return aTitle;
}
