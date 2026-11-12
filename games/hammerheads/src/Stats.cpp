#include "Stats.h"
#include "Application.h"
#include "GameScreen.h"
#include "UserManager.h"

using namespace Sexy;

static Stats* gStats = nullptr; // 0x597f2c

// 0x42f170 (its body is Reset)
Stats::Stats()
{
	Reset();
}

// 0x42efe0
void Stats::Reset()
{
	mValues.clear();
	for (int i = 0; i < NUM_STATS; i++)
		mValues.push_back(0.0);

	mValues[STAT_HEALTH] = 5.0;
	mValues[STAT_HEALTH_CAPACITY] = 5.0;
	mValues[STAT_CHALLENGE_GOAL] = 16.0;
	if (gApp->GetCheatFlag(8))
	{
		mValues[STAT_COINS_COLLECTED] = 500.0;
		mValues[STAT_COIN_BALANCE] = 500.0;
		mValues[STAT_HAMMER_LEVEL] = 1.0;
		mValues[STAT_HEALTH] = 6.0;
		mValues[STAT_HEALTH_CAPACITY] = 6.0;
	}
	if (gApp->GetCheatFlag(2))
	{
		mValues[STAT_HAMMER_LEVEL] = 2.0;
		mValues[STAT_HEALTH] = 7.0;
		mValues[STAT_HEALTH_CAPACITY] = 7.0;
		mValues[STAT_HAMMER_LEVEL] = 3.0;
		mValues[STAT_HAMMER_ON_FIRE] = 1.0;
		mValues[STAT_COINS_COLLECTED] = 50000.0;
		mValues[STAT_COIN_BALANCE] = 50000.0;
	}
	mValues[STAT_COMBO] = 1.0;
}

// 0x42ec50
double Stats::Get(int theIndex) const
{
	return mValues[theIndex];
}

// 0x42efc0: _ftol
int Stats::GetInt(int theIndex) const
{
	return (int)mValues[theIndex];
}

// 0x42ec10
void Stats::Set(int theIndex, double theValue)
{
	mValues[theIndex] = theValue;
	CheckTrophies(0);
}

// 0x42efa0
void Stats::SetInt(int theIndex, int theValue)
{
	mValues[theIndex] = (double)theValue;
	CheckTrophies(0);
}

// 0x42ef80
void Stats::AddInt(int theIndex, int theAmount)
{
	mValues[theIndex] = (double)theAmount + mValues[theIndex];
	CheckTrophies(0);
}

// 0x42ec30
void Stats::AddIntQuiet(int theIndex, int theAmount)
{
	mValues[theIndex] = (double)theAmount + mValues[theIndex];
}

// The 25 values a profile keeps, in the order of the original's 0x42ec60 (and read back by 0x42ed70)
static const int SAVED_STATS[Stats::NUM_SAVED_STATS] = {
	STAT_COIN_BALANCE, STAT_HEALTH, STAT_HEALTH_CAPACITY, STAT_HAMMER_LEVEL, STAT_HAMMER_ON_FIRE,
	STAT_GNOMES_SMASHED, STAT_KINGS_DESTROYED, STAT_PERFECT_BONUS_SCREENS, STAT_COINS_COLLECTED, STAT_LEVEL_OF_LAST_BOMB,
	STAT_LEVEL_OF_LAST_MISS, STAT_LEVEL_OF_LAST_ESCAPE, STAT_LEVEL_OF_LAST_PURCHASE, STAT_SUPPLIES_BOUGHT, STAT_LEVEL_OF_LAST_HEART,
	34, STAT_HEARTS_COLLECTED, STAT_LEVEL_OF_LAST_COIN, STAT_HEARTS_MISSED, STAT_BOMBS_HIT,
	STAT_MISSES, STAT_POWER_BOTTLES, STAT_CLOVERS, STAT_BONUS_HITS, STAT_BONUS_MISSES
};

// 0x42ec60
void Stats::GetSaved(double* theValues) const
{
	for (int i = 0; i < NUM_SAVED_STATS; i++)
		theValues[i] = mValues[SAVED_STATS[i]];
}

// 0x42ed70
void Stats::SetSaved(const double* theValues)
{
	for (int i = 0; i < NUM_SAVED_STATS; i++)
		mValues[SAVED_STATS[i]] = theValues[i];
}

// 0x42ee80
void Stats::ClearLevelCounts()
{
	mValues[41] = 0.0;
	mValues[42] = 0.0;
	mValues[43] = 0.0;
}

// 0x42eeb0
void Stats::ClearBonusCounts()
{
	for (int i = 45; i <= 49; i++)
		mValues[i] = 0.0;
}

// 0x42ef00: the original copies four values at a time
void Stats::CopyFrom(const Stats& theStats)
{
	mValues = theStats.mValues;
}

// 0x42f1c0
Stats* Sexy::GetStats()
{
	if (gStats == nullptr)
		gStats = new Stats();
	return gStats;
}

// 0x42d0a0
void Sexy::CheckTrophies(int theLevelOffset)
{
	if (gApp->mGameScreen == nullptr)
		return;
	if (gApp->mTrophyRoom != nullptr)
		return;

	User* aUser = gApp->mUserManager->GetCurrentUser();
	Stats* aStats = GetStats();
	int aLevel = gApp->mGameScreen->mLevel + theLevelOffset;
	std::vector<int> aNewTrophies;

	if (!aUser->HasTrophy(TROPHY_GNOME_CRUSHER) && aStats->GetInt(STAT_GNOMES_SMASHED) == 100)
		aNewTrophies.push_back(TROPHY_GNOME_CRUSHER);
	if (!aUser->HasTrophy(TROPHY_ULTIMATE_GNOME_CRUSHER) && aStats->GetInt(STAT_GNOMES_SMASHED) == 1000)
		aNewTrophies.push_back(TROPHY_ULTIMATE_GNOME_CRUSHER);
	if (!aUser->HasTrophy(TROPHY_PULVERIZER) && aStats->GetInt(STAT_GNOMES_SMASHED) == 2500)
		aNewTrophies.push_back(TROPHY_PULVERIZER);
	if (!aUser->HasTrophy(TROPHY_REGICIST) && aStats->GetInt(STAT_KINGS_DESTROYED) > 0)
		aNewTrophies.push_back(TROPHY_REGICIST);
	if (!aUser->HasTrophy(TROPHY_BONUS_BANKER) && aStats->GetInt(STAT_PERFECT_BONUS_SCREENS) > 0)
		aNewTrophies.push_back(TROPHY_BONUS_BANKER);
	if (!aUser->HasTrophy(TROPHY_POWER_PLAYER) && aStats->GetInt(STAT_HAMMER_LEVEL) == 3 && aStats->GetInt(STAT_HAMMER_ON_FIRE) == 1)
	{
		if (!gApp->IsShopOpen())
			aNewTrophies.push_back(TROPHY_POWER_PLAYER);
	}
	if (!aUser->HasTrophy(TROPHY_MOGUL) && aStats->GetInt(STAT_COINS_COLLECTED) >= 2500)
	{
		if (!gApp->IsChallengeOpen())
			aNewTrophies.push_back(TROPHY_MOGUL);
	}
	if (!aUser->HasTrophy(TROPHY_TROPHY_MASTER))
	{
		int aTrophy = 0;
		bool aHasAllOthers = true;
		while (aTrophy < NUM_TROPHIES)
		{
			if (aTrophy != TROPHY_TROPHY_MASTER && !aUser->HasTrophy(aTrophy))
			{
				aHasAllOthers = false;
				break;
			}
			aTrophy++;
		}
		if (aHasAllOthers)
			aNewTrophies.push_back(TROPHY_TROPHY_MASTER);
	}

	if (gApp->mGameMode == GAME_MODE_CLASSIC || gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
	{
		if (!aUser->HasTrophy(TROPHY_BLAST_MASTER) && aLevel - aStats->GetInt(STAT_LEVEL_OF_LAST_BOMB) > 5 && aLevel > 12)
			aNewTrophies.push_back(TROPHY_BLAST_MASTER);
		if (!aUser->HasTrophy(TROPHY_SUPREME_BLAST_MASTER) && aLevel - aStats->GetInt(STAT_LEVEL_OF_LAST_BOMB) > 10 && aLevel > 17)
			aNewTrophies.push_back(TROPHY_SUPREME_BLAST_MASTER);
		if (!aUser->HasTrophy(TROPHY_ACCURACY_AWARD) && aLevel - aStats->GetInt(STAT_LEVEL_OF_LAST_MISS) == 6)
			aNewTrophies.push_back(TROPHY_ACCURACY_AWARD);
		if (!aUser->HasTrophy(TROPHY_MASTER_MARKSMAN) && aLevel - aStats->GetInt(STAT_LEVEL_OF_LAST_MISS) == 11)
			aNewTrophies.push_back(TROPHY_MASTER_MARKSMAN);
		if (!aUser->HasTrophy(TROPHY_NO_GNOME_ESCAPED) && aLevel - aStats->GetInt(STAT_LEVEL_OF_LAST_ESCAPE) == 6)
			aNewTrophies.push_back(TROPHY_NO_GNOME_ESCAPED);
		if (!aUser->HasTrophy(TROPHY_SUPPLY_CHALLENGE) && aLevel - aStats->GetInt(STAT_LEVEL_OF_LAST_PURCHASE) > 10 && aLevel > 15)
			aNewTrophies.push_back(TROPHY_SUPPLY_CHALLENGE);
		if (!aUser->HasTrophy(TROPHY_GOLDEN_HEART) && aLevel - aStats->GetInt(STAT_LEVEL_OF_LAST_HEART) == 16)
			aNewTrophies.push_back(TROPHY_GOLDEN_HEART);
		if (!aUser->HasTrophy(TROPHY_VOW_OF_POVERTY) && aLevel - aStats->GetInt(STAT_LEVEL_OF_LAST_COIN) == 16)
			aNewTrophies.push_back(TROPHY_VOW_OF_POVERTY);
		if (!aUser->HasTrophy(TROPHY_HEART_HOG) && aStats->GetInt(STAT_HEARTS_COLLECTED) > 0 && aStats->GetInt(STAT_HEARTS_MISSED) == 0 && aLevel == 16)
			aNewTrophies.push_back(TROPHY_HEART_HOG);
		if (!aUser->HasTrophy(TROPHY_CLASSIC_SCORE_RIBBON) && aStats->GetInt(STAT_SCORE) > 99999)
		{
			if (!gApp->IsChallengeOpen())
				aNewTrophies.push_back(TROPHY_CLASSIC_SCORE_RIBBON);
		}
	}
	else if (gApp->mGameMode == GAME_MODE_MARATHON)
	{
		if (!aUser->HasTrophy(TROPHY_SILVER_MARATHON_MEDAL) && aLevel == 10)
			aNewTrophies.push_back(TROPHY_SILVER_MARATHON_MEDAL);
		if (!aUser->HasTrophy(TROPHY_GOLD_MARATHON_MEDAL) && aLevel == 20)
			aNewTrophies.push_back(TROPHY_GOLD_MARATHON_MEDAL);
		if (!aUser->HasTrophy(TROPHY_MARATHON_SCORE_RIBBON) && aStats->GetInt(STAT_SCORE) > 99999)
		{
			if (!gApp->IsChallengeOpen())
				aNewTrophies.push_back(TROPHY_MARATHON_SCORE_RIBBON);
		}
	}

	if (!aNewTrophies.empty())
		gApp->GameToTrophyRoom(aNewTrophies);
}
