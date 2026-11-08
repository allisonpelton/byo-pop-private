// Board: the parts of the original that are not translated yet. Each logs once ("not translated yet") and does
// nothing; they go as the real code lands (docs/GAME_NOTES.md lists what is left).

#include "Board.h"
#include <SDL.h>

using namespace Sexy;

#define NOT_TRANSLATED(name, addr) \
	do { static bool aLogged = false; if (!aLogged) { aLogged = true; SDL_Log("AstroPop: Board::" name " not translated yet (" addr ")"); } } while (0)

void Board::Fn46f010()
{
	NOT_TRANSLATED("Fn46f010", "0x46f010");
}

void Board::Fn466e80()
{
	NOT_TRANSLATED("Fn466e80", "0x466e80");
}

void Board::LoadLevel(bool theFlag)
{
	(void) theFlag;
	NOT_TRANSLATED("LoadLevel", "0x4663f0");
}

bool Board::Fn46a810()
{
	NOT_TRANSLATED("Fn46a810", "0x46a810");
	return false;
}

bool Board::AnyMoveLeft(bool* theFlag)
{
	(void) theFlag;
	NOT_TRANSLATED("AnyMoveLeft", "0x459bc0");
	return true;
}
