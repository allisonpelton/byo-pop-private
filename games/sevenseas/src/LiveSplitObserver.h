#ifndef __SEVENSEAS_LIVESPLITOBSERVER_H__
#define __SEVENSEAS_LIVESPLITOBSERVER_H__

// port(mod): the observer that fills the shared autosplit block (platform/AutosplitBlock.h, layout 2, game id "SSDX")
// after every update, for the site's Splits window and LiveSplit. It only reads the game: no random draw, no game
// state, no save, no drawing, no timing. It writes only while the "livesplit" feature of an enabled mod is on or, on
// the web, once the page has asked for the block (byopop_livesplit_block); until then it tests two flags.
// Windows and web builds only.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define SEVENSEAS_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class SevenSeasApp;

namespace LiveSplitObserver
{

#ifdef SEVENSEAS_HAS_LIVESPLIT
void			AfterUpdate(SevenSeasApp* theApp);     // SevenSeasApp::UpdateFrames, after the update
void			AppDestroyed(SevenSeasApp* theApp);    // the block's last write
void			SetLogFile(const std::string& theFileName);   // -livesplitlog
#else
inline void		AfterUpdate(SevenSeasApp*) {}
inline void		AppDestroyed(SevenSeasApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif

}

}

#endif
