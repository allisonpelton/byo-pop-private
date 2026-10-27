#ifndef __SITEPROFILES_H__
#define __SITEPROFILES_H__

// port: Bookworm Adventures's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace Sexy
{

class GameApp;
class EditWidget;

// The players' list changed (one was made, chosen or deleted): the page is told again
void SiteProfilesChanged(GameApp* theApp);

// The new-player dialog's box starts with the site's player under "fill" and "create", in place of "Lex"
void SiteFillNewUser(EditWidget* theEdit);

}

#endif

#endif
