#ifndef __BONNIES_GLOBALS_H__
#define __BONNIES_GLOBALS_H__

// The game's globals (0x401000-0x41d600 is the game's code; its data around 0x466000-0x47b700) and the exe data it
// reads: font descriptions, image names and texts live in the user's exe and are read through Exe.h, so that none of
// them is in this repository.

#include "Str.h"

namespace bonnies
{

struct FontDesc;
class BonniesApp;
class Game;
class GameScreen;
class Dictionary;
class Music;
class Options;
class Screen;

extern BonniesApp* gBonnies;      // 0x4752a0
extern Options* gOptions;         // 0x475a68
extern Game* gGame;               // 0x4766b8
extern Dictionary* gDictionary;   // 0x475e78
extern Music* gMusic;             // 0x477440

// The screens the app's state shows (0x402fc0), one at a time
extern Screen* gLogoScreen;       // 0x4791b0, state 1
extern Screen* gMenuScreen;       // 0x47a610, state 2
extern GameScreen* gGameScreen;   // 0x478510, state 3
extern Screen* gState4Screen;     // 0x478980, state 4
extern Screen* gState5Screen;     // 0x47ae70, state 5
extern Screen* gCreditsScreen;    // 0x4780e8, state 6
extern Screen* gState7Screen;     // 0x478d98, state 7
extern Screen* gOverlayScreen;    // 0x47b698: always there (0x41d150)

// A font description the exe's static initializers fill (0x24 bytes: two strings and five floats), built once from
// the exe at its address and kept
FontDesc* exeFont(uint32_t theAddress);
// The text of a string literal of the exe
inline const char* lit(uint32_t theAddress);

} // namespace bonnies

#include "Exe.h"

namespace bonnies
{
inline const char* lit(uint32_t theAddress)
{
	return exeString(theAddress);
}
}

#endif
