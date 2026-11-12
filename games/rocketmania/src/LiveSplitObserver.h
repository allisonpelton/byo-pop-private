#ifndef __ROCKETMANIA_LIVESPLITOBSERVER_H__
#define __ROCKETMANIA_LIVESPLITOBSERVER_H__

// port: the autosplit observer. Once something asks for the shared autosplit block (platform/AutosplitBlock.h, layout 2) it
// fills the block after every app update with the current game's state and its events (GO, level clear, game over), which the
// site's Splits window (web/autosplit.js) and, natively, a LiveSplit script turn into start, split and reset. It only reads the
// game: no random numbers, no game state, no saves, no drawing, no timing (LiveSplitObserver.cpp says what it reads).
//
// Windows and web builds: elsewhere these are empty inlines. On the web the page's speedrun timer asks by calling the export
// byopop_livesplit_block (which returns the block's address in the game's memory); natively the "livesplit" feature of an enabled
// mod does (a mod.json with "features": {"livesplit": true}).

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define ROCKETMANIA_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class RocketApp;

namespace LiveSplitObserver
{

#ifdef ROCKETMANIA_HAS_LIVESPLIT
// RocketApp::UpdateFrames, after the update
void			AfterUpdate(RocketApp* theApp);
// ~RocketApp: the last write (active = 0, no game), so a reader can tell a clean quit from a crash
void			AppDestroyed(RocketApp* theApp);
// -livesplitlog=<file>: one line for each write of the block that changes what a splitter reads (tests)
void			SetLogFile(const std::string& theFileName);
#else
inline void		AfterUpdate(RocketApp*) {}
inline void		AppDestroyed(RocketApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif

}

}

#endif
