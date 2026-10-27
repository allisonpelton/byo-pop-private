// port: Ning Po's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "NingPo.h"
#include "WebProfiles.h"
#include "WebSettings.h"

using namespace ningpo;

// NingPo::enterName's box: setMaxLength(0xc), and the characters from 0x20 up (PGDialogText::handleEvent)
static const int kMaxChars = 12;

static bool sStarted = false, sReady = false, sMusicSet = false;

void ningpo::SiteStart()
{
	Byopop::SiteScoreNames aScores(NULL, NULL, NULL, kMaxChars, -1, "Up to 12 characters; ASCII, case kept");
	Byopop::SiteStart("ningpo", aScores);
	sStarted = true;
}

// The site's volumes (framework/WebSettings.h): the options' two settings, 0 to 1, as their sliders set them (SoundSlider and
// MusicSlider in NingPoDialogs.cpp: the music one also sets the playing music's volume). After loadSettings, which has read them
// from the registry, so that the site's value wins over the saved one; the playing music takes it once init() is done.
static int Volumes(int theMusic, int theSfx)
{
	NingPo* anApp = GetNingPo();
	if (!sStarted || (anApp == NULL))
		return 0;
	if (theSfx >= 0)
		anApp->mSoundSetting = theSfx / 100.0f;
	if (theMusic >= 0)
	{
		anApp->mMusicSetting = theMusic / 100.0f;
		sMusicSet = true;
		if (sReady)
			anApp->setMusicVolume(anApp->mMusicSetting);
	}
	return 1;
}

void ningpo::SiteReady()
{
	sReady = true;
	// only what the site set: with no site volume the game's music keeps the volume it has
	NingPo* anApp = GetNingPo();
	if (sMusicSet && (anApp != NULL))
		anApp->setMusicVolume(anApp->mMusicSetting);
}

static struct Register
{
	Register() { Byopop::gSiteVolume = Volumes; }
} sRegister;

std::string ningpo::SiteScoreName()
{
	return Byopop::SiteFitTyped(Byopop::SiteNameFor("ningpo"), NULL, kMaxChars, -1);
}
#endif
