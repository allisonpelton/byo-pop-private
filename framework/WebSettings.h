#ifndef __WEBSETTINGS_H__
#define __WEBSETTINGS_H__

// port: how a game with a volume engine of its own takes the site's music and sound volumes (docs/WEB_PLAYER.md, "Site settings";
// framework/WebSettings.cpp, byopop_site_apply). Web only: natively this header is empty.
//
// SexyAppBase's SetMusicVolume and SetSfxVolume do not reach a game whose sound has its own volume variables (TipTop's
// gSoundVolume and music table, Ning Po's two settings, Word Harmony's category volumes). Such a game's web-only file sets
// gSiteVolume to a function that sets its own, and byopop_site_apply calls it in place of the framework's.

#ifdef __EMSCRIPTEN__

namespace Byopop
{

// theMusic and theSfx: -1 leaves the game's own, else a level from 0 to 100. Returns 1 when the volumes are set, 0 when the game
// has not finished starting (the page calls again): the game's own start must come first, as it reads its saved volumes then.
typedef int (*SiteVolumeFn)(int theMusic, int theSfx);
extern SiteVolumeFn gSiteVolume;

// True when the site's "Full screen at start" is "Windowed" (web/settings.js; byopop_site_windowed, set by the page before the game
// starts). A game whose own option can be full screen reads it where it reads that option, and starts with the option off, as
// the player would set it in the game's options: Word Harmony ("FullScreen"), TipTop (gFullScreen), Ning Po ("Fullscreen"), Cosmic Bugs
// and Water Bugs (Screen_SetFullscreen). The SexyAppFramework games need nothing: SexyAppBase always starts the web build windowed.
bool SiteWindowed();

}

#endif

#endif
