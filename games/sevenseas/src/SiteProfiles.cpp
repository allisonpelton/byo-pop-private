// port: sevenseas's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "SevenSeasApp.h"
#include "ImageFont.h"
#include "WebProfiles.h"

using namespace Sexy;

// the score box's own AllowChar: printable ASCII
struct Printable : public EditListener
{
	virtual bool AllowChar(int theId, SexyChar theChar) { return (theChar >= 32) && (theChar < 127); }
};
static Printable sPrintable;

// The score box as the game's dialog makes it: its font and limits (As many characters as fit 104 pixels of the dialog font; printable ASCII (32 to 126), case kept)
static Byopop::SiteScoreNames Scores(SevenSeasApp* theApp)
{
	return Byopop::SiteScoreNames(theApp, &theApp->mLastScoreName, theApp->mFonts[FONT_MT11], -1, 0x68,
		"As many characters as fit 104 pixels of the dialog font; printable ASCII (32 to 126), case kept", &sPrintable);
}

static void Start(SexyAppBase* theApp)
{
	Byopop::SiteScoreNames aScores = Scores((SevenSeasApp*) theApp);
	Byopop::SiteStart("sevenseas", aScores);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteScoresChanged(SevenSeasApp* theApp)
{
	Byopop::SiteScoreNames aScores = Scores(theApp);
	Byopop::SiteChanged("sevenseas", aScores);
}
#endif
