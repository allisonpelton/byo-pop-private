// LOWOS on the port's platform (see Lowos.h). The functions keep the original's names, arguments and checks;
// what DirectX did (surfaces, flips, input devices, sound buffers) is done on the port's buffers instead.

#include "Lowos.h"
#include "framework/SDLSoundManager.h"
#include "framework/SexyAppBase.h"
#include "platform/Audio.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace lowos
{

// The screen is the original's 640x480; a test run can make it larger (HostSetCanvas, TipTopApp's -canvas) to see
// what the game draws beyond it
static int SCREEN_W = 640;
static int SCREEN_H = 480;
// The game asks for mode 1 of the mode table (0x47e318): 640x480, 32 bits, R 8:16, G 8:8, B 8:0.
static const PixelFormat SCREEN_FORMAT = {8, 16, 8, 8, 8, 0};

static std::vector<uint32_t> gScreen(640 * 480);
static ClockFunc gClock = nullptr;
static MessageFunc gMessageFunc = nullptr;
static bool gActive = true;

// Screen state (0x48b160 block)
static bool gScreenInited = false;    // 0x48b160
static bool gWindowed = true;         // 0x48b180 == 0
static int gModeRequest = 0;
static bool gSwitchRequested = false; // 0x48b19f: Alt+Enter or F11 seen by the input poll
static SwitchCallback gOnSwitch = nullptr;  // 0x48b174
static ActivateCallback gActivate = nullptr;  // 0x48b16c
static CloseCallback gClose = nullptr;        // 0x48b170
static bool gInCallback = false;      // 0x48b17c
static uint32_t gColorKey = 0;        // 0x48a278 (logical format)
static Surface* gSurfaces = nullptr;  // 0x48a274
static uint8_t gCursorLock = 0;       // 0x48b17e
static int gMessageDepth = 0;         // 0x48a224

// --- the host ---

uint32_t* HostScreen()
{
	return gScreen.data();
}

void HostScreenSize(int* theWidth, int* theHeight)
{
	*theWidth = SCREEN_W;
	*theHeight = SCREEN_H;
}

void HostSetCanvas(int theWidth, int theHeight)
{
	SCREEN_W = theWidth;
	SCREEN_H = theHeight;
	gScreen.assign((size_t)theWidth * (size_t)theHeight, 0);
}

void HostSetClock(ClockFunc theClock)
{
	gClock = theClock;
}

void HostSetMessageFunc(MessageFunc theFunc)
{
	gMessageFunc = theFunc;
}

// WM_ACTIVATEAPP (0x42ebf7). The activate callback is told 1 when the window was deactivated and 0 when it was
// activated (0x42ed73 pushes 1 on wParam == 0, 0x42edde pushes 0 otherwise): it pauses the game and the music, and
// hides the board, while it is 1.
void HostSetActive(bool isActive)
{
	if (gActive == isActive)
		return;
	gActive = isActive;
	if (gScreenInited && gActivate != nullptr && !gInCallback)
	{
		gInCallback = true;
		gActivate(isActive ? 0 : 1);
		gInCallback = false;
	}
}

void HostRequestScreenSwitch()
{
	gSwitchRequested = true;
}

// WM_CLOSE (0x42ebf7)
void HostClose()
{
	if (gClose != nullptr)
		gClose();
}

int HostTakeScreenModeRequest()
{
	int aRequest = gModeRequest;
	gModeRequest = 0;
	return aRequest;
}

void HostSetWindowed(bool isWindowed)
{
	gWindowed = isWindowed;
}

// --- input state (keyboard 0x48b040, mouse 0x48b380) ---

// One device's event queue (0x431130/0x4310f0/0x4311c0): 16 events of (code | down << 8), the held keys as bits,
// and auto-repeat of the held key if its bit is in the repeat mask.
struct InputQueue
{
	uint8_t mHead = 0;            // +0x00
	uint8_t mCount = 0;           // +0x01
	uint16_t mEvents[16] = {};    // +0x02
	uint8_t mRepeatCode = 0;      // +0x22
	uint32_t mRepeatLeft = 0;     // +0x24
	uint32_t mLastTick = 0;       // +0x28
	uint32_t mDown[8] = {};       // +0x2c
	uint32_t mRepeatMask[8] = {}; // +0x4c
};

static bool gKeyboardInited = false;  // 0x48b040
static InputQueue gKeys;              // 0x48b044
static InputQueue gChars;             // 0x48b0b0 (typed characters, from WM_CHAR)
static bool gMouseInited = false;     // 0x48b380
static InputQueue gMouse;             // 0x48b384
static uint32_t gRepeatDelay = 0;     // 0x47e36c
static uint32_t gRepeatRate = 0;      // 0x47e370
static int gMouseX = 0;
static int gMouseY = 0;

uint32_t TickCount()
{
	return gClock ? gClock() : 0;
}

// 0x4310f0
static void PushEvent(InputQueue* theQueue, uint8_t theCode, char isDown)
{
	if (theQueue->mCount < 0x10)
	{
		theQueue->mEvents[(theQueue->mHead + theQueue->mCount) & 0xf] =
			(uint16_t)((isDown != 0 ? 0x100 : 0) | theCode);
		theQueue->mCount++;
	}
}

// 0x431130
static void SetInputState(InputQueue* theQueue, uint8_t theCode, uint8_t isDown)
{
	uint32_t aWord = theCode >> 5;
	uint32_t aBit = 1u << (theCode & 0x1f);
	if ((theQueue->mDown[aWord] & aBit) != ((uint32_t)isDown << (theCode & 0x1f)))
	{
		theQueue->mDown[aWord] ^= aBit;
		theQueue->mLastTick = TickCount();
		if (isDown == 1)
		{
			PushEvent(theQueue, theCode, 1);
			theQueue->mRepeatCode = 0;
			if ((aBit & theQueue->mRepeatMask[aWord]) != 0)
			{
				theQueue->mRepeatCode = theCode;
				theQueue->mRepeatLeft = gRepeatDelay;
			}
		}
		else if (theQueue->mRepeatCode == theCode)
			theQueue->mRepeatCode = 0;
	}
}

// 0x4311c0: the held key's repeats since the last poll
static void RepeatInput(InputQueue* theQueue)
{
	if (theQueue->mRepeatCode != 0)
	{
		uint32_t aNow = TickCount();
		uint32_t aLast = theQueue->mLastTick;
		if (aNow < aLast)
			aLast = aNow;
		theQueue->mLastTick = aNow;
		uint32_t anElapsed = aNow - aLast;
		if (theQueue->mRepeatLeft <= anElapsed)
		{
			uint32_t aLeft;
			do
			{
				anElapsed -= theQueue->mRepeatLeft;
				PushEvent(theQueue, theQueue->mRepeatCode, 1);
				while (gRepeatRate < anElapsed)
				{
					PushEvent(theQueue, theQueue->mRepeatCode, 1);
					anElapsed -= gRepeatRate;
				}
				aLeft = gRepeatRate - anElapsed;
				theQueue->mRepeatLeft = aLeft;
			} while (aLeft <= anElapsed);
		}
		theQueue->mRepeatLeft -= anElapsed;
	}
}

void HostMouseMove(int x, int y)
{
	gMouseX = x;
	gMouseY = y;
}

// The window procedure's WM_xBUTTONDOWN/UP (0x42ebf7 -> 0x4322a0): with the mouse set to window messages
// (InitMouse(0)), button n is queued as code n.
void HostMouseButton(int theButton, bool isDown)
{
	if (gMouseInited)
		PushEvent(&gMouse, (uint8_t)theButton, isDown ? 1 : 0);
}

// WM_CHAR (0x42ee8e)
void HostChar(int theChar)
{
	PushEvent(&gChars, (uint8_t)theChar, 1);
}

// The keyboard poll (0x4316d0): Alt+Enter asks for a screen mode switch and Alt+F4 closes the window (the host
// handles that); other keys go to the scan code queue.
void HostKey(int theScanCode, bool isDown)
{
	if (!gKeyboardInited || theScanCode <= 0 || theScanCode > 0xff)
		return;
	bool anAlt = (gKeys.mDown[1] & 0x1000000) != 0 || (gKeys.mDown[5] & 0x1000000) != 0;
	if (theScanCode == 0x1c && isDown && anAlt)
	{
		gSwitchRequested = true;
		return;
	}
	if (theScanCode == 0x3e && isDown && anAlt)
		return;
	SetInputState(&gKeys, (uint8_t)theScanCode, isDown ? 1 : 0);
}

int VirtualKeyToScanCode(int theVirtualKey)
{
	static const char* const kLetters = "\x1e\x30\x2e\x20\x12\x21\x22\x23\x17\x24\x25\x26\x32\x31\x18\x19\x10\x13\x1f"
		"\x14\x16\x2f\x11\x2d\x15\x2c";
	if (theVirtualKey >= 'A' && theVirtualKey <= 'Z')
		return (uint8_t)kLetters[theVirtualKey - 'A'];
	if (theVirtualKey >= '1' && theVirtualKey <= '9')
		return 0x02 + theVirtualKey - '1';
	if (theVirtualKey >= 0x70 && theVirtualKey <= 0x79)  // F1..F10
		return 0x3b + theVirtualKey - 0x70;
	switch (theVirtualKey)
	{
	case '0': return 0x0b;
	case 0x08: return 0x0e;  // backspace
	case 0x09: return 0x0f;  // tab
	case 0x0d: return 0x1c;  // return
	case 0x10: return 0x2a;  // shift
	case 0x11: return 0x1d;  // control
	case 0x12: return 0x38;  // alt
	case 0x13: return 0xc5;  // pause
	case 0x1b: return 0x01;  // escape
	case 0x20: return 0x39;  // space
	case 0x21: return 0xc9;  // page up
	case 0x22: return 0xd1;  // page down
	case 0x23: return 0xcf;  // end
	case 0x24: return 0xc7;  // home
	case 0x25: return 0xcb;  // left
	case 0x26: return 0xc8;  // up
	case 0x27: return 0xcd;  // right
	case 0x28: return 0xd0;  // down
	case 0x2d: return 0xd2;  // insert
	case 0x2e: return 0xd3;  // delete
	case 0x7a: return 0x57;  // F11
	case 0x7b: return 0x58;  // F12
	case 0xbd: return 0x0c;  // minus
	case 0xbb: return 0x0d;  // equals
	case 0xbc: return 0x33;  // comma
	case 0xbe: return 0x34;  // period
	case 0xbf: return 0x35;  // slash
	case 0xba: return 0x27;  // semicolon
	case 0xde: return 0x28;  // apostrophe
	case 0xdb: return 0x1a;  // [
	case 0xdd: return 0x1b;  // ]
	case 0xdc: return 0x2b;  // backslash
	case 0xc0: return 0x29;  // grave
	}
	return 0;
}

// --- app and screen ---

// 0x42d1b0
void MessageBox_(const char* theText, const char* theCaption)
{
	gMessageDepth++;
	if (gMessageDepth < 2)
	{
		if (gMessageFunc != nullptr)
			gMessageFunc(theText, theCaption);
		else
			fprintf(stderr, "%s: %s\n", theCaption, theText);
	}
	gMessageDepth--;
}

// 0x42d070: the original dispatched window messages and polled the input devices here. The host delivers the
// input as it comes; this runs the key repeat and the screen mode switch the keyboard asked for.
void Pump()
{
	if (!gInCallback)
	{
		RepeatInput(&gKeys);
		RepeatInput(&gMouse);
		if (gScreenInited && gSwitchRequested)
		{
			if (gOnSwitch != nullptr)
			{
				gInCallback = true;
				gOnSwitch(0);
				gInCallback = false;
				gSwitchRequested = false;
				return;
			}
			ScreenScope(0);
			gSwitchRequested = false;
		}
	}
}

// 0x42d0f0: the counter runs in milliseconds of the app's clock
int64_t PerfFrequency()
{
	return 1000;
}

// 0x42d150
int64_t PerfCounter()
{
	static uint32_t sLast = 0;
	static int64_t sHigh = 0;
	uint32_t aNow = TickCount();
	if (aNow < sLast)
		sHigh += (int64_t)1 << 32;
	sLast = aNow;
	return sHigh + aNow;
}

// 0x42d260: a spin lock between the game and its music thread; the port runs both on one thread.
void Lock(volatile char* theLock)
{
	*theLock = 1;
}

void Unlock(volatile char* theLock)
{
	*theLock = 0;
}

void InitLock(volatile char* theLock)
{
	*theLock = 0;
}

// 0x42f290: the window, DirectDraw and the screen mode. theMode 1 is 640x480 in 32 bits (the only mode the
// game uses); the callbacks: theActivate (0x41b2da) and theClose (0x41b2b8) came from the window procedure,
// theOnSwitch (0x41b36c) from the keyboard's switch request, theRestore (0x41b34b) after a display format change
// (never in the port: the format is fixed). Returns 1 in full screen, 2 windowed.
char InitScreen(uint8_t theMode, ActivateCallback theActivate, CloseCallback theClose, SwitchCallback theOnSwitch,
	RestoreCallback theRestore, const char* theTitle, char theWindowed, uint8_t theUnknown)
{
	(void)theMode; (void)theRestore; (void)theTitle; (void)theUnknown;
	gActivate = theActivate;
	gClose = theClose;
	gOnSwitch = theOnSwitch;
	gModeRequest = theWindowed == 1 ? 1 : 2;
	gWindowed = theWindowed != 1;
	std::fill(gScreen.begin(), gScreen.end(), 0);
	gScreenInited = true;
	return gWindowed ? 2 : 1;
}

// 0x42f750
void UninitScreen()
{
	if (gScreenInited)
	{
		gScreenInited = false;
		while (gSurfaces != nullptr)
			UnregisterSurface(gSurfaces);
	}
}

// 0x42f850: the display mode's format, size and bytes per pixel
uint8_t GetDisplayFormat(PixelFormat* theFormat, uint16_t* theWidth, uint16_t* theHeight)
{
	if (theWidth != nullptr)
		*theWidth = SCREEN_W;
	if (theHeight != nullptr)
		*theHeight = SCREEN_H;
	if (theFormat != nullptr)
		*theFormat = SCREEN_FORMAT;
	return 4;
}

// 0x42f8c0: the format and size the game asked for
uint8_t GetLogicalFormat(PixelFormat* theFormat, uint16_t* theWidth, uint16_t* theHeight)
{
	return GetDisplayFormat(theFormat, theWidth, theHeight);
}

// 0x42fda0: 0 toggles full screen, 1 full screen, 2 windowed, 3 re-creates the current mode. Returns the mode
// it is in (1 or 2), 0 if the screen is not up.
char ScreenScope(char theMode)
{
	if (!gScreenInited)
		return 0;
	char aCurrent = gWindowed ? 2 : 1;
	char aWanted = theMode == 0 ? (aCurrent == 1 ? 2 : 1) : theMode;
	if (aWanted == 3)
		aWanted = aCurrent;
	if (aWanted != aCurrent)
	{
		gModeRequest = aWanted;
		gWindowed = aWanted == 2;
		FlushInput();
	}
	return aWanted;
}

// 0x42ff80: counted show/hide of the cursor around the game's own dialogs
void ShowCursorLock(char isShown)
{
	if (gScreenInited)
	{
		if (isShown == 0)
		{
			if (gCursorLock != 0)
				gCursorLock--;
		}
		else
			gCursorLock++;
	}
}

// 0x42f930
uint8_t* LockScreen(uint32_t* thePitch)
{
	if (thePitch != nullptr)
		*thePitch = SCREEN_W * 4;
	return (uint8_t*)gScreen.data();
}

// 0x42f9c0
void UnlockScreen()
{
}

// 0x42f9f0: the original flipped (or blitted to the window), waited for the vertical blank and polled input.
// The host presents gScreen after the frame; this polls. *theScreenMode is 1 in full screen, 2 windowed.
char EndFrame(char* theScreenMode)
{
	Pump();
	*theScreenMode = gWindowed ? 2 : 1;
	return 0;
}

// --- surfaces and blits ---

// 0x42ffd0
Surface* RegisterSurface(uint8_t* theBits, uint32_t thePitch, uint32_t theHeight, uint8_t theFlags)
{
	Surface* aSurface = new Surface();
	aSurface->mBits = theBits;
	aSurface->mPitch = thePitch;
	aSurface->mHeight = theHeight;
	aSurface->mFlags = theFlags;
	aSurface->mLost = 0;
	Surface** aLink = &gSurfaces;
	while (*aLink != nullptr)
		aLink = &(*aLink)->mNext;
	*aLink = aSurface;
	aSurface->mNext = nullptr;
	return aSurface;
}

// 0x430060
void UnregisterSurface(Surface* theSurface)
{
	Surface** aLink = &gSurfaces;
	while (*aLink != theSurface)
		aLink = &(*aLink)->mNext;
	*aLink = theSurface->mNext;
	delete theSurface;
}

// 0x4300c0
uint32_t SurfaceWidth(const Surface* theSurface)
{
	return theSurface->mPitch / 4;
}

// 0x4300f0
uint16_t SurfaceHeight(const Surface* theSurface)
{
	return (uint16_t)theSurface->mHeight;
}

// 0x430100: a pixel's value, in the logical format
uint32_t GetPixel(const Surface* theSurface, int x, int y)
{
	const uint8_t* p = theSurface->mBits + theSurface->mPitch * y + x * 4;
	return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
}

// 0x430170
uint8_t* SurfaceBits(const Surface* theSurface)
{
	return theSurface->mBits;
}

// 0x430180
uint8_t SurfaceFlags(const Surface* theSurface)
{
	return theSurface->mFlags;
}

static uint32_t ReadPixel(const uint8_t* p, uint8_t theBpp)
{
	switch (theBpp)
	{
	case 1: return p[0];
	case 2: return p[0] | p[1] << 8;
	case 3: return p[0] | p[1] << 8 | p[2] << 16;
	default: return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
	}
}

static void WritePixel(uint8_t* p, uint8_t theBpp, uint32_t theValue)
{
	p[0] = (uint8_t)theValue;
	if (theBpp > 1)
		p[1] = (uint8_t)(theValue >> 8);
	if (theBpp > 2)
		p[2] = (uint8_t)(theValue >> 16);
	if (theBpp > 3)
		p[3] = (uint8_t)(theValue >> 24);
}

// 0x43ba50: a channel value scaled by a difference of bit depths
static uint32_t ScaleChannel(uint32_t theValue, int theBitsDiff)
{
	if (theBitsDiff > 0)
		return theValue >> theBitsDiff;
	if (theBitsDiff < 0)
		return theValue << -theBitsDiff;
	return theValue;
}

// 0x430770
uint32_t ConvertColor(const PixelFormat* theFrom, const PixelFormat* theTo, uint32_t theColor)
{
	uint32_t c0 = ScaleChannel(theColor >> theFrom->mShift0 & ((1u << theFrom->mBits0) - 1),
		theFrom->mBits0 - theTo->mBits0);
	uint32_t c1 = ScaleChannel(theColor >> theFrom->mShift1 & ((1u << theFrom->mBits1) - 1),
		theFrom->mBits1 - theTo->mBits1);
	uint32_t c2 = ScaleChannel(theColor >> theFrom->mShift2 & ((1u << theFrom->mBits2) - 1),
		theFrom->mBits2 - theTo->mBits2);
	return c2 << theTo->mShift2 | c0 << theTo->mShift0 | c1 << theTo->mShift1;
}

static bool SameFormat(const PixelFormat* a, const PixelFormat* b)
{
	return memcmp(a, b, sizeof(PixelFormat)) == 0;
}

// The software blitter: 0x437e3c and its 44 specialised routines, the pixel loops of every format, mode and level, at
// 0x4346f0 to 0x437b73 (each is a counterpart, as this one function does them all):
// 0x4346f0, 0x434837, 0x4348c3, 0x434b06, 0x434c57, 0x434e90, 0x4350ec, 0x43532f, 0x4355ca, 0x4358a8, 0x435a75,
// 0x435c4c, 0x435e3b, 0x436012, 0x4360e5, 0x436380, 0x43665e, 0x43677a, 0x43689b, 0x4369d3, 0x436af0, 0x436b76,
// 0x436c4c, 0x436d59, 0x436e0c, 0x436ec3, 0x436f97, 0x437054, 0x4370a5, 0x437129, 0x4371d5, 0x43729d, 0x437366,
// 0x43744b, 0x43751e, 0x437573, 0x437628, 0x43771c, 0x4377e4, 0x4378ad, 0x437992, 0x437a65, 0x437abc, 0x437b73
// They copy, skipping the color key (SURF_COLORKEY) and blending with a level (SURF_TRANSLUCENT, or each pixel's own
// with SURF_ALPHABYTE): each channel becomes s + (d - s) * (255 - level) / 255, truncated toward zero (the table built
// by 0x438067).
void BlitPixels(const PixelFormat* theSrcFormat, const PixelFormat* theDestFormat, uint8_t* theDest,
	const uint8_t* theSrc, int theHeight, int theWidth, uint32_t theDestPitch, uint32_t theSrcPitch,
	uint8_t theFlags, uint8_t theSrcBpp, uint8_t theDestBpp, uint32_t theColorKey, uint8_t theLevel)
{
	if (theWidth == 0 || theHeight == 0)
		return;
	bool aBlend = (theFlags & SURF_TRANSLUCENT) != 0 && theLevel != 0xff;
	if (aBlend && theLevel == 0)
		return;
	bool aKey = (theFlags & SURF_COLORKEY) != 0;
	bool anAlphaByte = (theFlags & SURF_ALPHABYTE) != 0;
	bool aConvert = !SameFormat(theSrcFormat, theDestFormat);
	for (int y = 0; y < theHeight; y++)
	{
		const uint8_t* s = theSrc + y * theSrcPitch;
		uint8_t* d = theDest + y * theDestPitch;
		for (int x = 0; x < theWidth; x++, s += theSrcBpp, d += theDestBpp)
		{
			uint8_t aLevel = theLevel;
			if (anAlphaByte)
			{
				// 0x436e0c, 0x436ec3 (32 bits): the pixel's own level in the byte before it, times the level when
				// translucent (the table at 0x47ebc4 holds a * level / 255); 0 draws nothing
				aLevel = *s;
				s = s + 1;
				if (aBlend)
					aLevel = (uint8_t)((uint32_t)aLevel * theLevel / 0xff);
				if (aLevel == 0)
					continue;
			}
			uint32_t v = ReadPixel(s, theSrcBpp);
			if (aKey && v == theColorKey)
				continue;
			if (aConvert)
				v = ConvertColor(theSrcFormat, theDestFormat, v);
			if ((theFlags & SURF_ADDITIVE) != 0)
			{
				// 0x4370a5, 0x437129 (32 bits): black adds nothing; each channel adds, scaled by the level when
				// translucent (the table at 0x47ebc0 holds c * level / 255), and saturates
				if (v == 0)
					continue;
				uint32_t aDest = ReadPixel(d, theDestBpp);
				uint32_t aResult = aDest;
				for (int aShift = 0; aShift < 24; aShift += 8)
				{
					int sc = (int)(v >> aShift & 0xff);
					if (aBlend)
						sc = sc * theLevel / 0xff;
					int c = (int)(aDest >> aShift & 0xff) + sc;
					if (c > 0xff)
						c = 0xff;
					aResult = (aResult & ~(0xffu << aShift)) | (uint32_t)c << aShift;
				}
				WritePixel(d, theDestBpp, aResult);
				continue;
			}
			if (aBlend || anAlphaByte)
			{
				uint32_t aDest = ReadPixel(d, theDestBpp);
				uint32_t aResult = v;
				for (int aShift = 0; aShift < 24; aShift += 8)
				{
					int sc = (int)(v >> aShift & 0xff);
					int dc = (int)(aDest >> aShift & 0xff);
					int c = sc + (int)((int64_t)(dc - sc) * (0xff - aLevel) / 0xff);
					aResult = (aResult & ~(0xffu << aShift)) | (uint32_t)c << aShift;
				}
				v = aResult;
			}
			WritePixel(d, theDestBpp, v);
		}
	}
}

// 0x430700 (ConvertBitmap) -> 0x437c69: a whole image into another format
void ConvertBitmap(const PixelFormat* theFrom, const PixelFormat* theTo, uint8_t* theDest, const uint8_t* theSrc,
	int theHeight, int theWidth, uint32_t theDestPitch, uint32_t theSrcPitch, uint8_t theFromBpp,
	uint8_t theToBpp)
{
	BlitPixels(theFrom, theTo, theDest, theSrc, theHeight, theWidth, theDestPitch, theSrcPitch, 0, theFromBpp,
		theToBpp, 0, 0xff);
}

// 0x430190 LOWOS_Blit: a rectangle of pixels to the back buffer at (theDestX, theDestY), clipped to the screen
void Blit(int theWidth, int theHeight, int theDestX, int theDestY, int theSrcX, int theSrcY, uint32_t theSrcPitch,
	const uint8_t* theSrcBits, uint8_t theFlags, uint8_t theLevel)
{
	if (theWidth == 0 || theHeight == 0)
		return;
	if (theDestX < 0)
	{
		theWidth += theDestX;
		if (theWidth < 1)
			return;
		theSrcX -= theDestX;
		theDestX = 0;
	}
	if (theDestY < 0)
	{
		theHeight += theDestY;
		if (theHeight < 1)
			return;
		theSrcY -= theDestY;
		theDestY = 0;
	}
	int anOver = theWidth - SCREEN_W + theDestX;
	if (anOver >= 1)
	{
		theWidth -= anOver;
		if (theWidth <= 0)
			return;
	}
	anOver = theHeight - SCREEN_H + theDestY;
	if (anOver >= 1)
	{
		theHeight -= anOver;
		if (theHeight <= 0)
			return;
	}
	uint8_t aFlags = theFlags;
	if ((theFlags & SURF_TRANSLUCENT) != 0)
	{
		if (theLevel == 0)
			return;
		if (theLevel == 0xff)
			aFlags = theFlags & 0xfb;
	}
	uint32_t aKey = ConvertColor(&SCREEN_FORMAT, &SCREEN_FORMAT, gColorKey);
	uint8_t* aDest = (uint8_t*)gScreen.data() + theDestY * SCREEN_W * 4 + theDestX * 4;
	const uint8_t* aSrc = theSrcBits + theSrcY * theSrcPitch + theSrcX * (4 + ((aFlags >> 1) & 1));
	BlitPixels(&SCREEN_FORMAT, &SCREEN_FORMAT, aDest, aSrc, theHeight, theWidth, SCREEN_W * 4, theSrcPitch,
		aFlags, 4, 4, aKey, theLevel);
}

// 0x4306f0
void SetColorKey(uint32_t theColor)
{
	gColorKey = theColor;
}

// 0x430800
void FillRectangle(int theWidth, int theHeight, int theX, int theY, uint32_t theColor)
{
	uint32_t aColor = ConvertColor(&SCREEN_FORMAT, &SCREEN_FORMAT, theColor);
	for (int y = theY; y < theY + theHeight; y++)
	{
		if (y < 0 || y >= SCREEN_H)
			continue;
		for (int x = theX; x < theX + theWidth; x++)
			if (x >= 0 && x < SCREEN_W)
				gScreen[y * SCREEN_W + x] = aColor;
	}
}

// 0x4308e0 LOWOS_BlitRegistered: the original blitted from the surface's DirectDraw copy when it had one;
// the result is the same as LOWOS_Blit's.
void BlitRegistered(int theWidth, int theHeight, int theDestX, int theDestY, int theSrcX, int theSrcY,
	const Surface* theSurface)
{
	if (theWidth == 0 || theHeight == 0)
		return;
	Blit(theWidth, theHeight, theDestX, theDestY, theSrcX, theSrcY, theSurface->mPitch, theSurface->mBits,
		theSurface->mFlags & 0x1f, 0);
}

// 0x430b50
void BlitLevel(int theWidth, int theHeight, int theDestX, int theDestY, int theSrcX, int theSrcY,
	const Surface* theSurface, uint8_t theLevel)
{
	if (theLevel == 0xff)
	{
		BlitRegistered(theWidth, theHeight, theDestX, theDestY, theSrcX, theSrcY, theSurface);
		return;
	}
	Blit(theWidth, theHeight, theDestX, theDestY, theSrcX, theSrcY, theSurface->mPitch, theSurface->mBits,
		theSurface->mFlags | SURF_TRANSLUCENT, theLevel);
}

// 0x430bc0 LOWOS_BlitRegisteredScaled: nearest-neighbour scaling with the original's 16.16 steps (the step is
// size << 16 / (size + 1) and is added before each sample). Not clipped: the game keeps it on the screen.
uint32_t BlitScaled(int theDestWidth, int theDestHeight, int theSrcWidth, int theSrcHeight, int theDestX,
	int theDestY, int theSrcX, int theSrcY, const Surface* theSurface)
{
	if (theDestWidth == 0 || theDestHeight == 0)
		return 0;
	if (theDestWidth == theSrcWidth && theDestHeight == theSrcHeight)
	{
		BlitRegistered(theDestWidth, theDestHeight, theDestX, theDestY, theSrcX, theSrcY, theSurface);
		return 0;
	}
	if ((theSurface->mFlags & SURF_SCREENFORMAT) == 0)
		return 1;
	uint32_t aKey = gColorKey;
	uint32_t aPitch = 0;
	uint8_t* aScreen = LockScreen(&aPitch);
	uint32_t aStepX = ((uint32_t)theSrcWidth << 16) / (uint32_t)(theDestWidth + 1);
	uint32_t aStepY = ((uint32_t)theSrcHeight << 16) / (uint32_t)(theDestHeight + 1);
	uint32_t aSrcY = (uint32_t)theSrcY << 16;
	int anEndY = theDestHeight + theDestY;
	for (int y = theDestY; y < anEndY; y++)
	{
		aSrcY += aStepY;
		uint32_t* aDest = (uint32_t*)(aScreen + y * aPitch) + theDestX;
		const uint8_t* aRow = theSurface->mBits + (aSrcY >> 16) * theSurface->mPitch;
		uint32_t aSrcX = (uint32_t)theSrcX << 16;
		for (int x = 0; x < theDestWidth; x++, aDest++)
		{
			aSrcX += aStepX;
			uint32_t v = ReadPixel(aRow + (aSrcX >> 16) * 4, 4);
			if ((theSurface->mFlags & SURF_COLORKEY) == 0 || v != aKey)
			{
				if (y >= 0 && y < SCREEN_H && theDestX + x >= 0 && theDestX + x < SCREEN_W)
					*aDest = v;
			}
		}
	}
	UnlockScreen();
	return 0;
}

// --- input ---

// 0x4310d0
void SetKeyRepeat(uint32_t theDelay, uint32_t theRate)
{
	gRepeatDelay = theDelay;
	gRepeatRate = theRate;
}

// 0x431c40
char InitKeyboard()
{
	UninitKeyboard();
	gKeyboardInited = true;
	for (int i = 0; i < 8; i++)
		gKeys.mRepeatMask[i] = 0xffffffff;
	return 0;
}

// 0x431c80
void UninitKeyboard()
{
	gKeyboardInited = false;
	gKeys = InputQueue();
	gChars = InputQueue();
}

// 0x431ca0: Enter and Tab count as up while Alt is down (Alt+Enter and Alt+Tab belong to the system)
uint32_t KeyIsDown(uint8_t theScanCode)
{
	if (!gKeyboardInited)
		return 0;
	if (theScanCode == 0x1c || theScanCode == 0xf)
	{
		if ((gKeys.mDown[1] & 0x1000000) != 0)
			return 0x1000000;
		if ((gKeys.mDown[5] & 0x1000000) != 0)
			return 0x1000000;
	}
	return gKeys.mDown[theScanCode >> 5] >> (theScanCode & 0x1f) & 1;
}

static uint16_t PopEvent(InputQueue* theQueue)
{
	uint16_t anEvent = theQueue->mEvents[theQueue->mHead];
	theQueue->mHead = (theQueue->mHead + 1) & 0xf;
	theQueue->mCount--;
	return anEvent;
}

// 0x431d00
uint8_t NextKey()
{
	if (!gKeyboardInited || gKeys.mCount == 0)
		return 0;
	return (uint8_t)PopEvent(&gKeys);
}

// 0x431d50
uint8_t NextChar()
{
	if (!gKeyboardInited || gChars.mCount == 0)
		return 0;
	return (uint8_t)PopEvent(&gChars);
}

// 0x431da0
void FlushInput()
{
	if (gKeyboardInited)
	{
		gKeys.mCount = 0;
		gChars.mCount = 0;
		for (int i = 0; i < 8; i++)
			gKeys.mDown[i] = 0;
	}
}

// 0x431e00
void SetKeyRepeatMask(const uint32_t* theMask)
{
	for (int i = 0; i < 8; i++)
		gKeys.mRepeatMask[i] = theMask[i];
}

// 0x431e20: useDirectInput 0 takes the buttons from window messages (the game's choice)
char InitMouse(uint8_t useDirectInput)
{
	(void)useDirectInput;
	UninitMouse();
	gMouseInited = true;
	return 0;
}

// 0x431e50
void UninitMouse()
{
	if (gMouseInited)
	{
		gMouseInited = false;
		gMouse = InputQueue();
	}
}

// 0x431ee0: the next button event as 1 << button, with bit 31 set for a press
uint32_t NextMouseEvent()
{
	if (!gMouseInited || gMouse.mCount == 0)
		return 0;
	uint16_t anEvent = PopEvent(&gMouse);
	return (1u << (anEvent & 0x1f)) | (uint32_t)(anEvent >> 8) << 31;
}

// 0x431f80: the cursor in screen pixels, clamped to the screen
void GetMousePos(uint16_t* x, uint16_t* y)
{
	int ax = gMouseX < 1 ? 0 : (gMouseX < SCREEN_W ? gMouseX : SCREEN_W);
	int ay = gMouseY < 1 ? 0 : (gMouseY < SCREEN_H ? gMouseY : SCREEN_H);
	if (x != nullptr)
		*x = (uint16_t)ax;
	if (y != nullptr)
		*y = (uint16_t)ay;
}

// --- sound ---

struct SoundSlot
{
	std::shared_ptr<Platform::SoundData> mData;
	int mVoice = 0;
	// BYOPOP_SOUNDLOG: the number of the play the log has running (0 = none) and the update it ends on by the sound's
	// length, so that a run on the virtual clock (-fast) logs the plays the real clock would
	int mLogNum = 0;
	int mLogEnd = 0;
	int mLogVolume = 0;
};

static bool gSoundInited = false;     // 0x48a294
static Sound* gSounds = nullptr;      // 0x48a290
static std::vector<SoundSlot> gSlots;

// 0x432e50
char InitSound()
{
	gSoundInited = true;
	return 0;
}

// 0x432e70
void UninitSound()
{
	gSoundInited = false;
	while (gSounds != nullptr)
		FreeSound(gSounds);
	gSlots.clear();
}

static FILE* gSoundLog = nullptr;
static const int* gSoundLogUpdate = nullptr;

void SetSoundLog(FILE* theLog, const int* theUpdate)
{
	gSoundLog = theLog;
	gSoundLogUpdate = theUpdate;
}

// 0x432ea0: a sound from 16-bit PCM (theWav points at the samples of the WAV image the resource converter made,
// 0x2c bytes after its header); theFlags bit 1: stereo.
Sound* CreateSound(const void* theWav, uint32_t theSize, uint32_t theParam, uint8_t theFlags)
{
	if (!gSoundInited)
		return nullptr;
	Sound* aSound = new Sound();
	aSound->mData = theWav;
	aSound->mSize = theSize;
	aSound->mParam = theParam;
	aSound->mFlags = theFlags;
	aSound->mLost = 0;
	aSound->mNext = nullptr;
	const uint8_t* aHeader = (const uint8_t*)theWav - 0x2c;
	std::vector<uint8_t> aFile(aHeader, aHeader + 0x2c + theSize);
	SoundSlot aSlot;
	aSlot.mData = Platform::Mixer::Get().LoadSoundData(aFile, "tiptop sound");
	aSound->mHandle = (int)gSlots.size();
	gSlots.push_back(aSlot);
	if (gSoundLog != nullptr)
		fprintf(gSoundLog, "%d load %d bytes=%u flags=%u\n", *gSoundLogUpdate, aSound->mHandle, theSize, theFlags);
	Sound** aLink = &gSounds;
	while (*aLink != nullptr)
		aLink = &(*aLink)->mNext;
	*aLink = aSound;
	return aSound;
}

// 0x432f30
void FreeSound(Sound* theSound)
{
	Sound** aLink = &gSounds;
	while (*aLink != theSound)
		aLink = &(*aLink)->mNext;
	*aLink = theSound->mNext;
	SoundSlot& aSlot = gSlots[theSound->mHandle];
	if (aSlot.mVoice != 0)
		Platform::Mixer::Get().StopVoice(aSlot.mVoice);
	aSlot = SoundSlot();
	delete theSound;
}

// BYOPOP_SOUNDLOG (tools/soundpreset.py): a call of PlaySound as the framework's sound log words it. Whether a sound is
// still playing is taken from its length in updates, not from the mixer, which a run on the virtual clock (-fast) keeps
// on the machine's clock: the log is the one a run at the game's own speed gives.
static void LogSoundCall(const Sound* theSound, SoundSlot& theSlot, uint32_t theVolume, char theMode)
{
	int aNow = Sexy::gSexyAppBase->mUpdateCount;
	bool isPlaying = theSlot.mLogNum != 0 && theSlot.mLogEnd > aNow;
	// the volume in hundredths of a decibel, as DirectSound had it (0 = full)
	int aCentiDb = theVolume == 0 ? -10000 : (int)(2000.0 * log10((double)theVolume / 4294967295.0));
	if (aCentiDb > 0)
		aCentiDb = 0;
	if ((theMode == 0 || theMode == 1) && theSlot.mLogNum != 0)
	{
		Sexy::SoundLog("stop %d #%d", theSound->mHandle, theSlot.mLogNum);
		theSlot.mLogNum = 0;
	}
	if (theMode == 0)
		return;
	if (theMode == 2 && isPlaying)
	{
		if (aCentiDb != theSlot.mLogVolume)
			Sexy::SoundLog("volume %d #%d %d", theSound->mHandle, theSlot.mLogNum, aCentiDb);
		theSlot.mLogVolume = aCentiDb;
		return;
	}
	if (theVolume == 0 || !theSlot.mData)
		return;
	static int sLogNum = 0;
	theSlot.mLogNum = ++sLogNum;
	theSlot.mLogVolume = aCentiDb;
	theSlot.mLogEnd = aNow + (int)std::ceil((double)theSlot.mData->mFrames * 1000.0 / theSlot.mData->mRate /
		Sexy::gSexyAppBase->mFrameTime);
	Sexy::SoundLog("play %d #%d vol=%d pan=0 pitch=1.0000 loop=0", theSound->mHandle, theSlot.mLogNum, aCentiDb);
}

// 0x432f80: theMode 0 stops the sound, 1 (re)starts it, 2 starts it unless it is playing. theVolume is a linear
// amplitude, 0xffffffff = full (the original made it DirectSound decibels: 2000 * log10(volume / 2^32)).
void PlaySound(Sound* theSound, uint32_t theVolume, char theMode)
{
	if (theSound == nullptr || theSound->mLost)
		return;
	SoundSlot& aSlot = gSlots[theSound->mHandle];
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	bool isPlaying = aSlot.mVoice != 0 && aMixer.IsVoicePlaying(aSlot.mVoice);
	if (isPlaying && (theMode == 1 || theMode == 0))
	{
		aMixer.StopVoice(aSlot.mVoice);
		aSlot.mVoice = 0;
	}
	// a test run's mixer may not play (-fast, no device), so the log has every stop the call asks for, and the
	// comparison decides from the sound's length whether it still played
	if (gSoundLog != nullptr && (theMode == 0 || theMode == 1))
		fprintf(gSoundLog, "%d stop %d\n", *gSoundLogUpdate, theSound->mHandle);
	if (!gActive)
		return;
	if (Sexy::SoundLogOn())
		LogSoundCall(theSound, aSlot, theVolume, theMode);
	float aGain = (float)((double)(theVolume >> 14) * (1.0 / 262144.0));
	if (theMode != 0 && aSlot.mVoice != 0 && isPlaying)
		aMixer.SetVoiceGain(aSlot.mVoice, aGain, aGain);
	if (theVolume == 0)
		return;
	if (theMode != 1 && (theMode != 2 || isPlaying))
		return;
	if (aSlot.mData)
		aSlot.mVoice = aMixer.StartVoice(aSlot.mData, aGain, aGain, 1.0, false);
	if (gSoundLog != nullptr)
		fprintf(gSoundLog, "%d play%s %d\n", *gSoundLogUpdate, theMode == 2 ? "2" : "", theSound->mHandle);
}

} // namespace lowos
