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

void Board::FillTopRow(bool, bool, bool, bool, int)
{
	NOT_TRANSLATED("FillTopRow", "0x451720");
}

void Board::GameOver()
{
	NOT_TRANSLATED("GameOver", "0x450f80");
}

void Board::Fn46e9b0()
{
	NOT_TRANSLATED("Fn46e9b0", "0x46e9b0");
}
