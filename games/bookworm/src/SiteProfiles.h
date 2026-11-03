#ifndef __BOOKWORM_SITEPROFILES_H__
#define __BOOKWORM_SITEPROFILES_H__

// port: BookWorm's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace Sexy
{

class BookWormApp;
class EditWidget;

// The players' list changed (a player was made, deleted or chosen): the page is told again
void SiteProfilesChanged(BookWormApp* theApp);

// A new-player dialog's box starts with the site's player under "fill" and "create"
void SiteFillNewUser(EditWidget* theEdit);

}

#endif

#endif
