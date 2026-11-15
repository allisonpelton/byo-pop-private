#ifndef __ALCHEMY_LIVESPLITOBSERVER_H__
#define __ALCHEMY_LIVESPLITOBSERVER_H__

// port(mod): Alchemy's autosplit observer. Once something asks for it (natively an enabled mod with the feature "livesplit",
// on the web the page, which calls byopop_livesplit_block), it fills the shared autosplit block (platform/AutosplitBlock.h,
// layout 2, game id "ALDX") after every app update with the current board's state and the counters for each GO, board
// cleared, game over and win, which the Splits window (web/autosplit.js) times Alchemy Deluxe's speedrun.com categories
// from. It only reads the game: no random numbers, no game state, no saves, no drawing, no timing. Until something asks,
// it does nothing but test one flag.
//
// Windows and web builds only (the block is read by LiveSplit and by the page): elsewhere these are empty inlines.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define ALCHEMY_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class WinAlchApp;

namespace LiveSplitObserver
{

#ifdef ALCHEMY_HAS_LIVESPLIT
// WinAlchApp::UpdateFrames, after the update
void			AfterUpdate(WinAlchApp* theApp);
// ~WinAlchApp: the last write (inactive, no board), so a reader can tell a clean quit from a crash
void			AppDestroyed(WinAlchApp* theApp);
// -livesplitlog=<file>: one line per block write (tests)
void			SetLogFile(const std::string& theFileName);
#else
inline void		AfterUpdate(WinAlchApp*) {}
inline void		AppDestroyed(WinAlchApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif

}

}

#endif
