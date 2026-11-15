#include "HighScore.h"

#include <cstdio>

using namespace pizzafrenzy;

namespace
{
// The parameters' names (0x5211b4, 0x5211d0 and 0x5211ec)
const char* const PLAYER_PARAM = "player";
const char* const POINTS_PARAM = "points";
const char* const LEVEL_PARAM = "level";
// 0x4fa850: the character shown before the points
const char POINTS_PREFIX = '$';
}

// 0x418480
void HighScore::toParams(engine::Params& theParams)
{
	rankToParams(theParams);
	theParams.setString(PLAYER_PARAM, mName);
	theParams.setInt(POINTS_PARAM, mPoints);
	theParams.setInt(LEVEL_PARAM, mLevel);
}

// 0x418930
void HighScore::fromParams(const engine::Params& theParams)
{
	rankFromParams(theParams);
	mName = theParams.getString(PLAYER_PARAM, mName);
	mPoints = (int)theParams.getInt(POINTS_PARAM, mPoints);
	mLevel = (int)theParams.getInt(LEVEL_PARAM, mLevel);
}

// 0x4189f0
void HighScore::defaultParams(engine::Params& theParams)
{
	rankDefaultParams(theParams);
	theParams.setString(PLAYER_PARAM, "");
	theParams.setInt(POINTS_PARAM, 0);
	theParams.setInt(LEVEL_PARAM, 0);
}

// 0x418ab0
void HighScore::displayParams(engine::Params& theParams)
{
	theParams.setString(PLAYER_PARAM, mName);
	char aNumber[16];
	snprintf(aNumber, sizeof(aNumber), "%d", mPoints);
	theParams.setString(POINTS_PARAM, POINTS_PREFIX + std::string(aNumber));
	snprintf(aNumber, sizeof(aNumber), "%d", mLevel);
	theParams.setString(LEVEL_PARAM, aNumber);
}

// 0x437150: the first entry whose points the new one's are above, a binary search over the entries (the original's
// upper bound over the entries, which a board keeps in order)
int HighScoreBoard::findPosition(engine::ScoreEntry* theEntry)
{
	int aLow = 0;
	int aCount = getCount();
	while (0 < aCount)
	{
		int aHalf = aCount / 2;
		if (theEntry->getScore() <= getEntry(aLow + aHalf)->getScore())
		{
			aLow = aLow + aHalf + 1;
			aCount = aCount - aHalf - 1;
		}
		else
			aCount = aHalf;
	}
	return aLow;
}
