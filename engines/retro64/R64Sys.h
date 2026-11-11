#ifndef __RETRO64_R64SYS_H__
#define __RETRO64_R64SYS_H__

// Retro64's system layer (0x428000..0x42a800 (cosmicbugs), 0x4284e0..0x42ace0 (waterbugs), 0x42e1d0 (cosmicbugs),
// 0x42e6b0 (waterbugs)..): what the original asked of Windows, DirectDraw, DirectInput and BASS. Each game's host
// (src/CosmicBugsApp, src/WaterBugsApp) implements SysHost on the shared platform layer.

#include <cstdint>
#include <vector>

namespace r64
{

struct PixelFormat;

class SysHost
{
public:
	virtual ~SysHost() {}
	// A whole file of the game's; thePath is relative to the game's folder, with either separator
	virtual bool ReadFile(const char* thePath, std::vector<uint8_t>& theData) = 0;
	// 0x42e6e0 (cosmicbugs), 0x42ebc0 (waterbugs) (through 0x428f50 (cosmicbugs), 0x429430 (waterbugs)): a rectangle of
	// the screen that changed, for the next present
	virtual void AddUpdateRect(int x, int y, int theWidth, int theHeight) {}
	// 0x428890 (cosmicbugs), 0x428d70 (waterbugs): puts the mouse pointer at (x, y) of the game's screen (the original
	// moves the system's)
	virtual void SetMousePos(int x, int y) {}
	// 0x429e40 (cosmicbugs), 0x42a320 (waterbugs): where the mouse pointer is on the game's screen (the original reads
	// the system's and takes off the window's place)
	virtual void GetMousePos(int* x, int* y) { *x = 0; *y = 0; }

	// The screen (the original's DirectDraw surfaces, 0x42e1d0 (cosmicbugs), 0x42e6b0 (waterbugs)..): 800x600 in
	// Sys_ScreenFormat's format. LockScreen gives its pixels to draw into (false: it cannot be drawn now), Present
	// shows what was drawn (theWait: on the display's refresh); SetScreenMode applies the windowed or full screen mode,
	// stretched to the display or centred in it.
	virtual bool LockScreen(uint8_t** theBits, int* thePitch) = 0;
	virtual void UnlockScreen() {}
	virtual void Present(int theWait) {}
	virtual void SetScreenMode(int theFullscreen, int theCentered) {}
	// timeGetTime
	virtual uint32_t TimeMs() = 0;
	// The window is the active one (WM_ACTIVATEAPP)
	virtual bool IsActive() { return true; }
	// port(mod): a fix of the fixes mod is on (never in the original, nor without the mod)
	virtual bool FixOn(const char* theFix) { return false; }
	// port(site): the site's "Always skip start-up logos" (or -skiplogos) is on
	virtual bool SkipLogos() { return false; }
	// BASS's samples (a sound's id, -1 when it cannot be loaded) and its one music (BASS_MusicLoad with
	// BASS_SAMPLE_LOOP | BASS_MUSIC_RAMP; PlayMusic(1) from the start); theVolume is BASS's music volume, 0 to 100
	virtual int LoadSound(const char* thePath) { return -1; }
	virtual void PlaySound(int theSound) {}
	virtual void StopSounds() {}
	virtual bool LoadMusic(const char* thePath) { return false; }
	virtual void FreeMusic() {}
	virtual void PlayMusic(int theRestart) {}
	virtual void PauseMusic() {}
	virtual void SetMusicVolume(int theVolume) {}
	// The player's files (cp.cfg, cb.dat): where one is opened (the original's FixPath: the game's folder), and
	// one that was written (the web build keeps it)
	virtual const char* SavePath(const char* theName) = 0;
	virtual void SaveDone() {}
	// GetUserNameA, "" when there is none
	virtual void UserName(char* theName, int theSize)
	{
		if (theSize > 0)
			theName[0] = 0;
	}
};

void Sys_SetHost(SysHost* theHost);
SysHost* Sys_Host();

// fopen/fseek/ftell/fread on the whole file, as the loaders do: false when it cannot be read
bool Sys_ReadFile(const char* thePath, std::vector<uint8_t>& theData);
// The port's: a game file is there
bool Sys_FileExists(const char* thePath);
// The port's: the host's SavePath, SaveDone and UserName (0x429d30 (cosmicbugs), 0x42a210 (waterbugs))
const char* Sys_SavePath(const char* theName);
void Sys_SaveDone();
void Sys_UserName(char* theName, int theSize);
// 0x428f50 (cosmicbugs), 0x429430 (waterbugs)
void Sys_AddUpdateRect(int x, int y, int theWidth, int theHeight);
// 0x428890 (cosmicbugs), 0x428d70 (waterbugs)
void Sys_SetMousePos(int x, int y);
// 1: PumpMessages goes on while the window is not the active one (the game's demo mode sets it)
extern int gPumpMessages;
// The input (0x4289d0..0x428b20 (cosmicbugs), 0x428eb0..0x429000 (waterbugs)), which the window's messages fill: the
// mouse buttons held, the keys held (by virtual-key code) and a ring of the last 100 keys pressed. The host feeds them
// its events with Sys_KeyEvent and Sys_MouseButton.
// 0x4289d0 (cosmicbugs), 0x428eb0 (waterbugs): 1 the left button, 2 the right, 3 the middle: held
int Mouse_State(int theButton);
int Mouse_Pressed(int theButton);        // 0x428a00 (cosmicbugs), 0x428ee0 (waterbugs): once for each press
// 0x428aa0 (cosmicbugs), 0x428f80 (waterbugs): the queue's newest key, 0 when it is empty
int Key_Pop();
void Key_Push(int theKey);               // 0x428ad0 (cosmicbugs), 0x428fb0 (waterbugs)
// 0x428b00 (cosmicbugs), 0x428fe0 (waterbugs): empties the queue (not the keys held)
void Key_Clear();
int Key_Down(int theKey);                // 0x428b20 (cosmicbugs), 0x429000 (waterbugs): held
// 0x428b40 (cosmicbugs), 0x429020 (waterbugs): once for each press (the key's slot goes from 1 to 2)
int Key_Pressed(int theKey);
// 0x429e40 (cosmicbugs), 0x42a320 (waterbugs): the pointer on the game's screen
void Mouse_Update(int* x, int* y);
void Sys_KeyEvent(int theKey, int theDown);
void Sys_MouseButton(int theButton, int theDown);
// 0x428eb0 (cosmicbugs), 0x429390 (waterbugs): the screen's pixel format (gScreenFormat 0x46e5b8 (cosmicbugs),
// 0x46e5b8 (waterbugs) points at it)
const PixelFormat* Sys_ScreenFormat();
void Sys_SetScreenFormat(const PixelFormat* theFmt);

// The screen (0x428e70..0x428ff0 (cosmicbugs), 0x429350..0x4294d0 (waterbugs)) and the window's messages
struct ScreenLock
{
	int mWidthPadding;          // +0x00  nothing reads it
	int mW;                     // +0x04
	int mH;                     // +0x08
	int mPitch;                 // +0x0c
	uint8_t* mBits;             // +0x10
};
uint32_t TimeMs();                       // 0x428e70 (cosmicbugs), 0x429350 (waterbugs)
void Sys_Sleep(int theMs);               // 0x428820 (cosmicbugs), 0x428d00 (waterbugs)
bool Sys_FixOn(const char* theFix);      // port(mod): a fix of the fixes mod is on
bool Sys_SkipLogos();                    // port(site): the start-up logos are skipped, as Esc on the first one does
// 0x428ec0 (cosmicbugs), 0x4293a0 (waterbugs): called after PumpMessages opened the screen again, with the screen's format
void SetScreenLostCallback(void (*theCallback)(const PixelFormat*));
// 0x428ed0 (cosmicbugs), 0x4293b0 (waterbugs): called after PumpMessages opened the screen again
void SetScreenChangeCallback(void (*theCallback)());
int Screen_Fullscreen();                 // 0x428f00 (cosmicbugs), 0x4293e0 (waterbugs)
void Screen_SetFullscreen(int theOn);    // 0x428ee0 (cosmicbugs), 0x4293c0 (waterbugs): at the next PumpMessages
// 0x428f40 (cosmicbugs), 0x429420 (waterbugs): 0 the screen is stretched to the display
int Screen_Centered();
void Screen_SetCentered(int theOn);      // 0x428f10 (cosmicbugs), 0x4293f0 (waterbugs): at the next PumpMessages
void Present(int theWait);               // 0x428f70 (cosmicbugs), 0x429450 (waterbugs)
int LockScreen(ScreenLock* theLock);     // 0x428fa0 (cosmicbugs), 0x429480 (waterbugs): 0 when it cannot be drawn
void UnlockScreen();                     // 0x428fe0 (cosmicbugs), 0x4294c0 (waterbugs)
// 0x42a070 (cosmicbugs), 0x42a550 (waterbugs): the window's messages are the host's, between frames; this is what
// follows them: 0 while the window is inactive, when the original waited for it, else 1 after the screen mode asked for
// (F7, F8 and the options) is applied
int PumpMessages();
// 0x42a340 (cosmicbugs), 0x42a820 (waterbugs): cp.cfg's settings (the mode, the sound and the music), written with the
// defaults when it is missing
void LoadConfig();
void SaveConfig();                       // 0x429db0 (cosmicbugs), 0x42a290 (waterbugs)
// WM_ACTIVATEAPP (and WM_SETFOCUS, WM_KILLFOCUS): the keys held are let go, the sound paused or resumed
void Sys_Activate(int theActive);

// The sound (BASS): the samples by id and the one music. A sound list (0x18 bytes) keeps them by file name.
struct SoundEntry
{
	SoundEntry* mNext;          // +0x000
	SoundEntry* mPrev;          // +0x004
	char mName[0x100];          // +0x008
	int mSound;                 // +0x108
};
struct SoundList
{
	SoundEntry* mCurrent;       // +0x00
	SoundEntry* mHead;          // +0x04
	SoundEntry* mTail;          // +0x08
	void* mCountPadding;        // the four bytes before count: nothing reads them
	int mCount;                 // +0x10
	int mEndPadding;            // nothing reads it
};
// 0x42b370 (cosmicbugs), 0x42b850 (waterbugs): its id, -1 when it cannot be loaded
int Sample_Load(const char* thePath);
int Sound_Play(int theSound);            // 0x429fb0 (cosmicbugs), 0x42a490 (waterbugs)
int Sound_StopAll();                     // 0x42a570 (cosmicbugs), 0x42aa50 (waterbugs)
int Sound_On();                          // 0x4290f0 (cosmicbugs), 0x4295d0 (waterbugs)
void Sound_SetOn(int theOn);             // 0x42a010 (cosmicbugs), 0x42a4f0 (waterbugs)
// 0x428650 (cosmicbugs), 0x428b30 (waterbugs): the id of the sound loaded from thePath (0 when there is none: the first
// one's)
int SoundList_Find(SoundList* theList, const char* thePath);
// 0x428770 (cosmicbugs), 0x428c50 (waterbugs): loads it unless it is in the list
int SoundList_Load(SoundList* theList, const char* thePath);
int Music_SetVolume(int theVolume);      // 0x429000 (cosmicbugs), 0x4294e0 (waterbugs): 0 to 255
int Music_Load(const char* thePath);     // 0x429030 (cosmicbugs), 0x429510 (waterbugs)
int Music_Free();                        // 0x429070 (cosmicbugs), 0x429550 (waterbugs)
// 0x429090 (cosmicbugs), 0x429570 (waterbugs): from the start, when the music is on
int Music_Play();
int Music_Resume();                      // 0x4290c0 (cosmicbugs), 0x4295a0 (waterbugs)
int Music_On();                          // 0x429100 (cosmicbugs), 0x4295e0 (waterbugs)
void Music_SetOn(int theOn);             // 0x42a020 (cosmicbugs), 0x42a500 (waterbugs)
// 0x42ad70 (cosmicbugs), 0x42b250 (waterbugs): BASS_Init (the port: on the host's sound)
void Sound_Start();
void Sound_Shutdown();                   // 0x42a6a0 (cosmicbugs), 0x42ab80 (waterbugs)
// 0x42a610 (cosmicbugs), 0x42aaf0 (waterbugs): the music paused and the sounds stopped
int Sound_Pause();

} // namespace r64

#endif
