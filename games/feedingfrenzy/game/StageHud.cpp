#include "StageHud.h"
#include "App.h"
#include "FishAnimImage.h"
#include "FishFood.h"
#include "GameSession.h"
#include "GameStrings.h"
#include "ImageNode.h"
#include "PlayerFish.h"
#include "StageMgr.h"

#include <stdio.h>

using namespace engine;

namespace feedingfrenzy
{

// The nodes of the layout the HUD keeps, by name
const char* const SCORE_TEXT_NAME = "scoreText";
const char* const TIMER_TEXT_NAME = "timerText";
const char* const COUNTER_EATEN_NAME = "counterTextA";
const char* const COUNTER_TO_EAT_NAME = "counterTextB";
const char* const LIFE_COUNT_TEXT_NAME = "lifeCountText";
const char* const BONUS_GROUP_NAME = "bonusGroup";
const char* const SPEED_ICON_NAME = "bonusBubbleSpeed";
const char* const DOUBLE_SCORE_ICON_NAME = "bonusBubble2X";
const char* const MENU_BUBBLE_NAME_FORMAT = "menuBubble%d";
const char* const MENU_IMAGE_NAME_FORMAT = "menuImg%d";
// The nodes it looks for at each frame, and the names the meters are given
const char* const TIMER_GROUP_NAME = "timerGroup";
const char* const COUNTER_GROUP_NAME = "counterGroup";
const char* const LIFE_PLUG_NAME = "hudLifePlug";
const char* const FRENZY_TEXT_NAME = "frenzyText";
const char* const DOUBLE_FRENZY_TEXT_NAME = "frenzyText2x";
const char* const FRENZY_MULT_TEXT_NAME = "frenzyMultText";
const char* const GROW_METER_NAME = "growMeter";
const char* const SPECIAL_METER_NAME = "specialMeter";
// The files
const char* const MARKER_IMAGE = "resources\\HUD\\growMeterMarker.png";
const char* const SPLATS_FILE = "config\\hudSplats.xml";
const char* const FRENZY_FONT_FILE = "resources/hud/frenzyFontWhite.xml";
const char* const MULT_FORMAT = "%dX";
// The splat and the sound that warn of the time running out
const char* const TIME_WARNING_SPLAT = "timeWarning";
const char* const TIME_WARNING_SOUND = "timerWarning";
const float TIME_WARNING_X = 288.0f;
const float TIME_WARNING_Y = 458.0f;
const float TIME_WARNING_VOLUME = 1.0f;
const float TIME_WARNING_PITCH = 1.0f;
// The warning is given at each of the last seconds, and none has been given when the HUD starts
const int WARNING_FIRST_SECOND = 10;
const float NO_WARNING_YET = 100000.0f;
// Where the meters sit and how wide the growth's marks go
const float GROW_METER_X = 79.0f;
const float GROW_METER_Y = 58.0f;
const float SPECIAL_METER_X = 435.0f;
const float SPECIAL_METER_Y = 62.0f;
const float SPECIAL_METER_WIDTH = 102.0f;
const float SPECIAL_METER_HEIGHT = 8.0f;
const float MARKER_X = 73.0f;
const float MARKER_Y = 64.0f;
const float MARKER_WIDTH = 239.0f;
// The power's bar is yellow from this share of it, and red under
const float SPECIAL_LOW_SHARE = 0.5f;
const float SPECIAL_BAR_RED = 0.9f;
const float SPECIAL_YELLOW_GREEN = 0.9f;
const float SPECIAL_RED_GREEN = 0.1f;
const float SPECIAL_BAR_BLUE = 0.1f;
// A bonus icon blinks in its last seconds, and shows or hides each time this many seconds have passed
const float BONUS_BLINK_FROM_SECONDS = 2.0f;
const float BONUS_BLINK_SECONDS = 0.15f;
// The most lives the HUD counts
const int MAX_LIVES_SHOWN = 99;
// The letters of the frenzy's word that show for each fish in the chain, up to those of the word, and the chain that
// the word is shown twice from
const int FRENZY_LETTERS = 7;
const int DOUBLE_FRENZY_FIRST_CHAIN = 8;
const int DOUBLE_FRENZY_CHAIN_OFFSET = 7;
// The colour offsets of the frenzy's words and of its multiplier (the words are drawn in white)
const float FRENZY_RED = -1.0f;
const float FRENZY_GREEN = -0.15f;
const float FRENZY_BLUE = -0.9608f;
const float DOUBLE_FRENZY_RED = -0.965f;
const float DOUBLE_FRENZY_GREEN = -0.9f;
const int MULT_YELLOW_FROM = 2;
const int MULT_ORANGE_FROM = 3;

// 0x431f00
StageHud::StageHud() :
    mGrowMeter(nullptr), mSpecialMeter(nullptr), mScoreText(nullptr), mTimerText(nullptr), mLifeCountText(nullptr),
    mCounterTextEaten(nullptr), mCounterTextToEat(nullptr), mBonusGroup(nullptr), mDoubleScoreIcon(nullptr),
    mSpeedBoostIcon(nullptr), mSpeedBoostBlinkSeconds(0.0f), mDoubleScoreBlinkSeconds(0.0f), mGrowthTotal(-1.0f),
    mWarnSeconds(NO_WARNING_YET), mSplats(nullptr)
{
	for (int aBubble = 0; aBubble < HUD_MENU_BUBBLES; aBubble++)
	{
		mMenuBubbles[aBubble] = nullptr;
		mBubbleLevels[aBubble] = 0.0f;
	}
}

// 0x431f30
void StageHud::load(const std::string& theFile, StageMgr* theStage)
{
	mRoot = new ScreenLayout();
	mRoot->loadLayout(theFile);

	mGrowMeter = new GrowMeter();
	mGrowMeter->setName(GROW_METER_NAME);
	mGrowMeter->setPosition(GROW_METER_X, GROW_METER_Y);
	mRoot->addChild(mGrowMeter);

	mSpecialMeter = new GrowMeter();
	mSpecialMeter->setName(SPECIAL_METER_NAME);
	mRoot->addChild(mSpecialMeter);
	mSpecialMeter->setPosition(SPECIAL_METER_X, SPECIAL_METER_Y);
	Rect aSpecialRect;
	aSpecialRect.set(0.0f, 0.0f, SPECIAL_METER_WIDTH, SPECIAL_METER_HEIGHT);
	mSpecialMeter->setRect(aSpecialRect);
	mSpecialMeter->setVisible(false);

	mScoreText = static_cast<TextItem*>(mRoot->getNode(SCORE_TEXT_NAME));
	mTimerText = static_cast<TextItem*>(mRoot->getNode(TIMER_TEXT_NAME));
	mCounterTextEaten = static_cast<TextItem*>(mRoot->getNode(COUNTER_EATEN_NAME));
	mCounterTextToEat = static_cast<TextItem*>(mRoot->getNode(COUNTER_TO_EAT_NAME));
	mLifeCountText = static_cast<TextItem*>(mRoot->getNode(LIFE_COUNT_TEXT_NAME));
	mBonusGroup = mRoot->getNode(BONUS_GROUP_NAME);
	mSpeedBoostIcon = mRoot->getNode(SPEED_ICON_NAME);
	mDoubleScoreIcon = mRoot->getNode(DOUBLE_SCORE_ICON_NAME);

	for (int aBubble = 0; aBubble < HUD_MENU_BUBBLES; aBubble++)
	{
		char aName[32];
		snprintf(aName, sizeof(aName), MENU_BUBBLE_NAME_FORMAT, aBubble);
		Node* aFound = mRoot->getNode(aName);
		if (aFound != nullptr)
			mMenuBubbles[aBubble] = aFound;
	}

	// a mark across the growth's meter where each size after the first begins
	PlayerFish* aPlayer = theStage->getPlayer();
	int aSizeCount = aPlayer->getGrowthLevelCount();
	mGrowthTotal = -1.0f;
	if (aSizeCount > 2)
	{
		int aLastSize = aSizeCount - 1;
		mGrowthTotal = aPlayer->getGrowthScore(aLastSize);
		for (int aSize = 1; aSize < aLastSize; aSize++)
		{
			Image* aMarkerImage = App::instance()->getImage(MARKER_IMAGE);
			ImageNode* aMarker = new ImageNode(aMarkerImage);
			mRoot->addChild(aMarker);
			double aShare = (double)aPlayer->getGrowthScore(aSize) / mGrowthTotal;
			aMarker->setPosition((float)(MARKER_X + aShare * MARKER_WIDTH), MARKER_Y);
		}
	}

	mSplats = new SplatFactory();
	mRoot->addChild(mSplats);
	mSplats->load(SPLATS_FILE);
	mWarnSeconds = NO_WARNING_YET;
}

// 0x432550
void StageHud::setMenuBubble(int theIndex, const std::string& theImage, float theLevel,
                             engine::HsvShiftFilter* theShift)
{
	Image* anImage = FishAnimImage::getMenuImage(theImage, theShift);
	ImageNode* aNode = new ImageNode(anImage);
	aNode->layout();
	char aName[32];
	snprintf(aName, sizeof(aName), MENU_IMAGE_NAME_FORMAT, theIndex);
	Container* aPlace = static_cast<Container*>(mRoot->getNode(aName));
	if (aPlace != nullptr)
		aPlace->addChild(aNode);
	mMenuBubbles[theIndex]->setVisible(true);
	mBubbleLevels[theIndex] = theLevel;
}

// 0x4327c0 (the blinking of the two bonus icons, written once)
void StageHud::blinkBonusIcon(Node* theIcon, float theSecondsLeft, float& theBlinkSeconds, float theSeconds)
{
	if (theSecondsLeft <= 0.0f)
	{
		theIcon->setVisible(false);
		return;
	}
	if (theSecondsLeft >= BONUS_BLINK_FROM_SECONDS)
	{
		theIcon->setVisible(true);
		theBlinkSeconds = BONUS_BLINK_SECONDS;
	}
	else
	{
		theBlinkSeconds -= theSeconds;
		if (theBlinkSeconds < 0.0f)
		{
			theBlinkSeconds = BONUS_BLINK_SECONDS;
			theIcon->setVisible(!theIcon->isVisible());
		}
	}
	mBonusGroup->setVisible(true);
}

// 0x4327c0
void StageHud::update(UpdateInfo& theInfo)
{
	FishFood* aFood = FishFood::instance();
	StageMgr* aStage = aFood->getStageMgr();
	PlayerFish* aPlayer = aStage->getPlayer();
	GameSession* aSession = aFood->getSession();

	if (mGrowthTotal > 0.0f)
	{
		float aShare = aPlayer->getScore() / mGrowthTotal;
		mGrowMeter->setValue(aShare >= 1.0f ? 1.0f : aShare);
	}

	mSpecialMeter->setVisible(aPlayer->isSpecialReady());
	float aSpecialShare = aPlayer->getSpecialProgress();
	mSpecialMeter->setValue(aSpecialShare >= 1.0f ? 1.0f : aSpecialShare);
	if (aSpecialShare >= SPECIAL_LOW_SHARE)
		mSpecialMeter->setBarColor(SPECIAL_BAR_RED, SPECIAL_YELLOW_GREEN, SPECIAL_BAR_BLUE);
	else
		mSpecialMeter->setBarColor(SPECIAL_BAR_RED, SPECIAL_RED_GREEN, SPECIAL_BAR_BLUE);

	mScoreText->setNumber(aSession->getScore());

	switch (aStage->getType())
	{
	case STAGE_NORMAL:
	case STAGE_BONUS:
	case STAGE_MENU:
		mRoot->getNode(TIMER_GROUP_NAME)->setVisible(false);
		break;
	case STAGE_TIMED_NORMAL:
	case STAGE_TIMED_BONUS:
	case STAGE_SURVIVAL:
	{
		mRoot->getNode(TIMER_GROUP_NAME)->setVisible(true);
		float aSecondsLeft = aStage->getSecondsLeft();
		int aWholeSeconds = (int)aSecondsLeft;
		if (aWholeSeconds <= WARNING_FIRST_SECOND && aSecondsLeft < mWarnSeconds)
		{
			mSplats->create(TIME_WARNING_SPLAT, TIME_WARNING_X, TIME_WARNING_Y);
			aFood->playSound(TIME_WARNING_SOUND, TIME_WARNING_VOLUME, TIME_WARNING_PITCH);
			mWarnSeconds = (float)aWholeSeconds;
		}
		break;
	}
	}
	mTimerText->setTime((int)aStage->getSecondsLeft(), false);

	if (aStage->getType() == STAGE_TIMED_BONUS)
	{
		mRoot->getNode(COUNTER_GROUP_NAME)->setVisible(true);
		mCounterTextEaten->setNumber(aStage->countEaten());
		mCounterTextToEat->setNumber(aStage->countFishToEat());
	}
	else
		mRoot->getNode(COUNTER_GROUP_NAME)->setVisible(false);

	for (int aBubble = 0; aBubble < HUD_MENU_BUBBLES; aBubble++)
	{
		if (mMenuBubbles[aBubble] != nullptr)
			mMenuBubbles[aBubble]->setVisible(aStage->getPlayer()->getLevel() <= mBubbleLevels[aBubble]);
	}

	int aLives = aSession->getLives();
	mLifeCountText->setNumber(aLives <= MAX_LIVES_SHOWN ? aLives : MAX_LIVES_SHOWN);

	mBonusGroup->setVisible(false);
	blinkBonusIcon(mDoubleScoreIcon, aPlayer->getDoubleScoreSeconds(), mDoubleScoreBlinkSeconds, theInfo.mSeconds);
	blinkBonusIcon(mSpeedBoostIcon, aPlayer->getSpeedBoostSeconds(), mSpeedBoostBlinkSeconds, theInfo.mSeconds);

	// the frenzy's word grows with the chain, and shows again over it from the eighth fish
	int aChain = aPlayer->getChainCount();
	int aFrenzyLevel = aPlayer->updateFrenzyLevel();
	std::string aWord = aFood->getStrings().get(FRENZY_TEXT_NAME);
	aWord = aWord.substr(0, aChain < FRENZY_LETTERS ? aChain : FRENZY_LETTERS);
	TextItem* aFrenzyText = static_cast<TextItem*>(mRoot->getNode(FRENZY_TEXT_NAME));
	if (aFrenzyText != nullptr)
	{
		aFrenzyText->setText(aWord);
		aFrenzyText->mColorOffset.set(FRENZY_RED, FRENZY_GREEN, FRENZY_BLUE);

		TextItem* aDoubleText = static_cast<TextItem*>(mRoot->getNode(DOUBLE_FRENZY_TEXT_NAME));
		if (aChain < DOUBLE_FRENZY_FIRST_CHAIN)
			aDoubleText->setText("");
		else
		{
			int aDoubleLetters = aChain - DOUBLE_FRENZY_CHAIN_OFFSET;
			aWord = aFood->getStrings().get(FRENZY_TEXT_NAME);
			aWord = aWord.substr(0, aDoubleLetters < FRENZY_LETTERS ? aDoubleLetters : FRENZY_LETTERS);
			aDoubleText->setText(aWord);
			aDoubleText->mColorOffset.set(DOUBLE_FRENZY_RED, DOUBLE_FRENZY_GREEN, 0.0f);
		}

		TextItem* aMultText = static_cast<TextItem*>(mRoot->getNode(FRENZY_MULT_TEXT_NAME));
		char aMult[16];
		snprintf(aMult, sizeof(aMult), MULT_FORMAT, aFrenzyLevel);
		aMultText->setFont(App::instance()->getFont(FRENZY_FONT_FILE));
		if (aFrenzyLevel < MULT_ORANGE_FROM)
		{
			if (aFrenzyLevel < MULT_YELLOW_FROM)
				aMultText->mColorOffset.set(0.0f, 0.0f, 0.0f);
			else
				aMultText->mColorOffset.set(FRENZY_RED, FRENZY_GREEN, FRENZY_BLUE);
		}
		else
			aMultText->mColorOffset.set(DOUBLE_FRENZY_RED, DOUBLE_FRENZY_GREEN, 0.0f);
		aMultText->setText(aMult);
	}

	// the lives' counter is covered in the timed game, which has none
	mRoot->getNode(LIFE_PLUG_NAME)->setVisible(aSession->getMode() == GAME_MODE_TIMED);
}

} // namespace feedingfrenzy
