#ifndef __BOOKWORM_STATKEEPER_H__
#define __BOOKWORM_STATKEEPER_H__

// StatKeeper (Board.cpp): word and tile statistics of a game (Board::mGameStats) or of the current level
// (Board::mLevelStats). The level summary, the hall of fame and the high-score tables read it.
// Original: class Sexy::StatKeeper, sizeof 0x78, vftable 0x4f9b14 (1 slot: scalar deleting destructor
// 0x4059c0). Offsets in the comments are absolute.
// Behaviour spec: re/bookworm/analysis/board_logic.md 2.4 and 6.3.

#include "Common.h"
#include <cstdio>
#include <string>

namespace Sexy
{

class BookWormApp;
class WordScore;

class StatKeeper
{
public:
	// +0x00 vfptr
	BookWormApp*	mApp;                   // +0x04
	int				mScore;                 // +0x08 words + bonus words
	int				mWords;                 // +0x0c
	int				mLetters;               // +0x10 sum of word lengths ("QU" counts 2)
	int				mTiles;                 // +0x14 sum of tiles used
	int				mFireTilesUsed;         // +0x18
	int				mGreenTilesUsed;        // +0x1c
	int				mGoldTilesUsed;         // +0x20
	int				mSapphireTilesUsed;     // +0x24 never zeroed in the original (port: zeroed, NAMES.md D39)
	int				mDiamondTilesUsed;      // +0x28 never zeroed in the original (port: zeroed)
	int				mTilesBurned;           // +0x2c
	int				mScrambles;             // +0x30
	int				mBonusWords;            // +0x34
	SexyString		mBestWord;              // +0x38 highest-scoring word (the first one wins a tie)
	int				mBestWordScore;         // +0x54
	SexyString		mLongestWord;           // +0x58 (the first one wins a tie)
	int				mLongestWordScore;      // +0x74 score of mLongestWord

public:
	StatKeeper(BookWormApp* theApp);                            // 0x408690
	virtual ~StatKeeper();                                      // 0x405140 body, 0x4059c0 slot 0

	void			Reset();                                    // 0x406950 (+0x24/+0x28 untouched)
	void			AddWord(const WordScore& theWord);          // 0x4068a0
	void			AddBonusWord(int thePoints);                // 0x403690
	void			IncBurned();                                // 0x4036b0
	void			IncScrambles();                             // 0x4036c0
	void			Save(FILE* theFile);                        // 0x404e10
	void			Load(FILE* theFile);                        // 0x405fa0
};

}

#endif
