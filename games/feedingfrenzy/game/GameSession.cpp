#include "GameSession.h"
#include "FishFood.h"
#include "StageMgr.h"

namespace feedingfrenzy
{

// The fish, the continues and the score the game starts with, and the score that wins the first extra fish
const int START_LIVES = 3;
const int START_CONTINUES = 2;
const int FIRST_EXTRA_LIFE_SCORE = 6000;
// The score from which the extra fish come farther apart, and how much further each is
const int EXTRA_LIFE_STEP_CHANGE_SCORE = 12000;
const int EXTRA_LIFE_STEP_LOW = 6000;
const int EXTRA_LIFE_STEP_HIGH = 12000;

// 0x4356a0
void GameSession::reset()
{
	mContinues = START_CONTINUES;
	mNextExtraLifeScore = FIRST_EXTRA_LIFE_SCORE;
	mLives = START_LIVES;
	mScore = 0;
	mSecondsPlayed = 0.0f;
}

// 0x4356c0
void GameSession::useContinue()
{
	mContinues--;
	mLives = START_LIVES;
}

// 0x435750
void GameSession::addScore(int thePoints)
{
	mScore += thePoints;
	if (mNextExtraLifeScore < mScore && mMode != GAME_MODE_TIMED)
	{
		mLives++;
		std::string aMessage = FishFood::instance()->getStrings().get("extraLife");
		FishFood::instance()->getStageMgr()->showMessage(aMessage);
		FishFood::instance()->playSound("extraLife", 1.0f, 1.0f);
		if (mScore < EXTRA_LIFE_STEP_CHANGE_SCORE)
			mNextExtraLifeScore += EXTRA_LIFE_STEP_LOW;
		else
			mNextExtraLifeScore += EXTRA_LIFE_STEP_HIGH;
		return;
	}
	if (mScore < 0)
		mScore = 0;
}

} // namespace feedingfrenzy
