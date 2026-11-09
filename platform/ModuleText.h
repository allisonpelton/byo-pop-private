#ifndef __PLATFORM_MODULETEXT_H__
#define __PLATFORM_MODULETEXT_H__

/* A libxmp-lite pattern cell as a tracker shows it: the note, the instrument, the volume and the effect, the effect as
   libxmp-lite's own type in hex ("C-5 01 v40 0F06", dots for what is empty; "===" a key off, "^^^" a cut, "~~~" a fade).
   s holds at least 16 characters. Shared by the site's Music window's two readers of a module's rows: its own decoder
   (web/music/openmpt_web.c) and a game's mixer, for the tune the game plays (platform/Audio.cpp, byopop_music_row; web only).
   C and C++; the includer includes <xmp.h> first. */

#include <stdio.h>
#include <string.h>

static void byopop_xmp_cell(const struct xmp_event* e, char* s)
{
	static const char* names[12] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };
	static const char hex[] = "0123456789ABCDEF";
	char note[4] = "...", ins[3] = "..", vol[4] = "...", fx[5] = "....";
	if (e->note == XMP_KEY_OFF) strcpy(note, "===");
	else if (e->note == XMP_KEY_CUT) strcpy(note, "^^^");
	else if (e->note == XMP_KEY_FADE) strcpy(note, "~~~");
	else if (e->note > 0 && e->note <= 120) { int n = e->note - 1; note[0] = names[n % 12][0]; note[1] = names[n % 12][1]; note[2] = (char)('0' + n / 12); }
	if (e->ins) { ins[0] = hex[(e->ins >> 4) & 15]; ins[1] = hex[e->ins & 15]; }
	if (e->vol) { int v = e->vol - 1; vol[0] = 'v'; vol[1] = (char)('0' + (v / 10) % 10); vol[2] = (char)('0' + v % 10); }
	if (e->fxt || e->fxp) { fx[0] = hex[(e->fxt >> 4) & 15]; fx[1] = hex[e->fxt & 15]; fx[2] = hex[(e->fxp >> 4) & 15]; fx[3] = hex[e->fxp & 15]; }
	sprintf(s, "%s %s %s %s", note, ins, vol, fx);
}

#endif
