// origrun's message-box hook (on unless --msgbox show): what the game's side (msgbox.c, inside the dsound stand-in) and
// the host (origrun.c) share. See docs/ORIGRUN.md, "Message boxes and beeps (hooked)".
//
// origrun creates one named file mapping per run before it starts the game, Local\origrun-msgbox-<id>, named after the
// run's id (which it passes in ORIGRUN_MSGBOX_ID), with the script's msgbox rules in it. Each game process of the copy
// that loads the stand-in opens it, answers its boxes by those rules and writes each box and beep to the ring, which
// origrun drains into its own log.
#ifndef ORIGRUN_MSGBOX_H
#define ORIGRUN_MSGBOX_H

#include <windows.h>

#define MBX_MAGIC   0x58424d4fu   // "OMBX"
#define MBX_VERSION 1
#define MBX_RULES   32            // origrun's MAX_MSGBOX
#define MBX_RING    64            // boxes and beeps the game can hand over before origrun drains them

typedef struct MbxRule { char text[128]; char button[32]; } MbxRule;   // "msgbox <button> <text>"

enum { MBX_REC_BOX, MBX_REC_BEEP };
typedef struct MbxRec {
	LONG kind;                 // MBX_REC_*
	DWORD pid;
	char fn[24];               // the hooked function: MessageBoxA, MessageBoxIndirectW, MessageBeep ...
	DWORD type;                // the box's style (MB_*), or MessageBeep's argument
	LONG answer;               // the ID the call returned (IDOK ...; 0: a style Windows refuses, so no box)
	LONG rule;                 // the msgbox rule that chose the button (1-based); 0: the default button
	char caption[256];         // in the ANSI code page, line breaks as \n
	char text[1024];
	char buttons[128];         // "Yes | No", as origrun's dialog watch lists a box's buttons
	char answerLabel[32];
} MbxRec;

typedef struct MbxShared {
	DWORD magic, version, size;
	LONG nrules;               // written by origrun before it sets magic
	MbxRule rules[MBX_RULES];
	volatile LONG installed;   // game processes that turned the hooks on
	volatile LONG patched;     // import table entries pointed at the hooks, in all of them
	volatile LONG boxes;       // boxes answered without a window
	volatile LONG beeps;       // MessageBeep calls answered without a sound
	volatile LONG lock;        // held by a writer while it adds a record
	volatile LONG head;        // records written (the game)
	volatile LONG tail;        // records drained (origrun)
	volatile LONG dropped;     // records not written: the ring was full
	MbxRec ring[MBX_RING];
} MbxShared;

#endif
