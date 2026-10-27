// port: dynomite's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "FossilizedApp.h"
#include "ImageFont.h"
#include "WebProfiles.h"

using namespace Sexy;

// The score box as the game's dialog makes it: its font and limits (Up to 20 characters and as many as fit 115 pixels of the small white font; ASCII, case kept)
static Byopop::SiteScoreNames Scores(FossilizedApp* theApp)
{
	return Byopop::SiteScoreNames(theApp, &theApp->mLastScoreName, theApp->mWhiteFontSmall, 0x14, 0x73,
		"Up to 20 characters and as many as fit 115 pixels of the small white font; ASCII, case kept", NULL);
}

static void Start(SexyAppBase* theApp)
{
	Byopop::SiteScoreNames aScores = Scores((FossilizedApp*) theApp);
	Byopop::SiteStart("dynomite", aScores);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteScoresChanged(FossilizedApp* theApp)
{
	Byopop::SiteScoreNames aScores = Scores(theApp);
	Byopop::SiteChanged("dynomite", aScores);
}
#endif
