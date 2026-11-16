// GameStats.cpp: see GameStats.h.

#include "GameStats.h"

#include "DataSync.h"

using namespace Sexy;

// 0x404b70
int Sexy::GetStyleShotPoints(int theStyleShot, bool theSecondTable)
{
	if (!theSecondTable)
	{
		switch (theStyleShot)
		{
		case 0: case 1: case 5: case 7: case 10: case 11: case 12: case 13:
			return 25000;
		case 2: case 3: case 4: case 15:
			return 50000;
		case 6: case 16:
			return 10000;
		case 8: case 14:
			return 100000;
		case 9:
			return 5000;
		default:
			return 0;
		}
	}
	switch (theStyleShot)
	{
	case 0: case 1: case 2: case 3: case 4: case 5: case 7: case 11: case 13: case 16:
		return 5000;
	case 6: case 9: case 10: case 12: case 17: case 18: case 19:
		return 2500;
	case 8: case 14: case 15:
		return 10000;
	default:
		return 0;
	}
}

// 0x402da0
void GameStats::Reset()
{
	mTotalScore = 0;
	mBestShot = 0;
	mBestPegCount = 0;
	for (int i = 0; i < NUM_FREE_BALL_KINDS; i++)
		mFreeBalls[i] = 0;
	for (int i = 0; i < NUM_STYLE_SHOTS; i++)
		mStyleShots[i] = 0;
}

// 0x402e00: a version byte (1), the counts, and the number of style shot counts (20) before them
void GameStats::SyncState(DataSync& theSync)
{
	if (theSync.IsReading())
		Reset();
	int aVersion = 1;
	theSync.SyncByte(aVersion);
	theSync.SyncLong(mTotalScore);
	theSync.SyncLong(mBestShot);
	theSync.SyncLong(mBestPegCount);
	for (int i = 0; i < NUM_FREE_BALL_KINDS; i++)
		theSync.SyncLong(mFreeBalls[i]);
	int aStyleShotCount = NUM_STYLE_SHOTS;
	theSync.SyncLong(aStyleShotCount);
	for (int i = 0; i < aStyleShotCount; i++)
	{
		int aDiscarded = 0;
		theSync.SyncLong(i < NUM_STYLE_SHOTS ? mStyleShots[i] : aDiscarded);
	}
}

// 0x402eb0
void GameStats::CheckBestShot(int theScore)
{
	if (mBestShot < theScore)
		mBestShot = theScore;
}

// 0x402ed0
void GameStats::Add(const GameStats& theOther)
{
	mTotalScore += theOther.mTotalScore;
	if (mBestShot < theOther.mBestShot)
		mBestShot = theOther.mBestShot;
	if (mBestPegCount < theOther.mBestPegCount)
		mBestPegCount = theOther.mBestPegCount;
	for (int i = 0; i < NUM_FREE_BALL_KINDS; i++)
		mFreeBalls[i] += theOther.mFreeBalls[i];
	for (int i = 0; i < NUM_STYLE_SHOTS; i++)
		mStyleShots[i] += theOther.mStyleShots[i];
}

// 0x402f50
int GameStats::GetTotalFreeBalls() const
{
	return mFreeBalls[0] + mFreeBalls[1] + mFreeBalls[2] + mFreeBalls[3];
}

// 0x402f60
int GameStats::GetTotalStyleShots() const
{
	int aTotal = 0;
	for (int i = 0; i < NUM_STYLE_SHOTS; i++)
		aTotal += mStyleShots[i];
	return aTotal;
}

// 0x407460
int GameStats::GetTotalStylePoints() const
{
	int aTotal = 0;
	for (int i = 0; i < NUM_STYLE_SHOTS; i++)
		aTotal += GetStyleShotPoints(i, false) * mStyleShots[i];
	return aTotal;
}
