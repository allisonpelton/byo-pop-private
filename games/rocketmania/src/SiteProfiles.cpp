// port: rocketmania's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "RocketApp.h"
#include "ImageFont.h"
#include "WebProfiles.h"

using namespace Sexy;

// The score box as the game's dialog makes it: its font and limits (As many characters as fit 150 pixels of the dialog font; ASCII, case kept)
static Byopop::SiteScoreNames Scores(RocketApp* theApp)
{
	return Byopop::SiteScoreNames(theApp, &theApp->mLastScoreName, theApp->mFonts[FONT_DOMO14_BOLD], -1, 150,
		"As many characters as fit 150 pixels of the dialog font; ASCII, case kept", NULL);
}

static void Start(SexyAppBase* theApp)
{
	Byopop::SiteScoreNames aScores = Scores((RocketApp*) theApp);
	Byopop::SiteStart("rocketmania", aScores);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteScoresChanged(RocketApp* theApp)
{
	Byopop::SiteScoreNames aScores = Scores(theApp);
	Byopop::SiteChanged("rocketmania", aScores);
}
#endif
