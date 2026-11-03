#ifndef __SITEPROFILES_H__
#define __SITEPROFILES_H__

// port: alchemy's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace Sexy
{

class AlchemyApp;

// The game keeps no players: it asks for a name at each high score and remembers the last one (LastScoreName). When the game has
// loaded (SiteProfiles.cpp registers with the framework), the page is told that name and how the site's players' names would
// fit the score box, and its queued renames of it are applied; when the dialog's OK changes the name, it is told again.
void SiteScoresChanged(AlchemyApp* theApp);

}

#endif

#endif
