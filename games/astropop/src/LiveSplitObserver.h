#ifndef __ASTROPOP_LIVESPLITOBSERVER_H__
#define __ASTROPOP_LIVESPLITOBSERVER_H__

// port(autosplit): the autosplit block's observer (the shared layout 2, platform/AutosplitBlock.h; docs/WEB_PLAYER.md, "One
// design for every port"). Once something asks for the block (the web page's speedrun timer, by calling
// byopop_livesplit_block; natively an enabled mod with the feature "livesplit"; a test, with -livesplitlog=<file>) it fills
// the block after every app update with the current game's state and counters for each GO, level clear and game over, which
// the Splits window (web/autosplit.js) turns into start, split and reset. It only reads the game: no random numbers, no
// game state, no saves, no drawing, no timing. Until something asks, each update costs one test.
//
// Windows and web builds: elsewhere these are empty inlines.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define ASTROPOP_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class AstroPopApp;

namespace LiveSplitObserver
{

#ifdef ASTROPOP_HAS_LIVESPLIT
// AstroPopApp::UpdateFrames, after the update
void			AfterUpdate(AstroPopApp* theApp);
// ~AstroPopApp: the last write (inactive, no game), so a reader can tell a clean quit from a crash
void			AppDestroyed(AstroPopApp* theApp);
// -livesplitlog=<file>: asks for the block and writes a line each time something of it other than the clock and the score changes (tests)
void			SetLogFile(const std::string& theFileName);
#else
inline void		AfterUpdate(AstroPopApp*) {}
inline void		AppDestroyed(AstroPopApp*) {}
inline void		SetLogFile(const std::string&) {}
#endif

}

}

#endif
