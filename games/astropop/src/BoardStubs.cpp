// Board: the parts of the original that are not translated yet. Each logs once ("not translated yet") and does
// nothing; they go as the real code lands (docs/GAME_NOTES.md lists what is left).

#include "Board.h"
#include <SDL.h>

using namespace Sexy;

#define NOT_TRANSLATED(name, addr) \
	do { static bool aLogged = false; if (!aLogged) { aLogged = true; SDL_Log("AstroPop: Board::" name " not translated yet (" addr ")"); } } while (0)

void Board::Fn465840(int theStage)
{
	(void) theStage;
	NOT_TRANSLATED("Fn465840", "0x465840");
}

bool Board::Fn468e70()
{
	NOT_TRANSLATED("Fn468e70", "0x468e70");
	return false;
}

bool Board::AnyMoveLeft(bool* theFlag)
{
	(void) theFlag;
	NOT_TRANSLATED("AnyMoveLeft", "0x459bc0");
	return true;
}

void Board::BuildStoryText()
{
	NOT_TRANSLATED("BuildStoryText", "0x47a940");
}

void Board::UpdateSupercharge(bool theCheck, int theColour)
{
	(void) theCheck;
	(void) theColour;
	NOT_TRANSLATED("UpdateSupercharge", "0x45ac80");
}
