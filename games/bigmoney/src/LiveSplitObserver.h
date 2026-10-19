#ifndef __BIGMONEY_LIVESPLITOBSERVER_H__
#define __BIGMONEY_LIVESPLITOBSERVER_H__

// port(mods): the LiveSplit observer. While the "livesplit" feature mod is on (ModRules.h), it fills the
// LiveSplit block (LiveSplitBlock.h) after every app update with the current board's state and counters for
// each GO, level clear and game over, which the auto splitter (tools/livesplit/BigMoneyDeluxe-byopop.asl)
// turns into start, split and reset. It only reads the game: no random numbers, no game state, no saves, no
// drawing. On activation it copies the mod's split templates (livesplit/*.lss) to <user dir>/livesplit,
// each only if it is missing there.
//
// Windows builds only (LiveSplit is a Windows program): elsewhere these are empty inlines.

#include <string>

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#define BIGMONEY_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class WinBMApp;

namespace LiveSplitObserver
{

#ifdef BIGMONEY_HAS_LIVESPLIT
// WinBMApp::UpdateFrames, after the update
void			AfterUpdate(WinBMApp* theApp);
// ~WinBMApp: the last write (active = 0, no board), so a reader can tell a clean quit from a crash
void			AppDestroyed(WinBMApp* theApp);
// -livesplitlog=<file>: one line per block write (tests)
void			SetLogFile(const std::string& theFileName);
#else
inline void		AfterUpdate(WinBMApp*) {}
inline void		AppDestroyed(WinBMApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif

}

}

#endif
