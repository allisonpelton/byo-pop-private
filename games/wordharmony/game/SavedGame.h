#ifndef __WORDHARMONY_GAME_SAVEDGAME_H__
#define __WORDHARMONY_GAME_SAVEDGAME_H__

// A game left to be continued (0x8c bytes, constructor 0x41efe0, destructor 0x41f4e0): the settings keep one per mode
// (Settings::mSavedGames). gameOver fills it with the ball system's saveState and the controller's counts.

#include <string>

class SavedGame
{
public:
	// 0x41efe0: the mode only (the rest is filled before the game is saved)
	explicit SavedGame(int theMode) : mMode(theMode) {}

	int mLevel;           // +0x00
	int mMode;            // +0x04
	int mScore;           // +0x08
	int m0C;              // +0x0c
	int mWildcards;       // +0x10
	int mHintsUsed;       // +0x14
	int mLevelHintsUsed;  // +0x18
	int mHints;           // +0x1c
	int mLevelPoints;     // +0x20
	int mLettersUsed;     // +0x24
	int mTemplate;        // +0x28
	int mTime;            // +0x2c
	std::string m30;      // +0x30
	// +0x4c, +0x5c, +0x6c, +0x7c: four vectors (the ball system's state), with the save code
};

#endif
