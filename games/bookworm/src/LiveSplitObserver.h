#ifndef __BOOKWORM_LIVESPLITOBSERVER_H__
#define __BOOKWORM_LIVESPLITOBSERVER_H__

// port(mod): the LiveSplit observer. While the "livesplit" feature mod is on (ModFeatures.h), it fills the
// LiveSplit block (LiveSplitBlock.h) after every app update with the current board's state and counters for
// each GO, level clear and game over, which the auto splitter (tools/livesplit/BookWormDeluxe-byopop.asl)
// turns into start, split and reset. It only reads the game: no random numbers, no game state, no saves, no
// drawing. On activation it copies the mod's split templates (livesplit/*.lss) to <user dir>/livesplit,
// each only if it is missing there.
//
// Windows builds only (LiveSplit is a Windows program): elsewhere these are empty inlines.

#include <string>

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#define BOOKWORM_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class BookWormApp;

namespace LiveSplitObserver
{

#ifdef BOOKWORM_HAS_LIVESPLIT
// BookWormApp::UpdateFrames, after the update
void			AfterUpdate(BookWormApp* theApp);
// ~BookWormApp: the last write (active = 0, no board), so a reader can tell a clean quit from a crash
void			AppDestroyed(BookWormApp* theApp);
// -livesplitlog=<file>: one line per block write (tests)
void			SetLogFile(const std::string& theFileName);
#else
inline void		AfterUpdate(BookWormApp*) {}
inline void		AppDestroyed(BookWormApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif

}

}

#endif
