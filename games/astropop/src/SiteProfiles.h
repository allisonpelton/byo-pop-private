#ifndef __SITEPROFILES_H__
#define __SITEPROFILES_H__

// port: AstroPop's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace Sexy
{

class AstroPopApp;
class EditWidget;

// The list of players changed (one was made or deleted): the page is told again
void SiteProfilesChanged(AstroPopApp* theApp);

// The new-player box starts with the site's player under "fill" and "create", in the form the game accepts
void SiteFillUserName(EditWidget* theEdit);

}

#endif

#endif
