// Board: the playfield screen. Only what the objects on it need so far; the rest of the original (constructor
// 0x480020, Update 0x479480, Draw 0x461e50, input 0x45c850 .. 0x478cd0) follows.

#include "Board.h"
#include "AstroPopApp.h"

using namespace Sexy;

Board::Board()
{
	mFlag8E8 = false;
	mFlag8E9 = false;
	mField193C = 0;
	mDropTicks = 0;
	mApp = gApp;
}
