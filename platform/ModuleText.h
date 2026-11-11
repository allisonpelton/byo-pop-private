#ifndef __PLATFORM_MODULETEXT_H__
#define __PLATFORM_MODULETEXT_H__

/* A libxmp-lite pattern cell as a tracker shows it: the note, the instrument, the volume and the effect, the effect as
   libxmp-lite's own type in hex ("C-5 01 v40 0F06", dots for what is empty; "===" a key off, "^^^" a cut, "~~~" a fade).
   s holds at least 16 characters. Shared by the site's Music window's two readers of a module's rows: its own decoder
   (web/music/openmpt_web.c) and a game's mixer, for the tune the game plays (platform/Audio.cpp, byopop_music_row; web only).
   C and C++; the libxmp-lite parts only where the includer included <xmp.h> first (OpenMPTPlayer.cpp uses only
   byopop_name_line). */

#include <stdio.h>
#include <string.h>

/* One instrument's name as a line of the Music window's list of them: its number as the cells write it (two hex digits), a
   space, the name without its trailing spaces, '\n'; nothing for an empty name. Bytes outside printable ASCII (a name's
   code page is the tracker's) become '?'. Appended at out + at, at most max bytes in all with the closing 0; returns the
   new length. Used for libopenmpt's names as for libxmp-lite's. */
static inline int byopop_name_line(char* out, int at, int max, int number, const char* name)
{
	static const char hex[] = "0123456789ABCDEF";
	int n = name ? (int)strlen(name) : 0;
	while (n > 0 && (name[n - 1] == ' ' || name[n - 1] == 0)) n--;
	int lead = 0;
	while (lead < n && name[lead] == ' ') lead++;
	if (lead >= n || number < 1 || number > 255 || at + 3 + (n - lead) + 2 > max) return at;
	out[at++] = hex[(number >> 4) & 15];
	out[at++] = hex[number & 15];
	out[at++] = ' ';
	for (int i = lead; i < n; i++) { unsigned char c = (unsigned char)name[i]; out[at++] = c >= 32 && c < 127 ? (char)c : '?'; }
	out[at++] = '\n';
	out[at] = 0;
	return at;
}

#ifdef XMP_KEY_OFF
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

/* A libxmp-lite module's instruments' names (for a MOD or S3M, whose samples are its instruments, the samples' names), as
   byopop_name_line's lines at out, at most max bytes with the closing 0; returns their length. */
static inline int byopop_xmp_names(const struct xmp_module* m, char* out, int max)
{
	int at = 0;
	if (max > 0) out[0] = 0;
	for (int i = 0; i < m->ins; i++) at = byopop_name_line(out, at, max, i + 1, m->xxi[i].name);
	return at;
}

#endif /* XMP_KEY_OFF */

#endif
