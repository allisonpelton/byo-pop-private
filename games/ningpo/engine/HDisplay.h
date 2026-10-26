#ifndef __NINGPO_HDISPLAY_H__
#define __NINGPO_HDISPLAY_H__

// HDisplay (vtable 0x463e00, 0x422b10..0x424b90): a 640x480 GDI back buffer of 15-bit pixels (5-5-5, red at bit 10:
// 0x4238f0(5,5,5,10,5,0)) that every drawing call writes straight into and that the flip (0x423500) copies to the
// window, rectangle by rectangle. Images become surfaces of the same pixels, dithered down from 24 bits, plus an
// 8-bit alpha plane when they have transparency; all drawing is a blend of a surface's pixels into the back buffer
// with one of the framework's blend modes (0x423960, 0x423b80). port: the back buffer is a plain array the host
// shows after each flip.

#include "Pg.h"

#include <cstdint>
#include <vector>

namespace pg
{

class HFileSystem;
class PGSprite;

// An image as loaded (0x427ec0): format 1 has 8 bits a pixel, 2 has 32 (blue, green, red, alpha in memory), 3 has
// 16 (blue at bit 11, green at bit 6, red at bit 1, as 0x4282e0 packs a 16-bit Targa's pixels)
struct HImage
{
	int mWidth = 0;
	int mHeight = 0;
	int mFormat = 0;
	std::vector<uint8_t> mBits;

	void create(int theWidth, int theHeight, int theFormat);  // 0x427ec0
	int bytesPerPixel() const;                                // 0x427f80
	int pitch() const { return mWidth * bytesPerPixel(); }    // 0x427fc0
	void flipVertical();                                      // 0x4280b0
};

// 0x428180: the loader by the name's extension (.tga 0x4282e0, .jpg/.jpeg 0x428960, .png 0x428590)
bool LoadImage(const HFileSystem* theFileSystem, const char* theName, HImage& theImage);

// A surface (0x1c bytes, vtable 0x463dd4)
struct HSurface
{
	uint8_t* mAlpha = nullptr;     // +0x04 one byte a pixel, mWidth a row; nullptr when the image is opaque
	uint16_t* mPixels = nullptr;   // +0x08
	int mWidth = 0;                // +0x0c
	int mHeight = 0;               // +0x10
	int mBpp = 16;                 // +0x14
	int mPitch = 0;                // +0x18 in bytes

	~HSurface();
};

// A font (0x422b90): a sprite of 16 characters a row from character 0x10 on, each mWidth x mHeight
struct HFont
{
	PGSprite* mSprite;
	int mWidth;
	int mHeight;
};

// The nine numbers of a blend mode (0x423960): for the source, the factor (0 none, 1 full, 2 the pixel's alpha,
// 3 one minus it, 4..7 the constants A and B and one minus them), a second factor (0, 1, 2..5 the constants), whether
// to take one minus their product and the constant A; the same four for the destination with the constant B; and
// how to combine the two (0 add, 1 multiply, 2 source minus destination, 3 destination minus source).
// BLEND_ALPHA is plain alpha blending, the mode of nearly every call.
#define BLEND_ALPHA 2, 1, 0, 0xff, 3, 1, 0, 0xff, 0

class HDisplay
{
public:
	std::vector<uint16_t> mScreen;   // +0x14 the back buffer's pixels (the DIB section, 0x424bc0)
	int mScreenPitch = 0;            // +0x24 in bytes
	int mTextMode[9] = {};           // +0x28..+0x48 the blend mode of drawText (0x423230)
	HRectList mDirty;                // +0x4c what was drawn since the last flip
	int mFrameTimes[16] = {};        // +0x2050 the last frames' durations (0x423500)
	int mWidth = 0;                  // +0x2090
	int mHeight = 0;                 // +0x2094

	// 0x424390 initScreen: the window and the back buffer
	bool init(int theWidth, int theHeight, bool theFullscreen);
	void shutdown();  // 0x4246e0
	// 0x4235c0
	bool setSize(int theWidth, int theHeight);

	// 0x424810 (slot 0x18): a surface of an image's pixels
	HSurface* createSurface(const HImage& theImage);
	// 0x423ed0 (slot 0x14): a copy of a surface
	HSurface* copySurface(const HSurface* theSurface);
	// 0x423e60 (slot 0x1c)
	void freeSurface(HSurface* theSurface);

	// 0x422dc0 (slot 0x24): blends theSrc of the surface into the back buffer at theDest's top left, unclipped
	bool blend(const PGRect* theDest, const PGRect* theSrc, HSurface* theSurface, int s1, int s2, int s3, int s4,
		int d1, int d2, int d3, int d4, int theOp);
	// 0x422fb0 (slot 0x28): copies theSrc at theDest's top left, unclipped; with an alpha plane only the opaque
	// pixels
	bool copy(const PGRect* theDest, const PGRect* theSrc, HSurface* theSurface);
	// 0x4231f0 (slot 0x2c): the whole surface at a point, unclipped
	bool draw(const PGPoint* theAt, HSurface* theSurface);
	// 0x423620: blend() clipped to the screen
	bool blitEx(const PGRect* theDest, const PGRect* theSrc, HSurface* theSurface, int s1, int s2, int s3, int s4,
		int d1, int d2, int d3, int d4, int theOp);
	// 0x4237a0: copy() clipped to the screen
	bool blit(const PGRect* theDest, const PGRect* theSrc, HSurface* theSurface);

	// 0x422b90 (slot 0x38), 0x422bc0 (slot 0x34)
	HFont* createFont(PGSprite* theSprite, int theWidth, int theHeight);
	void freeFont(HFont* theFont);
	// 0x423280 (slot 0x3c): theAlign 0 left, 1 right, 2 centered on theX; clipped to theClip
	void drawText(const char* theText, int theX, int theY, const PGRect* theClip, int theAlign, HFont* theFont,
		const float* theColor);
	// 0x423230 (slot 0x40): the blend mode of drawText
	void setBlendMode(int s1, int s2, int s3, int s4, int d1, int d2, int d3, int d4, int theOp);
	// 0x423aa0 (slot 0x4c): flag 0 makes plain alpha blending a test at half alpha
	void setBlendFlag(int theIndex, char theValue);

	// 0x424770 (slot 0x0c): the whole back buffer in the clear colour
	void clear();
	// 0x424710 (slot 0x50): copies rectangles of the back buffer to the window
	void present(const PGRect* theRects, int theCount);
	// 0x423500: shows what was drawn since the last flip
	void flip();
};

// The display the host shows (the one created last)
HDisplay* GetScreenDisplay();

} // namespace pg

#endif
