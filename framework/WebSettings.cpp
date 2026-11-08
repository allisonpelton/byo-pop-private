// port: the calls the web page makes for the site's settings (docs/WEB_PLAYER.md, "Site settings"; web/settings.js,
// web/shell.html). Web only; natively this file is empty. The calls:
//
// byopop_site_windowed() tells the games the player chose "Windowed" for the site's "Full screen at start" (SiteWindowed, framework/WebSettings.h).
//
// byopop_site_skiplogos() tells them the site's "Always skip start-up logos" (SiteSkipLogos, framework/WebSettings.h, which
// natively -skiplogos sets).
//
// byopop_site_accel3d() tells them the site's "3D acceleration" (SiteAccel3D, framework/WebSettings.h).
//
// byopop_mixer_gain() sets the page's starting volume for the games with no volume option of their own (Cosmic Bugs, Water Bugs), on the
// mixer's music and sound master gains (platform/Audio.h, SetSiteGains).
//
// The page's mute, byopop_audio_mute() and byopop_audio_muted(), act on the platform's audio output
// (Platform::Mixer::SetPageMute): below every game's own volume, mute and options, so they change and save none of
// them, and they reach every game that plays through the mixer, the SexyAppFramework ones and the engine ones alike.
//
// byopop_site_tutorials() turns the game's own tutorial switch off, for the site's "Always skip tutorials", through the function
// the game registers (gSiteTutorials, framework/WebSettings.h).
//
// byopop_site_apply() sets the game's own options that a site setting names: custom cursors, and the music and sound
// volumes. It is the page's one call for them, repeated until the game is up (it returns 0 until the app has
// finished SexyAppBase::Init, which has read the game's saved options, so the setting wins over what the game
// saved), and it does what the game's options dialog does, through the same functions. A value of -1 leaves the
// game's own option alone. A game with a sound engine of its own (TipTop, Ning Po, Word Harmony) registers the function
// that sets its volumes (framework/WebSettings.h), which is called in place of SetMusicVolume and SetSfxVolume.

#include "WebSettings.h"

// "Always skip start-up logos": on both builds (framework/WebSettings.h)
static bool sSiteSkipLogos = false;

bool Byopop::SiteSkipLogos()
{
	return sSiteSkipLogos;
}

void Byopop::SetSiteSkipLogos(bool theSkip)
{
	sSiteSkipLogos = theSkip;
}

#ifdef __EMSCRIPTEN__
#include "SexyAppBase.h"
#include "WebSettings.h"
#include "../platform/Audio.h"

#include <emscripten.h>

using namespace Sexy;

// A game with a volume engine of its own sets this (framework/WebSettings.h)
Byopop::SiteVolumeFn Byopop::gSiteVolume = NULL;
// A game with a tutorial switch of its own sets this (framework/WebSettings.h)
Byopop::SiteTutorialsFn Byopop::gSiteTutorials = NULL;

static bool sSiteWindowed = false;

bool Byopop::SiteWindowed()
{
	return sSiteWindowed;
}

static int sSiteAccel3D = -1;

int Byopop::SiteAccel3D()
{
	return sSiteAccel3D;
}

extern "C"
{

// 1 when the site's "Full screen at start" is "Windowed": called before the game starts (SiteWindowed)
EMSCRIPTEN_KEEPALIVE void byopop_site_windowed(int theWindowed)
{
	sSiteWindowed = theWindowed != 0;
}

// 1 when the site's "Always skip start-up logos" is on, 0 off: called before the game starts (SiteSkipLogos). -1 only reads it.
// Returns the value in effect, 1 or 0.
EMSCRIPTEN_KEEPALIVE int byopop_site_skiplogos(int theSkip)
{
	if (theSkip >= 0)
		Byopop::SetSiteSkipLogos(theSkip != 0);
	return Byopop::SiteSkipLogos() ? 1 : 0;
}

// The site's "3D acceleration", called before the game starts (SiteAccel3D): -1 the game's own choice, 1 always, 0 never
EMSCRIPTEN_KEEPALIVE void byopop_site_accel3d(int theAccel3D)
{
	sSiteAccel3D = theAccel3D < 0 ? -1 : (theAccel3D != 0 ? 1 : 0);
}

// The starting volume of a game that has no volume option of its own: a level from 0 to 100 for the music and for the sounds
// (100 is the game's own volume), through the mixer's gains; a level below 0 leaves that one as it is. Works at any time after the
// module has loaded, also before the game starts. Returns the master gains in effect (the game's times the page's), in percent, as
// music * 1000 + sound, which the page's tests read (-1, -1 only reads them).
EMSCRIPTEN_KEEPALIVE int byopop_mixer_gain(int theMusic, int theSfx)
{
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	aMixer.SetSiteGains(theMusic < 0 ? aMixer.GetSiteMusicGain() : (theMusic > 100 ? 100 : theMusic) / 100.0f,
		theSfx < 0 ? aMixer.GetSiteSfxGain() : (theSfx > 100 ? 100 : theSfx) / 100.0f);
	return (int)(aMixer.GetMusicMasterGain() * 100.0f + 0.5f) * 1000 + (int)(aMixer.GetSfxMasterGain() * 100.0f + 0.5f);
}

// 1 mutes the page's sound, 0 brings it back; works at any time after the module has loaded
EMSCRIPTEN_KEEPALIVE void byopop_audio_mute(int theMuted)
{
	Platform::Mixer::Get().SetPageMute(theMuted != 0);
}

// 1 while the page's mute is on
EMSCRIPTEN_KEEPALIVE int byopop_audio_muted()
{
	return Platform::Mixer::Get().IsPageMuted() ? 1 : 0;
}

// Turns the game's own tutorial switch off (gSiteTutorials). Returns 1 once it is off, 0 when the game cannot take it yet (call
// again later), -1 when the game has registered no switch.
EMSCRIPTEN_KEEPALIVE int byopop_site_tutorials()
{
	if (Byopop::gSiteTutorials == NULL)
		return -1;
	SexyAppBase* anApp = gSexyAppBase;
	if ((anApp != NULL) && ((!anApp->mInitialized) || (anApp->mShutdown)))
		return 0;
	return Byopop::gSiteTutorials() ? 1 : 0;
}

// theCursors: -1 leaves the game's option, 0 turns custom cursors off, 1 on (SexyAppBase::EnableCustomCursors).
// theMusic, theSfx: -1 leaves the game's option, else a level from 0 to 100 (SetMusicVolume, SetSfxVolume, which
// the game's options dialog calls with the slider's 0 to 1). Returns 1 once the settings are applied, 0 when the
// game has not finished starting (or has stopped): call again later.
EMSCRIPTEN_KEEPALIVE int byopop_site_apply(int theCursors, int theMusic, int theSfx)
{
	SexyAppBase* anApp = gSexyAppBase;
	if ((anApp == NULL) || (!anApp->mInitialized) || (anApp->mShutdown))
		return 0;

	if (theCursors >= 0)
		anApp->EnableCustomCursors(theCursors != 0);
	if (Byopop::gSiteVolume != NULL)
	{
		// the game's own volumes (it has its own sound engine): it says when it can take them
		if (((theMusic >= 0) || (theSfx >= 0)) &&
			!Byopop::gSiteVolume(theMusic > 100 ? 100 : theMusic, theSfx > 100 ? 100 : theSfx))
			return 0;
		return 1;
	}
	if (theMusic >= 0)
		anApp->SetMusicVolume((theMusic > 100 ? 100 : theMusic) / 100.0);
	if (theSfx >= 0)
		anApp->SetSfxVolume((theSfx > 100 ? 100 : theSfx) / 100.0);
	return 1;
}

}
#endif
