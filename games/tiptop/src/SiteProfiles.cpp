// port: TipTop's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "Game.h"
#include "Gui.h"
#include "Music.h"
#include "WebProfiles.h"
#include "WebSettings.h"

#include <string>

using namespace tiptop;

// The score's text field takes at most 10 characters (TextInputProc's creation: the longest text 0xa), the printable ones
static const int kMaxChars = 10;

static bool sStarted = false;

void tiptop::SiteStart()
{
	Byopop::SiteScoreNames aScores(NULL, NULL, NULL, kMaxChars, -1, "Up to 10 characters; ASCII, case kept");
	Byopop::SiteStart("tiptop", aScores);
	sStarted = true;
}

// The site's volumes (framework/WebSettings.h): the options' two sliders' variables, as the sliders set them (SliderProc of the
// sound: gSoundVolume, 0 to 0xffff; of the music: BuildMusicVolumes, 0 to 0x100). After GameInit, which reads Preferences.dat, so
// that the site's value wins over the saved one.
static int Volumes(int theMusic, int theSfx)
{
	if (!sStarted)
		return 0;
	if (theSfx >= 0)
		gSoundVolume = (uint16_t) ((theSfx * 0xffff + 50) / 100);
	if (theMusic >= 0)
		BuildMusicVolumes((uint32_t) ((theMusic * 0x100 + 50) / 100));
	return 1;
}

static struct Register
{
	Register() { Byopop::gSiteVolume = Volumes; }
} sRegister;

void tiptop::SiteFillScoreName(Handler* theField)
{
	std::string aName = Byopop::SiteFitTyped(Byopop::SiteNameFor("tiptop"), NULL, kMaxChars, -1);
	for (size_t i = 0; i < aName.size(); i++)
		Send(theField, MSG_CHAR, (unsigned char) aName[i], 0);
}
#endif
