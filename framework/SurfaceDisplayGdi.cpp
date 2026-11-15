#ifdef _WIN32

#include "SurfaceDisplayGdi.h"

#include <SDL_syswm.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string.h>

namespace Sexy
{

struct GdiStretch::Impl
{
	HWND mWindow;
	// a test's copy of the window
	bool mShadow;
	HDC mShadowDC;
	HBITMAP mShadowBitmap;
	HBITMAP mShadowOld;
	uint32_t* mShadowBits;
	int mShadowWidth;
	int mShadowHeight;
	HDC mReferenceDC;
	HBITMAP mReferenceBitmap;
	HBITMAP mReferenceOld;
	uint32_t* mReferenceBits;
	// the window read back
	HDC mReadDC;
	HBITMAP mReadBitmap;
	HBITMAP mReadOld;
	uint32_t* mReadBits;

	Impl() : mWindow(NULL), mShadow(false), mShadowDC(NULL), mShadowBitmap(NULL), mShadowOld(NULL), mShadowBits(NULL), mShadowWidth(0),
		mShadowHeight(0), mReferenceDC(NULL), mReferenceBitmap(NULL), mReferenceOld(NULL), mReferenceBits(NULL), mReadDC(NULL), mReadBitmap(NULL),
		mReadOld(NULL), mReadBits(NULL)
	{
	}

	static void MakeBitmap(int theWidth, int theHeight, HDC& theDC, HBITMAP& theBitmap, HBITMAP& theOld, uint32_t*& theBits)
	{
		BITMAPINFO anInfo;
		memset(&anInfo, 0, sizeof(anInfo));
		anInfo.bmiHeader.biSize = sizeof(anInfo.bmiHeader);
		anInfo.bmiHeader.biWidth = theWidth;
		anInfo.bmiHeader.biHeight = -theHeight;
		anInfo.bmiHeader.biPlanes = 1;
		anInfo.bmiHeader.biBitCount = 32;
		anInfo.bmiHeader.biCompression = BI_RGB;
		theDC = CreateCompatibleDC(NULL);
		void* aBits = NULL;
		theBitmap = CreateDIBSection(theDC, &anInfo, DIB_RGB_COLORS, &aBits, NULL, 0);
		theBits = (uint32_t*) aBits;
		theOld = (HBITMAP) SelectObject(theDC, theBitmap);
	}

	static void FreeBitmap(HDC& theDC, HBITMAP& theBitmap, HBITMAP& theOld, uint32_t*& theBits)
	{
		if (theDC != NULL)
		{
			SelectObject(theDC, theOld);
			DeleteObject(theBitmap);
			DeleteDC(theDC);
		}
		theDC = NULL;
		theBitmap = NULL;
		theOld = NULL;
		theBits = NULL;
	}

	void FreeShadow()
	{
		FreeBitmap(mShadowDC, mShadowBitmap, mShadowOld, mShadowBits);
		FreeBitmap(mReferenceDC, mReferenceBitmap, mReferenceOld, mReferenceBits);
		FreeBitmap(mReadDC, mReadBitmap, mReadOld, mReadBits);
		mShadowWidth = 0;
		mShadowHeight = 0;
	}

	// the frame into theDC, as the comment of Present says
	static void Paint(HDC theDC, const uint32_t* thePixels, int thePitch, int theWidth, int theHeight, const SDL_Rect& theDest,
		const SDL_Rect* theRects, int theCount, int theWindowWidth, int theWindowHeight)
	{
		SetStretchBltMode(theDC, COLORONCOLOR);
		BITMAPINFO anInfo;
		memset(&anInfo, 0, sizeof(anInfo));
		anInfo.bmiHeader.biSize = sizeof(anInfo.bmiHeader);
		anInfo.bmiHeader.biWidth = thePitch / 4;
		anInfo.bmiHeader.biHeight = -theHeight;
		anInfo.bmiHeader.biPlanes = 1;
		anInfo.bmiHeader.biBitCount = 32;
		anInfo.bmiHeader.biCompression = BI_RGB;

		HRGN aClip = NULL;
		if (theCount <= 0)
		{
			// all of it: the bars around the screen first
			HBRUSH aBlack = (HBRUSH) GetStockObject(BLACK_BRUSH);
			RECT aBars[4] = {
				{ 0, 0, theDest.x, theWindowHeight },
				{ theDest.x + theDest.w, 0, theWindowWidth, theWindowHeight },
				{ theDest.x, 0, theDest.x + theDest.w, theDest.y },
				{ theDest.x, theDest.y + theDest.h, theDest.x + theDest.w, theWindowHeight } };
			for (int i = 0; i < 4; i++)
				if ((aBars[i].right > aBars[i].left) && (aBars[i].bottom > aBars[i].top))
					FillRect(theDC, &aBars[i], aBlack);
		}
		else
		{
			const RECT aDestRect = { theDest.x, theDest.y, theDest.x + theDest.w, theDest.y + theDest.h };
			for (int i = 0; i < theCount; i++)
			{
				RECT aRect = { theRects[i].x, theRects[i].y, theRects[i].x + theRects[i].w, theRects[i].y + theRects[i].h };
				RECT aClipped;
				if (!IntersectRect(&aClipped, &aRect, &aDestRect))
					continue;
				HRGN aPiece = CreateRectRgnIndirect(&aClipped);
				if (aClip == NULL)
					aClip = aPiece;
				else
				{
					CombineRgn(aClip, aClip, aPiece, RGN_OR);
					DeleteObject(aPiece);
				}
			}
			if (aClip == NULL)
				return;
			SelectClipRgn(theDC, aClip);
		}
		StretchDIBits(theDC, theDest.x, theDest.y, theDest.w, theDest.h, 0, 0, theWidth, theHeight, thePixels, &anInfo, DIB_RGB_COLORS, SRCCOPY);
		if (aClip != NULL)
		{
			SelectClipRgn(theDC, NULL);
			DeleteObject(aClip);
		}
	}
};

GdiStretch::GdiStretch() : mImpl(new Impl())
{
}

GdiStretch::~GdiStretch()
{
	Detach();
	delete mImpl;
}

bool GdiStretch::Attach(SDL_Window* theWindow, bool theShadow)
{
	Detach();
	SDL_SysWMinfo anInfo;
	SDL_VERSION(&anInfo.version);
	if ((theWindow == NULL) || (!SDL_GetWindowWMInfo(theWindow, &anInfo)) || (anInfo.subsystem != SDL_SYSWM_WINDOWS) ||
		(anInfo.info.win.window == NULL))
		return false;
	mImpl->mWindow = anInfo.info.win.window;
	mImpl->mShadow = theShadow;
	return true;
}

void GdiStretch::Detach()
{
	mImpl->FreeShadow();
	mImpl->mWindow = NULL;
}

bool GdiStretch::WaitForComposition()
{
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
}

bool GdiStretch::Present(const uint32_t* thePixels, int thePitch, int theWidth, int theHeight, const SDL_Rect& theDest, const SDL_Rect* theRects,
	int theCount, int theWindowWidth, int theWindowHeight)
{
	if ((mImpl->mWindow == NULL) || (thePixels == NULL))
		return false;
	HDC aDC = GetDC(mImpl->mWindow);
	if (aDC == NULL)
		return false;
	Impl::Paint(aDC, thePixels, thePitch, theWidth, theHeight, theDest, theRects, theCount, theWindowWidth, theWindowHeight);
	ReleaseDC(mImpl->mWindow, aDC);

	if (mImpl->mShadow)
	{
		if ((mImpl->mShadowDC == NULL) || (mImpl->mShadowWidth != theWindowWidth) || (mImpl->mShadowHeight != theWindowHeight))
		{
			mImpl->FreeShadow();
			Impl::MakeBitmap(theWindowWidth, theWindowHeight, mImpl->mShadowDC, mImpl->mShadowBitmap, mImpl->mShadowOld, mImpl->mShadowBits);
			Impl::MakeBitmap(theWindowWidth, theWindowHeight, mImpl->mReferenceDC, mImpl->mReferenceBitmap, mImpl->mReferenceOld,
				mImpl->mReferenceBits);
			Impl::MakeBitmap(theWindowWidth, theWindowHeight, mImpl->mReadDC, mImpl->mReadBitmap, mImpl->mReadOld, mImpl->mReadBits);
			mImpl->mShadowWidth = theWindowWidth;
			mImpl->mShadowHeight = theWindowHeight;
		}
		Impl::Paint(mImpl->mShadowDC, thePixels, thePitch, theWidth, theHeight, theDest, theRects, theCount, theWindowWidth, theWindowHeight);
	}
	return true;
}

bool GdiStretch::CheckWindow(const uint32_t* thePixels, int thePitch, int theWidth, int theHeight, const SDL_Rect& theDest, int theWindowWidth,
	int theWindowHeight, int* theX, int* theY, uint32_t* theShown, uint32_t* theWanted)
{
	if ((mImpl->mWindow == NULL) || (mImpl->mReadDC == NULL) || (mImpl->mReferenceDC == NULL) || (mImpl->mShadowWidth != theWindowWidth) ||
		(mImpl->mShadowHeight != theWindowHeight))
		return true;
	GdiFlush();
	HDC aDC = GetDC(mImpl->mWindow);
	if (aDC == NULL)
		return true;
	// the window as the present left it, then painted whole, each read back
	BitBlt(mImpl->mReadDC, 0, 0, theWindowWidth, theWindowHeight, aDC, 0, 0, SRCCOPY);
	Impl::Paint(aDC, thePixels, thePitch, theWidth, theHeight, theDest, NULL, 0, theWindowWidth, theWindowHeight);
	GdiFlush();
	BitBlt(mImpl->mReferenceDC, 0, 0, theWindowWidth, theWindowHeight, aDC, 0, 0, SRCCOPY);
	ReleaseDC(mImpl->mWindow, aDC);
	GdiFlush();
	for (int y = 0; y < theWindowHeight; y++)
	{
		const uint32_t* aShown = mImpl->mReadBits + (size_t) y * theWindowWidth;
		const uint32_t* aWanted = mImpl->mReferenceBits + (size_t) y * theWindowWidth;
		for (int x = 0; x < theWindowWidth; x++)
		{
			if ((aShown[x] & 0xFFFFFF) != (aWanted[x] & 0xFFFFFF))
			{
				*theX = x;
				*theY = y;
				*theShown = aShown[x] & 0xFFFFFF;
				*theWanted = aWanted[x] & 0xFFFFFF;
				return false;
			}
		}
	}
	return true;
}

bool GdiStretch::CheckShadow(const uint32_t* thePixels, int thePitch, int theWidth, int theHeight, const SDL_Rect& theDest, int theWindowWidth,
	int theWindowHeight, int* theX, int* theY, uint32_t* theShown, uint32_t* theWanted)
{
	if ((mImpl->mShadowDC == NULL) || (mImpl->mReferenceDC == NULL) || (mImpl->mShadowWidth != theWindowWidth) ||
		(mImpl->mShadowHeight != theWindowHeight))
		return true;
	GdiFlush();
	Impl::Paint(mImpl->mReferenceDC, thePixels, thePitch, theWidth, theHeight, theDest, NULL, 0, theWindowWidth, theWindowHeight);
	GdiFlush();
	for (int y = 0; y < theWindowHeight; y++)
	{
		const uint32_t* aShown = mImpl->mShadowBits + (size_t) y * theWindowWidth;
		const uint32_t* aWanted = mImpl->mReferenceBits + (size_t) y * theWindowWidth;
		for (int x = 0; x < theWindowWidth; x++)
		{
			if ((aShown[x] & 0xFFFFFF) != (aWanted[x] & 0xFFFFFF))
			{
				*theX = x;
				*theY = y;
				*theShown = aShown[x] & 0xFFFFFF;
				*theWanted = aWanted[x] & 0xFFFFFF;
				return false;
			}
		}
	}
	return true;
}

}

#endif
