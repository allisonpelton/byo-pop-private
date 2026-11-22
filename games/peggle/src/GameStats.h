// GameStats.h: the counts a game (or a profile's whole play) adds up (the original's Sexy::GameStats, 0x6c bytes).

#ifndef __GAMESTATS_H__
#define __GAMESTATS_H__

namespace Sexy
{

class DataSync;

static const int NUM_FREE_BALL_KINDS = 4;
static const int NUM_STYLE_SHOTS = 20;
static const int NUM_STYLE_SHOT_GROUPS = 4;
static const int STYLE_SHOTS_PER_GROUP = NUM_STYLE_SHOTS / NUM_STYLE_SHOT_GROUPS;

// 0x404b70: the points one style shot of a kind is worth (the second table is the smaller one used for the second player)
int GetStyleShotPoints(int theStyleShot, bool theSecondTable);

struct GameStats
{
	GameStats() { Reset(); }

	void Reset();					 // 0x402da0
	void SyncState(DataSync& theSync); // 0x402e00
	void CheckBestShot(int theScore); // 0x402eb0
	void Add(const GameStats& theOther); // 0x402ed0
	int GetTotalFreeBalls() const;	 // 0x402f50
	int GetTotalStyleShots() const;	 // 0x402f60
	int GetTotalStylePoints() const; // 0x407460

	int mTotalShots;					 // 0x0: the shots taken (LogicMgr::BeginShot counts it)
	int mBestShot;						 // 0x4: the highest score of one shot
	int mBestPegCount;					 // 0x8
	int mFreeBalls[NUM_FREE_BALL_KINDS]; // 0xc
	int mStyleShots[NUM_STYLE_SHOTS];	 // 0x1c
};

} // namespace Sexy

#endif
