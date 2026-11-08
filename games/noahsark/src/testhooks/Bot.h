#ifndef __NOAH_BOT_H__
#define __NOAH_BOT_H__

// Test hooks: the bot's choice of a move, shared by -autoplay (Autoplay.cpp, which clicks the cells itself) and the
// test command "move" (TestCommands.cpp, which hands the same clicks to a driver as real input). It reads the board and
// moves nothing; it draws from its own generator (a seed the caller keeps), never the game's.

namespace Sexy
{

class Board;
struct Piece;

// the clicks of a move: one cell (a Stampede group) or two (a pair, a swap)
struct BotMove
{
	int		mCount;
	int		mCellX[2];
	int		mCellY[2];
	BotMove() : mCount(0) { mCellX[0] = mCellX[1] = mCellY[0] = mCellY[1] = 0; }
};

int		BotRand(unsigned int& theSeed, int theRange);       // the bot's generator (a linear congruential one)
bool	BotQuiet(Piece* thePiece);                          // an animal that stands still on its cell
bool	BotBoardSettled(Board* theBoard);                   // nothing walks, falls or fades, no pair is being joined
// A move for the game mode on this board (a Stampede group, a puzzle swap, a pair that can be joined), chosen at random
// among the moves there are with theSeed; false when there is none (the generator is then not drawn from)
bool	BotChooseMove(Board* theBoard, int theGameMode, unsigned int& theSeed, BotMove& theMove);

}

#endif
