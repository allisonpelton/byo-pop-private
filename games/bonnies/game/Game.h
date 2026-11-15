#ifndef __BONNIES_GAME_H__
#define __BONNIES_GAME_H__

// The game in progress (0x405d60-0x40cd90, 0xd94 bytes, the object 0x4766b8): the board of 9 columns of 9 cells
// (a cell is column * 9 + row, row 8 at the bottom; odd columns sit half a cell lower, so a cell has six
// neighbours), the word being spelled, the score and the player's saved game.

#include "Board.h"
#include "Str.h"

#include <cstring>
#include <new>

namespace bonnies
{

struct Book;

// The player's record (+0xb8c of the game, 0xb4 bytes, ctor 0x40e540, saved by 0x40e440)
struct Record
{
	// 0x40e540
	Record();
	// 0x40e260
	void reset();
	// 0x40e310
	void load(int theFile);
	// 0x40e440
	void save(int theFile);

	int mGames;               // +0x00 (+0xb8c)
	int mBestLevel;           // +0x04 (+0xb90)
	int mLastLevel;           // +0x08 (+0xb94)
	int mWords;               // +0x0c: the words spelled
	int mLetters;             // +0x10: and their letters (the statistics page divides one by the other)
	Str mLongestWord;         // +0x14 (+0xba0): the longest word spelled
	int mBestStreak;          // +0x1c: the most words of five letters or more in a row
	int mStreak;              // +0x20: the words of five letters or more in a row now
	int mSpare;               // +0x24: saved and loaded; nothing else reads or writes it
	int mSeen[33];            // +0x28 (+0xbb4): the hints shown
	int mFixedTileTaps;       // +0xac: the taps on a fixed tile (the first two show its hint)
	int mStarTileTaps;        // +0xb0: the taps on a '*' tile (the first two show its hint)
};

class Game
{
public:
	// The constructor sets only mInProgress, and a cell's mBounces is set only when its tile falls, yet the refill
	// (0x4079a0) reads it for a '*' tile at the bottom and the save (0x406770) writes it for every cell: the original
	// reads whatever its heap held there. The port's game is made on zeroed memory, so those reads give 0 and the
	// saves repeat (DEVIATIONS.md).
	static void* operator new(size_t theSize)
	{
		void* aBlock = ::operator new(theSize);
		std::memset(aBlock, 0, theSize);
		return aBlock;
	}

	// 0x406ff0
	Game();
	// 0x406490
	~Game();

	// 0x406520: the cells of flag 0x20
	int countFlag20();
	// 0x406570: the cells with a tile
	int countTiles();
	// 0x4065d0: the '*' tiles
	int countStars();
	// 0x406710: the level's book chapter
	Book* currentBook();
	// 0x406730
	void setDifficulty(int theDifficulty);
	// 0x406770: saves the game under the player's name (another when that file cannot be written)
	void save(Str theName);
	// 0x406e40: clears flag 0x40 of every cell
	void clearTileMarks();
	// 0x406e80: the difficulty's factor (1, 2 or 4)
	char difficultyFactor();
	// 0x406ea0, 0x406ed0: the path's theIndex-th cell
	int pathColumn(int theIndex);
	int pathRow(int theIndex);
	// 0x406f00
	void setPath(int theIndex, int theColumn, int theRow);
	// 0x406f30, 0x406f90: the letter and the kind of the path's theIndex-th cell
	char pathLetter(int theIndex);
	char pathKind(int theIndex);
	// 0x407090: the word on the path, looked up
	void updateWord();
	// 0x407170: the tiles moved to new cells (theStar adds a '*' first)
	void shuffle(int theStar);
	// 0x4079a0: '*' tiles at the bottom fall out, the tiles fall into the empty cells and new ones come in at the top
	void refill();
	// 0x408440: the neighbour in direction theDirection (0 up, then clockwise), whether it is on the board
	int neighbour(int theColumn, int theRow, int theDirection, int* theNewColumn, int* theNewRow);
	// 0x408530
	void addLevelBonus();
	// 0x4085e0
	void beginLevel();
	// 0x408650: one empty cell gets a tile
	void fillOneTile();
	// 0x408900: the level's board
	int newBoard();
	// 0x408b30: a click or drag on a tile (theColumn -1: off the board); 1 when it is ignored
	int pick(int theColumn, int theRow, int theLastColumn, int theLastRow, int theDrag);
	// 0x4093f0: a word on the board for the hint, whether one was found
	int findHint();
	// 0x409680: the tutorial's hints
	void updateHints();
	// 0x40a8b0, 0x40a8f0, 0x40a930: the path's word, its spelling, whether it is a word
	Str word();
	Str spelling();
	int isWord();
	// 0x40a950: the next level
	void nextLevel();
	// 0x40a990: a new game (theReset: the record too)
	void prepareSave(int theReset);
	// 0x40b100: the saved game theName, played in theMode
	void load(Str theName, int theMode);
	// 0x40b760: the path's word's score
	int wordScore();
	// 0x40b8d0: the path's word spelled
	void submit();
	// port: submit's ending of a won level (the test command "win" calls it too)
	void levelWon();
	// 0x40aa30: the falling tiles, the clock and the end of the game
	void update();

	int mFilling;             // +0x000: newBoard is filling the board
	Str mWord;                // +0x004: the path's letters
	Str mSpelling;            // +0x00c: as the dictionary spells it
	int mIsWord;              // +0x014
	int mDifficulty;          // +0x018
	char mLetters[BOARD_CELLS];        // +0x01c: the tiles (0 none)
	int mKinds[BOARD_CELLS];           // +0x070: 1 '#' (a stone), 2 '(' (a copycat), 4, 8: falling
	uint32_t mTileFlags[BOARD_CELLS];  // +0x1b4: 2 fixed, 4, 0x20, 0x40 marked, 0x80 falling, 0x100
	int mChangeTimes[BOARD_CELLS];     // +0x2f8: when a tile last changed (a stone counted up, a copycat copied, a fixed tile freed)
	char mOldLetters[BOARD_CELLS];     // +0x43c: the letter it had before, drawn as it changes ('^': a freed tile)
	float mFallOffsets[BOARD_CELLS];   // +0x490: how far above its cell a falling tile is
	float mFallSpeeds[BOARD_CELLS];    // +0x5d4
	int mBounces[BOARD_CELLS];         // +0x718
	int mFrom[BOARD_CELLS][2];         // +0x85c: the cell a tile came from in the last shuffle
	int mPathColumns[PATH_LETTERS];     // +0xae4
	int mPathRows[PATH_LETTERS];        // +0xb34
	char mCopyLetter;         // +0xb84: the letter a copycat tile shows
	int mBonusGiven;          // +0xb88: the bonus given on this level (it stops at 20 in a classic game, 40 in an action game)
	Record mRecord;           // +0xb8c
	int mInProgress;          // +0xc40: a game is in progress
	int mGameOver;            // +0xc44: the game is over: its save is not continued
	int mNoMoves;             // +0xc48: the game ended for lack of a move (the shuffle button's dialog)
	Str mLastWord;            // +0xc4c: the last word spelled (the tutorial's steps wait for one)
	int mWordChecked;         // +0xc54
	int mChapterTextShown;    // +0xc58: the chapter's own text has been shown as a hint
	int mClassic;             // +0xc5c: 1 a classic game (no clock: a count of moves), 0 an action game (the clock runs)
	int mGameOverTime;        // +0xc60: when the game ends (0 not set)
	int mLevelStartScore;     // +0xc64: the score the level began with
	int mScore;               // +0xc68
	int mShownScore;          // +0xc6c
	int mScoreTime;           // +0xc70: when the score last changed (the shown score counts up to it)
	int mWildFell;            // +0xc74: a '?' tile came from the super wild odds and its hint is still to be shown
	uint32_t mShuffleTime;    // +0xc78
	int mCopyTime;            // +0xc7c: when a copycat tile last copied a letter
	int mFallFrame;           // +0xc80: the frame a tile last fell
	int mCountTime;           // +0xc84: when the clock or the moves' count last ticked
	int mMovesLag;            // +0xc88: moves given and not yet shown in the count (0 or below)
	float mClockLag;          // +0xc8c: the clock time given and not yet shown (0 or below: the shown clock is mClock + mClockLag)
	int mLevel;               // +0xc90: 1 to 50
	float mClock;             // +0xc94: the clock's time left, 1 to 0 (an action game's)
	int mGoal;                // +0xc98
	int mClockBonus;          // +0xc9c: the level's time bonus, counted into the score at its end
	int mWordsPlayed;         // +0xca0: the words spelled on this level
	int mGameWords;           // +0xca4: the words spelled in this game
	int mShuffles;            // +0xca8: the shuffles on this level
	int mMovesAtHint;         // +0xcac: the words and shuffles when the hint was shown
	float mWildOdds;          // +0xcb4: '?' tiles due
	float mSuperWildOdds;     // +0xcb8: '!' tiles due
	float mStarOdds;          // +0xcbc: '*' tiles due
	float mStoneOdds;         // +0xcc0: '#' tiles due
	float mCopyOdds;          // +0xcc4: '(' tiles due
	Str mLevelBestWord;       // +0xcc8: the longest word of this level (the summary's)
	Str mBestWord;            // +0xcd0: the longest word of the game (the high score table's)
	int mWordFrame;           // +0xcd8: the frame a word was last spelled (nothing reads it)
	int mPathClearTime;       // +0xcdc: when the path was last cleared or spelled (nothing reads it)
	int mPathLength;          // +0xce0
	int mHintFound;           // +0xce4
	int mHintLength;          // +0xce8
	int mHintColumns[PATH_LETTERS];     // +0xcec
	int mHintRows[PATH_LETTERS];        // +0xd3c
	uint32_t mLastMoveTime;   // +0xd8c: when the player last moved (the idle hint's)
	int mShuffleWords;        // +0xd90: the words spelled at the last shuffle (-99 none)
};

// 0x405d60: sparkles from the tile at theColumn, theRow (theSmall: small ones), theCount of them (theExact) or
// theCount * theRate / 1000, the fraction by chance
void sparkleTile(int theColumn, int theRow, float theOffsetY, int theSmall, int theExact, float theCount, int theRate);
// 0x406070: sparkles from every cell, outwards
void sparkleBoard();
// 0x406330: theStr with theValue added
Str& appendInt(Str& theStr, int theValue);
// 0x406360, 0x4063e0: plays a sound
int playSound(const Str& theName, float theVolume, int thePassedOn, int theDelay);
int playSound(const Str& theName);
// 0x401070
int playSound(const Str& theName, float theVolume);
// 0x4024a0: saves the game in progress as theName's
void saveGameAs(const Str& theName);
// 0x4023a0: whether the player theName has a saved game
int playerSaveExists(const Str& theName);
// 0x406620: whether the cells are neighbours, and the direction from the first to the second
int isNeighbour(int theColumn1, int theRow1, int theColumn2, int theRow2, int* theDirection);

} // namespace bonnies

#endif
