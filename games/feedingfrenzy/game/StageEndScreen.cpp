#include "StageEndScreen.h"
#include "App.h"
#include "FadeContainer.h"
#include "FishAnimImage.h"
#include "FishFood.h"
#include "FoodBankRank.h"
#include "FunFacts.h"
#include "GameSession.h"
#include "GameStrings.h"
#include "ImageNode.h"
#include "Random.h"
#include "StageInfo.h"
#include "StageMgr.h"
#include "TextItem.h"

#include <algorithm>
#include <stdio.h>

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The nodes of the layouts the screen shows and hides, by name
const char* const BONUS_COUNT_GROUP_NAME = "BonusCountGroup";
const char* const TIME_BONUS_GROUP_NAME = "TimeBonusGroup";
const char* const TIME_MODE_GROUP_NAME = "TimeModeGroup";
const char* const UPPER_TIME_MODE_GROUP_NAME = "UpperTimeModeGroup";
const char* const FUN_FACT_GROUP_NAME = "FunFactGroup";
const char* const FOOD_BANK_GROUP_NAME = "FoodBankGroup";
const char* const UPPER_FOOD_BANK_GROUP_NAME = "UpperFoodBankGroup";
const char* const MENU_IMAGES_NAME = "MenuImages";
const char* const UPPER_FOOD_BANK_CATEGORY_NAME = "upperFoodBankCategoryText";
const char* const TIME_BONUS_HEADER_NAME = "TimeBonusHeader";
const char* const TIME_BONUS_TIME_LEFT_GROUP_NAME = "TimeBonusTimeLeftGroup";
const char* const TIME_BONUS_SCORE_GROUP_NAME = "TimeBonusScoreGroup";
const char* const TIME_BONUS_FINAL_SCORE_GROUP_NAME = "TimeBonusFinalScoreGroup";
// The texts and numbers of the layouts
const char* const BONUS_RATIO_TEXT_NAME = "bonusRatioText";
const char* const BONUS_PERCENT_TEXT_NAME = "bonusPercentText";
const char* const CONGRATS_TEXT_NAME = "congrats";
const char* const TIME_BONUS_FINAL_SCORE_NAME = "TimeBonusFinalScore";
const char* const TIME_BONUS_TIME_LEFT_NAME = "TimeBonusTimeLeft";
const char* const TIME_BONUS_SCORE_NAME = "TimeBonusScore";
const char* const TIME_MODE_TIME_LEFT_NAME = "TimeModeTimeLeft";
const char* const UPPER_TIME_MODE_TIME_LEFT_NAME = "upperTimeModeTimeLeft";
const char* const BONUS_FOOD_BANK_TEXT_NAME = "bonusFoodBankText";
const char* const UPPER_FOOD_BANK_TEXT_NAME = "upperFoodBankText";
const char* const FUN_FACT_TEXT_NAME = "FunFactText";
// The pictures of the bonus stage's fish: the one the normal game has and the one the timed game has
const char* const BONUS_MENU_IMAGE_NAME = "bonusMenuImg";
const char* const BONUS_MENU_IMAGE_TIMED_NAME = "bonusMenuImgTimed";
// The names of the menu's nodes are made from these and the kind's number
const char* const MENU_TEXT_FORMAT = "menuText%d";
const char* const MENU_IMAGE_FORMAT = "menuImg%d";
// The game's texts
const char* const BONUS_STATS_TEXT = "bonusStats";
const char* const NO_TIME_BONUS_TEXT = "noTimeBonus";
const char* const RANK_UP_TEXT = "rankUpFormat";
// The formats of the numbers the screen writes
const char* const RATIO_FORMAT = "%02d / %02d";
const char* const PERCENT_FORMAT = "%d %%";
const char* const COUNT_FORMAT = "%01d";
// The registry key of the food bank
const char* const FOOD_BANK_KEY = "foodBank";
// The font and width a fun fact is set in, and the fun fact the first stage of the timed game has
const char* const FUN_FACT_FONT = "resources/menus/infoText14.xml";
const float FUN_FACT_WIDTH = 370.0f;
const int TIMED_FIRST_STAGE_FACT = 50;
// The sounds
const char* const TICK_SOUND = "mouseOver";
const float TICK_VOLUME = 0.9f;
const float TICK_PITCH = 1.5f;
const char* const BITE_SOUND = "bite1";
const char* const RANK_SOUND = "bite4";
const char* const HEADER_SOUND = "starPickup";
const char* const NO_BONUS_SOUND = "playerDie";
const char* const DONE_SOUND = "playerSpawn";
const float RESULT_SOUND_VOLUME = 1.0f;
const float RESULT_SOUND_PITCH = 1.0f;
// The splat each bite of the menu makes, where the first one is, and how far the next ones are from it
const char* const MENU_SPLAT_NAME = "chompSplatLevelEnd";
const int MENU_SPLAT_FIRST_X = 155;
const int MENU_SPLAT_SPACING = 76;
const float MENU_SPLAT_Y = 200.0f;
// A mini splat is chosen by a number from this to just below that, which nothing uses
const int MINI_SPLAT_FIRST = 1;
const int MINI_SPLAT_LAST = 3;
// The kind of fish that the bonus bubbles are counted as (the last of the menu)
const int BONUS_KIND = RESULT_KIND_COUNT - 1;
// The points a second of time bonus is worth
const int TIME_BONUS_POINTS_PER_SECOND = 50;
// The seconds between the steps of the counters
const float PERCENT_STEP_SECONDS = 0.05f;
const float TIME_BONUS_STEP_SECONDS = 0.05f;
const float FINAL_SCORE_STEP_SECONDS = 0.05f;
const float TIME_MODE_STEP_SECONDS = 0.05f;
const float FOOD_BANK_STEP_SECONDS = 0.03f;
const float MENU_STEP_SECONDS = 0.05f;
// A count of the final score has at most this many digits in a step
const int FINAL_SCORE_STEP_DIGITS = 3;
// The seconds each pause lasts
const float BONUS_START_WAIT_SECONDS = 0.8f;
const float TIME_BONUS_WAIT_SECONDS = 0.8f;
const float FOOD_BANK_WAIT_SECONDS = 0.6f;
const float MENU_BITES_WAIT_SECONDS = 0.6f;
const float MENU_BITE_WAIT_SECONDS = 0.5f;
const float MENU_COUNT_WAIT_SECONDS = 0.6f;
const float RANK_WAIT_SECONDS = 0.6f;
const float FUN_FACT_WAIT_SECONDS = 0.75f;
const float TIME_LEFT_WAIT_SECONDS = 0.6f;
// The fun fact fades in over this many seconds, to full opacity
const float FUN_FACT_FADE_SECONDS = 0.4f;
const float FUN_FACT_FADE_TARGET = 1.0f;

// A node of the screen is shown or hidden
void showNode(ScreenLayout* theScreen, const char* theName, bool theVisible)
{
	Node* aNode = theScreen->getNode(theName);
	if (aNode != nullptr)
		aNode->setVisible(theVisible);
}

// A number is put in a text node of the screen
void setNodeNumber(ScreenLayout* theScreen, const char* theName, int theNumber)
{
	TextItem* aText = static_cast<TextItem*>(theScreen->getNode(theName));
	if (aText != nullptr)
		aText->setNumber(theNumber);
}

// A time (in seconds) is put in a text node of the screen
void setNodeTime(ScreenLayout* theScreen, const char* theName, int theSeconds, bool theWithHours)
{
	TextItem* aText = static_cast<TextItem*>(theScreen->getNode(theName));
	if (aText != nullptr)
		aText->setTime(theSeconds, theWithHours);
}

// The sound of a counting step
void playTick()
{
	FishFood::instance()->playSound(TICK_SOUND, TICK_VOLUME, TICK_PITCH);
}

// A sound of the results
void playResultSound(const char* theName)
{
	FishFood::instance()->playSound(theName, RESULT_SOUND_VOLUME, RESULT_SOUND_PITCH);
}
}

// 0x429a50
StageEndScreen::StageEndScreen(StageMgr* theStage) :
    StageInfoScreen(theStage), mState(RESULTS_IDLE), mNextState(RESULTS_IDLE), mWaitSeconds(0.0f), mStageType(0),
    mGameMode(0), mScoreBase(0), mFoodBankBase(0), mTimeBase(0), mMenuIndex(0), mMenuCount(0), mMenuText(),
    mMenuLevels()
{
}

// 0x426920
bool StageEndScreen::loadLayout(std::string theFile)
{
	bool aLoaded = StageInfoScreen::loadLayout(theFile);
	findMenuTexts();
	return aLoaded;
}

// 0x4263d0
void StageEndScreen::findMenuTexts()
{
	for (int aKind = 0; aKind < RESULT_KIND_COUNT; aKind++)
	{
		char aName[32];
		snprintf(aName, sizeof(aName), MENU_TEXT_FORMAT, aKind);
		mMenuText[aKind] = static_cast<TextItem*>(getNode(aName));
	}
}

// 0x4290f0
void StageEndScreen::setUpResults(int theStageType, int theGameMode, SplatFactory* theEatSplats)
{
	mStageType = theStageType;
	mGameMode = theGameMode;
	mEatSplats = theEatSplats;
	if (theEatSplats != nullptr)
		addChild(theEatSplats);

	setUpFunFact();
	setUpFoodBank();
	if (mStageType >= STAGE_TIMED_BONUS && mStageType <= STAGE_SURVIVAL)
		setUpBonusResults();
	else
		setUpMenuResults();
}

// 0x426f10
void StageEndScreen::setMenuBubble(int theIndex, const std::string& theImage, float theLevel, HsvShiftFilter* theShift)
{
	Image* anImage = FishAnimImage::getMenuImage(theImage, theShift);
	ImageNode* aNode = new ImageNode(anImage);
	aNode->layout();
	char aName[32];
	snprintf(aName, sizeof(aName), MENU_IMAGE_FORMAT, theIndex);
	Container* aPlace = static_cast<Container*>(getNode(aName));
	if (aPlace != nullptr)
		aPlace->addChild(aNode);

	bool aTimedGame = FishFood::instance()->getSession()->getMode() == GAME_MODE_TIMED;
	showNode(this, BONUS_MENU_IMAGE_NAME, !aTimedGame);
	showNode(this, BONUS_MENU_IMAGE_TIMED_NAME, aTimedGame);
	mMenuLevels[theIndex] = theLevel;
}

// 0x427a60
void StageEndScreen::setUpFunFact()
{
	FunFacts* aFacts = FishFood::instance()->getFunFacts();
	Font* aFont = App::instance()->getFont(FUN_FACT_FONT);
	std::string aFact;
	if (mStage->getStageInfo()->getIndex() < 2 && mGameMode == GAME_MODE_TIMED)
		aFact = aFacts->getFact(TIMED_FIRST_STAGE_FACT, aFont, FUN_FACT_WIDTH);
	else
		aFact = aFacts->getRandomFact(aFont, FUN_FACT_WIDTH);
	setText(FUN_FACT_TEXT_NAME, aFact);
}

// 0x426440
void StageEndScreen::setUpFoodBank()
{
	FishFood* aFood = FishFood::instance();
	FoodBank& aFoodBank = aFood->getFoodBank();
	mFoodBankBase = aFoodBank.getCount();
	if (aFood->getSession()->getMode() != GAME_MODE_TIMED)
	{
		aFoodBank.add(mStage->countEatenOfAll());
		aFoodBank.save(FOOD_BANK_KEY);
	}
	setNodeNumber(this, BONUS_FOOD_BANK_TEXT_NAME, mFoodBankBase);
	setNodeNumber(this, UPPER_FOOD_BANK_TEXT_NAME, mFoodBankBase);
}

// 0x426750 (the normal game), 0x426560 (the timed game)
void StageEndScreen::setUpMenuResults()
{
	bool aTimedGame = mGameMode != GAME_MODE_NORMAL;
	showNode(this, BONUS_COUNT_GROUP_NAME, false);
	showNode(this, TIME_BONUS_GROUP_NAME, false);
	showNode(this, TIME_MODE_GROUP_NAME, false);
	showNode(this, UPPER_TIME_MODE_GROUP_NAME, false);
	showNode(this, FUN_FACT_GROUP_NAME, false);
	showNode(this, FOOD_BANK_GROUP_NAME, false);
	showNode(this, UPPER_FOOD_BANK_CATEGORY_NAME, false);
	showNode(this, UPPER_FOOD_BANK_GROUP_NAME, !aTimedGame);
	showNode(this, MENU_IMAGES_NAME, true);
	if (aTimedGame)
	{
		GameSession* aSession = FishFood::instance()->getSession();
		aSession->resetSecondsPlayed();
		aSession->addSecondsPlayed(mStage->getSecondsLeft());
	}
	mMenuIndex = 0;
	waitThen(RESULTS_MENU_BITES, MENU_BITES_WAIT_SECONDS);
}

// 0x427fd0 (the normal game), 0x4285e0 (the timed game)
void StageEndScreen::setUpBonusResults()
{
	FishFood* aFood = FishFood::instance();
	GameSession* aSession = aFood->getSession();
	bool aTimedGame = mGameMode != GAME_MODE_NORMAL;
	float aSecondsLeft = mStage->getSecondsLeft();
	float aBonusSeconds = std::max(aSecondsLeft, 0.0f);

	if (aTimedGame)
		setNodeTime(this, TIME_MODE_TIME_LEFT_NAME, (int)aBonusSeconds, false);
	showNode(this, BONUS_COUNT_GROUP_NAME, true);
	showNode(this, TIME_BONUS_GROUP_NAME, true);
	showNode(this, TIME_MODE_GROUP_NAME, false);
	showNode(this, UPPER_TIME_MODE_GROUP_NAME, false);
	showNode(this, FUN_FACT_GROUP_NAME, false);
	showNode(this, FOOD_BANK_GROUP_NAME, !aTimedGame);
	showNode(this, UPPER_FOOD_BANK_GROUP_NAME, false);
	showNode(this, MENU_IMAGES_NAME, false);

	int aEaten = mStage->countEaten();
	int aToEat = mStage->countFishToEat();
	char aRatio[32];
	snprintf(aRatio, sizeof(aRatio), RATIO_FORMAT, aEaten, aToEat);
	setText(BONUS_RATIO_TEXT_NAME, aRatio);
	setText(CONGRATS_TEXT_NAME, aFood->getStrings().get(BONUS_STATS_TEXT));

	float aBonus = aBonusSeconds * TIME_BONUS_POINTS_PER_SECOND;
	mScoreBase = aSession->getScore();
	setNodeNumber(this, TIME_BONUS_FINAL_SCORE_NAME, mScoreBase);
	setNodeTime(this, TIME_BONUS_TIME_LEFT_NAME, (int)aBonusSeconds, false);
	showNode(this, TIME_BONUS_HEADER_NAME, false);
	showNode(this, TIME_BONUS_TIME_LEFT_GROUP_NAME, false);
	showNode(this, TIME_BONUS_SCORE_GROUP_NAME, false);
	showNode(this, TIME_BONUS_FINAL_SCORE_GROUP_NAME, false);
	if (mStage->isTimed())
		aSession->addScore((int)aBonus);
	else
		setText(TIME_BONUS_HEADER_NAME, aFood->getStrings().get(NO_TIME_BONUS_TEXT));

	int aPercent = aToEat > 0 ? (int)((double)aEaten / (double)aToEat * 100.0) : 0;
	mBonusPercentCounter.init(0, aPercent, PERCENT_STEP_SECONDS, true);
	mBonusPercentCounter.connect(this, &StageEndScreen::onBonusPercent);
	mBonusPercentCounter.begin();
	mState = RESULTS_BONUS_START;
	mWaitSeconds = BONUS_START_WAIT_SECONDS;

	if (aTimedGame && mStage->isTimed())
		aSession->addSecondsPlayed(mStage->getSecondsLeft());
}

// 0x426a10
void StageEndScreen::onBonusPercent(int theNumber)
{
	playTick();
	char aText[32];
	snprintf(aText, sizeof(aText), PERCENT_FORMAT, theNumber);
	setText(BONUS_PERCENT_TEXT_NAME, aText);
}

// 0x426b10
void StageEndScreen::onTimeBonusStep(int theNumber)
{
	setNodeTime(this, TIME_BONUS_TIME_LEFT_NAME, getWholeSecondsLeft() - theNumber, false);
	setNodeNumber(this, TIME_BONUS_SCORE_NAME, theNumber * TIME_BONUS_POINTS_PER_SECOND);
	playTick();
}

// 0x426be0
void StageEndScreen::onTimeModeStep(int theNumber)
{
	setNodeTime(this, TIME_MODE_TIME_LEFT_NAME, mTimeBase + theNumber, false);
	playTick();
}

// 0x426c70
void StageEndScreen::onFinalScoreStep(int theNumber)
{
	setNodeNumber(this, TIME_BONUS_FINAL_SCORE_NAME, mScoreBase + theNumber);
	setNodeNumber(this, TIME_BONUS_SCORE_NAME,
	              std::max(0, getWholeSecondsLeft() * TIME_BONUS_POINTS_PER_SECOND - theNumber));
	playTick();
}

// 0x426d50
void StageEndScreen::onBonusFoodBankStep(int theNumber)
{
	setNodeNumber(this, BONUS_FOOD_BANK_TEXT_NAME, mFoodBankBase + theNumber);
	playTick();
}

// 0x426dd0
void StageEndScreen::onMenuCountStep(int theNumber)
{
	char aText[32];
	snprintf(aText, sizeof(aText), COUNT_FORMAT, theNumber);
	if (mMenuText[mMenuIndex] != nullptr)
		mMenuText[mMenuIndex]->setText(aText);
	setNodeNumber(this, UPPER_FOOD_BANK_TEXT_NAME, mFoodBankBase + mMenuCount - theNumber);
	playTick();
}

// 0x4291b0
void StageEndScreen::startTimeBonusScore()
{
	if (!mStage->isTimed())
	{
		startFoodBankCount();
		return;
	}
	showNode(this, TIME_BONUS_SCORE_GROUP_NAME, true);
	mNextState = RESULTS_TIME_BONUS;
	mTimeCounter.init(0, getWholeSecondsLeft(), TIME_BONUS_STEP_SECONDS, false);
	mTimeCounter.connect(this, &StageEndScreen::onTimeBonusStep);
	mTimeCounter.begin();
	mState = RESULTS_WAIT;
	mWaitSeconds = TIME_BONUS_WAIT_SECONDS;
}

// 0x428c70
void StageEndScreen::startFinalScore()
{
	if (!mStage->isTimed())
	{
		mState = RESULTS_IDLE;
		return;
	}
	showNode(this, TIME_BONUS_FINAL_SCORE_GROUP_NAME, true);
	mNextState = RESULTS_FINAL_SCORE;
	mFinalScoreCounter.init(0, getWholeSecondsLeft() * TIME_BONUS_POINTS_PER_SECOND, FINAL_SCORE_STEP_SECONDS, true);
	mFinalScoreCounter.setMaxDigits(FINAL_SCORE_STEP_DIGITS);
	mFinalScoreCounter.connect(this, &StageEndScreen::onFinalScoreStep);
	mFinalScoreCounter.begin();
	mState = RESULTS_WAIT;
	mWaitSeconds = TIME_BONUS_WAIT_SECONDS;
}

// 0x428d50
void StageEndScreen::startFoodBankCount()
{
	mFoodBankCounter.init(0, mStage->countEatenOfAll(), FOOD_BANK_STEP_SECONDS, false);
	mFoodBankCounter.connect(this, &StageEndScreen::onBonusFoodBankStep);
	mFoodBankCounter.begin();
	waitThen(RESULTS_FOOD_BANK, FOOD_BANK_WAIT_SECONDS);
}

// 0x428dc0
void StageEndScreen::startMenuCount(int theIndex)
{
	if (mMenuLevels[theIndex] == 0.0f)
		theIndex = BONUS_KIND;
	mMenuIndex = theIndex;
	if (theIndex == BONUS_KIND)
		mMenuCount = mStage->countBonusEaten();
	else
		mMenuCount = mStage->countEatenOfLevel(mMenuLevels[theIndex]);
	mMenuCounter.init(mMenuCount, 0, MENU_STEP_SECONDS, true);
	mMenuCounter.connect(this, &StageEndScreen::onMenuCountStep);
	mMenuCounter.begin();
}

// 0x428e60
void StageEndScreen::startTimeModeCount()
{
	showNode(this, TIME_MODE_GROUP_NAME, true);
	mTimeCounter.init(0, getWholeSecondsLeft(), TIME_MODE_STEP_SECONDS, true);
	mTimeCounter.begin();
	mTimeBase = (int)FishFood::instance()->getSession()->getSecondsPlayed();
	mTimeCounter.disconnectAll();
	mTimeCounter.connect(this, &StageEndScreen::onTimeModeStep);
	setNodeTime(this, TIME_MODE_TIME_LEFT_NAME, mTimeBase, false);
	waitThen(RESULTS_TIME_MODE, FOOD_BANK_WAIT_SECONDS);
}

// 0x429280
void StageEndScreen::startMenuCounting()
{
	waitThen(RESULTS_MENU_COUNTING, MENU_COUNT_WAIT_SECONDS);
	startMenuCount(0);
}

// 0x425eb0
void StageEndScreen::startRankUp()
{
	mMenuIndex = 0;
	waitThen(RESULTS_RANK, RANK_WAIT_SECONDS);
}

// 0x425ee0
void StageEndScreen::startFunFact()
{
	mMenuIndex = 0;
	waitThen(RESULTS_FUN_FACT, FUN_FACT_WAIT_SECONDS);
}

// 0x425f10
void StageEndScreen::startTimeLeft()
{
	waitThen(RESULTS_TIME_LEFT, TIME_LEFT_WAIT_SECONDS);
}

void StageEndScreen::waitThen(int theNextState, float theSeconds)
{
	mNextState = theNextState;
	mState = RESULTS_WAIT;
	mWaitSeconds = theSeconds;
}

int StageEndScreen::getWholeSecondsLeft() const
{
	return (int)mStage->getSecondsLeft();
}

// 0x4292b0
void StageEndScreen::update(UpdateInfo& theInfo)
{
	ScreenLayout::update(theInfo);
	if (!mAcceptable)
		return;

	switch (mState)
	{
	case RESULTS_BONUS_START:
		mWaitSeconds -= theInfo.mSeconds;
		if (mWaitSeconds < 0.0f)
			mState = RESULTS_BONUS_PERCENT;
		break;

	case RESULTS_BONUS_PERCENT:
		mBonusPercentCounter.update(theInfo);
		if (mBonusPercentCounter.isDone())
		{
			startTimeBonusScore();
			showNode(this, TIME_BONUS_HEADER_NAME, true);
			if (!mStage->isTimed())
			{
				playResultSound(NO_BONUS_SOUND);
			}
			else
			{
				playResultSound(HEADER_SOUND);
				showNode(this, TIME_BONUS_TIME_LEFT_GROUP_NAME, true);
			}
		}
		break;

	case RESULTS_TIME_BONUS:
		mTimeCounter.update(theInfo);
		if (mTimeCounter.isDone())
			startFinalScore();
		break;

	case RESULTS_FINAL_SCORE:
		mFinalScoreCounter.update(theInfo);
		if (mFinalScoreCounter.isDone())
		{
			if (mGameMode == GAME_MODE_TIMED)
				startTimeModeCount();
			else
				startFoodBankCount();
		}
		break;

	case RESULTS_WAIT:
		mWaitSeconds -= theInfo.mSeconds;
		if (mWaitSeconds < 0.0f)
			mState = mNextState;
		break;

	case RESULTS_FOOD_BANK:
		mFoodBankCounter.update(theInfo);
		if (mFoodBankCounter.isDone())
		{
			playResultSound(DONE_SOUND);
			mState = RESULTS_IDLE;
		}
		break;

	case RESULTS_TIME_MODE:
		mTimeCounter.update(theInfo);
		if (mTimeCounter.isDone())
		{
			playResultSound(DONE_SOUND);
			mState = RESULTS_IDLE;
		}
		break;

	case RESULTS_MENU_BITES:
		mWaitSeconds -= theInfo.mSeconds;
		if (mWaitSeconds < 0.0f)
			showMenuBite();
		break;

	case RESULTS_MENU_COUNTING:
		mMenuCounter.update(theInfo);
		if (mMenuCounter.isDone())
		{
			if (mMenuIndex < BONUS_KIND)
			{
				mFoodBankBase += mMenuCount;
				startMenuCount(mMenuIndex + 1);
			}
			else
			{
				playResultSound(HEADER_SOUND);
				startRankUp();
			}
		}
		break;

	case RESULTS_RANK:
		setText(UPPER_FOOD_BANK_CATEGORY_NAME,
		        FishFood::instance()->getStrings().format(
		            RANK_UP_TEXT, {getFoodBankRankName(FishFood::instance()->getFoodBank())}));
		showNode(this, UPPER_FOOD_BANK_CATEGORY_NAME, true);
		startFunFact();
		playResultSound(RANK_SOUND);
		break;

	case RESULTS_FUN_FACT:
	{
		FadeContainer* aFactGroup = static_cast<FadeContainer*>(getNode(FUN_FACT_GROUP_NAME));
		if (aFactGroup != nullptr)
		{
			aFactGroup->setOpacity(0.0f);
			aFactGroup->fade(true, FUN_FACT_FADE_SECONDS, FUN_FACT_FADE_TARGET, false);
		}
		playResultSound(DONE_SOUND);
		mState = RESULTS_IDLE;
		break;
	}

	case RESULTS_TIME_LEFT:
		showNode(this, UPPER_TIME_MODE_GROUP_NAME, true);
		setNodeTime(this, UPPER_TIME_MODE_TIME_LEFT_NAME, getWholeSecondsLeft(), true);
		playResultSound(HEADER_SOUND);
		startFunFact();
		break;
	}
}

// 0x4292b0 (the update's state of the bites)
void StageEndScreen::showMenuBite()
{
	if (mMenuLevels[mMenuIndex] == 0.0f)
		mMenuIndex = BONUS_KIND;
	int aCount;
	if (mMenuIndex == BONUS_KIND)
		aCount = mStage->countBonusEaten();
	else
		aCount = mStage->countEatenOfLevel(mMenuLevels[mMenuIndex]);
	char aText[32];
	snprintf(aText, sizeof(aText), COUNT_FORMAT, aCount);
	if (mMenuText[mMenuIndex] != nullptr)
		mMenuText[mMenuIndex]->setText(aText);

	randomInt(MINI_SPLAT_FIRST, MINI_SPLAT_LAST);
	if (mEatSplats != nullptr)
		mEatSplats->create(MENU_SPLAT_NAME, (float)(mMenuIndex * MENU_SPLAT_SPACING + MENU_SPLAT_FIRST_X), MENU_SPLAT_Y);
	playResultSound(BITE_SOUND);

	if (mMenuIndex == BONUS_KIND)
	{
		if (mGameMode == GAME_MODE_NORMAL)
			startMenuCounting();
		else
			startTimeLeft();
	}
	else
	{
		mMenuIndex++;
		mWaitSeconds = MENU_BITE_WAIT_SECONDS;
	}
}
