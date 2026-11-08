#ifndef __ALCHEMY_BOT_H__
#define __ALCHEMY_BOT_H__

// Test hooks: the bot's choice of where the piece to place goes (shared by -autoplay and the `move` test command,
// docs/TEST_COMMANDS.md). The choice reads the board and never changes it. It draws from the bot's own generator (a
// seed passed in), never the game's, so asking for a move changes nothing in the game.
//
// A cell that completes a row or a column is best (the tutorial asks for it too); otherwise the more neighbours a cell
// has, the sooner a row or column fills; a tenth of the choices are a random cell that fits. A piece that fits nowhere
// goes to the forge.

namespace Sexy
{

class Board;

struct BotChoice
{
	enum Kind { KIND_NONE, KIND_PLACE, KIND_FORGE };

	Kind	mKind;
	int		mCol;       // KIND_PLACE: the cell
	int		mRow;
	bool	mDeliberateMiss;    // KIND_PLACE: a cell the piece cannot go to, chosen on purpose (-autoplaymiss)

	BotChoice() : mKind(KIND_NONE), mCol(0), mRow(0), mDeliberateMiss(false) {}
};

// One step of the bot's generator (an LCG): the same numbers -autoplay has always drawn
int BotRand(unsigned int* theSeed, int theRange);

// The bot's choice for the board's piece to place (KIND_NONE when there is none). theMiss > 0: one choice in theMiss is a
// cell the piece cannot go to.
BotChoice ChooseBotMove(Board* theBoard, unsigned int* theSeed, int theMiss);

// Where the pointer goes so that the piece, which hangs 0x30 pixels up and left of it, is over the cell, in game coordinates
void BotCellToPointer(int theCol, int theRow, int* theX, int* theY);

}

#endif
