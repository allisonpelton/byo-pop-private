#ifndef __MUMMYMAZE_LIVESPLITOBSERVER_H__
#define __MUMMYMAZE_LIVESPLITOBSERVER_H__

// port(mod): the autosplit observer. Once something asks for the block (natively the "livesplit" feature mod, on the web the page's
// speedrun timer, which calls byopop_livesplit_block), it fills the shared autosplit block (platform/AutosplitBlock.h, layout 2)
// after every update with the state of the game in play: its mode, level, treasures, and the moments of its start, each chamber
// solved, the treasure of the last pyramid and Abandon Hope. It only reads the game: no random numbers, no game state, no saves,
// no drawing, no timing. Until it is asked it does nothing but test a flag.
//
// Windows and web builds; elsewhere these are empty inlines.

#include <string>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define MUMMYMAZE_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class MMApp;

namespace LiveSplitObserver
{

#ifdef MUMMYMAZE_HAS_LIVESPLIT
// MMApp::UpdateFrames, after the update
void			AfterUpdate(MMApp* theApp);
// ~MMApp: the last write (active = 0, no board), so that a reader can tell a clean quit from a crash
void			AppDestroyed(MMApp* theApp);
// -livesplitlog=<file>: one line for each event (tests)
void			SetLogFile(const std::string& theFileName);
// A saved game was loaded (the classic game's LoadGame, a saved adventure's slot): the game that starts next resumes it, and its
// start is no run's start
void			SavedGameLoaded();
#else
inline void		AfterUpdate(MMApp*) {}
inline void		AppDestroyed(MMApp*) {}
inline void		SetLogFile(const std::string&) {}
inline void		SavedGameLoaded() {}
#endif

}

}

#endif
