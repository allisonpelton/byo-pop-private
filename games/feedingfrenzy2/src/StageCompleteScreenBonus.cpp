#include "StageCompleteScreen.h"

#include "FeedingFrenzyTwo.h"
#include "GameData.h"
#include "NumberText.h"
#include "PlayerRanks.h"
#include "Spawn.h"
#include "Stage.h"
#include "StringTable.h"
#include "TextRollUp.h"

#include <climits>
#include <cmath>
#include <cstdio>

using namespace engine;

namespace
{

const float NEXT_WAIT = 0.5f;               // before each next step (0x493810, 0x5019b3, 0x53b4ad and 0x501996)
const float ROLL_UP_WAIT = 0.05f;           // between the numbers' steps (0x493810 and 0x53b4ad (feedingfrenzy2))
const float FOOD_BANK_STEP_WAIT = 0.05f;    // between the food bank's steps (0x53dbf6 (feedingfrenzy2))
const float RANK_WAIT = 0.97f;              // from the rank's header to the rank
const int FISH_LARGEST_STEP = 5;            // the fish eaten and their share
const int CLOCK_LARGEST_STEP = 10;          // the time left
const int SCORE_LARGEST_STEP = 500;         // the time bonus and the final score
const int TIME_BONUS_PER_SECOND = 100;
const double PERCENT = 100.0;
// The food bank steps by one while fewer than this are left, else by ten (0x53db89 (feedingfrenzy2))
const int FOOD_BANK_FINE_STEPS = 11;
const int FOOD_BANK_COARSE_STEP = 10;

// The x87's conversion of a double to an int, truncating (0x4c3ae0 (feedingfrenzy2)): out of range, or not a number,
// it gives the integer indefinite, the least int
int truncateToInt(double theValue)
{
	if (!(theValue > (double)INT_MIN - 1.0 && theValue < (double)INT_MAX + 1.0))
		return INT_MIN;
	return (int)theValue;
}

std::string formatNumber(int theNumber)
{
	char aText[16];
	snprintf(aText, sizeof(aText), "%d", theNumber);
	return aText;
}

// The strings' entry (0x48a5f0 (feedingfrenzy2))
std::string appString(const char* theName)
{
	return getApp()->mStrings->getString(theName);
}

} // namespace

// The food bank counted up to its total, then the rank (StageCompleteScreenBonus::FoodBankState, 0x24 bytes, its
// constructor 0x51d253 (feedingfrenzy2))
class StageCompleteScreenBonus::FoodBankState : public CountState
{
public:
	explicit FoodBankState(StageCompleteScreenBonus* theScreen)
	    : CountState(theScreen), mWait(0), mPhase(FOOD_BANK_COUNTING), mShown(0)
	{
	}

	// 0x04 (0x501996 (feedingfrenzy2)): counting from the food bank before the stage
	void enter() override
	{
		mWait = NEXT_WAIT;
		mPhase = FOOD_BANK_COUNTING;
		mShown = mScreen->mFoodBank;
	}
	// 0x0c (0x53dbf6 (feedingfrenzy2)): a step at each short wait's end until the total is shown; then the rank's
	// header, the rank, and the count's end
	void update(UpdateContext* theContext) override
	{
		mWait = (float)((double)mWait - theContext->mElapsed);
		if (mPhase == FOOD_BANK_COUNTING)
		{
			if (mWait < 0.0f)
			{
				step();
				mWait = FOOD_BANK_STEP_WAIT;
			}
			if (mShown != mScreen->mTotal)
				return;
			mPhase = FOOD_BANK_HEADER;
			mWait = NEXT_WAIT;
			return;
		}
		if (mPhase == FOOD_BANK_HEADER)
		{
			if (!(mWait < 0.0f))
				return;
			mScreen->mFoodBankRankHeader->setVisible(true);
			// The shell theme's sound drumRoll at full volume and pitch: not yet translated, as the sounds are not
			mPhase = FOOD_BANK_RANK;
			mWait = RANK_WAIT;
			return;
		}
		if (!(mWait < 0.0f))
			return;
		mScreen->mFoodBankRank->setVisible(true);
		// The shell theme's sound frenzyRankBest at full volume and pitch: not yet translated, as the sounds are not
		mScreen->mStateMgr.changeState(nullptr);
	}
	// 0x1c (0x53999e (feedingfrenzy2)): the food bank shown, and the count's end
	void skip() override
	{
		mScreen->finishFoodBank();
		mScreen->mStateMgr.changeState(nullptr);
	}

private:
	enum
	{
		FOOD_BANK_COUNTING = 0,
		FOOD_BANK_HEADER = 1,
		FOOD_BANK_RANK = 2
	};

	// 0x53db89 (feedingfrenzy2): one more, or ten more while more than ten are left (the total at once when past it),
	// with the shell theme's sound countUpClick (not yet translated, as the sounds are not), and the food bank's text
	void step()
	{
		int aLeft = mScreen->mTotal - mShown;
		if (aLeft < 0)
			mShown = mScreen->mTotal;
		else if (aLeft < FOOD_BANK_FINE_STEPS)
			mShown = mShown + 1;
		else
			mShown = mShown + FOOD_BANK_COARSE_STEP;
		mScreen->mFoodBankCount->setText(groupedNumber(mShown, true));
	}

	float mWait;   // +0x10
	int mPhase;    // +0x14
	int mShown;    // +0x18: the food bank shown
};

// The time left, the time bonus and the final score counted up, each shown as its count begins
// (StageCompleteScreenBonus::TimeLeftCountupState, 0x170 bytes, its constructor 0x51d297 (feedingfrenzy2))
class StageCompleteScreenBonus::TimeLeftCountupState : public CountState
{
public:
	explicit TimeLeftCountupState(StageCompleteScreenBonus* theScreen)
	    : CountState(theScreen), mWait(0), mPhase(COUNTUP_TIME_LEFT)
	{
	}

	// 0x04 (0x53b4ad (feedingfrenzy2)): the time left counted from none, the bonus from none and the final score from
	// the score before the bonus
	void enter() override
	{
		mWait = NEXT_WAIT;
		mTimeLeft.start(0, mScreen->getSecondsLeft(), mScreen->mTimeLeftText, TextRollUp::FORM_CLOCK, CLOCK_LARGEST_STEP,
		                ROLL_UP_WAIT);
		mTimeBonus.start(0, mScreen->mTimeBonus, mScreen->mTimeBonusText, TextRollUp::FORM_GROUPED,
		                 SCORE_LARGEST_STEP, ROLL_UP_WAIT);
		mFinalScore.start(mScreen->mScoreBefore, mScreen->mFinalScore, mScreen->mFinalScoreText,
		                  TextRollUp::FORM_GROUPED, SCORE_LARGEST_STEP, ROLL_UP_WAIT);
		mPhase = COUNTUP_TIME_LEFT;
	}
	// 0x0c (0x53f68f (feedingfrenzy2)): after each wait, a number shown and counted to its end, half a second apart;
	// then the food bank
	void update(UpdateContext* theContext) override
	{
		mWait = (float)((double)mWait - theContext->mElapsed);
		if (0.0f <= mWait)
			return;
		switch (mPhase)
		{
		case COUNTUP_TIME_LEFT:
			mScreen->mTimeLeftText->setVisible(true);
			if (!mTimeLeft.update(theContext))
				return;
			mPhase = COUNTUP_TIME_BONUS;
			break;
		case COUNTUP_TIME_BONUS:
			mScreen->mTimeBonusText->setVisible(true);
			if (!mTimeBonus.update(theContext))
				return;
			mPhase = COUNTUP_FINAL_SCORE;
			break;
		case COUNTUP_FINAL_SCORE:
			mScreen->mFinalScoreText->setVisible(true);
			if (mFinalScore.update(theContext))
				mPhase = COUNTUP_DONE;
			return;
		case COUNTUP_DONE:
			mScreen->mStateMgr.changeState(new FoodBankState(mScreen));
			return;
		default:
			return;
		}
		mWait = NEXT_WAIT;
	}
	// 0x1c (0x53b56f (feedingfrenzy2)): the time bonus and the food bank shown, and the count's end
	void skip() override
	{
		mScreen->finishTimeBonus();
		mScreen->finishFoodBank();
		mScreen->mStateMgr.changeState(nullptr);
	}

private:
	enum
	{
		COUNTUP_TIME_LEFT = 0,
		COUNTUP_TIME_BONUS = 1,
		COUNTUP_FINAL_SCORE = 2,
		COUNTUP_DONE = 3
	};

	float mWait;              // +0x10
	int mPhase;               // +0x14
	TextRollUp mTimeLeft;     // +0x18
	TextRollUp mTimeBonus;    // +0x88
	TextRollUp mFinalScore;   // +0xf8
};

// Half a second, then the time bonus or the word that there is none (StageCompleteScreenBonus::TimeBonusState,
// 0x1c bytes, its constructor 0x51d301 (feedingfrenzy2))
class StageCompleteScreenBonus::TimeBonusState : public CountState
{
public:
	explicit TimeBonusState(StageCompleteScreenBonus* theScreen) : CountState(theScreen), mWait(0) {}

	// 0x04 (0x5019b3 (feedingfrenzy2))
	void enter() override { mWait = NEXT_WAIT; }
	// 0x0c (0x53ceb0 (feedingfrenzy2)): with a fish left, the no-bonus group and the food bank; else the bonus group
	// and the time left's count
	void update(UpdateContext* theContext) override
	{
		mWait = (float)((double)mWait - theContext->mElapsed);
		if (!(mWait < 0.0f))
			return;
		if (mScreen->isFishLeft())
		{
			mScreen->mNoTimeBonusGroup->setVisible(true);
			// The shell theme's sound dispNoTimeBonusSnd: not yet translated, as the sounds are not
			mScreen->mStateMgr.changeState(new FoodBankState(mScreen));
		}
		else
		{
			mScreen->mTimeBonusGroup->setVisible(true);
			// The stage theme's sound bonus1Up and the shell theme's dispTimeBonusSnd: not yet translated, as the
			// sounds are not
			mScreen->mStateMgr.changeState(new TimeLeftCountupState(mScreen));
		}
	}
	// 0x1c (0x53b56f (feedingfrenzy2), the time left's): the time bonus and the food bank shown, and the count's end
	void skip() override
	{
		mScreen->finishTimeBonus();
		mScreen->finishFoodBank();
		mScreen->mStateMgr.changeState(nullptr);
	}

private:
	float mWait;  // +0x10
};

// The fish eaten and their share counted up (StageCompleteScreenBonus::CountFishRatioState, 0x108 bytes, its
// constructor 0x4a1b70 (feedingfrenzy2))
class StageCompleteScreenBonus::CountFishRatioState : public CountState
{
public:
	explicit CountFishRatioState(StageCompleteScreenBonus* theScreen)
	    : CountState(theScreen), mWait(0), mPhase(RATIO_FISH)
	{
	}

	// 0x04 (0x493810 (feedingfrenzy2)): the screen filled again (so the time bonus is added to the score a second
	// time); the fish eaten counted from none, written before the separator and the supply, and the share from none
	// before the percent sign. (+0x18 and +0x1c, cleared here, are read by nothing)
	void enter() override
	{
		mScreen->fill();
		mWait = NEXT_WAIT;
		mPhase = RATIO_FISH;
		mFishEaten.start(0, mScreen->mFishEaten, mScreen->mEatRatioText, TextRollUp::FORM_AFFIXED, FISH_LARGEST_STEP,
		                 ROLL_UP_WAIT);
		mFishEaten.setAffixes("", appString("ratioSep") + formatNumber(mScreen->mFishSupply));
		mPercent.start(0, mScreen->mEatPercent, mScreen->mEatPercentText, TextRollUp::FORM_AFFIXED, FISH_LARGEST_STEP,
		               ROLL_UP_WAIT);
		mPercent.setAffixes("", appString("pctSign"));
	}
	// 0x0c (0x49c5c0 (feedingfrenzy2)): after half a second the fish eaten counted to their end, half a second more
	// and the share; then the time bonus
	void update(UpdateContext* theContext) override
	{
		mWait = (float)((double)mWait - theContext->mElapsed);
		if (!(mWait < 0.0f))
			return;
		if (mPhase == RATIO_FISH)
		{
			if (mFishEaten.update(theContext))
			{
				mPhase = RATIO_PERCENT;
				mWait = NEXT_WAIT;
			}
		}
		else if (mPhase == RATIO_PERCENT)
		{
			if (mPercent.update(theContext))
				mPhase = RATIO_DONE;
		}
		else if (mPhase == RATIO_DONE)
			mScreen->mStateMgr.changeState(new TimeBonusState(mScreen));
	}
	// 0x1c (0x4a0170 (feedingfrenzy2)): the fish written in full; the no-bonus group, or the time bonus in full; the
	// food bank shown, and the count's end
	void skip() override
	{
		mScreen->finishFish();
		if (mScreen->isFishLeft())
		{
			// The shell theme's sound dispNoTimeBonusSnd: not yet translated, as the sounds are not
			mScreen->mNoTimeBonusGroup->setVisible(true);
		}
		else
		{
			// The stage theme's sound bonus1Up: not yet translated, as the sounds are not
			mScreen->finishTimeBonus();
		}
		mScreen->finishFoodBank();
		mScreen->mStateMgr.changeState(nullptr);
	}

private:
	enum
	{
		RATIO_FISH = 0,
		RATIO_PERCENT = 1,
		RATIO_DONE = 2
	};

	float mWait;              // +0x10
	int mPhase;               // +0x14
	TextRollUp mFishEaten;    // +0x20
	TextRollUp mPercent;      // +0x90
};

StageCompleteScreenBonus::StageCompleteScreenBonus()
    : mFoodBank(0), mTotal(0), mEatRatioText(nullptr), mEatPercentText(nullptr), mTimeBonusGroup(nullptr),
      mTimeLeftText(nullptr), mTimeBonusText(nullptr), mFinalScoreText(nullptr), mFoodBankRankHeader(nullptr),
      mFoodBankRank(nullptr), mFoodBankCount(nullptr), mNoTimeBonusGroup(nullptr), mFishEaten(0), mFishSupply(0),
      mEatPercent(0), mScoreBefore(0), mFinalScore(0), mTimeBonus(0)
{
}

void StageCompleteScreenBonus::setup(const std::string& theStageId, Stage* theStage, int theFoodBank)
{
	mStage = theStage;
	mFoodBank = theFoodBank;
	mTotal = theFoodBank;
	findComponents();
	fill();
	mStateMgr.changeState(new CountFishRatioState(this));
}

void StageCompleteScreenBonus::skip()
{
	CountState* aState = mStateMgr.getState();
	if (aState != nullptr)
		aState->skip();
}

void StageCompleteScreenBonus::findComponents()
{
	mEatRatioText = static_cast<TextItem*>(findComponent("eatRatioText"));
	mEatPercentText = static_cast<TextItem*>(findComponent("eatPctText"));
	mTimeBonusGroup = findComponent("timeBonusGrp");
	mNoTimeBonusGroup = findComponent("noTimeBonusGrp");
	mTimeLeftText = static_cast<TextItem*>(findComponent("timeLeftText"));
	mTimeBonusText = static_cast<TextItem*>(findComponent("timeBonusText"));
	mFinalScoreText = static_cast<TextItem*>(findComponent("finalScoreText"));
	mFoodBankCount = static_cast<TextItem*>(findComponent("foodBankCount"));
	mFoodBankRank = static_cast<TextItem*>(findComponent("foodBankRank"));
	mFoodBankRankHeader = findComponent("foodBankRankHeader");
}

void StageCompleteScreenBonus::fill()
{
	FeedingFrenzyTwo* anApp = getApp();
	countFish();
	mTotal = mFishEaten + mFoodBank;
	mFoodBankCount->setText(groupedNumber(mFoodBank, true));
	mFoodBankRank->setText(anApp->mPlayerRanks->getTitle(mTotal));
	mFoodBankRankHeader->setVisible(false);
	mFoodBankRank->setVisible(false);
	// ("%d %s %d", with spaces the count's own writing leaves out)
	mEatRatioText->setText(formatNumber(0) + " " + appString("ratioSep") + " " + formatNumber(mFishSupply));
	mEatPercentText->setText(formatNumber(0) + appString("pctSign"));
	mTimeBonusGroup->setVisible(false);
	mNoTimeBonusGroup->setVisible(false);
	mTimeBonus = getSecondsLeft() * TIME_BONUS_PER_SECOND;
	GameData* aData = anApp->mGameData.get();
	mScoreBefore = aData->getScore();
	aData->addScore(mTimeBonus);
	mFinalScore = mTimeBonus + mScoreBefore;
	mTimeLeftText->setVisible(false);
	mTimeBonusText->setVisible(false);
	mFinalScoreText->setVisible(false);
}

void StageCompleteScreenBonus::countFish()
{
	mFishEaten = 0;
	mFishSupply = 0;
	for (const Ref<NonPlayerSpawn>& aSpawn : mStage->getNonPlayerSpawns())
	{
		if (aSpawn->getSupply() == 0)
			continue;
		mFishSupply = mFishSupply + aSpawn->getSupply();
		mFishEaten = mFishEaten + aSpawn->getDeaths();
	}
	mEatPercent = truncateToInt((double)mFishEaten / (double)mFishSupply * PERCENT);
}

int StageCompleteScreenBonus::getSecondsLeft() const
{
	return truncateToInt(ceil((double)mStage->getTimeLeft()));
}

void StageCompleteScreenBonus::finishFish()
{
	mEatPercentText->setText(formatNumber(mEatPercent) + appString("pctSign"));
	mEatPercentText->setVisible(true);
	mEatRatioText->setText(formatNumber(mFishEaten) + appString("ratioSep") + formatNumber(mFishSupply));
	mEatRatioText->setVisible(true);
}

void StageCompleteScreenBonus::finishTimeBonus()
{
	if (isFishLeft())
	{
		mNoTimeBonusGroup->setVisible(true);
		return;
	}
	mTimeBonusGroup->setVisible(true);
	mTimeLeftText->setVisible(true);
	mTimeLeftText->setText(stageClock(getSecondsLeft()));
	mTimeBonusText->setText(groupedNumber(mTimeBonus));
	mTimeBonusText->setVisible(true);
	mFinalScoreText->setText(groupedNumber(mFinalScore));
	mFinalScoreText->setVisible(true);
}

void StageCompleteScreenBonus::finishFoodBank()
{
	mFoodBankCount->setText(groupedNumber(mTotal, true));
	mFoodBankCount->setVisible(true);
	mFoodBankRankHeader->setVisible(true);
	mFoodBankRank->setVisible(true);
}
