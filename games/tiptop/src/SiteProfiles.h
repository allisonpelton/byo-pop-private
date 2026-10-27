#ifndef __TIPTOP_SITEPROFILES_H__
#define __TIPTOP_SITEPROFILES_H__

// port: TipTop's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace tiptop
{

struct Handler;

// TipTop keeps no players: it asks for a name at each high score (a text field of 10 characters, which starts empty) and does not
// remember it. At the end of GameInit the page is told how the site's players' names fit that field.
void SiteStart();

// Under "fill" and "create" the high score's text field starts with the site's player: its characters are typed into the field
// (the way keys reach it) as it is made
void SiteFillScoreName(Handler* theField);

}

#endif

#endif
