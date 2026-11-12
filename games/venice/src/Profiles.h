// The players' profiles and the high score lists, all kept in one file (userdata\profiles.dat) that the game rewrites whenever
// a profile or a list changes.

#ifndef VENICE_PROFILES_H
#define VENICE_PROFILES_H

#include <cstdint>

// The game modes (a profile's current mode, an index into the lists of per-mode values and the high score lists); the data
// files choose the mode through the set_game_mode and set_mode events
enum GameMode
{
	GameModeFlood = 0,
	GameModeJourney = 1,
	GameModeSurvival = 2,
	GameModeTrickShot = 3,
	GameModeCount = 4,
	HighScoreListCount = 5 // the file holds a fifth list that no mode uses
};

const int ProfileCount = 10;
const int ProfileNameSize = 64;
const int TrickShotRegionCount = 8;
const int TrickShotsPerRegion = 13;
const int TriesPerLevel = 3;

// 0x1278 bytes in the original, and in the file
struct Profile
{
	char mName[ProfileNameSize]; // empty for an unused slot
	int mLevelIndex[GameModeCount]; // the number of the level the player is on, less one, for each mode
	int mPadBeforeInstructionsHidden;
	int mInstructionsHidden; // the first level's instructions are not shown again: set by the tick on the tutorial screen and once the level select screen has been sent
	int mCupidMessageSeen; // set once the cupid has come and its message has been shown
	int mScore;
	int mMode; // the GameMode being played
	int mPadBeforeDifficulty;
	float mDifficulty; // the difficulty the next level starts at (set from the data file's initial value while it is 0)
	char mPadBeforeTipCycle[0x1000];
	int mTipCycle; // 0 to 2: which of the three tips the loading screens show next
	char mPadBeforeStartTries[0x14];
	int mStartTries[GameModeCount]; // reset to 3 with the progress
	char mPadBeforeTriesLeft[4];
	int mTriesLeft[GameModeCount]; // the tries left to finish the level, 3 at the start of each
	char mPadBeforeLevelStartScore[4];
	int mLevelStartScore[GameModeCount]; // the score the player had when the level began
	char mPadBeforeTrickShotDone[4];
	int mTrickShotDone[TrickShotRegionCount][TrickShotsPerRegion]; // 1 for each trick shot the player has made
	int mTricksInARow;
	int mFlagReachedBuilding3; // set once the player has played past level 2
	int mFlagFinishedRelaxed;
	int mFlagFinishedGame;
	int mCupidUsed; // set once a jewel fell in the water outside the tutorial (the cupid saves only the first)
	int mExplainFloodSeen; // set once the flood explanation screen has been shown
};
static_assert(sizeof(Profile) == 0x1278, "a profile is 0x1278 bytes in the file");

// 100 bytes in the original, and in the file
struct HighScoreEntry
{
	char mName[32];
	int mScore;
	char mTimeText[64]; // "m:ss", empty for a score without a time
};
static_assert(sizeof(HighScoreEntry) == 100, "a high score entry is 100 bytes in the file");

const int HighScoresPerList = 10;

extern Profile gProfiles[ProfileCount]; // 0xa1c940
extern int gCurrentProfile; // 0xa29578
extern HighScoreEntry gHighScores[HighScoreListCount][HighScoresPerList]; // 0xa281f0

inline Profile& CurrentProfile() { return gProfiles[gCurrentProfile]; }

void LoadProfiles(); // 0x436300
void SaveProfiles(); // 0x435e70
void SetDefaultProfiles(); // 0x435f10: also the lists' starting scores (0x408a20)
bool EnterHighScore(const char* theTimeText); // 0x4363d0: the current profile's score goes in its mode's list
void ResetProgress(); // 0x436ae0: starts the current mode over
void ClearTrickShotProgress(); // 0x408910
int CountProfiles(); // 0x436bb0
const char* FormatTime(unsigned int theTicks); // 0x43a530: "m:ss" of a time in 4 ms ticks

#endif
