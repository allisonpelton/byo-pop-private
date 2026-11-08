#ifndef __BONNIES_PLATFORM_H__
#define __BONNIES_PLATFORM_H__

// The platform layer (0x41f9b0-0x420da0) and the platform object every app derives from (the vtable 0x4597d0,
// 0x42a4d0-0x42aed0; the object is 0x481a58). Its virtuals are the app's: the engine's app (App.h) and the game's
// override them. The Windows parts (the window's position and menu, the single-instance mutex) go to the host.

#include "Str.h"

namespace bonnies
{

struct Event;

class Platform
{
public:
	// 0x42a4e0
	Platform();
	// 0x42a4f0
	virtual ~Platform();

	// slot 0, 0x42a690 (PL_BaseInit): makes the engine's objects and opens the screen
	virtual void init(int theWidth, int theHeight, int theFullScreen);
	// slot 1, 0x42ade0: frees them
	virtual void shutdown();
	// slot 2, 0x42a500: notes the time while active
	virtual void draw();
	// slot 3, 0x42a510: a frame's housekeeping (events, resources, floating texts, sound)
	virtual void update();
	// slot 4, 0x42a570
	virtual int handleEvent(Event* theEvent);
	// slots 5-8 (0x421700, 0x421620, 0x421530, 0x421430) pass their texts on with empty ones; slot 9, 0x42a580: a message
	// box with theText1 (the game's shows its own dialog with the texts as its title, text and buttons)
	virtual int messageBox(int theFlags, const Str& theText1, const Str& theText2 = Str(), const Str& theText3 = Str(),
		const Str& theText4 = Str(), const Str& theText5 = Str());
	// slot 10, 0x42a4d0
	virtual int isPaused();
	// slot 11, 0x4217d0
	virtual bool isRunning();
	// slots 12, 13, 0x4217e0: the options' volumes
	virtual float getSoundVolume();
	virtual float getMusicVolume();
	// slot 14, 0x4217f0: the window moved (WM_MOVE)
	virtual void windowMoved(int theX, int theY, int theClientX, int theClientY);
	// slots 15, 16, 0x401050: where the window goes
	virtual void getWindowPos(int* theX, int* theY);
	virtual void getWindowPos2(int* theX, int* theY);
	// slot 17, 0x42a4d0: whether the app idles (the frame rate drops to 0.75 a second)
	virtual int isIdle();
	// slot 18, 0x421800: a command of the window menu (which the port has not: see ENGINE.md)
	virtual void menuCommand(int theCommand);
	// slot 19, 0x42a4d0 (the app's: whether a modal screen is up)
	virtual int isModal();
	// slots 20, 21, 0x41f560: around a screen's push
	virtual void beginScreen();
	virtual void endScreen();

	int mDebugTexts;        // +0x04: text boxes drawn
	int mDebugScreens;      // +0x08: screen rectangles drawn
	int mShowFPS;           // +0x0c: the frame rate shown
	int mFlashRects;        // +0x10: the redrawn rectangles outlined
	int mFullRedraw;        // +0x14: the whole screen redrawn each frame
	int mFrameLimit;        // +0x18: frames a second at most (0 none)
	int mField1C;           // +0x1c (1)
	int mDesktopWidth;      // +0x20
	int mDesktopHeight;     // +0x24
	int mDesktopBits;       // +0x28
	int mSkipDraws;         // +0x2c: frames not drawn
	uint32_t mActiveTime;   // +0x30: the last update while active
	uint32_t mInactiveTime; // +0x34: the last update while inactive
	int mField38;           // +0x38
};

extern Platform* gPlatform;  // 0x481a58
extern int gQuit;            // 0x481a5c

// 0x41f9b0: sets 0x4809b8 and runs the main loop (0x403b80, the game's)
// 0x41f9d0: MessageBoxA; theFlags 1 OK (2 with cancel), 4 yes/no (2 with cancel); the result 1 OK, 2 cancel, 4 yes, 8 no
int systemMessageBox(const Str& theText, const Str& theCaption, unsigned int theFlags);
// 0x41fb20: the window's menu (none: the menu handle 0x4809c4 is never set) and its frame
void updateWindowMenu();
// 0x41fbb0: CheckMenuItem on that menu
void checkMenuItem(unsigned int theId, int theChecked);
// 0x41fbd0, 0x41fc10: moves the window (to 50, 100)
void moveWindow(int theX, int theY);
void moveWindowDefault();
// 0x41fc50: the desktop's size and bits per pixel
void getDesktop(int* theWidth, int* theHeight, int* theBits);
// 0x41fcd0, 0x41fdf0: the clipboard's text
void setClipboard(const Str& theText);
Str getClipboard();
// 0x41fee0: Print Screen (GetAsyncKeyState) becomes a key down and up of SDLK_PRINT when released
void checkPrintScreen();
// 0x41ff40: the cursor's position in the window when windowed
int getCursorPos(int* theX, int* theY);
// 0x41ffc0: whether another copy runs (the named mutex), with a message box; 0x4200e0 closes the mutex
int checkRunning(const Str& theName, const Str& theText, const Str& theCaption);
void closeRunning();
// 0x420100: whether the window is minimized
int isIconic();
// 0x420160: GetUserNameA
Str userName();
// 0x420230: the double-click time in ms, 1.2 times the system's; 0x420260: the double-click distance, 20
int doubleClickTime();
int doubleClickDistance();
// 0x420270, 0x420320, 0x4203d0, 0x420480: SavedGames, Sound, Data and Graphics (with a slash when theSlash)
Str savedGamesDir(bool theSlash);
Str soundDir(bool theSlash);
Str dataDir(bool theSlash);
Str graphicsDir(bool theSlash);
// 0x420530: whether the app is inactive (not visible, minimized, or windowed without the input focus)
int isInactive();
// 0x420580: whether Windows is Vista or later; 0x420590: whether the desktop has 256 colours or fewer
bool isVista();
int isLowColour();
// 0x4205c0: ShellExecuteA "open"
void openURL(const Str& theURL);
// 0x420760: pumps the events once
void pumpEvents();
// 0x420770: waits until theTime, pumping events and sleeping; with theInactive it wakes up every 200 ms and stops
// once the app is active again
void waitUntil(int theTime, int theInactive);
// 0x41f4c0: the last time the app was active (0x480978)
extern uint32_t gLastActiveTime;
void noteActiveTime();

// Portal.cfg (0x420830-0x420da0, the vtable 0x458be4; main makes it, 0x480b08 points to it): the distributor's
// settings
class PortalConfig final
{
public:
	// 0x420830
	PortalConfig();
	// 0x420860 (not virtual)
	~PortalConfig();
	// slot 0, 0x420ad0: reads Data/Portal.cfg
	virtual void load();
	// slot 1, 0x41f560: nothing (called before it is deleted)
	virtual void unload();

	int mStartFullScreen;      // +0x04
	int mQuote;                // +0x08 (1)
	int mDistributorLogoFirst; // +0x0c
	// +0x10, the buy page's address: port, gone with the registration
};

extern PortalConfig* gPortalConfig;  // 0x480b08

// 0x420870: reads "theKey = value" at thePos of theText: the value (up to a control character) and whether it is true
// (it does not start with 0, F or N)
int readConfigValue(const Str& theText, int* thePos, const Str& theKey, Str* theValue, int* theBool);

} // namespace bonnies

#endif
