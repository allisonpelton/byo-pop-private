// origrun's DirectSound stand-in: a dsound.dll placed beside a COPY of an original game (never the
// install) so the game's sound never reaches the audio device. Everything is accepted; nothing opens a
// device. The sound-relevant calls are logged with a timestamp and an identity for each sound (its byte
// size and a hash of the PCM the game wrote), so a port session can compare "which sound plays when"
// against its own build.
//
// It catches plain DirectSound and anything layered on it that loads it by name. Sound that goes another way (a BASS
// that makes its DirectSound through COM, which loads the system's dsound.dll from its registered path, winmm's waveOut
// and PlaySound, FMOD's own output) is silenced by
// the session mute below: as it loads, the stand-in mutes the process's own default audio session on every render
// device, so whatever the process plays there later is muted from its first sample. The host's Core Audio watch
// (origrun.c) checks every session of the game's and says whether any sound reached the speakers.
//
// Modes, from the ORIGRUN_SOUND environment variable the host sets:
//   nodriver  DirectSoundCreate* return DSERR_NODRIVER, as if the machine had no sound card. Used for the
//             very first run of a game, before a run has shown it stays silent.
//   stub      (default) hand back fake objects that accept every call and log, but never play anything.
// With ORIGRUN_CLOCK=virtual (origrun --clock virtual) the stand-in also carries origrun's virtual clock (vclock.c), and
// its log's times and its fake play cursor follow that clock instead of the wall clock. With ORIGRUN_CURSOR=scripted
// (origrun --cursor scripted) it points the copy's GetCursorPos at origrun's scripted mouse position (cursor.c). With
// ORIGRUN_MSGBOX=hook (every origrun run with the stand-in, unless --msgbox show) it answers the copy's MessageBox calls
// with no window and its MessageBeep calls with no sound (msgbox.c).
// Written from the public DirectSound API (dsound.h); no other project's wrapper was read.

#define COBJMACROS
#define INITGUID
#include <windows.h>
#include <dsound.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <stdio.h>
#include <stdint.h>
#include "coverage.h"

static FILE*       gLog;
static CRITICAL_SECTION gLogLock;
static LARGE_INTEGER gFreq, gStart;
static int         gNoDriver;   // 1 => nodriver mode
static int         gWav;        // 1 => also record buffers to WAV (ORIGRUN_SOUND=wav)
static char        gDir[MAX_PATH];
static LONG        gNextId = 1;
static LONG        gPlaySeq = 0;

// origrun's virtual clock, scripted cursor and message-box hook (vclock.c, cursor.c, msgbox.c): installed from DllMain
// when ORIGRUN_CLOCK=virtual, ORIGRUN_CURSOR=scripted or ORIGRUN_MSGBOX=hook
void vclock_install(HMODULE self);
void vclock_detach(void);
// the network block (net.c), always on, installed first and reported last
void net_install(HMODULE self);
void net_detach(void);
int  vclock_ms(double* out);

static void log_init(void)
{
	InitializeCriticalSection(&gLogLock);
	QueryPerformanceFrequency(&gFreq);
	QueryPerformanceCounter(&gStart);

	DWORD n = GetEnvironmentVariableA("ORIGRUN_DIR", gDir, sizeof(gDir));
	if (n == 0 || n >= sizeof(gDir))
		return;
	char* dir = gDir;

	char mode[32] = "stub";
	GetEnvironmentVariableA("ORIGRUN_SOUND", mode, sizeof(mode));
	gNoDriver = (lstrcmpiA(mode, "nodriver") == 0);
	gWav = (lstrcmpiA(mode, "wav") == 0);

	char path[MAX_PATH];
	wsprintfA(path, "%s\\logs", dir);
	CreateDirectoryA(path, NULL);
	if (gWav) { wsprintfA(path, "%s\\snd", dir); CreateDirectoryA(path, NULL); }
	wsprintfA(path, "%s\\logs\\sound.log", dir);
	gLog = fopen(path, "a");
	if (gLog)
	{
		SYSTEMTIME st; GetLocalTime(&st);
		fprintf(gLog, "# dsound stand-in attached pid=%lu mode=%s  %04d-%02d-%02d %02d:%02d:%02d\n",
			GetCurrentProcessId(), gNoDriver ? "nodriver" : "stub",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		fflush(gLog);
	}
}

// Milliseconds since the stand-in attached: on the virtual clock when it runs (it starts as the stand-in attaches).
static double wall_ms(void)
{
	LARGE_INTEGER c; QueryPerformanceCounter(&c);
	return (double)(c.QuadPart - gStart.QuadPart) * 1000.0 / (double)gFreq.QuadPart;
}
static double now_ms(void)
{
	double v; if (vclock_ms(&v)) return v;
	return wall_ms();
}

static void logf(const char* fmt, ...)
{
	if (!gLog) return;
	EnterCriticalSection(&gLogLock);
	va_list ap; va_start(ap, fmt);
	fprintf(gLog, "%10.1f  ", now_ms());
	vfprintf(gLog, fmt, ap);
	fputc('\n', gLog);
	fflush(gLog);
	va_end(ap);
	LeaveCriticalSection(&gLogLock);
}

// FNV-1a 64-bit, so a session can tell two sounds apart without storing their data.
static uint64_t fnv1a(const void* p, size_t n)
{
	const unsigned char* b = (const unsigned char*)p;
	uint64_t h = 1469598103934665603ULL;
	for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ULL; }
	return h;
}

// ---------------------------------------------------------------------------------------------------
// The process's audio sessions, muted. Windows puts every stream a process opens without a session of its own (waveOut,
// PlaySound, DirectSound, BASS and FMOD through either, the mixer API's wave volume) into the process's default audio
// session of the device it plays on, and that session's mute holds for every stream that joins it later. So the stand-in
// mutes the default session (GUID_NULL, not shared with other processes) on every active render device before the game
// can play anything: the sound the game plays past the stand-in (the BASS of Alchemy, Pixelus and Rocket Mania makes
// its DirectSound through COM, CLSID_DirectSound, which loads the system's dsound.dll) is muted from its first sample,
// and never reaches the speakers. COM may not be used in
// DllMain, so DllMain starts a thread that does it, and the stand-in's entry points wait for that thread before they
// answer: a game that asks the stand-in for DirectSound gets it only once its sessions are muted. A stream with a
// session GUID of its own, or on a device that became active later, is not covered; the host's watch mutes and reports
// those. Measured 2026-11-03: the mute holds after this thread has released its objects and ended, and waveOut, PlaySound
// and DirectSound streams opened later all join the muted session.
static HANDLE gMuteDone;   // set once the mute thread has finished (or could not start)
static LONG   gMuteLate;   // the wait for it ran out once: do not wait again
static const GUID kNullGuid = { 0 };
typedef HRESULT (WINAPI *CoInitEx_t)(LPVOID, DWORD);
typedef HRESULT (WINAPI *CoCreate_t)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
typedef void    (WINAPI *CoUninit_t)(void);

static DWORD WINAPI mute_thread(LPVOID unused)
{
	LARGE_INTEGER q0, q1; QueryPerformanceCounter(&q0);   // the time it takes, on the wall clock
	// ole32 is loaded here, outside the loader lock, rather than imported, so the stand-in's own load is as before
	HMODULE ole = LoadLibraryA("ole32.dll");
	CoInitEx_t coInit = ole ? (CoInitEx_t)GetProcAddress(ole, "CoInitializeEx") : NULL;
	CoCreate_t coCreate = ole ? (CoCreate_t)GetProcAddress(ole, "CoCreateInstance") : NULL;
	CoUninit_t coUninit = ole ? (CoUninit_t)GetProcAddress(ole, "CoUninitialize") : NULL;
	int devs = 0, muted = 0, playing = 0;
	HRESULT why = E_FAIL;
	if (coInit && coCreate && coUninit) {
		HRESULT hi = coInit(NULL, COINIT_MULTITHREADED);
		IMMDeviceEnumerator* en = NULL;
		why = coCreate(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&en);
		if (SUCCEEDED(why) && en) {
			IMMDeviceCollection* col = NULL;
			if (SUCCEEDED(why = IMMDeviceEnumerator_EnumAudioEndpoints(en, eRender, DEVICE_STATE_ACTIVE, &col)) && col) {
				UINT n = 0; IMMDeviceCollection_GetCount(col, &n);
				for (UINT i = 0; i < n; i++) {
					IMMDevice* dev = NULL;
					if (FAILED(IMMDeviceCollection_Item(col, i, &dev)) || !dev) continue;
					devs++;
					IAudioSessionManager* m = NULL;
					if (SUCCEEDED(why = IMMDevice_Activate(dev, &IID_IAudioSessionManager, CLSCTX_ALL, NULL, (void**)&m)) && m) {
						ISimpleAudioVolume* sv = NULL;   // makes the session if the process has none yet
						if (SUCCEEDED(why = IAudioSessionManager_GetSimpleAudioVolume(m, &kNullGuid, FALSE, &sv)) && sv) {
							if (SUCCEEDED(why = ISimpleAudioVolume_SetMute(sv, TRUE, NULL))) muted++;
							ISimpleAudioVolume_Release(sv);
						}
						// a stream already playing in it has played unmuted until now
						IAudioSessionControl* ctl = NULL; AudioSessionState st = AudioSessionStateInactive;
						if (SUCCEEDED(IAudioSessionManager_GetAudioSessionControl(m, &kNullGuid, 0, &ctl)) && ctl) {
							IAudioSessionControl_GetState(ctl, &st); IAudioSessionControl_Release(ctl);
						}
						if (st == AudioSessionStateActive) playing++;
						IAudioSessionManager_Release(m);
					}
					IMMDevice_Release(dev);
				}
				IMMDeviceCollection_Release(col);
			}
			IMMDeviceEnumerator_Release(en);
		}
		if (SUCCEEDED(hi)) coUninit();
	}
	QueryPerformanceCounter(&q1);
	if (devs && muted == devs)
		logf("audio: the process's default audio session muted on %d render device%s (it took %.1f ms of wall time)", devs, devs == 1 ? "" : "s",
			(double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)gFreq.QuadPart);
	else
		logf("audio: WARNING: the process's default audio session muted on %d of %d render devices (0x%08lx); the host's watch mutes the rest",
			muted, devs, (unsigned long)why);
	if (playing)
		logf("audio: WARNING: a stream was already playing in it on %d device%s, unmuted until now (the game played before it loaded the stand-in)",
			playing, playing == 1 ? "" : "s");
	SetEvent(gMuteDone);
	return 0;
}
static void mute_start(void)
{
	gMuteDone = CreateEventA(NULL, TRUE, FALSE, NULL);
	HANDLE t = gMuteDone ? CreateThread(NULL, 0, mute_thread, NULL, 0, NULL) : NULL;
	if (t) { CloseHandle(t); return; }
	logf("audio: WARNING: no thread to mute the process's audio sessions (err %lu); the host's watch mutes them", GetLastError());
	if (gMuteDone) SetEvent(gMuteDone);
}
// Called by every entry point a game reaches before it can play through the stand-in, outside the loader lock.
static void mute_wait(void)
{
	if (!gMuteDone || gMuteLate) return;
	if (WaitForSingleObject(gMuteDone, 5000) == WAIT_TIMEOUT && !InterlockedExchange(&gMuteLate, 1))
		logf("audio: WARNING: the process's audio sessions were not muted after 5 s; going on without waiting");
}

// ---------------------------------------------------------------------------------------------------
// WAV recording (ORIGRUN_SOUND=wav). We write what the game puts into its DirectSound buffers as WAV at
// each buffer's declared format: static buffers one file per Play (in play order), and a looping/streaming
// buffer (BASS/FMOD mixed music) as one growing file. Sizes in the header are patched on every append, so a
// file stays valid even if the game is killed. The ~90s cap keeps a looping stream from growing without end.
#define WAV_CAP_BYTES (48000u*2u*2u*90u)

static void le16(unsigned char* p, unsigned v){ p[0]=(unsigned char)v; p[1]=(unsigned char)(v>>8); }
static void le32(unsigned char* p, unsigned v){ p[0]=(unsigned char)v; p[1]=(unsigned char)(v>>8); p[2]=(unsigned char)(v>>16); p[3]=(unsigned char)(v>>24); }

static FILE* wav_open(const char* path, const WAVEFORMATEX* f)
{
	if (!f || f->nChannels==0 || f->nSamplesPerSec==0 || f->wBitsPerSample==0) return NULL;
	FILE* w = fopen(path, "wb"); if (!w) return NULL;
	unsigned block = f->nBlockAlign ? f->nBlockAlign : (f->nChannels*f->wBitsPerSample/8);
	unsigned rate  = f->nAvgBytesPerSec ? f->nAvgBytesPerSec : f->nSamplesPerSec*block;
	unsigned char h[44];
	memcpy(h,"RIFF",4); le32(h+4,36); memcpy(h+8,"WAVE",4);
	memcpy(h+12,"fmt ",4); le32(h+16,16); le16(h+20,1); le16(h+22,f->nChannels);
	le32(h+24,f->nSamplesPerSec); le32(h+28,rate); le16(h+32,(unsigned short)block); le16(h+34,f->wBitsPerSample);
	memcpy(h+36,"data",4); le32(h+40,0);
	fwrite(h,1,44,w); fflush(w);
	return w;
}
static void wav_append(FILE* w, const void* data, DWORD n, DWORD* total)
{
	if (!w || !n) return;
	fwrite(data,1,n,w); *total += n;
	unsigned char s[4];
	fseek(w,4,SEEK_SET);  le32(s,36+*total); fwrite(s,1,4,w);
	fseek(w,40,SEEK_SET); le32(s,*total);    fwrite(s,1,4,w);
	fseek(w,0,SEEK_END);  fflush(w);
}
static void wav_oneshot(const char* path, const WAVEFORMATEX* f, const void* data, DWORD n)
{
	FILE* w = wav_open(path,f); if (!w) return;
	DWORD t=0; wav_append(w,data,n,&t); fclose(w);
}

// ---------------------------------------------------------------------------------------------------
// IDirectSoundBuffer8 stand-in. One object serves the IDirectSoundBuffer and IDirectSoundBuffer8 faces.

typedef struct MyBuf {
	IDirectSoundBuffer8 iface;
	LONG   ref;
	int    id;
	DWORD  bytes;
	WAVEFORMATEX wfx;
	LONG   vol, pan, freq;
	void*  lockmem;      // memory we hand out on Lock so the game can write its PCM
	DWORD  locklen;
	DWORD  lockoff;
	// WAV mode and the fake advancing play cursor
	int    playing;      // Play called, Stop not yet
	int    looping;      // last Play was DSBPLAY_LOOPING
	int    streaming;    // looping buffer -> record as one growing WAV (music)
	DWORD  byterate;     // bytes/sec from the format, for the cursor
	int    wall;         // a buffer of a COM-made device (BASS's): its cursor runs on the wall clock (see CF_CreateInstance)
	double playStartMs;  // buf_now() when pos0 was taken: the wall clock, or the virtual clock when it runs
	DWORD  pos0;         // the play cursor at playStartMs (Play, SetCurrentPosition, SetFrequency), or where Stop left it
	void*  pcm;          // last full-buffer contents (for a static one-shot dump)
	DWORD  pcmBytes;
	FILE*  wav;          // streaming WAV, lazily opened
	DWORD  wavBytes;
	unsigned long long hash; // hash of the last write that filled the whole buffer from 0 (0 if none)
} MyBuf;

static const IDirectSoundBuffer8Vtbl gBufVtbl;

// A buffer from its description; logged as created unless it is a duplicate, which logs its own line
static MyBuf* MakeBuf(const DSBUFFERDESC* d, int logCreate)
{
	MyBuf* b = (MyBuf*)calloc(1, sizeof(MyBuf));
	b->iface.lpVtbl = (IDirectSoundBuffer8Vtbl*)&gBufVtbl;
	b->ref = 1;
	b->id = InterlockedIncrement(&gNextId);
	b->bytes = d ? d->dwBufferBytes : 0;
	b->vol = 0; b->pan = 0;
	if (d && d->lpwfxFormat) {
		b->wfx = *d->lpwfxFormat;
		b->freq = d->lpwfxFormat->nSamplesPerSec;
	}
	if (logCreate)
		logf("buffer %d create  bytes=%lu  fmt=%uHz/%ubit/%uch  flags=0x%lx",
			b->id, b->bytes, b->wfx.nSamplesPerSec, b->wfx.wBitsPerSample, b->wfx.nChannels,
			d ? d->dwFlags : 0);
	return b;
}
static MyBuf* NewBuf(const DSBUFFERDESC* d) { return MakeBuf(d, 1); }

static HRESULT STDMETHODCALLTYPE Buf_QueryInterface(IDirectSoundBuffer8* This, REFIID riid, void** ppv)
{
	// Hand back the same object for every buffer face; a no-op notify for IDirectSoundNotify.
	*ppv = This;
	((MyBuf*)This)->iface.lpVtbl->AddRef(This);
	return S_OK;
}
static ULONG STDMETHODCALLTYPE Buf_AddRef(IDirectSoundBuffer8* This)  { return InterlockedIncrement(&((MyBuf*)This)->ref); }
static ULONG STDMETHODCALLTYPE Buf_Release(IDirectSoundBuffer8* This)
{
	MyBuf* b = (MyBuf*)This;
	LONG r = InterlockedDecrement(&b->ref);
	if (r == 0) { if (b->wav) fclose(b->wav); free(b->pcm); free(b->lockmem); free(b); }
	return r;
}
static HRESULT STDMETHODCALLTYPE Buf_GetCaps(IDirectSoundBuffer8* This, LPDSBCAPS c)
{ if (c) { c->dwBufferBytes = ((MyBuf*)This)->bytes; } return DS_OK; }
// The play cursor, moved on by origrun's clock (the wall clock, or the virtual clock when it runs) at the rate of the
// buffer's format and frequency, from where Play, SetCurrentPosition or SetFrequency last left it. A buffer played
// once stops at its end with its cursor back at 0, as DirectSound's does; a looping one plays until it is stopped.
// A streaming source (BASS, FMOD) keeps writing more audio as this cursor moves, and a game that asks GetStatus
// whether a sound still plays hears the sound's length.
static double buf_rate(const MyBuf* b)   // bytes per millisecond
{
	double r = b->byterate;
	if (b->freq && b->wfx.nSamplesPerSec && b->freq != b->wfx.nSamplesPerSec) r = r * b->freq / b->wfx.nSamplesPerSec;
	return r / 1000.0;
}
// A buffer's clock: the virtual clock when it runs, except for a COM-made device's buffers (see CF_CreateInstance)
static double buf_now(MyBuf* b) { return b->wall ? wall_ms() : now_ms(); }
static DWORD buf_cursor(MyBuf* b)
{
	if (!b->playing || !b->byterate || !b->bytes) return b->bytes ? b->pos0 % b->bytes : 0;
	double ms = buf_now(b) - b->playStartMs;
	unsigned long long adv = b->pos0 + (unsigned long long)(buf_rate(b) * (ms > 0 ? ms : 0));
	if (b->looping) return (DWORD)(adv % b->bytes);
	if (adv >= b->bytes) { b->playing = 0; b->pos0 = 0; return 0; }
	return (DWORD)adv;
}
static void buf_rebase(MyBuf* b, DWORD pos) { b->pos0 = pos; b->playStartMs = buf_now(b); }

// Who asked GetStatus, logged as the process detaches: a game whose sounds' ends now take their length on origrun's
// clock (before 2026-11-03 every buffer answered "stopped").
static struct { HMODULE mod; LONG calls, playing; } gStatusBy[8];
static LONG gStatusCalls;
static void note_status(void* ra, int playing)
{
	HMODULE m = NULL;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)ra, &m);
	InterlockedIncrement(&gStatusCalls);
	if (!m) return;
	for (int i = 0; i < 8; i++) {
		HMODULE cur = gStatusBy[i].mod;
		if (!cur) { cur = (HMODULE)InterlockedCompareExchangePointer((PVOID volatile*)&gStatusBy[i].mod, m, NULL); if (!cur) cur = m; }
		if (cur != m) continue;
		InterlockedIncrement(&gStatusBy[i].calls);
		if (playing) InterlockedIncrement(&gStatusBy[i].playing);
		return;
	}
}
static void log_status_summary(void)
{
	if (!gStatusCalls) { logf("GetStatus: never asked"); return; }
	for (int i = 0; i < 8 && gStatusBy[i].mod; i++) {
		char name[MAX_PATH] = "?";
		GetModuleFileNameA(gStatusBy[i].mod, name, sizeof(name));
		const char* base = strrchr(name, '\\'); base = base ? base + 1 : name;
		logf("GetStatus: %ld calls from %s, %ld of them answered playing", gStatusBy[i].calls, base, gStatusBy[i].playing);
	}
}

static HRESULT STDMETHODCALLTYPE Buf_GetCurrentPosition(IDirectSoundBuffer8* This, LPDWORD p, LPDWORD w)
{
	MyBuf* b = (MyBuf*)This;
	DWORD pos = buf_cursor(b);
	if (p) *p = pos;
	if (w) *w = b->bytes ? (pos + (b->byterate/50)) % b->bytes : 0;   // write cursor a little ahead
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE Buf_GetFormat(IDirectSoundBuffer8* This, LPWAVEFORMATEX f, DWORD cb, LPDWORD got)
{ MyBuf* b=(MyBuf*)This; if (f && cb>=sizeof(WAVEFORMATEX)) *f=b->wfx; if (got) *got=sizeof(WAVEFORMATEX); return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_GetVolume(IDirectSoundBuffer8* This, LPLONG v) { if (v) *v=((MyBuf*)This)->vol; return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_GetPan(IDirectSoundBuffer8* This, LPLONG p)    { if (p) *p=((MyBuf*)This)->pan; return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_GetFrequency(IDirectSoundBuffer8* This, LPDWORD f){ if (f) *f=((MyBuf*)This)->freq; return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_GetStatus(IDirectSoundBuffer8* This, LPDWORD s)
{
	// Playing from Play until Stop, or until a buffer played once has played its length (buf_cursor); looping too for
	// one played with DSBPLAY_LOOPING.
	MyBuf* b = (MyBuf*)This;
	buf_cursor(b);
	DWORD st = b->playing ? (DSBSTATUS_PLAYING | (b->looping ? DSBSTATUS_LOOPING : 0)) : 0;
	note_status(__builtin_return_address(0), st != 0);
	if (s) *s = st;
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE Buf_Initialize(IDirectSoundBuffer8* This, LPDIRECTSOUND d, LPCDSBUFFERDESC x) { return DS_OK; }

static HRESULT STDMETHODCALLTYPE Buf_Lock(IDirectSoundBuffer8* This, DWORD off, DWORD bytes,
	LPVOID* p1, LPDWORD n1, LPVOID* p2, LPDWORD n2, DWORD flags)
{
	MyBuf* b = (MyBuf*)This;
	DWORD want = bytes ? bytes : b->bytes;
	if (flags & DSBLOCK_ENTIREBUFFER) want = b->bytes;
	if (want == 0) want = 1;
	free(b->lockmem);
	b->lockmem = malloc(want);
	b->locklen = want;
	b->lockoff = off;
	if (b->lockmem) ZeroMemory(b->lockmem, want);
	if (p1) *p1 = b->lockmem;
	if (n1) *n1 = want;
	if (p2) *p2 = NULL;
	if (n2) *n2 = 0;
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE Buf_Unlock(IDirectSoundBuffer8* This, LPVOID p1, DWORD n1, LPVOID p2, DWORD n2)
{
	MyBuf* b = (MyBuf*)This;
	if (b->lockmem && n1) {
		DWORD n = n1 < b->locklen ? n1 : b->locklen;
		unsigned long long h = fnv1a(b->lockmem, n);
		logf("buffer %d data   off=%lu bytes=%lu  hash=%016llx", b->id, b->lockoff, n, h);
		if (b->lockoff == 0 && n >= b->bytes && b->bytes) b->hash = h;
		if (gWav) {
			// Keep a copy of the full buffer (static one-shots are written whole on Play).
			if (b->lockoff == 0 && n >= b->bytes && b->bytes) {
				free(b->pcm); b->pcm = malloc(b->bytes); if (b->pcm){ memcpy(b->pcm, b->lockmem, b->bytes); b->pcmBytes = b->bytes; }
			}
			// For a streaming (looping) buffer, append each write to its growing WAV.
			if (b->streaming) {
				if (!b->wav) { char p[MAX_PATH]; wsprintfA(p,"%s\\snd\\stream_buf%03d.wav",gDir,b->id); b->wav = wav_open(p,&b->wfx); }
				if (b->wav && b->wavBytes < WAV_CAP_BYTES) wav_append(b->wav, b->lockmem, n, &b->wavBytes);
			}
		}
	}
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE Buf_Play(IDirectSoundBuffer8* This, DWORD r1, DWORD pri, DWORD flags)
{
	MyBuf* b = (MyBuf*)This;
	DWORD from = buf_cursor(b);   // a stopped buffer plays on from where it stopped; a playing one goes on
	b->looping = (flags & DSBPLAY_LOOPING) != 0;
	b->playing = 1;
	b->byterate = b->wfx.nAvgBytesPerSec ? b->wfx.nAvgBytesPerSec :
		(b->wfx.nSamplesPerSec * (b->wfx.nBlockAlign ? b->wfx.nBlockAlign : (b->wfx.nChannels*b->wfx.wBitsPerSample/8)));
	buf_rebase(b, from);
	logf("buffer %d play    %s  vol=%ld pan=%ld freq=%luHz", b->id,
		b->looping ? "looping" : "once", b->vol, b->pan, b->freq);
	if (gWav) {
		if (b->looping) {
			b->streaming = 1;   // music / looping source: recorded as it is written (see Unlock)
		} else if (b->pcm && b->pcmBytes) {
			LONG seq = InterlockedIncrement(&gPlaySeq);
			char p[MAX_PATH]; wsprintfA(p,"%s\\snd\\play_%04ld_buf%03d.wav",gDir,seq,b->id);
			wav_oneshot(p,&b->wfx,b->pcm,b->pcmBytes);
			logf("buffer %d wav     play_%04ld_buf%03d.wav (%lu bytes)", b->id, seq, b->id, b->pcmBytes);
		}
	}
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE Buf_SetCurrentPosition(IDirectSoundBuffer8* This, DWORD p)
{ MyBuf* b=(MyBuf*)This; buf_cursor(b); buf_rebase(b, b->bytes ? p % b->bytes : 0); return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_SetFormat(IDirectSoundBuffer8* This, LPCWAVEFORMATEX f)
{ if (f) { ((MyBuf*)This)->wfx=*f; ((MyBuf*)This)->freq=f->nSamplesPerSec; } return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_SetVolume(IDirectSoundBuffer8* This, LONG v)
{ MyBuf* b=(MyBuf*)This; b->vol=v; logf("buffer %d volume  %ld", b->id, v); return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_SetPan(IDirectSoundBuffer8* This, LONG p)
{ MyBuf* b=(MyBuf*)This; b->pan=p; logf("buffer %d pan     %ld", b->id, p); return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_SetFrequency(IDirectSoundBuffer8* This, DWORD f)
{ MyBuf* b=(MyBuf*)This; buf_rebase(b, buf_cursor(b)); b->freq=f; logf("buffer %d freq    %luHz", b->id, f); return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_Stop(IDirectSoundBuffer8* This)
{ MyBuf* b=(MyBuf*)This; b->pos0=buf_cursor(b); b->playing=0; if(b->wav) fflush(b->wav); logf("buffer %d stop", b->id); return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_Restore(IDirectSoundBuffer8* This) { return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_SetFX(IDirectSoundBuffer8* This, DWORD n, LPDSEFFECTDESC d, LPDWORD r) { if (r && d) for (DWORD i=0;i<n;i++) r[i]=DSFXR_LOCSOFTWARE; return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_AcquireResources(IDirectSoundBuffer8* This, DWORD f, DWORD n, LPDWORD r) { return DS_OK; }
static HRESULT STDMETHODCALLTYPE Buf_GetObjectInPath(IDirectSoundBuffer8* This, REFGUID o, DWORD i, REFGUID in, void** pp) { if (pp) *pp=NULL; return E_NOTIMPL; }

static const IDirectSoundBuffer8Vtbl gBufVtbl = {
	Buf_QueryInterface, Buf_AddRef, Buf_Release, Buf_GetCaps, Buf_GetCurrentPosition, Buf_GetFormat,
	Buf_GetVolume, Buf_GetPan, Buf_GetFrequency, Buf_GetStatus, Buf_Initialize, Buf_Lock, Buf_Play,
	Buf_SetCurrentPosition, Buf_SetFormat, Buf_SetVolume, Buf_SetPan, Buf_SetFrequency, Buf_Stop,
	Buf_Unlock, Buf_Restore, Buf_SetFX, Buf_AcquireResources, Buf_GetObjectInPath,
};

// ---------------------------------------------------------------------------------------------------
// IDirectSound8 stand-in.

typedef struct MyDS { IDirectSound8 iface; LONG ref; int wall; } MyDS;   // wall: made through COM (CF_CreateInstance)
static const IDirectSound8Vtbl gDsVtbl;

static HRESULT STDMETHODCALLTYPE DS_QueryInterface(IDirectSound8* This, REFIID riid, void** ppv)
{ *ppv = This; InterlockedIncrement(&((MyDS*)This)->ref); return S_OK; }
static ULONG STDMETHODCALLTYPE DS_AddRef(IDirectSound8* This)  { return InterlockedIncrement(&((MyDS*)This)->ref); }
static ULONG STDMETHODCALLTYPE DS_Release(IDirectSound8* This)
{ LONG r = InterlockedDecrement(&((MyDS*)This)->ref); if (r==0) free(This); return r; }
static HRESULT STDMETHODCALLTYPE DS_CreateSoundBuffer(IDirectSound8* This, LPCDSBUFFERDESC d, LPDIRECTSOUNDBUFFER* pp, LPUNKNOWN u)
{ if (!pp) return E_POINTER; MyBuf* b = NewBuf(d); b->wall = ((MyDS*)This)->wall; *pp = (LPDIRECTSOUNDBUFFER)b; return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_GetCaps(IDirectSound8* This, LPDSCAPS c)
{ if (c) { DWORD sz=c->dwSize; ZeroMemory(c,sz); c->dwSize=sz; c->dwFlags=DSCAPS_PRIMARYSTEREO|DSCAPS_PRIMARY16BIT|DSCAPS_CONTINUOUSRATE; c->dwMinSecondarySampleRate=100; c->dwMaxSecondarySampleRate=200000; c->dwFreeHwMixingAllBuffers=32; } return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_DuplicateSoundBuffer(IDirectSound8* This, LPDIRECTSOUNDBUFFER o, LPDIRECTSOUNDBUFFER* pp)
{
	// A duplicate shares its source's data, so the log names the source and the hash of its data: that is how a
	// played duplicate is matched to the sound it holds.
	if (!pp) return E_POINTER;
	MyBuf* s = (MyBuf*)o;
	if (!s) return E_INVALIDARG;
	// made as its source was (size and format), and with its source's volume, pan and frequency, as DirectSound's
	// duplicate starts; never a stream of its own, which would record its own WAV
	DSBUFFERDESC d; ZeroMemory(&d, sizeof(d));
	d.dwSize = sizeof(d); d.dwBufferBytes = s->bytes; d.lpwfxFormat = &s->wfx;
	MyBuf* b = MakeBuf(&d, 0);
	{
		b->vol = s->vol; b->pan = s->pan; b->freq = s->freq; b->hash = s->hash;
		b->byterate = s->byterate; b->wall = s->wall;
		if (s->pcm && s->pcmBytes) { b->pcm = malloc(s->pcmBytes); if (b->pcm) { memcpy(b->pcm, s->pcm, s->pcmBytes); b->pcmBytes = s->pcmBytes; } }
		logf("buffer %d duplicate of %d  hash=%016llx", b->id, s->id, s->hash);
	}
	*pp = (LPDIRECTSOUNDBUFFER)b;
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DS_SetCooperativeLevel(IDirectSound8* This, HWND h, DWORD lvl) { return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_Compact(IDirectSound8* This) { return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_GetSpeakerConfig(IDirectSound8* This, LPDWORD c) { if (c) *c=DSSPEAKER_STEREO; return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_SetSpeakerConfig(IDirectSound8* This, DWORD c) { return DS_OK; }
// Called only on an object made through COM (DirectSoundCreate* initialise their own): no device in nodriver mode, as
// DirectSound's own Initialize answers on a machine without one
static HRESULT STDMETHODCALLTYPE DS_Initialize(IDirectSound8* This, LPCGUID g)
{ if (gNoDriver) { logf("IDirectSound::Initialize -> DSERR_NODRIVER"); return DSERR_NODRIVER; } return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_VerifyCertification(IDirectSound8* This, LPDWORD c) { if (c) *c=DS_CERTIFIED; return DS_OK; }

static const IDirectSound8Vtbl gDsVtbl = {
	DS_QueryInterface, DS_AddRef, DS_Release, DS_CreateSoundBuffer, DS_GetCaps, DS_DuplicateSoundBuffer,
	DS_SetCooperativeLevel, DS_Compact, DS_GetSpeakerConfig, DS_SetSpeakerConfig, DS_Initialize,
	DS_VerifyCertification,
};

static IDirectSound8* NewDS(void)
{
	MyDS* d = (MyDS*)calloc(1, sizeof(MyDS));
	d->iface.lpVtbl = (IDirectSound8Vtbl*)&gDsVtbl;
	d->ref = 1;
	logf("device create");
	return &d->iface;
}

// ---------------------------------------------------------------------------------------------------
// Exported entry points (dsound.dll's public surface).

HRESULT WINAPI DirectSoundCreate(LPCGUID dev, LPDIRECTSOUND* pp, LPUNKNOWN u)
{
	mute_wait();
	if (gNoDriver) { logf("DirectSoundCreate -> DSERR_NODRIVER"); if (pp) *pp=NULL; return DSERR_NODRIVER; }
	if (!pp) return E_POINTER;
	*pp = (LPDIRECTSOUND)NewDS();
	return DS_OK;
}
HRESULT WINAPI DirectSoundCreate8(LPCGUID dev, LPDIRECTSOUND8* pp, LPUNKNOWN u)
{
	mute_wait();
	if (gNoDriver) { logf("DirectSoundCreate8 -> DSERR_NODRIVER"); if (pp) *pp=NULL; return DSERR_NODRIVER; }
	if (!pp) return E_POINTER;
	*pp = NewDS();
	return DS_OK;
}
// One device, the default one (a null GUID), unless the machine is meant to have none: FMOD 3 (Atomica) lists the
// drivers first and fails its init on an empty list.
HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA cb, LPVOID ctx)
{ mute_wait(); if (cb && !gNoDriver) cb(NULL, "Primary Sound Driver", "", ctx); return DS_OK; }
HRESULT WINAPI DirectSoundEnumerateW(LPDSENUMCALLBACKW cb, LPVOID ctx)
{ mute_wait(); if (cb && !gNoDriver) cb(NULL, L"Primary Sound Driver", L"", ctx); return DS_OK; }
HRESULT WINAPI DirectSoundCaptureCreate(LPCGUID g, LPDIRECTSOUNDCAPTURE* pp, LPUNKNOWN u)
{ if (pp) *pp=NULL; return DSERR_NODRIVER; }
HRESULT WINAPI DirectSoundCaptureCreate8(LPCGUID g, LPDIRECTSOUNDCAPTURE8* pp, LPUNKNOWN u)
{ if (pp) *pp=NULL; return DSERR_NODRIVER; }
HRESULT WINAPI DirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA cb, LPVOID ctx) { return DS_OK; }
HRESULT WINAPI DirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW cb, LPVOID ctx) { return DS_OK; }
HRESULT WINAPI GetDeviceID(LPCGUID src, LPGUID dst) { if (dst && src) *dst=*src; return DS_OK; }
HRESULT WINAPI DirectSoundFullDuplexCreate(LPCGUID a, LPCGUID b, LPCDSCBUFFERDESC c, LPCDSBUFFERDESC d,
	HWND h, DWORD lvl, LPDIRECTSOUNDFULLDUPLEX* fd, LPDIRECTSOUNDCAPTUREBUFFER8* cb,
	LPDIRECTSOUNDBUFFER8* rb, LPUNKNOWN u) { return DSERR_NODRIVER; }

// COM activation: CoCreateInstance of CLSID_DirectSound or CLSID_DirectSound8. COM finds the stand-in for them through
// the manifest origrun writes beside the copy's exe (<exe>.manifest, registration-free COM: the copy's own classes, read
// by Windows as the process starts; nothing is registered and nothing is hooked). Without it COM loads the system's
// dsound.dll from the registry, past the stand-in: the BASS of Alchemy, Pixelus and Rocket Mania makes its DirectSound
// that way, and played its music on the real device (2026-11-03). The object is the stand-in's, as DirectSoundCreate
// makes it; the caller then calls its Initialize. Its buffers' cursors run on the wall clock, as the real device's did:
// on the virtual clock BASS has to mix its music as fast as that clock runs, and the game slowed with it (Alchemy's case
// ran at 18x instead of 960x).
static HRESULT STDMETHODCALLTYPE CF_QueryInterface(IClassFactory* This, REFIID riid, void** ppv)
{
	if (!ppv) return E_POINTER;
	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory)) { *ppv = This; return S_OK; }
	*ppv = NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE CF_AddRef(IClassFactory* This) { return 2; }   // a static object
static ULONG STDMETHODCALLTYPE CF_Release(IClassFactory* This) { return 1; }
static HRESULT STDMETHODCALLTYPE CF_CreateInstance(IClassFactory* This, IUnknown* outer, REFIID riid, void** ppv)
{
	if (!ppv) return E_POINTER;
	*ppv = NULL;
	if (outer) return CLASS_E_NOAGGREGATION;
	logf("COM: a DirectSound object made through CoCreateInstance, by the stand-in");
	IDirectSound8* ds = NewDS();
	((MyDS*)ds)->wall = 1;
	*ppv = ds;
	return S_OK;
}
static HRESULT STDMETHODCALLTYPE CF_LockServer(IClassFactory* This, BOOL lock) { return S_OK; }
static const IClassFactoryVtbl gFactoryVtbl = { CF_QueryInterface, CF_AddRef, CF_Release, CF_CreateInstance, CF_LockServer };
static IClassFactory gFactory = { (IClassFactoryVtbl*)&gFactoryVtbl };
HRESULT WINAPI DllCanUnloadNow(void) { return S_FALSE; }
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
	mute_wait();
	if (!ppv) return E_POINTER;
	*ppv = NULL;
	if (!IsEqualCLSID(rclsid, &CLSID_DirectSound) && !IsEqualCLSID(rclsid, &CLSID_DirectSound8)) return CLASS_E_CLASSNOTAVAILABLE;
	return CF_QueryInterface(&gFactory, riid, ppv);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r)
{
	if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(h); coverage_install(); log_init(); net_install(h); vclock_install(h); mute_start(); }
	else if (reason == DLL_PROCESS_DETACH) { log_status_summary(); net_detach(); vclock_detach(); coverage_detach(); }
	return TRUE;
}
