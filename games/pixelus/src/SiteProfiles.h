#ifndef __PIXELUS_SITEPROFILES_H__
#define __PIXELUS_SITEPROFILES_H__

// port: Pixelus's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace Sexy
{

class PixelusApp;
class EditWidget;

// The players' list changed (UserDirectory::Save, which every change ends with): the page is told again
void SiteProfilesChanged(PixelusApp* theApp);

// The new-player and rename dialogs' box starts with the site's player under "fill" and "create" (theRename: the rename dialog,
// which starts with the player's own name and keeps it under "game" and "fill")
void SiteFillNewUser(EditWidget* theEdit);

}

#endif

#endif
