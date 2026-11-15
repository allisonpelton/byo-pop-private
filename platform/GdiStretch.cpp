// GDI's own stretch of a frame into a window (see GdiStretch.h).

#include "GdiStretch.h"

#include <SDL.h>
#ifdef _WIN32
#include <SDL_syswm.h>
#include <windows.h>
#endif

#include <cstring>

namespace Platform
{

bool StretchToWindow(SDL_Window* theWindow, const uint32_t* thePixels, int thePitch, int theWidth, int theHeight,
	const SDL_Rect* theRects, int theCount, int theScale, int* theDiffering)
{
#ifdef _WIN32
	SDL_SysWMinfo anInfo;
	SDL_VERSION(&anInfo.version);
	if ((thePixels == NULL) || (!SDL_GetWindowWMInfo(theWindow, &anInfo)) || (anInfo.subsystem != SDL_SYSWM_WINDOWS))
		return false;
	HWND aWindow = anInfo.info.win.window;
	HDC aDC = GetDC(aWindow);
	if (aDC == NULL)
		return false;
	SetStretchBltMode(aDC, COLORONCOLOR);
	BITMAPINFO aBitmapInfo;
	memset(&aBitmapInfo, 0, sizeof(aBitmapInfo));
	aBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	aBitmapInfo.bmiHeader.biWidth = thePitch / 4;
	aBitmapInfo.bmiHeader.biHeight = -theHeight;  // top-down, as the frame
	aBitmapInfo.bmiHeader.biPlanes = 1;
	aBitmapInfo.bmiHeader.biBitCount = 32;
	aBitmapInfo.bmiHeader.biCompression = BI_RGB;
	bool aFailed = false;
	for (int i = 0; i < theCount; i++)
	{
		const SDL_Rect& aRect = theRects[i];
		// StretchDIBits takes a source rect's y from the bitmap's bottom row, a top-down bitmap's too; it answers 0 when it fails
		if (StretchDIBits(aDC, aRect.x * theScale, aRect.y * theScale, aRect.w * theScale, aRect.h * theScale, aRect.x,
			theHeight - aRect.y - aRect.h, aRect.w, aRect.h, thePixels, &aBitmapInfo, DIB_RGB_COLORS, SRCCOPY) == 0)
			aFailed = true;
	}
	if (theDiffering != NULL)
	{
		*theDiffering = -1;
		const int aWidth = theWidth * theScale, aHeight = theHeight * theScale;
		BITMAPINFO aReadInfo = aBitmapInfo;
		aReadInfo.bmiHeader.biWidth = aWidth;
		aReadInfo.bmiHeader.biHeight = -aHeight;
		void* aReadBits = NULL;
		HDC aMemoryDC = CreateCompatibleDC(aDC);
		HBITMAP aBitmap = CreateDIBSection(aDC, &aReadInfo, DIB_RGB_COLORS, &aReadBits, NULL, 0);
		if ((aMemoryDC != NULL) && (aBitmap != NULL) && (aReadBits != NULL))
		{
			HGDIOBJ anOld = SelectObject(aMemoryDC, aBitmap);
			BitBlt(aMemoryDC, 0, 0, aWidth, aHeight, aDC, 0, 0, SRCCOPY);
			GdiFlush();
			int aCount = 0;
			for (int y = 0; y < aHeight; y++)
			{
				const uint32_t* aSrc = (const uint32_t*) ((const uint8_t*) thePixels + (size_t) (y / theScale) * thePitch);
				const uint32_t* aDst = (const uint32_t*) aReadBits + (size_t) y * aWidth;
				for (int x = 0; x < aWidth; x++)
					if ((aSrc[x / theScale] & 0xFFFFFF) != (aDst[x] & 0xFFFFFF))
						aCount++;
			}
			*theDiffering = aCount;
			SelectObject(aMemoryDC, anOld);
		}
		if (aBitmap != NULL)
			DeleteObject(aBitmap);
		if (aMemoryDC != NULL)
			DeleteDC(aMemoryDC);
	}
	ReleaseDC(aWindow, aDC);
	return !aFailed;
#else
	(void) theWindow;
	(void) thePixels;
	(void) thePitch;
	(void) theWidth;
	(void) theHeight;
	(void) theRects;
	(void) theCount;
	(void) theScale;
	(void) theDiffering;
	return false;
#endif
}

bool WaitForComposition()
{
#ifdef _WIN32
	typedef HRESULT (WINAPI *DwmFlushFn)(void);
	static DwmFlushFn aFlush = NULL;
	static bool aLooked = false;
	if (!aLooked)
	{
		aLooked = true;
		HMODULE aModule = LoadLibraryW(L"dwmapi.dll");
		if (aModule != NULL)
			aFlush = (DwmFlushFn) (void*) GetProcAddress(aModule, "DwmFlush");
	}
	return (aFlush != NULL) && (SUCCEEDED(aFlush()));
#else
	return false;
#endif
}

}
