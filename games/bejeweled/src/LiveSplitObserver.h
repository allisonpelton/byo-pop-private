#ifndef __LIVESPLITOBSERVER_H__
#define __LIVESPLITOBSERVER_H__

// port(variant): the LiveSplit observer (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15). While the
// features.livesplit mod is on, it fills the LiveSplit block (LiveSplitBlock.h) after every app update:
// the current board's state, and counters for each GO, level clear and game over, which the published
// auto splitter (games/bejeweled/tools/livesplit/BejeweledDeluxe-byopop.asl) turns into start, split and
// reset. It only reads the game: no Rand(), no game state, no saves, no drawing.
//
// Desktop (Windows) and web builds: elsewhere these are empty inlines, the feature is still parsed (so
// mods-status.json is the same) and GameConfig warns that it is ignored. In the web build the page's
// speedrun timer (web/splits.js, web/autosplit.js) turns the block on by asking for its address
// (byopop_livesplit_block), as the feature does, and reads it from the game's memory every frame.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define BEJ_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class WinDMApp;

namespace LiveSplitObserver
{

#ifdef BEJ_HAS_LIVESPLIT
// The feature is on, or it was on and the block still says active (the next AfterUpdate turns it off)
bool			Wanted();
// WinDMApp::UpdateFrames (0x4d4921), after the update and the test hooks
void			AfterUpdate(WinDMApp* theApp);
// ~WinDMApp (0x4d4b08): the last write, active = 0 and boardSerial = 0, so lsprobe and other readers can
// tell a clean quit from a crash. The script ignores inactive copies: it resets on a quit through the
// board-less update after Shutdown deleted a game in progress (resetQuit), or else its exit action
void			AppDestroyed(WinDMApp* theApp);
// -livesplitlog=<file> (WinDMApp::HandleCmdLineParam): one line per block write, for the oracle
void			SetLogFile(const std::string& theFileName);
#else
inline bool		Wanted() { return false; }
inline void		AfterUpdate(WinDMApp*) {}
inline void		AppDestroyed(WinDMApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif

}

}

#endif
