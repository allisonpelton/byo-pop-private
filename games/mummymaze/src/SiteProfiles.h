#ifndef __SITEPROFILES_H__
#define __SITEPROFILES_H__

// port: Mummy Maze's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace Sexy
{

class MummyApp;
class EditWidget;

// The adventures' list changed (one was made or deleted): the page is told again
void SiteProfilesChanged(MummyApp* theApp);

// The new-adventure dialog's box starts with the site's player under "fill" and "create", in the form the game accepts (letters and
// numbers, 20 characters)
void SiteFillAdventureName(EditWidget* theEdit);

}

#endif

#endif
