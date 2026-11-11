#include "Display.h"

#include "App.h"
#include "Font.h"
#include "Host.h"

#include <algorithm>

namespace jb
{

// 0x42ba30 (mahjongchina), 0x431a70 (mahjongjapan) (the screen is the software display's own buffer,
// 0x5d81cc (mahjongchina), 0x5d92f4 (mahjongjapan) with pitch 0x5d81b8 (mahjongchina), 0x5d92e0 (mahjongjapan) in the
// original)
Display::Display()
{
	if (gScreen == nullptr)
	{
		gScreenPitch = SCREEN_WIDTH * 4;
		gScreen = new uint8_t[(size_t)gScreenPitch * SCREEN_HEIGHT]();
	}
}

// 0x42bcd0 (mahjongchina), 0x431d10 (mahjongjapan)
Display::~Display()
{
	// 0x42c5c0 (mahjongchina), 0x432610 (mahjongjapan): the cached images and the tracked ones deleted. The original
	// deleted the tracked images while each one's Release (0x433810 (mahjongjapan)) took it out of the list it walked,
	// so every second one was skipped and leaked; they are let go of first here.
	for (auto& anEntry : mImages)
		delete anEntry.second;
	mImages.clear();
	std::vector<Image*> aCreated;
	aCreated.swap(mCreated);
	for (Image* anImage : aCreated)
	{
		anImage->mDisplay = nullptr;
		delete anImage;
	}
	// 0x42c7a0 (mahjongchina), 0x4327f0 (mahjongjapan)
	for (auto& anEntry : mFonts)
		delete anEntry.second;
	mFonts.clear();
	Release();
	FreeBlitBuffer();
	delete[] gScreen;
	gScreen = nullptr;
}

// 0x42cd30 (mahjongchina), 0x432db0 (mahjongjapan), 0x42c4d0 (mahjongchina), 0x432520 (mahjongjapan): the 32-bit copies
// are freed, to be made again at the next draw
void Display::Release()
{
	for (auto& anEntry : mImages)
	{
		if (anEntry.second != nullptr)
			anEntry.second->FreePixels();
	}
	for (Image* anImage : mCreated)
		anImage->FreePixels();
}

// 0x42bfc0 (mahjongchina), 0x432000 (mahjongjapan) (the window's style and size are the host's;
// 0x42cc20 (mahjongchina), 0x432c90 (mahjongjapan) makes the screen)
int Display::SetMode(bool theFullScreen, bool theNoAccel)
{
	mAccelerated = false;
	(void)theNoAccel;
	gFullScreen = theFullScreen;
	GetHost()->SetFullScreen(theFullScreen);
	Invalidate();
	return 0;
}

// 0x42c8a0 (mahjongchina), 0x4328f0 (mahjongjapan)
bool Display::BeginFrame()
{
	return gScreen != nullptr;
}

// 0x42c9b0 (mahjongchina), 0x432a00 (mahjongjapan)
void Display::Present()
{
	GetHost()->Present(gScreen, gScreenPitch);
}

// 0x42c3e0 (mahjongchina), 0x432430 (mahjongjapan)
Image* Display::GetImage(int theId, Module theModule)
{
	auto anIt = mImages.find(theId);
	if (anIt != mImages.end())
		return anIt->second;
	Image* anImage = new Image();
	anImage->LoadResource(theId, theModule);
	mImages[theId] = anImage;
	return anImage;
}

// 0x42c690 (mahjongchina), 0x4326e0 (mahjongjapan)
Font* Display::GetFont(int theImageId, int theTextId, Module theModule)
{
	auto anIt = mFonts.find(theImageId);
	if (anIt != mFonts.end())
		return anIt->second;
	Font* aFont = new Font();
	aFont->Load(theImageId, theTextId, theModule);
	mFonts[theImageId] = aFont;
	return aFont;
}

// 0x42d2f0 (mahjongchina), 0x433380 (mahjongjapan)
bool Display::LoadFileImage(int theId, const char* thePath)
{
	if (mImages.find(theId) != mImages.end())
		return true;
	Image* anImage = new Image();
	if (!anImage->LoadFile(thePath))
	{
		delete anImage;
		return false;
	}
	mImages[theId] = anImage;
	return true;
}

// 0x42d420 (mahjongchina), 0x4334b0 (mahjongjapan)
void Display::Free(int theId)
{
	auto anIt = mImages.find(theId);
	if (anIt != mImages.end())
	{
		delete anIt->second;
		mImages.erase(anIt);
	}
}

// 0x42d4a0 (mahjongchina), 0x433530 (mahjongjapan)
Image* Display::CreateImage(int theWidth, int theHeight)
{
	Image* anImage = new Image();
	anImage->Create(theWidth, theHeight, this);
	mCreated.push_back(anImage);
	return anImage;
}

// 0x433740 (mahjongjapan)
Image* Display::LoadDllImage(int theId)
{
	Image* anImage = new Image();
	anImage->LoadDll(theId);
	return anImage;
}

// 0x4337a0 (mahjongjapan)
Image* Display::LoadDllImageKeyed(int theId, uint32_t theKey)
{
	Image* anImage = new Image();
	anImage->LoadDllKeyed(theId, theKey);
	return anImage;
}

// 0x433810 (mahjongjapan)
void Display::Untrack(Image* theImage)
{
	auto anIt = std::find(mCreated.begin(), mCreated.end(), theImage);
	if (anIt != mCreated.end())
		mCreated.erase(anIt);
}

// 0x42d720 (mahjongchina), 0x433870 (mahjongjapan) (the image comes from Resource.dll the first time)
void Display::DrawById(int theX, int theY, int theId, const Rect* theSrc, int theScaleX, int theScaleY,
	int theAlpha, Rect& theOut)
{
	Image* anImage = GetImage(theId, Module::DLL);
	Draw(theX, theY, anImage, nullptr, theSrc, theScaleX, theScaleY, theAlpha, theOut);
}

// 0x42d770 (mahjongchina), 0x4338c0 (mahjongjapan)
void Display::DrawImage(int theX, int theY, Image* theImage, const Rect* theSrc, int theScaleX, int theScaleY,
	int theAlpha, Rect& theOut)
{
	Draw(theX, theY, theImage, nullptr, theSrc, theScaleX, theScaleY, theAlpha, theOut);
}

// 0x42d7b0 (mahjongchina), 0x433900 (mahjongjapan)
void Display::DrawImageKeyed(int theX, int theY, Image* theImage, uint32_t theKey, const Rect* theSrc,
	int theScaleX, int theScaleY, int theAlpha, Rect& theOut)
{
	Draw(theX, theY, theImage, &theKey, theSrc, theScaleX, theScaleY, theAlpha, theOut);
}

// 0x42d7f0 (mahjongchina), 0x433940 (mahjongjapan) (the accelerated display's alpha state is not ported)
void Display::Draw(int theX, int theY, Image* theImage, const uint32_t* theKey, const Rect* theSrc, int theScaleX,
	int theScaleY, int theAlpha, Rect& theOut)
{
	if (-1 < theAlpha)
	{
		int anAlpha = theAlpha;
		if (theAlpha < 0x100)
		{
			if (theAlpha == 0)
			{
				SetRect(theOut, theX, theY, theX, theY);
				return;
			}
		}
		else
			anAlpha = 0xff;
		if (theKey != nullptr)
			theImage->SetColourKey(*theKey);
		Rect aSrc;
		if (theSrc == nullptr)
			SetRect(aSrc, 0, 0, theImage->mWidth, theImage->mHeight);
		else
			aSrc = *theSrc;
		theImage->Draw(theX, theY, aSrc, theScaleX, theScaleY, anAlpha);
		int aHeight = MulDiv(aSrc.bottom - aSrc.top, theScaleY, 100);
		int aWidth = MulDiv(aSrc.right - aSrc.left, theScaleX, 100);
		SetRect(theOut, theX, theY, aWidth + theX, aHeight + theY);
		return;
	}
	SetRect(theOut, theX, theY, theX, theY);
}

// 0x42d950 (mahjongchina), 0x433aa0 (mahjongjapan)
void Display::GetImageInfo(int theId, ImageInfo& theInfo)
{
	GetImage(theId, Module::DLL)->GetInfo(theInfo);
}

} // namespace jb
