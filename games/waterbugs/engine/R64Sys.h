#ifndef __COSMICBUGS_R64SYS_H__
#define __COSMICBUGS_R64SYS_H__

// Retro64's system layer (0x4284e0..0x42ace0, 0x42e6b0..): what the original asked of Windows, DirectDraw,
// DirectInput and BASS. The port's host (src/CosmicBugsApp) implements SysHost on the shared platform layer.

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
	// 0x42ebc0 (through 0x429430): a rectangle of the screen that changed, for the next present
	virtual void AddUpdateRect(int x, int y, int w, int h) {}
	// 0x428d70: puts the mouse pointer at (x, y) of the game's screen (the original moves the system's)
	virtual void SetMousePos(int x, int y) {}
	// 0x42a320: where the mouse pointer is on the game's screen (the original reads the system's and takes off the
	// window's place)
	virtual void GetMousePos(int* x, int* y) { *x = 0; *y = 0; }

	// The screen (the original's DirectDraw surfaces, 0x42e6b0..): 800x600 in Sys_ScreenFormat's format. LockScreen
	// gives its pixels to draw into (false: it cannot be drawn now), Present shows what was drawn (theWait: on the
	// display's refresh); SetScreenMode applies the windowed or full screen mode, stretched to the display or
	// centred in it.
	virtual bool LockScreen(uint8_t** theBits, int* thePitch) = 0;
	virtual void UnlockScreen() {}
	virtual void Present(int theWait) {}
	virtual void SetScreenMode(int theFullscreen, int theCentered) {}
	// timeGetTime
	virtual uint32_t TimeMs() = 0;
	// The window is the active one (WM_ACTIVATEAPP)
	virtual bool IsActive() { return true; }
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
// The port's: the host's SavePath, SaveDone and UserName (0x42a210)
const char* Sys_SavePath(const char* theName);
void Sys_SaveDone();
void Sys_UserName(char* theName, int theSize);
// 0x429430
void Sys_AddUpdateRect(int x, int y, int w, int h);
// 0x428d70
void Sys_SetMousePos(int x, int y);
// 0x70819c: while it is 1, PumpMessages does not wait for the window to be active (and the original picks which
// focus messages pause the sound by it); only the demo's power-ups set it
extern int gPumpMessages;

// The input (0x428eb0..0x429000), which the window's messages fill: the mouse buttons held, the keys held (by
// virtual-key code) and a ring of the last 100 keys pressed. The host feeds them its events with Sys_KeyEvent and
// Sys_MouseButton.
int Mouse_State(int theButton);          // 0x428eb0: 1 the left button, 2 the right, 3 the middle: held
int Mouse_Pressed(int theButton);        // 0x428ee0: once for each press
int Key_Pop();                           // 0x428f80: the queue's newest key, 0 when it is empty
void Key_Push(int theKey);               // 0x428fb0
void Key_Clear();                        // 0x428fe0: empties the queue (not the keys held)
int Key_Down(int theKey);                // 0x429000: held
int Key_Pressed(int theKey);             // 0x429020: once for each press (the key's slot goes from 1 to 2)
void Mouse_Update(int* x, int* y);       // 0x42a320: the pointer on the game's screen
void Sys_KeyEvent(int theKey, int theDown);
void Sys_MouseButton(int theButton, int theDown);
// 0x429390: the screen's pixel format (gScreenFormat 0x46e5b8 points at it)
const PixelFormat* Sys_ScreenFormat();
void Sys_SetScreenFormat(const PixelFormat* theFmt);

// The screen (0x429350..0x4294d0) and the window's messages
struct ScreenLock
{
	int unk00;
	int w;                     // +0x04
	int h;                     // +0x08
	int pitch;                 // +0x0c
	uint8_t* bits;             // +0x10
};
uint32_t TimeMs();                       // 0x429350
void Sys_Sleep(int theMs);               // 0x428d00
// 0x4293a0 and 0x4293b0: called after PumpMessages opened the screen again, the first with the screen's format
void SetScreenLostCallback(void (*theCallback)(const PixelFormat*));
void SetScreenChangeCallback(void (*theCallback)());
int Screen_Fullscreen();                 // 0x4293e0
void Screen_SetFullscreen(int theOn);    // 0x4293c0: at the next PumpMessages
int Screen_Centered();                   // 0x429420: 0 the screen is stretched to the display
void Screen_SetCentered(int theOn);      // 0x4293f0: at the next PumpMessages
void Present(int theWait);               // 0x429450
int LockScreen(ScreenLock* theLock);     // 0x429480: 0 when it cannot be drawn
void UnlockScreen();                     // 0x4294c0
// 0x42a550: the window's messages are the host's, between frames; this is what follows them: 0 while the window is
// inactive, when the original waited for it (unless gPumpMessages), else 1 after the screen mode asked for (F7, F8
// and the options) is applied
int PumpMessages();
// 0x42a820: cp.cfg's settings (the mode, the sound and the music), written with the defaults when it is missing
void LoadConfig();
void SaveConfig();                       // 0x42a290
// WM_ACTIVATEAPP (and WM_SETFOCUS, WM_KILLFOCUS): the keys held are let go, the sound paused or resumed
void Sys_Activate(int theActive);

// The sound (BASS): the samples by id and the one music. A sound list (0x18 bytes) keeps them by file name.
struct SoundEntry
{
	SoundEntry* next;          // +0x000
	SoundEntry* prev;          // +0x004
	char name[0x100];          // +0x008
	int sound;                 // +0x108
};
struct SoundList
{
	SoundEntry* current;       // +0x00
	SoundEntry* head;          // +0x04
	SoundEntry* tail;          // +0x08
	void* unk0c;
	int count;                 // +0x10
	int unk14;
};
int Sample_Load(const char* thePath);    // 0x42b850: its id, -1 when it cannot be loaded
int Sound_Play(int theSound);            // 0x42a490
int Sound_StopAll();                     // 0x42aa50
int Sound_On();                          // 0x4295d0
void Sound_SetOn(int theOn);             // 0x42a4f0
// 0x428b30: the id of the sound loaded from thePath (0 when there is none: the first one's)
int SoundList_Find(SoundList* theList, const char* thePath);
// 0x428c50: loads it unless it is in the list
int SoundList_Load(SoundList* theList, const char* thePath);
int Music_SetVolume(int theVolume);      // 0x4294e0: 0 to 255
int Music_Load(const char* thePath);     // 0x429510
int Music_Free();                        // 0x429550
int Music_Play();                        // 0x429570: from the start, when the music is on
int Music_Resume();                      // 0x4295a0
int Music_On();                          // 0x4295e0
void Music_SetOn(int theOn);             // 0x42a500
void Sound_Start();                      // 0x42b250: BASS_Init (the port: on the host's sound)
void Sound_Shutdown();                   // 0x42ab80
int Sound_Pause();                       // 0x42aaf0: the music paused and the sounds stopped

} // namespace r64

#endif
