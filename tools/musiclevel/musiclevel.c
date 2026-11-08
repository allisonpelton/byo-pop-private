// Measures an original's music engine (docs/WEB_PLAYER.md, "Music levels"): decodes a module with a game's own bass.dll or
// fmod.dll, a copy under build/, silently (BASS's "no sound" device and a decoding channel; FMOD 3's no-sound output with a
// DSP unit reading the mix), and prints the RMS and peak of its first seconds in dBFS. It runs no game and plays nothing.
// 32-bit, as the games' DLLs are; build it with tools/musiclevel/build.sh. The port's side is portlevel (CMakeLists.txt).
//   musiclevel bass <dll> <module> <seconds> [amplify 0-100]     BASS 1.7 to 2.3
//   musiclevel fmod <dll> <module> <seconds> [mastervolume 0-256] FMOD 3.5 to 3.7
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// a stdcall function called with n DWORDs, esp restored afterwards whatever the callee popped
static DWORD __attribute__((noinline)) callsafe(void* f, int n, const DWORD* a)
{
	DWORD r;
	__asm__ volatile(
		"mov %%esp, %%edi\n\t"
		"mov %2, %%ecx\n\t"
		"1: test %%ecx, %%ecx\n\t"
		"jz 2f\n\t"
		"dec %%ecx\n\t"
		"push (%3,%%ecx,4)\n\t"
		"jmp 1b\n\t"
		"2: call *%1\n\t"
		"mov %%edi, %%esp\n\t"
		: "=a"(r) : "r"(f), "r"(n), "r"(a) : "ecx", "edx", "edi", "memory");
	return r;
}
#define CALL(f, ...) ({ DWORD _a[] = {__VA_ARGS__}; callsafe((void*)(f), sizeof(_a) / 4, _a); })

static double sum2 = 0; static double peak = 0; static long long count = 0; static long long want = 0;
static void take16(const short* s, int n)
{
	for (int i = 0; i < n && count < want; i++, count++) { double v = s[i] / 32768.0; sum2 += v * v; if (fabs(v) > peak) peak = fabs(v); }
}
static void report(void)
{
	printf("RMS %.2f dBFS peak %.2f dBFS over %lld samples\n", 10 * log10(sum2 / (count ? count : 1)), 20 * log10(peak > 0 ? peak : 1e-9), count);
}

static FARPROC need(HMODULE h, const char* n)
{
	FARPROC p = GetProcAddress(h, n);
	if (!p) { fprintf(stderr, "no %s\n", n); exit(3); }
	return p;
}

static int bass(HMODULE h, const char* file, double secs, int amp)
{
	DWORD ver = ((DWORD(WINAPI*)(void))need(h, "BASS_GetVersion"))();
	int v2 = (ver >> 24) == 2;          // 2.2/2.3: 0x0203xxxx; 1.x and 2.0: MAKELONG(major, minor)
	int major = v2 ? 2 : (int)(ver & 0xffff), minor = v2 ? (int)((ver >> 16) & 0xff) : (int)(ver >> 16);
	FARPROC init = need(h, "BASS_Init"), load = need(h, "BASS_MusicLoad"), get = need(h, "BASS_ChannelGetData");
	FARPROC err = GetProcAddress(h, "BASS_ErrorGetCode");
	// device: 0 is "no sound" from 2.0 on, -2 before
	DWORD ok = (major >= 2) ? CALL(init, 0, 44100, 0, 0, 0) : CALL(init, (DWORD)-2, 44100, 0, 0);
	printf("BASS %d.%d init %lu err %ld\n", major, minor, ok, err ? (long)CALL(err) : 0);
	DWORD flags = 0x200000; // BASS_MUSIC_DECODE
	DWORD m = 0;
	if (!m) m = CALL(load, 0, (DWORD)file, 0, 0, flags, 44100);                          // DWORD offset, freq
	if (!m) m = CALL(load, 0, (DWORD)file, 0, 0, flags);                                 // 1.x
	printf("load %lu err %ld\n", m, err ? (long)CALL(err) : 0);
	if (!m) return 4;
	if (amp >= 0)
	{
		FARPROC setamp = GetProcAddress(h, "BASS_MusicSetAmplify"), setattr = GetProcAddress(h, "BASS_MusicSetAttribute");
		if (setamp) printf("amplify %d: %lu\n", amp, CALL(setamp, m, amp));
		else if (setattr) printf("amplify %d: %lu\n", amp, CALL(setattr, m, 0, amp));   // BASS_MUSIC_ATTRIB_AMPLIFY
	}
	want = (long long)(secs * 44100) * 2;
	static short buf[8192];
	while (count < want)
	{
		DWORD got = CALL(get, m, (DWORD)buf, sizeof(buf));
		if (got == (DWORD)-1 || got == 0) { printf("data end %ld err %ld\n", (long)got, err ? (long)CALL(err) : 0); break; }
		take16(buf, got / 2);
	}
	report();
	return 0;
}

// FMOD 3's DSP callback (cdecl in 3.5): after the clip-and-copy unit (priority 1000) the buffer is the 16-bit stereo output,
// len frames of it
static void* __cdecl dsp(void* orig, void* buf, int len, int param)
{
	(void)orig; (void)param;
	take16((const short*)buf, len * 2);
	return buf;
}

static int fmod3(HMODULE h, const char* file, double secs, int vol)
{
	float ver = ((float(WINAPI*)(void))need(h, "_FSOUND_GetVersion@0"))();
	FARPROC setout = need(h, "_FSOUND_SetOutput@4"), init = need(h, "_FSOUND_Init@12"), load = need(h, "_FMUSIC_LoadSong@4");
	FARPROC play = need(h, "_FMUSIC_PlaySong@4"), mk = need(h, "_FSOUND_DSP_Create@12"), act = need(h, "_FSOUND_DSP_SetActive@8");
	FARPROC mvol = need(h, "_FMUSIC_SetMasterVolume@8"), err = GetProcAddress(h, "_FSOUND_GetError@0");
	printf("FMOD %.2f setoutput %lu\n", ver, CALL(setout, 0));   // FSOUND_OUTPUT_NOSOUND
	printf("init %lu err %ld\n", CALL(init, 44100, 32, 0), err ? (long)CALL(err) : 0);
	DWORD m = CALL(load, (DWORD)file);
	printf("load %lu err %ld\n", m, err ? (long)CALL(err) : 0);
	if (!m) return 4;
	if (vol >= 0) printf("mastervolume %d: %lu\n", vol, CALL(mvol, m, vol));
	// FSOUND_OUTPUT_NOSOUND still mixes, in real time, so the run takes as long as the seconds asked
	DWORD d = CALL(mk, (DWORD)dsp, 1100, 0);
	CALL(act, d, 1);
	want = (long long)(secs * 44100) * 2;
	CALL(play, m);
	DWORD t0 = GetTickCount();
	while (count < want && GetTickCount() - t0 < (DWORD)(secs * 1000 + 5000)) Sleep(50);
	report();
	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 5) { fprintf(stderr, "usage: musiclevel bass|fmod <dll> <module> <seconds> [amp|vol]\n"); return 2; }
	setvbuf(stdout, NULL, _IONBF, 0);
	HMODULE h = LoadLibraryExA(argv[2], NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!h) { fprintf(stderr, "load %s failed %lu\n", argv[2], GetLastError()); return 2; }
	int extra = argc > 5 ? atoi(argv[5]) : -1;
	return argv[1][0] == 'b' ? bass(h, argv[3], atof(argv[4]), extra) : fmod3(h, argv[3], atof(argv[4]), extra);
}
