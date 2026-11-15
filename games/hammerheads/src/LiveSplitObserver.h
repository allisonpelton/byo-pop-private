#ifndef __HAMMERHEADS_LIVESPLITOBSERVER_H__
#define __HAMMERHEADS_LIVESPLITOBSERVER_H__

// port(autosplit): the autosplit observer. Once something asks for the shared autosplit block (platform/AutosplitBlock.h,
// layout 2, game id HHDX) it fills the block after every app update with the state of the game in play and its events (Go,
// level clear, win, game over), which the site's Splits window (web/autosplit.js) and, natively, a LiveSplit script turn into
// start, split and reset. It only reads the game: no random numbers, no game state, no saves, no drawing, no timing.
//
// The game calls the hooks below at the moments the speedrun rules name; each one only notes the moment, and AfterUpdate (the
// end of the app's update) writes it. Windows and web builds: elsewhere these are empty inlines. On the web the page's speedrun
// timer asks by calling the export byopop_livesplit_block; natively the "livesplit" feature of an enabled mod does, or
// -livesplitlog=<file>, which also logs each change of the block (tests).

#include <string>
#include <vector>

#if defined(_WIN32) || defined(__EMSCRIPTEN__)
#define HAMMERHEADS_HAS_LIVESPLIT 1
#endif

namespace Sexy
{

class Application;

namespace LiveSplitObserver
{

#ifdef HAMMERHEADS_HAS_LIVESPLIT
// Application::UpdateFrames, after the update
void			AfterUpdate(Application* theApp);
// ~Application: the last write (active = 0, no board)
void			AppDestroyed(Application* theApp);
// -livesplitlog=<file>
void			SetLogFile(const std::string& theFileName);

// Application::CreateGameScreen: a game screen with a game on it, new, saved (theResumed) or from a restart point (theResumed)
void			BoardCreated(bool theResumed);
// ReadyGoDialog::ButtonDepress: Go! was pressed on a "Ready?" box
void			GoPressed();
// Application::LevelFinished: theLevel is the level that ended (1-based), theIsWin the game's last
void			LevelEnded(int theLevel, bool theIsWin);
// Gameplay::GameOver: the health ran out
void			GameEnded();
// Application::GameToTrophyRoom: the trophies the room shows
void			TrophiesAwarded(const std::vector<int>& theTrophies);
#else
inline void		AfterUpdate(Application*) {}
inline void		AppDestroyed(Application*) {}
inline void		SetLogFile(const std::string&) {}
inline void		BoardCreated(bool) {}
inline void		GoPressed() {}
inline void		LevelEnded(int, bool) {}
inline void		GameEnded() {}
inline void		TrophiesAwarded(const std::vector<int>&) {}
#endif

}

}

#endif
