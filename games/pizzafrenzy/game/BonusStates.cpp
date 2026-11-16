#include "BonusStates.h"
#include "App.h"
#include "UserGameData.h"

using namespace pizzafrenzy;

namespace
{
const char* const SOUND_FLIP = "popup_flipover";
const char* const SOUND_WAIT = "popup_wait";
const char* const SOUND_POP = "popup";
const char* const SOUND_PULLDOWN = "story_ScreenPulldown";
const char* const MUSIC_LEVEL_COMPLETE = "music_levelComplete";
const char* const ACTION_PAUSE_GAME = "pauseGame";
const char* const ACTION_BLUR = "blur";
const char* const ACTION_MENU = "menu";
const char* const ACTION_LEVEL_END = "levelEnd";
// the round's end titles
const int STRING_ROUND_MISSED = 0xd0;
const int STRING_ROUND_PERFECT = 0xd1;
const int STRING_ROUND_OVER = 0xd9;
const int STRING_ROUND_NO_TIME = 0xda;

// The seconds left at which the waiting sound starts
const float HURRY_SECONDS = 5.0f;

// The pauses of the scoring: before the first topping, and between the toppings and the stages after
const float SCORE_START_SECONDS = 0.5f;
const float SCORE_STEP_SECONDS = 0.1f;

// The pauses of a pizza's way in: after the target pizza, between its toppings, and after the last
const float ARRIVE_AFTER_PIZZA_SECONDS = 0.5f;
const float ARRIVE_TOPPING_SECONDS = 0.1f;
const float ARRIVE_AFTER_TOPPINGS_SECONDS = 0.3f;

// The title shows for two and a half seconds, and goes for two
const float TITLE_SHOWN_SECONDS = 2.5f;
const float TITLE_LEAVING_SECONDS = 2.0f;

void playSound(const std::string& theName)
{
	PizzaFrenzy::instance()->mSounds->play(theName, 1.0f, 1.0f);
}
}

// 0x460390
void BonusPlayState::enter()
{
	mGame->mScreen->connect();
	mGame->mPlaying = true;
	playSound(SOUND_FLIP);
	mVoice = PizzaFrenzy::instance()->mSounds->getVoice(SOUND_WAIT);
	if (mVoice != nullptr)
		mVoice->setLoop(true);
	mHurryPlayed = false;
	engine::App::instance()->mRightDownSignal.connect(mGame, &BonusGame::onRightClick);
}

// 0x45dd10
void BonusPlayState::exit()
{
	if (mVoice != nullptr)
		mVoice->stop();
	playSound(SOUND_FLIP);
	engine::App::instance()->mRightDownSignal.disconnect(mGame);
}

// 0x45c5e0
void BonusPlayState::update(engine::UpdateInfo& theInfo)
{
	mGame->mTimeLeft = mGame->mTimeLeft - theInfo.mSeconds;
	mGame->mTimerText->setTime((int)mGame->mTimeLeft, true);
	if (mGame->mTimeLeft < HURRY_SECONDS && !mHurryPlayed)
	{
		if (mVoice != nullptr)
			mVoice->play();
		mHurryPlayed = true;
	}
	if (mGame->mTimeLeft <= 0.0f)
		mGame->endPizza();
}

// 0x460010
void BonusPlayState::action(const std::string& theAction)
{
	Topping* aTopping = PizzaFrenzy::instance()->mTileManifest->getTopping(theAction);
	if (aTopping != nullptr)
		mGame->selectTopping(aTopping);
}

// 0x460040
void BonusScoreState::enter()
{
	mGame->mStats->mBonusSecondsLeft = (int)mGame->mTimeLeft;
	mStage = STAGE_START;
	mTimer = SCORE_START_SECONDS;
	mGame->mPlaying = false;
	mGame->selectTopping(nullptr);
}

// 0x45eb00
void BonusScoreState::update(engine::UpdateInfo& theInfo)
{
	mTimer = mTimer - theInfo.mSeconds;
	if (!(mTimer < 0.0f))
		return;
	if (mStage == STAGE_START)
	{
		mIndex = 0;
		mStage = STAGE_SCORING;
		return;
	}
	if (mStage == STAGE_SCORING)
	{
		bool aMore = mGame->scoreTopping(mIndex);
		if (aMore)
			mIndex++;
		else
			mStage = STAGE_LEAVING;
		mTimer = SCORE_STEP_SECONDS;
		return;
	}
	if (mStage == STAGE_LEAVING)
	{
		mGame->mTargetRamp.setForward(false);
		mGame->mUserRamp.setForward(false);
		mStage = STAGE_OUT;
	}
	mGame->mTargetRamp.update(theInfo.mSeconds);
	mGame->mTargetMotion.modify(mGame->mTargetRamp.getProgress(), mGame->mTargetPizza);
	mGame->mUserRamp.update(theInfo.mSeconds);
	mGame->mUserMotion.modify(mGame->mUserRamp.getProgress(), mGame->mUserPizza);
	mGame->mButtonsMotion.modify(mGame->mUserRamp.getProgress(), mGame->mToppingButtons);
	if (mGame->mTargetRamp.isFinished() && mGame->mUserRamp.isFinished())
	{
		if (0.0f < mGame->mTimeLeft)
			mGame->mStates.changeState(new BonusNextPizzaState(mGame));
		else
			mGame->mStates.changeState(new BonusEndState(mGame));
	}
}

// 0x45d030
void BonusNextPizzaState::enter()
{
	mShown = 0;
	mTimer = 0;
	mStage = 0;
	mGame->mPlaying = false;
	std::vector<engine::Ref<Pizza>>& aPizzas = mGame->mLevel->mPizzas;
	if (mGame->mNextPizza == aPizzas.size())
	{
		mGame->mStates.changeState(new BonusEndState(mGame));
		return;
	}
	Pizza* aPizza = aPizzas[mGame->mNextPizza++];
	mGame->mPizzasLeft--;
	mGame->mPizzaCountText->setNumber(mGame->mPizzasLeft, "");
	mGame->beginPizza(aPizza);
	mCount = (int)mGame->mTargetToppings.size();
	mGame->mTargetRamp.setForward(true);
	mGame->mUserRamp.setForward(true);
}

// 0x45d9e0
void BonusNextPizzaState::update(engine::UpdateInfo& theInfo)
{
	mTimer = mTimer - theInfo.mSeconds;
	if (!(mTimer < 0.0f))
		return;
	switch (mStage)
	{
	case 0:
		mGame->mTargetRamp.start();
		playSound(SOUND_PULLDOWN);
		mStage = 1;
		mTimer = 0;
		break;
	case 1:
		mGame->mTargetRamp.update(theInfo.mSeconds);
		mGame->mTargetMotion.modify(mGame->mTargetRamp.getProgress(), mGame->mTargetPizza);
		if (mGame->mTargetRamp.isFinished())
		{
			mStage = 2;
			mTimer = ARRIVE_AFTER_PIZZA_SECONDS;
		}
		break;
	case 2:
		if (mShown < mCount)
		{
			mGame->mTargetToppings[mShown]->setVisible(true);
			playSound(SOUND_POP);
			mShown++;
			mTimer = ARRIVE_TOPPING_SECONDS;
		}
		else
		{
			mStage = 3;
			mTimer = ARRIVE_AFTER_TOPPINGS_SECONDS;
		}
		break;
	case 3:
		mGame->mUserRamp.start();
		playSound(SOUND_PULLDOWN);
		mStage = 4;
		mTimer = 0;
		break;
	case 4:
		mGame->mUserRamp.update(theInfo.mSeconds);
		mGame->mUserMotion.modify(mGame->mUserRamp.getProgress(), mGame->mUserPizza);
		mGame->mButtonsMotion.modify(mGame->mUserRamp.getProgress(), mGame->mToppingButtons);
		if (mGame->mUserRamp.isFinished())
			mStage = 5;
		break;
	case 5:
		mGame->mStates.changeState(new BonusPlayState(mGame));
		break;
	}
}

// 0x45d1b0
void BonusEndState::enter()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mGame->mPlaying = false;
	GameStats* aStats = mGame->getStats();
	int aText;
	if (aStats->mBonusSecondsLeft == 0)
		aText = STRING_ROUND_NO_TIME;
	else if (0 < aStats->mMissedToppings)
		aText = STRING_ROUND_MISSED;
	else if (aStats->mMissedToppings == 0 && aStats->mOkayToppings == 0 && aStats->mGoodToppings == 0 &&
		0 < aStats->mPerfectToppings)
		aText = STRING_ROUND_PERFECT;
	else
		aText = STRING_ROUND_OVER;
	mTitle = makeEndTitle(mGame->mScreen, aText);
	mTitleLeaving = false;
	mTimer = TITLE_SHOWN_SECONDS;
	mGame->finishLevel();
	mGame->mScreen->disconnect();
	anApp->mMusicPlayer->play(anApp->mTileManifest->getMusic(MUSIC_LEVEL_COMPLETE));
}

// 0x45d4b0: the title shows and then its letters leave, and the application is told the level is over
void BonusEndState::update(engine::UpdateInfo& theInfo)
{
	mTimer = mTimer - theInfo.mSeconds;
	if (!mTitleLeaving)
	{
		if (mTimer < 0.0f)
		{
			mTitle->leave();
			mTitleLeaving = true;
			mTimer = TITLE_LEAVING_SECONDS;
		}
		return;
	}
	if (mTimer < 0.0f)
		PizzaFrenzy::instance()->screenAction(ACTION_LEVEL_END);
}

// 0x4026e0
void BonusGameState::enter()
{
	mApp->mScreens->showScreen(mApp->mDecorateScreen, 0);
	mApp->saveGame();
	mApp->mMode->setPaused(false);
}

// 0x402720
void BonusGameState::update(engine::UpdateInfo& theInfo)
{
	mApp->mMode->update(theInfo);
}

// 0x407790: the player's pause or a lost focus pauses the round; its end unlocks the toppings, and the application
// goes on to the summary; the rest are the buttons of the toppings
void BonusGameState::action(const std::string& theAction)
{
	if (theAction.compare(ACTION_PAUSE_GAME) == 0 || theAction.compare(ACTION_BLUR) == 0
	    || theAction.compare(ACTION_MENU) == 0)
	{
		mApp->mMode->setPaused(true);
		mApp->pauseGame(this);
		return;
	}
	if (theAction.compare(ACTION_LEVEL_END) == 0)
	{
		mApp->mUserGameData->mUnlockTopping = true;
		mApp->mUserGameData->save();
		mApp->endLevel();
		return;
	}
	mApp->mMode->action(theAction);
}
