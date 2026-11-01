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
// Build: tools/origrun/build.sh (llvm-mingw i686). Written from the public Win32 API; no other project's
// DirectDraw or DirectSound wrapper was read.

#define COBJMACROS
#define INITGUID
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif
#include <shlobj.h>
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
	size_t o=0;
	for (int y=0;y<h;y++){ raw[o++]=0; memcpy(raw+o, rgb+(size_t)y*w*3, (size_t)w*3); o+=(size_t)w*3; }

	// zlib stream: 2-byte header, stored DEFLATE blocks, Adler-32.
	size_t zcap = raw_len + raw_len/65535 + 64;
	unsigned char* z = (unsigned char*)malloc(zcap);
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
		gDesk = OpenDesktopA("bejeweled-test", 0, FALSE,
			DESKTOP_ENUMERATE|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS|DESKTOP_CREATEWINDOW|DESKTOP_HOOKCONTROL|STANDARD_RIGHTS_REQUIRED);
		if (!gDesk) gDesk = OpenDesktopA("bejeweled-test", 0, FALSE, DESKTOP_ENUMERATE|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS);
		if (gDesk) break;
		Sleep(200);
	}
	if (!gDesk) { logline("could not open the game desktop (err %lu)",GetLastError()); return 0; }
	if (!SetThreadDesktop(gDesk)) { logline("SetThreadDesktop failed (err %lu)",GetLastError()); return 0; }
	return 1;
}

static HWND resolve_window(DWORD pid, DWORD* outPid)
{
	HDESK d = gDesk ? gDesk : OpenDesktopA("bejeweled-test", 0, FALSE, DESKTOP_ENUMERATE|DESKTOP_READOBJECTS);
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
	HDESK d=OpenDesktopA("bejeweled-test",0,FALSE,DESKTOP_ENUMERATE|DESKTOP_READOBJECTS);
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

// allowPrint==0 skips PrintWindow (which sends WM_PRINT to the game's window thread): a capture taken while
// that thread is held by SuspendThread (the "at" path) must not message it, or it would deadlock. The window-DC
// and screen-DC blits read the window's pixels without messaging the game, so they are safe while it is held.
static int capture_ex(HWND hwnd, const char* name, int allowPrint)
{
	RECT r; if (!hwnd || !GetWindowRect(hwnd,&r)) return 0;   // window gone; caller may retry
	int w=r.right-r.left, h=r.bottom-r.top;
	if (w<=0||h<=0) return 0;
	HDC screen=GetDC(NULL); HDC mem=CreateCompatibleDC(screen);
	BITMAPINFO bi; ZeroMemory(&bi,sizeof(bi));
	bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth=w; bi.bmiHeader.biHeight=-h;   // top-down
	bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
	void* bits=NULL;
	HBITMAP dib=CreateDIBSection(mem,&bi,DIB_RGB_COLORS,&bits,NULL,0);
	HGDIOBJ old=SelectObject(mem,dib);

	unsigned char* px=(unsigned char*)bits;

	// The client area in window coordinates: a game that draws with DirectDraw leaves PrintWindow's copy of its
	// client area a single flat colour (the class background), so test that region, not the title bar.
	POINT o={0,0}; ClientToScreen(hwnd,&o); RECT cr={0}; GetClientRect(hwnd,&cr);
	int cx0=o.x-r.left, cy0=o.y-r.top, cx1=cx0+cr.right, cy1=cy0+cr.bottom;
	if (cx0<0) cx0=0; if (cy0<0) cy0=0; if (cx1>w) cx1=w; if (cy1>h) cy1=h;

	// Try, in order: PrintWindow (GDI and DWM-redirected content), then a blit of the window DC. There is no
	// screen-DC fallback: a blit of the desktop at the window's rectangle can return what is on the user's own
	// screen (a flip-model window's DC once did), so a window neither path reaches stays flat.
	const char* how="printwindow";
	BOOL ok = allowPrint ? PrintWindow(hwnd, mem, PW_RENDERFULLCONTENT) : FALSE;
	if (!allowPrint || !ok || region_flat(px,w,cx0,cy0,cx1,cy1)) {
		HDC wdc=GetWindowDC(hwnd);
		if (wdc){ BitBlt(mem,0,0,w,h,wdc,0,0,SRCCOPY); ReleaseDC(hwnd,wdc); how="windowdc"; }
		if (region_flat(px,w,cx0,cy0,cx1,cy1)) how="flat (DirectDraw/Direct3D content not reachable by GDI)";
	}
	int flat = region_flat(px,w,cx0,cy0,cx1,cy1);

	unsigned char* rgb=(unsigned char*)malloc((size_t)w*h*3);
	for (int i=0;i<w*h;i++){ rgb[i*3+0]=px[i*4+2]; rgb[i*3+1]=px[i*4+1]; rgb[i*3+2]=px[i*4+0]; }
	char path[MAX_PATH]; wsprintfA(path,"%s\\%s.png",gShotDir,name);
	int wrote=png_write(path,rgb,w,h);
	free(rgb);
	SelectObject(mem,old); DeleteObject(dib); DeleteDC(mem); ReleaseDC(NULL,screen);
	logline("shot %s -> %s (%dx%d) [%s]", name, path, w, h, how);
	return wrote ? (flat ? 2 : 1) : 0;   // 2 = written but flat (a DirectDraw screen, or not drawn yet)
}
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
		if (waited >= timeout_ms || gStop) {
			if (last==2) logline("shot %s: content stayed flat for %dms (DirectDraw not reachable by GDI, or a solid screen)",name,timeout_ms);
			else if (!last) logline("shot %s skipped (no window within %dms)",name,timeout_ms);
			return last;
		}
		Sleep(200); waited+=200;
	}
}

// ---------------------------------------------------------------------------------- bursts, clips, "at N"

static double qpc_ms(void){ LARGE_INTEGER c,f; QueryPerformanceCounter(&c); QueryPerformanceFrequency(&f); return (double)c.QuadPart*1000.0/(double)f.QuadPart; }

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
static int capture_held(const char* name)
{
	HWND hw=resolve_window(0,NULL); if(!hw){ logline("at: no window for '%s'",name); return 0; }
	if (gFlipsExpr[0]) { uint32_t f0; if (read_counter(gFlipsExpr,&f0)) { uint32_t f=f0; int w=0; while(f<=f0 && w<100 && !gStop){ Sleep(1); read_counter(gFlipsExpr,&f); w++; } } }  // a static screen presents nothing new, so cap the wait
	HANDLE th=open_window_thread(hw,THREAD_SUSPEND_RESUME);
	if (th) SuspendThread(th);
	int ok=capture_ex(hw,name,0);   // no PrintWindow while the thread is held
	if (th){ ResumeThread(th); CloseHandle(th); }
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
	double capSum=0, capMax=0; int taken=0;
	double t0=qpc_ms();
	for (int i=0;i<count && !gStop;i++) {
		char leaf[320]; wsprintfA(leaf,"%s/%04d",name,i);   // written under shots/<name>/
		double frameStart=qpc_ms();
		int ok; double cap0;
		if (byUpdates) { if (wait_counter(start + (uint32_t)i*k, 4000)<0){ logline("burst %s: counter unreadable, stopping",name); break; } cap0=qpc_ms(); ok=capture_held(leaf); }
		else { HWND hw=resolve_window(0,NULL); if(!hw){ logline("burst %s: no window, stopping at %d",name,i); break; } cap0=qpc_ms(); ok=capture(hw,leaf); }
		double dt=qpc_ms()-cap0;   // the capture itself, not any wait before it
		if (ok){ taken++; capSum+=dt; if(dt>capMax)capMax=dt; }
		if (!byUpdates) { double rest=interval_ms-(qpc_ms()-frameStart); if (rest>1) Sleep((DWORD)rest); }
	}
	int clipMs = byUpdates ? (k*gUpdateMs>0?k*gUpdateMs:interval_ms) : interval_ms;
	write_clip_json(dir, clipMs);
	logline("burst %s: %d frames in %s (clip interval %d ms); capture avg %.1f ms, max %.1f ms over %.0f ms wall",
		name, taken, dir, clipMs, taken?capSum/taken:0.0, capMax, qpc_ms()-t0);
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
static void exec_at(const char* inner)
{
	char cmd[32]={0}; int off=0; sscanf(inner,"%31s%n",cmd,&off); const char* rest=inner+off; while(*rest==' ')rest++;
	if (ieq(cmd,"shot")){ char n[256]="shot"; sscanf(rest,"%255s",n); capture_held(n); return; }
	if (ieq(cmd,"burst")){ char n[256]="burst"; int c=10,k=1; char kw[16]=""; int iv=100;
		if (sscanf(rest,"%255s %d %15s %d",n,&c,kw,&iv)>=2) { if (ieq(kw,"update")) do_burst(n,c,0,1,iv>0?iv:1); else do_burst(n,c,iv>0?iv:atoi(kw),0,0); }
		return; }
	HWND hw=resolve_window(0,NULL); if(!hw){ logline("at: no window for '%s'",inner); return; }
	HANDLE th=open_window_thread(hw,THREAD_SUSPEND_RESUME); if(th) SuspendThread(th);
	if (ieq(cmd,"click")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) post_click(hw,x,y,0); }
	else if (ieq(cmd,"rclick")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) post_click(hw,x,y,1); }
	else if (ieq(cmd,"key")){ char v[32]; if(sscanf(rest,"%31s",v)==1) post_key(hw,parse_vk(v)); }
	else if (ieq(cmd,"text")){ post_text(hw,rest); }
	else if (ieq(cmd,"move")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) PostMessageA(hw,WM_MOUSEMOVE,0,xy(x,y)); }
	else if (ieq(cmd,"focus")){ post_focus(hw, ieq(rest,"regain")); }
	else logline("at: cannot do '%s' at an update",cmd);
	if (th){ ResumeThread(th); CloseHandle(th); }
}

// ----------------------------------------------------------------------------------------- input posting

static LPARAM xy(int x,int y){ return (LPARAM)((y<<16)|(x&0xffff)); }

// Map a point given in the top window's client coords to the deepest child window under it, returning that
// window and the point in its own client coords. This lets a click land on a dialog's child button control
// (the PopCap trial dialog's "Play") as well as on a game that draws and hit-tests in its own window.
static HWND child_at(HWND top, int x, int y, POINT* outClient)
{
	POINT pt={x,y}; ClientToScreen(top,&pt);
	HWND tgt=WindowFromPoint(pt); if(!tgt) tgt=top;
	POINT cp=pt; ScreenToClient(tgt,&cp);
	*outClient=cp; return tgt;
}

static void post_click(HWND hwnd,int x,int y,int right)
{
	POINT cp; HWND t=child_at(hwnd,x,y,&cp); LPARAM lp=xy(cp.x,cp.y);
	PostMessageA(t, WM_MOUSEMOVE, 0, lp); Sleep(20);
	if (right){ PostMessageA(t,WM_RBUTTONDOWN,MK_RBUTTON,lp); Sleep(40); PostMessageA(t,WM_RBUTTONUP,0,lp); }
	else      { PostMessageA(t,WM_LBUTTONDOWN,MK_LBUTTON,lp); Sleep(40); PostMessageA(t,WM_LBUTTONUP,0,lp); }
}
static void post_key(HWND hwnd,int vk)
{
	UINT sc=MapVirtualKeyA(vk,MAPVK_VK_TO_VSC);
	LPARAM down=(LPARAM)(1|(sc<<16));
	LPARAM up=(LPARAM)(1|(sc<<16)|(1u<<30)|(1u<<31));
	PostMessageA(hwnd,WM_KEYDOWN,vk,down); Sleep(20); PostMessageA(hwnd,WM_KEYUP,vk,up);
}
static void post_text(HWND hwnd,const char* s){ for(;*s;s++){ PostMessageA(hwnd,WM_CHAR,(unsigned char)*s,1); Sleep(10);} }
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
static void hw_move(HWND hwnd,int x,int y)
{
	POINT pt={x,y}; ClientToScreen(hwnd,&pt); SetCursorPos(pt.x,pt.y);
	INPUT in; ZeroMemory(&in,sizeof(in)); in.type=INPUT_MOUSE; in.mi.dwFlags=MOUSEEVENTF_MOVE; SendInput(1,&in,sizeof(INPUT));
}
static void hw_click(HWND hwnd,int x,int y,int right)
{
	hw_move(hwnd,x,y); Sleep(20);
	INPUT in[2]; ZeroMemory(in,sizeof(in));
	in[0].type=INPUT_MOUSE; in[0].mi.dwFlags = right?MOUSEEVENTF_RIGHTDOWN:MOUSEEVENTF_LEFTDOWN;
	in[1].type=INPUT_MOUSE; in[1].mi.dwFlags = right?MOUSEEVENTF_RIGHTUP:MOUSEEVENTF_LEFTUP;
	SendInput(1,&in[0],sizeof(INPUT)); Sleep(40); SendInput(1,&in[1],sizeof(INPUT));
}
static void hw_key(int vk)
{
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
							if (is_our_pid(pid)) {
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
static HKEY open_game_key(const char* sub)
{
	HKEY k=NULL;
	if (RegCreateKeyExA(HKEY_CURRENT_USER,hkcu_sub(sub),0,NULL,0,KEY_QUERY_VALUE|KEY_SET_VALUE,NULL,&k,NULL)!=ERROR_SUCCESS) return NULL;
	return k;
}

// The game's cached Direct3D test results, the "Test3D" subkey D3DTester keeps beside Is3D. On rundesk's desktop
// DirectDraw has no hardware, so a test that re-runs there fails and caches the failure, and the game then refuses
// 3D on the user's own desktop too. origrun snapshots the values before a run and puts them back after it.
typedef struct { char name[64]; DWORD type; BYTE data[512]; DWORD len; } RegVal;
typedef struct { int n; RegVal v[16]; } RegVals;   // n = -1: the subkey does not exist
static void read_test3d(const char* sub, RegVals* out)
{
	char path[600]; wsprintfA(path,"%s\\Test3D",hkcu_sub(sub));
	HKEY k; out->n=-1;
	if (RegOpenKeyExA(HKEY_CURRENT_USER,path,0,KEY_QUERY_VALUE,&k)!=ERROR_SUCCESS) return;
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
	char path[600]; wsprintfA(path,"%s\\Test3D",hkcu_sub(sub));
	RegDeleteKeyA(HKEY_CURRENT_USER,path);
	if (before->n>=0) {
		HKEY k;
		if (RegCreateKeyExA(HKEY_CURRENT_USER,path,0,NULL,0,KEY_SET_VALUE,NULL,&k,NULL)==ERROR_SUCCESS) {
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

// ----------------------------------------------------------------------------------------------- driver

typedef struct { char script[MAX_PATH]; } DriverArg;
static DWORD WINAPI driver_thread(LPVOID p)
{
	DriverArg* a=(DriverArg*)p;
	if (!attach_desktop()) return 0;   // so the game's window handles are usable from this thread
	// Informational only: the game's window can be delayed (activation phone-home) or flaky under load, so we
	// never bail on this -- the script's own waitshot/record commands poll for the window.
	DWORD pid=find_game_pid(20000);
	if (pid) { gGamePidHint=pid; logline("game window first seen: pid=%lu", pid); }
	else logline("no window in the first 20s; proceeding anyway (the script polls for it)");
	if (GetEnvironmentVariableA("ORIGRUN_MONITOR",NULL,0) || getenv("ORIGRUN_MONITOR")) {
		for (int t=0;t<30 && !gStop;t++){ DWORD rp=0; HWND rw=resolve_window(0,&rp); logline("--- t+%dms  resolve_window -> hwnd=%p pid=%lu ---",t*1000,(void*)rw,rp); dump_desktop(); Sleep(1000); }
	}
	if (a->script[0]) run_script(pid, a->script);
	else { capture_ready("title", 40000); }
	return 0;
}

// ------------------------------------------------------------------------------------------------- main

static const char* lookup_game(const char* name, char* srcDir, char* exeLeaf, char* regKey, char* flag3d);

int main(int argc, char** argv)
{
	crc_init();
	InitializeCriticalSection(&gRpmLock);
	const char* game=NULL; const char* exe=NULL; const char* script=NULL;
	const char* run=NULL; const char* sound="stub"; int timeout=120; int fresh=0; int copyOnly=0;
	const char* patches=NULL; const char* counter=NULL; const char* flips=NULL;
	int haveSeed=0; uint32_t seed=0; int updateMs=0; int noPatch=0; const char* stageRoot=NULL;
	const char* key3dArg=NULL; const char* flag3dArg=NULL;

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
		else { fprintf(stderr,"unknown or incomplete option: %s\n",argv[i]); return 2; }
	}
	if (!game && !exe) die("need --game <name> or --exe <path>");
	if (timeout>240) timeout=240;   // the machine rule: nothing longer than about 4 minutes

	// where the tool and rundesk live (origrun.exe sits in build/origrun-bin/)
	char selfDir[MAX_PATH]; GetModuleFileNameA(NULL,selfDir,MAX_PATH);
	char* s=strrchr(selfDir,'\\'); if(s)*s=0;
	char repoRaw[MAX_PATH], repo[MAX_PATH];
	wsprintfA(repoRaw,"%s\\..\\..",selfDir);                    // build/origrun-bin -> repo root
	GetFullPathNameA(repoRaw, MAX_PATH, repo, NULL);           // canonicalize (drop the ..\..)
	lstrcpynA(gRepo, repo, MAX_PATH);
	char rundesk[MAX_PATH]; wsprintfA(rundesk,"%s\\build\\native\\rundesk.exe",repo);
	if (GetFileAttributesA(rundesk)==INVALID_FILE_ATTRIBUTES)
		{ logline("rundesk not found at %s -- build it: cmake --build build/native --target rundesk", rundesk); return 2; }

	if (!run) run = game ? game : "run";
	wsprintfA(gRunDir,"%s\\build\\origrun\\%s",repo,run);
	joinp(gShotDir,gRunDir,"shots");
	// The game copy goes under gRunDir by default; --stage-root puts it under a SHORT path instead (shots and
	// logs stay under gRunDir). A worktree's build/origrun path is long, and some launchers (Bookworm Adventures)
	// exit at once when their folder sits that deep, so a worktree session stages the copy under, e.g., main's
	// build/origrun or a short drive-root folder.
	if (stageRoot && stageRoot[0]) { char sr[MAX_PATH]; lstrcpynA(sr,stageRoot,MAX_PATH); for(char* q=sr;*q;q++) if(*q=='/')*q='\\';
		wsprintfA(gGameDir,"%s\\%s\\game",sr,run); }
	else joinp(gGameDir,gRunDir,"game");

	// Resolve source and exe.
	char srcDir[MAX_PATH]={0}, exeLeaf[MAX_PATH]={0}, tsvKey3d[256]={0}, tsvFlag3d[256]={0};
	if (game) {
		if (!lookup_game(game,srcDir,exeLeaf,tsvKey3d,tsvFlag3d)) { logline("unknown game '%s' -- add it to tools/origrun/games.tsv or use --exe",game); return 2; }
		if (GetFileAttributesA(srcDir)==INVALID_FILE_ATTRIBUTES) { logline("source folder not found: %s",srcDir); return 2; }
	}

	mkdirs(gRunDir); mkdirs(gShotDir);
	char logs[MAX_PATH]; joinp(logs,gRunDir,"logs"); mkdirs(logs);
	InitializeCriticalSection(&gLogLock); gLogLockInit=1;
	char logpath[MAX_PATH]; joinp(logpath,logs,"origrun.log"); gLogFile=fopen(logpath,"w");

	// Per-game config (git-ignored, beside games.tsv): lines "counter=", "flips=", "update_ms=", "seed=",
	// "patches=". CLI options override it. It records a game's update-counter address and patch-list path so a
	// session need not repeat them on every run.
	static char cfgCounter[256]="", cfgFlips[256]="", cfgPatches[MAX_PATH]="";
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
	gUpdateMs = updateMs ? updateMs : (cfgUpdateMs ? cfgUpdateMs : 10);
	if (!haveSeed && cfgHaveSeed) { haveSeed=1; seed=cfgSeed; }
	if (gCounterExpr[0]) logline("update counter: %s (one update = %d ms)%s%s", gCounterExpr, gUpdateMs, gFlipsExpr[0]?", flips=":"", gFlipsExpr);
	if (haveSeed) logline("RNG seed to pin: %u (0x%08X)", seed, seed);

	// --3d: the game's registry key and where it keeps its 3D flag (games.tsv's columns 4 and 5, or the options).
	// The flag is polled whenever it is known, so every run logs the mode the game used; --3d also sets the mode.
	static char key3d[256]="";
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

	// Place the sound stand-in beside the copied exe (unless real sound was asked for).
	int wantSilent = !ieq(sound,"real");
	if (wantSilent) {
		char src[MAX_PATH],dst[MAX_PATH];
		wsprintfA(src,"%s\\dsound.dll",selfDir);
		wsprintfA(dst,"%s\\dsound.dll",gGameDir);
		if (CopyFileA(src,dst,FALSE)) logline("placed sound stand-in: %s",dst);
		else logline("warning: could not place dsound stand-in from %s (err %lu) -- sound may play",src,GetLastError());
	} else logline("--sound real: no stand-in placed; the game will use the real audio device");

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
	char cmd[MAX_PATH*2]; wsprintfA(cmd,"\"%s\" \"%s\"",rundesk,gExePath);
	STARTUPINFOA si; ZeroMemory(&si,sizeof(si)); si.cb=sizeof(si);
	PROCESS_INFORMATION pi; ZeroMemory(&pi,sizeof(pi));
	logline("launching under rundesk: %s",gExePath);
	if (!CreateProcessA(NULL,cmd,NULL,NULL,FALSE,0,NULL,gGameDir,&si,&pi))
		{ logline("CreateProcess(rundesk) failed (err %lu)",GetLastError()); return 2; }

	if (script) load_msgbox_rules(script);   // register dialog answers before any dialog can appear
	HANDLE ath=CreateThread(NULL,0,audio_thread,NULL,0,NULL);
	DriverArg da; ZeroMemory(&da,sizeof(da)); if(script) lstrcpynA(da.script,script,MAX_PATH);
	HANDLE dth=CreateThread(NULL,0,driver_thread,&da,0,NULL);
	HANDLE gth=CreateThread(NULL,0,dialog_thread,NULL,0,NULL);   // log/answer message boxes on the game desktop
	HANDLE mth=gMode3dExpr[0] ? CreateThread(NULL,0,mode3d_thread,NULL,0,NULL) : NULL;   // the game's 3D mode

	// Wait for the game (rundesk returns when the whole job ends), or hit the timeout.
	DWORD w=WaitForSingleObject(pi.hProcess, (DWORD)timeout*1000);
	if (w==WAIT_TIMEOUT) {
		logline("timeout (%ds) -- ending the run (rundesk's job kills the game and its children)",timeout);
		TerminateProcess(pi.hProcess,1);   // only our rundesk child; its job kill-on-close ends the game
		WaitForSingleObject(pi.hProcess,5000);
	} else logline("game exited");

	InterlockedExchange(&gStop,1);
	WaitForSingleObject(dth,3000);
	WaitForSingleObject(gth,3000);
	WaitForSingleObject(ath,3000);
	if (mth) WaitForSingleObject(mth,3000);
	CloseHandle(pi.hThread); CloseHandle(pi.hProcess);

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
		restore_test3d(key3d,&prevTest3d);
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

// ------------------------------------------------------------------------------------- games.tsv lookup

// name <TAB> source-folder <TAB> exe-leaf    (lines starting with # ignored). Read beside origrun.exe or
// at tools/origrun/games.tsv in the repo.
static const char* lookup_game(const char* name, char* srcDir, char* exeLeaf, char* regKey, char* flag3d)
{
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
		if (ieq(line,name)) {
			lstrcpynA(srcDir,t1+1,MAX_PATH); lstrcpynA(exeLeaf,t2+1,MAX_PATH);
			if (t3 && strcmp(t3+1,"-")) lstrcpynA(regKey,t3+1,256);
			if (t4 && strcmp(t4+1,"-")) lstrcpynA(flag3d,t4+1,256);
			ret="ok"; break;
		}
	}
	fclose(f);
	return ret;
}
