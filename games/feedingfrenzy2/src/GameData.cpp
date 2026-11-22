#include "GameData.h"

#include "Geometry.h"
#include "UserData.h"

using engine::minOf;

void GameData::newGame(int theMode)
{
	mMode = theMode;
	mLives = NEW_GAME_LIVES;
	mScore = 0;
	mContinues = NEW_GAME_CONTINUES;
	mStage = 0;
	mTimeRollover = 0;
	mFood = 0;
	findNextLifeScore();
}

bool GameData::addScore(int thePoints)
{
	mScore += thePoints;
	if (mScore < 0)
		mScore = 0;
	if (!(mScore > mNextLifeScore) || mMode == GAME_MODE_TIMED)
		return false;
	mLives++;
	mNextLifeScore += mLifeScoreStep;
	mLifeScoreStep = minOf(mLifeScoreStep + EXTRA_LIFE_SCORE_STEP, EXTRA_LIFE_SCORE_MOST);
	return true;
}

void GameData::findNextLifeScore()
{
	mNextLifeScore = 0;
	mLifeScoreStep = 0;
	if (mScore < 0)
		return;
	do
	{
		mNextLifeScore = mNextLifeScore + mLifeScoreStep;
		int aStep = mLifeScoreStep + EXTRA_LIFE_SCORE_STEP;
		if (aStep > EXTRA_LIFE_SCORE_MOST)
			aStep = EXTRA_LIFE_SCORE_MOST;
		mLifeScoreStep = aStep;
	} while (mNextLifeScore <= mScore);
}

const StageManifest::Stage* GameData::getStageEntry() const
{
	if (mStage < 0)
		return nullptr;
	// 0x483bd0 (feedingfrenzy2): the entry at the place, unchecked (its count is read and not used)
	return &mStages.getStages()[mStage];
}
