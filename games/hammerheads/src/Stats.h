#ifndef __HAMMERHEADS_STATS_H__
#define __HAMMERHEADS_STATS_H__

// The game's running numbers (what the final screens count and the trophies are decided from): one vector of
// doubles that everything reads and writes by index. The same indices are what a saved game stores.

#include <vector>

namespace Sexy
{

enum StatIndex
{
	STAT_GNOMES_SMASHED = 0,
	STAT_BOMBS_HIT = 1,
	STAT_MISSES = 2,
	STAT_HEARTS_COLLECTED = 5,
	STAT_COINS_COLLECTED = 6,
	STAT_CLOVERS = 7,
	STAT_POWER_BOTTLES = 8,
	STAT_COIN_BALANCE = 9,
	STAT_HEALTH = 10,
	STAT_HEALTH_CAPACITY = 11,
	STAT_SCORE = 12,
	STAT_LEVEL_OF_LAST_BOMB = 13,
	STAT_LEVEL_OF_LAST_MISS = 14,
	STAT_LEVEL_OF_LAST_ESCAPE = 15,
	STAT_SUPPLIES_BOUGHT = 16,
	STAT_HEARTS_MISSED = 17,
	STAT_HAMMER_LEVEL = 18,
	STAT_CHALLENGE_MISSES = 21,			// clicks that hit nothing in a challenge (provisional name; written by the Avatar)
	STAT_COMBO = 22,					// the score multiplier, 1.0 to 2.0
	STAT_QUICK_STREAK = 23,
	STAT_SPEED_CHAIN = 24,
	STAT_SPEED_TIER = 25,
	STAT_SPEED_TIMER = 26,
	STAT_HAMMER_ON_FIRE = 27,
	STAT_KINGS_DESTROYED = 29,
	STAT_PERFECT_BONUS_SCREENS = 30,
	STAT_BONUS_HITS = 31,
	STAT_BONUS_MISSES = 32,
	STAT_LEVEL_OF_LAST_PURCHASE = 33,
	STAT_LEVEL_OF_LAST_HEART = 35,
	STAT_LEVEL_OF_LAST_COIN = 36,
	STAT_FREEZE_TIMER = 45,				// time left of the ice-cream power
	STAT_ROUND_SMASHED = 46,			// gnomes smashed since the last bonus round (ClearBonusCounts clears 45 to 49)
	STAT_STREAK = 47,					// gnomes smashed since the last miss
	STAT_BEST_STREAK = 48,
	STAT_MISSES_IN_LEVEL = 41,			// the three counts of the level being played (ClearLevelCounts)
	STAT_SWINGS_IN_LEVEL = 42
};

class Stats
{
public:
	enum
	{
		NUM_STATS = 50,
		NUM_SAVED_STATS = 25
	};

	std::vector<double>	mValues;

public:
	Stats();													// 0x42f170 (clears, then Reset)
	void			Reset();									// 0x42efe0: all zero, then the starting health and (with the test flags) the starting kit

	double			Get(int theIndex) const;					// 0x42ec50
	int				GetInt(int theIndex) const;					// 0x42efc0: truncated
	void			Set(int theIndex, double theValue);			// 0x42ec10: checks the trophies
	void			SetInt(int theIndex, int theValue);			// 0x42efa0: checks the trophies
	void			AddInt(int theIndex, int theAmount);		// 0x42ef80: checks the trophies
	void			AddIntQuiet(int theIndex, int theAmount);	// 0x42ec30: no trophy check

	void			GetSaved(double* theValues) const;			// 0x42ec60: the 25 values a profile keeps
	void			SetSaved(const double* theValues);			// 0x42ed70
	void			ClearLevelCounts();							// 0x42ee80: three counts of the level just played
	void			ClearBonusCounts();							// 0x42eeb0: five counts of the bonus screen just played
	void			CopyFrom(const Stats& theStats);			// 0x42ef00
};

Stats* GetStats();												// 0x42f1c0: made on first use

// 0x42d0a0: hands the player every trophy whose condition holds now, for the level the game is on plus theLevelOffset
void CheckTrophies(int theLevelOffset);

}

#endif
