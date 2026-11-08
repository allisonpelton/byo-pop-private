#ifndef __BONNIES_START_H__
#define __BONNIES_START_H__

#include <string>
#include <vector>

// What the port's host calls to start the game (src/main.cpp, src/BonniesHost.cpp), after loadExe and
// runStaticInitializers (engine/Exe.h). The host includes only this, Exe.h and Host.h: the engine's header names
// (Graphics.h, Widget.h, ...) are the framework's too.

namespace bonnies
{

// Dictionary.h, Book.h: the tables copied from the exe
void loadLetterOdds();
void loadBooks();
// BonniesApp.h: 0x401000 WinMain's game
int bonniesMain();

// port (test runs): the board, the hint the game shows, the level and the buttons on screen, as text for
// tools/realplay.py (GameScreen.cpp); the host writes it only in a test run whose BONNIES_STATE names a file
std::string testState();
// port (test runs): the longest word on the board by the game's dictionary, with its tiles' places, for the
// Browser pane's player (src/BonniesHost.cpp, bonnies_test_state; GameScreen.cpp). Read only.
std::string testWord();

#ifdef __EMSCRIPTEN__
// port (web): the game's players for the site's player setting (src/SiteProfiles.cpp; framework/WebProfiles.h), in
// game/SitePlayers.cpp. Names are the game's text.
// The saved players in PlayersPanel::refill's order, and the current one (gOptions->mName, "" with none)
std::vector<std::string> sitePlayers();
std::string siteCurrent();
// theName as askName's box keeps it when typed in (TextField's CHARACTER: mNamesOnly, mMaxLength 20), trimmed as its OK
std::string siteFit(const std::string& theName);
// NamePanel's OK for theName (fitted, not a player yet): saves a new game as theName's and makes it the current player
void siteCreate(const std::string& theName);
// PlayersPanel's choose: theName (one of sitePlayers()) is the current player
void siteSelect(const std::string& theName);
// PlayersPanel's Delete (as the port fixes it): theName's saved game is removed
void siteDelete(const std::string& theName);
// The site's music and sound volumes, 0 to 1 (-1: the game's own), as the options' sliders set them
void siteVolumes(float theMusic, float theSound);
// Set by src/SiteProfiles.cpp: the start (the first menu, before it shows the player), the players changed, and the site
// player's name for askName's box ("" for none)
extern void (*gSiteStart)();
extern void (*gSitePlayersChanged)();
extern std::string (*gSiteNameFor)();
#endif

} // namespace bonnies

#endif
