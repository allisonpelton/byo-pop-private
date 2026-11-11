// origrun's registry map, inside the game (--reg-map popcap; docs/ORIGRUN.md, "The registry map"). Built into the dsound
// stand-in beside the virtual clock, whose import patcher (vclock.c) points the copy's RegCreateKeyExA, RegOpenKeyExA,
// RegSetValueExA and RegQueryValueExA imports here. The user allowed this hook on 2026-11-10 ("Yes hook registry calls");
// it is off unless a run asks for it.
//
// A PopCap game of 2004 keeps its settings and its players under HKEY_LOCAL_MACHINE\SOFTWARE\PopCap\<Game>. A copy that
// origrun starts on the host is not elevated: it can neither make nor write that key, so nothing a run writes is there for
// the next run. With the map on, a create or an open of HKEY_LOCAL_MACHINE with a path that is SOFTWARE\PopCap or lies
// under it is made on HKEY_CURRENT_USER with the same path. The handle the game gets is the HKCU key's, so every later
// call on it (set, query, enumerate, delete, close) works on HKCU with no hook of its own. Nothing else is redirected: any
// other root or path goes to Windows as it is, and so does a key the game reaches from a handle it opened itself
// (HKLM\SOFTWARE, then PopCap\<Game> below it). The HKLM key is not read while the map is on: a copy starts from what
// HKCU\Software\PopCap\<Game> holds. RegSetValueExA and RegQueryValueExA are hooked only to count the values written and
// read through mapped keys, for the log; they change nothing.
// Written from the public Win32 documentation.

#include <windows.h>

void hooks_rescan(void);                 // vclock.c: patch the copy's DLLs loaded since it last looked
void hooks_log(const char* fmt, ...);    // vclock.c: the stand-in's hook log (clock.log, cursor.log or msgbox.log)

static LONG gOpens, gCreates, gMisses, gWrites, gReads, gFound, gLogged;
// The handles mapped opens returned, to count the values that go through them. A handle the game has closed stays
// here until its slot is taken, and Windows may give the same value to another key, so the two value counts are a
// guide, not an account.
#define REG_HANDLES 32
static HKEY gMapped[REG_HANDLES];
static LONG gMappedNext;

void regmap_stats(LONG* creates, LONG* opens, LONG* misses, LONG* writes, LONG* reads, LONG* found)
{
	*creates = gCreates; *opens = gOpens; *misses = gMisses; *writes = gWrites; *reads = gReads; *found = gFound;
}

// The path is SOFTWARE\PopCap or lies under it (letter case as the registry: none).
static int popcap_path(HKEY root, LPCSTR sub)
{
	static const char kKey[] = "SOFTWARE\\PopCap";
	if (root != HKEY_LOCAL_MACHINE || !sub) return 0;
	int i = 0;
	for (; kKey[i]; i++) {
		char a = sub[i], b = kKey[i];
		if (a >= 'a' && a <= 'z') a = (char)(a - 32);
		if (b >= 'a' && b <= 'z') b = (char)(b - 32);
		if (a != b) return 0;
	}
	return sub[i] == 0 || sub[i] == '\\';
}

static void mapped(LSTATUS r, PHKEY res, LONG* count, LPCSTR sub, const char* how)
{
	if (r != ERROR_SUCCESS) { InterlockedIncrement(&gMisses); return; }
	InterlockedIncrement(count);
	if (res && *res) gMapped[(InterlockedIncrement(&gMappedNext) - 1) % REG_HANDLES] = *res;
	if (InterlockedExchange(&gLogged, 1) == 0)   // the one line that says the map took effect
		hooks_log("pid %lu: registry map in effect: %s of HKEY_LOCAL_MACHINE\\%s went to HKEY_CURRENT_USER\\%s", GetCurrentProcessId(), how, sub, sub);
}

static int is_mapped(HKEY k)
{
	if (!k) return 0;
	for (int i = 0; i < REG_HANDLES; i++) if (gMapped[i] == k) return 1;
	return 0;
}

LSTATUS WINAPI reg_RegCreateKeyExA(HKEY k, LPCSTR sub, DWORD reserved, LPSTR cls, DWORD options, REGSAM sam,
	const LPSECURITY_ATTRIBUTES sa, PHKEY res, LPDWORD disposition)
{
	hooks_rescan();
	if (!popcap_path(k, sub)) return RegCreateKeyExA(k, sub, reserved, cls, options, sam, sa, res, disposition);
	LSTATUS r = RegCreateKeyExA(HKEY_CURRENT_USER, sub, reserved, cls, options, sam, sa, res, disposition);
	mapped(r, res, &gCreates, sub, "a create");
	return r;
}

LSTATUS WINAPI reg_RegOpenKeyExA(HKEY k, LPCSTR sub, DWORD options, REGSAM sam, PHKEY res)
{
	hooks_rescan();
	if (!popcap_path(k, sub)) return RegOpenKeyExA(k, sub, options, sam, res);
	LSTATUS r = RegOpenKeyExA(HKEY_CURRENT_USER, sub, options, sam, res);
	mapped(r, res, &gOpens, sub, "an open");
	return r;
}

LSTATUS WINAPI reg_RegSetValueExA(HKEY k, LPCSTR name, DWORD reserved, DWORD type, const BYTE* data, DWORD size)
{
	LSTATUS r = RegSetValueExA(k, name, reserved, type, data, size);
	if (r == ERROR_SUCCESS && is_mapped(k)) InterlockedIncrement(&gWrites);
	return r;
}

LSTATUS WINAPI reg_RegQueryValueExA(HKEY k, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size)
{
	LSTATUS r = RegQueryValueExA(k, name, reserved, type, data, size);
	if (is_mapped(k)) { InterlockedIncrement(&gReads); if (r == ERROR_SUCCESS || r == ERROR_MORE_DATA) InterlockedIncrement(&gFound); }
	return r;
}
