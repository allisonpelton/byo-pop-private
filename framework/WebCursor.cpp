// port: the game's custom cursor for the web page (docs/WEB_PLAYER.md, "The game's cursor on the page"). Over the
// canvas a SexyAppFramework game hides the browser's cursor and draws its image cursor into the frame
// (SexyAppBase::EnforceCursor, DDInterface::DrawCursor); the page polls the two calls below and shows the same image
// as the CSS cursor everywhere else (web/shell.html, pollCursor). They only read the game's state: nothing the game
// does, draws or saves depends on them. Web only; natively this file is empty.
//
// byopop_cursor() is 0 when the page should show the browser's own cursor, otherwise a number that changes whenever
// the image it should show does. byopop_cursor_copy() puts that image where the page can read it:
// Module.byopopCursor = {id, width, height, hotX, hotY, screenWidth, screenHeight, rgba}, rgba a Uint8Array of
// width * height straight-alpha RGBA pixels, screenWidth/Height the game's logical screen (the page scales the
// cursor by the canvas's size over it).

#ifdef __EMSCRIPTEN__
#include "DDInterface.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

#include <emscripten.h>
#include <stdint.h>
#include <vector>

using namespace Sexy;

// The page's side of byopop_cursor_copy: the pixels are copied out of the heap, so the page keeps them
EM_JS(void, byopop_cursor_deliver, (int theId, int theWidth, int theHeight, int theHotX, int theHotY,
	int theScreenWidth, int theScreenHeight, const uint8_t* thePixels), {
	Module['byopopCursor'] = {
		id: theId, width: theWidth, height: theHeight, hotX: theHotX, hotY: theHotY,
		screenWidth: theScreenWidth, screenHeight: theScreenHeight,
		rgba: HEAPU8.slice(thePixels, thePixels + theWidth * theHeight * 4)
	};
});

namespace
{

const int kCursorAreaSize = 64;	// DisplayGeometry.cpp's CURSOR_AREA_SIZE: the image is centred on the mouse in a 64x64 area
const int kMaxCursorSize = 512;	// a cursor is small; anything larger is not copied

MemoryImage* sShownImage = NULL;
int sShownBitsChanged = 0;
int sShownId = 0;

// The image the page's cursor should show, or NULL for the browser's own cursor. It is what EnforceCursor picks for
// the system cursor to hide (the image of the current cursor, when custom cursors are on or it is CURSOR_CUSTOM),
// except that a mouse outside the canvas counts as the pointer: the game then shows no cursor of its own, but the
// page, where the mouse is, should show the game's pointer.
MemoryImage* ShownCursorImage()
{
	SexyAppBase* anApp = gSexyAppBase;
	if ((anApp == NULL) || (!anApp->mInitialized) || (anApp->mShutdown) || (anApp->mSEHOccured) ||
		(anApp->mDDInterface == NULL))
		return NULL;

	int aCursorNum = anApp->mMouseIn ? anApp->mCursorNum : (int) CURSOR_POINTER;
	if ((aCursorNum < 0) || (aCursorNum >= NUM_CURSORS))
		aCursorNum = CURSOR_POINTER;
	if ((!anApp->mCustomCursorsEnabled) && (aCursorNum != CURSOR_CUSTOM))
		return NULL;

	MemoryImage* anImage = dynamic_cast<MemoryImage*>(anApp->mCursorImages[aCursorNum]);
	if ((anImage == NULL) || (anImage->mWidth <= 0) || (anImage->mHeight <= 0) ||
		(anImage->mWidth > kMaxCursorSize) || (anImage->mHeight > kMaxCursorSize))
		return NULL;
	// the pixels, read as they are (GetBits would unpalletize the image); DrawCursor has made them by its first frame
	if ((anImage->mBits == NULL) && ((anImage->mColorTable == NULL) || (anImage->mColorIndices == NULL)))
		return NULL;
	return anImage;
}

}

extern "C"
{

// 0 for the browser's own cursor; otherwise an id that is the same while the page's cursor image is the same
EMSCRIPTEN_KEEPALIVE int byopop_cursor()
{
	MemoryImage* anImage = ShownCursorImage();
	if (anImage == NULL)
		return 0;
	if ((anImage != sShownImage) || (anImage->mBitsChangedCount != sShownBitsChanged))
	{
		sShownImage = anImage;
		sShownBitsChanged = anImage->mBitsChangedCount;
		sShownId = (sShownId >= 0x3fffffff) ? 1 : sShownId + 1;
	}
	return sShownId;
}

// Copies the image byopop_cursor() is about to Module.byopopCursor. Returns its id, 0 when there is none.
EMSCRIPTEN_KEEPALIVE int byopop_cursor_copy()
{
	static std::vector<uint8_t> sPixels;

	int anId = byopop_cursor();
	MemoryImage* anImage = ShownCursorImage();
	if ((anId == 0) || (anImage == NULL))
		return 0;

	int aWidth = anImage->mWidth;
	int aHeight = anImage->mHeight;
	size_t aCount = (size_t) aWidth * aHeight;
	sPixels.resize(aCount * 4);
	for (size_t i = 0; i < aCount; i++)
	{
		uint32_t aPixel = (anImage->mBits != NULL) ? (uint32_t) anImage->mBits[i] : (uint32_t) anImage->mColorTable[anImage->mColorIndices[i]];
		sPixels[i * 4 + 0] = (uint8_t) (aPixel >> 16);
		sPixels[i * 4 + 1] = (uint8_t) (aPixel >> 8);
		sPixels[i * 4 + 2] = (uint8_t) aPixel;
		sPixels[i * 4 + 3] = (uint8_t) (aPixel >> 24);
	}

	// where DDInterface::DrawCursor puts the image's corner, from the mouse: the mouse is at this point of the image
	int aHotX = kCursorAreaSize / 2 - (kCursorAreaSize - aWidth) / 2;
	int aHotY = kCursorAreaSize / 2 - (kCursorAreaSize - aHeight) / 2;
	byopop_cursor_deliver(anId, aWidth, aHeight, aHotX, aHotY, gSexyAppBase->mDDInterface->mWidth,
		gSexyAppBase->mDDInterface->mHeight, &sPixels[0]);
	return anId;
}

}
#endif
