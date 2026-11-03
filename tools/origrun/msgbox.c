// origrun's message-box hook, inside the game (on unless --msgbox show; docs/ORIGRUN.md, "Message boxes and beeps
// (hooked)"). Built into the dsound stand-in beside the virtual clock and the scripted cursor, whose import patcher
// (vclock.c) points the copy's imports of MessageBoxA/W, MessageBoxExA/W, MessageBoxIndirectA/W, MessageBoxTimeoutA/W
// and MessageBeep here. The user allowed these hooks on 2026-11-03 ("Yes, hook them"), and nothing else is hooked.
//
// A hooked box makes no window, so Windows plays no sound for it and the game keeps the focus. Its caption, text and
// buttons go to origrun through the shared block (msgbox.h), which logs them in the words of its dialog watch, and the
// call returns the button that watch would have clicked: the one a msgbox rule names (the first rule whose text the
// caption or text contains and whose button the box has), else the box's default button (MB_DEFBUTTON1 to 4 pick the
// nth of its type's buttons; the first when there is no such button). MessageBeep is logged and returns TRUE with no
// sound. Written from the public Win32 documentation; MessageBoxTimeout, which user32 exports without documenting it,
// takes MessageBoxEx's arguments and then a timeout in milliseconds.

#include <windows.h>
#include <string.h>
#include "msgbox.h"

void hooks_rescan(void);                 // vclock.c: patch the copy's DLLs loaded since it last looked
void hooks_log(const char* fmt, ...);    // vclock.c: the stand-in's hook log (clock.log, cursor.log or msgbox.log)

static MbxShared* gMbx;
static LONG gBoxes, gBeeps;   // this process's

// Called from the stand-in's install (vclock.c) with ORIGRUN_MSGBOX=hook: open origrun's block. 0 (and the error in
// GetLastError) when there is none or it is another version.
int msgbox_open(void)
{
	char id[64] = "", name[128];
	GetEnvironmentVariableA("ORIGRUN_MSGBOX_ID", id, sizeof(id));
	wsprintfA(name, "Local\\origrun-msgbox-%s", id);
	HANDLE m = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);
	MbxShared* s = m ? (MbxShared*)MapViewOfFile(m, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(MbxShared)) : NULL;
	if (!s || s->magic != MBX_MAGIC || s->version != MBX_VERSION || s->size != sizeof(MbxShared)) {
		DWORD e = s ? ERROR_REVISION_MISMATCH : GetLastError();
		if (s) UnmapViewOfFile(s);
		if (m) CloseHandle(m);
		SetLastError(e); return 0;
	}
	gMbx = s;   // mapped for the life of the process
	return 1;
}
void msgbox_installed(void) { InterlockedIncrement(&gMbx->installed); }
void msgbox_patched(void) { InterlockedIncrement(&gMbx->patched); }
void msgbox_stats(LONG* boxes, LONG* beeps) { *boxes = gBoxes; *beeps = gBeeps; }

// Case-insensitive substring search, as origrun's dialog watch matches a rule (an empty needle always matches).
static int istr_has(const char* hay, const char* needle)
{
	if (!needle || !*needle) return 1;
	size_t nl = strlen(needle);
	for (; *hay; hay++) if (_strnicmp(hay, needle, nl) == 0) return 1;
	return 0;
}

// A box's text for the log (line breaks as \n, so it stays on one line) and for matching rules (line breaks as spaces).
static void flatten(const char* s, char* esc, int ecap, char* plain, int pcap)
{
	int e = 0, p = 0;
	for (; s && *s; s++) {
		int br = (*s == '\r' || *s == '\n');
		if (br && s[0] == '\r' && s[1] == '\n') s++;
		if (br) { if (e + 2 < ecap) { esc[e++] = '\\'; esc[e++] = 'n'; } if (p + 1 < pcap) plain[p++] = ' '; }
		else { if (e + 1 < ecap) esc[e++] = *s; if (p + 1 < pcap) plain[p++] = *s; }
	}
	esc[e] = 0; plain[p] = 0;
}

// Wide text in the ANSI code page, as origrun's dialog watch reads a box (GetWindowTextA). At most 2047 characters.
static const char* from_w(LPCWSTR s, char* out, int cap)
{
	if (!s) return NULL;
	WCHAR t[2048]; lstrcpynW(t, s, 2048);
	if (!WideCharToMultiByte(CP_ACP, 0, t, -1, out, cap, NULL, NULL)) out[0] = 0;
	return out;
}

// The buttons of a box's style, in Windows' order, and the IDs they return; MB_HELP adds Help last. 0 for a style
// Windows refuses (a type above MB_CANCELTRYCONTINUE), which shows no box.
typedef struct { const char* label; int id; } Btn;
static int box_buttons(UINT type, Btn* b)
{
	static const Btn kOk = { "OK", IDOK }, kCancel = { "Cancel", IDCANCEL }, kAbort = { "Abort", IDABORT },
		kRetry = { "Retry", IDRETRY }, kIgnore = { "Ignore", IDIGNORE }, kYes = { "Yes", IDYES }, kNo = { "No", IDNO },
		kTry = { "Try Again", IDTRYAGAIN }, kCont = { "Continue", IDCONTINUE }, kHelp = { "Help", IDHELP };
	int n = 0;
	switch (type & MB_TYPEMASK) {
	case MB_OK:                b[n++] = kOk; break;
	case MB_OKCANCEL:          b[n++] = kOk; b[n++] = kCancel; break;
	case MB_ABORTRETRYIGNORE:  b[n++] = kAbort; b[n++] = kRetry; b[n++] = kIgnore; break;
	case MB_YESNOCANCEL:       b[n++] = kYes; b[n++] = kNo; b[n++] = kCancel; break;
	case MB_YESNO:             b[n++] = kYes; b[n++] = kNo; break;
	case MB_RETRYCANCEL:       b[n++] = kRetry; b[n++] = kCancel; break;
	case MB_CANCELTRYCONTINUE: b[n++] = kCancel; b[n++] = kTry; b[n++] = kCont; break;
	default: return 0;
	}
	if (type & MB_HELP) b[n++] = kHelp;
	return n;
}

// The record to origrun's ring. A full ring (origrun drains it every 250 ms) or a writer that never lets go of the lock
// loses the record, counted in dropped; the stand-in's own log still has the box.
static void report(const MbxRec* r)
{
	MbxShared* s = gMbx;
	int got = 0;
	for (int i = 0; i < 2000 && !got; i++) { if (!InterlockedCompareExchange(&s->lock, 1, 0)) got = 1; else SwitchToThread(); }
	if (!got) { InterlockedIncrement(&s->dropped); return; }
	LONG h = s->head;
	if (h - s->tail < MBX_RING) {
		s->ring[h % MBX_RING] = *r;
		MemoryBarrier();
		InterlockedExchange(&s->head, h + 1);
	} else InterlockedIncrement(&s->dropped);
	InterlockedExchange(&s->lock, 0);
}

// Every hooked box ends here, its caption and text in the ANSI code page (a NULL caption is Windows' "Error").
static int answer_box(const char* fn, LPCSTR cap, LPCSTR text, UINT type)
{
	hooks_rescan();
	MbxShared* s = gMbx;
	MbxRec r; ZeroMemory(&r, sizeof(r));
	r.kind = MBX_REC_BOX; r.pid = GetCurrentProcessId(); r.type = type;
	lstrcpynA(r.fn, fn, sizeof(r.fn));
	char capPlain[256], textPlain[1024], combined[1300];
	flatten(cap ? cap : "Error", r.caption, sizeof(r.caption), capPlain, sizeof(capPlain));
	flatten(text ? text : "", r.text, sizeof(r.text), textPlain, sizeof(textPlain));
	Btn b[4]; int n = box_buttons(type, b);
	for (int i = 0; i < n; i++) { if (i) lstrcatA(r.buttons, " | "); lstrcatA(r.buttons, b[i].label); }
	if (!n) {   // MessageBox refuses the style and shows nothing
		report(&r);
		hooks_log("%s (pid %lu): style 0x%x has no message box type Windows knows: returned 0, as Windows does, with no box "
			"(caption='%s' text='%s')", fn, (unsigned long)r.pid, type, r.caption, r.text);
		SetLastError(ERROR_INVALID_MSGBOX_STYLE);
		return 0;
	}
	wsprintfA(combined, "%.255s %.1023s", capPlain, textPlain);
	int pick = -1;
	for (LONG k = 0; k < s->nrules && k < MBX_RULES && pick < 0; k++) {
		if (!istr_has(combined, s->rules[k].text)) continue;
		for (int i = 0; i < n; i++) if (b[i].id != IDHELP && !lstrcmpiA(b[i].label, s->rules[k].button)) { pick = i; r.rule = k + 1; break; }
	}
	if (pick < 0) {
		pick = (int)((type & MB_DEFMASK) >> 8);
		if (pick >= n) pick = 0;
		if (b[pick].id == IDHELP) pick = 0;   // Help only sends WM_HELP and leaves the box up; MessageBox never returns it
	}
	r.answer = b[pick].id;
	lstrcpynA(r.answerLabel, b[pick].label, sizeof(r.answerLabel));
	report(&r);
	InterlockedIncrement(&gBoxes); InterlockedIncrement(&s->boxes);
	hooks_log("%s (pid %lu): caption='%s' text='%s' buttons=[%s] style 0x%x: answered '%s' (%s) with no window", fn,
		(unsigned long)r.pid, r.caption, r.text, r.buttons, type, r.answerLabel, r.rule ? "script rule" : "default button");
	return r.answer;
}

int WINAPI mbx_MessageBoxA(HWND h, LPCSTR text, LPCSTR cap, UINT type) { return answer_box("MessageBoxA", cap, text, type); }
int WINAPI mbx_MessageBoxExA(HWND h, LPCSTR text, LPCSTR cap, UINT type, WORD lang) { return answer_box("MessageBoxExA", cap, text, type); }
int WINAPI mbx_MessageBoxTimeoutA(HWND h, LPCSTR text, LPCSTR cap, UINT type, WORD lang, DWORD ms)
{ return answer_box("MessageBoxTimeoutA", cap, text, type); }
int WINAPI mbx_MessageBoxW(HWND h, LPCWSTR text, LPCWSTR cap, UINT type)
{ char c[1024], t[4096]; return answer_box("MessageBoxW", from_w(cap, c, sizeof(c)), from_w(text, t, sizeof(t)), type); }
int WINAPI mbx_MessageBoxExW(HWND h, LPCWSTR text, LPCWSTR cap, UINT type, WORD lang)
{ char c[1024], t[4096]; return answer_box("MessageBoxExW", from_w(cap, c, sizeof(c)), from_w(text, t, sizeof(t)), type); }
int WINAPI mbx_MessageBoxTimeoutW(HWND h, LPCWSTR text, LPCWSTR cap, UINT type, WORD lang, DWORD ms)
{ char c[1024], t[4096]; return answer_box("MessageBoxTimeoutW", from_w(cap, c, sizeof(c)), from_w(text, t, sizeof(t)), type); }

// MessageBoxIndirect's text and caption may be string resources of its hInstance (the exe's when it gives none).
int WINAPI mbx_MessageBoxIndirectA(const MSGBOXPARAMSA* p)
{
	if (!p) return 0;
	HINSTANCE hi = p->hInstance ? p->hInstance : GetModuleHandleA(NULL);
	char c[1024], t[4096];
	LPCSTR cap = p->lpszCaption, text = p->lpszText;
	if (cap && IS_INTRESOURCE(cap)) { if (!LoadStringA(hi, (UINT)(UINT_PTR)cap, c, sizeof(c))) c[0] = 0; cap = c; }
	if (text && IS_INTRESOURCE(text)) { if (!LoadStringA(hi, (UINT)(UINT_PTR)text, t, sizeof(t))) t[0] = 0; text = t; }
	return answer_box("MessageBoxIndirectA", cap, text, p->dwStyle);
}
int WINAPI mbx_MessageBoxIndirectW(const MSGBOXPARAMSW* p)
{
	if (!p) return 0;
	HINSTANCE hi = p->hInstance ? p->hInstance : GetModuleHandleA(NULL);
	char c[1024], t[4096]; WCHAR w[2048];
	const char* cap = NULL; const char* text = NULL;
	if (p->lpszCaption && IS_INTRESOURCE(p->lpszCaption)) { if (!LoadStringW(hi, (UINT)(UINT_PTR)p->lpszCaption, w, 2048)) w[0] = 0; cap = from_w(w, c, sizeof(c)); }
	else cap = from_w(p->lpszCaption, c, sizeof(c));
	if (p->lpszText && IS_INTRESOURCE(p->lpszText)) { if (!LoadStringW(hi, (UINT)(UINT_PTR)p->lpszText, w, 2048)) w[0] = 0; text = from_w(w, t, sizeof(t)); }
	else text = from_w(p->lpszText, t, sizeof(t));
	return answer_box("MessageBoxIndirectW", cap, text, p->dwStyle);
}

BOOL WINAPI mbx_MessageBeep(UINT type)
{
	hooks_rescan();
	MbxRec r; ZeroMemory(&r, sizeof(r));
	r.kind = MBX_REC_BEEP; r.pid = GetCurrentProcessId(); r.type = type; r.answer = TRUE;
	lstrcpynA(r.fn, "MessageBeep", sizeof(r.fn));
	report(&r);
	InterlockedIncrement(&gBeeps); InterlockedIncrement(&gMbx->beeps);
	hooks_log("MessageBeep(0x%x) (pid %lu): returned TRUE with no sound", type, (unsigned long)r.pid);
	return TRUE;
}
