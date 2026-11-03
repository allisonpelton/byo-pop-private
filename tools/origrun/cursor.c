// origrun's scripted cursor, inside the game (--cursor scripted; docs/ORIGRUN.md, "The scripted cursor"). Built into the
// dsound stand-in beside the virtual clock, whose import patcher (vclock.c) points the copy's GetCursorPos imports here.
// The user allowed this hook on 2026-11-03 ("Yes, hook it"): it replaces GetCursorPos, and nothing else.
//
// Once origrun has posted a mouse position (move, click, down ...), GetCursorPos returns that point in screen
// coordinates, as origrun converted it with the game window's client-to-screen offset when it posted it. Before that
// the call goes to Windows (which fails on rundesk's desktop and reads 0,0 in the sandbox). A game that reads the cursor
// after posted input (Bejeweled Twist's board) then sees the scripted mouse where a real one would be.
// Written from the public Win32 documentation.

#include <windows.h>
#include "cursor.h"

void hooks_rescan(void);   // vclock.c: patch the copy's DLLs loaded since it last looked (at most every 15 ms)

static CurShared* gCur;
static LONG gReads, gRealReads;   // this process's

// Called from the stand-in's install (vclock.c) with ORIGRUN_CURSOR=scripted: open origrun's block. 0 (and the error
// in GetLastError) when there is none or it is another version.
int cursor_open(void)
{
	char id[64] = "", name[128];
	GetEnvironmentVariableA("ORIGRUN_CURSOR_ID", id, sizeof(id));
	wsprintfA(name, "Local\\origrun-cursor-%s", id);
	HANDLE m = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);
	CurShared* c = m ? (CurShared*)MapViewOfFile(m, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(CurShared)) : NULL;
	if (!c || c->magic != CUR_MAGIC || c->version != CUR_VERSION || c->size != sizeof(CurShared)) {
		DWORD e = c ? ERROR_REVISION_MISMATCH : GetLastError();
		if (c) UnmapViewOfFile(c);
		if (m) CloseHandle(m);
		SetLastError(e); return 0;
	}
	gCur = c;   // mapped for the life of the process
	return 1;
}
void cursor_installed(void) { InterlockedIncrement(&gCur->installed); }
void cursor_stats(LONG* reads, LONG* real) { *reads = gReads; *real = gRealReads; }

BOOL WINAPI cursor_GetCursorPos(LPPOINT p)
{
	CurShared* c = gCur;
	hooks_rescan();
	if (!c->set || !p) {
		InterlockedIncrement(&gRealReads); InterlockedIncrement(&c->realReads);
		return GetCursorPos(p);   // the stand-in's own import: Windows'
	}
	LONGLONG v = InterlockedCompareExchange64(&c->pos, 0, 0);
	p->x = (LONG)(DWORD)v; p->y = (LONG)(v >> 32);
	InterlockedIncrement(&gReads); InterlockedIncrement(&c->reads);
	return TRUE;
}
