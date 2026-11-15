#ifndef __PIZZAFRENZY_HIGHSCORE_H__
#define __PIZZAFRENZY_HIGHSCORE_H__

// A high score (0x3c bytes, constructors 0x4184d0 and 0x418880, vtable 0x4fa838): a player's name, the points of the
// game and the day it reached. A board of them (vtable 0x4fd768, made by 0x437a10) orders them by points, the highest
// first, and an equal score goes behind the ones that have it.

#include "Scoreboard.h"

namespace pizzafrenzy
{

class HighScore : public engine::ScoreEntry
{
public:
	HighScore() : mPoints(0), mLevel(0) {}  // 0x4184d0
	HighScore(const std::string& theName, int thePoints, int theLevel) :  // 0x418880
		mName(theName), mPoints(thePoints), mLevel(theLevel)
	{
	}

	// 0x04: 0x4be760
	int getScore() override { return mPoints; }
	// 0x08: 0x418480
	void toParams(engine::Params& theParams) override;
	// 0x0c: 0x418930
	void fromParams(const engine::Params& theParams) override;
	// 0x10: 0x418ab0: the points with a dollar sign before them
	void displayParams(engine::Params& theParams) override;
	// 0x14: 0x4189f0
	void defaultParams(engine::Params& theParams) override;

private:
	std::string mName;  // +0x10
	int mPoints;        // +0x2c
	int mLevel;         // +0x30
};

// The board of a mode's high scores (0x8c bytes, constructed by 0x437a10 with room for ten)
class HighScoreBoard : public engine::Scoreboard
{
public:
	HighScoreBoard(const std::string& theName, int theMaxCount) : Scoreboard(theName, theMaxCount) {}

	// 0x04: 0x436ca0
	engine::ScoreEntry* createEntry() override { return new HighScore(); }
	// 0x08: 0x437120 (0x437150): the first entry with fewer points than the new one
	int findPosition(engine::ScoreEntry* theEntry) override;
};

} // namespace pizzafrenzy

#endif
