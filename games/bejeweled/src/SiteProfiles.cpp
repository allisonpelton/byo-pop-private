// port: Bejeweled's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "WinDMApp.h"
#include "RecordScoreDialog.h"
#include "ImageFont.h"
#include "WebProfiles.h"

using namespace Sexy;

// The score box as RecordScoreDialog makes it: mMainFont, 104 pixels, no character limit, no filter of its own
static Byopop::SiteScoreNames Scores(WinDMApp* theApp)
{
	return Byopop::SiteScoreNames(theApp, &theApp->mLastScoreName, theApp->mMainFont, -1, RecordScoreDialog::NAME_MAX_PIXELS,
		"As many characters as fit 104 pixels of the main font; ASCII, case kept");
}

static void Start(SexyAppBase* theApp)
{
	Byopop::SiteScoreNames aScores = Scores((WinDMApp*) theApp);
	Byopop::SiteStart("bejeweled", aScores);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteScoresChanged(WinDMApp* theApp)
{
	Byopop::SiteScoreNames aScores = Scores(theApp);
	Byopop::SiteChanged("bejeweled", aScores);
}
#endif
