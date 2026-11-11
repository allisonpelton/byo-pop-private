// CustomResourceManager.cpp: CustomResourceManager (CustomResourceManager.h).
#include "CustomResourceManager.h"

#include "DDImage.h"
#include "SexyAppBase.h"
#include "WinDMApp.h"

#include <vector>

using namespace Sexy;

static bool HasAttribute(const XMLParamMap& theMap, const char* theName)
{
	return theMap.find(theName) != theMap.end();
}

// the mark on the variant of a shared image that was shrunk, so that it is never taken for the full picture of its file
static const char* const kSmallVariant = "~640";

// 0x5afda3: the picture at five eighths of its size, for the 640 by 480 game. Every 8 by 8 pixels become 5 by 5: a new
// pixel covers 1.6 by 1.6 old ones, and each channel is the sum of the old pixels by the part of them it covers, in
// 256ths, cut off. The new size is rounded up, and what lies past the right and bottom edge counts as theFill. (The
// original writes the result as a .tga into the user's folder, sm_<path>, and loads that from then on while it is newer
// than the picture; the port shrinks at each load and keeps no copy.)
void CustomResourceManager::ShrinkImage(MemoryImage* theImage, unsigned long theFill, bool commitBits)
{
	// along one axis: the first old pixel of each of the five new ones within a group of eight, how many it covers, and
	// the fifths of each that it takes (they add up to 8)
	static const int kFirst[5] = {0, 1, 3, 4, 6};
	static const int kCount[5] = {2, 3, 2, 3, 2};
	static const int kWeight[5][3] = {{5, 3, 0}, {2, 5, 1}, {4, 4, 0}, {1, 5, 2}, {3, 5, 0}};

	int aWidth = theImage->GetWidth();
	int aHeight = theImage->GetHeight();
	ulong* aBits = theImage->GetBits();
	if (aBits == NULL || aWidth <= 0 || aHeight <= 0)
		return;
	int aNewWidth = (aWidth * 5 + 7) / 8;
	int aNewHeight = (aHeight * 5 + 7) / 8;
	std::vector<ulong> aNewBits((size_t)aNewWidth * aNewHeight);
	for (int aY = 0; aY < aNewHeight; aY++)
	{
		int aRow = (aY / 5) * 8 + kFirst[aY % 5];
		for (int aX = 0; aX < aNewWidth; aX++)
		{
			int aCol = (aX / 5) * 8 + kFirst[aX % 5];
			unsigned int aSum[4] = {0, 0, 0, 0};
			for (int j = 0; j < kCount[aY % 5]; j++)
			{
				for (int i = 0; i < kCount[aX % 5]; i++)
				{
					ulong aPixel = (aCol + i < aWidth && aRow + j < aHeight) ? aBits[(aRow + j) * aWidth + aCol + i] : theFill;
					unsigned int aPart = kWeight[aX % 5][i] * kWeight[aY % 5][j] * 4;
					aSum[0] += (aPixel >> 24 & 0xff) * aPart;
					aSum[1] += (aPixel >> 16 & 0xff) * aPart;
					aSum[2] += (aPixel >> 8 & 0xff) * aPart;
					aSum[3] += (aPixel & 0xff) * aPart;
				}
			}
			aNewBits[(size_t)aY * aNewWidth + aX] = (ulong)(aSum[0] >> 8) << 24 | (aSum[1] >> 8) << 16 | (aSum[2] >> 8) << 8 | aSum[3] >> 8;
		}
	}
	theImage->SetBits(aNewBits.data(), aNewWidth, aNewHeight, commitBits);
}

// whether the small game shrinks this picture: not one whose name says "no resize", and not a small one
static bool ShrinksAt640(const std::string& thePath)
{
	if (thePath.find("nr_") != std::string::npos)
		return false;
	size_t aSlash = thePath.find_last_of("\\/");
	return thePath.compare(aSlash == std::string::npos ? 0 : aSlash + 1, 3, "sm_") != 0;
}

// 0x5b4852: an image of the game's data (a backdrop, a planet, the galaxy's map). The 640 by 480 game takes it at five
// eighths of its size
Image* CustomResourceManager::GetCachedImage(const std::string& thePath)
{
	WinDMApp* anApp = (WinDMApp*)mApp;
	if (anApp->mWidth != 640 || !ShrinksAt640(thePath))
		return mApp->GetImage(thePath, true);
	anApp->mShrinkLoads++;
	anApp->mShrinkFill = 0;
	Image* anImage = mApp->GetImage(thePath, true);
	anApp->mShrinkLoads--;
	return anImage;
}

// 0x5b4a69
bool CustomResourceManager::DoLoadImage(ImageRes* theRes)
{
	bool aSmallArt = HasAttribute(theRes->mXMLAttributes, "sm_path");
	if (aSmallArt)
	{
		// the small art is for the 640x480 mode; the others take the path
		const char* aKey = (mApp->mWidth == 640) ? "sm_path" : "path";
		theRes->mPath = "images\\" + SexyStringToStringFast(theRes->mXMLAttributes[aKey]);
	}

	if (theRes->mPath == "images\\!")
	{
		// a blank image for the program to draw into
		theRes->mImage = new DDImage(mApp->mDDInterface);
		theRes->mImage.mOwnsUnshared = true;
		return true;
	}

	// the 640 by 480 game: a picture without small art of its own is shrunk as it loads, the border of one whose name
	// says it has no alpha ("al_") filled with black. The shared image of the shrunk picture is kept apart from the full
	// one's by its variant
	WinDMApp* anApp = (WinDMApp*)mApp;
	size_t aMark = theRes->mVariant.rfind(kSmallVariant);
	if (aMark != std::string::npos && aMark + strlen(kSmallVariant) == theRes->mVariant.size())
		theRes->mVariant.erase(aMark);
	bool aShrink = anApp->mWidth == 640 && !aSmallArt && ShrinksAt640(theRes->mPath);
	if (!aShrink)
		return ResourceManager::DoLoadImage(theRes);
	theRes->mVariant += kSmallVariant;
	anApp->mShrinkLoads++;
	anApp->mShrinkFill = theRes->mPath.find("al_") != std::string::npos ? 0xff000000 : 0;
	bool aLoaded = ResourceManager::DoLoadImage(theRes);
	anApp->mShrinkLoads--;
	return aLoaded;
}

// 0x5b4f8c
bool CustomResourceManager::DoLoadFont(FontRes* theRes)
{
	if (HasAttribute(theRes->mXMLAttributes, "sm_path"))
	{
		const char* aKey = (mApp->mWidth == 640) ? "sm_path" : "path";
		theRes->mPath = "data\\" + SexyStringToStringFast(theRes->mXMLAttributes[aKey]);
	}
	return ResourceManager::DoLoadFont(theRes);
}
