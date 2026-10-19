#ifndef __BOOKWORM_WORDSCORE_H__
#define __BOOKWORM_WORDSCORE_H__

// WordScore (Board.cpp): the word spelled by a list of tiles, its tile-type counts, validity and score.
// The Board embeds two: mWord (the current selection) and mLastWord (the word drawn above the board).
// Original: class Sexy::WordScore, sizeof 0x48, vftable 0x4f9b1c (1 slot: scalar deleting destructor
// 0x4059f0). Offsets in the comments are absolute.
// Behaviour spec: re/bookworm/analysis/board_logic.md 2.3 and 6.3.

#include "Common.h"
#include "Piece.h"
#include <cstdio>
#include <set>
#include <string>

namespace Sexy
{

class BookWormApp;

class WordScore
{
public:
	// +0x00 vfptr
	BookWormApp*	mApp;                       // +0x04 set by the Board ctor; Build does nothing while NULL
	SexyString		mWord;                      // +0x08 upper case; a 'Q' tile adds "QU"
	bool			mValid;                     // +0x24 in the dictionary (CheckValid)
	// +0x25..+0x27 padding
	int				mTypeCount[NUM_TILE_TYPES]; // +0x28..+0x3c tiles of each TileType
	int				mNumPieces;                 // +0x40 tiles in the word
	int				mScore;                     // +0x44

public:
	WordScore();                                                // 0x405190
	WordScore(const WordScore& theOther);                       // 0x406560 (BoardDraw::DrawWord)
	virtual ~WordScore();                                       // 0x4051d0 body, 0x4059f0 slot 0
	WordScore&		operator=(const WordScore& theOther);       // 0x407b90

	void			Clear();                                    // 0x4069c0 (mScore is kept)
	void			Build(const PieceList* thePieces);          // 0x406a10
	int				CalcScore();                                // 0x404f60 (the live formula, app mScoringMethod 1)
	int				CalcScoreByLength();                        // 0x4040e0 (mScoringMethod 0: never selected)
	bool			CheckValid(const std::set<SexyString>* theDictionary); // 0x406130
	SexyString		CommonPrefix(const SexyString& theOther);   // 0x406b00 (OverlayWidget::Draw)
	void			Save(FILE* theFile);                        // 0x404fe0
	void			Load(FILE* theFile);                        // 0x406180
};

}

#endif
