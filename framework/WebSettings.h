#ifndef __WEBSETTINGS_H__
#define __WEBSETTINGS_H__

// port: how a game with a volume engine of its own takes the site's music and sound volumes (docs/WEB_PLAYER.md, "Site settings";
// framework/WebSettings.cpp, byopop_site_apply). Web only: natively this header is empty.
//
// SexyAppBase's SetMusicVolume and SetSfxVolume do not reach a game whose sound has its own volume variables (TipTop's
// gSoundVolume and music table, Ning Po's two settings, Word Harmony's category volumes). Such a game's web-only file sets
// gSiteVolume to a function that sets its own, and byopop_site_apply calls it in place of the framework's.

namespace Byopop
{

// The site's "Always skip start-up logos" (web/settings.js, skipLogos; docs/WEB_PLAYER.md, "Always skip start-up logos"), on both
// builds: true when the page set it before the game started (byopop_site_skiplogos), or natively under -skiplogos
// (SexyAppBase::HandleCmdLineParam), so that a session can test its wiring under rundesk. A game reads it where its start-up
// logos are (the PopCap logo, a publisher's or developer's logo, a "presents" splash; not the loading screen's bar or the
// title screen's own animation) and goes on from each logo as a click or key on it would, the original's own skip. A game
// whose own host parses its command line (not SexyAppBase's) calls SetSiteSkipLogos(true) on -skiplogos itself.
bool SiteSkipLogos();
void SetSiteSkipLogos(bool theSkip);

}

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

// The site's "Always skip tutorials" (web/settings.js; byopop_site_tutorials): a game with a switch of its own for its tutorial
// or first-play help sets gSiteTutorials, in its web-only file, to a function that turns that switch off as its own options
// do (an options dialog's checkbox, a player's "tutorial done" flag), and returns 1 once it has, 0 while it cannot yet (no
// player loaded, the game not started: the page calls again). The site only ever turns the switch off, and only when the
// setting is on: off, its default, leaves the game's own choice alone. The port is unchanged; the function is the only hook.
typedef int (*SiteTutorialsFn)();
extern SiteTutorialsFn gSiteTutorials;

// The site's "3D acceleration" (web/settings.js; byopop_site_accel3d, set by the page before the game starts): -1 leaves it to
// the game's own choice, 1 is 3D always, 0 never. A game with a 3D option reads it where it reads that option; 1 still
// means 2D in a browser without WebGL2, which the game checks as it does for its own option. Read by Bejeweled Twist;
// perhaps later by Bookworm Adventures 1 and 2 and Rocket Mania, once their passes settle their 3D modes.
int SiteAccel3D();

}

#endif

#endif
