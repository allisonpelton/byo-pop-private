#ifndef __NINGPO_PG_H__
#define __NINGPO_PG_H__

// Ning Po Mahjong Deluxe (Pyrogon, published by PopCap Games) runs on Pyrogon's own framework (shared with Candy
// Cruncher): PGApp, PGControl, PGDialog and PGSprite on top of the "H" layer (HDisplay, HInput, HAudio, HCvarManager,
// HRegistry, a file system over the game's ningpo.pyz archive). The engine/ folder is that framework, translated: the parts
// the game uses, in the original's shape. Where the original talked to Windows (the window, GDI, DirectSound, the
// registry, timeGetTime, Sleep, ShellExecute) the engine calls the port's host instead (Host below), which the app
// in src/NingPoApp.cpp implements on the shared framework and platform layer.
//
// Addresses in the comments are those of the Ningpo.exe linked on 2002-09-25 (games/ningpo/docs/README.md).

#include <cstdint>
#include <string>
#include <vector>

namespace pg
{

// The original's rectangles are {left, top, right, bottom}; right and bottom are exclusive for drawing
struct PGRect
{
	int left, top, right, bottom;
};

struct PGPoint
{
	int x, y;
};

// PyTime: the original keeps its times as 64-bit milliseconds (timeGetTime(), widened)
typedef int64_t PyTime;

// The input events (0x30 bytes in the original; the window procedure 0x433290 makes them from the window's messages)
enum
{
	EV_NONE = 0,
	EV_QUIT = 1,             // WM_CLOSE
	EV_PASTE = 2,            // Ctrl+V
	EV_TOGGLE_SCREEN = 3,    // Alt+Enter
	EV_PAINT = 4,            // WM_PAINT: everything is redrawn
	EV_ACTIVATE = 5,         // WM_ACTIVATE: mValue says active
	EV_HIDDEN = 6,           // the window was minimized (mValue 1) or shown again (0)
	EV_UP = 7,               // a button or key went up
	EV_DOWN = 8,             // a button or key went down, or a character was typed
	EV_MOUSE_MOVE = 0xc,
	EV_WHEEL = 0xd,          // mValue: the notches
	EV_TICK = 0xe,           // the 10 ms timer, and the input's idle pass
};

// mCode of a mouse event
enum
{
	BUTTON_LEFT = 1,
	BUTTON_RIGHT = 2,
};

struct PGEvent
{
	int mType;          // +0x00
	int mUnused04;
	PyTime mTime;       // +0x08 when it was posted
	int mValue;         // +0x10 EV_ACTIVATE, EV_HIDDEN, EV_WHEEL
	int mModifiers;     // +0x14 Ctrl 4 | Alt 2 | Shift 1
	int mUnused18;
	int mUnused1c;
	int mCode;          // +0x20 the mouse button, or the virtual key of a key without a character
	uint8_t mChar;      // +0x24 the character typed (EV_DOWN with mCode 0), or of a key that went up
	uint8_t mUnused25[3];
	int mX;             // +0x28 the mouse position
	int mY;             // +0x2c
};

// The port's host: what the original got from Windows
class Host
{
public:
	virtual ~Host() {}
	// timeGetTime()
	virtual uint32_t TimeMs() = 0;
	// Sleep(): the host runs its frame here (the window's messages, which post input events, and the screen)
	virtual void Sleep(int theMs) = 0;
	// port: a test hook, called as the game reads its event queue, before the event read becomes the current one
	virtual void EventRead() {}
	// port: a test hook, a sound the mixer started (theVoice of the 8, or -1 when none was free)
	virtual void SoundStarted(const char* theName, int theVoice) { (void)theName; (void)theVoice; }
	// The display flipped: the screen holds a new frame
	virtual void ScreenChanged() = 0;
	// A file of the game's folder (ningpo.pyz)
	virtual bool ReadGameFile(const std::string& theName, std::vector<uint8_t>& theData) = 0;
	// The registry values under HKEY_CURRENT_USER\Software\<company>\<product>: strings (REG_SZ) and binary data
	virtual bool RegistryReadString(const std::string& theName, std::string& theValue) = 0;
	virtual bool RegistryWriteString(const std::string& theName, const std::string& theValue) = 0;
	virtual bool RegistryReadData(const std::string& theName, std::vector<uint8_t>& theData) = 0;
	virtual bool RegistryWriteData(const std::string& theName, const void* theData, size_t theSize) = 0;
	// The text on the clipboard (Ctrl+V)
	virtual std::string ClipboardText() = 0;
	// The name of the user's account, empty when there is none (0x414168, GetUserNameA): the name entry's default
	virtual std::string UserName() = 0;
	// The screen mode the game asks for
	virtual void SetFullscreen(bool theFullscreen) = 0;
	virtual void Log(const char* theText) = 0;
	virtual void ShowMessage(const char* theCaption, const char* theText) = 0;
};

void SetHost(Host* theHost);
Host* GetHost();

// 0x40d2fa PGLog, 0x40d2c1 PGAlert, 0x40d289 PGMessageBox (flags dropped: the port only shows the text)
void PGLog(const char* theFormat, ...);
void PGAlert(const char* theFormat, ...);
void PGMessageBox(unsigned int theFlags, const char* theCaption, const char* theFormat, ...);

// 0x41415f HTimeGetTime
uint32_t HTimeGetTime();
// 0x40d3f0 HSin: the sine of an angle in degrees (the x87's fsin, in its precision)
double HSin(float theDegrees);
// 0x414192: Sleep()
void HSleep(uint32_t theMs);
// port: Host::EventRead (a test hook)
void HEventRead();
// 0x4138ed HStrncpy: copies at most theSize - 1 characters and always terminates
void HStrncpy(char* theDest, const char* theSrc, int theSize);
// 0x41387c HStricmp
int HStricmp(const char* a, const char* b);
// 0x4139f9 HSnprintf
int HSnprintf(char* theBuf, int theSize, const char* theFormat, ...);

// 0x40d47b PyTime::sub; 0x40d45c: the difference, 0 when it does not fit 32 bits
inline int64_t PyTimeSub(PyTime a, PyTime b) { return a - b; }
int PyTimeDelta(PyTime a, PyTime b);

// The dirty rectangles (0x413d10..0x413fd3): at most 512, merged as they are added
class HRectList
{
public:
	PGRect mRects[512];
	int mCount;

	HRectList() : mCount(0) {}
	void add(int theTop, int theLeft, int theBottom, int theRight);  // 0x413d3f
	void add(const PGRect& theRect) { add(theRect.top, theRect.left, theRect.bottom, theRect.right); }  // 0x413ed6
	int count() const { return mCount; }  // 0x413fbd
	int area() const;                     // 0x413fd3
	PGRect* get(int i) { return &mRects[i]; }  // 0x413fc4
	void clear() { mCount = 0; }          // 0x413fb5
};

// 0x413eed: the rectangles overlap or touch
bool RectsTouch(const PGRect& a, const PGRect& b);

} // namespace pg

#endif
