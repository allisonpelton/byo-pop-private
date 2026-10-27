// port: bigmoney's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "WinBMApp.h"
#include "RecordScoreDialog.h"
#include "ImageFont.h"
#include "WebProfiles.h"

using namespace Sexy;

// The score box as the game's dialog makes it: its font and limits (As many characters as fit 140 pixels of the dialog font; ASCII, case kept)
static Byopop::SiteScoreNames Scores(WinBMApp* theApp)
{
	return Byopop::SiteScoreNames(theApp, &theApp->mLastScoreName, theApp->mFont3, -1, RecordScoreDialog::NAME_MAX_PIXELS,
		"As many characters as fit 140 pixels of the dialog font; ASCII, case kept", NULL);
}

static void Start(SexyAppBase* theApp)
{
	Byopop::SiteScoreNames aScores = Scores((WinBMApp*) theApp);
	Byopop::SiteStart("bigmoney", aScores);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteScoresChanged(WinBMApp* theApp)
{
	Byopop::SiteScoreNames aScores = Scores(theApp);
	Byopop::SiteChanged("bigmoney", aScores);
}
#endif
