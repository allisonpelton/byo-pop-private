// port: noahsark's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "NoahApp.h"
#include "ImageFont.h"
#include "WebProfiles.h"

using namespace Sexy;

// the score box's own AllowChar: printable ASCII
struct Printable : public EditListener
{
	virtual bool AllowChar(int theId, SexyChar theChar) { return (theChar >= 32) && (theChar < 127); }
};
static Printable sPrintable;

// The score box as the game's dialog makes it: its font and limits (As many characters as fit 120 pixels of the small font; printable ASCII (32 to 126), case kept)
static Byopop::SiteScoreNames Scores(NoahApp* theApp)
{
	return Byopop::SiteScoreNames(theApp, &theApp->mLastScoreName, theApp->mFonts[FONT_SMALL], -1, 0x78,
		"As many characters as fit 120 pixels of the small font; printable ASCII (32 to 126), case kept", &sPrintable);
}

static void Start(SexyAppBase* theApp)
{
	Byopop::SiteScoreNames aScores = Scores((NoahApp*) theApp);
	Byopop::SiteStart("noahsark", aScores);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteScoresChanged(NoahApp* theApp)
{
	Byopop::SiteScoreNames aScores = Scores(theApp);
	Byopop::SiteChanged("noahsark", aScores);
}
#endif
