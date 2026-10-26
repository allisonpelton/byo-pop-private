#ifndef __TIPTOP_LOWOS_H__
#define __TIPTOP_LOWOS_H__

// LOWOS, TipTop's own low-level layer (TipTop.exe 0x42d000-0x438000; the name is the original's, from its window
// class "LOWOS Screen" and its debug strings), on the port's platform. The original drew into DirectDraw surfaces,
// read DirectInput and window messages, played DirectSound buffers and timed frames with the performance counter.
// Here the screen is a 640x480 32-bit buffer that the host (TipTopApp) presents through the framework, input comes
// from the host's widget, sounds go to Platform::Mixer and the clock is the app's clock (virtual with -fast).
//
// Pixel formats are the original's: three channels as (bits, shift) pairs. The screen mode the game asks for
// (mode 1: 640x480, 32 bits, 8:16 8:8 8:0) is the format of every converted image and of the colors the game passes
// (color key 0xfefe01, fill colors), so the port has one format throughout.

#include <cstddef>
#include <cstdint>

namespace lowos
{

struct PixelFormat
{
	uint8_t mBits0, mShift0, mBits1, mShift1, mBits2, mShift2;
};

// A decoded image: the original's 0x20-byte header in front of the pixels (made by the PCX/JPEG converters,
// 0x438ab0, 0x438d40, 0x4391a0).
struct Image
{
	PixelFormat mFormat;     // +0x00
	uint32_t mPitch;         // +0x08
	uint32_t mHeight;        // +0x0c
	uint8_t mBytesPerPixel;  // +0x10 (1 = 8-bit with mPalette)
	uint32_t mWidth;         // +0x14
	uint8_t* mBits;          // +0x18
	uint8_t* mPalette;       // +0x1c (768 bytes RGB, 8-bit images only)
};

// Surface flags (byte +0xc of a registered surface, LOWOS_Blit's param_9)
enum
{
	SURF_SCREENFORMAT = 0x01,  // pixels are in the display format (every surface the game registers)
	SURF_ALPHABYTE = 0x02,     // each pixel is preceded by a byte (unused by TipTop)
	SURF_TRANSLUCENT = 0x04,   // blend with a level 1..254 (0 = invisible, 255 = opaque)
	SURF_COLORKEY = 0x08,      // pixels of the color key (lowos::SetColorKey) are transparent
	SURF_ADDITIVE = 0x10,      // unused by TipTop
};

// A registered surface (0x18 bytes, LOWOS_RegisterSurface 0x42ffd0): the original also kept a DirectDraw copy
// at +0x10 for hardware blits; the port blits in software only.
struct Surface
{
	uint8_t* mBits;   // +0x00
	uint32_t mPitch;  // +0x04
	uint32_t mHeight; // +0x08
	uint8_t mFlags;   // +0x0c
	uint8_t mLost;    // +0x0d
	Surface* mNext;   // +0x14
};

// A sound (0x18 bytes, 0x432ea0): the original's DirectSound buffer at +0x10.
struct Sound
{
	const void* mData;  // +0x00 the WAV image the resource converter made
	uint32_t mSize;     // +0x04
	uint32_t mParam;    // +0x08
	uint8_t mFlags;     // +0x0c
	uint8_t mLost;      // +0x0d
	int mHandle;        // the port's mixer sound (index into the engine's table)
	Sound* mNext;       // +0x14
};

// --- the host (TipTopApp) ---
// The screen the frame was drawn into, 640x480 XRGB8888, row pitch 2560 bytes.
uint32_t* HostScreen();
// The host's clock in milliseconds (framework app clock: real, or virtual with -fast).
typedef uint32_t (*ClockFunc)();
void HostSetClock(ClockFunc theClock);
// Input from the host's widget (positions in screen pixels).
void HostMouseMove(int x, int y);
void HostMouseButton(int theButton, bool isDown);  // 0 = left, 1 = right
void HostChar(int theChar);                        // a typed character (WM_CHAR)
void HostKey(int theScanCode, bool isDown);        // a DirectInput scan code (DIK_*)
int VirtualKeyToScanCode(int theVirtualKey);       // Windows VK_* to DIK_*, 0 if the game has no use for it
// The screen mode the game asked for since the last call (ScreenScope, e.g. F11 or the options' checkbox):
// 1 = full screen, 2 = windowed, 0 = no change. The host applies it and reports the mode it is in.
int HostTakeScreenModeRequest();
void HostSetWindowed(bool isWindowed);
// The host's own screen mode toggle (Alt+Enter, the page's button): handled like the original's Alt+Enter, by the
// game's switch callback at the next EndFrame.
void HostRequestScreenSwitch();
// The window's close button (WM_CLOSE): the game's close callback.
void HostClose();
// A message box (0x42d1b0): the host shows it as an SDL message box, or logs it in test runs.
typedef void (*MessageFunc)(const char* theText, const char* theCaption);
void HostSetMessageFunc(MessageFunc theFunc);
// Set when the game window is in the foreground (WM_ACTIVATEAPP): the game's activate callback is told, and sounds
// play only then (0x432f80).
void HostSetActive(bool isActive);

// --- app and screen (0x42d020-0x42f750) ---
void MessageBox_(const char* theText, const char* theCaption);                  // 0x42d1b0
void Pump();                                                                     // 0x42d070
int64_t PerfFrequency();                                                         // 0x42d0f0
int64_t PerfCounter();                                                           // 0x42d150
void Lock(volatile char* theLock);                                               // 0x42d260
void Unlock(volatile char* theLock);                                             // 0x42d2a0
void InitLock(volatile char* theLock);                                           // 0x42d2b0
typedef void (*ActivateCallback)(char isActive);
typedef void (*CloseCallback)();
typedef void (*SwitchCallback)(char theMode);
typedef uint32_t (*RestoreCallback)();
char InitScreen(uint8_t theMode, ActivateCallback theActivate, CloseCallback theClose, SwitchCallback theOnSwitch,
	RestoreCallback theRestore, const char* theTitle, char theWindowed, uint8_t theUnknown);  // 0x42f290
void UninitScreen();                                                             // 0x42f750
uint8_t GetDisplayFormat(PixelFormat* theFormat, uint16_t* theWidth, uint16_t* theHeight);  // 0x42f850
uint8_t GetLogicalFormat(PixelFormat* theFormat, uint16_t* theWidth, uint16_t* theHeight);  // 0x42f8c0
char ScreenScope(char theMode);                                                  // 0x42fda0 LOWOS_ScreenScope
void ShowCursorLock(char isShown);                                               // 0x42ff80
uint8_t* LockScreen(uint32_t* thePitch);                                         // 0x42f930 LOWOS_LockScreen
void UnlockScreen();                                                             // 0x42f9c0
char EndFrame(char* theScreenMode);                                              // 0x42f9f0 LOWOS_EndFrame

// --- surfaces and blits (0x42ffd0-0x430e70) ---
Surface* RegisterSurface(uint8_t* theBits, uint32_t thePitch, uint32_t theHeight, uint8_t theFlags);  // 0x42ffd0
void UnregisterSurface(Surface* theSurface);                                     // 0x430060
uint32_t SurfaceWidth(const Surface* theSurface);                                // 0x4300c0
uint16_t SurfaceHeight(const Surface* theSurface);                               // 0x4300f0
uint32_t GetPixel(const Surface* theSurface, int x, int y);                      // 0x430100
uint8_t* SurfaceBits(const Surface* theSurface);                                 // 0x430170
uint8_t SurfaceFlags(const Surface* theSurface);                                 // 0x430180
void Blit(int theWidth, int theHeight, int theDestX, int theDestY, int theSrcX, int theSrcY, uint32_t theSrcPitch,
	const uint8_t* theSrcBits, uint8_t theFlags, uint8_t theLevel);              // 0x430190 LOWOS_Blit
void SetColorKey(uint32_t theColor);                                             // 0x4306f0
uint32_t ConvertColor(const PixelFormat* theFrom, const PixelFormat* theTo, uint32_t theColor);  // 0x430770
void FillRectangle(int theWidth, int theHeight, int theX, int theY, uint32_t theColor);  // 0x430800
void BlitRegistered(int theWidth, int theHeight, int theDestX, int theDestY, int theSrcX, int theSrcY,
	const Surface* theSurface);                                                 // 0x4308e0
void BlitLevel(int theWidth, int theHeight, int theDestX, int theDestY, int theSrcX, int theSrcY,
	const Surface* theSurface, uint8_t theLevel);                               // 0x430b50
uint32_t BlitScaled(int theDestWidth, int theDestHeight, int theSrcWidth, int theSrcHeight, int theDestX,
	int theDestY, int theSrcX, int theSrcY, const Surface* theSurface);         // 0x430bc0
// The software blitter (0x437e3c) and the format converter (0x437c69, ConvertBitmap 0x430700).
void BlitPixels(const PixelFormat* theSrcFormat, const PixelFormat* theDestFormat, uint8_t* theDest,
	const uint8_t* theSrc, int theHeight, int theWidth, uint32_t theDestPitch, uint32_t theSrcPitch,
	uint8_t theFlags, uint8_t theSrcBpp, uint8_t theDestBpp, uint32_t theColorKey, uint8_t theLevel);
void ConvertBitmap(const PixelFormat* theFrom, const PixelFormat* theTo, uint8_t* theDest, const uint8_t* theSrc,
	int theHeight, int theWidth, uint32_t theDestPitch, uint32_t theSrcPitch, uint8_t theFromBpp,
	uint8_t theToBpp);

// --- input (0x4310d0-0x4322c9) ---
void SetKeyRepeat(uint32_t theDelay, uint32_t theRate);                          // 0x4310d0
char InitKeyboard();                                                             // 0x431c40 (0 = success)
void UninitKeyboard();                                                           // 0x431c80
uint32_t KeyIsDown(uint8_t theScanCode);                                         // 0x431ca0
uint8_t NextKey();                                                               // 0x431d00 (scan codes)
uint8_t NextChar();                                                              // 0x431d50 (typed characters)
void FlushInput();                                                               // 0x431da0
void SetKeyRepeatMask(const uint32_t* theMask);                                  // 0x431e00 (8 dwords)
char InitMouse(uint8_t useDirectInput);                                          // 0x431e20 (0 = success)
void UninitMouse();                                                              // 0x431e50
uint32_t NextMouseEvent();                                                       // 0x431ee0
void GetMousePos(uint16_t* x, uint16_t* y);                                      // 0x431f80

// --- sound (0x432330-0x433240) ---
char InitSound();                                                                // 0x432e50 (0 = success)
void UninitSound();                                                              // 0x432e70
Sound* CreateSound(const void* theWav, uint32_t theSize, uint32_t theParam, uint8_t theFlags);  // 0x432ea0
void FreeSound(Sound* theSound);                                                 // 0x432f30
void PlaySound(Sound* theSound, uint32_t theVolume, char theMode);               // 0x432f80

} // namespace lowos

#endif
