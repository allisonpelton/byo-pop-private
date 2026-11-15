#ifndef __JB_HOST_H__
#define __JB_HOST_H__

// The port's host: what the engine got from Windows (the clock, the resources of the exe and of Resource.dll, the
// registry, the command line, the folders, ShellExecute, message boxes). The app in src/ implements it on the
// shared framework and platform layer, as Ning Po's does (games/ningpo/engine/Pg.h).

#include "JBBase.h"

#include <string>

namespace jb
{

class Host
{
public:
	virtual ~Host() {}
	// GetTickCount, timeGetTime
	virtual uint32_t TickCount() = 0;
	// the machine's own clock, which the deal's time limit reads whatever clock the game runs on (fixes mod
	// "steadyClock")
	virtual uint32_t RealTickCount() { return TickCount(); }
	// port: the number of the update the game is in, for the test log of the sound (BYOPOP_SOUNDLOG, whose lines the
	// framework's sound log writes the same way); the clock when the host counts none
	virtual uint32_t UpdateCount() { return TickCount(); }
	// the number that seeds the random generator: the clock, or the seed given to the run (fixes mod "seededDeals")
	virtual uint32_t SeedTick() { return TickCount(); }
	// FindResourceA, SizeofResource, LockResource: a type given by name ("TXT", "JPG"...) or, with a null name, by
	// its id (RT_BITMAP 2, RT_STRING 6...)
	virtual const uint8_t* FindResource(Module theModule, const char* theTypeName, int theTypeId, int theId,
		uint32_t* theSize) = 0;
	// LoadStringA
	virtual bool LoadString(Module theModule, int theId, std::string& theText) = 0;
	// The registry under Software\<company>\<product>[\<section>] of HKEY_CURRENT_USER, or of
	// HKEY_LOCAL_MACHINE when theMachine (read only: ActivateLinks)
	virtual bool RegReadInt(const char* theSection, const char* theName, bool theMachine, int& theValue) = 0;
	virtual bool RegWriteInt(const char* theSection, const char* theName, int theValue) = 0;
	// GetCommandLineA, after the program's name
	virtual std::string CommandLine() = 0;
	// SHGetFolderPathA's folder for a CSIDL (with the company and product appended, and a backslash at the end),
	// or the exe's folder
	virtual std::string FolderPath(int theCsidl) = 0;
	virtual std::string ExeFolder() = 0;  // with a separator at the end (Splash appends "SplashLogo")
	// GetSystemMetrics(SM_CXSCREEN, SM_CYSCREEN), or -ScreenWidth and -ScreenHeight (0x426480 (mahjongjapan)): the
	// desktop's size
	virtual void ScreenSize(int& theWidth, int& theHeight) { theWidth = SCREEN_WIDTH; theHeight = SCREEN_HEIGHT; }
	// MessageBoxA (0x435390 (mahjongchina), 0x4308f0 (mahjongjapan), 0x4353e0 (mahjongchina), 0x430940 (mahjongjapan)):
	// the answer (IDOK 1, IDYES 6, IDNO 7)
	virtual int MessageBox(const char* theText, const char* theCaption, unsigned int theFlags) = 0;
	virtual void Log(const char* theText) = 0;
	// GetKeyState of the mouse buttons, Shift and Ctrl as the MK_ flags (0x422740 (mahjongchina),
	// 0x42c660 (mahjongjapan)): 1 left, 2 right, 4 Shift, 8 Ctrl, 0x10 middle
	virtual int InputFlags() = 0;
	// GetCursorPos and ScreenToClient, in the 800x600 screen; theOutside when it is off the window
	virtual void GetMousePos(int& theX, int& theY, bool& theOutside) = 0;
	// The window's cursor (0x4229c0 (mahjongchina), 0x42c8f0 (mahjongjapan)): 0 arrow, 1 the hand (exe cursor 0x34), 2
	// (0x4c or 0x4b), 3 (0x33), 4 wait
	virtual void SetCursor(int theKind) = 0;
	// SetCapture, ReleaseCapture
	virtual void CaptureMouse(bool theOn) = 0;
	// The clipboard's text (0x422630 (mahjongchina), 0x42c550 (mahjongjapan), 0x4226a0 (mahjongchina),
	// 0x42c5c0 (mahjongjapan))
	virtual bool GetClipboard(std::string& theText) = 0;
	virtual bool SetClipboard(const char* theText) = 0;
	// Sleep (the frame's pacing, 0x421d70 (mahjongchina), 0x42bd40 (mahjongjapan))
	virtual void Sleep(uint32_t theMs) = 0;
	// The display's mode (0x42bfc0 (mahjongchina), 0x432000 (mahjongjapan), 0x42c310 (mahjongchina),
	// 0x432360 (mahjongjapan)): full screen or a window
	virtual void SetFullScreen(bool theOn) = 0;
	// The software screen shown (0x42c9b0 (mahjongchina), 0x432a00 (mahjongjapan)): 800x600, 32-bit B G R A rows
	// top-down
	virtual void Present(const uint8_t* thePixels, int thePitch) = 0;
};

void SetHost(Host* theHost);
Host* GetHost();

} // namespace jb

#endif
