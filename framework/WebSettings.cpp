// port: the calls the web page makes for the site's settings (docs/WEB_PLAYER.md, "Site settings"; web/settings.js,
// web/shell.html). Web only; natively this file is empty. Two groups:
//
// The page's mute, byopop_audio_mute() and byopop_audio_muted(), act on the platform's audio output
// (Platform::Mixer::SetPageMute): below every game's own volume, mute and options, so they change and save none of
// them, and they reach every game that plays through the mixer, the SexyAppFramework ones and the engine ones alike.
//
// byopop_site_apply() sets the game's own options that a site setting names: custom cursors, and the music and sound
// volumes. It is the page's one call for them, repeated until the game is up (it returns 0 until the app has
// finished SexyAppBase::Init, which has read the game's saved options, so the setting wins over what the game
// saved), and it does what the game's options dialog does, through the same functions. A value of -1 leaves the
// game's own option alone. A game with a sound engine of its own (TipTop, Ning Po, Word Harmony) registers the function
// that sets its volumes (framework/WebSettings.h), which is called in place of SetMusicVolume and SetSfxVolume.

#ifdef __EMSCRIPTEN__
#include "SexyAppBase.h"
#include "WebSettings.h"
#include "../platform/Audio.h"

#include <emscripten.h>

using namespace Sexy;

// A game with a volume engine of its own sets this (framework/WebSettings.h)
Byopop::SiteVolumeFn Byopop::gSiteVolume = NULL;

extern "C"
{

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
