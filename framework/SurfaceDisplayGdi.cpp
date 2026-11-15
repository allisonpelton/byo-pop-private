#ifdef _WIN32

#include "SurfaceDisplayGdi.h"
#include "LinearStretch.h"

#include <SDL_syswm.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
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
	// PresentLinear's band: mBandWidth x mBandRows pixels, under kBandBytes
	HDC mBandDC;
	HBITMAP mBandBitmap;
	HBITMAP mBandOld;
	uint32_t* mBandBits;
	int mBandWidth;
	int mBandRows;
	enum { kBandBytes = 1000 * 1024 };

	Impl() : mWindow(NULL), mShadow(false), mShadowDC(NULL), mShadowBitmap(NULL), mShadowOld(NULL), mShadowBits(NULL), mShadowWidth(0),
		mShadowHeight(0), mReferenceDC(NULL), mReferenceBitmap(NULL), mReferenceOld(NULL), mReferenceBits(NULL), mReadDC(NULL), mReadBitmap(NULL),
		mReadOld(NULL), mReadBits(NULL), mBandDC(NULL), mBandBitmap(NULL), mBandOld(NULL), mBandBits(NULL), mBandWidth(0), mBandRows(0)
	{
	}

	void FreeBand()
	{
		FreeBitmap(mBandDC, mBandBitmap, mBandOld, mBandBits);
		mBandWidth = 0;
		mBandRows = 0;
	}

	// a band at least theWidth wide; false when GDI gives none
	bool MakeBand(int theWidth)
	{
		if ((mBandDC != NULL) && (mBandWidth >= theWidth))
			return true;
		FreeBand();
		const int aRows = std::max(1, (int) kBandBytes / (4 * std::max(1, theWidth)));
		MakeBitmap(theWidth, aRows, mBandDC, mBandBitmap, mBandOld, mBandBits);
		if (mBandBits == NULL)
		{
			FreeBand();
			return false;
		}
		mBandWidth = theWidth;
		mBandRows = aRows;
		return true;
	}

	// the black bars of a window of theWindowWidth x theWindowHeight around theDest
	static void PaintBars(HDC theDC, const SDL_Rect& theDest, int theWindowWidth, int theWindowHeight)
	{
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

	// PresentLinear's painting into theDC (the window's, or the shadow's)
	bool PaintLinear(HDC theDC, const LinearStretch& theStretch, const uint32_t* thePixels, int thePitch, const SDL_Rect& theDest,
		const SDL_Rect* theRects, int theCount, int theWindowWidth, int theWindowHeight)
	{
		if (!MakeBand(theDest.w))
			return false;
		if (theCount <= 0)
		{
			PaintBars(theDC, theDest, theWindowWidth, theWindowHeight);
			theRects = &theDest;
			theCount = 1;
		}
		for (int i = 0; i < theCount; i++)
		{
			// the part in the presentation rect's own pixels
			const int x0 = std::max(theRects[i].x, theDest.x) - theDest.x;
			const int y0 = std::max(theRects[i].y, theDest.y) - theDest.y;
			const int x1 = std::min(theRects[i].x + theRects[i].w, theDest.x + theDest.w) - theDest.x;
			const int y1 = std::min(theRects[i].y + theRects[i].h, theDest.y + theDest.h) - theDest.y;
			if ((x0 >= x1) || (y0 >= y1))
				continue;
			for (int y = y0; y < y1; y += mBandRows)
			{
				const int aRows = std::min(mBandRows, y1 - y);
				// GDI may still be reading the band for the last copy
				GdiFlush();
				theStretch.Draw(thePixels, thePitch, x0, y, x1, y + aRows, mBandBits, mBandWidth);
				BitBlt(theDC, theDest.x + x0, theDest.y + y, x1 - x0, aRows, mBandDC, 0, 0, SRCCOPY);
			}
		}
		return true;
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
			PaintBars(theDC, theDest, theWindowWidth, theWindowHeight);
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
	mImpl->FreeBand();
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

bool GdiStretch::PresentLinear(const LinearStretch& theStretch, const uint32_t* thePixels, int thePitch, const SDL_Rect& theDest,
	const SDL_Rect* theRects, int theCount, int theWindowWidth, int theWindowHeight)
{
	if ((mImpl->mWindow == NULL) || (thePixels == NULL))
		return false;
	HDC aDC = GetDC(mImpl->mWindow);
	if (aDC == NULL)
		return false;
	const bool aPainted = mImpl->PaintLinear(aDC, theStretch, thePixels, thePitch, theDest, theRects, theCount, theWindowWidth, theWindowHeight);
	ReleaseDC(mImpl->mWindow, aDC);
	if (!aPainted)
		return false;

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
		mImpl->PaintLinear(mImpl->mShadowDC, theStretch, thePixels, thePitch, theDest, theRects, theCount, theWindowWidth, theWindowHeight);
	}
	return true;
}

bool GdiStretch::ReadWindow(std::vector<uint32_t>& theBits, int theWindowWidth, int theWindowHeight)
{
	if ((mImpl->mWindow == NULL) || (theWindowWidth <= 0) || (theWindowHeight <= 0))
		return false;
	HDC aReadDC = NULL;
	HBITMAP aBitmap = NULL, anOld = NULL;
	uint32_t* aBits = NULL;
	Impl::MakeBitmap(theWindowWidth, theWindowHeight, aReadDC, aBitmap, anOld, aBits);
	bool aRead = false;
	HDC aDC = (aBits != NULL) ? GetDC(mImpl->mWindow) : NULL;
	if (aDC != NULL)
	{
		GdiFlush();
		aRead = BitBlt(aReadDC, 0, 0, theWindowWidth, theWindowHeight, aDC, 0, 0, SRCCOPY) != FALSE;
		ReleaseDC(mImpl->mWindow, aDC);
		GdiFlush();
	}
	if (aRead)
	{
		theBits.resize((size_t) theWindowWidth * theWindowHeight);
		for (size_t i = 0; i < theBits.size(); i++)
			theBits[i] = aBits[i] & 0xFFFFFF;
	}
	Impl::FreeBitmap(aReadDC, aBitmap, anOld, aBits);
	return aRead;
}

bool GdiStretch::CheckShadowAgainst(const std::vector<uint32_t>& theWanted, int theWindowWidth, int theWindowHeight, int* theX, int* theY,
	uint32_t* theShown, uint32_t* theWantedColour)
{
	if ((mImpl->mShadowDC == NULL) || (mImpl->mShadowWidth != theWindowWidth) || (mImpl->mShadowHeight != theWindowHeight) ||
		(theWanted.size() != (size_t) theWindowWidth * theWindowHeight))
		return true;
	GdiFlush();
	for (int y = 0; y < theWindowHeight; y++)
	{
		const uint32_t* aShown = mImpl->mShadowBits + (size_t) y * theWindowWidth;
		const uint32_t* aWanted = &theWanted[(size_t) y * theWindowWidth];
		for (int x = 0; x < theWindowWidth; x++)
		{
			if ((aShown[x] & 0xFFFFFF) != (aWanted[x] & 0xFFFFFF))
			{
				*theX = x;
				*theY = y;
				*theShown = aShown[x] & 0xFFFFFF;
				*theWantedColour = aWanted[x] & 0xFFFFFF;
				return false;
			}
		}
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
