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

// ------------------------------------------------------------------------------------------------- globals

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

static int capture(HWND hwnd, const char* name)
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

	// Try, in order: PrintWindow (GDI and DWM-redirected content), a blit of the window DC, a blit of the
	// desktop's screen DC at the window's rectangle (this thread is attached to the game's desktop).
	const char* how="printwindow";
	BOOL ok = PrintWindow(hwnd, mem, PW_RENDERFULLCONTENT);
	if (!ok || region_flat(px,w,cx0,cy0,cx1,cy1)) {
		HDC wdc=GetWindowDC(hwnd);
		if (wdc){ BitBlt(mem,0,0,w,h,wdc,0,0,SRCCOPY); ReleaseDC(hwnd,wdc); how="windowdc"; }
		if (region_flat(px,w,cx0,cy0,cx1,cy1)) {
			BitBlt(mem,0,0,w,h,screen,r.left,r.top,SRCCOPY); how="screendc";
			if (region_flat(px,w,cx0,cy0,cx1,cy1)) how="flat (DirectDraw/Direct3D content not reachable by GDI)";
		}
	}

	unsigned char* rgb=(unsigned char*)malloc((size_t)w*h*3);
	for (int i=0;i<w*h;i++){ rgb[i*3+0]=px[i*4+2]; rgb[i*3+1]=px[i*4+1]; rgb[i*3+2]=px[i*4+0]; }
	char path[MAX_PATH]; wsprintfA(path,"%s\\%s.png",gShotDir,name);
	int wrote=png_write(path,rgb,w,h);
	free(rgb);
	SelectObject(mem,old); DeleteObject(dib); DeleteDC(mem); ReleaseDC(NULL,screen);
	logline("shot %s -> %s (%dx%d) [%s]", name, path, w, h, how);
	return wrote;
}

// Poll up to timeout_ms for any visible game window, then capture it. Returns 1 if a shot was written.
static int capture_ready(const char* name, int timeout_ms)
{
	int waited=0;
	for (;;) {
		HWND hwnd = resolve_window(0,NULL);
		if (hwnd && capture(hwnd, name)) return 1;   // keep trying if the window died mid-capture
		if (waited >= timeout_ms || gStop) { logline("shot %s skipped (no window within %dms)",name,timeout_ms); return 0; }
		Sleep(100); waited+=100;
	}
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
		HWND hwnd = resolve_window(0,NULL);   // the live main window of any of our processes, fresh each command
		if (!hwnd && !ieq(cmd,"quit")) { logline("(%s) no live window -- skipping",cmd); continue; }
		if (ieq(cmd,"move")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) PostMessageA(hwnd,WM_MOUSEMOVE,0,xy(x,y)); }
		else if (ieq(cmd,"click")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) post_click(hwnd,x,y,0); }
		else if (ieq(cmd,"rclick")){ int x,y; if(sscanf(rest,"%d %d",&x,&y)==2) post_click(hwnd,x,y,1); }
		else if (ieq(cmd,"key")){ char v[32]; if(sscanf(rest,"%31s",v)==1) post_key(hwnd,parse_vk(v)); }
		else if (ieq(cmd,"text")){ post_text(hwnd,rest); }
		else if (ieq(cmd,"focus")){ post_focus(hwnd, ieq(rest,"regain")); logline("focus %s",rest); }
		else if (ieq(cmd,"shot")){ char n[256]="shot"; sscanf(rest,"%255s",n); capture(hwnd,n); }
		else if (ieq(cmd,"quit")){ if(hwnd){ PostMessageA(hwnd,WM_SYSCOMMAND,SC_CLOSE,0); PostMessageA(hwnd,WM_CLOSE,0,0);} logline("quit (posted close)"); break; }
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

// ----------------------------------------------------------------------------------------------- driver

typedef struct { char script[MAX_PATH]; } DriverArg;
static DWORD WINAPI driver_thread(LPVOID p)
{
	DriverArg* a=(DriverArg*)p;
	if (!attach_desktop()) return 0;   // so the game's window handles are usable from this thread
	// Informational only: the game's window can be delayed (activation phone-home) or flaky under load, so we
	// never bail on this -- the script's own waitshot/record commands poll for the window.
	DWORD pid=find_game_pid(20000);
	if (pid) logline("game window first seen: pid=%lu", pid);
	else logline("no window in the first 20s; proceeding anyway (the script polls for it)");
	if (GetEnvironmentVariableA("ORIGRUN_MONITOR",NULL,0) || getenv("ORIGRUN_MONITOR")) {
		for (int t=0;t<30 && !gStop;t++){ DWORD rp=0; HWND rw=resolve_window(0,&rp); logline("--- t+%dms  resolve_window -> hwnd=%p pid=%lu ---",t*1000,(void*)rw,rp); dump_desktop(); Sleep(1000); }
	}
	if (a->script[0]) run_script(pid, a->script);
	else { capture_ready("title", 40000); }
	return 0;
}

// ------------------------------------------------------------------------------------------------- main

static const char* lookup_game(const char* name, char* srcDir, char* exeLeaf);

int main(int argc, char** argv)
{
	crc_init();
	const char* game=NULL; const char* exe=NULL; const char* script=NULL;
	const char* run=NULL; const char* sound="stub"; int timeout=120; int fresh=0; int copyOnly=0;

	for (int i=1;i<argc;i++) {
		if (ieq(argv[i],"--game")&&i+1<argc) game=argv[++i];
		else if (ieq(argv[i],"--exe")&&i+1<argc) exe=argv[++i];
		else if (ieq(argv[i],"--script")&&i+1<argc) script=argv[++i];
		else if (ieq(argv[i],"--run")&&i+1<argc) run=argv[++i];
		else if (ieq(argv[i],"--sound")&&i+1<argc) sound=argv[++i];
		else if (ieq(argv[i],"--timeout")&&i+1<argc) timeout=atoi(argv[++i]);
		else if (ieq(argv[i],"--copy-only")) copyOnly=1;
		else if (ieq(argv[i],"--fresh")) fresh=1;
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
	char rundesk[MAX_PATH]; wsprintfA(rundesk,"%s\\build\\native\\rundesk.exe",repo);
	if (GetFileAttributesA(rundesk)==INVALID_FILE_ATTRIBUTES)
		{ logline("rundesk not found at %s -- build it: cmake --build build/native --target rundesk", rundesk); return 2; }

	if (!run) run = game ? game : "run";
	wsprintfA(gRunDir,"%s\\build\\origrun\\%s",repo,run);
	joinp(gGameDir,gRunDir,"game");
	joinp(gShotDir,gRunDir,"shots");

	// Resolve source and exe.
	char srcDir[MAX_PATH]={0}, exeLeaf[MAX_PATH]={0};
	if (game) {
		if (!lookup_game(game,srcDir,exeLeaf)) { logline("unknown game '%s' -- add it to tools/origrun/games.tsv or use --exe",game); return 2; }
		if (GetFileAttributesA(srcDir)==INVALID_FILE_ATTRIBUTES) { logline("source folder not found: %s",srcDir); return 2; }
	}

	mkdirs(gRunDir); mkdirs(gShotDir);
	char logs[MAX_PATH]; joinp(logs,gRunDir,"logs"); mkdirs(logs);
	InitializeCriticalSection(&gLogLock); gLogLockInit=1;
	char logpath[MAX_PATH]; joinp(logpath,logs,"origrun.log"); gLogFile=fopen(logpath,"w");

	if (exe) {
		lstrcpynA(gExePath,exe,MAX_PATH);
		for (char* q=gExePath;*q;q++) if(*q=='/')*q='\\';
		lstrcpynA(gGameDir,gExePath,MAX_PATH); char* e=strrchr(gGameDir,'\\'); if(e)*e=0;
	} else {
		if (fresh) { logline("note: --fresh re-copy not deleting existing copy (delete build/origrun/%s manually)",run); }
		if (GetFileAttributesA(gGameDir)==INVALID_FILE_ATTRIBUTES || fresh) {
			logline("copying %s -> %s ...",srcDir,gGameDir);
			copy_tree(srcDir,gGameDir);
		} else logline("reusing existing copy at %s",gGameDir);
		joinp(gExePath,gGameDir,exeLeaf);
	}
	for (char* q=gExePath;*q;q++) if(*q=='/')*q='\\';     // CreateProcess dislikes forward slashes
	for (char* q=gGameDir;*q;q++) if(*q=='/')*q='\\';
	if (GetFileAttributesA(gExePath)==INVALID_FILE_ATTRIBUTES) { logline("copied exe not found: %s",gExePath); return 2; }

	if (copyOnly) {   // just stage the copy (e.g. to carve a wrapped exe into it), then stop.
		logline("--copy-only: copy ready."); logline("  dir: %s", gGameDir); logline("  exe: %s", gExePath);
		return 0;
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

	// Environment the stand-in reads, inherited through rundesk.
	SetEnvironmentVariableA("ORIGRUN_DIR",gRunDir);
	SetEnvironmentVariableA("ORIGRUN_SOUND", wantSilent ? sound : "real");

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
	CloseHandle(pi.hThread); CloseHandle(pi.hProcess);

	logline("done. captures in %s, logs in %s", gShotDir, logs);
	logline(gAudioSeen ? "RESULT: an audio session appeared and was muted (see warning above)."
	                   : "RESULT: no audio session from the game was seen.");
	return 0;
}

// ------------------------------------------------------------------------------------- games.tsv lookup

// name <TAB> source-folder <TAB> exe-leaf    (lines starting with # ignored). Read beside origrun.exe or
// at tools/origrun/games.tsv in the repo.
static const char* lookup_game(const char* name, char* srcDir, char* exeLeaf)
{
	char selfDir[MAX_PATH]; GetModuleFileNameA(NULL,selfDir,MAX_PATH);
	char* s=strrchr(selfDir,'\\'); if(s)*s=0;
	char tsv[MAX_PATH]; wsprintfA(tsv,"%s\\games.tsv",selfDir);
	FILE* f=fopen(tsv,"r");
	if (!f){ wsprintfA(tsv,"%s\\..\\..\\tools\\origrun\\games.tsv",selfDir); f=fopen(tsv,"r"); }
	if (!f) return NULL;
	char line[1024]; const char* ret=NULL;
	while (fgets(line,sizeof(line),f)) {
		char* nl=strpbrk(line,"\r\n"); if(nl)*nl=0;
		if (line[0]==0||line[0]=='#') continue;
		char* t1=strchr(line,'\t'); if(!t1) continue; *t1=0;
		char* t2=strchr(t1+1,'\t'); if(!t2) continue; *t2=0;
		if (ieq(line,name)) { lstrcpynA(srcDir,t1+1,MAX_PATH); lstrcpynA(exeLeaf,t2+1,MAX_PATH); ret="ok"; break; }
	}
	fclose(f);
	return ret;
}
