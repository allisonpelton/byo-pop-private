#ifndef __BEJEWELED2_SITEPROFILES_H__
#define __BEJEWELED2_SITEPROFILES_H__

// port: Bejeweled 2's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace Sexy
{

class WinDMApp;
class EditWidget;

// The roster changed (a player made, renamed, deleted or chosen): the page is told again
void SiteProfilesChanged(WinDMApp* theApp);

// The new-user dialog's box starts with the site's player under "fill" and "create"
void SiteFillNewUser(EditWidget* theEdit);

}

#endif

#endif
