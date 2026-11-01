// origrun: run a COPY of an original PopCap game, invisibly and (by default) silently, so a port session
// can compare the original with its build screen by screen. See docs/ORIGRUN.md.
//
//   origrun --game <name> --script <file.txt> [options]
//   origrun --exe "<path to a copied exe>" --script <file.txt> [options]
//
// What it does, in order:
//   1. Copies the game's folder into build/origrun/<run>/game (the install is never written to or run in
//      place). --exe runs an already-placed copy as-is.
//   2. Places the sound stand-in (dsound.dll) beside the copied exe, unless --sound real.
//   3. Launches the copy through build/native/rundesk.exe, which puts it on a private, invisible desktop
//      at below-normal priority. The game inherits that desktop; nothing is shown and focus is never taken.
//   4. From this host (not injected into the game) it finds the game's top-level window on that desktop and
//      runs the input script: waits, mouse moves/clicks, keys, typed text, focus lose/regain, and screen
//      shots to PNG. Input is posted to the window, so it works across the desktop boundary.
//   5. Watches Windows Core Audio for a session belonging to the game; if one appears it mutes it and notes
//      it in the run log (a stand-in let sound through, or --sound real was used).
//   6. Waits for the game to exit, or ends it (and its children, via rundesk's job) at the timeout.
//   7. With the game's registry key and 3D-flag address known (games.tsv), logs the 3D mode the game ran in, read
//      from its memory, and puts its Is3D setting and cached Direct3D test results back afterwards; --3d on|off
//      sets the setting for the run and fails the run (exit code 3) when the game ends up in the other mode.
//
// Captures are taken only inside Windows Sandbox (--sandbox; docs/ORIGRUN.md, "Windows Sandbox"): there a capture can
// see only the sandbox's own display. A run on the host is scripted input without shots.
//
// Build: tools/origrun/build.sh (llvm-mingw i686). Written from the public Win32 API; no other project's
// DirectDraw or DirectSound wrapper was read.

#define COBJMACROS
#define INITGUID
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif
#define PSAPI_VERSION 2   // EnumProcessModulesEx and GetModuleBaseNameA from kernel32 (K32...), no psapi.lib
#include <psapi.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <lm.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <tlhelp32.h>
#include <ddraw.h>
#include <iphlpapi.h>
#include <bcrypt.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <audioclient.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// --------------------------------------------------------------------------------------------------- util

static void die(const char* msg) { fprintf(stderr, "origrun: %s\n", msg); exit(2); }
static FILE* gLogFile = NULL;               // <run>/logs/origrun.log, opened once gRunDir is known
static CRITICAL_SECTION gLogLock; static int gLogLockInit = 0;
static void logline(const char* fmt, ...)
{
	if (gLogLockInit) EnterCriticalSection(&gLogLock);
	SYSTEMTIME st; GetLocalTime(&st);
	char ts[16]; wsprintfA(ts, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
	char body[2048]; va_list ap; va_start(ap, fmt); _vsnprintf(body, sizeof(body)-1, fmt, ap); va_end(ap); body[sizeof(body)-1]=0;
	printf("%s%s\n", ts, body); fflush(stdout);
	if (gLogFile) { fprintf(gLogFile, "%s%s\n", ts, body); fflush(gLogFile); }
	if (gLogLockInit) LeaveCriticalSection(&gLogLock);
}

static int ieq(const char* a, const char* b) { return lstrcmpiA(a, b) == 0; }

// Case-insensitive substring search (needle may be empty -> always matches).
static int istr_has(const char* hay, const char* needle)
{
	if (!needle || !*needle) return 1;
	size_t nl = strlen(needle);
	for (; *hay; hay++) if (_strnicmp(hay, needle, nl) == 0) return 1;
	return 0;
}

// Copy a control/button label, dropping the '&' accelerator marks, for matching and logging.
static void strip_amp(const char* in, char* out, int cap)
{
	int j = 0;
	for (; *in && j < cap-1; in++) if (*in != '&') out[j++] = *in;
	out[j] = 0;
}

// Join a dir and a leaf into out (MAX_PATH).
static void joinp(char* out, const char* a, const char* b) { wsprintfA(out, "%s\\%s", a, b); }

// Create a directory and every missing parent (like mkdir -p).
static void mkdirs(const char* path)
{
	char tmp[MAX_PATH]; lstrcpynA(tmp, path, MAX_PATH);
	for (char* p = tmp + 3; *p; p++) {          // skip drive "C:\"
		if (*p == '\\' || *p == '/') { char c = *p; *p = 0; CreateDirectoryA(tmp, NULL); *p = c; }
	}
	CreateDirectoryA(tmp, NULL);
}

// --------------------------------------------------------------------------------------------------- copy

static void copy_tree(const char* src, const char* dst)
{
	CreateDirectoryA(dst, NULL);
	char pat[MAX_PATH]; joinp(pat, src, "*");
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA(pat, &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		if (ieq(fd.cFileName, ".") || ieq(fd.cFileName, "..")) continue;
		char s[MAX_PATH], d[MAX_PATH];
		joinp(s, src, fd.cFileName); joinp(d, dst, fd.cFileName);
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) copy_tree(s, d);
		else {
			if (!CopyFileA(s, d, FALSE)) logline("warning: could not copy %s (err %lu)", fd.cFileName, GetLastError());
			SetFileAttributesA(d, FILE_ATTRIBUTE_NORMAL);
		}
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

// Some Steam copies load Steam.dll from the Steam library root (a parent of the game folder), which the copy
// does not include -> the game shows "Unable to load Steam.dll". If the copy lacks one, look up the source's
// parents (the Steam layout: ...\SteamLibrary\steamapps\common\<game>) for Steam.dll and copy it beside the exe.
static void ensure_steam_dll(const char* srcDir, const char* gameDir)
{
	char dst[MAX_PATH]; joinp(dst,gameDir,"Steam.dll");
	if (GetFileAttributesA(dst)!=INVALID_FILE_ATTRIBUTES) return;   // the copy already has one
	char up[MAX_PATH]; lstrcpynA(up,srcDir,MAX_PATH);
	for (int lvl=0; lvl<5; lvl++) {
		char* slash=strrchr(up,'\\'); if(!slash) break; *slash=0;
		char cand[MAX_PATH]; joinp(cand,up,"Steam.dll");
		if (GetFileAttributesA(cand)!=INVALID_FILE_ATTRIBUTES) {
			if (CopyFileA(cand,dst,FALSE)) logline("copied Steam.dll from the library root into the copy: %s",cand);
			else logline("warning: could not copy Steam.dll from %s (err %lu)",cand,GetLastError());
			return;
		}
	}
}

// ------------------------------------------------------------------------------------------------- globals

static char   gRepo[MAX_PATH];       // the repository root (origrun.exe sits in build/origrun-bin/)
static char   gRunDir[MAX_PATH];     // build/origrun/<run>
static char   gGameDir[MAX_PATH];    // <run>/game
static char   gExePath[MAX_PATH];    // copied exe
static char   gShotDir[MAX_PATH];
static volatile LONG gAudioSeen = 0;
static volatile LONG gStop = 0;

#define MAX_MSGBOX 32
static struct { char text[128]; char button[32]; } gMsgbox[MAX_MSGBOX];
static volatile LONG gMsgboxN = 0;
static DWORD  gGamePidHint = 0;

// The game's own update counter, read from outside the process with ReadProcessMemory so a capture or an
// input can be timed to a game update rather than the wall clock (see "at" in the script section). It is an
// address expression the game session supplies: "0x48b124" reads the int at that address, and "[0x579e30]+0x4c0"
// reads the pointer at 0x579e30 (a SexyAppBase game's app-pointer global) then the int 0x4c0 bytes into it (its
// mUpdateCount). gFlipsExpr, optional, is a counter the game bumps once per presented frame, so a shot can wait
// for the frame that showed update N. gUpdateMs is one update's length, used for a clip's interval_ms.
static char   gCounterExpr[256] = "";
static char   gFlipsExpr[256] = "";
// gPendingExpr and gStateExpr, optional and used together: a byte the game sets when an update is done and clears
// once that update is drawn (SexyAppBase's mHasPendingDraw), and the int of its update loop's step (mUpdateAppState:
// 0 messages, 1 and 2 an update, 3 done), so a shot inside Windows Sandbox knows which update its frame shows
// (capture_held).
static char   gPendingExpr[256] = "";
static char   gStateExpr[256] = "";
// gEmptyExpr, optional with the two above: a byte the game sets when DrawDirtyStuff skipped its draw without
// presenting (SexyAppBase's mLastDrawWasEmpty: nothing was dirty, or the draw throttle that runs while loading), so
// the frame on screen is an older one and the shot says "or an earlier one".
static char   gEmptyExpr[256] = "";
// --watch <expr> (up to 4): values of the game's memory logged at each change with the update counter, polled every
// millisecond (watch_thread), to measure how many updates something takes in the original (a loading screen, say).
static char   gWatchExpr[4][256];
static int    gWatchN = 0;
static int    gUpdateMs = 10;        // SexyAppBase runs 100 updates/s

// --3d on|off: the game's own Hardware Acceleration setting for the run, and the check that the game really ran in
// that mode. gMode3dExpr is where the game keeps the mode it ended up in (an address expression like the counter's,
// read as one byte: a SexyAppBase game's mDDInterface->mIs3D); games.tsv gives it per game. gMode3dWant is -1
// without --3d. Readings stop counting at "quit" (gMode3dFrozen), so a value read while the game tears its
// DDInterface down cannot decide the verdict.
static char   gMode3dExpr[256] = "";
static int    gMode3dWant = -1;
static volatile LONG gMode3dFrozen = 0;
static volatile LONG gMode3dLast = -1;     // the last mode read before quit: 1 on, 0 off, -1 never read
static volatile LONG gMode3dFlips = 0;     // how often it changed after the first reading
// --stage-root, if given: with <repo>\build, the only places origrun ever deletes a file (path_deletable).
static char   gStageRoot[MAX_PATH] = "";
// Set only by detect_windows_sandbox(), from what the machine itself reports: no option or environment variable sets
// it. Inside Windows Sandbox a capture can see nothing but the sandbox's own display, so shots are taken there and
// nowhere else (capture_refused).
static int    gInSandbox = 0;
static int    gAsSystem = 0;   // inside the sandbox, origrun runs as SYSTEM (wsb exec -r System)
static char   gDeskName[64] = "bejeweled-test";   // rundesk's hidden desktop; "Default" inside the sandbox

// Is this pid one of our copied game's processes? (image path under gGameDir)
static int is_our_pid(DWORD pid)
{
	if (pid == 0 || pid == GetCurrentProcessId()) return 0;
	HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!p) return 0;
	char path[MAX_PATH]; DWORD n = MAX_PATH;
	int ok = QueryFullProcessImageNameA(p, 0, path, &n);
	CloseHandle(p);
	if (!ok) return 0;
	size_t gl = strlen(gGameDir);
	return _strnicmp(path, gGameDir, gl) == 0;
}

static int file_exists(const char* p) { DWORD a=GetFileAttributesA(p); return a!=INVALID_FILE_ATTRIBUTES && !(a&FILE_ATTRIBUTE_DIRECTORY); }

static unsigned char* read_file(const char* path, size_t* outLen)
{
	FILE* f=fopen(path,"rb"); if(!f) return NULL;
	fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
	if (n<0){ fclose(f); return NULL; }
	unsigned char* b=(unsigned char*)malloc((size_t)n+1);
	if (b && fread(b,1,(size_t)n,f)!=(size_t)n){ free(b); b=NULL; }
	fclose(f);
	if (b) *outLen=(size_t)n;
	return b;
}

// ------------------------------------------------------------------------------------- the copy's patch list
//
// A per-game list applies static byte patches to the STAGED COPY's files (never the install) before launch:
// the same kind of change as the activation patches the user already expects on an unactivated copy, done to a
// file before it runs, not injected into a running process. origrun uses it for two things:
//   - pinning the original's RNG seed, so a run gives the same boards and other random content as the port it
//     is compared with (the {seed} replacement below carries --seed into the bytes the game would seed from);
//   - and any other patch a game session finds it needs for a clean comparison (for instance one that lets the
//     copy skip its trial prompt). origrun does not know or generate any of those bytes: a game session that
//     knows its own code supplies the exact expected and replacement bytes in the list.
//
// Each line:  <kind> <file> <where> <expected> <replacement> <reason...>
//   kind         a free word for the log (seed, test, ...)
//   file         a path under the copy's folder, e.g. TipTop.exe
//   where        off:0xN (a file offset) | rva:0xN | va:0xN (converted through the file's PE section table)
//   expected     the bytes that must be there now, hex ('.'/'_' may group them), e.g. a1.00.00
//   replacement  the same count of bytes; the token {seed} stands for --seed as 4 bytes, little-endian
// Lines beginning with # are comments. Every entry is checked against the file's current bytes first; one
// mismatch or malformed line refuses the WHOLE list and the run stops, so a patch is never half-applied. An
// entry whose replacement needs {seed} is skipped when no --seed was given. The patch list lives in a
// git-ignored place beside games.tsv (tools/origrun/patches/<game>.txt) because a trial-skip entry is a
// protection detail; see docs/ORIGRUN.md.
//
// Each patched file first keeps its bytes in <file>.origrun-unpatched, and every run restores those before
// (re-)applying, so a list is always checked against the original bytes and a run with no list runs unpatched.

#define MAX_PATCH 256
typedef struct { char kind[24], file[MAX_PATH], where[64], reason[256];
	unsigned char exp[256], rep[256]; int n, usesSeed, line; long off; } Patch;

static int hexval(char c){ if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return c-'a'+10; if(c>='A'&&c<='F')return c-'A'+10; return -1; }

// Parse a hex token (optional '.'/'_' grouping, and {seed} when allowed) into bytes; -1 on error.
static int hexbytes(const char* s, unsigned char* out, int cap, int allowSeed, uint32_t seed, int* usesSeed)
{
	int n=0;
	while (*s) {
		if (allowSeed && _strnicmp(s,"{seed}",6)==0) { if(n+4>cap) return -1; for(int k=0;k<4;k++)out[n++]=(unsigned char)(seed>>(8*k)); if(usesSeed)*usesSeed=1; s+=6; continue; }
		if (*s=='.'||*s=='_'){ s++; continue; }
		int hi=hexval(s[0]), lo=hi<0?-1:hexval(s[1]);
		if (hi<0||lo<0||n>=cap) return -1;
		out[n++]=(unsigned char)(hi*16+lo); s+=2;
	}
	return n;
}
static void hexstr(const unsigned char* b, int n, char* out, int cap){ int o=0; for(int i=0;i<n&&o+3<cap;i++)o+=wsprintfA(out+o,"%02X",b[i]); out[o]=0; }

// Convert an RVA or VA to a file offset through the PE section table; -1 if not backed by file bytes.
static long pe_to_off(const unsigned char* b, size_t n, int isVa, uint32_t addr)
{
	if (n<0x40 || b[0]!='M' || b[1]!='Z') return -1;
	uint32_t pe; memcpy(&pe,b+0x3c,4);
	if ((size_t)pe+24+96>n || memcmp(b+pe,"PE\0\0",4)!=0) return -1;
	uint16_t nsec,optsz,magic; memcpy(&nsec,b+pe+6,2); memcpy(&optsz,b+pe+20,2);
	const unsigned char* opt=b+pe+24; memcpy(&magic,opt,2);
	uint32_t base=0,hdrs=0;
	if (magic==0x10b) memcpy(&base,opt+28,4); else { uint64_t b64; memcpy(&b64,opt+24,8); base=(uint32_t)b64; }
	memcpy(&hdrs,opt+60,4);
	if (isVa && addr<base) return -1;
	uint32_t rva = isVa ? addr-base : addr;
	if (rva<hdrs) return (long)rva;
	const unsigned char* sec=opt+optsz;
	for (int i=0;i<nsec;i++,sec+=40) {
		if (sec+40>b+n) return -1;
		uint32_t vs,va,rs,rp; memcpy(&vs,sec+8,4); memcpy(&va,sec+12,4); memcpy(&rs,sec+16,4); memcpy(&rp,sec+20,4);
		uint32_t span = vs>rs?vs:rs;
		if (rva>=va && rva<va+span) return (rva-va<rs) ? (long)(rp+(rva-va)) : -1;
	}
	return -1;
}
static long resolve_where(const char* where, const unsigned char* b, size_t n)
{
	const char* c=strchr(where,':'); if(!c) return -1;
	uint32_t v=(uint32_t)strtoul(c+1,NULL,16);
	if (_strnicmp(where,"off:",4)==0) return (long)v;
	if (_strnicmp(where,"rva:",4)==0) return pe_to_off(b,n,0,v);
	if (_strnicmp(where,"va:",3)==0)  return pe_to_off(b,n,1,v);
	return -1;
}

static void restore_one(const char* path)
{
	char bak[MAX_PATH]; wsprintfA(bak,"%s.origrun-unpatched",path);
	if (!file_exists(bak)) return;
	if (CopyFileA(bak,path,FALSE)) logline("restored the unpatched %s",path);
	else logline("warning: could not restore %s (err %lu)",path,GetLastError());
}
// Put back every file a previous run patched (each keeps its original bytes beside it).
static void restore_unpatched(void)
{
	char pat[MAX_PATH]; joinp(pat,gGameDir,"*.origrun-unpatched");
	WIN32_FIND_DATAA fd; HANDLE h=FindFirstFileA(pat,&fd);
	if (h==INVALID_HANDLE_VALUE) return;
	do {
		char orig[MAX_PATH]; joinp(orig,gGameDir,fd.cFileName);
		orig[strlen(orig)-(int)strlen(".origrun-unpatched")]=0;
		restore_one(orig);
	} while (FindNextFileA(h,&fd));
	FindClose(h);
}

// Load, check and apply the patch list. Returns 1 on success (or when there is nothing to do), 0 when the list
// was refused -- the caller then stops the run, so a wrong list can never be run.
static int apply_patches(const char* listPath, int haveSeed, uint32_t seed)
{
	if (!file_exists(listPath)) { logline("no patch list (%s): the copy runs unpatched", listPath); return 1; }
	FILE* f=fopen(listPath,"r"); if(!f){ logline("cannot open patch list %s",listPath); return 0; }
	static Patch ps[MAX_PATCH]; int np=0, lineno=0; char line[1024]; int bad=0;
	while (fgets(line,sizeof(line),f)) {
		lineno++;
		char* nl=strpbrk(line,"\r\n"); if(nl)*nl=0;
		char* p=line; while(*p==' '||*p=='\t')p++;
		if (*p==0||*p=='#') continue;
		if (np>=MAX_PATCH){ logline("patch list: too many entries (max %d)",MAX_PATCH); bad=1; break; }
		Patch* q=&ps[np]; ZeroMemory(q,sizeof(*q)); q->line=lineno;
		char expTok[600]="", repTok[600]="";
		int got=sscanf(p,"%23s %259s %63s %599s %599s %255[^\n]", q->kind, q->file, q->where, expTok, repTok, q->reason);
		if (got<5){ logline("patch list line %d: need kind file where expected replacement",lineno); bad=1; continue; }
		int ue=0; int ne=hexbytes(expTok,q->exp,256,0,0,NULL);
		int nr=hexbytes(repTok,q->rep,256,1,seed,&ue);
		if (ne<0||nr<0){ logline("patch list line %d: bad hex",lineno); bad=1; continue; }
		if (ne!=nr){ logline("patch list line %d: expected %d bytes but replacement is %d",lineno,ne,nr); bad=1; continue; }
		q->n=ne; q->usesSeed=ue;
		if (ue && !haveSeed){ logline("patch (%s, line %d) needs a seed; --seed not given, so it is skipped",q->kind,lineno); continue; }
		np++;
	}
	fclose(f);
	if (bad) { logline("patch list refused (see the lines above); the run stops"); return 0; }
	if (np==0) { logline("patch list has no applicable entries; the copy runs unpatched"); return 1; }

	// Pass 1: read each named file, back it up if needed, and check every expected block. Nothing is written.
	for (int i=0;i<np;i++) {
		Patch* q=&ps[i];
		char full[MAX_PATH]; joinp(full,gGameDir,q->file);
		size_t len=0; unsigned char* b=read_file(full,&len);
		if (!b){ logline("patch (%s, line %d): cannot read %s",q->kind,q->line,full); return 0; }
		long off=resolve_where(q->where,b,len);
		if (off<0 || (size_t)off+q->n>len){ logline("patch (%s, line %d): %s is not a file location in %s",q->kind,q->line,q->where,q->file); free(b); return 0; }
		q->off=off;
		if (memcmp(b+off,q->exp,q->n)!=0) {
			char want[600],have[600]; hexstr(q->exp,q->n,want,sizeof(want)); hexstr(b+off,q->n,have,sizeof(have));
			logline("patch (%s, line %d): %s@%s has %s, expected %s -- list refused, run stops",q->kind,q->line,q->file,q->where,have,want);
			free(b); return 0;
		}
		free(b);
	}
	// Pass 2: back up and write. Everything was checked, so this does not half-apply a list.
	for (int i=0;i<np;i++) {
		Patch* q=&ps[i];
		char full[MAX_PATH]; joinp(full,gGameDir,q->file);
		char bak[MAX_PATH]; wsprintfA(bak,"%s.origrun-unpatched",full);
		if (!file_exists(bak) && !CopyFileA(full,bak,TRUE)) { logline("patch (%s): cannot back up %s (err %lu)",q->kind,full,GetLastError()); return 0; }
		HANDLE h=CreateFileA(full,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		if (h==INVALID_HANDLE_VALUE){ logline("patch (%s): cannot open %s to write (err %lu)",q->kind,full,GetLastError()); return 0; }
		SetFilePointer(h,q->off,NULL,FILE_BEGIN); DWORD wrote=0; WriteFile(h,q->rep,q->n,&wrote,NULL); CloseHandle(h);
		char want[600],have[600]; hexstr(q->exp,q->n,have,sizeof(have)); hexstr(q->rep,q->n,want,sizeof(want));
		logline("patched %s@%s: %s -> %s  (%s: %s)",q->file,q->where,have,want,q->kind,q->reason[0]?q->reason:"(no reason given)");
	}
	logline("applied %d patch(es) to the copy", np);
	return 1;
}

// --------------------------------------------------------------------------------- the update counter (RPM)
//
// Read the game's update counter from outside the process. The expression is "0xADDR" (the int at ADDR) or
// "[0xPTR]+0xOFF" (the int OFF bytes past the pointer stored at PTR). ReadProcessMemory and the thread-suspend
// used by "at" are documented debugging APIs, used only on the process origrun itself launched; nothing is
// injected into the game.
static HANDLE gGameProc = NULL;      // PROCESS_VM_READ handle, opened lazily for the live window's pid
static DWORD  gGameProcPid = 0;
static HWND resolve_window(DWORD pid, DWORD* outPid);

static int rpm_u32(DWORD_PTR addr, uint32_t* out)
{
	// Resolve the pid of the current top-level window each time: a SexyApp game can relaunch itself at start
	// (a display-mode switch), so the first window's pid goes stale and the counter lives in the new process.
	DWORD pid=0; resolve_window(0,&pid); if (!pid) pid=gGamePidHint;
	if (pid && pid!=gGameProcPid) {
		if (gGameProc) { CloseHandle(gGameProc); gGameProc=NULL; }
		gGameProc=OpenProcess(PROCESS_VM_READ,FALSE,pid); gGameProcPid=pid;
		if (gGameProc) {
			unsigned char mz[2]={0,0}; SIZE_T g=0; BOOL ok=ReadProcessMemory(gGameProc,(LPCVOID)0x400000,mz,2,&g);
			logline("reading the game's memory (pid %lu); image base reads %s (%02X %02X)", pid, (ok&&g==2)?"ok":"FAILED", mz[0], mz[1]);
		} else logline("OpenProcess(PROCESS_VM_READ, pid %lu) failed (err %lu)", pid, GetLastError());
	}
	if (!gGameProc) return 0;
	SIZE_T got=0;
	if (ReadProcessMemory(gGameProc,(LPCVOID)addr,out,4,&got) && got==4) return 1;
	DWORD e=GetLastError();
	static int warned=0; if (!warned) { warned=1; logline("ReadProcessMemory @0x%08lX failed (err %lu) -- check the counter address", (unsigned long)addr, e); }
	if (e!=ERROR_PARTIAL_COPY) { CloseHandle(gGameProc); gGameProc=NULL; gGameProcPid=0; }   // real loss of the process
	return 0;
}
// An address expression: a hex number, or "[e]" (the pointer stored at address e), followed by any number of
// "+0xOFF"/"-0xOFF". Brackets nest, so "[[0x8259e4]+0x36c]+0x38" follows two pointers. A null pointer fails.
static int eval_addr(const char** ps, uint32_t* out)
{
	const char* s=*ps; while(*s==' ')s++;
	uint32_t v;
	if (*s=='[') {
		s++; uint32_t at; if (!eval_addr(&s,&at)) return 0;
		while(*s==' ')s++; if (*s!=']') return 0; s++;
		if (!rpm_u32(at,&v) || !v) return 0;
	} else {
		char* e; v=(uint32_t)strtoul(s,&e,16); if (e==s) return 0; s=e;
	}
	for (;;) {
		while(*s==' ')s++;
		if (*s!='+' && *s!='-') break;
		int neg = *s=='-'; s++; while(*s==' ')s++;
		char* e; uint32_t o=(uint32_t)strtoul(s,&e,16); if (e==s) return 0; s=e;
		v = neg ? v-o : v+o;
	}
	*ps=s; *out=v; return 1;
}
// Read the int at an address expression. The lock keeps the counter reads of the driver thread and the 3D-mode
// poll from racing on rpm_u32's shared process handle.
static CRITICAL_SECTION gRpmLock;
static int read_counter(const char* expr, uint32_t* out)
{
	if (!expr || !*expr) return 0;   // rpm_u32 opens the process handle lazily on first use
	EnterCriticalSection(&gRpmLock);
	const char* s=expr; uint32_t addr=0;
	int ok = eval_addr(&s,&addr) && rpm_u32((DWORD_PTR)addr,out);
	LeaveCriticalSection(&gRpmLock);
	return ok;
}

// Whether the game, as read now, shows the update its counter holds: nothing waits to be drawn and no update runs.
static int drawn_now(int* readFail)
{
	uint32_t pd=0, st=0;
	if (!read_counter(gPendingExpr,&pd) || !read_counter(gStateExpr,&st)) { *readFail=1; return 0; }
	return !(pd&0xff) && (st==0 || st==3);
}
// Read while held, once drawn_now: whether that last draw was skipped (1), presented (0), or cannot be told (-1).
static int draw_was_empty(void)
{
	uint32_t e=0;
	if (!gEmptyExpr[0] || !read_counter(gEmptyExpr,&e)) return -1;
	return (e&0xff) ? 1 : 0;
}

// The thread that owns the game's main window, for SuspendThread/ResumeThread around an "at" capture/input.
static HANDLE open_window_thread(HWND hwnd, DWORD access)
{
	if (!hwnd) return NULL;
	DWORD tid=GetWindowThreadProcessId(hwnd,NULL); if(!tid) return NULL;
	return OpenThread(access,FALSE,tid);
}

// --------------------------------------------------------------------------------------- PNG (truecolor)

static uint32_t crc_table[256];
static void crc_init(void){ for(uint32_t n=0;n<256;n++){uint32_t c=n;for(int k=0;k<8;k++)c=(c&1)?0xedb88320u^(c>>1):(c>>1);crc_table[n]=c;} }
static uint32_t crc_upd(uint32_t c,const unsigned char*b,size_t n){for(size_t i=0;i<n;i++)c=crc_table[(c^b[i])&0xff]^(c>>8);return c;}

static void put32(FILE* f, uint32_t v){unsigned char b[4]={(unsigned char)(v>>24),(unsigned char)(v>>16),(unsigned char)(v>>8),(unsigned char)v};fwrite(b,1,4,f);}
static void chunk(FILE* f, const char* type, const unsigned char* data, uint32_t len)
{
	put32(f, len);
	uint32_t c = crc_upd(0xffffffffu,(const unsigned char*)type,4);
	if (len) c = crc_upd(c, data, len);
	fwrite(type,1,4,f); if(len) fwrite(data,1,len,f);
	put32(f, c ^ 0xffffffffu);
}

// Write RGB (3 bytes/px, top-down) as a PNG using stored (uncompressed) DEFLATE blocks.
static int png_write(const char* path, const unsigned char* rgb, int w, int h)
{
	FILE* f = fopen(path, "wb"); if (!f) return 0;
	static const unsigned char sig[8]={137,80,78,71,13,10,26,10}; fwrite(sig,1,8,f);
	unsigned char ihdr[13];
	ihdr[0]=(unsigned char)(w>>24);ihdr[1]=(unsigned char)(w>>16);ihdr[2]=(unsigned char)(w>>8);ihdr[3]=(unsigned char)w;
	ihdr[4]=(unsigned char)(h>>24);ihdr[5]=(unsigned char)(h>>16);ihdr[6]=(unsigned char)(h>>8);ihdr[7]=(unsigned char)h;
	ihdr[8]=8; ihdr[9]=2; ihdr[10]=0; ihdr[11]=0; ihdr[12]=0;   // 8-bit, colour type 2 (truecolor)
	chunk(f,"IHDR",ihdr,13);

	// Raw image data: one filter byte (0) per scanline + RGB bytes.
	size_t raw_len = (size_t)(w*3+1)*h;
	unsigned char* raw = (unsigned char*)malloc(raw_len);
	if (!raw) { fclose(f); return 0; }
	size_t o=0;
	for (int y=0;y<h;y++){ raw[o++]=0; memcpy(raw+o, rgb+(size_t)y*w*3, (size_t)w*3); o+=(size_t)w*3; }

	// zlib stream: 2-byte header, stored DEFLATE blocks, Adler-32.
	// 2-byte header, a 5-byte header per stored block of up to 65535 bytes, the data, a 4-byte Adler-32. (It once
	// reserved one byte per block: a 818x647 window ran 43 bytes past the end and crashed origrun mid-shot.)
	size_t zcap = 2 + 5*(raw_len/65535 + 1) + raw_len + 4;
	unsigned char* z = (unsigned char*)malloc(zcap);
	if (!z) { free(raw); fclose(f); return 0; }
	size_t zo=0; z[zo++]=0x78; z[zo++]=0x01;
	size_t p=0;
	while (p < raw_len) {
		size_t blk = raw_len-p; if (blk>65535) blk=65535;
		int last = (p+blk>=raw_len);
		z[zo++]=(unsigned char)(last?1:0);
		z[zo++]=(unsigned char)(blk&0xff); z[zo++]=(unsigned char)((blk>>8)&0xff);
		z[zo++]=(unsigned char)(~blk&0xff); z[zo++]=(unsigned char)((~blk>>8)&0xff);
		memcpy(z+zo, raw+p, blk); zo+=blk; p+=blk;
	}
	uint32_t a1=1,a2=0;
	for (size_t i=0;i<raw_len;i++){ a1=(a1+raw[i])%65521; a2=(a2+a1)%65521; }
	uint32_t adler=(a2<<16)|a1;
	z[zo++]=(unsigned char)(adler>>24); z[zo++]=(unsigned char)(adler>>16); z[zo++]=(unsigned char)(adler>>8); z[zo++]=(unsigned char)adler;

	chunk(f,"IDAT",z,(uint32_t)zo);
	chunk(f,"IEND",NULL,0);
	fclose(f); free(raw); free(z);
	return 1;
}

// ------------------------------------------------------------------------------------- window + capture

// The game recreates its top-level window during start-up (a mode switch), so a handle grabbed once goes
// stale. We track the game by pid and re-resolve its current main window before every action.
typedef struct { DWORD pidWanted; HWND found; DWORD foundPid; long bestArea; } FindCtx;
static BOOL CALLBACK enum_cb(HWND hwnd, LPARAM lp)
{
	FindCtx* c = (FindCtx*)lp;
	DWORD pid=0; GetWindowThreadProcessId(hwnd, &pid);
	if (c->pidWanted) { if (pid != c->pidWanted) return TRUE; }
	else if (!is_our_pid(pid)) return TRUE;
	if (!IsWindowVisible(hwnd)) return TRUE;
	if (GetWindow(hwnd, GW_OWNER) != NULL) return TRUE;   // top-level only
	RECT r; GetWindowRect(hwnd, &r);
	long area = (long)(r.right-r.left) * (r.bottom-r.top);
	if ((r.right-r.left) < 64 || (r.bottom-r.top) < 64) return TRUE;
	if (area > c->bestArea) { c->bestArea = area; c->found = hwnd; c->foundPid = pid; }
	return TRUE;   // keep scanning for the largest
}

// Resolve the current main window, either of a known pid or of any of our processes. Returns the window and,
// via outPid, its pid.
// The driver thread attaches to the game's desktop so the HWNDs it enumerates are valid for GetWindowRect,
// PrintWindow and PostMessage (window handles belong to a desktop; a thread on another desktop cannot use
// them). gDesk is that attached desktop.
static HDESK gDesk;
static int attach_desktop(void)
{
	// rundesk creates the desktop a moment after we launch it, so poll for it (up to ~15s).
	for (int waited=0; waited<15000 && !gStop; waited+=200) {
		gDesk = OpenDesktopA(gDeskName, 0, FALSE,
			DESKTOP_ENUMERATE|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS|DESKTOP_CREATEWINDOW|DESKTOP_HOOKCONTROL|STANDARD_RIGHTS_REQUIRED);
		if (!gDesk) gDesk = OpenDesktopA(gDeskName, 0, FALSE, DESKTOP_ENUMERATE|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS);
		if (gDesk) break;
		Sleep(200);
	}
	if (!gDesk) { logline("could not open the game desktop (err %lu)",GetLastError()); return 0; }
	if (!SetThreadDesktop(gDesk)) { logline("SetThreadDesktop failed (err %lu)",GetLastError()); return 0; }
	return 1;
}

static HWND resolve_window(DWORD pid, DWORD* outPid)
{
	HDESK d = gDesk ? gDesk : OpenDesktopA(gDeskName, 0, FALSE, DESKTOP_ENUMERATE|DESKTOP_READOBJECTS);
	if (!d) return NULL;
	FindCtx c; ZeroMemory(&c,sizeof(c)); c.pidWanted=pid;
	EnumDesktopWindows(d, enum_cb, (LPARAM)&c);
	if (d != gDesk) CloseDesktop(d);
	if (c.found && outPid) *outPid = c.foundPid;
	return c.found;
}

static BOOL CALLBACK dump_cb(HWND hwnd, LPARAM lp)
{
	DWORD pid=0; GetWindowThreadProcessId(hwnd,&pid);
	RECT r={0}; GetWindowRect(hwnd,&r);
	char cls[64]={0}, txt[96]={0}; GetClassNameA(hwnd,cls,64); GetWindowTextA(hwnd,txt,96);
	logline("  win hwnd=%p pid=%lu vis=%d owner=%p %dx%d ours=%d cls='%s' '%s'",
		(void*)hwnd,pid,IsWindowVisible(hwnd),(void*)GetWindow(hwnd,GW_OWNER),
		r.right-r.left,r.bottom-r.top,is_our_pid(pid),cls,txt);
	return TRUE;
}
static void dump_desktop(void)
{
	HDESK d=OpenDesktopA(gDeskName,0,FALSE,DESKTOP_ENUMERATE|DESKTOP_READOBJECTS);
	if(!d){ logline("  (cannot open desktop to dump)"); return; }
	logline("desktop windows:"); EnumDesktopWindows(d,dump_cb,0); CloseDesktop(d);
}

// Wait for the game's window to first appear; returns its pid (0 on timeout).
static DWORD find_game_pid(int timeout_ms)
{
	int waited=0;
	while (waited < timeout_ms && !gStop) {
		DWORD pid=0;
		if (resolve_window(0,&pid) && pid) return pid;
		Sleep(100); waited+=100;
	}
	return 0;
}

// 1 if every pixel of the region (BGRA, w pixels per row) has the same colour, or the region is empty.
static int region_flat(const unsigned char* px, int w, int x0, int y0, int x1, int y1)
{
	if (x1<=x0 || y1<=y0) return 1;
	const unsigned char* f = px + ((size_t)y0*w + x0)*4;
	for (int y=y0;y<y1;y++) {
		const unsigned char* p = px + ((size_t)y*w + x0)*4;
		for (int x=x0;x<x1;x++,p+=4)
			if (p[0]!=f[0] || p[1]!=f[1] || p[2]!=f[2]) return 0;
	}
	return 1;
}

// Which GPU APIs the game has loaded (DXGI, Direct3D 8 to 12, OpenGL or Vulkan), read from outside the game
// (EnumProcessModulesEx), for the log. Returns 1 and the modules' names when one is loaded, -1 when the list cannot
// be read, 0 when none is.
static int gCaptureRefused = 0;
static int is_gpu_module(const char* base)
{
	static const char* const kGpu[] = { "dxgi.dll", "d3d8.dll", "d3d9.dll", "d3d11.dll", "d3d12.dll", "opengl32.dll",
		"vulkan-1.dll" };
	for (size_t j=0; j<sizeof(kGpu)/sizeof(kGpu[0]); j++) if (lstrcmpiA(base,kGpu[j])==0) return 1;
	size_t n=strlen(base);   // d3d10*: d3d10.dll, d3d10_1.dll, d3d10core.dll, D3D10Warp.dll and the like
	return n>=9 && _strnicmp(base,"d3d10",5)==0 && lstrcmpiA(base+n-4,".dll")==0;
}
static int process_uses_gpu_api(DWORD pid, char* which, int whichLen)
{
	which[0]=0;
	HANDLE p = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);
	if (!p) return -1;
	static HMODULE mods[1024]; DWORD need=0; int found=-1;
	for (int tries=0; tries<5 && found<0; tries++) {   // the list can be mid-change while the game loads a DLL
		if (!EnumProcessModulesEx(p, mods, sizeof(mods), &need, LIST_MODULES_ALL)) { Sleep(10); continue; }
		found=0;
		DWORD n = need/sizeof(HMODULE); if (n > 1024) n = 1024;
		for (DWORD i=0; i<n; i++) {
			char base[MAX_PATH];
			if (!GetModuleBaseNameA(p, mods[i], base, sizeof(base)) || !is_gpu_module(base)) continue;
			if (found && lstrlenA(which)+2<whichLen) lstrcatA(which,", ");
			if (lstrlenA(which)+lstrlenA(base)<whichLen) lstrcatA(which,base);
			found=1;
		}
	}
	CloseHandle(p);
	return found;
}
// The rule every capture follows, before it reads the window: 1 (logged) when the shot is refused.
// On the host a capture can show the user's own screen: through the screen DC, the window DC and PrintWindow alike
// (three times by 2026-11-01). So origrun takes no shot of any kind there, and only inside Windows Sandbox, where
// the only display is the sandbox's own, does it capture at all.
static int capture_refused(HWND hwnd, const char* name)
{
	if (gInSandbox) return 0;
	if (!gCaptureRefused) logline("shot %s refused, and every later shot of this run: origrun takes no shot on the host, "
		"only inside Windows Sandbox (--sandbox)", name);
	else logline("shot %s refused (no shots on the host)", name);
	gCaptureRefused=1;
	return 1;
}

// allowPrint==0 skips PrintWindow (which sends WM_PRINT to the game's window thread): a capture taken while
// that thread is held by SuspendThread (the "at" path) must not message it, or it would deadlock. The window-DC
// blit reads the window's pixels without messaging the game, so it is safe while the thread is held.
// held: a game thread the caller suspended for this shot. It is resumed as soon as the pixels are in memory (or on
// any early return), so converting and writing the PNG does not stretch the hold: every millisecond the game is held
// its clock runs on, and on resume it catches up on updates it never draws.
static double qpc_ms(void){ LARGE_INTEGER c,f; QueryPerformanceCounter(&c); QueryPerformanceFrequency(&f); return (double)c.QuadPart*1000.0/(double)f.QuadPart; }
// How long the game was last held (from SuspendThread to ResumeThread), for the log: the game's clock runs on for
// all of it.
static double gHoldT0 = 0, gLastHeldMs = 0;
static uint32_t gHeldAt = 0;   // the update counter read while the last held shot was held
// The update whose frame the last "at"/update-timed shot shows (0: not known), and the file that lists it for each
// shot (shots/landed.tsv: name, update, how sure), so the port can take its own shot at that update. A run starts the
// file afresh (landed_reset).
static uint32_t gLandedAt = 0;
static void note_landed(const char* name, uint32_t upd, const char* how)
{
	gLandedAt=upd;
	char p[MAX_PATH]; wsprintfA(p,"%s\\landed.tsv",gShotDir);
	FILE* f=fopen(p,"a"); if (f){ fprintf(f,"%s\t%u\t%s\n",name,upd,how); fclose(f); }
}
static void landed_reset(void){ char p[MAX_PATH]; wsprintfA(p,"%s\\landed.tsv",gShotDir); DeleteFileA(p); }
// The game's window thread while origrun holds it, so that crash_filter can resume it before anything else.
static PVOID volatile gHeldThread = NULL;
// Returns 1 once the thread is really stopped. SuspendThread only asks for the suspension; GetThreadContext returns
// only after it has taken effect, so no frame or counter is read while the thread may still be running on.
static int hold_begin(HANDLE th)
{
	if (!th) return 0;
	gHoldT0=qpc_ms();
	if (SuspendThread(th)==(DWORD)-1) { logline("hold: SuspendThread failed (err %lu); not held", GetLastError()); return 0; }
	InterlockedExchangePointer(&gHeldThread, th);
	CONTEXT c; ZeroMemory(&c,sizeof(c)); c.ContextFlags=CONTEXT_CONTROL;
	if (!GetThreadContext(th,&c)) { static LONG warned=0; if (!InterlockedExchange(&warned,1)) logline("hold: GetThreadContext failed (err %lu), so the suspension may not have taken effect when the hold is used", GetLastError()); }
	return 1;
}
static void hold_end(HANDLE th)
{
	if (!th) return;
	if (InterlockedCompareExchangePointer(&gHeldThread, NULL, th) == th) ResumeThread(th);   // crash_filter may have resumed it already
	gLastHeldMs=qpc_ms()-gHoldT0;
}
static void release_held(HANDLE* held){ if (held && *held){ hold_end(*held); *held=NULL; } }

// The sandbox's shot (capture_held): PrintWindow on a helper thread, so that it can be asked while the game is held.
// One helper serves the whole run. It is started on the desktop of the thread that captures (a new thread starts on
// the process's first desktop, where PrintWindow returns TRUE with a black window), at raised priority so that it is
// in PrintWindow at once. gPwState hands a job over: the capturing thread posts it (PW_POSTED) and waits until the
// helper says it is entering PrintWindow (PW_IN). A job that is given up (PW_ABANDONED: the helper did not start, or
// PrintWindow did not return in time) is freed by the helper once it gets to it; until then the state is not
// PW_IDLE and every further shot is refused.
enum { PW_IDLE, PW_POSTED, PW_IN, PW_DONE, PW_ABANDONED };
static volatile LONG gPwState = PW_IDLE;
static HANDLE gPwThread, gPwGo, gPwDone, gPwReady;
static HWND gPwHwnd; static HDC gPwMem; static HBITMAP gPwDib; static HGDIOBJ gPwOld; static BOOL gPwOk;
static DWORD gPwDeskErr;   // SetThreadDesktop's error in the helper, 0 when it worked
static void pw_free_job(void){ SelectObject(gPwMem,gPwOld); DeleteObject(gPwDib); DeleteDC(gPwMem); gPwMem=NULL; }
static DWORD WINAPI pw_helper(LPVOID desk)
{
	if (!SetThreadDesktop((HDESK)desk)) { gPwDeskErr=GetLastError()?GetLastError():1; SetEvent(gPwReady); return 0; }
	SetEvent(gPwReady);
	for (;;) {
		WaitForSingleObject(gPwGo,INFINITE);
		if (InterlockedCompareExchange(&gPwState,PW_IN,PW_POSTED)!=PW_POSTED) { pw_free_job(); InterlockedExchange(&gPwState,PW_IDLE); continue; }
		gPwOk=PrintWindow(gPwHwnd,gPwMem,PW_RENDERFULLCONTENT);
		if (InterlockedCompareExchange(&gPwState,PW_DONE,PW_IN)!=PW_IN) { pw_free_job(); InterlockedExchange(&gPwState,PW_IDLE); continue; }
		SetEvent(gPwDone);
	}
}
// Starts the helper on first use. 0 (logged once) if it cannot be started on the capturing thread's desktop.
static int pw_ready(const char* name)
{
	static int failed=0;
	if (gPwThread) return 1;
	if (failed) { logline("shot %s refused: no PrintWindow helper (see above)", name); return 0; }
	gPwGo=CreateEventA(NULL,FALSE,FALSE,NULL); gPwDone=CreateEventA(NULL,FALSE,FALSE,NULL); gPwReady=CreateEventA(NULL,TRUE,FALSE,NULL);
	HANDLE t = (gPwGo && gPwDone && gPwReady) ? CreateThread(NULL,0,pw_helper,GetThreadDesktop(GetCurrentThreadId()),0,NULL) : NULL;
	DWORD err = t ? 0 : GetLastError();
	DWORD rw = t ? (SetThreadPriority(t,THREAD_PRIORITY_HIGHEST), WaitForSingleObject(gPwReady,2000)) : WAIT_FAILED;
	if (!t || rw!=WAIT_OBJECT_0 || gPwDeskErr) {   // a helper that is late is never used: no job is ever posted to it
		logline("shot %s refused, and every later update-timed shot: the PrintWindow helper %s (err %lu)", name,
			!t ? "could not be started" : rw!=WAIT_OBJECT_0 ? "did not start within 2 s" : "could not be put on origrun's desktop",
			!t ? err : gPwDeskErr);
		failed=1; if (t) CloseHandle(t); return 0;
	}
	gPwThread=t; return 1;
}
static uint32_t gBracketAt = 0;
static const char* gBracketSure = "";   // how sure that update is (landed.tsv's third column)   // the update a bracketed shot shows (capture_held sets it before the hold's capture)
// bracket (only with held, inside Windows Sandbox): the game is held; PrintWindow is posted to the helper, the game is
// released once the helper is inside PrintWindow, and its message loop answers it (see capture_held). Nothing else
// reads the window on this path. Returns 3 and writes nothing if PrintWindow returned FALSE, so the caller may try
// again; 0 for a refusal.
static int capture_ex2(HWND hwnd, const char* name, int allowPrint, HANDLE* held, int bracket)
{
	RECT r; if (!hwnd || !GetWindowRect(hwnd,&r)) { release_held(held); return 0; }   // window gone; caller may retry
	if (capture_refused(hwnd,name)) { release_held(held); return 0; }
	int w=r.right-r.left, h=r.bottom-r.top;
	if (w<=0||h<=0) { release_held(held); return 0; }
	HDC mem=CreateCompatibleDC(NULL);   // never a DC of the screen
	BITMAPINFO bi; ZeroMemory(&bi,sizeof(bi));
	bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth=w; bi.bmiHeader.biHeight=-h;   // top-down
	bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
	void* bits=NULL;
	HBITMAP dib=CreateDIBSection(mem,&bi,DIB_RGB_COLORS,&bits,NULL,0);
	if (!dib || !bits) {
		release_held(held);
		logline("shot %s: no bitmap for a %dx%d window (err %lu)", name, w, h, GetLastError());
		if (dib) DeleteObject(dib); DeleteDC(mem); return 0;
	}
	HGDIOBJ old=SelectObject(mem,dib);

	unsigned char* px=(unsigned char*)bits;

	// The client area in window coordinates: a game that draws with DirectDraw leaves PrintWindow's copy of its
	// client area a single flat colour (the class background), so test that region, not the title bar.
	POINT o={0,0}; ClientToScreen(hwnd,&o); RECT cr={0}; GetClientRect(hwnd,&cr);
	int cx0=o.x-r.left, cy0=o.y-r.top, cx1=cx0+cr.right, cy1=cy0+cr.bottom;
	if (cx0<0) cx0=0; if (cy0<0) cy0=0; if (cx1>w) cx1=w; if (cy1>h) cy1=h;

	// What reads the window: PrintWindow (GDI and DWM-redirected content), then a blit of the window's own DC, which
	// legacy DirectDraw windows need. Never the screen, and nothing at all outside Windows Sandbox: capture_refused,
	// asked once when the capture starts, depends only on where origrun runs, which no run changes.
	const char* how="printwindow";
	BOOL ok=FALSE;
	if (bracket && held && *held) {
		if (gPwState!=PW_IDLE) { release_held(held); SelectObject(mem,old); DeleteObject(dib); DeleteDC(mem); logline("shot %s refused: an earlier PrintWindow has not returned", name); return 0; }
		gPwHwnd=hwnd; gPwMem=mem; gPwDib=dib; gPwOld=old; gPwOk=FALSE;
		InterlockedExchange(&gPwState,PW_POSTED); SetEvent(gPwGo);
		double t0=qpc_ms();
		while (gPwState==PW_POSTED && qpc_ms()-t0<100) SwitchToThread();
		if (InterlockedCompareExchange(&gPwState,PW_ABANDONED,PW_POSTED)==PW_POSTED) {   // the helper frees the job
			release_held(held);
			logline("shot %s refused: the PrintWindow helper did not start within 100 ms", name);
			return 0;
		}
		// The helper is entering PrintWindow; a margin for it to send its message before the game runs on (Sleep would
		// round up to the timer's 15.6 ms tick). Not checked: a helper pre-empted here for longer than the margin and an
		// update gets its message answered a frame late, and the shot then shows N+1 (rare at its priority; an exact
		// release would wait until the helper is blocked in the send).
		{ double t1=qpc_ms(); while (qpc_ms()-t1<0.5) SwitchToThread(); }
		release_held(held);
		DWORD wr=WaitForSingleObject(gPwDone,3000);
		if (wr!=WAIT_OBJECT_0) {
			if (InterlockedCompareExchange(&gPwState,PW_ABANDONED,PW_IN)==PW_IN) {   // the helper frees the job
				logline("shot %s refused: PrintWindow did not return within 3 s of the release", name);
				return 0;
			}
			WaitForSingleObject(gPwDone,INFINITE);   // it returned just now: its event is set
		}
		uint32_t c1=0; read_counter(gCounterExpr,&c1);
		ok=gPwOk; InterlockedExchange(&gPwState,PW_IDLE);
		if (!ok) { SelectObject(mem,old); DeleteObject(dib); DeleteDC(mem); return 3; }
		how="printwindow-bracketed (sandbox)";
		logline("shot %s: printwindow-bracketed (sandbox): update %u (%s); the counter read %u once PrintWindow returned", name, gBracketAt, gBracketSure, c1);
	} else if (allowPrint) ok=PrintWindow(hwnd, mem, PW_RENDERFULLCONTENT);
	if (allowPrint && !ok) how="printwindow failed";
	if (!bracket && (!allowPrint || !ok || region_flat(px,w,cx0,cy0,cx1,cy1))) {
		HDC wdc=GetWindowDC(hwnd);
		if (wdc){ BitBlt(mem,0,0,w,h,wdc,0,0,SRCCOPY); ReleaseDC(hwnd,wdc); how="windowdc"; }
	}
	release_held(held);   // the pixels are in memory: the game may run on
	int flat = region_flat(px,w,cx0,cy0,cx1,cy1);
	if (flat) how="flat (DirectDraw/Direct3D content not reachable by GDI)";

	unsigned char* rgb=(unsigned char*)malloc((size_t)w*h*3);
	if (!rgb) { logline("shot %s: out of memory for %dx%d", name, w, h); SelectObject(mem,old); DeleteObject(dib); DeleteDC(mem); return 0; }
	for (int i=0;i<w*h;i++){ rgb[i*3+0]=px[i*4+2]; rgb[i*3+1]=px[i*4+1]; rgb[i*3+2]=px[i*4+0]; }
	char path[MAX_PATH]; wsprintfA(path,"%s\\%s.png",gShotDir,name);
	int wrote=png_write(path,rgb,w,h);
	free(rgb);
	SelectObject(mem,old); DeleteObject(dib); DeleteDC(mem);
	logline("shot %s -> %s (%dx%d) [%s]", name, path, w, h, how);
	return wrote ? (flat ? 2 : 1) : 0;   // 2 = written but flat (a DirectDraw screen, or not drawn yet)
}
static int capture_ex(HWND hwnd, const char* name, int allowPrint){ return capture_ex2(hwnd,name,allowPrint,NULL,0); }
static int capture(HWND hwnd, const char* name){ return capture_ex(hwnd,name,1); }

// Poll up to timeout_ms for any visible game window, then capture it. If the grab is flat (often a window that
// has not drawn its first frame yet -- several DirectDraw games are slow to init, then DO capture with
// PrintWindow), keep re-grabbing until content appears or the timeout, and keep the last flat grab otherwise.
// Returns 1 if a non-flat shot was written, 2 if only a flat one, 0 if no window.
static int capture_ready(const char* name, int timeout_ms)
{
	int waited=0, last=0;
	for (;;) {
		HWND hwnd = resolve_window(0,NULL);
		if (hwnd) { last=capture(hwnd, name); if (last==1) return 1; }   // non-flat: done
		if (gCaptureRefused) return 0;
		if (waited >= timeout_ms || gStop) {
			if (last==2) logline("shot %s: content stayed flat for %dms (DirectDraw not reachable by GDI, or a solid screen)",name,timeout_ms);
			else if (!last) logline("shot %s skipped (no window within %dms)",name,timeout_ms);
			return last;
		}
		Sleep(200); waited+=200;
	}
}

// ---------------------------------------------------------------------------------- bursts, clips, "at N"


static void write_clip_json(const char* dir, int interval_ms)
{
	char p[MAX_PATH]; wsprintfA(p,"%s\\clip.json",dir);
	FILE* f=fopen(p,"w"); if(f){ fprintf(f,"{\"interval_ms\": %d}\n",interval_ms); fclose(f); }
}

// Wait until the game's update counter reaches target (or timeout). Returns the value last read, or -1 if the
// counter could not be read at all. Polls at ~1 ms, so with a 100 Hz update (10 ms each) it lands within one
// update of the target.
static int wait_counter(uint32_t target, int timeout_ms)
{
	int w=0; uint32_t v=0; int everRead=0;
	while (!gStop) {
		if (read_counter(gCounterExpr,&v)) { everRead=1; if (v>=target) return (int)v; }
		else if (!everRead && w>=1500) { logline("counter could not be read (bad address, or the process is gone); giving up on 'at'/update timing"); return -1; }
		if (w>=timeout_ms) { logline("counter %u not reached within %dms (last %u)",target,timeout_ms,everRead?v:0); return everRead?(int)v:-1; }
		Sleep(1); w++;
	}
	return everRead?(int)v:-1;
}

// Capture the game while its window thread is held, so the frame cannot change under the grab. If gFlipsExpr is
// set, first let the game present the frame that reflects the current update (wait for one more flip), so the
// shot is update N rather than possibly N-1.
// Outside Windows Sandbox the shot is refused before the game is ever held (capture_refused).
static int capture_held(const char* name)
{
	gLastHeldMs=0; gHeldAt=0; gLandedAt=0;   // a shot that is not held, or not taken, reports no hold of an earlier one
	HWND hw=resolve_window(0,NULL); if(!hw){ logline("at: no window for '%s'",name); return 0; }
	if (capture_refused(hw,name)) return 0;
	// Inside Windows Sandbox nothing reads a held game: BitBlt from the window DC and from the screen DC return TRUE
	// but copy no pixels, held or not; DXGI finds no output to duplicate; and PrintWindow, the one thing that sees the
	// game there, needs the game's window thread, which answers it from its message loop (measured on Bookworm
	// Adventures, 2026-11-01). Reading the counter just before and just after an unheld PrintWindow and keeping the
	// shot when both agree does not work there: PrintWindow takes about an update's time, the reads agreed once in
	// 1 to 20 tries, and shots landed up to 238 updates late. So the game is held, PrintWindow is posted to the helper,
	// and the game is released once the helper is in PrintWindow; the game answers it at its next message check.
	// Which frame that shows: SexyAppBase checks messages after an update and its PROCESS_2 step but draws the update
	// only in a later Process call, so a hold inside update N, its PROCESS_2 or the message check after it gets the
	// message answered before frame N is drawn (and a draw it skips to catch up leaves an older frame). With
	// --pending-draw and --update-state (mHasPendingDraw, mUpdateAppState) the hold is kept only when no update waits
	// to be drawn and none is running (the step is "messages" or "done"; a draw and the catch-up updates run in "done"
	// with the flag still set). The last draw then followed update N, or found nothing changed, and the game's next
	// message check comes before its next update, so the shot shows update N. Without them the shot shows N or an
	// earlier update, and says so. The update is logged and listed in landed.tsv.
	if (gInSandbox && gCounterExpr[0]) {
		if (!pw_ready(name)) return 0;
		double t0=qpc_ms(); int holds=0;
		int exact = gPendingExpr[0] && gStateExpr[0];
		for (int fails=0; fails<3; ) {
			if (gStop) { logline("shot %s not taken: the run is stopping", name); return 0; }
			// unheld, wait until the game shows its last update; then hold and look again
			int rf=0;
			while (exact && !drawn_now(&rf) && !rf && qpc_ms()-t0<500 && !gStop) SwitchToThread();
			if (exact && !rf && qpc_ms()-t0>=500) { logline("shot %s refused: the game did not show a drawn update for 500 ms (%d holds)", name, holds); return 0; }
			HANDLE th=open_window_thread(hw,THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT);
			if (th && !hold_begin(th)) { CloseHandle(th); th=NULL; }
			if (!th) { logline("shot %s refused: the game's window thread could not be held", name); return 0; }
			holds++;
			HANDLE held=th;
			uint32_t c=0; int ok=read_counter(gCounterExpr,&c), shown=1;
			if (ok && exact) { rf=0; shown=drawn_now(&rf); ok=!rf; }
			if (!ok) {
				release_held(&held); CloseHandle(th);
				logline("shot %s refused: the update counter%s could not be read while the game was held", name, exact?" or the pending-draw flag or update step":"");
				return 0;
			}
			if (!shown) { release_held(&held); CloseHandle(th); continue; }   // the game moved on in between: look again
			int empty = exact ? draw_was_empty() : -1;
			const char* sure = !exact ? "or-earlier" : empty==1 ? "or-earlier" : empty==0 ? "drawn" : "drawn-unless-skipped";
			gBracketSure=sure;
			gBracketAt=c;
			int r=capture_ex2(hw,name,1,&held,1);
			release_held(&held); CloseHandle(th);
			if (r!=3) { if (r) note_landed(name,c,sure); return r; }
			fails++;
		}
		logline("shot %s refused: PrintWindow failed three times", name);
		return 0;
	}
	if (gFlipsExpr[0]) { uint32_t f0; if (read_counter(gFlipsExpr,&f0)) { uint32_t f=f0; int w=0; while(f<=f0 && w<100 && !gStop){ Sleep(1); read_counter(gFlipsExpr,&f); w++; } } }  // a static screen presents nothing new, so cap the wait
	// Without a flips counter, hold at the start of the next update: the counter goes to N at the start of update N,
	// and the game draws after the update, so a hold taken as soon as the counter reads N races that draw and mostly
	// catches update N-1's frame (measured on Bookworm Adventures: one update behind the port, jittering by one). Once
	// the counter reads N+1, update N's frame is on screen (unless the game skipped that draw to catch up).
	if (!gFlipsExpr[0] && gCounterExpr[0]) { uint32_t c; if (read_counter(gCounterExpr,&c)) wait_counter(c+1, 200); }
	// The counter race: the game can begin another update between the counter reaching N+1 above and the hold taking
	// effect, so the frame held may be a later update's than the one waited for. That is harmless: gHeldAt is read
	// once the thread has really stopped (hold_begin's GetThreadContext), so it names the update actually held, the
	// counter is one aligned 32-bit value that ReadProcessMemory never reads half-written, and a burst counts and
	// reports every frame held past its update (none of 85 on Bookworm Adventures, 2026-11-01).
	HANDLE th=open_window_thread(hw,THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT);
	if (th && !hold_begin(th)) { CloseHandle(th); th=NULL; }
	if (th) read_counter(gCounterExpr,&gHeldAt);
	HANDLE held=th;
	int ok=capture_ex2(hw,name,0,&held,0);   // no PrintWindow while held: the window DC, asked again first; resumes after the grab
	release_held(&held);
	if (ok && gHeldAt) note_landed(name, gFlipsExpr[0] ? gHeldAt : gHeldAt-1, "held");
	if (th) CloseHandle(th);
	return ok;
}

// burst <name> <count> <interval_ms>              -- a clip of <count> frames, one every interval_ms
// burst <name> <count> update <k>                 -- a frame every k updates, timed to the game's counter
// Writes shots/<name>/0000.png ... and shots/<name>/clip.json. Reports how long each capture took.
static void do_burst(const char* name, int count, int interval_ms, int byUpdates, int k)
{
	if (count<1) count=1; if (count>2000) count=2000;
	char dir[MAX_PATH]; wsprintfA(dir,"%s\\%s",gShotDir,name); mkdirs(dir);
	uint32_t start=0;
	if (byUpdates) { if (!read_counter(gCounterExpr,&start)) { logline("burst %s: no update counter configured; use an interval instead",name); return; } }
	double capSum=0, capMax=0, holdSum=0, holdMax=0; int taken=0, late=0, lateMax=0;
	double t0=qpc_ms();
	for (int i=0;i<count && !gStop;i++) {
		char leaf[320]; wsprintfA(leaf,"%s/%04d",name,i);   // written under shots/<name>/
		double frameStart=qpc_ms();
		int ok; double cap0;
		if (byUpdates) { if (wait_counter(start + (uint32_t)i*k, 4000)<0){ logline("burst %s: counter unreadable, stopping",name); break; } cap0=qpc_ms(); ok=capture_held(leaf); holdSum+=gLastHeldMs; if (gLastHeldMs>holdMax) holdMax=gLastHeldMs;
			{ int d=(int)(gLandedAt-(start+(uint32_t)i*k)); if (gLandedAt && d>0){ late++; if(d>lateMax) lateMax=d; } } }
		else { HWND hw=resolve_window(0,NULL); if(!hw){ logline("burst %s: no window, stopping at %d",name,i); break; } cap0=qpc_ms(); ok=capture(hw,leaf); }
		if (gCaptureRefused) { logline("burst %s: captures refused (no shots on the host), stopping at %d",name,i); break; }
		if (gPwState!=PW_IDLE) { logline("burst %s: a PrintWindow has not returned, stopping at %d",name,i); break; }
		double dt=qpc_ms()-cap0;   // the capture itself, not any wait before it
		if (ok){ taken++; capSum+=dt; if(dt>capMax)capMax=dt; }
		if (!byUpdates) { double rest=interval_ms-(qpc_ms()-frameStart); if (rest>1) Sleep((DWORD)rest); }
	}
	int clipMs = byUpdates ? (k*gUpdateMs>0?k*gUpdateMs:interval_ms) : interval_ms;
	write_clip_json(dir, clipMs);
	logline("burst %s: %d frames in %s (clip interval %d ms); capture avg %.1f ms, max %.1f ms over %.0f ms wall",
		name, taken, dir, clipMs, taken?capSum/taken:0.0, capMax, qpc_ms()-t0);
	if (byUpdates && holdMax>0) logline("  the game was held avg %.2f ms, max %.2f ms per frame; %d frames landed past their update (by up to %d; shots/landed.tsv lists each)", taken?holdSum/taken:0.0, holdMax, late, lateMax);
	if (!byUpdates && capMax > interval_ms*0.5)
		logline("  note: a capture took up to %.1f ms of the %d ms interval, so the original's timing is disturbed during the burst; use 'burst %s %d update <k>' for exact spacing",capMax,interval_ms,name,count);
}

// Run one timed inner command (the tail of "at N ...") while the window thread is held. Capture/clicks/keys
// are posted; the thread is resumed right after. burst manages its own per-frame holds, so it is run unheld.
static LPARAM xy(int,int);
static void post_click(HWND,int,int,int);
static void post_key(HWND,int);
static void post_text(HWND,const char*);
static void post_focus(HWND,int);
static int  parse_vk(const char*);
// Set while exec_at holds the game: input is posted without the sleeps between its messages. Those sleeps only
// space messages for a running game; a held one reads every queued message, in order, before its next update
// (UpdateAppStep), and each millisecond of hold is time its clock runs on. A held click now holds the game for
// about 0.04 ms instead of 60.
static int gInputHeld = 0;
static void input_gap(DWORD ms){ if (!gInputHeld) Sleep(ms); }
static void exec_at(const char* inner)
{
	char cmd[32]={0}; int off=0; sscanf(inner,"%31s%n",cmd,&off); const char* rest=inner+off; while(*rest==' ')rest++;
	if (ieq(cmd,"shot")){ char n[256]="shot"; sscanf(rest,"%255s",n); capture_held(n); if (gLastHeldMs>0) logline("at shot %s: the game was held %.2f ms", n, gLastHeldMs); return; }
	if (ieq(cmd,"burst")){ char n[256]="burst"; int c=10,k=1; char kw[16]=""; int iv=100;
		if (sscanf(rest,"%255s %d %15s %d",n,&c,kw,&iv)>=2) { if (ieq(kw,"update")) do_burst(n,c,0,1,iv>0?iv:1); else do_burst(n,c,iv>0?iv:atoi(kw),0,0); }
		return; }
	HWND hw=resolve_window(0,NULL); if(!hw){ logline("at: no window for '%s'",inner); return; }
	HANDLE th=open_window_thread(hw,THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT);
	if (th && !hold_begin(th)) { CloseHandle(th); th=NULL; }
	gInputHeld = (th!=NULL);
	if (ieq(cmd,"click")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) post_click(hw,x,y,0); }
	else if (ieq(cmd,"rclick")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) post_click(hw,x,y,1); }
	else if (ieq(cmd,"key")){ char v[32]; if(sscanf(rest,"%31s",v)==1) post_key(hw,parse_vk(v)); }
	else if (ieq(cmd,"text")){ post_text(hw,rest); }
	else if (ieq(cmd,"move")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) PostMessageA(hw,WM_MOUSEMOVE,0,xy(x,y)); }
	else if (ieq(cmd,"focus")){ post_focus(hw, ieq(rest,"regain")); }
	else logline("at: cannot do '%s' at an update",cmd);
	gInputHeld = 0;
	if (th){ hold_end(th); CloseHandle(th); logline("at %s: the game was held %.2f ms", cmd, gLastHeldMs); }
}

// ----------------------------------------------------------------------------------------- input posting

static LPARAM xy(int x,int y){ return (LPARAM)((y<<16)|(x&0xffff)); }

// Map a point given in the top window's client coords to the deepest child window under it, returning that
// window and the point in its own client coords. This lets a click land on a dialog's child button control
// (the PopCap trial dialog's "Play") as well as on a game that draws and hit-tests in its own window.
// rundesk's desktop is shared by every session's test runs, so the window at the point may be another game's
// (found 2026-11-01: Bookworm Adventures' letter clicks went to a window lying over its board). A window of another
// process never gets the click: the search then stays inside the game's own window, and its descent stops at any
// child window that belongs to another process.
static HWND child_at(HWND top, int x, int y, POINT* outClient)
{
	POINT pt={x,y}; ClientToScreen(top,&pt);
	HWND tgt=WindowFromPoint(pt);
	DWORD topPid=0, tgtPid=0; GetWindowThreadProcessId(top,&topPid);
	if (tgt) GetWindowThreadProcessId(tgt,&tgtPid);
	if (!tgt || tgtPid!=topPid) {
		static LONG foreign=0;
		if (tgt && InterlockedIncrement(&foreign)<=3)
			logline("a window of another process (pid %lu) lies over the click point %d,%d on the shared desktop; the click goes to the game's own window", tgtPid, x, y);
		tgt=top;
		for (int depth=0; depth<16; depth++) {
			POINT c=pt; ScreenToClient(tgt,&c);
			HWND ch=ChildWindowFromPointEx(tgt,c,CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);
			if (!ch || ch==tgt) break;
			DWORD chPid=0; GetWindowThreadProcessId(ch,&chPid);
			if (chPid!=topPid) break;   // a child window of another process: the click stays on its parent
			tgt=ch;
		}
	}
	POINT cp=pt; ScreenToClient(tgt,&cp);
	*outClient=cp; return tgt;
}

static void post_click(HWND hwnd,int x,int y,int right)
{
	POINT cp; HWND t=child_at(hwnd,x,y,&cp); LPARAM lp=xy(cp.x,cp.y);
	PostMessageA(t, WM_MOUSEMOVE, 0, lp); input_gap(20);
	if (right){ PostMessageA(t,WM_RBUTTONDOWN,MK_RBUTTON,lp); input_gap(40); PostMessageA(t,WM_RBUTTONUP,0,lp); }
	else      { PostMessageA(t,WM_LBUTTONDOWN,MK_LBUTTON,lp); input_gap(40); PostMessageA(t,WM_LBUTTONUP,0,lp); }
}
static void post_key(HWND hwnd,int vk)
{
	UINT sc=MapVirtualKeyA(vk,MAPVK_VK_TO_VSC);
	LPARAM down=(LPARAM)(1|(sc<<16));
	LPARAM up=(LPARAM)(1|(sc<<16)|(1u<<30)|(1u<<31));
	PostMessageA(hwnd,WM_KEYDOWN,vk,down); input_gap(20); PostMessageA(hwnd,WM_KEYUP,vk,up);
}
static void post_text(HWND hwnd,const char* s){ for(;*s;s++){ PostMessageA(hwnd,WM_CHAR,(unsigned char)*s,1); input_gap(10);} }
static void post_focus(HWND hwnd,int regain)
{
	if (regain){
		PostMessageA(hwnd,WM_ACTIVATEAPP,TRUE,0);
		PostMessageA(hwnd,WM_ACTIVATE,MAKEWPARAM(WA_ACTIVE,0),0);
		PostMessageA(hwnd,WM_SETFOCUS,0,0);
	} else {
		PostMessageA(hwnd,WM_ACTIVATEAPP,FALSE,0);
		PostMessageA(hwnd,WM_ACTIVATE,MAKEWPARAM(WA_INACTIVE,0),0);
		PostMessageA(hwnd,WM_KILLFOCUS,0,0);
	}
}

// Hardware-level input through the desktop, for a game that reads the mouse with DirectInput / GetCursorPos
// instead of window messages (TipTop): SetCursorPos places the system cursor (which GetCursorPos returns), and
// SendInput delivers button/key events to the desktop the driver thread is attached to, where DirectInput's
// non-exclusive device picks them up. Documented input APIs; nothing is injected into the game. The script's
// hw* commands use this; plain click/key still post window messages (which work for the SAF/LOWOS message path).
// Only inside Windows Sandbox: on the host SetCursorPos and SendInput reach the user's own input desktop, whatever
// desktop the game is on, so hw* input is refused there as shots are.
static int hw_refused(const char* what)
{
	if (gInSandbox) return 0;
	logline("%s refused: origrun sends no desktop-wide input on the host (it would reach the user's own desktop), "
		"only inside Windows Sandbox (--sandbox)", what);
	return 1;
}
static void hw_move(HWND hwnd,int x,int y)
{
	if (hw_refused("hwmove")) return;
	POINT pt={x,y}; ClientToScreen(hwnd,&pt); SetCursorPos(pt.x,pt.y);
	INPUT in; ZeroMemory(&in,sizeof(in)); in.type=INPUT_MOUSE; in.mi.dwFlags=MOUSEEVENTF_MOVE; SendInput(1,&in,sizeof(INPUT));
}
static void hw_click(HWND hwnd,int x,int y,int right)
{
	if (hw_refused(right?"hwrclick":"hwclick")) return;
	hw_move(hwnd,x,y); Sleep(20);
	INPUT in[2]; ZeroMemory(in,sizeof(in));
	in[0].type=INPUT_MOUSE; in[0].mi.dwFlags = right?MOUSEEVENTF_RIGHTDOWN:MOUSEEVENTF_LEFTDOWN;
	in[1].type=INPUT_MOUSE; in[1].mi.dwFlags = right?MOUSEEVENTF_RIGHTUP:MOUSEEVENTF_LEFTUP;
	SendInput(1,&in[0],sizeof(INPUT)); Sleep(40); SendInput(1,&in[1],sizeof(INPUT));
}
static void hw_key(int vk)
{
	if (hw_refused("hwkey")) return;
	UINT sc=MapVirtualKeyA(vk,MAPVK_VK_TO_VSC);
	INPUT in[2]; ZeroMemory(in,sizeof(in));
	in[0].type=INPUT_KEYBOARD; in[0].ki.wVk=(WORD)vk; in[0].ki.wScan=(WORD)sc; in[0].ki.dwFlags=KEYEVENTF_SCANCODE;
	in[1].type=INPUT_KEYBOARD; in[1].ki.wVk=(WORD)vk; in[1].ki.wScan=(WORD)sc; in[1].ki.dwFlags=KEYEVENTF_SCANCODE|KEYEVENTF_KEYUP;
	SendInput(1,&in[0],sizeof(INPUT)); Sleep(20); SendInput(1,&in[1],sizeof(INPUT));
}

// ------------------------------------------------------------------------------------------- script run

static int parse_vk(const char* s){ if (s[0]=='0'&&(s[1]=='x'||s[1]=='X')) return (int)strtol(s,NULL,16); return atoi(s); }

// Load "msgbox <button> <text>" rules from the script before launch, so even a dialog that appears before
// the script's timed commands run is answered. Other commands are ignored here.
static void load_msgbox_rules(const char* scriptPath)
{
	FILE* f=fopen(scriptPath,"r"); if(!f) return;
	char line[1024];
	while (fgets(line,sizeof(line),f)) {
		char* nl=strpbrk(line,"\r\n"); if(nl)*nl=0;
		char* p=line; while(*p==' '||*p=='\t')p++;
		char cmd[32]={0}; int off=0; if(sscanf(p,"%31s%n",cmd,&off)!=1) continue;
		if (!ieq(cmd,"msgbox")) continue;
		char* rest=p+off; while(*rest==' ')rest++;
		char btn[32]={0}; int o=0; sscanf(rest,"%31s%n",btn,&o); char* txt=rest+o; while(*txt==' ')txt++;
		LONG k=gMsgboxN; if (k<MAX_MSGBOX){ lstrcpynA(gMsgbox[k].button,btn,32); lstrcpynA(gMsgbox[k].text,txt,128); gMsgboxN=k+1; logline("msgbox rule: click '%s' when a dialog says ~'%s'",btn,txt); }
	}
	fclose(f);
}

// Does the script need Windows Sandbox: does it capture anything (shot, waitshot, record, burst, or "at <N>
// shot|burst") or send desktop-wide input (hwmove, hwclick, hwrclick, hwkey)? A script that cannot be read counts as
// one that does; a run without a script takes a title shot.
static int is_hw_cmd(const char* c){ return ieq(c,"hwmove")||ieq(c,"hwclick")||ieq(c,"hwrclick")||ieq(c,"hwkey"); }
static int script_needs_sandbox(const char* scriptPath)
{
	if (!scriptPath || !scriptPath[0]) return 1;
	FILE* f=fopen(scriptPath,"r"); if(!f) return 1;
	char line[1024]; int shots=0;
	while (!shots && fgets(line,sizeof(line),f)) {
		char c1[32]={0}, c2[32]={0}, c3[32]={0};
		int n=sscanf(line,"%31s %31s %31s",c1,c2,c3);
		if (n>=1 && (ieq(c1,"shot")||ieq(c1,"waitshot")||ieq(c1,"record")||ieq(c1,"burst")||is_hw_cmd(c1))) shots=1;
		if (n>=3 && ieq(c1,"at") && (ieq(c3,"shot")||ieq(c3,"burst")||is_hw_cmd(c3))) shots=1;
	}
	fclose(f);
	return shots;
}

static void run_script(DWORD pid, const char* scriptPath)
{
	FILE* f=fopen(scriptPath,"r"); if(!f){ logline("cannot open script %s",scriptPath); return; }
	char line[1024];
	while (fgets(line,sizeof(line),f) && !gStop) {
		char* nl=strpbrk(line,"\r\n"); if(nl)*nl=0;
		char* p=line; while(*p==' '||*p=='\t')p++;
		if (*p==0||*p=='#') continue;
		char cmd[32]={0}; int off=0; sscanf(p,"%31s%n",cmd,&off); char* rest=p+off; while(*rest==' ')rest++;
		// Commands that manage timing/the window themselves are not gated on an immediate window.
		if (ieq(cmd,"msgbox")) continue;   // msgbox rules are loaded up front (load_msgbox_rules), before launch
		if (ieq(cmd,"wait")){ int ms=atoi(rest); Sleep(ms); continue; }
		if (ieq(cmd,"waitshot")){ char n[256]="shot"; int ms=8000; sscanf(rest,"%255s %d",n,&ms); capture_ready(n,ms); continue; }
		if (ieq(cmd,"clickready")){   // clickready <x> <y> <timeout_ms>: wait for the window, then click
			int x=0,y=0,ms=20000; if(sscanf(rest,"%d %d %d",&x,&y,&ms)>=2){
				int w=0; HWND hw=NULL; while(!(hw=resolve_window(0,NULL)) && w<ms && !gStop){ Sleep(150); w+=150; }
				if(hw){ Sleep(300); post_click(hw,x,y,0); logline("clickready %d %d (waited %dms)",x,y,w); }
				else logline("clickready: no window within %dms",ms);
			} continue; }
		if (ieq(cmd,"record")){
			// record <prefix> <duration_ms> <interval_ms>: grab a numbered frame every interval whenever a
			// window is present (catches a DRM/activation dialog that appears at an unpredictable time).
			char pre[200]="frame"; int dur=20000, iv=1000; sscanf(rest,"%199s %d %d",pre,&dur,&iv);
			int t=0, idx=0;
			while (t<dur && !gStop) {
				HWND hw=resolve_window(0,NULL);
				if (hw){ char n[256]; wsprintfA(n,"%s_%03d",pre,idx++); capture(hw,n); }
				Sleep(iv); t+=iv;
			}
			continue;
		}
		if (ieq(cmd,"burst")){
			// burst <name> <count> <interval_ms>   or   burst <name> <count> update <k>
			char n[256]="burst"; int c=10; char kw[16]=""; int iv=100;
			int got=sscanf(rest,"%255s %d %15s %d",n,&c,kw,&iv);
			if (got>=3 && ieq(kw,"update")) do_burst(n,c,0,1,iv>0?iv:1);
			else do_burst(n,c,(got>=3?atoi(kw):100),0,0);
			continue;
		}
		if (ieq(cmd,"at")){
			// at <N> <command...>: when the update counter reaches N, hold the game's window thread and run the
			// command (shot/click/key/text/move/focus/burst). Needs an update counter (--counter or config).
			if (!gCounterExpr[0]) { logline("at: no update counter configured (--counter or games.cfg); skipping '%s'",rest); continue; }
			unsigned long N=0; int o=0; if (sscanf(rest,"%lu%n",&N,&o)!=1) { logline("at: need 'at <N> <command>'"); continue; }
			const char* inner=rest+o; while(*inner==' ')inner++;
			int v=wait_counter((uint32_t)N,30000);
			logline("at %lu: counter reached %d -> %s", N, v, inner);
			exec_at(inner);
			continue;
		}
		HWND hwnd = resolve_window(0,NULL);   // the live main window of any of our processes, fresh each command
		if (!hwnd && !ieq(cmd,"quit")) { logline("(%s) no live window -- skipping",cmd); continue; }
		if (ieq(cmd,"move")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) PostMessageA(hwnd,WM_MOUSEMOVE,0,xy(x,y)); }
		else if (ieq(cmd,"click")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) post_click(hwnd,x,y,0); }
		else if (ieq(cmd,"rclick")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) post_click(hwnd,x,y,1); }
		else if (ieq(cmd,"key")){ char v[32]; if(sscanf(rest,"%31s",v)==1) post_key(hwnd,parse_vk(v)); }
		else if (ieq(cmd,"hwmove")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) hw_move(hwnd,x,y); logline("hwmove %d %d",x,y); }
		else if (ieq(cmd,"hwclick")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) hw_click(hwnd,x,y,0); logline("hwclick %d %d",x,y); }
		else if (ieq(cmd,"hwrclick")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) hw_click(hwnd,x,y,1); logline("hwrclick %d %d",x,y); }
		else if (ieq(cmd,"hwkey")){ char v[32]; if(sscanf(rest,"%31s",v)==1) hw_key(parse_vk(v)); logline("hwkey %s",v); }
		else if (ieq(cmd,"text")){ post_text(hwnd,rest); }
		else if (ieq(cmd,"focus")){ post_focus(hwnd, ieq(rest,"regain")); logline("focus %s",rest); }
		else if (ieq(cmd,"shot")){ char n[256]="shot"; sscanf(rest,"%255s",n); capture(hwnd,n); }
		else if (ieq(cmd,"quit")){ InterlockedExchange(&gMode3dFrozen,1); if(hwnd){ PostMessageA(hwnd,WM_SYSCOMMAND,SC_CLOSE,0); PostMessageA(hwnd,WM_CLOSE,0,0);} logline("quit (posted close)"); break; }
		else logline("unknown script command: %s",cmd);
	}
	fclose(f);
}

// ----------------------------------------------------------------------------------- Core Audio watchdog

// Host runs with --sandbox watch the host's audio sessions of the sandbox's own processes instead of the game's:
// they are logged and their loudest peak kept, not muted (the user allows the originals' sound), to tell whether
// the sandbox's sound reaches the host.
typedef struct MeterVtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void*, REFIID, void**);
	ULONG   (STDMETHODCALLTYPE *AddRef)(void*);
	ULONG   (STDMETHODCALLTYPE *Release)(void*);
	HRESULT (STDMETHODCALLTYPE *GetPeakValue)(void*, float*);
} MeterVtbl;
typedef struct { const MeterVtbl* lpVtbl; } Meter;
static const GUID kIID_IAudioMeterInformation = {0xC02216F6,0x8C67,0x4B5B,{0x9D,0x00,0xD0,0x08,0xE7,0x3E,0x00,0x64}};
static int  gAudioSandbox = 0;
static volatile LONG gSbAudioPeak = -1;   // the loudest peak seen, in thousandths; -1 while no session was seen
static int proc_leaf(DWORD pid, char* leaf, int cap)
{
	HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid); if (!p) return 0;
	char path[MAX_PATH]; DWORD n=MAX_PATH; int ok=QueryFullProcessImageNameA(p,0,path,&n); CloseHandle(p);
	if (!ok) return 0;
	const char* l=strrchr(path,'\\'); lstrcpynA(leaf,l?l+1:path,cap);
	return 1;
}
// Windows Sandbox's own processes, by their exact names: the in-box WindowsSandbox.exe and WindowsSandboxClient.exe,
// and the Store package's WindowsSandboxRemoteSession.exe. Never a part of a name: another product's "sandbox"
// (Sandboxie, say) is none of this.
static int is_sandbox_leaf(const char* leaf)
{
	return ieq(leaf,"WindowsSandbox.exe") || ieq(leaf,"WindowsSandboxClient.exe") || ieq(leaf,"WindowsSandboxRemoteSession.exe");
}
// WindowsSandboxServer.exe is the wsb command's broker, not a sandbox: the first wsb call of a logon starts it, and
// it stays. Whether a sandbox runs is what wsb list says.
static int is_broker_leaf(const char* leaf){ return ieq(leaf,"WindowsSandboxServer.exe"); }
static int pid_before(DWORD pid);

static void audio_watch(void)
{
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))) return;
	IMMDeviceEnumerator* en=NULL;
	if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator,NULL,CLSCTX_ALL,&IID_IMMDeviceEnumerator,(void**)&en))) { CoUninitialize(); return; }
	while (!gStop) {
		IMMDevice* dev=NULL;
		if (SUCCEEDED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(en,eRender,eConsole,&dev)) && dev) {
			IAudioSessionManager2* mgr=NULL;
			if (SUCCEEDED(IMMDevice_Activate(dev,&IID_IAudioSessionManager2,CLSCTX_ALL,NULL,(void**)&mgr)) && mgr) {
				IAudioSessionEnumerator* se=NULL;
				if (SUCCEEDED(IAudioSessionManager2_GetSessionEnumerator(mgr,&se)) && se) {
					int count=0; IAudioSessionEnumerator_GetCount(se,&count);
					for (int i=0;i<count;i++) {
						IAudioSessionControl* ctl=NULL;
						if (FAILED(IAudioSessionEnumerator_GetSession(se,i,&ctl)) || !ctl) continue;
						IAudioSessionControl2* c2=NULL;
						if (SUCCEEDED(IAudioSessionControl_QueryInterface(ctl,&IID_IAudioSessionControl2,(void**)&c2)) && c2) {
							DWORD pid=0; IAudioSessionControl2_GetProcessId(c2,&pid);
							char leaf[MAX_PATH];
							if (gAudioSandbox) {
								if (pid && proc_leaf(pid,leaf,sizeof(leaf)) && (is_sandbox_leaf(leaf) || is_broker_leaf(leaf) || (ieq(leaf,"vmwp.exe") && !pid_before(pid)))) {
									float pk=0; Meter* mi=NULL;
									if (SUCCEEDED(IAudioSessionControl2_QueryInterface(c2,&kIID_IAudioMeterInformation,(void**)&mi)) && mi) { mi->lpVtbl->GetPeakValue(mi,&pk); mi->lpVtbl->Release(mi); }
									LONG v=(LONG)(pk*1000.0f), was=gSbAudioPeak;
									if (was<0) logline("the sandbox's %s (pid %lu) has an audio session on the host", leaf, pid);
									if (v>was) InterlockedExchange(&gSbAudioPeak,v);
								}
							}
							else if (is_our_pid(pid)) {
								ISimpleAudioVolume* sv=NULL;
								if (SUCCEEDED(IAudioSessionControl2_QueryInterface(c2,&IID_ISimpleAudioVolume,(void**)&sv)) && sv) {
									ISimpleAudioVolume_SetMute(sv,TRUE,NULL);
									ISimpleAudioVolume_Release(sv);
								}
								if (InterlockedExchange(&gAudioSeen,1)==0)
									logline("WARNING: game opened an audio session (pid %lu) -- muted it. A sound stand-in let output through, or --sound real was used.", pid);
							}
							IAudioSessionControl2_Release(c2);
						}
						IAudioSessionControl_Release(ctl);
					}
					IAudioSessionEnumerator_Release(se);
				}
				IAudioSessionManager2_Release(mgr);
			}
			IMMDevice_Release(dev);
		}
		for (int i=0;i<10 && !gStop;i++) Sleep(50);
	}
	IMMDeviceEnumerator_Release(en);
	CoUninitialize();
}

static DWORD WINAPI audio_thread(LPVOID p){ audio_watch(); return 0; }

// ------------------------------------------------------------------------------------- dialog watch
//
// Message boxes (DRM/activation/update nags, Steam/DirectX errors) are standard dialogs of window class
// "#32770". We watch our process's desktop for them and, using only documented window APIs from this host
// (never injected into the game), log each box's caption, text and buttons with the run time, then answer
// it: with the button a script "msgbox <button> <text>" rule names, else the dialog's default button.
// Gaps this cannot close (see docs/ORIGRUN.md): the box has already made its own sound by the time it is
// visible, and a MessageBeep or PlaySound system sound is not a window and is not seen here.

#define MAX_DLG 128
static struct { HWND hwnd; DWORD firstTick; int logged; int handled; } gDlg[MAX_DLG];
static int gDlgN = 0;

typedef struct { char text[512]; HWND btn[16]; char label[16][48]; int nbtn; } DlgInfo;
static BOOL CALLBACK dlg_child_cb(HWND h, LPARAM lp)
{
	DlgInfo* di = (DlgInfo*)lp;
	char cls[32]; GetClassNameA(h, cls, sizeof(cls));
	if (ieq(cls, "Static")) {
		char t[256]; if (GetWindowTextA(h, t, sizeof(t)) > 0) {
			size_t l = strlen(di->text);
			if (l < sizeof(di->text)-2) { if (l) { di->text[l++]=' '; di->text[l]=0; } lstrcpynA(di->text+l, t, (int)(sizeof(di->text)-l)); }
		}
	} else if (ieq(cls, "Button") && di->nbtn < 16) {
		LONG st = GetWindowLongA(h, GWL_STYLE); int type = st & 0xf;
		if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) {
			char t[48]; GetWindowTextA(h, t, sizeof(t));
			di->btn[di->nbtn] = h; strip_amp(t, di->label[di->nbtn], 48); di->nbtn++;
		}
	}
	return TRUE;
}

static void handle_dialog(HWND dlg)
{
	char cap[256]=""; GetWindowTextA(dlg, cap, sizeof(cap));
	DlgInfo di; ZeroMemory(&di, sizeof(di)); EnumChildWindows(dlg, dlg_child_cb, (LPARAM)&di);
	char btns[512]=""; for (int i=0;i<di.nbtn;i++){ lstrcatA(btns, i?" | ":""); lstrcatA(btns, di.label[i]); }
	logline("DIALOG caption='%s' text='%s' buttons=[%s]", cap, di.text, btns);

	char combined[768]; wsprintfA(combined, "%s %s", cap, di.text);
	HWND target = NULL; const char* why = "default button";
	LONG n = gMsgboxN;
	for (int r = 0; r < n && !target; r++) {
		if (istr_has(combined, gMsgbox[r].text)) {
			for (int i=0;i<di.nbtn;i++) if (ieq(di.label[i], gMsgbox[r].button)) { target = di.btn[i]; why = "script rule"; break; }
		}
	}
	if (!target) {   // dialog's own default button
		LRESULT def = SendMessageA(dlg, DM_GETDEFID, 0, 0);
		if (HIWORD(def) == DC_HASDEFID) { HWND b = GetDlgItem(dlg, LOWORD(def)); if (b) target = b; }
	}
	if (!target && di.nbtn > 0) target = di.btn[0];
	if (target) {
		char t[48]="?"; strip_amp((GetWindowTextA(target,t,sizeof(t)),t), t, 48);
		logline("  answered with '%s' (%s)", t, why);
		PostMessageA(target, BM_CLICK, 0, 0);
	} else {
		logline("  no button to answer with; closing");
		PostMessageA(dlg, WM_CLOSE, 0, 0);
	}
}

static BOOL CALLBACK dlg_enum_cb(HWND h, LPARAM lp)
{
	if (!IsWindowVisible(h)) return TRUE;
	char cls[32]; GetClassNameA(h, cls, sizeof(cls));
	if (!ieq(cls, "#32770")) return TRUE;
	DWORD pid=0; GetWindowThreadProcessId(h, &pid);
	if (!is_our_pid(pid)) return TRUE;
	int idx=-1; for (int i=0;i<gDlgN;i++) if (gDlg[i].hwnd==h) { idx=i; break; }
	DWORD now = GetTickCount();
	if (idx < 0) { if (gDlgN>=MAX_DLG) return TRUE; idx=gDlgN++; gDlg[idx].hwnd=h; gDlg[idx].firstTick=now; gDlg[idx].logged=0; gDlg[idx].handled=0; }
	if (!gDlg[idx].logged) { char cap[256]=""; GetWindowTextA(h,cap,sizeof(cap)); logline("dialog appeared: '%s' (pid %lu)", cap, pid); gDlg[idx].logged=1; }
	// Small grace so a concurrent waitshot/record can still catch it before we dismiss it.
	if (!gDlg[idx].handled && now - gDlg[idx].firstTick >= 300) { handle_dialog(h); gDlg[idx].handled=1; }
	return TRUE;
}

static DWORD WINAPI dialog_thread(LPVOID p)
{
	for (int w=0; !gDesk && !gStop && w<30000; w+=50) Sleep(50);   // wait for the driver to open the desktop
	if (!gDesk) return 0;
	SetThreadDesktop(gDesk);   // dialog handles are per-desktop, like the main window's
	while (!gStop) {
		EnumDesktopWindows(gDesk, dlg_enum_cb, 0);
		for (int i=0;i<5 && !gStop;i++) Sleep(50);
	}
	return 0;
}

// ------------------------------------------------------------------------------------- the 3D mode (--3d)
//
// The Hardware Acceleration setting a SexyAppBase game reads at start is its registry value "Is3D" (a DWORD under
// the game's HKCU key, which games.tsv names). origrun sets it for the run and puts the earlier value back
// afterwards. The game may still refuse it: its Direct3D test can reject the card, or 3D init can fail and drop it to
// software. So the mode the game ended up in is read from its memory (gMode3dExpr) while it runs, the way the update
// counter is, and a run whose mode differs from the one asked for fails loudly.

static const char* kIs3DValue = "Is3D";
static const char* hkcu_sub(const char* sub)
{
	if (_strnicmp(sub,"HKCU\\",5)==0) return sub+5;
	if (_strnicmp(sub,"HKEY_CURRENT_USER\\",18)==0) return sub+18;
	return sub;
}
// Where the game keeps a key named the host's way (games.tsv, --3d-key): its root, and the path in *path. On the host
// that is the user's HKCU, with UAC's VirtualStore copy for a game that writes under HKLM. Inside the sandbox origrun
// and the game run as SYSTEM: HKCU is SYSTEM's own hive (HKEY_USERS\S-1-5-18) for both, so an HKCU key is opened as
// it is; and UAC never virtualizes SYSTEM, so a VirtualStore key is the 32-bit game's HKLM key itself
// (HKLM\SOFTWARE\X, which the 32-bit view keeps in HKLM\SOFTWARE\WOW6432Node\X). Every open uses the 32-bit view,
// the games' own (origrun is 32-bit too, so on the host nothing changes).
static const char kVsMachine[]="Software\\Classes\\VirtualStore\\MACHINE\\";
static HKEY game_key(const char* sub, char* path, int cap)
{
	const char* p=hkcu_sub(sub);
	if (gInSandbox && gAsSystem && _strnicmp(p,kVsMachine,sizeof(kVsMachine)-1)==0) {
		p+=sizeof(kVsMachine)-1;
		static const char kWow[]="SOFTWARE\\WOW6432Node\\";
		if (_strnicmp(p,kWow,sizeof(kWow)-1)==0) _snprintf(path,cap-1,"SOFTWARE\\%s",p+sizeof(kWow)-1);
		else lstrcpynA(path,p,cap);
		path[cap-1]=0;
		return HKEY_LOCAL_MACHINE;
	}
	lstrcpynA(path,p,cap);
	return HKEY_CURRENT_USER;
}
static HKEY open_game_key(const char* sub)
{
	char path[600]; HKEY root=game_key(sub,path,sizeof(path)), k=NULL;
	if (RegCreateKeyExA(root,path,0,NULL,0,KEY_QUERY_VALUE|KEY_SET_VALUE|KEY_WOW64_32KEY,NULL,&k,NULL)!=ERROR_SUCCESS) return NULL;
	return k;
}

// The game's cached Direct3D test results, the "Test3D" subkey D3DTester keeps beside Is3D. On rundesk's desktop
// DirectDraw has no hardware, so a test that re-runs there fails and caches the failure, and the game then refuses
// 3D on the user's own desktop too. origrun snapshots the values before a run and puts them back after it.
typedef struct { char name[64]; DWORD type; BYTE data[512]; DWORD len; } RegVal;
typedef struct { int n; RegVal v[16]; } RegVals;   // n = -1: the subkey does not exist
static void read_test3d(const char* sub, RegVals* out)
{
	char base[600], path[620]; HKEY root=game_key(sub,base,sizeof(base)); wsprintfA(path,"%s\\Test3D",base);
	HKEY k; out->n=-1;
	if (RegOpenKeyExA(root,path,0,KEY_QUERY_VALUE|KEY_WOW64_32KEY,&k)!=ERROR_SUCCESS) return;
	out->n=0;
	for (DWORD i=0; out->n<16; i++) {
		RegVal* v=&out->v[out->n]; DWORD nl=sizeof(v->name), dl=sizeof(v->data);
		if (RegEnumValueA(k,i,v->name,&nl,NULL,&v->type,v->data,&dl)!=ERROR_SUCCESS) break;
		v->len=dl; out->n++;
	}
	RegCloseKey(k);
}
static int same_test3d(const RegVals* a, const RegVals* b)
{
	if (a->n!=b->n) return 0;
	for (int i=0;i<a->n;i++) {
		int found=0;
		for (int j=0;j<b->n && !found;j++)
			found = !lstrcmpiA(a->v[i].name,b->v[j].name) && a->v[i].type==b->v[j].type && a->v[i].len==b->v[j].len && !memcmp(a->v[i].data,b->v[j].data,a->v[i].len);
		if (!found) return 0;
	}
	return 1;
}
static void restore_test3d(const char* sub, const RegVals* before)
{
	RegVals now; read_test3d(sub,&now);
	if (same_test3d(before,&now)) return;
	// what the game wrote, so a run can preset it (--reg) and the game skips its Direct3D test next time
	for (int i=0;i<now.n;i++) {
		char hex[3*64+1]=""; DWORD m=now.v[i].len<64?now.v[i].len:64;
		for (DWORD b=0;b<m;b++) wsprintfA(hex+3*b,"%02x%s",now.v[i].data[b],b+1<m?",":"");
		logline("3D: the game's Test3D value %s (type %lu): %s", now.v[i].name, now.v[i].type, hex);
	}
	char base[600], path[620]; HKEY root=game_key(sub,base,sizeof(base)); wsprintfA(path,"%s\\Test3D",base);
	RegDeleteKeyExA(root,path,KEY_WOW64_32KEY,0);
	if (before->n>=0) {
		HKEY k;
		if (RegCreateKeyExA(root,path,0,NULL,0,KEY_SET_VALUE|KEY_WOW64_32KEY,NULL,&k,NULL)==ERROR_SUCCESS) {
			for (int i=0;i<before->n;i++) RegSetValueExA(k,before->v[i].name,0,before->v[i].type,before->v[i].data,before->v[i].len);
			RegCloseKey(k);
		}
	}
	logline("3D: the game rewrote its cached Direct3D test results (Test3D); put the earlier ones back");
}
// Reads the game's Is3D: 1 and *out when it is set, 0 when it is not.
static int reg_get_is3d(const char* sub, DWORD* out)
{
	HKEY k=open_game_key(sub); if(!k) return 0;
	DWORD type=0, v=0, n=sizeof(v);
	int ok = RegQueryValueExA(k,kIs3DValue,NULL,&type,(BYTE*)&v,&n)==ERROR_SUCCESS && type==REG_DWORD;
	RegCloseKey(k); if (ok) *out=v; return ok;
}
static int reg_set_is3d(const char* sub, int isSet, DWORD v)
{
	HKEY k=open_game_key(sub); if(!k) return 0;
	LONG r = isSet ? RegSetValueExA(k,kIs3DValue,0,REG_DWORD,(const BYTE*)&v,sizeof(v)) : RegDeleteValueA(k,kIs3DValue);
	RegCloseKey(k); return r==ERROR_SUCCESS || (!isSet && r==ERROR_FILE_NOT_FOUND);
}

// --dgvoodoo <dir>: rundesk's desktop gives legacy DirectDraw no hardware, so a game's 3D mode needs a DirectDraw and
// Direct3D 7 implementation over Direct3D 11 in its copy: dgVoodoo2's DDraw.dll and D3DImm.dll (from <dir>\MS\x86 or
// <dir>), read from wherever the user keeps them and never copied into the repository, plus a dgVoodoo.conf origrun
// writes. The files placed are listed in origrun-wrapper.txt in the copy, and every run removes a previous run's, so
// a wrapper never carries over into a run that did not ask for it.
static const char* kWrapMarker = "origrun-wrapper.txt";
static const char* kDgvConf =
	"; Written by origrun for this staged copy (docs/ORIGRUN.md, \"Hardware acceleration\"): dgVoodoo2 in a window\r\n"
	"; on rundesk's hidden desktop, flip-model presentation, no watermark, the mouse never\r\n"
	"; captured, the screen saver left alone.\r\n"
	"Version = 0x287\r\n\r\n"
	"[General]\r\nOutputAPI = d3d11_fl11_0\r\nFullScreenMode = false\r\nScalingMode = unspecified\r\n"
	"KeepWindowAspectRatio = true\r\nCaptureMouse = false\r\nCenterAppWindow = false\r\nDisableScreenSaver = false\r\n\r\n"
	"[GeneralExt]\r\nPresentationModel = flip_discard\r\nWatermarkDisplayDuration = 0\r\nFreeMouse = true\r\n\r\n"
	"[Glide]\r\n3DfxWatermark = false\r\n3DfxSplashScreen = false\r\n\r\n"
	"[DirectX]\r\nDisableAndPassThru = false\r\nAppControlledScreenMode = false\r\nDisableAltEnterToToggleScreenMode = true\r\n"
	"dgVoodooWatermark = false\r\n";

// Deletes go only where origrun stages copies. A path may be deleted only when its full long path and its final path
// (junctions and links resolved) are both under <repo>\build\ or under --stage-root, resolved the same way.
static int full_long_w(const char* in, WCHAR* out, DWORD cap)
{
	WCHAR w[MAX_PATH], full[MAX_PATH];
	if (!MultiByteToWideChar(CP_ACP,0,in,-1,w,MAX_PATH)) return 0;
	DWORD n=GetFullPathNameW(w,MAX_PATH,full,NULL); if (!n || n>=MAX_PATH) return 0;
	n=GetLongPathNameW(full,out,cap); if (!n || n>=cap) return 0;   // the path must exist
	n=lstrlenW(out); while (n>3 && out[n-1]==L'\\') out[--n]=0;
	return 1;
}
static int final_w(const char* in, WCHAR* out, DWORD cap)
{
	HANDLE h=CreateFileA(in,0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
	if (h==INVALID_HANDLE_VALUE) return 0;
	DWORD n=GetFinalPathNameByHandleW(h,out,cap,FILE_NAME_NORMALIZED); CloseHandle(h);
	if (!n || n>=cap) return 0;
	while (n>3 && out[n-1]==L'\\') out[--n]=0;
	return 1;
}
static int under_w(const WCHAR* p, const WCHAR* root)
{
	int n=lstrlenW(root);
	return n>0 && lstrlenW(p)>n+1 && p[n]==L'\\' && CompareStringOrdinal(p,n,root,n,TRUE)==CSTR_EQUAL;
}
static int path_deletable(const char* path)
{
	char roots[2][MAX_PATH]; int nr=0;
	joinp(roots[nr++],gRepo,"build");
	if (gStageRoot[0]) lstrcpynA(roots[nr++],gStageRoot,MAX_PATH);
	WCHAR pl[MAX_PATH], pf[MAX_PATH+8], rl[MAX_PATH], rf[MAX_PATH+8];
	if (!full_long_w(path,pl,MAX_PATH) || !final_w(path,pf,MAX_PATH+8)) return 0;
	for (int i=0;i<nr;i++)
		if (full_long_w(roots[i],rl,MAX_PATH) && final_w(roots[i],rf,MAX_PATH+8) && under_w(pl,rl) && under_w(pf,rf)) return 1;
	return 0;
}
static int delete_guarded(const char* path)
{
	if (!path_deletable(path)) { logline("not deleting %s: it is not under build\\ or --stage-root", path); SetLastError(ERROR_ACCESS_DENIED); return 0; }
	return DeleteFileA(path);
}

// A file's size and SHA-256 (64 hex digits). 0 if it cannot be read whole.
static int file_sig(const char* path, unsigned long long* size, char hex[65])
{
	HANDLE f=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (f==INVALID_HANDLE_VALUE) return 0;
	LARGE_INTEGER sz; int ok=GetFileSizeEx(f,&sz)!=0;
	BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_HASH_HANDLE hh=NULL;
	if (ok) ok = BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0));
	if (ok) ok = BCRYPT_SUCCESS(BCryptCreateHash(alg,&hh,NULL,0,NULL,0,0));
	unsigned long long total=0;
	while (ok) {
		unsigned char buf[16384]; DWORD n=0;
		if (!ReadFile(f,buf,sizeof(buf),&n,NULL)) { ok=0; break; }
		if (!n) break;
		total+=n;
		if (!BCRYPT_SUCCESS(BCryptHashData(hh,buf,n,0))) ok=0;
	}
	unsigned char d[32];
	if (ok) ok = BCRYPT_SUCCESS(BCryptFinishHash(hh,d,32,0)) && total==(unsigned long long)sz.QuadPart;
	if (ok) { for (int i=0;i<32;i++) wsprintfA(hex+i*2,"%02x",d[i]); hex[64]=0; *size=total; }
	if (hh) BCryptDestroyHash(hh);
	if (alg) BCryptCloseAlgorithmProvider(alg,0);
	CloseHandle(f);
	return ok;
}

// The marker lists each file origrun placed, one line each: "<size> <sha256> <name>". A line is added only once its
// file is in place, and a file is deleted only while its size and hash still match its line.
typedef struct { unsigned long long size; char hash[65]; char name[MAX_PATH]; char raw[MAX_PATH+96]; int parsed; } WrapEntry;
static int write_marker(const WrapEntry* e, int n)
{
	char mk[MAX_PATH]; joinp(mk,gGameDir,kWrapMarker);
	if (!n) return GetFileAttributesA(mk)==INVALID_FILE_ATTRIBUTES || delete_guarded(mk);
	FILE* m=fopen(mk,"wb"); if (!m) return 0;
	int ok=1;
	for (int i=0;i<n;i++) {
		if (e[i].parsed) { if (fprintf(m,"%llu %s %s\r\n",e[i].size,e[i].hash,e[i].name)<0) ok=0; }
		else if (fprintf(m,"%s\r\n",e[i].raw)<0) ok=0;
	}
	if (fclose(m)) ok=0;
	return ok;
}
// Delete the files of a failed placement (each only while it matches its line), and leave the marker listing what is
// left. 0 if anything is left.
static int rollback_wrapper(WrapEntry* e, int n)
{
	WrapEntry left[4]; int nl=0;
	for (int i=0;i<n;i++) {
		char p[MAX_PATH]; joinp(p,gGameDir,e[i].name);
		unsigned long long sz; char hx[65];
		if (file_sig(p,&sz,hx) && sz==e[i].size && !lstrcmpiA(hx,e[i].hash) && delete_guarded(p)) continue;
		if (GetFileAttributesA(p)==INVALID_FILE_ATTRIBUTES) continue;
		logline("--dgvoodoo: could not take %s back out of the copy (err %lu)", p, GetLastError());
		if (nl<4) left[nl++]=e[i];
	}
	if (!write_marker(left,nl)) logline("--dgvoodoo: could not rewrite %s\\%s", gGameDir, kWrapMarker);
	return nl==0;
}

// Take out the wrapper an earlier run placed. 0 stops the run: a file of it that could not be deleted, or that changed
// since it was placed (its size or SHA-256 differ, or its line records neither), stays in the copy and in the marker,
// and a wrapper must never carry over into a run that did not ask for it.
static int remove_wrapper(void)
{
	char mk[MAX_PATH]; joinp(mk,gGameDir,kWrapMarker);
	FILE* f=fopen(mk,"rb"); if(!f) return 1;
	if (!path_deletable(mk)) {
		fclose(f);
		logline("%s is not under build\\ or --stage-root, so origrun deletes nothing there; stopping the run", mk);
		return 0;
	}
	WrapEntry e[16]; int n=0; char ln[MAX_PATH+96];
	while (fgets(ln,sizeof(ln),f)) {
		char* q=strpbrk(ln,"\r\n"); if(q)*q=0;
		if (!ln[0]) continue;
		if (n>=16) { fclose(f); logline("%s has more lines than origrun ever writes; deleting nothing; stopping the run", mk); return 0; }
		WrapEntry* x=&e[n++]; ZeroMemory(x,sizeof(*x)); lstrcpynA(x->raw,ln,sizeof(x->raw));
		int off=0;
		if (sscanf(ln,"%llu %64s %n",&x->size,x->hash,&off)==2 && off>0 && lstrlenA(x->hash)==64 && ln[off]) {
			lstrcpynA(x->name,ln+off,MAX_PATH);
			x->parsed = !strpbrk(x->name,"\\/:") && lstrcmpA(x->name,".") && lstrcmpA(x->name,"..");
		}
	}
	fclose(f);
	WrapEntry left[16]; int nl=0, removed=0;
	for (int i=0;i<n;i++) {
		if (!e[i].parsed) { logline("wrapper marker line '%s' records no size and hash; nothing deleted for it", e[i].raw); left[nl++]=e[i]; continue; }
		char p[MAX_PATH]; joinp(p,gGameDir,e[i].name);
		if (GetFileAttributesA(p)==INVALID_FILE_ATTRIBUTES) continue;   // already gone
		unsigned long long sz; char hx[65];
		if (!file_sig(p,&sz,hx) || sz!=e[i].size || lstrcmpiA(hx,e[i].hash)) {
			logline("%s changed since origrun placed it (its size or SHA-256 differ); left in place", p);
			left[nl++]=e[i]; continue;
		}
		if (!delete_guarded(p)) { logline("deleting %s failed (err %lu)", p, GetLastError()); left[nl++]=e[i]; continue; }
		removed++;
	}
	if (nl) {
		if (!write_marker(left,nl)) logline("could not rewrite %s", mk);
		logline("removed %d of the previous run's wrapper files; %d stay in the copy (listed in %s). Stopping the run: "
			"a wrapper must not carry over into a run that did not ask for it. Delete them by hand, or stage a fresh copy.", removed, nl, mk);
		return 0;
	}
	if (!delete_guarded(mk)) { logline("deleting %s failed (err %lu); stopping the run", mk, GetLastError()); return 0; }
	logline("removed the previous run's wrapper from the copy (%d files)", removed);
	return 1;
}
static int place_wrapper(const char* dir, const char* installDir)
{
	// only ever into a staged copy: never an install folder, and only where a rollback may delete
	if ((installDir && installDir[0] && !lstrcmpiA(gGameDir,installDir)) || istr_has(gGameDir,"\\Program Files") || istr_has(gGameDir,"\\steamapps\\")) {
		logline("--dgvoodoo: %s looks like an install folder, not a staged copy; refusing to place the wrapper", gGameDir); return 0; }
	if (!path_deletable(gGameDir)) {
		logline("--dgvoodoo: %s is not under build\\ or --stage-root; refusing to place the wrapper", gGameDir); return 0; }
	static const char* dlls[] = { "DDraw.dll", "D3DImm.dll" };
	char src[2][MAX_PATH];
	for (int i=0;i<2;i++) {
		wsprintfA(src[i],"%s\\MS\\x86\\%s",dir,dlls[i]);
		if (!file_exists(src[i])) wsprintfA(src[i],"%s\\%s",dir,dlls[i]);
		if (!file_exists(src[i])) { logline("--dgvoodoo: %s not found under %s (MS\\x86 or the folder itself)", dlls[i], dir); return 0; }
	}
	const char* placed[3] = { dlls[0], dlls[1], "dgVoodoo.conf" };
	for (int i=0;i<3;i++) { char d[MAX_PATH]; joinp(d,gGameDir,placed[i]);
		if (file_exists(d)) { logline("--dgvoodoo: the copy already has its own %s; refusing to overwrite it", placed[i]); return 0; } }
	WrapEntry ent[3]; int n=0;
	for (int i=0;i<3;i++) {
		char d[MAX_PATH]; joinp(d,gGameDir,placed[i]);
		int created=0, ok;
		if (i<2) {
			ok = CopyFileA(src[i],d,TRUE);   // never over an existing file
			DWORD err=GetLastError();
			created = ok || (err!=ERROR_FILE_EXISTS && err!=ERROR_ALREADY_EXISTS);   // a failed copy may leave a part
			if (!ok) logline("--dgvoodoo: copying %s failed (err %lu)", src[i], err);
		} else {
			HANDLE h=CreateFileA(d,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
			ok = h!=INVALID_HANDLE_VALUE; created=ok;
			if (ok) { DWORD len=(DWORD)strlen(kDgvConf), wr=0; ok = WriteFile(h,kDgvConf,len,&wr,NULL) && wr==len; CloseHandle(h); }
			if (!ok) logline("--dgvoodoo: writing %s failed (err %lu)", d, GetLastError());
		}
		if (ok) {
			ZeroMemory(&ent[n],sizeof(ent[n]));
			ok = file_sig(d,&ent[n].size,ent[n].hash);
			if (ok) { lstrcpynA(ent[n].name,placed[i],MAX_PATH); ent[n].parsed=1; n++; created=0;   // now recorded: rolled back by its line
				ok = write_marker(ent,n);
				if (!ok) logline("--dgvoodoo: cannot write %s\\%s", gGameDir, kWrapMarker); }
			else logline("--dgvoodoo: cannot read back %s", d);
		}
		if (!ok) {
			if (created && GetFileAttributesA(d)!=INVALID_FILE_ATTRIBUTES && !delete_guarded(d))
				logline("--dgvoodoo: could not delete the partly placed %s (err %lu)", d, GetLastError());
			if (!rollback_wrapper(ent,n)) logline("--dgvoodoo: the wrapper could not be taken back out; the next run stops until it is");
			return 0;
		}
	}
	logline("placed dgVoodoo2 in the copy: %s, %s (from %s) and origrun's dgVoodoo.conf (windowed, flip model, no watermark)", dlls[0], dlls[1], dir);
	return 1;
}

// Polls the game's 3D flag every 100 ms while its window is up, logging the first reading and every change, until
// "quit" freezes the verdict or the run ends.
static DWORD WINAPI mode3d_thread(LPVOID p)
{
	for (int w=0; !gDesk && !gStop && w<30000; w+=50) Sleep(50);   // the driver opens the game desktop
	if (gDesk) SetThreadDesktop(gDesk);
	double t0=qpc_ms(); int last=-1;
	while (!gStop && !gMode3dFrozen) {
		uint32_t v=0;
		if (resolve_window(0,NULL) && read_counter(gMode3dExpr,&v) && !gMode3dFrozen) {
			int on = (v & 0xff) != 0;
			if (on != last) {
				logline("3D mode: the game reports %s (flag byte %u, %.1f s after launch)", on?"ON (Direct3D)":"OFF (software)", v&0xff, (qpc_ms()-t0)/1000.0);
				if (last >= 0) InterlockedIncrement(&gMode3dFlips);
				last = on;
			}
			InterlockedExchange(&gMode3dLast,on);
		}
		Sleep(100);
	}
	return 0;
}

// Logs each change of a --watch value ("watch 1: 48 -> 47 at update 2150"; "-" while it cannot be read, as through a
// null pointer), at most 4000 lines per watch. Polls every millisecond under timeBeginPeriod(1): Sleep in origrun's
// process otherwise rounds up to the 15.6 ms timer tick, which would miss values that change every update.
static DWORD WINAPI watch_thread(LPVOID p)
{
	for (int w=0; !gDesk && !gStop && w<30000; w+=50) Sleep(50);
	timeBeginPeriod(1);
	char last[4][16]; int lines[4]={0};
	for (int i=0;i<gWatchN;i++) lstrcpyA(last[i],"?");
	while (!gStop) {
		if (resolve_window(0,NULL)) {
			for (int i=0;i<gWatchN;i++) {
				uint32_t v=0; char now[16];
				if (read_counter(gWatchExpr[i],&v)) wsprintfA(now,"%lu",(unsigned long)v); else lstrcpyA(now,"-");
				if (lstrcmpA(now,last[i]) && lines[i]<4000) {
					uint32_t c=0; int hc = gCounterExpr[0] && read_counter(gCounterExpr,&c);
					if (hc) logline("watch %d: %s -> %s at update %lu", i+1, last[i], now, (unsigned long)c);
					else logline("watch %d: %s -> %s", i+1, last[i], now);
					if (++lines[i]==4000) logline("watch %d: 4000 changes logged; no more", i+1);
				}
				lstrcpyA(last[i],now);
			}
		}
		Sleep(1);
	}
	timeEndPeriod(1);
	return 0;
}

// ----------------------------------------------------------------------------------------------- driver

typedef struct { char script[MAX_PATH]; } DriverArg;
static DWORD WINAPI driver_thread(LPVOID p)
{
	DriverArg* a=(DriverArg*)p;
	if (!attach_desktop()) return 0;   // so the game's window handles are usable from this thread
	// Informational only: the game's window can be delayed (activation phone-home) or flaky under load, so we
	// never bail on this -- the script's own waitshot/record commands poll for the window.
	DWORD pid=find_game_pid(20000);
	if (pid) {
		gGamePidHint=pid;
		char gpu[MAX_PATH]; int g=process_uses_gpu_api(pid,gpu,sizeof(gpu));
		logline("game window first seen: pid=%lu; GPU modules: %s; %s", pid, g>0 ? gpu : (g<0 ? "(list not readable)" : "none"),
			gInSandbox ? "inside Windows Sandbox, so shots are taken" : "on the host, so no shot is taken");
	}
	else logline("no window in the first 20s; proceeding anyway (the script polls for it)");
	if (GetEnvironmentVariableA("ORIGRUN_MONITOR",NULL,0) || getenv("ORIGRUN_MONITOR")) {
		for (int t=0;t<30 && !gStop;t++){ DWORD rp=0; HWND rw=resolve_window(0,&rp); logline("--- t+%dms  resolve_window -> hwnd=%p pid=%lu ---",t*1000,(void*)rw,rp); dump_desktop(); Sleep(1000); }
	}
	if (a->script[0]) run_script(pid, a->script);
	else { capture_ready("title", 40000); }
	return 0;
}

// ------------------------------------------------------------------------------------------------- main

static const char* lookup_game(const char* name, const char* byExe, char* srcDir, char* exeLeaf, char* regKey, char* flag3d);

// --------------------------------------------------------------------------------------------- launching

typedef struct { HANDLE proc, thread, job; } Launched;
// rundesk given: the host's way, the game on rundesk's hidden desktop (rundesk's job ends it). rundesk NULL: inside
// the sandbox, the game on the sandbox's own WinSta0\Default, in a job of origrun's that ends it and its children.
static int launch_game(const char* rundesk, Launched* L)
{
	ZeroMemory(L,sizeof(*L));
	STARTUPINFOA si; ZeroMemory(&si,sizeof(si)); si.cb=sizeof(si);
	PROCESS_INFORMATION pi; ZeroMemory(&pi,sizeof(pi));
	char cmd[MAX_PATH*2];
	if (rundesk) {
		wsprintfA(cmd,"\"%s\" \"%s\"",rundesk,gExePath);
		logline("launching under rundesk: %s",gExePath);
		if (!CreateProcessA(NULL,cmd,NULL,NULL,FALSE,0,NULL,gGameDir,&si,&pi))
			{ logline("CreateProcess(rundesk) failed (err %lu)",GetLastError()); return 0; }
	} else {
		char cwd[MAX_PATH]; lstrcpynA(cwd,gExePath,MAX_PATH); char* e=strrchr(cwd,'\\'); if(e)*e=0;
		L->job=CreateJobObjectA(NULL,NULL);
		if (L->job) {
			JOBOBJECT_EXTENDED_LIMIT_INFORMATION li; ZeroMemory(&li,sizeof(li));
			li.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			SetInformationJobObject(L->job,JobObjectExtendedLimitInformation,&li,sizeof(li));
		}
		si.lpDesktop=(LPSTR)"WinSta0\\Default";
		wsprintfA(cmd,"\"%s\"",gExePath);
		logline("launching on the sandbox's own desktop (WinSta0\\Default): %s",gExePath);
		if (!CreateProcessA(NULL,cmd,NULL,NULL,FALSE,CREATE_SUSPENDED|BELOW_NORMAL_PRIORITY_CLASS,NULL,cwd,&si,&pi))
			{ logline("CreateProcess failed (err %lu)",GetLastError()); if (L->job) CloseHandle(L->job); L->job=NULL; return 0; }
		if (L->job && !AssignProcessToJobObject(L->job,pi.hProcess)) logline("warning: the game is not in origrun's job (err %lu)",GetLastError());
		ResumeThread(pi.hThread);
	}
	L->proc=pi.hProcess; L->thread=pi.hThread;
	return 1;
}
// 1 when the timeout came first. With a job, the run lasts until every process in it has ended (a launcher's game too).
static int wait_game(Launched* L, int timeout)
{
	if (!L->job) return WaitForSingleObject(L->proc,(DWORD)timeout*1000)==WAIT_TIMEOUT;
	DWORD t0=GetTickCount();
	for (;;) {
		JOBOBJECT_BASIC_ACCOUNTING_INFORMATION ai;
		if (QueryInformationJobObject(L->job,JobObjectBasicAccountingInformation,&ai,sizeof(ai),NULL) && ai.ActiveProcesses==0) return 0;
		if (GetTickCount()-t0 >= (DWORD)timeout*1000) return 1;
		Sleep(200);
	}
}
static void end_game(Launched* L)
{
	if (L->job) TerminateJobObject(L->job,1);
	else TerminateProcess(L->proc,1);   // only our rundesk child; its job kill-on-close ends the game
	WaitForSingleObject(L->proc,5000);
}
static void close_game(Launched* L){ if(L->thread) CloseHandle(L->thread); if(L->proc) CloseHandle(L->proc); if(L->job) CloseHandle(L->job); }

// Drive and watch a launched game to its end: the script, message boxes, audio, the 3D flag, the timeout; then the
// game's 3D setting back, and the results. Host runs and runs inside the sandbox share it.
static int watch_run(Launched* L, const char* script, int timeout, const char* key3d, int hadPrev3d, DWORD prev3d,
	RegVals* prevTest3dP, const char* logs, const char* logpath)
{
	if (script) load_msgbox_rules(script);   // register dialog answers before any dialog can appear
	HANDLE ath=CreateThread(NULL,0,audio_thread,NULL,0,NULL);
	DriverArg da; ZeroMemory(&da,sizeof(da)); if(script) lstrcpynA(da.script,script,MAX_PATH);
	HANDLE dth=CreateThread(NULL,0,driver_thread,&da,0,NULL);
	HANDLE gth=CreateThread(NULL,0,dialog_thread,NULL,0,NULL);   // log/answer message boxes on the game desktop
	HANDLE mth=gMode3dExpr[0] ? CreateThread(NULL,0,mode3d_thread,NULL,0,NULL) : NULL;   // the game's 3D mode
	HANDLE wth=gWatchN ? CreateThread(NULL,0,watch_thread,NULL,0,NULL) : NULL;   // --watch

	// Wait for the game (rundesk returns when the whole job ends; inside the sandbox, origrun's own job), or hit the
	// timeout.
	if (wait_game(L,timeout)) {
		logline("timeout (%ds) -- ending the run (the job kills the game and its children)",timeout);
		end_game(L);
	} else logline("game exited");

	InterlockedExchange(&gStop,1);
	WaitForSingleObject(dth,3000);
	WaitForSingleObject(gth,3000);
	WaitForSingleObject(ath,3000);
	if (mth) WaitForSingleObject(mth,3000);
	if (wth) WaitForSingleObject(wth,3000);
	close_game(L);

	// What the game left in its setting (it writes Is3D at exit, and turns it off after a fallback), then the earlier
	// value and test results back.
	if (key3d[0]) {
		DWORD after=0; int hasAfter=reg_get_is3d(key3d,&after);
		if (hasAfter!=hadPrev3d || (hasAfter && after!=prev3d) || gMode3dWant>=0) {
			char was[32]; if (hasAfter) wsprintfA(was,"%lu",after); else lstrcpyA(was,"unset");
			if (reg_set_is3d(key3d,hadPrev3d,prev3d)) {
				if (hadPrev3d) logline("3D: the game left %s=%s; restored %s=%lu", kIs3DValue, was, kIs3DValue, prev3d);
				else logline("3D: the game left %s=%s; removed it again (it was unset)", kIs3DValue, was);
			}
			else logline("WARNING: could not restore HKCU\\%s\\%s (err %lu)", key3d, kIs3DValue, GetLastError());
		}
		restore_test3d(key3d,prevTest3dP);
	}

	logline("done. captures in %s, logs in %s", gShotDir, logs);
	logline(gAudioSeen ? "RESULT: an audio session appeared and was muted (see warning above)."
	                   : "RESULT: no audio session from the game was seen.");
	int rc=0;
	if (gMode3dExpr[0]) {
		LONG m=gMode3dLast;
		const char* flips = gMode3dFlips ? " (it changed during the run; see the 3D mode lines above)" : "";
		if (gMode3dWant>=0) {
			if (m<0) { logline("RESULT: 3D CHECK FAILED: asked for 3D %s, but the game's 3D flag (%s) was never read", gMode3dWant?"on":"off", gMode3dExpr); rc=3; }
			else if (m!=gMode3dWant) { logline("RESULT: 3D MISMATCH: asked for 3D %s, but the game ran with 3D %s%s", gMode3dWant?"on":"off", m?"on":"off", flips); rc=3; }
			else logline("RESULT: 3D %s, as asked%s", m?"on":"off", flips);
		} else if (m>=0) logline("RESULT: 3D %s (no --3d: the game's own setting)%s", m?"on":"off", flips);
		if (rc) fprintf(stderr,"origrun: the game did not run in the 3D mode asked for (see %s)\n", logpath);
	}
	return rc;
}

// ------------------------------------------------------------------------------------------ Windows Sandbox

// SMBIOS's system manufacturer and product (structure type 1), as the firmware gives them to the kernel.
static void smbios_string(const BYTE* str, const BYTE* end, int idx, char* out, int cap)
{
	out[0]=0; if (idx<=0) return;
	const BYTE* q=str;
	for (int i=1; q<end && *q; i++) {
		const BYTE* z=q; while (z<end && *z) z++;
		if (i==idx) { int n=(int)(z-q); if (n>=cap) n=cap-1; memcpy(out,q,n); out[n]=0; return; }
		q=z+1;
	}
}
static int smbios_system(char* manu, char* prod, int cap)
{
	manu[0]=prod[0]=0;
	UINT n=GetSystemFirmwareTable('RSMB',0,NULL,0); if (n<8) return 0;
	BYTE* b=(BYTE*)malloc(n); if (!b) return 0;
	if (GetSystemFirmwareTable('RSMB',0,b,n)!=n) { free(b); return 0; }
	DWORD len=*(DWORD*)(b+4); const BYTE* d=b+8; const BYTE* end=d+len; if (end>b+n) end=b+n;
	int ok=0;
	while (d+4<=end) {
		BYTE type=d[0], hl=d[1]; if (hl<4 || d+hl>end) break;
		const BYTE* str=d+hl;
		if (type==1 && hl>=6) { smbios_string(str,end,d[4],manu,cap); smbios_string(str,end,d[5],prod,cap); ok=1; break; }
		const BYTE* q=str; while (q+1<end && !(q[0]==0 && q[1]==0)) q++;
		d=q+2;
		if (type==127) break;
	}
	free(b);
	return ok;
}
// Is this Windows Sandbox? Three things a run on the host cannot fake by an option or an environment variable must
// all hold: the firmware reports a Hyper-V virtual machine (SMBIOS "Microsoft Corporation" / "Virtual Machine"; the
// host here is a physical MSI board), the local account WDAGUtilityAccount, the sandbox's own user (disabled on the
// host), is enabled, and origrun runs as that account or as SYSTEM (what "wsb exec -r System" gives), by its token.
static int detect_windows_sandbox(char* why, int cap)
{
	char manu[128], prod[128];
	int fw = smbios_system(manu,prod,sizeof(manu));
	int vm = fw && !strcmp(manu,"Microsoft Corporation") && !strcmp(prod,"Virtual Machine");
	int wdag=0; USER_INFO_1* ui=NULL;
	if (NetUserGetInfo(NULL,L"WDAGUtilityAccount",1,(LPBYTE*)&ui)==NERR_Success && ui) { wdag = !(ui->usri1_flags & UF_ACCOUNTDISABLE); NetApiBufferFree(ui); }
	char user[128]="?"; int userOk=0;
	HANDLE tok=NULL;
	if (OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&tok)) {
		BYTE buf[256]; DWORD n=0;
		if (GetTokenInformation(tok,TokenUser,buf,sizeof(buf),&n)) {
			PSID sid=((TOKEN_USER*)buf)->User.Sid;
			char name[128], dom[128]; DWORD nn=sizeof(name), dn=sizeof(dom); SID_NAME_USE use;
			if (IsWellKnownSid(sid,WinLocalSystemSid)) { lstrcpyA(user,"SYSTEM"); userOk=1; gAsSystem=1; }
			else if (LookupAccountSidA(NULL,sid,name,&nn,dom,&dn,&use)) { lstrcpynA(user,name,sizeof(user)); userOk=ieq(name,"WDAGUtilityAccount"); }
		}
		CloseHandle(tok);
	}
	_snprintf(why,cap-1,"SMBIOS %s / %s, WDAGUtilityAccount %s, running as %s",
		fw?manu:"(unreadable)", fw?prod:"(unreadable)", wdag?"enabled":"disabled or absent", user);
	why[cap-1]=0;
	return vm && wdag && userOk;
}

// One sandbox at a time on the whole machine: every session's origrun opens the same lock file, under the user's
// local application data folder as Windows reports it (not %LOCALAPPDATA%, which a shell can change), with no
// sharing. Windows closes the handle when origrun ends, however it ends, so the lock itself is never left behind.
// Its text says who holds it and, from just before "wsb start" until "wsb list" confirms the stop, which sandbox the
// run started (sandbox=<id>). An origrun killed in between (TerminateProcess runs no handler) leaves that line, and
// the next run to take the lock stops that sandbox before it starts its own (recover_stale_sandbox).
static HANDLE gSbLock = INVALID_HANDLE_VALUE;
static char gSbLockRun[80] = "", gSbLockSince[32] = "";
static char gStaleId[64] = "";    // a sandbox an earlier run left, from the lock's text
static DWORD gStalePid = 0;       // the pid of that run
static void lock_write(const char* sandboxId)
{
	if (gSbLock==INVALID_HANDLE_VALUE) return;
	char info[1024];
	int n=_snprintf(info,sizeof(info)-1,"pid=%lu\r\nrun=%s\r\nsince=%s\r\ndir=%s\r\n",GetCurrentProcessId(),gSbLockRun,gSbLockSince,gRunDir);
	if (n>0 && sandboxId && sandboxId[0]) { int m=_snprintf(info+n,sizeof(info)-1-n,"sandbox=%s\r\n",sandboxId); if (m>0) n+=m; }
	DWORD wr=0; SetFilePointer(gSbLock,0,NULL,FILE_BEGIN); SetEndOfFile(gSbLock);
	if (n>0) WriteFile(gSbLock,info,(DWORD)n,&wr,NULL);
	FlushFileBuffers(gSbLock);
}
static int is_guid36(const char* s)
{
	for (int i=0;i<36;i++) {
		char c=s[i];
		if (i==8||i==13||i==18||i==23) { if (c!='-') return 0; }
		else if (!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) return 0;
	}
	return s[36]==0 || s[36]=='\r' || s[36]=='\n';
}
static void release_sandbox_lock(void){ if (gSbLock!=INVALID_HANDLE_VALUE) { CloseHandle(gSbLock); gSbLock=INVALID_HANDLE_VALUE; } }
static int acquire_sandbox_lock(int waitSec, const char* run)
{
	PWSTR la=NULL;
	if (SHGetKnownFolderPath(&FOLDERID_LocalAppData,0,NULL,&la)!=S_OK || !la) { logline("--sandbox: no local application data folder"); return 0; }
	WCHAR dir[MAX_PATH], path[MAX_PATH];
	_snwprintf(dir,MAX_PATH-1,L"%s\\byo-pop",la); dir[MAX_PATH-1]=0;
	CoTaskMemFree(la);
	CreateDirectoryW(dir,NULL);
	_snwprintf(path,MAX_PATH-1,L"%s\\sandbox.lock",dir); path[MAX_PATH-1]=0;
	for (int waited=0;; waited++) {
		HANDLE h=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (h!=INVALID_HANDLE_VALUE) {
			char old[1024]; DWORD rd=0; if (!ReadFile(h,old,sizeof(old)-1,&rd,NULL)) rd=0; old[rd]=0;
			const char* q=strstr(old,"sandbox=");
			gStaleId[0]=0; gStalePid=0;
			if (q && is_guid36(q+8)) { lstrcpynA(gStaleId,q+8,37); const char* pp=strstr(old,"pid="); if (pp) gStalePid=strtoul(pp+4,NULL,10); }
			SYSTEMTIME st; GetLocalTime(&st);
			wsprintfA(gSbLockSince,"%04d-%02d-%02d %02d:%02d:%02d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
			lstrcpynA(gSbLockRun,run,sizeof(gSbLockRun));
			gSbLock=h;
			lock_write(gStaleId);   // an earlier run's sandbox stays named until it is confirmed gone
			logline("holding the Windows Sandbox lock %ls%s", path, waited?" (after waiting)":"");
			if (gStaleId[0]) logline("the lock names the sandbox %s, which the run of pid %lu started and never confirmed stopped", gStaleId, gStalePid);
			return 1;
		}
		DWORD e=GetLastError();
		if (e!=ERROR_SHARING_VIOLATION && e!=ERROR_LOCK_VIOLATION) { logline("--sandbox: cannot open the lock %ls (err %lu)", path, e); return 0; }
		if (waited==0) logline("another run holds the Windows Sandbox lock %ls; queued, waiting up to %d s", path, waitSec);
		else if (waited%60==0) logline("  still waiting for the sandbox lock (%d s)", waited);
		if (waited>=waitSec) { logline("--sandbox: the lock was not free within %d s; giving up", waitSec); return 0; }
		Sleep(1000);
	}
}

static char gWsbPath[MAX_PATH] = "";
static char gWsbLog[MAX_PATH] = "";
static char gSandboxId[64] = "";
// wsb.exe is 64-bit and origrun 32-bit: System32 is reached as Sysnative, and Windows 11 may also give the command
// as an app execution alias under the user's WindowsApps folder.
static int find_wsb(void)
{
	char p[MAX_PATH]; char win[MAX_PATH]; GetWindowsDirectoryA(win,MAX_PATH);
	wsprintfA(p,"%s\\Sysnative\\wsb.exe",win);
	if (GetFileAttributesA(p)==INVALID_FILE_ATTRIBUTES) wsprintfA(p,"%s\\System32\\wsb.exe",win);
	if (GetFileAttributesA(p)==INVALID_FILE_ATTRIBUTES) {
		PWSTR la=NULL; p[0]=0;
		if (SHGetKnownFolderPath(&FOLDERID_LocalAppData,0,NULL,&la)==S_OK && la) {
			WideCharToMultiByte(CP_ACP,0,la,-1,p,MAX_PATH-40,NULL,NULL); CoTaskMemFree(la);
			lstrcatA(p,"\\Microsoft\\WindowsApps\\wsb.exe");
		}
		if (!p[0] || GetFileAttributesA(p)==INVALID_FILE_ATTRIBUTES) return 0;
	}
	lstrcpynA(gWsbPath,p,MAX_PATH);
	logline("wsb: %s", gWsbPath);
	return 1;
}
// Run wsb.exe with no console window, its output appended to gWsbLog. wait=0 returns the process in *proc. Each call
// builds its command line in a buffer of its own (on the stack, or allocated for a long configuration), since the
// main thread, the guard, the Ctrl+C handler and the crash filter may each run wsb.
static int run_wsb(const char* args, int wait, DWORD waitMs, DWORD* exitCode, HANDLE* proc)
{
	char small[640]; char* cmd=small; size_t need=strlen(gWsbPath)+strlen(args)+4;
	if (need>sizeof(small) && (cmd=(char*)malloc(need))==NULL) { logline("could not run wsb %.80s (out of memory)", args); return 0; }
	sprintf(cmd,"\"%s\" %s",gWsbPath,args);
	SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};
	HANDLE out=CreateFileA(gWsbLog,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (out!=INVALID_HANDLE_VALUE) { char hdr[256]; int n=_snprintf(hdr,sizeof(hdr)-1,"---- wsb %.200s\r\n",args); DWORD wr=0; if (n>0) WriteFile(out,hdr,(DWORD)n,&wr,NULL); }
	STARTUPINFOA si; ZeroMemory(&si,sizeof(si)); si.cb=sizeof(si);
	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW; si.wShowWindow=SW_HIDE;
	si.hStdInput=NULL; si.hStdOutput=out; si.hStdError=out;
	PROCESS_INFORMATION pi; ZeroMemory(&pi,sizeof(pi));
	BOOL ok=CreateProcessA(NULL,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
	DWORD err=GetLastError();
	if (cmd!=small) free(cmd);
	if (out!=INVALID_HANDLE_VALUE) CloseHandle(out);
	if (!ok) { logline("could not run wsb %.80s (err %lu)", args, err); return 0; }
	CloseHandle(pi.hThread);
	if (!wait) { *proc=pi.hProcess; return 1; }
	DWORD w=WaitForSingleObject(pi.hProcess,waitMs);
	if (w==WAIT_TIMEOUT) { logline("wsb %.40s did not finish within %lu s", args, waitMs/1000); TerminateProcess(pi.hProcess,1); }
	DWORD ec=1; GetExitCodeProcess(pi.hProcess,&ec); if (exitCode) *exitCode=ec;
	CloseHandle(pi.hProcess);
	return w!=WAIT_TIMEOUT;
}
// Stops this run's sandbox, once: the main thread, the guard, the Ctrl+C handler and the crash filter may all ask.
// A stop that fails only warns here; the lock keeps the id for the next run to stop, and the sandbox's own shutdown
// timer (the dead-man's switch, main_in_sandbox) ends it anyway.
static volatile LONG gStopOnce = 0;
static void stop_sandbox(const char* why)
{
	if (!gSandboxId[0] || !gWsbPath[0]) return;
	if (InterlockedExchange(&gStopOnce,1)) return;
	char id[64]; lstrcpynA(id,gSandboxId,sizeof(id));
	char a[128]; wsprintfA(a,"stop --id %s",id);
	logline("stopping the sandbox %s (%s)", id, why);
	DWORD ec=1; int ran=run_wsb(a,1,60000,&ec,NULL);
	if (!ran || ec) logline("WARNING: wsb stop %s (exit %lu; see %s): the lock keeps the id for the next run to stop, and the sandbox shuts itself down on its own timer", ran?"failed":"could not be run", ec, gWsbLog);
	gSandboxId[0]=0;
}
static BOOL WINAPI sandbox_ctrl(DWORD type){ stop_sandbox("origrun was interrupted"); return FALSE; }
// The GUIDs wsb printed after the last "---- wsb <header>" line in gWsbLog: how many, and the last one in id.
static int wsb_guids(const char* header, char* id, int cap)
{
	FILE* f=fopen(gWsbLog,"r"); if (!f) return 0;
	static char text[65536]; size_t n=fread(text,1,sizeof(text)-1,f); fclose(f); text[n]=0;
	char hd[64]; wsprintfA(hd,"---- wsb %s",header);
	char* q=NULL; for (char* h=text; (h=strstr(h,hd))!=NULL; h++) q=h;
	if (!q) return 0;
	int found=0;
	for (; *q; q++) {
		int ok=1; static const int dash[]={8,13,18,23};
		for (int i=0;i<36 && ok;i++) {
			char c=q[i];
			int isDash = (i==dash[0]||i==dash[1]||i==dash[2]||i==dash[3]);
			if (isDash ? c!='-' : !((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) ok=0;
		}
		if (ok) { lstrcpynA(id,q,37<cap?37:cap); found++; q+=35; }
	}
	return found;
}

// A sandbox the lock still names: the run that started it ended without "wsb list" confirming the stop (it was
// killed, or its stop failed). That run has let the lock go, so it is ending or gone. Once it is gone, the sandbox is
// stopped by its id, and this run goes on only when wsb lists no sandbox any more.
static int recover_stale_sandbox(void)
{
	if (gStalePid && gStalePid!=GetCurrentProcessId()) {
		HANDLE h=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,gStalePid);
		if (h) {
			char p[MAX_PATH]; DWORD n=MAX_PATH; size_t l=0;
			int orig = QueryFullProcessImageNameA(h,0,p,&n) && (l=strlen(p))>=12 && !lstrcmpiA(p+l-12,"\\origrun.exe");
			if (orig && WaitForSingleObject(h,0)==WAIT_TIMEOUT) { logline("the run that left it (pid %lu) is still ending; waiting up to 30 s", gStalePid); WaitForSingleObject(h,30000); }
			CloseHandle(h);
		}
	}
	char a[128], id[64]="";
	wsprintfA(a,"stop --id %s",gStaleId);
	logline("stopping the sandbox %s, which an earlier run (pid %lu) started and never confirmed stopped", gStaleId, gStalePid);
	DWORD ec=1; run_wsb(a,1,60000,&ec,NULL);
	logline("wsb stop of the earlier run's sandbox exited %lu", ec);
	DWORD t=GetTickCount(); int listed;
	for (;;) {
		listed = run_wsb("list",1,60000,NULL,NULL) ? wsb_guids("list",id,sizeof(id)) : -1;
		if (!listed || GetTickCount()-t > 30000) break;
		Sleep(1000);
	}
	if (listed) { logline("--sandbox: wsb still lists a sandbox (%s) after the earlier run's was stopped; origrun starts none, and the lock keeps the id", listed>0?id:"wsb list failed"); return 0; }
	logline("the earlier run's sandbox is gone: wsb lists none");
	gStaleId[0]=0; gStalePid=0; lock_write(NULL);
	return 1;
}

// The sandbox must never show a window on the user's desktop. Docs say "wsb connect" is what opens one; this check
// watches for any visible window of a Windows Sandbox process, wsb's broker or vmconnect on the desktop the user
// sees, and the run stops its sandbox at once if one appears.
typedef struct { int found; char what[300]; } SbWin;
static BOOL CALLBACK sbwin_cb(HWND h, LPARAM lp)
{
	SbWin* c=(SbWin*)lp;
	if (!IsWindowVisible(h)) return TRUE;
	DWORD pid=0; GetWindowThreadProcessId(h,&pid);
	HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid); if (!p) return TRUE;
	char path[MAX_PATH]; DWORD n=MAX_PATH; int ok=QueryFullProcessImageNameA(p,0,path,&n); CloseHandle(p);
	if (!ok) return TRUE;
	const char* leaf=strrchr(path,'\\'); leaf=leaf?leaf+1:path;
	if (is_sandbox_leaf(leaf) || is_broker_leaf(leaf) || ieq(leaf,"vmconnect.exe")) {
		char t[128]="", cls[64]=""; GetWindowTextA(h,t,sizeof(t)); GetClassNameA(h,cls,sizeof(cls));
		RECT r; GetWindowRect(h,&r);
		_snprintf(c->what,sizeof(c->what)-1,"%s (pid %lu): '%s' class %s at %ld,%ld %ldx%ld",leaf,pid,t,cls,r.left,r.top,r.right-r.left,r.bottom-r.top); c->found=1; return FALSE;
	}
	return TRUE;
}
static int sandbox_window_shown(char* what, int cap)
{
	SbWin c; ZeroMemory(&c,sizeof(c));
	HDESK d=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS|DESKTOP_ENUMERATE);
	if (d) { EnumDesktopWindows(d,sbwin_cb,(LPARAM)&c); CloseDesktop(d); }
	if (!c.found) { d=OpenDesktopA("Default",0,FALSE,DESKTOP_READOBJECTS|DESKTOP_ENUMERATE); if (d) { EnumDesktopWindows(d,sbwin_cb,(LPARAM)&c); CloseDesktop(d); } }
	if (c.found) lstrcpynA(what,c.what,cap);
	return c.found;
}

// 0 when the escaped text does not fit: the run is then refused, never given a cut-off path.
static int xml_escape(const char* in, char* out, int cap)
{
	int o=0;
	for (; *in; in++) {
		char one[2]={*in,0}; const char* r=one;
		switch (*in) { case '&': r="&amp;"; break; case '<': r="&lt;"; break; case '>': r="&gt;"; break; case '"': r="&quot;"; break; case '\'': r="&apos;"; break; }
		int n=lstrlenA(r);
		if (o+n>=cap) { out[0]=0; return 0; }
		memcpy(out+o,r,n); o+=n;
	}
	out[o]=0;
	return 1;
}
static int read_marker_rc(const char* path, int* rc)
{
	FILE* f=fopen(path,"r"); if (!f) return 0;
	char ln[128]=""; int got = fgets(ln,sizeof(ln),f)!=NULL; fclose(f);
	if (!got) return 0;
	const char* q=strstr(ln,"rc="); *rc = q ? atoi(q+3) : 1;
	return 1;
}

// ------------------------------------------------------------------ the guard, and what the sandbox started

static DWORD gBeforePids[8192]; static int gBeforeN = 0;   // every process there was before the launch
static volatile LONG gGuardStop = 0, gGuardTrip = 0;
static int pid_before(DWORD pid){ for (int i=0;i<gBeforeN;i++) if (gBeforePids[i]==pid) return 1; return 0; }
static void snapshot_before(void)
{
	gBeforeN=0;
	HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); if (sn==INVALID_HANDLE_VALUE) return;
	PROCESSENTRY32 pe; pe.dwSize=sizeof(pe);
	for (BOOL ok=Process32First(sn,&pe); ok && gBeforeN<(int)(sizeof(gBeforePids)/sizeof(gBeforePids[0])); ok=Process32Next(sn,&pe)) gBeforePids[gBeforeN++]=pe.th32ProcessID;
	CloseHandle(sn);
}
// Any Windows Sandbox process running now (what=its name and pid). onlyNew: only those started since the snapshot.
// broker: count the broker too.
static int sandbox_procs(int onlyNew, char* what, int cap, int broker)
{
	int n=0; if (what) what[0]=0;
	HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); if (sn==INVALID_HANDLE_VALUE) return 0;
	PROCESSENTRY32 pe; pe.dwSize=sizeof(pe);
	for (BOOL ok=Process32First(sn,&pe); ok; ok=Process32Next(sn,&pe)) {
		if (!is_sandbox_leaf(pe.szExeFile)) continue;
		if (!broker && is_broker_leaf(pe.szExeFile)) continue;
		if (onlyNew && pid_before(pe.th32ProcessID)) continue;
		n++;
		char one[128]; wsprintfA(one,"%s%s %lu",n>1?", ":"",pe.szExeFile,pe.th32ProcessID);
		if (what && lstrlenA(what)+lstrlenA(one)<cap-1) lstrcatA(what,one);
	}
	CloseHandle(sn);
	return n;
}
// What the launch brought: every new process that is the sandbox's, a VM worker, or a child of one of those.
static void log_new_processes(const char* when)
{
	HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); if (sn==INVALID_HANDLE_VALUE) return;
	static PROCESSENTRY32 all[4096]; int n=0;
	PROCESSENTRY32 pe; pe.dwSize=sizeof(pe);
	for (BOOL ok=Process32First(sn,&pe); ok && n<4096; ok=Process32Next(sn,&pe)) if (!pid_before(pe.th32ProcessID)) all[n++]=pe;
	CloseHandle(sn);
	char line[1600]=""; int shown=0;
	for (int i=0;i<n;i++) {
		const char* x=all[i].szExeFile;
		int rel = is_sandbox_leaf(x) || istr_has(x,"vmwp") || istr_has(x,"vmmem");
		for (int j=0;j<n && !rel;j++) if (all[j].th32ProcessID==all[i].th32ParentProcessID && is_sandbox_leaf(all[j].szExeFile)) rel=1;
		if (!rel) continue;
		char one[160]; wsprintfA(one,"%s%s %lu (parent %lu)",shown?", ":"",x,all[i].th32ProcessID,all[i].th32ParentProcessID);
		if (lstrlenA(line)+lstrlenA(one)<(int)sizeof(line)-1) lstrcatA(line,one);
		shown++;
	}
	logline("%s: new processes of the sandbox: %s", when, shown?line:"none");
}
// The guard: from before "wsb start" to the end, every 100 ms, any visible window of a sandbox process, wsb's broker
// or vmconnect on the desktop the user sees stops this run's sandbox with "wsb stop --id", which closes its windows.
// The guard never ends a process itself: a window that is not this sandbox's costs only this run.
static DWORD WINAPI guard_thread(LPVOID p)
{
	while (!gGuardStop) {
		char what[300];
		if (sandbox_window_shown(what,sizeof(what))) {
			InterlockedExchange(&gGuardTrip,1);
			logline("STOP: a sandbox window was on the user's desktop: %s", what);
			stop_sandbox("a window showed on the user's desktop");
			break;
		}
		Sleep(100);
	}
	return 0;
}
static int file_there(const char* dir, const char* leaf){ char q[MAX_PATH]; joinp(q,dir,leaf); return GetFileAttributesA(q)!=INVALID_FILE_ATTRIBUTES; }
// origrun inside writes sandbox-started.txt when it begins and sandbox-done.txt when it ends, refusals included (a run
// whose checks fail inside writes only the done marker): either one means it ran.
static int inside_ran(void){ return file_there(gRunDir,"sandbox-started.txt") || file_there(gRunDir,"sandbox-done.txt"); }

static int sandbox_wsb_run(const char* cfg, int timeout, int probe)
{
	if (!find_wsb()) { logline("--sandbox: wsb.exe is not on this machine (Windows Sandbox and its wsb command, Windows 11 24H2 or later)"); return 2; }
	if (gStaleId[0] && !recover_stale_sandbox()) return 2;
	char what[1024], id[64];
	if (sandbox_procs(0,what,sizeof(what),0)) { logline("--sandbox: Windows Sandbox is already running (%s), and not for this run: origrun starts none while it does", what); return 2; }
	if (!run_wsb("list",1,60000,NULL,NULL)) return 2;
	if (wsb_guids("list",id,sizeof(id))) { logline("--sandbox: wsb list shows a running sandbox (%s), not this run's: origrun starts none while one runs", id); return 2; }

	int rc=2; HANDLE ex=NULL;
	char started[MAX_PATH], done[MAX_PATH]; joinp(started,gRunDir,"sandbox-started.txt"); joinp(done,gRunDir,"sandbox-done.txt");
	GUID g; WCHAR gw[64]; char ga[64];
	CoCreateGuid(&g); StringFromGUID2(&g,gw,64); WideCharToMultiByte(CP_ACP,0,gw+1,-1,ga,sizeof(ga),NULL,NULL);
	{ char* br=strchr(ga,'}'); if (br) *br=0; }
	lstrcpynA(gSandboxId,ga,sizeof(gSandboxId));   // set before start, so any later exit stops it
	gStopOnce=0;
	lock_write(gSandboxId);   // and named in the lock before it exists: a run killed from here on leaves its id there
	SetConsoleCtrlHandler(sandbox_ctrl,TRUE);
	snapshot_before();
	gGuardStop=0; gGuardTrip=0; gStop=0; gAudioSandbox=1; gSbAudioPeak=-1;
	HANDLE gt=CreateThread(NULL,0,guard_thread,NULL,0,NULL);   // watching before anything starts
	HANDLE at=CreateThread(NULL,0,audio_thread,NULL,0,NULL);
	static char args[8192];
	_snprintf(args,sizeof(args)-1,"start --id %s --config \"%s\"",gSandboxId,cfg); args[sizeof(args)-1]=0;
	logline("starting Windows Sandbox %s (no client window: origrun never runs wsb connect)", gSandboxId);
	DWORD t0=GetTickCount();
	{
		HANDLE sp=NULL; DWORD ec=1;
		if (!run_wsb(args,0,0,NULL,&sp)) goto out;
		while (WaitForSingleObject(sp,100)==WAIT_TIMEOUT) {
			if (gGuardTrip) { rc=4; TerminateProcess(sp,1); CloseHandle(sp); goto out; }
			if (GetTickCount()-t0 > 120000) { logline("wsb start did not finish within 120 s"); TerminateProcess(sp,1); WaitForSingleObject(sp,5000); CloseHandle(sp); goto out; }
		}
		GetExitCodeProcess(sp,&ec); CloseHandle(sp);
		if (ec!=0) { logline("wsb start failed (exit %lu, after %lu ms; see %s)", ec, GetTickCount()-t0, gWsbLog); goto out; }
		if (wsb_guids("start",id,sizeof(id)) && !ieq(id,gSandboxId)) { logline("wsb start gave the sandbox the id %s (asked for %s)", id, gSandboxId); lstrcpynA(gSandboxId,id,sizeof(gSandboxId)); lock_write(gSandboxId); }
		logline("wsb start returned after %lu ms", GetTickCount()-t0);
		log_new_processes("after wsb start");
	}
	if (gGuardTrip) { rc=4; goto out; }

	// Run origrun inside as SYSTEM: "wsb exec -r ExistingLogin" needs a user session, which only "wsb connect" (a
	// window) makes. exec is retried while the sandbox is still booting, until origrun inside says it has started.
	// The probe first writes a marker with cmd, to time the first command that runs inside.
	if (probe) _snprintf(args,sizeof(args)-1,"exec --id %s -c \"cmd.exe /c echo exec>C:\\run\\exec-marker.txt & C:\\origrun\\origrun.exe --in-sandbox C:\\run --probe\" -r System -d C:\\run",gSandboxId);
	else _snprintf(args,sizeof(args)-1,"exec --id %s -c \"C:\\origrun\\origrun.exe --in-sandbox C:\\run\" -r System -d C:\\run",gSandboxId);
	{
		DWORD t1=GetTickCount(); int up=0, sawExec=0, tries=0;
		while (!up && !gGuardTrip && GetTickCount()-t1 < 90000) {
			if (!run_wsb(args,0,0,NULL,&ex)) goto out;
			tries++;
			for (;;) {
				if (probe && !sawExec && file_there(gRunDir,"exec-marker.txt")) { sawExec=1; logline("the exec marker appeared %lu ms after wsb start began (exec try %d)", GetTickCount()-t0, tries); }
				if (inside_ran()) { up=1; break; }
				if (gGuardTrip) { rc=4; goto out; }
				if (GetTickCount()-t1 >= 90000) break;
				if (WaitForSingleObject(ex,100)==WAIT_OBJECT_0) {
					if (inside_ran()) { up=1; break; }
					DWORD xc=0; GetExitCodeProcess(ex,&xc); CloseHandle(ex); ex=NULL;
					logline("wsb exec returned %lu before origrun started inside; retrying", xc);
					Sleep(3000); break;
				}
			}
		}
		if (gGuardTrip) { rc=4; goto out; }
		if (!up) { logline("origrun did not start inside the sandbox within 90 s (see %s)", gWsbLog); goto out; }
		if (file_there(gRunDir,"sandbox-started.txt")) logline("origrun started inside %lu ms after wsb start began; its log is %s\\logs\\origrun-sandbox.log", GetTickCount()-t0, gRunDir);
		else logline("origrun inside ended before its run began (only its done marker is there), %lu ms after wsb start began", GetTickCount()-t0);
		DWORD t2=GetTickCount(), limit=(DWORD)(probe ? 30 : timeout+60)*1000;
		while (!file_there(gRunDir,"sandbox-done.txt")) {
			if (GetTickCount()-t2 > limit) { logline("no done marker within %lu s; stopping the sandbox", limit/1000); goto out; }
			if (gGuardTrip) { rc=4; goto out; }
			Sleep(100);
		}
		Sleep(300);   // let the marker's writer close it
		if (!read_marker_rc(done,&rc)) rc=2;
		logline("the run inside the sandbox ended with code %d", rc);
	}
out:
	{
		char sid[64]; lstrcpynA(sid,gSandboxId,sizeof(sid));
		log_new_processes("at the stop");
		DWORD ts=GetTickCount();
		stop_sandbox(rc==4 ? "a window showed" : "the run is over");
		logline("wsb stop took %lu ms", GetTickCount()-ts);
		if (ex) {   // wsb exec ends with the sandbox; never leave it behind
			if (WaitForSingleObject(ex,10000)==WAIT_TIMEOUT) { logline("wsb exec still ran after the stop; ending it"); TerminateProcess(ex,1); WaitForSingleObject(ex,5000); }
			CloseHandle(ex); ex=NULL;
		}
		// Confirm nothing is left: wsb lists no sandbox, and no sandbox process started since the launch runs.
		int listed=0, left=0, again=0; DWORD tc=GetTickCount();
		do {
			listed = run_wsb("list",1,60000,NULL,NULL) ? wsb_guids("list",id,sizeof(id)) : -1;
			left = sandbox_procs(1,what,sizeof(what),0);
			if (listed>0 && !again && sid[0] && GetTickCount()-tc > 10000) {   // a stop that did not take: once more
				char a[128]; wsprintfA(a,"stop --id %s",sid); again=1;
				logline("wsb still lists a sandbox 10 s after the stop; stopping %s again", sid);
				run_wsb(a,1,60000,NULL,NULL);
			}
			if (listed || left) Sleep(1000);
		} while ((listed || left) && GetTickCount()-tc < 30000);
		if (!listed) lock_write(NULL);   // confirmed gone: the lock no longer names it
		else logline("the lock keeps the sandbox's id %s: the next run stops it first, and the sandbox's own timer ends it", sid);
		if (listed>0) logline("WARNING: wsb list still shows a sandbox (%s) 30 s after the stop", id);
		else if (listed<0) logline("WARNING: wsb list could not be run after the stop");
		if (left) logline("WARNING: sandbox processes still run 30 s after the stop: %s", what);
		if (!listed && !left) logline("nothing is left: wsb lists no sandbox and no sandbox process started by the run is running (%lu ms after the stop)", GetTickCount()-tc);
		if (!listed && !left) {   // the broker stays: it is no sandbox, and nothing names it as this run's
			char b[512]=""; HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
			if (sn!=INVALID_HANDLE_VALUE) {
				PROCESSENTRY32 pe; pe.dwSize=sizeof(pe);
				for (BOOL ok=Process32First(sn,&pe); ok; ok=Process32Next(sn,&pe)) if (is_broker_leaf(pe.szExeFile)) {
					char one[96]; int ours=!pid_before(pe.th32ProcessID);
					wsprintfA(one,"%lu (%s)",pe.th32ProcessID,ours?"started during this run":"running before it");
					if (lstrlenA(b)+lstrlenA(one)<(int)sizeof(b)-3) { if (b[0]) lstrcatA(b,"; "); lstrcatA(b,one); }
				}
				CloseHandle(sn);
			}
			if (b[0]) logline("wsb's broker WindowsSandboxServer.exe is running, not a sandbox: %s", b);
		}
		log_new_processes("after the stop");
		(void)sid;
	}
	InterlockedExchange(&gGuardStop,1); if (gt) { WaitForSingleObject(gt,3000); CloseHandle(gt); }
	InterlockedExchange(&gStop,1); if (at) { WaitForSingleObject(at,3000); CloseHandle(at); }
	logline(gGuardTrip ? "RESULT: a sandbox window showed on the user's desktop; the run was stopped" : "RESULT: no sandbox window showed on the user's desktop");
	if (gSbAudioPeak<0) logline("RESULT: no audio session of the sandbox appeared on the host");
	else logline("RESULT: the sandbox had an audio session on the host; loudest peak %ld/1000%s", (long)gSbAudioPeak, gSbAudioPeak>0 ? " (sound reached the host)" : " (silent)");
	SetConsoleCtrlHandler(sandbox_ctrl,FALSE);
	return gGuardTrip ? 4 : rc;
}

// --reg <file>: registry presets for a game run inside the sandbox (see import_registry).
static const char* gRegArg = NULL;

// A final path (junctions and links resolved) as a host path for the configuration: no \\?\ prefix, never a network
// path, and only when its ANSI form is exact.
static int final_host_path(const char* in, char* out, int cap)
{
	WCHAR f[MAX_PATH+8]; if (!final_w(in,f,MAX_PATH+8)) return 0;
	const WCHAR* p=f;
	if (lstrlenW(p)>=8 && !memcmp(p,L"\\\\?\\UNC\\",8*sizeof(WCHAR))) return 0;
	if (lstrlenW(p)>=4 && !memcmp(p,L"\\\\?\\",4*sizeof(WCHAR))) p+=4;
	if (!(p[0] && p[1]==L':' && p[2]==L'\\')) return 0;   // a drive path only
	BOOL lossy=FALSE;
	if (!WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,p,-1,out,cap,NULL,&lossy) || lossy) return 0;
	return 1;
}
// What the sandbox maps, checked on the final paths: the run folder must sit directly under <repo>\build\origrun,
// and the binaries must be origrun's own build\origrun-bin. The configuration names those final paths, so what is
// mapped is what was checked.
static int mapped_folders_ok(const char* selfDir, char* runHost, char* binHost)
{
	char root[MAX_PATH], rootHost[MAX_PATH]; _snprintf(root,sizeof(root)-1,"%s\\build\\origrun",gRepo); root[sizeof(root)-1]=0;
	if (!final_host_path(gRunDir,runHost,MAX_PATH) || !final_host_path(root,rootHost,MAX_PATH)) { logline("--sandbox: cannot resolve the run folder %s or %s to a local path; refusing to map it", gRunDir, root); return 0; }
	size_t rl=strlen(rootHost);
	if (_strnicmp(runHost,rootHost,rl)!=0 || runHost[rl]!='\\' || !runHost[rl+1] || strchr(runHost+rl+1,'\\')) { logline("--sandbox: the run folder %s is not directly under %s; refusing to map it", runHost, rootHost); return 0; }
	if (!final_host_path(selfDir,binHost,MAX_PATH)) { logline("--sandbox: cannot resolve origrun's folder %s to a local path; refusing to map it", selfDir); return 0; }
	static const char tail[]="\\build\\origrun-bin"; size_t bl=strlen(binHost), tl=sizeof(tail)-1;
	if (bl<=tl || lstrcmpiA(binHost+bl-tl,tail)!=0) { logline("--sandbox: origrun runs from %s, not from a build\\origrun-bin folder; refusing to map it", binHost); return 0; }
	return 1;
}

// --sandbox, on the host: the copy, the script and origrun's binaries are staged; the run happens inside a fresh
// Windows Sandbox that sees only the run folder (read-write, as C:\run) and origrun's binaries (read-only, as
// C:\origrun), and the shots and the sandbox's log land in the run folder. probe: no game; origrun inside only
// writes what it finds there to probe.txt.
static int sandbox_host_run(const char* selfDir, const char* script, int timeout, const char* sound,
	const char* key3d, int vgpu, int waitSec, int probe)
{
	const char* exeRel="";
	if (!probe) {
		size_t rl=strlen(gRunDir);
		if (_strnicmp(gExePath,gRunDir,rl)!=0 || gExePath[rl]!='\\') { logline("--sandbox: the exe %s is not inside the run folder %s", gExePath, gRunDir); return 2; }
		exeRel=gExePath+rl+1;
	}
	char logs[MAX_PATH]; joinp(logs,gRunDir,"logs"); joinp(gWsbLog,logs,"wsb.txt");

	// What the run inside needs: sandbox-run.txt, read by "origrun --in-sandbox C:\run".
	char p[MAX_PATH];
	if (script) { joinp(p,gRunDir,"script.txt"); if (!CopyFileA(script,p,FALSE)) { logline("--sandbox: cannot copy the script %s (err %lu)", script, GetLastError()); return 2; } }
	joinp(p,gRunDir,"sandbox-run.txt");
	FILE* rf=fopen(p,"w"); if (!rf) { logline("--sandbox: cannot write %s", p); return 2; }
	if (probe) fprintf(rf,"probe=1\n");
	else fprintf(rf,"exe=%s\ntimeout=%d\nsound=%s\nupdate_ms=%d\n", exeRel, timeout, sound, gUpdateMs);
	if (script) fprintf(rf,"script=script.txt\n");
	if (gCounterExpr[0]) fprintf(rf,"counter=%s\n",gCounterExpr);
	if (gFlipsExpr[0]) fprintf(rf,"flips=%s\n",gFlipsExpr);
	if (gPendingExpr[0]) fprintf(rf,"pending=%s\n",gPendingExpr);
	if (gStateExpr[0]) fprintf(rf,"state=%s\n",gStateExpr);
	if (gEmptyExpr[0]) fprintf(rf,"empty=%s\n",gEmptyExpr);
	for (int i=0;i<gWatchN;i++) fprintf(rf,"watch=%s\n",gWatchExpr[i]);
	if (gMode3dWant>=0) fprintf(rf,"3d=%s\n",gMode3dWant?"on":"off");
	if (key3d && key3d[0]) fprintf(rf,"key3d=%s\n",key3d);
	if (gMode3dExpr[0]) fprintf(rf,"flag3d=%s\n",gMode3dExpr);
	fclose(rf);
	// --reg: registry presets, imported inside before the game starts, into the hive the game uses there.
	if (!probe) {
		char sfd[MAX_PATH], rg[MAX_PATH]; joinp(sfd,gRunDir,"sandbox-files"); joinp(rg,sfd,"registry.reg");
		if (gRegArg) {
			mkdirs(sfd);
			if (!CopyFileA(gRegArg,rg,FALSE)) { logline("--reg: cannot copy %s to %s (err %lu)", gRegArg, rg, GetLastError()); return 2; }
			logline("--reg: staged %s as %s; it is imported inside the sandbox before the game starts", gRegArg, rg);
		}
		else if (GetFileAttributesA(rg)!=INVALID_FILE_ATTRIBUTES) logline("registry presets: %s is imported inside the sandbox before the game starts", rg);
	}
	static const char* const markers[]={"sandbox-started.txt","sandbox-done.txt","exec-marker.txt","probe.txt"};
	for (int i=0;i<4;i++) { joinp(p,gRunDir,markers[i]); if (GetFileAttributesA(p)!=INVALID_FILE_ATTRIBUTES && !delete_guarded(p)) { logline("--sandbox: cannot clear %s", p); return 2; } }

	// The configuration: no network, clipboard, printers, microphone or camera; the RDP client in AppContainer
	// isolation; software rendering unless --sandbox-vgpu; and only the two folders.
	char runHost[MAX_PATH], binHost[MAX_PATH];
	if (!mapped_folders_ok(selfDir,runHost,binHost)) return 2;
	char runX[MAX_PATH*6], binX[MAX_PATH*6];
	if (!xml_escape(runHost,runX,sizeof(runX)) || !xml_escape(binHost,binX,sizeof(binX))) { logline("--sandbox: a mapped folder's path does not fit the configuration; refusing the run"); return 2; }
	static char cfg[4096];
	int cfgN=_snprintf(cfg,sizeof(cfg)-1,
		"<Configuration><vGPU>%s</vGPU><Networking>Disable</Networking>"
		"<MappedFolders>"
		"<MappedFolder><HostFolder>%s</HostFolder><SandboxFolder>C:\\run</SandboxFolder><ReadOnly>false</ReadOnly></MappedFolder>"
		"<MappedFolder><HostFolder>%s</HostFolder><SandboxFolder>C:\\origrun</SandboxFolder><ReadOnly>true</ReadOnly></MappedFolder>"
		"</MappedFolders>"
		"<AudioInput>Disable</AudioInput><VideoInput>Disable</VideoInput><ProtectedClient>Enable</ProtectedClient>"
		"<PrinterRedirection>Disable</PrinterRedirection><ClipboardRedirection>Disable</ClipboardRedirection>"
		"</Configuration>", vgpu?"Enable":"Disable", runX, binX);
	cfg[sizeof(cfg)-1]=0;
	if (cfgN<0 || cfgN>=(int)sizeof(cfg)-1) { logline("--sandbox: the configuration does not fit its buffer; refusing the run"); return 2; }
	char wsbFile[MAX_PATH]; joinp(wsbFile,gRunDir,"sandbox.wsb");
	FILE* wf=fopen(wsbFile,"w"); if (!wf) { logline("--sandbox: cannot write %s", wsbFile); return 2; }
	fputs(cfg,wf); fputs("\n",wf); fclose(wf);
	logline("sandbox configuration in %s: vGPU %s, networking, clipboard, printers, audio and video input off, protected client on; "
		"maps %s (read-write, C:\\run) and %s (read-only, C:\\origrun)%s", wsbFile, vgpu?"on":"off", runHost, binHost, probe?"; a probe, no game":"");

	const char* run=strrchr(gRunDir,'\\'); run=run?run+1:gRunDir;
	if (!acquire_sandbox_lock(waitSec, run)) return 2;
	int rc = sandbox_wsb_run(cfg,timeout,probe);
	release_sandbox_lock();   // the next queued run may start its sandbox
	logline("released the Windows Sandbox lock");
	{   // the inside run's verdicts, for this log
		char ip[MAX_PATH]; wsprintfA(ip,"%s\\origrun-sandbox.log",logs);
		FILE* f=fopen(ip,"r"); char ln[1024];
		if (f) { while (fgets(ln,sizeof(ln),f)) if (strstr(ln,"RESULT") || strstr(ln,"refused") || strstr(ln,"crashed")) { char* e=strpbrk(ln,"\r\n"); if(e)*e=0; logline("  inside: %s", ln); } fclose(f); }
		joinp(ip,gRunDir,"probe.txt");
		if (probe && (f=fopen(ip,"r"))!=NULL) { while (fgets(ln,sizeof(ln),f)) { char* e=strpbrk(ln,"\r\n"); if(e)*e=0; logline("  probe: %s", ln); } fclose(f); }
	}
	return rc;
}

// wsb exec -r System starts origrun in session 0, where nothing is ever on a display. If the sandbox has a user
// session (WDAGUtilityAccount logged on), origrun runs itself again there, as that user, and the game with it;
// otherwise the run stays in session 0. 1 and the inner run's code when it ran in a user session.
static int run_in_user_session(const char* dir, int timeout, int* rc)
{
	DWORD me=0; if (!ProcessIdToSessionId(GetCurrentProcessId(),&me) || me!=0) return 0;
	WTS_SESSION_INFOA* si=NULL; DWORD n=0; HANDLE tok=NULL; DWORD sid=0;
	if (!WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE,0,1,&si,&n)) { logline("sessions: cannot list them (err %lu)", GetLastError()); return 0; }
	for (DWORD i=0;i<n;i++) {
		HANDLE t=NULL; int ok = si[i].SessionId!=0 && WTSQueryUserToken(si[i].SessionId,&t);
		DWORD err = ok ? 0 : GetLastError();
		logline("session %lu ('%s', state %d): %s", si[i].SessionId, si[i].pWinStationName?si[i].pWinStationName:"", (int)si[i].State,
			si[i].SessionId==0 ? "services" : ok ? "a user is logged on" : (err==ERROR_NO_TOKEN ? "no user logged on" : "no token"));
		if (ok && !tok) { tok=t; sid=si[i].SessionId; } else if (ok) CloseHandle(t);
	}
	WTSFreeMemory(si);
	if (!tok) { logline("no user session in the sandbox: the game runs in session 0, as SYSTEM"); return 0; }
	char self[MAX_PATH]; GetModuleFileNameA(NULL,self,MAX_PATH);
	char cmd[MAX_PATH*3]; wsprintfA(cmd,"\"%s\" --in-sandbox \"%s\" --in-session",self,dir);
	void* env=NULL; CreateEnvironmentBlock(&env,tok,FALSE);
	STARTUPINFOA st; ZeroMemory(&st,sizeof(st)); st.cb=sizeof(st); st.lpDesktop=(LPSTR)"WinSta0\\Default";
	PROCESS_INFORMATION pi; ZeroMemory(&pi,sizeof(pi));
	HANDLE job=CreateJobObjectA(NULL,NULL);
	if (job) { JOBOBJECT_EXTENDED_LIMIT_INFORMATION li; ZeroMemory(&li,sizeof(li)); li.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE; SetInformationJobObject(job,JobObjectExtendedLimitInformation,&li,sizeof(li)); }
	if (gLogFile) fflush(gLogFile);
	BOOL ok=CreateProcessAsUserA(tok,NULL,cmd,NULL,NULL,FALSE,CREATE_SUSPENDED|CREATE_NO_WINDOW|(env?CREATE_UNICODE_ENVIRONMENT:0),env,dir,&st,&pi);
	DWORD err=GetLastError();
	if (env) DestroyEnvironmentBlock(env);
	CloseHandle(tok);
	if (!ok) { logline("could not start origrun in session %lu (err %lu): the game runs in session 0, as SYSTEM", sid, err); if (job) CloseHandle(job); return 0; }
	if (job) AssignProcessToJobObject(job,pi.hProcess);
	logline("running the game in session %lu, as its user (origrun pid %lu there; its lines follow)", sid, pi.dwProcessId);
	ResumeThread(pi.hThread);
	DWORD w=WaitForSingleObject(pi.hProcess,(DWORD)(timeout+30)*1000);
	if (w==WAIT_TIMEOUT) { logline("origrun in session %lu did not end in time; ending it", sid); if (job) TerminateJobObject(job,1); else TerminateProcess(pi.hProcess,1); WaitForSingleObject(pi.hProcess,5000); }
	DWORD ec=2; GetExitCodeProcess(pi.hProcess,&ec);
	CloseHandle(pi.hThread); CloseHandle(pi.hProcess); if (job) CloseHandle(job);
	*rc = w==WAIT_TIMEOUT ? 2 : (int)ec;
	return 1;
}

// --in-sandbox <dir>: origrun inside Windows Sandbox (started there by --sandbox through wsb exec). It runs only when
// it finds for itself that it is in the sandbox, reads the run from <dir>\sandbox-run.txt, puts the files the run
// staged under <dir>\sandbox-files\C onto the sandbox's C:, runs the game on the sandbox's own desktop and writes
// <dir>\sandbox-done.txt at the end.
// What the sandbox offers a game, for --probe: the session and desktop, the display and its modes, and whether
// DirectDraw has hardware and can take the screen full-screen at 800x600x16, as Water Bugs asks. Only ever called
// inside the sandbox (the mode change would change the user's display anywhere else).
typedef HRESULT (WINAPI *DDCreateFn)(GUID*, LPDIRECTDRAW*, IUnknown*);
static void probe_ddraw(FILE* o, const char* when)
{
	fprintf(o,"DirectDraw, %s:\n", when);
	HMODULE m=LoadLibraryA("ddraw.dll"); DDCreateFn f = m ? (DDCreateFn)GetProcAddress(m,"DirectDrawCreate") : NULL;
	if (!f) { fprintf(o,"  cannot load DirectDrawCreate\n"); return; }
	LPDIRECTDRAW dd=NULL; HRESULT hr=f(NULL,&dd,NULL);
	if (FAILED(hr) || !dd) { fprintf(o,"  DirectDrawCreate: 0x%08lx\n",(unsigned long)hr); return; }
	DDCAPS hal, hel; ZeroMemory(&hal,sizeof(hal)); ZeroMemory(&hel,sizeof(hel)); hal.dwSize=sizeof(hal); hel.dwSize=sizeof(hel);
	hr=IDirectDraw_GetCaps(dd,&hal,&hel);
	fprintf(o,"  GetCaps 0x%08lx: HAL caps 0x%08lx (no hardware %s, 3D %s), video memory %lu KB\n", (unsigned long)hr, hal.dwCaps,
		(hal.dwCaps&DDCAPS_NOHARDWARE)?"yes":"no", (hal.dwCaps&DDCAPS_3D)?"yes":"no", hal.dwVidMemTotal/1024);
	WNDCLASSA wc; ZeroMemory(&wc,sizeof(wc)); wc.lpfnWndProc=DefWindowProcA; wc.hInstance=GetModuleHandleA(NULL); wc.lpszClassName="origrun-probe";
	RegisterClassA(&wc);
	HWND w=CreateWindowExA(WS_EX_TOPMOST,"origrun-probe","origrun probe",WS_POPUP|WS_VISIBLE,0,0,320,240,NULL,NULL,wc.hInstance,NULL);
	hr=IDirectDraw_SetCooperativeLevel(dd,w,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
	fprintf(o,"  exclusive full-screen cooperative level: 0x%08lx\n",(unsigned long)hr);
	if (SUCCEEDED(hr)) {
		static const int modes[][3]={{800,600,16},{800,600,32},{640,480,16}};
		for (int i=0;i<3;i++) {
			HRESULT h2=IDirectDraw_SetDisplayMode(dd,modes[i][0],modes[i][1],modes[i][2]);
			HRESULT h3=E_FAIL;
			if (SUCCEEDED(h2)) {
				DDSURFACEDESC sd; ZeroMemory(&sd,sizeof(sd)); sd.dwSize=sizeof(sd); sd.dwFlags=DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
				sd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE|DDSCAPS_FLIP|DDSCAPS_COMPLEX; sd.dwBackBufferCount=1;
				LPDIRECTDRAWSURFACE ps=NULL; h3=IDirectDraw_CreateSurface(dd,&sd,&ps,NULL);
				if (ps) IDirectDrawSurface_Release(ps);
				IDirectDraw_RestoreDisplayMode(dd);
			}
			fprintf(o,"  SetDisplayMode %dx%dx%d: 0x%08lx; flipping primary surface: %s\n", modes[i][0],modes[i][1],modes[i][2], (unsigned long)h2,
				SUCCEEDED(h2) ? (SUCCEEDED(h3)?"ok":"failed") : "not tried");
		}
		IDirectDraw_SetCooperativeLevel(dd,w,DDSCL_NORMAL);
	}
	IDirectDraw_Release(dd);
	if (w) DestroyWindow(w);
}
static void probe_display(FILE* o)
{
	{   // the sessions, as run_in_user_session sees them
		WTS_SESSION_INFOA* si=NULL; DWORD n=0;
		if (WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE,0,1,&si,&n)) {
			for (DWORD i=0;i<n;i++) { HANDLE t=NULL; int ok = si[i].SessionId!=0 && WTSQueryUserToken(si[i].SessionId,&t); if (t) CloseHandle(t);
				fprintf(o,"session %lu '%s' state %d: %s\n", si[i].SessionId, si[i].pWinStationName?si[i].pWinStationName:"", (int)si[i].State, si[i].SessionId==0?"services":(ok?"a user is logged on":"no user")); }
			WTSFreeMemory(si);
		}
		fprintf(o,"active console session: %lu\n", WTSGetActiveConsoleSessionId());
	}
	{   // where the game would go: WinSta0\Default, as main_in_sandbox puts it
		HWINSTA ws=OpenWindowStationA("WinSta0",FALSE,MAXIMUM_ALLOWED);
		if (ws && SetProcessWindowStation(ws)) {
			HDESK d=OpenDesktopA("Default",0,FALSE,MAXIMUM_ALLOWED);
			if (d && SetThreadDesktop(d)) fprintf(o,"on WinSta0\\Default now\n"); else fprintf(o,"cannot use WinSta0\\Default (err %lu)\n", GetLastError());
		} else fprintf(o,"cannot use WinSta0 (err %lu)\n", GetLastError());
		HDESK in=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS); char nm[64]="(none)"; DWORD k=0;
		if (in) { GetUserObjectInformationA(in,UOI_NAME,nm,sizeof(nm),&k); CloseDesktop(in); }
		fprintf(o,"input desktop: %s\n", nm);
	}
	DWORD ss=0; ProcessIdToSessionId(GetCurrentProcessId(),&ss);
	char ws[64]="?", dk[64]="?"; DWORD n=0;
	GetUserObjectInformationA(GetProcessWindowStation(),UOI_NAME,ws,sizeof(ws),&n);
	GetUserObjectInformationA(GetThreadDesktop(GetCurrentThreadId()),UOI_NAME,dk,sizeof(dk),&n);
	HDC sdc=GetDC(NULL); int bpp=sdc?GetDeviceCaps(sdc,BITSPIXEL):0; if (sdc) ReleaseDC(NULL,sdc);
	fprintf(o,"session %lu, %s\\%s; screen %dx%d at %d bpp\n", ss, ws, dk, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), bpp);
	DEVMODEA dm; ZeroMemory(&dm,sizeof(dm)); dm.dmSize=sizeof(dm); int modes=0, m800=0, m640=0;
	for (DWORD i=0; EnumDisplaySettingsA(NULL,i,&dm); i++) { modes++; if (dm.dmPelsWidth==800&&dm.dmPelsHeight==600) m800|= dm.dmBitsPerPel==16?1:(dm.dmBitsPerPel==32?2:4); if (dm.dmPelsWidth==640&&dm.dmPelsHeight==480) m640=1; }
	fprintf(o,"display modes: %d; 800x600 at 16 bpp %s, at 32 bpp %s; 640x480 %s\n", modes, (m800&1)?"yes":"no", (m800&2)?"yes":"no", m640?"yes":"no");
	{ DISPLAY_DEVICEA dd; ZeroMemory(&dd,sizeof(dd)); dd.cb=sizeof(dd); if (EnumDisplayDevicesA(NULL,0,&dd,0)) fprintf(o,"display adapter: %s\n", dd.DeviceString); }
	probe_ddraw(o,"on the input desktop as found");
	{   // DirectDraw takes the screen only on the input desktop; inside the sandbox origrun may make Default that
		HDESK d=OpenDesktopA("Default",0,FALSE,DESKTOP_SWITCHDESKTOP);
		BOOL sw = d && SwitchDesktop(d); DWORD e=GetLastError(); if (d) CloseDesktop(d);
		HDESK in=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS); char nm[64]="(none)"; DWORD k=0;
		if (in) { GetUserObjectInformationA(in,UOI_NAME,nm,sizeof(nm),&k); CloseDesktop(in); }
		fprintf(o,"SwitchDesktop(Default): %s; input desktop now: %s\n", sw?"ok":"failed", nm);
		if (!sw) fprintf(o,"  (err %lu)\n", e);
		if (sw) probe_ddraw(o,"after SwitchDesktop(Default)");
	}
	{   // the folders and the network, as the .wsb asked
		HANDLE h=CreateFileA("C:\\origrun\\probe-write-test.txt",GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
		if (h!=INVALID_HANDLE_VALUE) { CloseHandle(h); DeleteFileA("C:\\origrun\\probe-write-test.txt"); fprintf(o,"C:\\origrun is WRITABLE (it should be read-only)\n"); }
		else fprintf(o,"C:\\origrun: a write fails (err %lu), as read-only\n", GetLastError());
		ULONG len=32768; IP_ADAPTER_INFO* ai=(IP_ADAPTER_INFO*)malloc(len); int all=0, addr=0;
		DWORD r = ai ? GetAdaptersInfo(ai,&len) : ERROR_NOT_ENOUGH_MEMORY;
		if (r==NO_ERROR) for (IP_ADAPTER_INFO* x=ai; x; x=x->Next) { all++; if (strcmp(x->IpAddressList.IpAddress.String,"0.0.0.0")) addr++; }
		free(ai);
		fprintf(o,"IPv4 network adapters: %d, with an address: %d%s\n", all, addr, r==ERROR_NO_DATA?" (none at all)":"");
	}
	char gpu[MAX_PATH]; int g=process_uses_gpu_api(GetCurrentProcessId(),gpu,sizeof(gpu));
	fprintf(o,"GPU modules in this process after DirectDraw: %s\n", g>0?gpu:(g<0?"(list not readable)":"none"));
}

// ------------------------------------------------------------------ registry presets, inside the sandbox

// --reg (on the host) stages a .reg file as <run>\sandbox-files\registry.reg; a session may also put one there
// itself. Inside, the origrun that launches the game imports it first, as the same user as the game. As SYSTEM
// (wsb exec -r System) that user's HKCU is SYSTEM's own hive, HKEY_USERS\S-1-5-18, so HKCU keys land where the game
// looks. UAC never virtualizes SYSTEM, so a key the host keeps in UAC's per-user VirtualStore
// (HKCU\Software\Classes\VirtualStore\MACHINE\...) is, for a game run as SYSTEM, the HKLM key itself: such keys are
// moved there, and the file is imported through the 32-bit view the 32-bit games use (reg import /reg:32, which keeps
// HKLM\SOFTWARE\X in HKLM\SOFTWARE\WOW6432Node\X). After the run the game's keys are exported to the logs.
#define MAX_REG_KEYS 64
static WCHAR gRegKeys[MAX_REG_KEYS][260]; static int gRegKeysN = 0;   // the keys the presets name

static int ci_prefix_w(const WCHAR* s, const WCHAR* pre)
{
	int n=lstrlenW(pre);
	return lstrlenW(s)>=n && CompareStringOrdinal(s,n,pre,n,TRUE)==CSTR_EQUAL;
}
static HKEY reg_root_w(const WCHAR* key, const WCHAR** sub)
{
	static const WCHAR* const names[]={L"HKEY_CURRENT_USER\\",L"HKEY_LOCAL_MACHINE\\",L"HKEY_USERS\\",L"HKEY_CLASSES_ROOT\\",L"HKEY_CURRENT_CONFIG\\"};
	HKEY roots[]={HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE,HKEY_USERS,HKEY_CLASSES_ROOT,HKEY_CURRENT_CONFIG};
	for (int i=0;i<5;i++) if (ci_prefix_w(key,names[i])) { *sub=key+lstrlenW(names[i]); return roots[i]; }
	return NULL;
}
// Runs a command line with no window, its output appended to outPath: 1 when it ran, its exit code in *ec.
static int run_hidden(const char* cmdline, const char* outPath, DWORD waitMs, DWORD* ec)
{
	SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};
	HANDLE out=CreateFileA(outPath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	STARTUPINFOA si; ZeroMemory(&si,sizeof(si)); si.cb=sizeof(si);
	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW; si.wShowWindow=SW_HIDE;
	si.hStdInput=NULL; si.hStdOutput=out; si.hStdError=out;
	PROCESS_INFORMATION pi; ZeroMemory(&pi,sizeof(pi));
	char* cmd=_strdup(cmdline);
	BOOL ok = cmd && CreateProcessA(NULL,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
	DWORD err=GetLastError(); free(cmd);
	if (out!=INVALID_HANDLE_VALUE) CloseHandle(out);
	*ec=1;
	if (!ok) { logline("could not run %.160s (err %lu)", cmdline, err); return 0; }
	CloseHandle(pi.hThread);
	if (WaitForSingleObject(pi.hProcess,waitMs)==WAIT_TIMEOUT) { logline("%.80s did not finish within %lu s; ending it", cmdline, waitMs/1000); TerminateProcess(pi.hProcess,1); WaitForSingleObject(pi.hProcess,5000); }
	GetExitCodeProcess(pi.hProcess,ec); CloseHandle(pi.hProcess);
	return 1;
}
static void import_registry(const char* logs)
{
	char rp[MAX_PATH]; wsprintfA(rp,"%s\\sandbox-files\\registry.reg",gRunDir);
	HANDLE h=CreateFileA(rp,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (h==INVALID_HANDLE_VALUE) return;   // no presets
	DWORD size=GetFileSize(h,NULL), rd=0;
	unsigned char* raw = (size!=INVALID_FILE_SIZE && size<=(4u<<20)) ? (unsigned char*)malloc(size+2) : NULL;
	int got = raw && ReadFile(h,raw,size,&rd,NULL) && rd==size;
	CloseHandle(h);
	if (!got) { logline("registry presets: cannot read %s (or it is over 4 MB); not imported", rp); free(raw); return; }
	// The file's encoding is kept: UTF-16LE with a BOM (regedit's "Version 5.00" export), UTF-8 with a BOM, or ANSI
	// (REGEDIT4).
	int enc=0; const unsigned char* body=raw; int blen=(int)size;
	if (size>=2 && raw[0]==0xFF && raw[1]==0xFE) { enc=2; body+=2; blen-=2; }
	else if (size>=3 && raw[0]==0xEF && raw[1]==0xBB && raw[2]==0xBF) { enc=1; body+=3; blen-=3; }
	UINT cp = enc==1 ? CP_UTF8 : CP_ACP;
	int inLen = enc==2 ? blen/2 : MultiByteToWideChar(cp,0,(const char*)body,blen,NULL,0);
	WCHAR* in=(WCHAR*)malloc((inLen+1)*sizeof(WCHAR)); WCHAR* outw=(WCHAR*)malloc((inLen+64)*sizeof(WCHAR));
	if (!in || !outw) { logline("registry presets: out of memory"); free(raw); free(in); free(outw); return; }
	if (enc==2) memcpy(in,body,inLen*sizeof(WCHAR)); else MultiByteToWideChar(cp,0,(const char*)body,blen,in,inLen);
	in[inLen]=0; free(raw);

	// Key lines are "[name]", or "[-name]" to delete one. Moving a key only ever shortens its line.
	static const WCHAR vs[]=L"HKEY_CURRENT_USER\\Software\\Classes\\VirtualStore\\MACHINE\\";
	static const WCHAR wow[]=L"SOFTWARE\\WOW6432Node";
	int o=0, keys=0, moved=0; gRegKeysN=0;
	for (int i=0;i<inLen;) {
		int e=i; while (e<inLen && in[e]!=L'\n') e++;   // the line is in[i..e)
		int s=i; while (s<e && (in[s]==L' '||in[s]==L'\t')) s++;
		int done=0;
		if (s<e && in[s]==L'[') {
			int del = s+1<e && in[s+1]==L'-', k0=s+1+del, k1=k0;
			while (k1<e && in[k1]!=L']') k1++;
			if (k1<e && k1-k0<1000) {
				WCHAR key[1024], tr[1024]; memcpy(key,in+k0,(k1-k0)*sizeof(WCHAR)); key[k1-k0]=0; lstrcpyW(tr,key);
				if (gAsSystem && ci_prefix_w(key,vs)) {
					const WCHAR* rest=key+lstrlenW(vs); int wl=lstrlenW(wow);
					if (ci_prefix_w(rest,wow) && (rest[wl]==0 || rest[wl]==L'\\')) _snwprintf(tr,1023,L"HKEY_LOCAL_MACHINE\\SOFTWARE%ls",rest+wl);
					else _snwprintf(tr,1023,L"HKEY_LOCAL_MACHINE\\%ls",rest);
					tr[1023]=0; moved++;
				}
				int tl=lstrlenW(tr);
				memcpy(outw+o,in+i,(k0-i)*sizeof(WCHAR)); o+=k0-i;
				memcpy(outw+o,tr,tl*sizeof(WCHAR)); o+=tl;
				memcpy(outw+o,in+k1,(e-k1)*sizeof(WCHAR)); o+=e-k1;
				if (!del) { keys++; if (gRegKeysN<MAX_REG_KEYS) lstrcpynW(gRegKeys[gRegKeysN++],tr,260); }
				done=1;
			}
		}
		if (!done) { memcpy(outw+o,in+i,(e-i)*sizeof(WCHAR)); o+=e-i; }
		if (e<inLen) outw[o++]=L'\n';
		i=e+1;
	}
	free(in);

	char op[MAX_PATH]; joinp(op,logs,"registry-imported.reg");
	FILE* f=fopen(op,"wb");
	if (!f) { logline("registry presets: cannot write %s; not imported", op); free(outw); return; }
	if (enc==2) { fwrite("\xFF\xFE",1,2,f); fwrite(outw,sizeof(WCHAR),o,f); }
	else {
		if (enc==1) fwrite("\xEF\xBB\xBF",1,3,f);
		int n=WideCharToMultiByte(cp,0,outw,o,NULL,0,NULL,NULL); char* b=(char*)malloc(n+1);
		if (b) { WideCharToMultiByte(cp,0,outw,o,b,n,NULL,NULL); fwrite(b,1,n,f); free(b); }
	}
	fclose(f); free(outw);

	char sys[MAX_PATH]; GetSystemDirectoryA(sys,MAX_PATH);
	char cmd[MAX_PATH*3]; _snprintf(cmd,sizeof(cmd)-1,"\"%s\\reg.exe\" import \"%s\" /reg:32",sys,op); cmd[sizeof(cmd)-1]=0;
	char rl[MAX_PATH]; joinp(rl,logs,"reg.txt");
	DWORD ec=1; run_hidden(cmd,rl,60000,&ec);
	int present=0; char missing[700]="";
	for (int i=0;i<gRegKeysN;i++) {
		const WCHAR* sub=NULL; HKEY root=reg_root_w(gRegKeys[i],&sub), k;
		if (root && RegOpenKeyExW(root,sub,0,KEY_READ|KEY_WOW64_32KEY,&k)==ERROR_SUCCESS) { present++; RegCloseKey(k); }
		else if (lstrlenA(missing)<400) { char one[300]; _snprintf(one,sizeof(one)-1,"%s%ls",missing[0]?"; ":"",gRegKeys[i]); one[sizeof(one)-1]=0; lstrcatA(missing,one); }
	}
	logline("registry presets: %s, %d keys (%d moved from UAC's VirtualStore to HKLM, since the game runs as %s), imported "
		"with reg import /reg:32 as %s (exit %lu; %s); %d of the %d keys are there now%s%s", rp, keys, moved,
		gAsSystem?"SYSTEM":"the sandbox's user", op, ec, rl, present, gRegKeysN, missing[0]?"; missing: ":"", missing);
}
// After the run: the game's keys as it left them, for the host to read. PopCap's under HKCU and HKLM (32-bit view),
// and every other key the presets named, each exported once to <run>\logs\registry-after-<n>.reg.
static void export_registry(const char* logs)
{
	static WCHAR keys[MAX_REG_KEYS+2][260]; int n=0;
	lstrcpyW(keys[n++],L"HKEY_CURRENT_USER\\Software\\PopCap");
	lstrcpyW(keys[n++],L"HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap");
	for (int i=0;i<gRegKeysN;i++) {
		int covered=0;
		for (int j=0;j<n && !covered;j++) { int l=lstrlenW(keys[j]); covered = ci_prefix_w(gRegKeys[i],keys[j]) && (gRegKeys[i][l]==0 || gRegKeys[i][l]==L'\\'); }
		if (!covered) lstrcpynW(keys[n++],gRegKeys[i],260);
	}
	char sys[MAX_PATH]; GetSystemDirectoryA(sys,MAX_PATH);
	char rl[MAX_PATH]; joinp(rl,logs,"reg.txt");
	char list[1200]=""; int written=0;
	for (int i=0;i<n;i++) {
		const WCHAR* sub=NULL; HKEY root=reg_root_w(keys[i],&sub), k;
		if (!root || RegOpenKeyExW(root,sub,0,KEY_READ|KEY_WOW64_32KEY,&k)!=ERROR_SUCCESS) continue;
		RegCloseKey(k);
		char ka[300]; if (!WideCharToMultiByte(CP_ACP,0,keys[i],-1,ka,sizeof(ka),NULL,NULL)) continue;
		char leaf[40], fp[MAX_PATH]; wsprintfA(leaf,"registry-after-%d.reg",written+1); joinp(fp,logs,leaf);
		char cmd[MAX_PATH*3]; _snprintf(cmd,sizeof(cmd)-1,"\"%s\\reg.exe\" export \"%s\" \"%s\" /y /reg:32",sys,ka,fp); cmd[sizeof(cmd)-1]=0;
		DWORD ec=1;
		if (run_hidden(cmd,rl,60000,&ec) && ec==0) {
			written++;
			if (lstrlenA(list)<900) { char one[340]; _snprintf(one,sizeof(one)-1,"%s%s (%s)",list[0]?", ":"",ka,leaf); one[sizeof(one)-1]=0; lstrcatA(list,one); }
		}
		else logline("registry after the run: reg export of %s failed (exit %lu)", ka, ec);
	}
	if (written) logline("registry after the run, as the game left it: %s, in %s", list, logs);
	else logline("registry after the run: none of the keys exists (HKCU and HKLM Software\\PopCap, and the keys the presets name)");
}

// The dead-man's switch. The host's origrun stops the sandbox when the run ends, but one that is killed
// (TerminateProcess runs no handler) stops nothing. So the sandbox shuts itself down: the origrun that wsb exec
// starts arms a power-off at the run's timeout plus 90 s, and again 30 s after it writes the done marker; a host that
// stops the sandbox first makes both moot. Only ever inside: Windows Sandbox detected, as SYSTEM, run from the
// read-only C:\origrun the sandbox maps.
static DWORD arm_guest_shutdown(DWORD sec)
{
	char self[MAX_PATH]=""; GetModuleFileNameA(NULL,self,MAX_PATH);
	if (!gInSandbox || !gAsSystem || !ieq(self,"C:\\origrun\\origrun.exe")) return ERROR_ACCESS_DENIED;
	HANDLE tok;
	if (OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&tok)) {
		TOKEN_PRIVILEGES tp; tp.PrivilegeCount=1; tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
		if (LookupPrivilegeValueA(NULL,"SeShutdownPrivilege",&tp.Privileges[0].Luid)) AdjustTokenPrivileges(tok,FALSE,&tp,0,NULL,NULL);
		CloseHandle(tok);
	}
	AbortSystemShutdownA(NULL);   // an earlier timer of this run is replaced
	return InitiateShutdownA(NULL,(LPSTR)"byo-pop origrun: the run inside Windows Sandbox is over or out of time",sec,
		SHUTDOWN_POWEROFF|SHUTDOWN_FORCE_OTHERS|SHUTDOWN_FORCE_SELF,
		SHTDN_REASON_MAJOR_OTHER|SHTDN_REASON_MINOR_OTHER|SHTDN_REASON_FLAG_PLANNED);
}

static int main_in_sandbox(const char* dir, int inSession, int probe)
{
	char why[300];
	gInSandbox = detect_windows_sandbox(why,sizeof(why));
	if (probe) {   // the values go to probe.txt either way; the display test runs only inside
		char pp[MAX_PATH]; joinp(pp,dir,"probe.txt"); FILE* o=fopen(pp,"w");
		if (o) {
			fprintf(o,"sandbox check: %s: %s\n", gInSandbox?"passed":"FAILED", why);
			if (gInSandbox) { HWND cw=GetConsoleWindow(); if (cw) ShowWindow(cw,SW_HIDE); probe_display(o); }
			fclose(o);
		}
		if (gInSandbox) {   // the dead-man's switch, as for a game run
			DWORD r=arm_guest_shutdown(30); FILE* a=fopen(pp,"a");
			if (a) { if (r) fprintf(a,"shutdown timer: could not arm it (err %lu)\n",r); else fprintf(a,"shutdown timer: armed; the sandbox powers off in 30 s unless the host stops it first\n"); fclose(a); }
		}
		char mk[MAX_PATH]; joinp(mk,dir,"sandbox-started.txt"); FILE* f=fopen(mk,"w"); if (f) { fprintf(f,"started\n"); fclose(f); }
		joinp(mk,dir,"sandbox-done.txt"); f=fopen(mk,"w"); if (f) { fprintf(f,"rc=%d probe\n", gInSandbox?0:2); fclose(f); }
		return gInSandbox?0:2;
	}
	if (!gInSandbox) {
		fprintf(stderr,"origrun --in-sandbox: this is not Windows Sandbox (%s); refusing to run\n", why);
		char rp[MAX_PATH]; joinp(rp,dir,"sandbox-run.txt");
		if (!inSession && GetFileAttributesA(rp)!=INVALID_FILE_ATTRIBUTES) {   // a staged run: tell the host at once
			char dn[MAX_PATH]; joinp(dn,dir,"sandbox-done.txt"); FILE* f=fopen(dn,"w"); if (f) { fprintf(f,"rc=2 not Windows Sandbox: %s\n", why); fclose(f); }
		}
		return 2;
	}
	{ HWND cw=GetConsoleWindow(); if (cw) ShowWindow(cw,SW_HIDE); }   // the LogonCommand's console, inside the sandbox
	if (!inSession) {   // the one started by the host (the one --in-session is its own child, in a user session)
		HANDLE once=CreateMutexA(NULL,TRUE,"Global\\byo-pop-origrun-in-sandbox");
		if (once && GetLastError()==ERROR_ALREADY_EXISTS) { fprintf(stderr,"origrun --in-sandbox: another origrun already runs in this sandbox\n"); return 2; }
	}
	lstrcpynA(gRunDir,dir,MAX_PATH);
	joinp(gShotDir,gRunDir,"shots"); mkdirs(gShotDir);
	char logs[MAX_PATH]; joinp(logs,gRunDir,"logs"); mkdirs(logs);
	InitializeCriticalSection(&gLogLock); gLogLockInit=1;
	char logpath[MAX_PATH]; joinp(logpath,logs,"origrun-sandbox.log"); gLogFile=fopen(logpath,inSession?"a":"w");
	if (!inSession) landed_reset();
	if (!inSession) { char mk[MAX_PATH]; joinp(mk,gRunDir,"sandbox-started.txt"); FILE* f=fopen(mk,"w"); if (f) { fprintf(f,"started\n"); fclose(f); } }
	{ DWORD ss=0; ProcessIdToSessionId(GetCurrentProcessId(),&ss); logline("inside Windows Sandbox, session %lu: %s", ss, why); }
	int rc=2;   // from here on, every way out of the first origrun writes the done marker the host waits for

	char exeRel[MAX_PATH]="", scriptRel[MAX_PATH]="", sound[32]="stub", key3d[256]="";
	int timeout=120;
	{
		char rp[MAX_PATH]; joinp(rp,gRunDir,"sandbox-run.txt");
		FILE* f=fopen(rp,"r"); char ln[512];
		if (!f) { logline("no %s", rp); goto done; }
		while (fgets(ln,sizeof(ln),f)) {
			char* e=strpbrk(ln,"\r\n"); if(e)*e=0;
			char* eq=strchr(ln,'='); if (!eq) continue; *eq=0; const char* v=eq+1;
			if (ieq(ln,"exe")) lstrcpynA(exeRel,v,MAX_PATH);
			else if (ieq(ln,"script")) lstrcpynA(scriptRel,v,MAX_PATH);
			else if (ieq(ln,"timeout")) timeout=atoi(v);
			else if (ieq(ln,"sound")) lstrcpynA(sound,v,sizeof(sound));
			else if (ieq(ln,"update_ms")) gUpdateMs=atoi(v)>0?atoi(v):10;
			else if (ieq(ln,"counter")) lstrcpynA(gCounterExpr,v,256);
			else if (ieq(ln,"flips")) lstrcpynA(gFlipsExpr,v,256);
			else if (ieq(ln,"pending")) lstrcpynA(gPendingExpr,v,256);
			else if (ieq(ln,"state")) lstrcpynA(gStateExpr,v,256);
			else if (ieq(ln,"empty")) lstrcpynA(gEmptyExpr,v,256);
			else if (ieq(ln,"watch") && gWatchN<4) lstrcpynA(gWatchExpr[gWatchN++],v,256);
			else if (ieq(ln,"3d")) gMode3dWant = ieq(v,"on") ? 1 : 0;
			else if (ieq(ln,"key3d")) lstrcpynA(key3d,v,256);
			else if (ieq(ln,"flag3d")) lstrcpynA(gMode3dExpr,v,256);
		}
		fclose(f);
	}
	if (!exeRel[0] || strstr(exeRel,"..") || exeRel[0]=='\\' || strchr(exeRel,':')) { logline("bad exe in sandbox-run.txt: '%s'", exeRel); goto done; }
	if (timeout>240) timeout=240;
	if (!inSession) {   // the dead-man's switch: a sandbox whose host origrun was killed still ends
		DWORD r=arm_guest_shutdown((DWORD)timeout+90);
		if (r) logline("WARNING: could not arm the sandbox's shutdown timer (err %lu)", r);
		else logline("shutdown timer: the sandbox powers off %d s from now (the run's timeout plus 90 s) unless the host stops it first", timeout+90);
	}
	joinp(gExePath,gRunDir,exeRel);
	joinp(gGameDir,gRunDir,"game");
	char script[MAX_PATH]=""; if (scriptRel[0]) joinp(script,gRunDir,scriptRel);

	// Saves and other files a run needs outside the game's folder (C:\ProgramData\..., say) are staged by the host
	// under <run>\sandbox-files\C\ and copied onto C: here. They, the registry and everything else the game writes
	// outside C:\run vanish with the sandbox.
	if (!inSession) {
		char sf[MAX_PATH]; wsprintfA(sf,"%s\\sandbox-files\\C",gRunDir);
		if (GetFileAttributesA(sf)!=INVALID_FILE_ATTRIBUTES) { logline("copying %s onto C:", sf); copy_tree(sf,"C:"); }
		if (run_in_user_session(gRunDir,timeout,&rc)) { logline("origrun in the user session ended with code %d", rc); goto done; }
	}

	// wsb exec runs origrun as SYSTEM; the game goes on the interactive window station's desktop, WinSta0\Default.
	{
		HWINSTA ws=OpenWindowStationA("WinSta0",FALSE,MAXIMUM_ALLOWED);
		if (ws && SetProcessWindowStation(ws)) logline("using window station WinSta0");
		else logline("warning: cannot use window station WinSta0 (err %lu)", GetLastError());
		lstrcpyA(gDeskName,"Default");
		// In session 0 the input desktop is Winlogon, and DirectDraw sets a display mode only on the input desktop
		// (the probe: every SetDisplayMode failed before the switch, 800x600x32 worked after it).
		if (!inSession) {
			HDESK d=OpenDesktopA("Default",0,FALSE,DESKTOP_SWITCHDESKTOP);
			if (d && SwitchDesktop(d)) logline("made WinSta0\\Default the sandbox's input desktop");
			else logline("warning: SwitchDesktop(Default) failed (err %lu)", GetLastError());
			if (d) CloseDesktop(d);
		}
	}
	SetEnvironmentVariableA("ORIGRUN_DIR",gRunDir);
	SetEnvironmentVariableA("ORIGRUN_SOUND", ieq(sound,"real") ? "real" : sound);

	import_registry(logs);   // the presets first, in the hive the game uses, so the 3D setting below wins over them
	DWORD prev3d=0; int hadPrev3d=0; static RegVals prevTest3d;
	if (key3d[0]) { hadPrev3d = reg_get_is3d(key3d,&prev3d); read_test3d(key3d,&prevTest3d); }
	if (gMode3dWant>=0 && key3d[0]) {
		if (!reg_set_is3d(key3d,1,(DWORD)gMode3dWant)) logline("--3d: could not write %s\\%s (err %lu)",key3d,kIs3DValue,GetLastError());
		else logline("--3d %s: set %s=%d inside the sandbox", gMode3dWant?"on":"off", kIs3DValue, gMode3dWant);
	}
	if (gCounterExpr[0]) logline("update counter: %s (one update = %d ms)%s%s%s%s%s%s%s%s", gCounterExpr, gUpdateMs, gFlipsExpr[0]?", flips=":"", gFlipsExpr, gPendingExpr[0]?", pending draw=":"", gPendingExpr, gStateExpr[0]?", update step=":"", gStateExpr, gEmptyExpr[0]?", draw empty=":"", gEmptyExpr);

	{
		Launched L;
		if (launch_game(NULL,&L)) rc=watch_run(&L, script[0]?script:NULL, timeout, key3d, hadPrev3d, prev3d, &prevTest3d, logs, logpath);
		export_registry(logs);
	}
done:
	if (inSession) return rc;   // the origrun in session 0 writes the done marker
	{ char dn[MAX_PATH]; joinp(dn,gRunDir,"sandbox-done.txt"); FILE* f=fopen(dn,"w"); if (f) { fprintf(f,"rc=%d\n",rc); fclose(f); } }
	{
		DWORD r=arm_guest_shutdown(30);
		if (r) logline("WARNING: could not re-arm the sandbox's shutdown timer (err %lu)", r);
		else logline("shutdown timer: the sandbox powers off in 30 s unless the host stops it first");
	}
	return rc;
}

// A crash of origrun itself leaves the game running on rundesk's desktop with nothing to stop it, so say where it
// happened (module and offset, for addr2line) before the process goes.
static LONG WINAPI crash_filter(EXCEPTION_POINTERS* ep)
{
	HANDLE held=(HANDLE)InterlockedExchangePointer(&gHeldThread,NULL);   // first of all, let a held game run on
	if (held) ResumeThread(held);
	stop_sandbox("origrun crashed");   // and never leave a sandbox running
	void* a=ep->ExceptionRecord->ExceptionAddress; HMODULE m=NULL; char mod[MAX_PATH]="?";
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCSTR)a,&m)) GetModuleFileNameA(m,mod,MAX_PATH);
	logline("origrun crashed: exception 0x%08lx at %p (%s+0x%lx), thread %lu; the game may still be running", ep->ExceptionRecord->ExceptionCode, a, mod, (unsigned long)((char*)a-(char*)m), GetCurrentThreadId());
	return EXCEPTION_CONTINUE_SEARCH;
}

// --run names one folder under build\origrun, which a sandbox maps read-write: letters, digits and - _ . + only, no
// "..", not starting or ending with a dot, at most 64 characters.
static int run_name_ok(const char* r)
{
	int n=lstrlenA(r);
	if (n<1 || n>64 || strstr(r,"..") || r[0]=='.' || r[n-1]=='.') return 0;
	for (const char* q=r;*q;q++) { char c=*q; if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='+')) return 0; }
	return 1;
}

int main(int argc, char** argv)
{
	crc_init();
	SetUnhandledExceptionFilter(crash_filter);
	InitializeCriticalSection(&gRpmLock);
	const char* game=NULL; const char* exe=NULL; const char* script=NULL;
	const char* run=NULL; const char* sound="stub"; int timeout=120; int fresh=0; int copyOnly=0;
	const char* patches=NULL; const char* counter=NULL; const char* flips=NULL; const char* pending=NULL; const char* ustate=NULL; const char* dempty=NULL;
	int haveSeed=0; uint32_t seed=0; int updateMs=0; int noPatch=0; const char* stageRoot=NULL;
	const char* key3dArg=NULL; const char* flag3dArg=NULL; const char* dgvArg=NULL;
	int sandboxHost=0, sandboxVgpu=0, sandboxWait=600, inSession=0, sandboxProbe=0, probe=0; const char* inSandboxDir=NULL;
	const char* regArg=NULL;

	for (int i=1;i<argc;i++) {
		if (ieq(argv[i],"--game")&&i+1<argc) game=argv[++i];
		else if (ieq(argv[i],"--exe")&&i+1<argc) exe=argv[++i];
		else if (ieq(argv[i],"--script")&&i+1<argc) script=argv[++i];
		else if (ieq(argv[i],"--run")&&i+1<argc) run=argv[++i];
		else if (ieq(argv[i],"--sound")&&i+1<argc) sound=argv[++i];
		else if (ieq(argv[i],"--timeout")&&i+1<argc) timeout=atoi(argv[++i]);
		else if (ieq(argv[i],"--patches")&&i+1<argc) patches=argv[++i];
		else if (ieq(argv[i],"--no-patches")) noPatch=1;
		else if (ieq(argv[i],"--seed")&&i+1<argc) { haveSeed=1; seed=(uint32_t)strtoul(argv[++i],NULL,0); }
		else if (ieq(argv[i],"--counter")&&i+1<argc) counter=argv[++i];
		else if (ieq(argv[i],"--flips")&&i+1<argc) flips=argv[++i];
		else if (ieq(argv[i],"--pending-draw")&&i+1<argc) pending=argv[++i];
		else if (ieq(argv[i],"--update-state")&&i+1<argc) ustate=argv[++i];
		else if (ieq(argv[i],"--draw-empty")&&i+1<argc) dempty=argv[++i];
		else if (ieq(argv[i],"--watch")&&i+1<argc) { if (gWatchN<4) lstrcpynA(gWatchExpr[gWatchN++],argv[++i],256); else { fprintf(stderr,"origrun: at most 4 --watch\n"); return 2; } }
		else if (ieq(argv[i],"--update-ms")&&i+1<argc) updateMs=atoi(argv[++i]);
		else if (ieq(argv[i],"--copy-only")) copyOnly=1;
		else if (ieq(argv[i],"--fresh")) fresh=1;
		else if (ieq(argv[i],"--stage-root")&&i+1<argc) stageRoot=argv[++i];
		else if (ieq(argv[i],"--3d")&&i+1<argc) {
			const char* v=argv[++i];
			if (ieq(v,"on")) gMode3dWant=1; else if (ieq(v,"off")) gMode3dWant=0;
			else { fprintf(stderr,"--3d takes on or off, not %s\n",v); return 2; }
		}
		else if (ieq(argv[i],"--3d-key")&&i+1<argc) key3dArg=argv[++i];
		else if (ieq(argv[i],"--3d-flag")&&i+1<argc) flag3dArg=argv[++i];
		else if (ieq(argv[i],"--dgvoodoo")&&i+1<argc) dgvArg=argv[++i];
		else if (ieq(argv[i],"--sandbox")) sandboxHost=1;
		else if (ieq(argv[i],"--sandbox-vgpu")) sandboxVgpu=1;
		else if (ieq(argv[i],"--sandbox-wait")&&i+1<argc) sandboxWait=atoi(argv[++i]);
		else if (ieq(argv[i],"--reg")&&i+1<argc) regArg=argv[++i];
		else if (ieq(argv[i],"--in-sandbox")&&i+1<argc) inSandboxDir=argv[++i];
		else if (ieq(argv[i],"--in-session")) inSession=1;
		else if (ieq(argv[i],"--probe")) probe=1;
		else if (ieq(argv[i],"--sandbox-probe")) { sandboxProbe=1; sandboxHost=1; }
		else { fprintf(stderr,"unknown or incomplete option: %s\n",argv[i]); return 2; }
	}
	if (inSandboxDir) return main_in_sandbox(inSandboxDir, inSession, probe);   // everything else comes from the run folder
	if (!game && !exe && !sandboxProbe) die("need --game <name> or --exe <path>");
	if (timeout>240) timeout=240;   // the machine rule: nothing longer than about 4 minutes
	if (!sandboxHost && !copyOnly && script_needs_sandbox(script)) {
		fprintf(stderr, "origrun: refusing to start: this run takes shots or sends hw* input, and origrun does neither on the "
			"host (a capture there can show the user's own screen, and hw* input reaches the user's desktop). Run it with "
			"--sandbox, or with a script that does neither.\n");
		return 2;
	}
	if (regArg && (!sandboxHost || sandboxProbe)) {
		fprintf(stderr, "origrun: --reg imports registry presets inside Windows Sandbox, for a game run there (--sandbox); on the "
			"host origrun writes no registry from a file\n");
		return 2;
	}
	if (regArg && GetFileAttributesA(regArg)==INVALID_FILE_ATTRIBUTES) { fprintf(stderr, "origrun: --reg: no file %s\n", regArg); return 2; }
	gRegArg=regArg;
	if (sandboxHost && stageRoot) { fprintf(stderr, "origrun: --sandbox stages the copy in the run folder; drop --stage-root (inside the sandbox the path is short)\n"); return 2; }

	// where the tool and rundesk live (origrun.exe sits in build/origrun-bin/)
	char selfDir[MAX_PATH]; GetModuleFileNameA(NULL,selfDir,MAX_PATH);
	char* s=strrchr(selfDir,'\\'); if(s)*s=0;
	char repoRaw[MAX_PATH], repo[MAX_PATH];
	wsprintfA(repoRaw,"%s\\..\\..",selfDir);                    // build/origrun-bin -> repo root
	GetFullPathNameA(repoRaw, MAX_PATH, repo, NULL);           // canonicalize (drop the ..\..)
	lstrcpynA(gRepo, repo, MAX_PATH);
	char rundesk[MAX_PATH]; wsprintfA(rundesk,"%s\\build\\native\\rundesk.exe",repo);
	if (!sandboxHost && GetFileAttributesA(rundesk)==INVALID_FILE_ATTRIBUTES)
		{ logline("rundesk not found at %s -- build it: cmake --build build/native --target rundesk", rundesk); return 2; }

	if (!run) run = game ? game : (sandboxProbe ? "sandbox-probe" : "run");
	if (!run_name_ok(run)) { fprintf(stderr, "origrun: --run '%s' is not a plain folder name: letters, digits and - _ . + only, no '..', at most 64 characters\n", run); return 2; }
	if (lstrlenA(repo)+lstrlenA(run)+16 > MAX_PATH-100) { fprintf(stderr, "origrun: the run folder's path would be too long (%s\\build\\origrun\\%s)\n", repo, run); return 2; }
	wsprintfA(gRunDir,"%s\\build\\origrun\\%s",repo,run);
	joinp(gShotDir,gRunDir,"shots");
	// The game copy goes under gRunDir by default; --stage-root puts it under a SHORT path instead (shots and
	// logs stay under gRunDir). A worktree's build/origrun path is long, and some launchers (Bookworm Adventures)
	// exit at once when their folder sits that deep, so a worktree session stages the copy under, e.g., main's
	// build/origrun or a short drive-root folder.
	if (stageRoot && stageRoot[0]) { char sr[MAX_PATH]; lstrcpynA(sr,stageRoot,MAX_PATH); for(char* q=sr;*q;q++) if(*q=='/')*q='\\';
		lstrcpynA(gStageRoot,sr,MAX_PATH);
		wsprintfA(gGameDir,"%s\\%s\\game",sr,run); }
	else joinp(gGameDir,gRunDir,"game");

	// Resolve source and exe.
	char srcDir[MAX_PATH]={0}, exeLeaf[MAX_PATH]={0}, tsvKey3d[256]={0}, tsvFlag3d[256]={0};
	if (game) {
		if (!lookup_game(game,NULL,srcDir,exeLeaf,tsvKey3d,tsvFlag3d)) { logline("unknown game '%s' -- add it to tools/origrun/games.tsv or use --exe",game); return 2; }
		if (GetFileAttributesA(srcDir)==INVALID_FILE_ATTRIBUTES) { logline("source folder not found: %s",srcDir); return 2; }
	}

	mkdirs(gRunDir);
	// One origrun per run folder: a second run with the same --run would stage over the first one's copy, script and
	// markers. The lock is taken before anything in the folder is written; Windows lets it go when origrun ends.
	{
		char lp[MAX_PATH]; joinp(lp,gRunDir,"run.lock");
		HANDLE rl=CreateFileA(lp,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (rl==INVALID_HANDLE_VALUE) {
			DWORD e=GetLastError();
			if (e==ERROR_SHARING_VIOLATION || e==ERROR_LOCK_VIOLATION) fprintf(stderr, "origrun: another origrun is using the run folder %s (it holds run.lock); give this run its own --run\n", gRunDir);
			else fprintf(stderr, "origrun: cannot open %s (err %lu)\n", lp, e);
			return 2;
		}
		char info[64]; int n=wsprintfA(info,"pid=%lu\r\n",GetCurrentProcessId()); DWORD wr=0;
		SetEndOfFile(rl); WriteFile(rl,info,(DWORD)n,&wr,NULL);
		// never closed: held until origrun ends
	}
	mkdirs(gShotDir);
	char logs[MAX_PATH]; joinp(logs,gRunDir,"logs"); mkdirs(logs);
	InitializeCriticalSection(&gLogLock); gLogLockInit=1;
	char logpath[MAX_PATH]; joinp(logpath,logs,"origrun.log"); gLogFile=fopen(logpath,"w");
	landed_reset();
	if (sandboxProbe) return sandbox_host_run(selfDir, NULL, timeout, "stub", NULL, sandboxVgpu, sandboxWait, 1);

	// Per-game config (git-ignored, beside games.tsv): lines "counter=", "flips=", "update_ms=", "seed=",
	// "patches=". CLI options override it. It records a game's update-counter address and patch-list path so a
	// session need not repeat them on every run.
	static char cfgCounter[256]="", cfgFlips[256]="", cfgPending[256]="", cfgState[256]="", cfgEmpty[256]="", cfgPatches[MAX_PATH]="";
	int cfgUpdateMs=0, cfgHaveSeed=0; uint32_t cfgSeed=0;
	if (game) {
		char cfg[MAX_PATH]; wsprintfA(cfg,"%s\\tools\\origrun\\%s.cfg",repo,game);
		FILE* cf=fopen(cfg,"r");
		if (cf) {
			char ln[512];
			while (fgets(ln,sizeof(ln),cf)) {
				char* p=ln; while(*p==' '||*p=='\t')p++; if(*p=='#'||*p==0) continue;
				char* nl=strpbrk(p,"\r\n"); if(nl)*nl=0;
				char* eq=strchr(p,'='); if(!eq) continue; *eq=0; char* v=eq+1; while(*v==' ')v++;
				if (ieq(p,"counter")) lstrcpynA(cfgCounter,v,256);
				else if (ieq(p,"flips")) lstrcpynA(cfgFlips,v,256);
				else if (ieq(p,"pending_draw")) lstrcpynA(cfgPending,v,256);
				else if (ieq(p,"update_state")) lstrcpynA(cfgState,v,256);
				else if (ieq(p,"draw_empty")) lstrcpynA(cfgEmpty,v,256);
				else if (ieq(p,"update_ms")) cfgUpdateMs=atoi(v);
				else if (ieq(p,"seed")) { cfgHaveSeed=1; cfgSeed=(uint32_t)strtoul(v,NULL,0); }
				else if (ieq(p,"patches")) lstrcpynA(cfgPatches,v,MAX_PATH);
			}
			fclose(cf);
			logline("read per-game config %s", cfg);
		}
	}
	if (counter) lstrcpynA(gCounterExpr,counter,256); else if (cfgCounter[0]) lstrcpynA(gCounterExpr,cfgCounter,256);
	if (flips)   lstrcpynA(gFlipsExpr,flips,256);     else if (cfgFlips[0])   lstrcpynA(gFlipsExpr,cfgFlips,256);
	if (pending) lstrcpynA(gPendingExpr,pending,256); else if (cfgPending[0]) lstrcpynA(gPendingExpr,cfgPending,256);
	if (ustate)  lstrcpynA(gStateExpr,ustate,256);     else if (cfgState[0])   lstrcpynA(gStateExpr,cfgState,256);
	if (dempty)  lstrcpynA(gEmptyExpr,dempty,256);     else if (cfgEmpty[0])   lstrcpynA(gEmptyExpr,cfgEmpty,256);
	if (!gPendingExpr[0] != !gStateExpr[0])
		logline("WARNING: --pending-draw and --update-state work only together; with one of them, sandbox shots say \"or an earlier one\"");
	if (gEmptyExpr[0] && !(gPendingExpr[0] && gStateExpr[0])) logline("WARNING: --draw-empty is used only with --pending-draw and --update-state");
	gUpdateMs = updateMs ? updateMs : (cfgUpdateMs ? cfgUpdateMs : 10);
	if (!haveSeed && cfgHaveSeed) { haveSeed=1; seed=cfgSeed; }
	if (gCounterExpr[0]) logline("update counter: %s (one update = %d ms)%s%s%s%s%s%s%s%s", gCounterExpr, gUpdateMs, gFlipsExpr[0]?", flips=":"", gFlipsExpr, gPendingExpr[0]?", pending draw=":"", gPendingExpr, gStateExpr[0]?", update step=":"", gStateExpr, gEmptyExpr[0]?", draw empty=":"", gEmptyExpr);
	if (haveSeed) logline("RNG seed to pin: %u (0x%08X)", seed, seed);

	// --3d: the game's registry key and where it keeps its 3D flag (games.tsv's columns 4 and 5, or the options).
	// The flag is polled whenever it is known, so every run logs the mode the game used; --3d also sets the mode.
	static char key3d[256]="";
	if (!game && exe && !(key3dArg && flag3dArg)) {
		// --exe alone (a copy staged by hand): the games.tsv row with the same exe name gives the 3D columns.
		const char* leaf=strrchr(exe,'\\'); const char* leaf2=strrchr(exe,'/'); if (leaf2>leaf) leaf=leaf2; leaf=leaf?leaf+1:exe;
		char sd[MAX_PATH]={0}, el[MAX_PATH]={0};
		const char* row=lookup_game(NULL,leaf,sd,el,tsvKey3d,tsvFlag3d);
		if (row && (tsvKey3d[0]||tsvFlag3d[0])) logline("--exe %s: games.tsv row '%s' has this exe name; using its 3D key and flag", leaf, row);
	}
	if (key3dArg) lstrcpynA(key3d,key3dArg,256); else lstrcpynA(key3d,tsvKey3d,256);
	if (flag3dArg) lstrcpynA(gMode3dExpr,flag3dArg,256); else lstrcpynA(gMode3dExpr,tsvFlag3d,256);
	if (gMode3dWant>=0 && (!key3d[0] || !gMode3dExpr[0])) {
		logline("--3d %s: no %s known for this game (games.tsv columns 4 and 5, or --3d-key and --3d-flag)%s",
			gMode3dWant?"on":"off",
			(!key3d[0] && !gMode3dExpr[0]) ? "registry key or 3D-flag address" : (!key3d[0] ? "registry key" : "3D-flag address"),
			game?"":"; name its games.tsv row with --game");
		return 2;
	}
	if (gMode3dExpr[0]) logline("3D flag: %s (read as a byte)", gMode3dExpr);

	if (exe) {
		lstrcpynA(gExePath,exe,MAX_PATH);
		for (char* q=gExePath;*q;q++) if(*q=='/')*q='\\';
		lstrcpynA(gGameDir,gExePath,MAX_PATH); char* e=strrchr(gGameDir,'\\'); if(e)*e=0;
	} else {
		if (fresh) { logline("note: --fresh re-copy not deleting existing copy (delete build/origrun/%s manually)",run); }
		if (GetFileAttributesA(gGameDir)==INVALID_FILE_ATTRIBUTES || fresh) {
			mkdirs(gGameDir);   // create the copy's folder and every parent (the stage root may be new)
			logline("copying %s -> %s ...",srcDir,gGameDir);
			copy_tree(srcDir,gGameDir);
		} else logline("reusing existing copy at %s",gGameDir);
		joinp(gExePath,gGameDir,exeLeaf);
		if (srcDir[0]) ensure_steam_dll(srcDir, gGameDir);   // Steam copies: bring Steam.dll in from the library root
	}
	for (char* q=gExePath;*q;q++) if(*q=='/')*q='\\';     // CreateProcess dislikes forward slashes
	for (char* q=gGameDir;*q;q++) if(*q=='/')*q='\\';
	if (GetFileAttributesA(gExePath)==INVALID_FILE_ATTRIBUTES) { logline("copied exe not found: %s",gExePath); return 2; }

	if (copyOnly) {   // just stage the copy (e.g. to carve a wrapped exe into it), then stop.
		logline("--copy-only: copy ready."); logline("  dir: %s", gGameDir); logline("  exe: %s", gExePath);
		return 0;
	}

	// Restore anything a previous run patched, then apply this game's patch list to the copy (never the install).
	// A refused list stops the run, so a wrong list is never run. --no-patches runs the copy unpatched.
	restore_unpatched();
	if (!noPatch) {
		char listPath[MAX_PATH]; listPath[0]=0;
		if (patches) lstrcpynA(listPath,patches,MAX_PATH);
		else if (cfgPatches[0]) {
			if (cfgPatches[1]==':' || cfgPatches[0]=='\\') lstrcpynA(listPath,cfgPatches,MAX_PATH);
			else wsprintfA(listPath,"%s\\tools\\origrun\\%s",repo,cfgPatches);
		}
		else if (game) wsprintfA(listPath,"%s\\tools\\origrun\\patches\\%s.txt",repo,game);
		if (listPath[0] && !apply_patches(listPath, haveSeed, seed)) return 2;
	} else logline("--no-patches: the copy runs unpatched");

	// The DirectDraw/Direct3D 7 wrapper: a previous run's never carries over. --dgvoodoo <dir> places it ("env" reads
	// the folder from ORIGRUN_DGVOODOO), and --3d on places it from ORIGRUN_DGVOODOO when that is set, since 3D cannot
	// run on rundesk's desktop without it.
	if (!remove_wrapper()) return 2;
	{
		const char* env=getenv("ORIGRUN_DGVOODOO"); const char* wrapDir=dgvArg;
		if (wrapDir && ieq(wrapDir,"env")) { wrapDir=env; if (!wrapDir||!*wrapDir) { logline("--dgvoodoo env: ORIGRUN_DGVOODOO is not set"); return 2; } }
		if (!wrapDir && gMode3dWant==1 && env && *env && !sandboxHost) { wrapDir=env; logline("--3d on: using dgVoodoo2 from ORIGRUN_DGVOODOO (%s)", env); }
		if (wrapDir) { if (!place_wrapper(wrapDir, srcDir)) return 2; }
		else if (gMode3dWant==1 && !sandboxHost) logline("note: --3d on without --dgvoodoo: legacy DirectDraw has no hardware on rundesk's desktop, so the game will fall back to software");
		else if (gMode3dWant==1) logline("--3d on: inside the sandbox the game runs on the sandbox's own desktop, without a wrapper%s", sandboxVgpu ? ", with the host's GPU (--sandbox-vgpu)" : "; without --sandbox-vgpu it has only software rendering (WARP)");
	}

	// Place the sound stand-in beside the copied exe (unless real sound was asked for).
	int wantSilent = !ieq(sound,"real");
	if (wantSilent) {
		char src[MAX_PATH],dst[MAX_PATH];
		wsprintfA(src,"%s\\dsound.dll",selfDir);
		wsprintfA(dst,"%s\\dsound.dll",gGameDir);
		if (CopyFileA(src,dst,FALSE)) logline("placed sound stand-in: %s",dst);
		else logline("warning: could not place dsound stand-in from %s (err %lu) -- sound may play",src,GetLastError());
	} else logline("--sound real: no stand-in placed; the game will use the real audio device");

	// --sandbox: everything is staged; the run itself, its registry and its shots happen inside Windows Sandbox.
	if (sandboxHost) return sandbox_host_run(selfDir, script, timeout, sound, key3d, sandboxVgpu, sandboxWait, 0);

	// Environment the stand-in reads, inherited through rundesk.
	SetEnvironmentVariableA("ORIGRUN_DIR",gRunDir);
	SetEnvironmentVariableA("ORIGRUN_SOUND", wantSilent ? sound : "real");

	// The game's 3D setting and cached test results go back after every run whose key is known: on rundesk's desktop
	// the game falls back to software and saves Is3D=0 at exit (and a re-run test caches a failure), which would
	// otherwise turn 3D off for the user's own play. --3d also sets the setting for this run.
	DWORD prev3d=0; int hadPrev3d=0; static RegVals prevTest3d;
	if (key3d[0]) { hadPrev3d = reg_get_is3d(key3d,&prev3d); read_test3d(key3d,&prevTest3d); }
	if (gMode3dWant>=0) {
		if (!reg_set_is3d(key3d,1,(DWORD)gMode3dWant)) { logline("--3d: could not write HKCU\\%s\\%s (err %lu)",key3d,kIs3DValue,GetLastError()); return 2; }
		char was[32]; if (hadPrev3d) wsprintfA(was,"%lu",prev3d); else lstrcpyA(was,"unset");
		logline("--3d %s: set HKCU\\%s\\%s=%d for this run (it was %s)", gMode3dWant?"on":"off", key3d, kIs3DValue, gMode3dWant, was);
	}


	// Launch: rundesk.exe "<copied exe>"   (CWD = game folder, so the game finds its data)
	Launched L;
	if (!launch_game(rundesk,&L)) return 2;
	return watch_run(&L, script, timeout, key3d, hadPrev3d, prev3d, &prevTest3d, logs, logpath);
}

// ------------------------------------------------------------------------------------- games.tsv lookup

// name <TAB> source-folder <TAB> exe-leaf    (lines starting with # ignored). Read beside origrun.exe or
// at tools/origrun/games.tsv in the repo.
// Finds a games.tsv row by its name, or (name NULL) by its exe file name, and returns the row's name.
static const char* lookup_game(const char* name, const char* byExe, char* srcDir, char* exeLeaf, char* regKey, char* flag3d)
{
	static char rowName[64];
	char selfDir[MAX_PATH]; GetModuleFileNameA(NULL,selfDir,MAX_PATH);
	char* s=strrchr(selfDir,'\\'); if(s)*s=0;
	// The repository's table first: a copy beside origrun.exe (an older habit) goes stale when its columns change.
	char tsv[MAX_PATH]; wsprintfA(tsv,"%s\\..\\..\\tools\\origrun\\games.tsv",selfDir);
	FILE* f=fopen(tsv,"r");
	if (!f){ wsprintfA(tsv,"%s\\games.tsv",selfDir); f=fopen(tsv,"r"); }
	if (!f) return NULL;
	char line[1024]; const char* ret=NULL;
	while (fgets(line,sizeof(line),f)) {
		char* nl=strpbrk(line,"\r\n"); if(nl)*nl=0;
		if (line[0]==0||line[0]=='#') continue;
		char* t1=strchr(line,'\t'); if(!t1) continue; *t1=0;
		char* t2=strchr(t1+1,'\t'); if(!t2) continue; *t2=0;
		// optional columns 4 and 5: the game's HKCU registry key and its 3D-flag address ("-" for none)
		char* t3=strchr(t2+1,'\t'); if(t3) *t3=0;
		char* t4=t3?strchr(t3+1,'\t'):NULL; if(t4) *t4=0;
		char* t5=t4?strchr(t4+1,'\t'):NULL; if(t5) *t5=0;
		if (name ? ieq(line,name) : ieq(t2+1,byExe)) {
			lstrcpynA(srcDir,t1+1,MAX_PATH); lstrcpynA(exeLeaf,t2+1,MAX_PATH);
			if (t3 && strcmp(t3+1,"-")) lstrcpynA(regKey,t3+1,256);
			if (t4 && strcmp(t4+1,"-")) lstrcpynA(flag3d,t4+1,256);
			lstrcpynA(rowName,line,sizeof(rowName)); ret=rowName; break;
		}
	}
	fclose(f);
	return ret;
}
