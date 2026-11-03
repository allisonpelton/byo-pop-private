// port: Atomica's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Atomica keeps no players, and its high-score box (HighScoreDialog, the monitor's name line) starts empty and does not remember
// the name. So there is nothing to rename: when the game has loaded, the page is only told how the site's players' names fit the
// box (20 characters). Web only; natively this file is empty.

#ifdef __EMSCRIPTEN__
#include "AtomicaApp.h"
#include "ImageFont.h"
#include "WebProfiles.h"

using namespace Sexy;

static void Start(SexyAppBase* theApp)
{
	AtomicaApp* anApp = (AtomicaApp*) theApp;
	// the box as HighScoreDialog makes it: mMonitorFontMedium, 20 characters, no width limit
	Byopop::SiteScoreNames aScores(anApp, NULL, anApp->mMonitorFontMedium, 20, -1, "Up to 20 characters; ASCII, case kept");
	Byopop::SiteStart("atomica", aScores);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;
#endif
