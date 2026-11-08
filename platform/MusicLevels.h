/* The originals' music engines against the port's renderers (docs/WEB_PLAYER.md, "Music levels"): the render settings and
   the levels the port plays a game's music at, shared by the games (platform/Audio.cpp, platform/openmpt/OpenMPTPlayer.cpp,
   framework/SDLMusicInterface.cpp, a game's own host) and the site's music player (web/music/openmpt_web.c, web/music.js), so
   that a tune plays as loud in the player as in its game. C, so that the player's decoder can include it.

   The renderers: S3M, IT, XM and MOD go through libxmp-lite and MO3 and OXM through libopenmpt, as Platform::Mixer picks them.
   Measured on 2026-11-08 with tools/musiclevel (the first 30 s of every game's modules, each engine's own DLL decoding silently
   against the port's mixer): libxmp-lite is within about 1 dB of BASS 1.7, 1.8, 2.0 and 2.3 at their default amplify (50) and
   of FMOD 3.50 to 3.61 at full master volume, on S3M and IT alike (Seven Seas' IT 1.9 dB); libopenmpt on the same S3M and IT
   files was 2 to 11 dB quieter, which is why the player renders them with libxmp-lite too. On MO3, libopenmpt against BASS
   2.2 and 2.3 at amplify 50 was between 1.6 dB louder and 2.4 dB quieter from one module to the next, with no setting that
   explains it; it is left as it is. */
#ifndef BYOPOP_MUSIC_LEVELS_H
#define BYOPOP_MUSIC_LEVELS_H

#include <math.h>

/* libopenmpt as the port renders MO3 and OXM: BASS 2.0's linear interpolation (2 taps), full stereo separation */
#define BYOPOP_OPENMPT_INTERPOLATION 2
#define BYOPOP_OPENMPT_SEPARATION 100
/* libxmp-lite's interpolation (XMP_INTERP_SPLINE) and its default mixing otherwise */
#define BYOPOP_XMP_INTERP 2

/* FMOD 3 mixes an XM at a sample pre-amp of 256 where libopenmpt uses 48 (soundlib/Sndfile.cpp; MixLevels::Compatible,
   Sndmix.cpp): Bonnie's Bookstore's OXM plays at this gain (its menu music recorded through origrun was within 1% after) */
#define BYOPOP_FMOD3_XM_GAIN (256.0 / 48.0)

/* BASS_MUSIC_ATTRIB_AMPLIFY (BASS 2.3 holds 0 to 100; its default is 50) as a gain in dB against libopenmpt's level, measured
   with BASS 2.3.0.1 decoding the first 60 s of Bookworm Adventures' music.mo3 at each value, interpolated in dB between them.
   Only Bookworm Adventures and its Volume 2 set it (to 12.0, held at 100); a song whose amplify is never set keeps
   libopenmpt's level. theAmp is BASS's value, 0 to 100 (more is held at 100). */
static inline double ByopopBassAmplifyDB(int theAmp)
{
	static const int kAmp[] = {0, 10, 25, 40, 50, 60, 75, 90, 100};
	static const double kDB[] = {-20.13, -16.59, -12.16, -8.09, -4.55, -2.07, 2.81, 7.49, 9.98};
	int n = (int)(sizeof(kAmp) / sizeof(kAmp[0])), i = 1;
	if (theAmp < 0)
		theAmp = 0;
	if (theAmp > 100)
		theAmp = 100;
	while (i < n - 1 && kAmp[i] < theAmp)
		i++;
	return kDB[i - 1] + (kDB[i] - kDB[i - 1]) * (theAmp - kAmp[i - 1]) / (double)(kAmp[i] - kAmp[i - 1]);
}

static inline double ByopopBassAmplifyGain(int theAmp)
{
	return pow(10.0, ByopopBassAmplifyDB(theAmp) / 20.0);
}

#endif
