#ifndef __DYNOMITE_LIVESPLITOBSERVER_H__
#define __DYNOMITE_LIVESPLITOBSERVER_H__

// port(mod): Dynomite's autosplit observer. Once something asks for it (natively an enabled mod with the feature "livesplit",
// on the web the page, which calls byopop_livesplit_block), it fills the shared autosplit block (platform/AutosplitBlock.h,
// layout 2, game id "DYDX") after every app update with the current game's state and the counters for each GO, level
// cleared, game over and win, which the Splits window (web/autosplit.js) times Dynomite Deluxe's speedrun.com categories
// from. It only reads the game: no random numbers, no game state, no saves, no drawing, no timing. Until something asks,
// it does nothing but test one flag.
//
// Windows and web builds only (the block is read by LiveSplit and by the page): elsewhere these are empty inlines.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define DYNOMITE_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class FossilizedApp;

namespace LiveSplitObserver
{
#ifdef DYNOMITE_HAS_LIVESPLIT
// FossilizedApp::UpdateFrames, after the update
void			AfterUpdate(FossilizedApp* theApp);
// ~FossilizedApp: the last write (inactive, no game), so a reader can tell a clean quit from a crash
void			AppDestroyed(FossilizedApp* theApp);
// -livesplitlog=<file>: a line whenever a field the rules read changes (tests)
void			SetLogFile(const std::string& theFileName);
#else
inline void		AfterUpdate(FossilizedApp*) {}
inline void		AppDestroyed(FossilizedApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif
}

}

#endif
