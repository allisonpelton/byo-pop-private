// Board: the parts of the original that are not translated yet. Each logs once ("not translated yet") and does
// nothing; they go as the real code lands (docs/GAME_NOTES.md lists what is left).

#include "Board.h"
#include <SDL.h>

using namespace Sexy;

#define NOT_TRANSLATED(name, addr) \
	do { static bool aLogged = false; if (!aLogged) { aLogged = true; SDL_Log("AstroPop: Board::" name " not translated yet (" addr ")"); } } while (0)

void Board::Fn475040()
{
	NOT_TRANSLATED("Fn475040", "0x475040");
}


bool Board::Fn46a810()
{
	NOT_TRANSLATED("Fn46a810", "0x46a810");
	return false;
}

void Board::DropUnsupported(std::vector<CellRef>& theCells)
{
	(void) theCells;
	NOT_TRANSLATED("DropUnsupported", "0x46cc20");
}

bool Board::AnyMoveLeft(bool* theFlag)
{
	(void) theFlag;
	NOT_TRANSLATED("AnyMoveLeft", "0x459bc0");
	return true;
}
