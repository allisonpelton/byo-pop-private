#include "GameHud.h"

#include "DrawContext.h"
#include "Draws.h"
#include "FeedingFrenzyTwo.h"
#include "GameData.h"
#include "Image.h"
#include "NumberText.h"
#include "Params.h"
#include "PlayerSpawn.h"
#include "ScoreSplat.h"
#include "Stage.h"
#include "StringTable.h"

#include <cmath>
#include <cstdio>

using namespace engine;

namespace
{

// The growth meter's colour (0x3f666666, 0x3f666666 and 0x3dcccccd in 0x482f20 (feedingfrenzy2))
const float GROWTH_RED = 0.9f;
const float GROWTH_GREEN = 0.9f;
const float GROWTH_BLUE = 0.1f;
// The arrows sit this far above the bar's foot, less half their height (0x566868 (feedingfrenzy2))
const double GROWTH_ARROW_RAISE = 3.0;
// The most lives the text shows (0x402550 (feedingfrenzy2))
const int MOST_LIVES_SHOWN = 99;
// A placed text's colour (0x408df0 (feedingfrenzy2), opaque) and its colour mode (the text's slot 0x50)
const int PLACED_TEXT_RED = 10;
const int PLACED_TEXT_GREEN = 0xd9;
const int PLACED_TEXT_BLUE = 0;
const int PLACED_TEXT_COLOR_MODE = 2;
// The seconds the time bonus's text says (0x494510 (feedingfrenzy2))
const int TIME_BONUS_SECONDS = 3;
// The suck's meter: red below half a charge, yellow from it (0x455a50 (feedingfrenzy2))
const float SUCK_LOW_CHARGE = 0.5f;
const float SUCK_RED = 0.9f;
const float SUCK_LOW_GREEN = 0.1f;
const float SUCK_HIGH_GREEN = 0.9f;
const float SUCK_BLUE = 0.1f;
// The clock's warning: under this many seconds, at this offset from the clock's group (0x44c910 (feedingfrenzy2))
const int TIME_WARNING_SECONDS = 11;
const double TIME_WARNING_X = 26.0;
const double TIME_WARNING_Y = 23.0;
// The chomp's bursts have three variants of each size (0x41cdc7 (feedingfrenzy2))
const int CHOMP_SPLAT_VARIANTS = 3;
// 0x54ddb0 (feedingfrenzy2)
const float HALF = 0.5f;
// The frenzy's word has a letter for each fish of a step (0x4605d0 (feedingfrenzy2))
const int FRENZY_STEP_FISH = 7;
// The multiplier's colour mode (0x4605d0 (feedingfrenzy2), the text's slot 0x50)
const int FRENZY_MULTIPLIER_COLOR_MODE = 2;

// The frenzy's colours by the multiplier, from 0 (the static initializer 0x4d7310 (feedingfrenzy2): 0x5acb60 to
// 0x5acbb0, read from 0x5acb50 for the step before)
const int FRENZY_COLOR_COUNT = 6;
const int FRENZY_COLOR_BYTES[FRENZY_COLOR_COUNT][3] = {
	{0x9d, 0x9d, 0x9d}, {0x32, 0xc6, 0x27}, {0xf7, 0x94, 0x1c}, {0x4a, 0xdf, 0xff}, {0xea, 0xe3, 0x21}, {0xed, 0x21, 0x2a},
};

// 0x5083da (feedingfrenzy2): a colour of four bytes; the frenzy's are opaque
Color frenzyColor(int theIndex)
{
	const int* aBytes = FRENZY_COLOR_BYTES[theIndex];
	return Color(channelOf(aBytes[0]), channelOf(aBytes[1]), channelOf(aBytes[2]), channelOf(0xff));
}

// 0x459e00 and 0x459670 (feedingfrenzy2): a rectangle's width and height
float widthOf(const RectF& theRect)
{
	return (float)((double)theRect.right - theRect.left);
}

float heightOf(const RectF& theRect)
{
	return (float)((double)theRect.bottom - theRect.top);
}

// A part of the layout by its name (0x46de50 (feedingfrenzy2)), null when it has none
template <class T>
T* partOf(ScreenLayout* theLayout, const char* theName)
{
	return static_cast<T*>(theLayout->findComponent(theName));
}

// 0x44ca60 (feedingfrenzy2) with its hours left out when there are none: "mm:ss", or "H:mm:ss" from an hour (by
// GetTimeFormatA). port: the locale's time separator, which the original reads (LOCALE_STIME), is the colon of the
// user's locale
std::string clockText(int theSeconds)
{
	char aText[32];
	int anHours = theSeconds / 60 / 60;
	if (anHours < 1)
		snprintf(aText, sizeof(aText), "%02d:%02d", theSeconds / 60 % 60, theSeconds % 60);
	else
		snprintf(aText, sizeof(aText), "%d:%02d:%02d", anHours, theSeconds / 60 % 60, theSeconds % 60);
	return aText;
}

} // namespace

GrowMeter::GrowMeter(float theWidth, float theHeight)
	: mMaximum(0.0f), mValue(0.0f), mWidth(theWidth), mHeight(theHeight), mFilledWidth(0.0f)
{
	mOwnStates |= STATE_VISIBLE;
	addState(STATE_VISIBLE);
	addState(STATE_BOUNDS_DIRTY);
}

void GrowMeter::paint(DrawContext* theContext)
{
	Color& aColor = theContext->back().mColor;
	aColor.r = mColor.r;
	aColor.g = mColor.g;
	aColor.b = mColor.b;
	theContext->fillRect(fabsf(mFilledWidth * mScale.x), fabsf(mHeight * mScale.y));
}

void GrowMeter::computeBounds()
{
	mBounds = RectF(mPosition.x, mPosition.y, mWidth + mPosition.x, mHeight + mPosition.y);
	removeState(STATE_BOUNDS_DIRTY);
}

void GrowMeter::setValue(float theValue)
{
	if (theValue < 0.0f)
		theValue = 0.0f;
	else if (mMaximum < theValue)
		theValue = mMaximum;
	mValue = theValue;
	if (0.0f < mMaximum)
		mFilledWidth = (theValue / mMaximum) * mWidth;
}

GameHud::GameHud(Stage* theStage)
	: mShownScore(0), mBonus2XShown(0), mBonusSpeedShown(0), mShownSeconds(0), mStage(theStage)
{
}

void GameHud::update(UpdateContext& theContext)
{
	ScreenLayout::update(theContext);
	scoreSplatQueue().update(theContext.mElapsed);
}

void GameHud::setup(Theme* theStageTheme)
{
	load("resource/HUD/hudLayout.xml", getApp()->mShellTheme.get());
	mGrowthBar = partOf<Container>(this, "growthBar");
	mGrowthBar->computeBounds();
	mGrowthMeter = new GrowMeter(widthOf(mGrowthBar->getBounds()), heightOf(mGrowthBar->getBounds()));
	mGrowthMeter->setName("growMeter");
	mGrowthMeter->setPosition(0.0f, 0.0f);
	mGrowthMeter->setColor(GROWTH_RED, GROWTH_GREEN, GROWTH_BLUE);
	mGrowthMeter->computeBounds();
	mGrowthBar->addChild(mGrowthMeter.get());
	mGrowthArrows = new Container();
	mGrowthArrows->setPosition(mGrowthBar->getPosition());
	addChild(mGrowthArrows.get());

	Container* aSpecialBar = partOf<Container>(this, "specialBar");
	aSpecialBar->computeBounds();
	mSuckMeter = new GrowMeter(widthOf(aSpecialBar->getBounds()), heightOf(aSpecialBar->getBounds()));
	mSuckMeter->setName("specialMeter");
	mSuckMeter->setPosition(0.0f, 0.0f);
	mSuckMeter->setVisible(false);
	mSuckMeter->setMaximum(1.0f);
	aSpecialBar->addChild(mSuckMeter.get());

	mScoreText = partOf<TextItem>(this, "scoreText");
	mTimerGroup = partOf<Component>(this, "timerGroup");
	mTimerText = partOf<TextItem>(this, "timerText");
	mCounterGroup = partOf<Component>(this, "counterGroup");
	mCounterTextA = partOf<TextItem>(this, "counterTextA");
	mCounterTextB = partOf<TextItem>(this, "counterTextB");
	mLifeCountText = partOf<TextItem>(this, "lifeCountText");
	mStageNameText = partOf<TextItem>(this, "stageNameText");
	mBonusGroup = partOf<Component>(this, "bonusGroup");
	mBonusBubbleSpeed = partOf<Component>(this, "bonusBubbleSpeed");
	mAbilityText = partOf<Component>(this, "abilityText");
	mAbilityPlug = partOf<Component>(this, "abilityPlug");
	mLifePlug = partOf<Component>(this, "lifePlug");
	mGrowthText = partOf<Component>(this, "growthText");
	mBossText = partOf<Component>(this, "bossText");
	mBonusBubbleSpeed->setVisible(false);
	mBonusSpeedShown = 0;
	mBonusBubble2X = partOf<Component>(this, "bonusBubble2X");
	mBonusBubble2X->setVisible(false);
	mBonus2XShown = 0;
	// The menu's four images (menuImg0 to menuImg3), each put in a selector (Selector, 0xd4 bytes): not yet translated
	mJumpHeightGroup = partOf<Component>(this, "jumpHeightGroup");
	mJumpHeight = partOf<Component>(this, "jumpHeight");
	mJumpHeightGroup->setVisible(false);
	// The jump's height effect (InOutFx, 0x2f0 bytes, after 0x473150 (feedingfrenzy2) with 3.0: its keys of position
	// and scale, at (121, 104), in jumpHeightGroup and hidden) and the medals medalPersonalBest and medalNewRecord in
	// it: not yet translated, as the jump is not
	mSplats = new SplatFactory();
	addChild(mSplats);
	mSplats->load("resource/HUD/hudSplats.xml");
	mPlacedTexts = new Container();
	addChild(mPlacedTexts.get());
	mPointsBubbles = new Container();
	addChild(mPointsBubbles.get());
}

void GameHud::reset()
{
	// The four menu images' selectors emptied (their slot 0x10c): not yet translated, as the selectors are not
	showScore(getApp()->mGameData->getScore());
	mShownSeconds = 0;
	mJumpHeightGroup->setVisible(false);
	// The jump's height effect (+0x230) hidden: not yet translated, as the jump is not
	scoreSplatQueue().clear();
	mPointsBubbles->removeAllChildren();
	bool isBoss = mStage->isBoss();
	mGrowthText->setVisible(!isBoss);
	mBossText->setVisible(isBoss);
	mLifePlug->setVisible(getApp()->mGameData->getMode() == GAME_MODE_TIMED);
}

void GameHud::setGrowthGoals(PlayerSpawn* theSpawn)
{
	mGrowthArrows->removeAllChildren();
	int aPhaseCount = theSpawn->getPhaseCount();
	float aMaximum = 0.0f;
	if (aPhaseCount != 0)
		aMaximum = (float)theSpawn->getPhase(aPhaseCount - 1).mFoodGoal;
	mGrowthMeter->setMaximum(aMaximum);
	if (aPhaseCount <= 1 || mStage->isBoss())
		return;
	// growthBar's slots 0x04 and 0x08 (its x and y) read, for nothing kept
	float aWidth = widthOf(mGrowthMeter->getBounds());
	float aHeight = heightOf(mGrowthMeter->getBounds());
	Ref<Surface> anArrow = mStage->getTheme()->getImage("growthArrow");
	anArrow->setPivot(PIVOT_CENTER);
	int aHalfHeight = anArrow->getHeight() / 2;
	float aTop = (float)(((double)aHeight - aHalfHeight) - GROWTH_ARROW_RAISE);
	for (int i = 0; i < aPhaseCount - 1; i++)
	{
		Image* anImage = new Image(anArrow.get());
		float aLeft = ((float)theSpawn->getPhase(i).mFoodGoal / aMaximum) * aWidth;
		anImage->setPosition(aLeft, aTop);
		mGrowthArrows->addChild(anImage);
	}
}

void GameHud::showScore(int theScore)
{
	mShownScore = theScore;
	mScoreText->setText(groupedNumber(theScore));
}

void GameHud::addShownScore(int thePoints)
{
	int aScore = mShownScore + thePoints;
	// The cold path 0x4edfc3 (feedingfrenzy2)
	if (aScore <= 0)
		aScore = 0;
	mShownScore = aScore;
	mScoreText->setText(groupedNumber(aScore));
}

void GameHud::showTime(int theSeconds)
{
	mTimerText->setText(clockText(theSeconds));
	if (theSeconds < TIME_WARNING_SECONDS && theSeconds < mShownSeconds)
	{
		float aX = (float)((double)mTimerGroup->getX() + TIME_WARNING_X);
		float aY = (float)((double)mTimerGroup->getY() + TIME_WARNING_Y);
		mSplats->spawn("timeWarning", aX, aY);
		// The stage theme's sound timerWarning: not yet translated, as the sounds are not
	}
	mShownSeconds = theSeconds;
}

void GameHud::showLives(int theLives)
{
	int aLives = MOST_LIVES_SHOWN;
	if (theLives < 100)
		aLives = theLives;
	// 0x408920 (feedingfrenzy2): the number (by 0x41b1a0 (feedingfrenzy2), "%d" then 0x44e580's grouping by the
	// locale, which leaves two digits alone) after the empty string
	char aText[16];
	snprintf(aText, sizeof(aText), "%d", aLives);
	mLifeCountText->setText(aText);
}

void GameHud::showSuckCharge(float theCharge)
{
	float aCharge = theCharge;
	if (aCharge < 0.0f)
		aCharge = 0.0f;
	else if (1.0f < aCharge)
		aCharge = 1.0f;
	mSuckMeter->setValue(aCharge);
	float aGreen = theCharge < SUCK_LOW_CHARGE ? SUCK_LOW_GREEN : SUCK_HIGH_GREEN;
	mSuckMeter->setColor(SUCK_RED, aGreen, SUCK_BLUE);
}

void GameHud::splatChomp(int theRank, const Vector2f& thePoint)
{
	int aVariant = drawWhole(1, CHOMP_SPLAT_VARIANTS);
	if (theRank < 0)
		theRank = 0;
	if (theRank > 2)
		theRank = 2;
	const char* aFormat = theRank == 0 ? "chompSplatMini%d" : theRank == 1 ? "chompSplatSmall%d" : "chompSplatLarge%d";
	char aName[32];
	snprintf(aName, sizeof(aName), aFormat, aVariant);
	mSplats->spawn(aName, thePoint.x, thePoint.y);
}

void GameHud::showPlacedText(const std::string& theText, const Vector2f& thePoint)
{
	Ref<ScoreSplat> aSplat = new ScoreSplat();
	Font* aFont = getApp()->mShellTheme->getFont("frenzyFontBold");
	aSplat->setupRising(theText, aFont, thePoint);
	aSplat->setColor(Color(channelOf(PLACED_TEXT_RED), channelOf(PLACED_TEXT_GREEN), channelOf(PLACED_TEXT_BLUE), 1.0f));
	aSplat->setColorMode(PLACED_TEXT_COLOR_MODE);
	aSplat->setContainer(mPlacedTexts.get());
	scoreSplatQueue().push(aSplat.get());
}

void GameHud::showTimeBonus(const Vector2f& thePoint)
{
	std::string aText = getEngine()->formatMessage(getApp()->mStrings->getString("timeSplat"), TIME_BONUS_SECONDS);
	showPlacedText(aText, thePoint);
}

void GameHud::splatFrenzy(int theStep)
{
	char anId[32];
	snprintf(anId, sizeof(anId), "frenzy%d", theStep);
	Engine* anEngine = getEngine();
	float aY = (float)((double)anEngine->getHeight() * HALF);
	float aX = (float)((double)anEngine->getWidth() * HALF);
	Splat* aSplat = mSplats->spawn("frenzySplat", aX, aY);
	aSplat->setText(getApp()->mStrings->getString(anId));
	aSplat->setColor(frenzyColor(theStep));
}

void GameHud::showFrenzy(int theCount, int theMultiplier)
{
	int aLit = theCount;
	int aStep = theMultiplier;
	if (0 < theCount)
	{
		aLit = theCount % FRENZY_STEP_FISH;
		if (aLit == 0)
		{
			aLit = FRENZY_STEP_FISH;
			aStep = theMultiplier - 1;
		}
	}
	StringTable* aStrings = getApp()->mStrings.get();
	if (FRENZY_STEP_FISH <= aLit)
		aLit = FRENZY_STEP_FISH;
	std::string aWord = aStrings->getString("frenzyMeter");
	TextItem* aLitText = partOf<TextItem>(this, "frenzyTextFg");
	aLitText->setColor(frenzyColor(aStep));
	aLitText->setText(aWord.substr(0, aLit));
	TextItem* anUnlitText = partOf<TextItem>(this, "frenzyTextBg");
	anUnlitText->setColor(frenzyColor(aStep - 1));
	anUnlitText->setText(aWord.substr(aLit, FRENZY_STEP_FISH));
	TextItem* aMultiplierText = partOf<TextItem>(this, "frenzyMultText");
	if (theMultiplier <= 1)
	{
		aMultiplierText->setVisible(false);
		return;
	}
	aMultiplierText->setVisible(true);
	aMultiplierText->setColorMode(FRENZY_MULTIPLIER_COLOR_MODE);
	aMultiplierText->setColor(frenzyColor(theMultiplier - 1));
	aMultiplierText->setText(getEngine()->formatMessage(aStrings->getString("frenzyMult"), theMultiplier));
}

void GameHud::showSuck(bool canSuck)
{
	mSuckMeter->setVisible(canSuck);
	mAbilityText->setVisible(canSuck);
	mAbilityPlug->setVisible(!canSuck);
}
