#ifndef __PLATFORM_PLACEHOLDERS_H__
#define __PLATFORM_PLACEHOLDERS_H__

// -placeholders (natively; ?placeholders=1 on the web pages): for testing copies of a game from other sources. A game
// file that is missing gets a stand-in instead of ending the game: an image becomes a magenta and black checkerboard,
// a sound is silent, a font without its descriptor draws with the framework's built-in letters (framework/
// SexyAppBase.cpp GetImage, ImageFont.cpp, SDLSoundManager.cpp). Files that cannot be stood in for (a game's word
// list, its difficulty table) are the game's to handle. Off by default: then a missing file fails as in the original.
//
// Each missing file is logged once ("placeholder: <path>"), written to <userdir>/missing-files.txt natively (the list of
// the last run with the switch on: a run without it leaves the file as it was), passed
// to the web page (Module.onMissingFile(path)) and counted in the mod menu's banner. A run with the switch on keeps
// its high scores apart (the score key gets a "placeholders" entry, platform/ModManager.cpp).

#include <string>
#include <vector>

namespace Platform
{

// Turns the switch on when argv has -placeholders. A game's main() calls it before its game-folder check, which a
// run with the switch lets through with only some of the game's files.
bool InitPlaceholders(int argc, char* argv[]);
void SetPlaceholders(bool on);
bool Placeholders();

// A game file that was missing and got a stand-in (theGamePath as the game asked for it, e.g. images\gemsheet6).
// Each path is recorded once.
void NoteMissing(const std::string& theGamePath);
const std::vector<std::string>& GetMissing();
// When loading has finished (SexyAppBase): logs "placeholders: N game file(s) missing" and writes the list, empty
// too, so a missing-files.txt from an earlier run never stays behind
void ReportMissing();

}

#endif
