#ifndef VENICE_LIVESPLITOBSERVER_H
#define VENICE_LIVESPLITOBSERVER_H

// port: the autosplit observer. Once something asks for the shared autosplit block (platform/AutosplitBlock.h, layout 2) it
// fills the block after every update of the scenes with the state of the game in play and its events (GO, level clear, game
// over, win), which the site's Splits window (web/autosplit.js) and, natively, a LiveSplit script turn into start, split and
// reset. It only reads the game: no random numbers, no game state, no saves, no drawing, no timing (LiveSplitObserver.cpp says
// what it reads).
//
// Windows and web builds: elsewhere these are empty inlines. On the web the page's speedrun timer asks by calling the export
// byopop_livesplit_block (which returns the block's address in the game's memory); natively the "livesplit" feature of an
// enabled mod does (a mod.json with "features": {"livesplit": true}, games/venice/mods/livesplit).

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define VENICE_HAS_LIVESPLIT 1
#endif

namespace LiveSplitObserver
{

#ifdef VENICE_HAS_LIVESPLIT
// RunDueUpdates, after each update of the scenes (the updates a game without the focus does not run are not counted)
void AfterUpdate();
// ~VeniceApp: the last write (active = 0, no board), so a reader can tell a clean quit from a crash
void AppDestroyed();
// -livesplitlog=<file>: one line for each write of the block that changes what a splitter reads (tests)
void SetLogFile(const std::string& theFileName);
#else
inline void AfterUpdate() {}
inline void AppDestroyed() {}
inline void SetLogFile(const std::string&) {}
#endif

}

#endif
