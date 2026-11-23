#ifndef __BOOKWORMADV_LIVESPLITOBSERVER_H__
#define __BOOKWORMADV_LIVESPLITOBSERVER_H__

// port(mod): the observer that fills the shared autosplit block (platform/AutosplitBlock.h, layout 2, game id "BWAD")
// after every update, for the site's Splits window and LiveSplit. It only reads the game: no random draw, no game
// state, no save, no drawing, no timing, and no call into the scripts (the profile and the battle's state are read on
// the C++ side). It writes only while the "livesplit" feature of an enabled mod is on or, on the web, once the page has
// asked for the block (byopop_livesplit_block); until then it tests two flags. Windows and web builds only.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define BOOKWORMADV_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class GameApp;

namespace LiveSplitObserver
{

#ifdef BOOKWORMADV_HAS_LIVESPLIT
void AfterUpdate(GameApp* theApp);                   // GameApp::UpdateFrames, after the update
void AppDestroyed(GameApp* theApp);                  // the block's last write
void SetLogFile(const std::string& theFileName);     // -livesplitlog
#else
inline void AfterUpdate(GameApp*) {}
inline void AppDestroyed(GameApp*) {}
inline void SetLogFile(const std::string&) {}
#endif

}  // namespace LiveSplitObserver

}  // namespace Sexy

#endif
