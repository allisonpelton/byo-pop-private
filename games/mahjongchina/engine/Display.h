#ifndef __MAHJONGCHINA_DISPLAY_H__
#define __MAHJONGCHINA_DISPLAY_H__

// The display (0x431a70, vtable 0x51e674): the images cached by id (+0xbc) and the fonts by image id (+0xcc), the
// blank images it tracks (+0xe0), the draws, and the screen. See docs/ENGINE.md, "The device boundary".
//
// Port: the screen is the software display's 800x600 32-bit buffer (gScreen), which the host shows (Host::Present);
// the window's modes, DirectDraw and Direct3D are the host's or are not ported (Image.h).

#include "Image.h"

#include <map>
#include <vector>

namespace jb
{

class Font;

class Display
{
public:
	bool mActive = true;                 // the app is active (the window's)
	bool mAccelerated = false;           // +0xb8 the mode was set with acceleration (never, in the port)
	std::map<int, Image*> mImages;       // +0xbc
	std::map<int, Font*> mFonts;         // +0xcc
	std::vector<Image*> mCreated;        // +0xe0

	// 0x431a70
	Display();
	// 0x431c50
	virtual ~Display();

	// 0x432db0: the surfaces released
	void Release();
	// 0x432000: the screen made for full screen or a window; 0 or a negative error
	int SetMode(bool theFullScreen, bool theNoAccel);
	// 0x4328f0: false when the screen cannot be drawn
	bool BeginFrame();
	// 0x432a00
	void Present();
	// 0x432a30 (the windowed present's wait for the blit, nothing in the port)
	void PresentDone() {}

	// 0x432430: the image of theId (from theModule, loaded and cached the first time; the cache is by id only)
	Image* GetImage(int theId, Module theModule);
	// 0x4326e0
	Font* GetFont(int theImageId, int theTextId, Module theModule);
	// 0x433380: false when the file did not load (then nothing is cached)
	bool LoadFileImage(int theId, const char* thePath);
	// 0x433490
	void Preload(int theId, Module theModule) { GetImage(theId, theModule); }
	// 0x4334b0
	void Free(int theId);
	// 0x433530: a blank 32-bit image the display tracks
	Image* CreateImage(int theWidth, int theHeight);
	// 0x433740, 0x4337a0: untracked images from Resource.dll
	static Image* LoadDllImage(int theId);
	static Image* LoadDllImageKeyed(int theId, uint32_t theKey);
	// 0x433810
	void Untrack(Image* theImage);
	// 0x433870: by id
	void DrawById(int theX, int theY, int theId, const Rect* theSrc, int theScaleX, int theScaleY, int theAlpha,
		Rect& theOut);
	// 0x4338c0, 0x433900
	static void DrawImage(int theX, int theY, Image* theImage, const Rect* theSrc, int theScaleX, int theScaleY,
		int theAlpha, Rect& theOut);
	static void DrawImageKeyed(int theX, int theY, Image* theImage, uint32_t theKey, const Rect* theSrc,
		int theScaleX, int theScaleY, int theAlpha, Rect& theOut);
	// 0x433940: theOut the rect drawn (empty at alpha 0 or below)
	static void Draw(int theX, int theY, Image* theImage, const uint32_t* theKey, const Rect* theSrc, int theScaleX,
		int theScaleY, int theAlpha, Rect& theOut);
	// 0x433aa0
	void GetImageInfo(int theId, ImageInfo& theInfo);
	// 0x42dff0, 0x42e120 (the device's fills)
	void FillRect(const Rect* theRect, uint32_t theColour) { ScreenFillRect(theRect, theColour); }
	void FillRectAlpha(Rect& theRect, int theAlpha, uint32_t theColour)
	{
		ScreenFillRectAlpha(theRect, theAlpha, theColour);
	}
};

} // namespace jb

#endif
