#ifndef __BONNIES_GRAPHICS_H__
#define __BONNIES_GRAPHICS_H__

// The graphics (0x421e50-0x428730, the object 0x4818a8): the 800x600 screen surface the engine draws into directly
// (the double buffer 0x4818a0 is never on), images with their frames, and the drawing primitives. Coordinates are
// fractions of the screen; a primitive turns them into pixels with __ftol. The x87 ran at double precision (Visual
// C++'s default control word), so the port computes in double and rounds to float where the original stored a float.

#include "Str.h"

#include <vector>

namespace bonnies
{

struct Image;

// A frame of an image (0x14 bytes): its size, its image and its top left corner in the image's pixels
struct Frame
{
	int mWidth;      // +0x00
	int mHeight;     // +0x04
	Image* mOwner;   // +0x08
	int mX;          // +0x0c
	int mY;          // +0x10
};

// An image (0x20 bytes): 32-bit pixels 0xAARRGGBB and its frames (from <name>_ims.txt, or one of the whole image)
struct Image
{
	int mWidth;          // +0x00
	int mHeight;         // +0x04
	int mBits;           // +0x08 (32)
	int mPitch;          // +0x0c, in bytes
	int mFrameCount;     // +0x10
	Frame* mFrames;      // +0x14
	uint32_t* mPixels;   // +0x18
	std::vector<uint32_t> mSurface;  // +0x1c: the SDL surface that holds the pixels
};

class Graphics
{
public:
	// 0x424320 (GM Init): SDL's video and the screen mode
	Graphics(int theWidth, int theHeight, int theFullScreen);

	// 0x424000: sets the screen mode (the window when not full screen) and clears the screen to white
	void setVideoMode(int theWidth, int theHeight, int theFullScreen, int theForce);
	// 0x423c90: shows theCount rectangles of the screen (all of it when theCount is 0) and waits for the next frame
	void present(int theCount, const float* theRects);
	// 0x4227d0: SDL_Quit
	void shutdown();
	// 0x422910: the cursor drawn by theArt, 32 rows of '.' (white), 'B' (black), 'H' (black, the hot spot), ended by a
	// zero or a row starting with '0'
	void setCursor(const char* const* theArt);

	int mField00;          // +0x00
	uint32_t mModeTime;    // +0x04: when the mode was set
	int mModeFrame;        // +0x08: the frame then
	int mFocusFrame;       // +0x0c: the last frame shown with the input focus
};

extern Graphics* gGraphics;  // 0x4818a8

// The screen (0x480b78-0x4818c4)
extern uint32_t* gPixels;    // 0x4818b4
extern int gPitch;           // 0x4818b8, in bytes
extern int gPitch4;          // 0x4818bc, in pixels
extern int gWidth;           // 0x480b80
extern int gHeight;          // 0x480f98
extern float gInvWidth;      // 0x480b78
extern float gInvHeight;     // 0x480f94
extern Str gMatteName;       // 0x4818ec: a matte to use instead of <name>_Matte.png (the menu's books)
extern int gClipX0;          // 0x480f9c
extern int gClipY0;          // 0x480b7c
extern int gClipX1;          // 0x480f88
extern int gClipY1;          // 0x480f8c
extern int gFullScreen;      // 0x480f90
extern int gFrame;           // 0x4818c0: frames shown
extern uint32_t gFrameTimes[64];  // 0x480fa0
extern uint32_t gLastPresent;     // 0x4818c4
extern int gRedShift[256];   // 0x480b88: i << 16
extern int gGreenShift[256]; // 0x4814a0: i << 8
extern int gBlueShift[256];  // 0x4810a0: i

// The window's caption and icon, set by the game before the graphics start (0x4818d4, 0x4818cc)
extern Str gCaption;
extern Str gIconPath;

// The cursors (0x473ee8, 0x473f68: the arrow and the hand), read from the game's exe
extern const char* const* gArrowCursor;
extern const char* const* gHandCursor;

// 0x421e50: frames per second over the last 64 frames, to a tenth
double framesPerSecond();
// 0x421ff0
int frameCount();
// 0x422000: decodes thePath (from the pak by its base name, else from the disk)
bool loadSurface(const Str& thePath, const Str& theType, struct HostSurface& theSurface);
// 0x422de0: Graphics/<name>.png or .jpg, its _Matte.png and its _ims.txt
Image* loadImageSet(const Str& theName);
// 0x4221f0
void freeImage(Image* theImage);
// 0x422ca0: whether Graphics/<name>.png or .jpg exists
int imageExists(const Str& theName);
// 0x422800: the window's caption; 0x422860: its icon (magenta transparent)
void setCaption(const Str& theCaption);
void setIcon(const Str& thePath);
// 0x422900: whether the window has the input focus
bool hasInputFocus();
// 0x422c40, 0x422c50, 0x422c60
void iconify();
void showCursor();
void hideCursor();
// 0x422c70
void buildShiftTables();
// 0x427a60, 0x427a70, 0x427a80, 0x427a90, 0x427aa0
int screenWidth();
int screenHeight();
int isFullScreen();
double pixelWidth();
double pixelHeight();

// 0x424c00: fills a rectangle with theColor (blended by its alpha, which is then lost)
void fillRect(float theX, float theY, float theW, float theH, uint32_t theColor);
// 0x421f00: a rectangle's outline theThickness pixels wide
void drawRect(float theX, float theY, float theW, float theH, uint32_t theColor, int theThickness);
// 0x422220: a bevelled panel: theColor inside, edges shaded from theEdgeColor, raised or sunk, theSize pixels deep with
// corners cut by theCorners (0-4)
void drawBevel(float theX, float theY, float theW, float theH, uint32_t theColor, uint32_t theEdgeColor, int theRaised,
	float theSize, float theCorners);
// 0x424520: a rectangle with corners of theRadius pixels
void fillRoundRect(float theX, float theY, float theW, float theH, uint32_t theColor, int theRadius);
// 0x4247b0: a triangle
void fillTriangle(float theX1, float theY1, float theX2, float theY2, float theX3, float theY3, uint32_t theColor);

} // namespace bonnies

#endif
