#ifndef __BEJ2_LIVESPLITOBSERVER_H__
#define __BEJ2_LIVESPLITOBSERVER_H__

// port(mod): Bejeweled 2's autosplit observer. Once something asks for it (natively an enabled mod with the feature "livesplit",
// on the web the page, which calls byopop_livesplit_block), it fills the shared autosplit block (platform/AutosplitBlock.h,
// layout 2, game id "B2DX") after every update with the state of the board and the counters for each GO, level cleared, game
// over and win, which the Splits window (web/autosplit.js) times Bejeweled 2's speedrun.com categories from. It only reads the
// game: no random numbers, no game state, no saves, no drawing, no timing. Until something asks, it does nothing but test one
// flag. The game's own code only counts (Board::mBoardSerial, mResumed, mLevelClears, mWins), and nothing reads those back.
//
// Windows and web builds only (the block is read by LiveSplit and by the page): elsewhere these are empty inlines.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define BEJ2_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class WinDMApp;

namespace LiveSplitObserver
{
#ifdef BEJ2_HAS_LIVESPLIT
// WinDMApp::UpdateFrames, after the update
void			AfterUpdate(WinDMApp* theApp);
// ~WinDMApp: the last write (inactive, no board), so a reader can tell a clean quit from a crash
void			AppDestroyed(WinDMApp* theApp);
// -livesplitlog=<file>: a line whenever a field the rules read changes (tests)
void			SetLogFile(const std::string& theFileName);
#else
inline void		AfterUpdate(WinDMApp*) {}
inline void		AppDestroyed(WinDMApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif
}

}

#endif
