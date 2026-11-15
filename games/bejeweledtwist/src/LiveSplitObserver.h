#ifndef __BEJEWELEDTWIST_LIVESPLITOBSERVER_H__
#define __BEJEWELEDTWIST_LIVESPLITOBSERVER_H__

// port(mod): the observer that fills the shared autosplit block (platform/AutosplitBlock.h, layout 2, game id "BJTW")
// after every update, for the site's Splits window and LiveSplit. It only reads the game: no random draw, no game
// state, no save, no drawing, no timing. It writes only while the "livesplit" feature of an enabled mod is on or, on
// the web, once the page has asked for the block (byopop_livesplit_block); until then it tests two flags.
// Windows and web builds only.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define BEJEWELEDTWIST_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class WinTwistApp;

namespace LiveSplitObserver
{

#ifdef BEJEWELEDTWIST_HAS_LIVESPLIT
void			AfterUpdate(WinTwistApp* theApp);      // WinTwistApp::UpdateFrames, after the update
void			AppDestroyed(WinTwistApp* theApp);     // the block's last write
void			SetLogFile(const std::string& theFileName);   // -livesplitlog
#else
inline void		AfterUpdate(WinTwistApp*) {}
inline void		AppDestroyed(WinTwistApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif

}

}

#endif
