// origrun's scripted cursor and keys (--cursor scripted, --keys scripted): what the game's side (cursor.c, inside the
// dsound stand-in) and the host (origrun.c) share. See docs/ORIGRUN.md, "The scripted cursor" and "The scripted keys".
//
// origrun creates one named file mapping per run before it starts the game, Local\origrun-cursor-<id>, named after the
// run's id (which it passes in ORIGRUN_CURSOR_ID). Each game process of the copy that loads the stand-in opens it, so a
// game that starts itself again reads the same position. The game is started by the origrun that created the block,
// so it runs as the same user and can always open it.
#ifndef ORIGRUN_CURSOR_H
#define ORIGRUN_CURSOR_H

#include <windows.h>

#define CUR_MAGIC   0x52535543u   // "CURS"
#define CUR_VERSION 2

typedef struct CurShared {
	DWORD magic, version, size;
	volatile LONG set;          // origrun sets 1 once it has posted a mouse position; until then GetCursorPos is Windows'
	volatile LONGLONG pos;      // the last posted position in screen coordinates: x in the low 32 bits, y in the high
	volatile LONG posts;        // positions origrun wrote
	volatile LONG installed;    // game processes that patched their GetCursorPos imports
	volatile LONG reads;        // GetCursorPos calls answered with the scripted position
	volatile LONG realReads;    // calls passed to Windows (before origrun's first mouse input)
	volatile LONG keysSet;      // origrun sets 1 once it has posted a key or button edge; until then GetKeyState is Windows'
	volatile LONG keyPosts;     // key and button edges origrun wrote
	volatile LONG keysInstalled; // game processes that patched their GetKeyState imports
	volatile LONG keyReads;     // GetKeyState calls answered with the scripted state
	volatile LONG keyRealReads; // calls passed to Windows (before origrun's first key or button edge)
	volatile BYTE keys[256];    // each virtual key as GetKeyState gives it: 0x80 while down, 0x01 toggled (each down flips it)
} CurShared;

#endif
