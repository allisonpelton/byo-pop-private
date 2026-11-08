// The autoplay bot's choice of a move, shared by -autoplay (Autoplay.cpp) and the script command `move`
// (Commands.cpp). It reads the board and moves nothing; its dice are the bot's own (theSeed), never the game's.
#pragma once

#include "Board.h"

namespace Sexy
{

struct BotChoice
{
	enum Kind { FIRE, STAY, SAIL };
	Kind	mKind;      // FIRE: a broadside from where the ship is; STAY: no free cell, a click on the ship; SAIL: a move to a cell
	int		mX;         // the cell clicked
	int		mY;
};

// The bot's move for a board whose ship is waiting (idle or blinking, the level not over, not the bonus screen). theSeed
// is the bot's generator state: the draws are made on it, in the order -autoplay's logs depend on.
BotChoice ChooseBotMove(Board* theBoard, unsigned int& theSeed);

// The click that makes a cell's move, in the game's own coordinates (640 x 480)
void BotCellToGame(Board* theBoard, int theCellX, int theCellY, int* theGameX, int* theGameY);

}
