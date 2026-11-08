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

void Board::Fn465290()
{
	NOT_TRANSLATED("Fn465290", "0x465290");
}

bool Board::Fn46a810()
{
	NOT_TRANSLATED("Fn46a810", "0x46a810");
	return false;
}

void Board::Fn46e420()
{
	NOT_TRANSLATED("Fn46e420", "0x46e420");
}

void Board::Fn46e9b0()
{
	NOT_TRANSLATED("Fn46e9b0", "0x46e9b0");
}

void Board::Fn45e680()
{
	NOT_TRANSLATED("Fn45e680", "0x45e680");
}
