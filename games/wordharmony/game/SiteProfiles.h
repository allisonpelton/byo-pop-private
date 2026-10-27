#ifndef __WORDHARMONY_SITEPROFILES_H__
#define __WORDHARMONY_SITEPROFILES_H__

// port: Word Harmony's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and src/SiteProfiles.cpp (the part that needs the framework, so it is in the game's
// program and not in this library) compiles to nothing.

#ifdef __EMSCRIPTEN__

#include <string>

class Game;

// Word Harmony keeps no players: it asks for a name at each high score (HighScoreEntryLayout, 16 characters, which starts with the
// last name given, the setting DefaultScoreName). When the game is configured the page is told that name and how the site's
// players' names fit the box, and its queued renames of it are applied.
void SiteStart(Game* theGame);

// The level the site's volume setting gives a category of Word Harmony's sound system (0 the sound, 1 the music), 0 to 1: Game::applyVolumes
// sets a category to this where it set it to 1. It is 1 without a site setting.
float SiteCategoryLevel(int theCategory);

// The name the high-score box starts with under "fill" and "create": the site's player as the box takes it, or "" to leave the
// game's own (the last name given)
std::string SiteScoreName();

#endif

#endif
