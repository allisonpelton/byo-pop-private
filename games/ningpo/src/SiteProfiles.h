#ifndef __NINGPO_SITEPROFILES_H__
#define __NINGPO_SITEPROFILES_H__

// port: Ning Po's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

#include <string>

namespace ningpo
{

// Ning Po keeps no players: it asks for a name at each best time (NingPo::enterName, a text box of 12 characters that starts with
// the account's name) and does not remember it. At the end of init() the page is told how the site's players' names fit the box.
void SiteStart();

// The name the best-time box starts with under "fill" and "create": the site's player as the box takes it (12 characters, the
// printable ones), or "" to leave the game's own (the account's name)
std::string SiteScoreName();

}

#endif

#endif
