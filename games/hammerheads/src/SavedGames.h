#ifndef __HAMMERHEADS_SAVEDGAMES_H__
#define __HAMMERHEADS_SAVEDGAMES_H__

// What a player's file keeps of games (0x469300-0x46e500): the saved game of each mode that "Continue" picks up, and the lists of
// restart points ("Resurrect") of the classic and the tough mode. The saved game holds the whole state of a game on screen: the
// stats, the cubbies with their creatures, the jesters, the aviators, the stickers, the order of the kinds of gnome, the clock,
// the shop or the challenge when one is open. The User's methods that fill it and put it back are in SavedGames.cpp, the ones
// that write it to the file and read it from there in ProfileFile.cpp.

#include <string>
#include <vector>

#include "Cubby.h"
#include "GameScreen.h"
#include "GnomeBouncing.h"
#include "GnomeFlying.h"
#include "Stats.h"

namespace Sexy
{

enum
{
	NUM_SAVED_GAMES = 3,			// one for each game mode (classic, tough cookie, marathon)
	SAVED_GAME_VERSION = 7			// what a saved game stores in mVersion; HasSavedGame wants it
};

// A sticker of the board: its name and whether it has been struck (0x4435f0 copies it out, 0x443120 puts it back)
struct StickerState
{
	std::string		mName;
	bool			mHit;

	StickerState() : mHit(false) {}
};

// The game at the start of a level, from which a game can go on: the numbers of a profile (Stats::GetSaved), the mode and the
// level. 0xd8 bytes in the original.
struct RestartPoint
{
	double			mStats[Stats::NUM_SAVED_STATS];	// +0x00 to +0xc0
	int				mMode;							// +0xc8
	int				mLevel;							// +0xcc: the level that was started; a game takes up at the next one
	int				mBonusRounds;					// +0xd0: the bonus rounds taken so far (the game screen's mBonusRoundsDone)

	RestartPoint();										// 0x469300 (the original leaves the stats after the fifth uninitialised; here they are 0)
	explicit RestartPoint(const std::string& theState);	// 0x4696a0
	void			Capture();							// 0x469350: from the game on screen
	void			Restore() const;					// 0x4693b0: puts the numbers back and starts the mode at the level after
	std::string		ToString() const;					// 0x469420
};

// The state of the game in one mode (0x140 bytes in the original)
struct SavedGame
{
	int				mVersion;							// +0x00: SAVED_GAME_VERSION once a game has been stored
	int				mState;								// +0x04: 0 none; 3 just stored; 1 written to the file; 2 read from it and found whole
	int				mLevel;								// +0x08
	double			mTime;								// +0x10: the seconds the level has run (the game screen's mLevelTime)
	int				mBonusRounds;						// +0x18: the file's "challengenum"
	int				mBoardOrder[GameScreen::NUM_SHUFFLED_BOARDS];	// +0x1c: the shuffled boards of the mode
	int				mCycle;								// +0x48: the file's "gnomebagfills"
	std::vector<double> mStats;							// +0x4c: all of Stats::mValues
	std::vector<CubbyState> mCubbies;					// +0x5c
	double			mTimerTotal;						// +0x70: the level clock (Timer)
	double			mTimerLeft;							// +0x78
	double			mTimerRate;							// +0x80
	int				mTimerPhase;						// +0x88
	bool			mInLevelIntro;						// +0x8c: the level's introduction is on screen
	int				mGameplayPhase;						// +0x90: Gameplay::mPhase
	double			mPhaseTimer;						// +0x98
	double			mNextPopTimer;						// +0xa0
	double			mCloverTimer;						// +0xa8
	double			mStrawberryTimer;					// +0xb0
	double			mDoubleHitTimer;					// +0xb8
	double			mFiveTimesTimer;					// +0xc0
	bool			mPopNow;							// +0xc8
	bool			mFirstCubbyStarted;					// +0xc9
	bool			mSaveItemHasBeenCreated;			// +0xca: Gameplay::mChallengeDone (the original stores the two flags crossed over, and puts them back the same way)
	bool			mMustCreateSaveItem;				// +0xcb: Gameplay::mChallengeShown
	bool			mFlagCC;							// +0xcc: Gameplay::mFlagCC
	int				mClicksOnLevel;					// +0xd0: the avatar's click count
	std::vector<BouncingState> mBouncing;				// +0xd4
	std::vector<FlyingState> mFlying;					// +0xe4
	std::vector<StickerState> mStickers;				// +0xf4
	std::vector<int> mKindList;							// +0x104: the game screen's list of kinds, and where in it (mKindIndex) the next one is
	int				mKindIndex;							// +0x114
	bool			mInShop;							// +0x118
	bool			mInChallenge;						// +0x119
	std::string		mChallengeState;					// +0x11c: what the challenge screen saved (its ValueList's text)
	bool			mInChallengeExplain;				// +0x138: the "Bonus Round!" box is up

	SavedGame();										// 0x46d440
};

}

#endif
