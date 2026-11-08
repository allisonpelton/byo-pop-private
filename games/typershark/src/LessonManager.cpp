// LessonManager.cpp: the level table of Typer Shark Deluxe 1.02 and the cursor that walks through it (see LessonManager.h).
// The file parser is in LessonManagerConfig.cpp.
//
// Not translated: the standard-library instances of the original (listed in LessonManager.h), which the port gets from
// std::vector, std::set and std::vector<std::vector<...> >. The test of the shipped file in the original constructor (the vector it
// reserves and frees again) has no effect.
// Kept quirks: NextWave draws from the generator in the order of the original and divides by a count that is 0 for an empty
// list; LastWaveIsBoss reads the element before the first one of a level without waves. SetLevel(36 or more) starts
// NextLevel's walk at level index -1, so the level it ends on is the argument minus 36 (the original; the map screen does not
// call it with a larger level than the table has).

#include "LessonManager.h"
#include "WinWSApp.h"
#include "Board.h"
#include "MTRand.h"

using namespace Sexy;

// the record and everything it owns (the original writes this out twice, for the levels and for the endless levels)
static void FreeLevel(LevelRecord* theLevel)
{
	for (size_t aWaveIndex = 0; aWaveIndex < theLevel->mWaves.size(); aWaveIndex++)
	{
		WaveRecord* aWave = theLevel->mWaves[aWaveIndex];
		for (size_t anEnemyIndex = 0; anEnemyIndex < aWave->mEnemies.size(); anEnemyIndex++)
			delete aWave->mEnemies[anEnemyIndex];
		delete aWave;
	}
	delete theLevel;
}

// 0x4270e0
LessonManager::LessonManager(WinWSApp* theApp) :
	mEndlessLevel(0),
	mPrevEndlessLevel(0),
	mApp(theApp),
	mLevel(0),
	mWave(0),
	mMinEndDiff(0),
	mMaxEndDiff(0),
	mCurrentWaveType(0),
	mCurrentWave(0),
	mAbyssMode(false),
	mEndlessCount(0)
{
	// (the original makes the set's head node here and clears the two vectors)
}

// 0x421c50
LessonManager::~LessonManager()
{
	for (size_t aLevelIndex = 0; aLevelIndex < mLevels.size(); aLevelIndex++)
		FreeLevel(mLevels[aLevelIndex]);
	for (size_t aLevelIndex = 0; aLevelIndex < mEndlessLevels.size(); aLevelIndex++)
		FreeLevel(mEndlessLevels[aLevelIndex]);
	// (the set's nodes and head, and the two vectors' storage, go with the members)
}

// 0x4131a0
bool LessonManager::IsEndless()
{
	if (mLevel != -1)
	{
		unsigned int aCount = (unsigned int) mLevels.size();
		if ((unsigned int) mLevel < aCount)
			return false;
	}
	return true;
}

// the record that 0x4131a0 chooses at the top of every getter below (inlined there)
LevelRecord* LessonManager::GetCurrentLevel()
{
	if (!IsEndless())
		return mLevels[mLevel];
	return mEndlessLevels[mEndlessLevel];
}

// 0x4137d0
bool LessonManager::IsBonusLevel()
{
	if (!IsEndless())
		return mLevels[mLevel]->mBonusLevel;
	return false;
}

// 0x4137f0
bool LessonManager::LastWaveIsBoss()
{
	LevelRecord* aLevel = GetCurrentLevel();
	unsigned int aCount = (unsigned int) aLevel->mWaves.size();
	WaveRecord* aWave = aLevel->mWaves[aCount - 1];
	return aWave->mType == WAVE_BOSS;
}

// 0x413860
LevelSettings* LessonManager::GetLevelSettings()
{
	return &GetCurrentLevel()->mSettings;
}

// 0x413890
int LessonManager::GetWaveType()
{
	return GetCurrentLevel()->mWaves[mWave]->mType;
}

// 0x4138d0
int LessonManager::GetWaveMinCol()
{
	return GetCurrentLevel()->mWaves[mWave]->mMinCol;
}

// 0x413920
int LessonManager::GetWaveMaxCol()
{
	return GetCurrentLevel()->mWaves[mWave]->mMaxCol;
}

// 0x413970: of the wave NextWave last chose (mCurrentWave); in the endless levels of the one before the level it drew
// (mPrevEndlessLevel), and 0 in a bonus level
int LessonManager::GetWaveKnockBack()
{
	if (!IsEndless() && !IsBonusLevel())
		return mLevels[mLevel]->mWaves[mCurrentWave]->mKnockBack;

	if (IsBonusLevel())
		return 0;
	return mEndlessLevels[mPrevEndlessLevel]->mWaves[mCurrentWave]->mKnockBack;
}

// 0x4139d0
EnemyRecord* LessonManager::GetEnemyRecord(int theIndex)
{
	return GetCurrentLevel()->mWaves[GetWave()]->mEnemies[theIndex];
}

// 0x413a10
int LessonManager::GetEnemyRecordCount()
{
	return (int) GetCurrentLevel()->mWaves[GetWave()]->mEnemies.size();
}

// 0x413a60
int LessonManager::GetWaveCount()
{
	return (int) GetCurrentLevel()->mWaves.size();
}

// 0x4149d0
void LessonManager::GoToBossWave()
{
	if (LastWaveIsBoss())
	{
		LevelRecord* aLevel = GetCurrentLevel();
		if (aLevel->mWaves.size() == 0)
		{
			mWave = -1;
			return;
		}
		mWave = (int) aLevel->mWaves.size() - 1;
	}
}

// 0x41eb40: chooses the wave that is played next, at random among those of the level that were not played yet (a boss wave
// is never chosen: GoToBossWave goes to it), and clears the played waves when only the boss wave would be left. In the endless
// levels it also chooses the level. thePrevLevel is true when the board asks for the wave type of the level before (it
// has already gone to the next one).
void LessonManager::NextWave(bool thePrevLevel)
{
	int aWaveType;
	if (!thePrevLevel)
		aWaveType = GetWaveType();
	else
	{
		int aLevel = mLevel;
		mLevel = aLevel - 1;
		aWaveType = GetWaveType();
		mLevel = aLevel;
	}
	mCurrentWaveType = aWaveType;
	mCurrentWave = GetWave();

	if (mAbyssMode)
	{
		unsigned int aCount = (unsigned int) mLevels[mLevel]->mWaves.size();
		if (aCount - 1U <= (unsigned int) mUsedWaves.size())
			mUsedWaves.clear();

		aCount = (unsigned int) mLevels[mLevel]->mWaves.size();
		mWave = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
		if (mUsedWaves.find(mWave) != mUsedWaves.end())
		{
			do
			{
				aCount = (unsigned int) mLevels[mLevel]->mWaves.size();
				mWave = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
			} while (mUsedWaves.find(mWave) != mUsedWaves.end());
		}
		mUsedWaves.insert(mWave);

		if (thePrevLevel)
		{
			mCurrentWave = GetWave();
			mCurrentWaveType = GetWaveType();
		}
		return;
	}

	bool aLastIsBoss = LastWaveIsBoss();
	if (IsEndless())
	{
		mPrevEndlessLevel = mEndlessLevel;
		unsigned int aCount = (unsigned int) mEndlessLevels.size();
		mEndlessLevel = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
		aCount = (unsigned int) mEndlessLevels[mEndlessLevel]->mWaves.size();
		mWave = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);

		aLastIsBoss = LastWaveIsBoss();
		if ((unsigned int) mEndlessLevels[mEndlessLevel]->mWaves.size() - (aLastIsBoss ? 1U : 0U) <= (unsigned int) mUsedWaves.size())
			mUsedWaves.clear();

		for (;;)
		{
			int aLevelIndex = mEndlessLevel;
			if (mEndlessLevels[aLevelIndex]->mWaves[mWave]->mType != WAVE_BOSS)
			{
				if (mUsedWaves.find(mWave) == mUsedWaves.end())
				{
					mUsedWaves.insert(mWave);
					return;
				}
			}
			aCount = (unsigned int) mEndlessLevels[aLevelIndex]->mWaves.size();
			mWave = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
		}
	}

	if ((unsigned int) mLevels[mLevel]->mWaves.size() - (aLastIsBoss ? 1U : 0U) <= (unsigned int) mUsedWaves.size())
		mUsedWaves.clear();

	unsigned int aCount = (unsigned int) mLevels[mLevel]->mWaves.size();
	for (;;)
	{
		mWave = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
		LevelRecord* aLevel = mLevels[mLevel];
		if (aLevel->mWaves[mWave]->mType != WAVE_BOSS)
		{
			if (mUsedWaves.find(mWave) == mUsedWaves.end())
			{
				mUsedWaves.insert(mWave);
				return;
			}
		}
		aCount = (unsigned int) aLevel->mWaves.size();
	}
}

// 0x430150: in every wave of the current level, the enemies that are stealth (types 3, 7, 9 and 10) go before the others,
// each group in the order it had
void LessonManager::OrderEnemies()
{
	std::vector<std::vector<EnemyRecord*> > anOrdered;
	int aSavedWave = mWave;
	mWave = 0;
	anOrdered.resize(GetWaveCount());   // 0x42f820 (vector::resize)

	int aWaveIndex = 0;
	while (aWaveIndex < GetWaveCount())
	{
		int anEnemyIndex = 0;
		while (anEnemyIndex < GetEnemyRecordCount())
		{
			switch (GetEnemyRecord(anEnemyIndex)->mType)
			{
			case ENEMY_STEALTH:
			case ENEMY_WHITE_STEALTH:
			case ENEMY_RED_STEALTH:
			case ENEMY_BLACK_STEALTH:
				anOrdered[aWaveIndex].push_back(GetEnemyRecord(anEnemyIndex));
				break;
			}
			anEnemyIndex++;
		}
		mWave++;
		aWaveIndex++;
	}

	mWave = 0;
	aWaveIndex = 0;
	while (aWaveIndex < GetWaveCount())
	{
		int anEnemyIndex = 0;
		while (anEnemyIndex < GetEnemyRecordCount())
		{
			switch (GetEnemyRecord(anEnemyIndex)->mType)
			{
			case ENEMY_BLUE:
			case ENEMY_BLACK:
			case ENEMY_RED:
			case ENEMY_TOXIC:
			case ENEMY_CRAZY_TOXIC:
			case ENEMY_WHITE:
			case ENEMY_PURPLE:
			case ENEMY_STANDARD:
			case ENEMY_TORPEDO:
			case ENEMY_GHOST:
			case ENEMY_GHOST_SHIP:
			case ENEMY_MECHA_SHARK:
			case ENEMY_MECHA_SQUID:
				anOrdered[aWaveIndex].push_back(GetEnemyRecord(anEnemyIndex));
				break;
			}
			anEnemyIndex++;
		}
		mWave++;
		aWaveIndex++;
	}
	mWave = aSavedWave;

	LevelRecord* aLevel = GetCurrentLevel();
	for (size_t aWaveNumber = 0; aWaveNumber < anOrdered.size(); aWaveNumber++)
	{
		WaveRecord* aWave = aLevel->mWaves[aWaveNumber];
		aWave->mEnemies.clear();
		for (size_t anEnemyNumber = 0; anEnemyNumber < anOrdered[aWaveNumber].size(); anEnemyNumber++)
			aWave->mEnemies.push_back(anOrdered[aWaveNumber][anEnemyNumber]);
	}
}

// 0x430640: goes to the next level. After level 36 (index 0x24) a level and a wave of the endless levels are drawn, and in
// every endless level the speeds grow with the level number. In the abyss the last level is the one that repeats, a little
// faster each time.
void LessonManager::NextLevel()
{
	mUsedWaves.clear();

	if (!mAbyssMode)
	{
		mLevel = mLevel + 1;
		mWave = 0;
		if (mLevel < 0x25)
		{
			if (mLevel == 0x24)
			{
				mEndlessCount++;
				unsigned int aCount = (unsigned int) mEndlessLevels.size();
				mEndlessLevel = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
				aCount = (unsigned int) mEndlessLevels[mEndlessLevel]->mWaves.size();
				mWave = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
				mUsedWaves.insert(mWave);
			}
		}
		else
		{
			mEndlessCount++;
			unsigned int aCount = (unsigned int) mEndlessLevels.size();
			mEndlessLevel = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
			aCount = (unsigned int) mEndlessLevels[mEndlessLevel]->mWaves.size();
			mWave = (int) ((unsigned int) gGameRand->NextNoAssert() % aCount);
			mUsedWaves.insert(mWave);

			for (size_t aLevelIndex = 0; aLevelIndex < mEndlessLevels.size(); aLevelIndex++)
			{
				LevelRecord* aLevel = mEndlessLevels[aLevelIndex];
				aLevel->mSettings.mMinSpeed = (double) (mLevel - 0x10) * 0.1 + aLevel->mSettings.mMinSpeed;
				aLevel->mSettings.mMaxSpeed = (double) (mLevel - 0x10) * 0.1 + aLevel->mSettings.mMaxSpeed;
				for (size_t aWaveIndex = 0; aWaveIndex < aLevel->mWaves.size(); aWaveIndex++)
				{
					WaveRecord* aWave = aLevel->mWaves[aWaveIndex];
					for (size_t anEnemyIndex = 0; anEnemyIndex < aWave->mEnemies.size(); anEnemyIndex++)
					{
						EnemyRecord* anEnemy = aWave->mEnemies[anEnemyIndex];
						anEnemy->mMinSpeed = (double) (mLevel - 0x10) * 0.1 + anEnemy->mMinSpeed;
						anEnemy->mMaxSpeed = (double) (mLevel - 0x10) * 0.1 + anEnemy->mMaxSpeed;
					}
				}
			}
		}

		if (!IsEndless())
			mWave = mLevels[mLevel]->mFirstWave;
	}
	else
	{
		mLevel = mLevel + 1;
		if ((mLevels.size() == 0) || ((unsigned int) mLevel >= (unsigned int) mLevels.size()))
		{
			// past the last level: it plays again, faster (0.05 once the board's score is past 5400, else 0.025)
			double aSpeedUp = 0.025;
			if (mApp->mBoard->mScore > 0x1518)    // +0x1c4 of the board: the score
				aSpeedUp = 0.05;

			mLevel = (int) mLevels.size() - 1;
			mUsedWaves.insert(mWave);

			LevelRecord* aLevel = mLevels[mLevel];
			aLevel->mSettings.mMinSpeed = aSpeedUp + aLevel->mSettings.mMinSpeed;
			aLevel = mLevels[mLevel];
			aLevel->mSettings.mMaxSpeed = aSpeedUp + aLevel->mSettings.mMaxSpeed;

			for (size_t aWaveIndex = 0; aWaveIndex < mLevels[mLevel]->mWaves.size(); aWaveIndex++)
			{
				WaveRecord* aWave = mLevels[mLevel]->mWaves[aWaveIndex];
				for (size_t anEnemyIndex = 0; anEnemyIndex < aWave->mEnemies.size(); anEnemyIndex++)
				{
					EnemyRecord* anEnemy = aWave->mEnemies[anEnemyIndex];
					anEnemy->mMinSpeed = aSpeedUp + anEnemy->mMinSpeed;
					anEnemy->mMaxSpeed = aSpeedUp + anEnemy->mMaxSpeed;
				}
			}
		}
	}

	OrderEnemies();
	mMinEndDiff = GetLevelSettings()->mMinEndDiff;
	mMaxEndDiff = GetLevelSettings()->mMaxEndDiff;
}

// 0x4309d0: theLevel is the level index (0 for level 1), or -1 for the endless levels. From 36 up it walks forward from
// -1 with NextLevel (theLevel - 35 times).
void LessonManager::SetLevel(int theLevel)
{
	mWave = 0;
	mLevel = theLevel;
	if (theLevel == -1)
	{
		mEndlessCount++;
		for (size_t aLevelIndex = 0; aLevelIndex < mEndlessLevels.size(); aLevelIndex++)
		{
			LevelRecord* aLevel = mEndlessLevels[aLevelIndex];
			aLevel->mSettings.mMinSpeed = (double) mEndlessCount * 0.1 + aLevel->mSettings.mMinSpeed;
			aLevel->mSettings.mMaxSpeed = (double) mEndlessCount * 0.1 + aLevel->mSettings.mMaxSpeed;
			for (size_t aWaveIndex = 0; aWaveIndex < aLevel->mWaves.size(); aWaveIndex++)
			{
				WaveRecord* aWave = aLevel->mWaves[aWaveIndex];
				for (size_t anEnemyIndex = 0; anEnemyIndex < aWave->mEnemies.size(); anEnemyIndex++)
				{
					EnemyRecord* anEnemy = aWave->mEnemies[anEnemyIndex];
					anEnemy->mMinSpeed = (double) mEndlessCount * 0.1 + anEnemy->mMinSpeed;
					anEnemy->mMaxSpeed = (double) mEndlessCount * 0.1 + anEnemy->mMaxSpeed;
				}
			}
		}
	}
	else if (theLevel > 0x23)
	{
		int aCount = theLevel - 0x23;
		mLevel = -1;
		if (aCount > 0)
		{
			do
			{
				NextLevel();
				aCount--;
			} while (aCount != 0);
		}
	}

	if (!IsEndless() && !mAbyssMode)
		mWave = mLevels[mLevel]->mFirstWave;
	OrderEnemies();
}
