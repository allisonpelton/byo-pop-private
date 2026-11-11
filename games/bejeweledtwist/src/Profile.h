// The players (0x21c bytes, no virtuals; 0x7333e5-0x736463, app +0x7ec): the list of names in users\users.dat and
// the current player, whose stats, settings and games in progress are in users\<name>\profile.dat. A name that is
// not a valid file name is stored under a folder name with '_' for its odd characters (0x733e9a).
//
// The fields keep the original's offsets in their comments.
#pragma once

#include "Buffer.h"
#include "Common.h"

#include <map>
#include <string>
#include <vector>

namespace Sexy {

class Profile {
public:
                                      // The replay of a best move (0x2c bytes; HighScoresPanel::ButtonDepress plays it)
	struct SavedGame {
		Buffer mData;                 // +0x00
		int mReplayStart;             // +0x20: the board's replay position (+0x23a4) when the move was made, saved as a short
		int mReplayEnd;               // +0x24: the board's replay position (+0x23a8) when the move was made, saved as a short
		int mBest;                    // +0x28: the best move's score, gems or cascade (the high scores' stats page)

		void Read(const Buffer& theBuffer);   // 0x6a3156
		void Write(Buffer& theBuffer) const;  // 0x6a3192
	};

	std::vector<SexyString> mNames;   // +0x00 (first at +0x04): every player, in users.dat's order
	SexyString mName;                 // +0x10: the current player
	bool mIsNew = false;              // +0x54: made this session, not loaded
	int mFirstGameTime = 0;           // +0x58: saved and loaded, never used otherwise
	int mBestScores[6] = {};          // +0x5c: the best score of each mode (classic, blitz, puzzle; zen takes its last)
	int mHighestScore = 0;            // +0x74: the highest score of any game that counts
	int mTotalLevelsDelta = 0;        // +0x78: a running total over games, added from the board counter at +0x14ec
	std::vector<bool> mSavedBools;    // +0x80: a list of bools, saved and loaded; the board counts the true ones, and nothing reads that
	int mTotalPowerGems = 0;          // +0x8c: a running total over games, added from the board counter at +0x1e8
	int mBestChain = 0;               // +0x90: the stats page's best chain
	int mTotalFlames = 0;             // +0x94: a running total over games (not puzzles); 10 sets achievement bit 1
	int mTotalElectros = 0;           // +0x98: a running total over games (not puzzles); 5 sets achievement bit 2
	bool mCustomCursors = false;      // +0x9c: the app's custom cursors (SetCustomCursorsEnabled)
	SavedGame mSavedGames[3];         // +0xa0: the replays of the best moves (the stats page's buttons)
	int mPuzzlesSolved = 0;           // +0x124: saved and loaded, never used otherwise
	int mBestLevels[6] = {};          // +0x128: the highest level reached in each mode
	int mGamesByMode[6] = {};         // +0x140: the games started in each mode
	int mTotalPoints = 0;             // +0x158: every game's points (the stats page)
	int mGemsCleared = 0;             // +0x15c
	int mRankPoints = 0;              // +0x160: the points that make up the rank
	int mAchievementBits = 0;         // +0x164: achievement bits, set as a game earns them
	int mGamesPlayed = 0;             // +0x168
	int mStartingRank = 0;            // +0x16c: 2 for a new player; saved and loaded, never used otherwise
	unsigned char mSavedBits[0x40] = {}; // +0x170: 512 bits, saved and loaded, the first in each byte's top bit; nothing reads them
	bool mShowReminder = false;       // +0x1b0: a flag the app clears when it starts a game (the board does not read it)
	bool mBeforeFirstMove = false;    // +0x1b1
	int mSavedLongs[10] = {};         // +0x1b4: ten saved and loaded values, never used otherwise
	int mLastModePlayed = 0;          // +0x1dc: saved and loaded, never used otherwise
	unsigned int mGemTypesUnlocked = 0; // +0x1e0: the gem types opened, a bit each (3 lightning, 4 flame, 6 ice, 7 angel)
	unsigned int mTipsSeen = 0;       // +0x1e4: the tips seen, a bit each, the tip's kind its bit (0 flame, 14 the basics, 17 the jump offer)
	unsigned int mModesUnlocked = 0;  // +0x1e8: the game modes open to the player, a bit per mode
	int mRank = 0;                    // +0x1ec: the rank (Ranks.h; -1 a new player's)
	int mReplayCount = 0;             // +0x1f0: counts the replays begun; the board takes the count as its id
	std::vector<int> mChallengeStages; // +0x1f4: each challenge category's stage, from 1 (0 closed); saved as shorts
	std::vector<int> mChallengeBests; // +0x204: each challenge category's best score, as many as mChallengeStages
	int mLastChallengeCategory = 0;   // +0x214: the category of the challenge met last (SetChallengeDone)
	int mModesSeenBits = 0;           // +0x218: a bit for each mode the player has picked

                                      // The challenge categories' stage (from 1; 0 for a closed category) and best score; the lists grow to reach theCategory
	void GrowChallengeLists(int theCategory);
	int GetChallengeStage(int theCategory);          // 0x73497f
	bool IsChallengeCategoryOpen(int theCategory);   // 0x7349fc
	void OpenChallengeCategory(int theCategory);     // 0x734a14: stage 1 unless it has got further
                                      // 0x733afc: theNumber of theCategory is met, so its stage is the next one; mLastChallengeCategory gets the category
	void SetChallengeDone(int theCategory, int theNumber);

	Profile();   // 0x734ef5
	~Profile();  // 0x734da6

                                      // 0x735a09: reads users\users.dat into the list, checking each player's file; a player whose file does not load is
	// dropped (and its folder deleted). theLastName gets the name of the player who played last.
	void LoadUsers(SexyString* theLastName);
	void SaveUsers();                               // 0x733f07
	bool Load(const SexyString& theName);           // 0x735001: makes theName the current player
	void Save();                                    // 0x734087: the current player's profile.dat
	bool GetSaveData(std::string* thePath, Buffer* theBuffer);  // port (web): what Save writes, and where
	bool Select(const SexyString& theName);         // 0x7354f8: saves the current player, then loads theName
	bool SelectAny();                               // 0x7359c0: the first player that loads
	// 0x73561d: a new player of that name (or, if the name is taken, the game's "same name" dialog)
	bool Create(const SexyString& theName);        // false when the name is taken
	bool AddName(const SexyString& theName);        // 0x734f91: false when the name is already in the list
	bool RemoveName(const SexyString& theName);     // 0x73485a
	void Delete(const SexyString& theName);         // 0x7348ec: the player and their folder
	void ClearMap();                                // 0x734a73
	SexyString GetName(int theIdx) const;           // 0x73447c: the list's name theIdx ("" out of range)
	// 0x7344c1: the tutorial taken as done, at the first rank (for a player made after the tutorial's player)
	void MarkTutorialDone();

	static std::string GetUserFolder(const SexyString& theName);  // 0x73400c: "<app data>users\<file name>"

	void SetGemTypeUnlocked(int theBit, bool on);           // 0x7337f8
	bool HasSeenTip(int theBit) const { return (mTipsSeen & (1u << (theBit & 31))) != 0; }  // 0x733897
	void SetTipSeen(int theBit, bool on);           // 0x7338b6
                                      // 0x73378b: the mode is open to the player (mModesUnlocked's bit); blitz never in an unregistered copy
	bool IsModeUnlocked(int theMode) const;
	void UnlockMode(int theMode) { mModesUnlocked |= 1u << (theMode & 31); }  // 0x7337c2
};

}  // namespace Sexy
