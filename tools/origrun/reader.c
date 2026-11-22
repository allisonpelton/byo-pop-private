// origrun's in-game frame reader, inside the game (docs/ORIGRUN.md, "The in-game frame reader"). Built into the dsound
// stand-in beside the virtual clock, and run only by the clock's held main thread (vclock.c, vc_hold), when origrun asks
// for a read through the shared block (vclock.h). The user approved shots of an original outside Windows Sandbox by an
// in-game reader only, "Yes, with safeguards" (2026-11-22, docs/RULES.md).
//
// What it reads: the game's own draw surface, the off-screen DirectDraw surface the game created to draw its frames into
// (a SexyAppFramework game's DDInterface mDrawSurface), found through the pointer expression origrun passes in
// ORIGRUN_READER_SURFACE (the per-game config's reader_surface=). It never touches the primary surface, the screen, a
// window's DC or anything another program drew. It hooks nothing: it calls the surface's own methods (QueryInterface,
// GetSurfaceDesc, Lock read-only, Unlock, Release) in the game's own thread while that thread is held in one of its waits,
// where origrun has checked that the game is between updates and that its draw surface holds the frame it last presented.
//
// Its checks, each a refusal with its own status (RD_*) that origrun logs: the expression reads a non-null pointer to
// committed, readable memory; the pointer's vtable and the methods called lie in the system's ddraw.dll (not a wrapper's,
// not a shim's); the interface gives IDirectDrawSurface7; the caps include DDSCAPS_OFFSCREENPLAIN and none of
// DDSCAPS_PRIMARYSURFACE, _FRONTBUFFER, _BACKBUFFER, _FLIP, _COMPLEX, _OVERLAY or _VISIBLE; the pixel format is RGB at 16,
// 24 or 32 bits with three masks (no palette, no FOURCC); the frame fits the pixel block; and the Lock gives the same size.
// origrun checks the size against the game's client area, the update counter and the caps again before it writes a PNG.
// Written from the public DirectDraw documentation.

#include <windows.h>
#include <ddraw.h>
#include <string.h>
#include "vclock.h"

int vc_read_expr(const char* e, DWORD* out);   // vclock.c: the counter grammar, read in the game's own memory
int vc_read_counter(DWORD* out);               // vclock.c: the update counter
void vc_log(const char* fmt, ...);             // vclock.c: the clock's log

static const GUID kIID_IDirectDrawSurface7 = {0x06675a80,0x3b9b,0x11d2,{0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b}};
static char      gSurfExpr[256];
static int       gSurfInit;
static HANDLE    gPixMap;
static BYTE*     gPix;
static HMODULE   gDdraw;            // the system's ddraw.dll, as loaded in the game
static char      gPixName[200];

#define CAPS_REFUSED (DDSCAPS_PRIMARYSURFACE|DDSCAPS_FRONTBUFFER|DDSCAPS_BACKBUFFER|DDSCAPS_FLIP|DDSCAPS_COMPLEX|DDSCAPS_OVERLAY|DDSCAPS_VISIBLE)

// The block's name and the expression, from the environment origrun gave the game (vclock.c calls this at the install).
void reader_init(const char* blockName)
{
	gSurfExpr[0] = 0;
	GetEnvironmentVariableA("ORIGRUN_READER_SURFACE", gSurfExpr, sizeof(gSurfExpr));
	wsprintfA(gPixName, "%s-pixels", blockName);
	gSurfInit = 1;
	if (gSurfExpr[0]) vc_log("reader: on; the draw surface is %s, read only when origrun asks while the game is held", gSurfExpr);
}

static int readable(const void* p, SIZE_T n)
{
	MEMORY_BASIC_INFORMATION m;
	if (!p || VirtualQuery(p, &m, sizeof(m)) != sizeof(m) || m.State != MEM_COMMIT) return 0;
	if (m.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return 0;
	return (const BYTE*)p + n <= (const BYTE*)m.BaseAddress + m.RegionSize;
}
// The system's ddraw.dll, and only that: a ddraw.dll from anywhere else (a wrapper in the copy's folder) is not it.
static HMODULE system_ddraw(void)
{
	if (gDdraw) return gDdraw;
	HMODULE m = GetModuleHandleA("ddraw.dll");
	if (!m) return NULL;
	char path[MAX_PATH], sys[MAX_PATH], wow[MAX_PATH];
	if (!GetModuleFileNameA(m, path, MAX_PATH)) return NULL;
	char* leaf = strrchr(path, '\\'); if (!leaf) return NULL;
	*leaf = 0;
	UINT n1 = GetSystemDirectoryA(sys, MAX_PATH), n2 = GetSystemWow64DirectoryA(wow, MAX_PATH);
	if (!((n1 && lstrcmpiA(path, sys) == 0) || (n2 && n2 < MAX_PATH && lstrcmpiA(path, wow) == 0))) return NULL;
	gDdraw = m;
	return m;
}
// p lies in the image of module m
static int in_module(const void* p, HMODULE m)
{
	MEMORY_BASIC_INFORMATION i;
	return p && VirtualQuery(p, &i, sizeof(i)) == sizeof(i) && i.Type == MEM_IMAGE && (HMODULE)i.AllocationBase == m;
}
// an interface whose vtable, and the given slots of it, are ddraw.dll's
static int ddraw_iface(void* obj, HMODULE dd, const int* slots, int n)
{
	if (!readable(obj, 4)) return 0;
	void** vt = *(void***)obj;
	if (!readable(vt, 4 * 40) || !in_module(vt, dd)) return 0;
	for (int i = 0; i < n; i++) if (!in_module(vt[slots[i]], dd)) return 0;
	return 1;
}
static int mask_bits(DWORD m) { int n = 0; for (; m; m &= m - 1) n++; return n; }

// One read, answered in the shared block. The caller is the held driver thread.
void reader_serve(VcShared* s)
{
	LONG req = s->readReq;
	s->readStatus = RD_NONE; s->readHr = 0; s->readSurface = 0; s->readCaps = 0;
	s->readW = s->readH = s->readPitch = s->readBpp = 0; s->readBytes = 0; s->readCounter = -1;
	s->readPfFlags = s->readRMask = s->readGMask = s->readBMask = 0;
	int st = RD_OK;
	IDirectDrawSurface7* s7 = NULL;
	if (!gSurfInit || !gSurfExpr[0]) { st = RD_NOEXPR; goto done; }
	DWORD ptr = 0;
	if (!vc_read_expr(gSurfExpr, &ptr) || !ptr) { st = RD_NOPTR; goto done; }
	s->readSurface = ptr;
	HMODULE dd = system_ddraw();
	static const int kQi[] = { 0 };
	if (!dd || !ddraw_iface((void*)(DWORD_PTR)ptr, dd, kQi, 1)) { st = RD_NOTDDRAW; goto done; }
	IUnknown* u = (IUnknown*)(DWORD_PTR)ptr;
	HRESULT hr = u->lpVtbl->QueryInterface(u, &kIID_IDirectDrawSurface7, (void**)&s7);
	if (hr != S_OK || !s7) { s7 = NULL; s->readHr = hr; st = RD_QI; goto done; }
	// the slots called: QueryInterface 0, Release 2, GetSurfaceDesc 22, Lock 25, Unlock 32
	static const int kSlots[] = { 0, 2, 22, 25, 32 };
	if (!ddraw_iface(s7, dd, kSlots, 5)) { st = RD_NOTDDRAW; goto done; }
	DDSURFACEDESC2 d; memset(&d, 0, sizeof(d)); d.dwSize = sizeof(d);
	hr = IDirectDrawSurface7_GetSurfaceDesc(s7, &d);
	if (hr != DD_OK) { s->readHr = hr; st = RD_DESC; goto done; }
	DWORD caps = d.ddsCaps.dwCaps;
	s->readCaps = caps; s->readW = (LONG)d.dwWidth; s->readH = (LONG)d.dwHeight;
	s->readPfFlags = d.ddpfPixelFormat.dwFlags; s->readBpp = (LONG)d.ddpfPixelFormat.dwRGBBitCount;
	s->readRMask = d.ddpfPixelFormat.dwRBitMask; s->readGMask = d.ddpfPixelFormat.dwGBitMask; s->readBMask = d.ddpfPixelFormat.dwBBitMask;
	if (!(caps & DDSCAPS_OFFSCREENPLAIN) || (caps & CAPS_REFUSED)) { st = RD_CAPS; goto done; }
	DWORD pf = d.ddpfPixelFormat.dwFlags, bpp = d.ddpfPixelFormat.dwRGBBitCount;
	DWORD rm = d.ddpfPixelFormat.dwRBitMask, gm = d.ddpfPixelFormat.dwGBitMask, bm = d.ddpfPixelFormat.dwBBitMask;
	if (!(pf & DDPF_RGB) || (pf & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXEDTO8 | DDPF_FOURCC))
		|| (bpp != 16 && bpp != 24 && bpp != 32) || !rm || !gm || !bm || (rm & gm) || (rm & bm) || (gm & bm)
		|| mask_bits(rm) > 8 || mask_bits(gm) > 8 || mask_bits(bm) > 8) { st = RD_FORMAT; goto done; }
	DWORD row = d.dwWidth * (bpp / 8);
	if (d.dwWidth < 1 || d.dwHeight < 1 || d.dwWidth > 4096 || d.dwHeight > 4096 || (unsigned long long)row * d.dwHeight > RD_PIXMAX) { st = RD_SIZE; goto done; }
	if (!gPix) {
		gPixMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, RD_PIXMAX, gPixName);
		gPix = gPixMap ? (BYTE*)MapViewOfFile(gPixMap, FILE_MAP_ALL_ACCESS, 0, 0, RD_PIXMAX) : NULL;
		if (!gPix) { vc_log("reader: no pixel block %s (err %lu)", gPixName, GetLastError()); st = RD_NOBLOCK; goto done; }
	}
	DDSURFACEDESC2 ld; memset(&ld, 0, sizeof(ld)); ld.dwSize = sizeof(ld);
	hr = IDirectDrawSurface7_Lock(s7, NULL, &ld, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
	if (hr != DD_OK) { s->readHr = hr; st = RD_LOCK; goto done; }
	if (ld.dwWidth != d.dwWidth || ld.dwHeight != d.dwHeight || !ld.lpSurface || ld.lPitch < (LONG)row
		|| ld.ddpfPixelFormat.dwRGBBitCount != bpp || ld.ddpfPixelFormat.dwRBitMask != rm || ld.ddpfPixelFormat.dwGBitMask != gm || ld.ddpfPixelFormat.dwBBitMask != bm) {
		IDirectDrawSurface7_Unlock(s7, NULL);
		st = RD_LOCKDESC; goto done;
	}
	for (DWORD y = 0; y < d.dwHeight; y++) memcpy(gPix + (SIZE_T)y * row, (const BYTE*)ld.lpSurface + (SIZE_T)y * (SIZE_T)ld.lPitch, row);
	s->readPitch = ld.lPitch;
	hr = IDirectDrawSurface7_Unlock(s7, NULL);
	if (hr != DD_OK) vc_log("reader: Unlock returned 0x%08lX", (unsigned long)hr);
	s->readBytes = (LONG)(row * d.dwHeight);
done:
	if (s7) IDirectDrawSurface7_Release(s7);
	{ DWORD c = 0; s->readCounter = vc_read_counter(&c) ? (LONG)c : -1; }
	s->readStatus = st;
	MemoryBarrier();
	InterlockedExchange(&s->readDone, req);
}
