// Sexy::ZenBoard: see the header.
#include "ZenBoard.h"

namespace Sexy {

// 0x7126f0 (bejeweled3): Board's constructor 0x74ae30, then the vftable; the "Title" of the settings is "Zen" (the song and the log's mode)
ZenBoard::ZenBoard()
{
	mSettings["Title"] = "Zen";
}

// slot 154 (0x268), 0x70e450 (bejeweled3)
int ZenBoard::GetLevelMax() const
{
	int aIndex = mLevelIndex;
	if (aIndex > 0x1d)
		aIndex = 0x1e;
	return aIndex * 0x2ee + 0x9c4;
}

// slot 161 (0x284), 0x70e4a0 (bejeweled3): a move is made for the player instead of the game's end. The two draws are of the board's
// own generator (+0x258: 0x5865a0 with the address in EDX), in this order: the column, then the row
void ZenBoard::NoMoreMoves(bool theAnnounce)
{
	(void)theAnnounce;
	unsigned long aCol = mRand.Next() & 7;
	unsigned long aRow = mRand.Next() & 7;
	if (aRow < ROWS && aCol < COLS && mGrid[aRow][aCol] != nullptr)
		MakeHypercube(mGrid[aRow][aCol], true);   // slot 167 (0x29c)
}

}  // namespace Sexy
