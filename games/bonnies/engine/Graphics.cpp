// The graphics: the screen, images, the cursor and the filled shapes (see Graphics.h). The images' primitives are in
// Draw.cpp.

#include "Graphics.h"
#include "Exe.h"
#include "File.h"
#include "Host.h"
#include "Platform.h"
#include "Resources.h"

#include <cstring>

namespace bonnies
{

Graphics* gGraphics = nullptr;
uint32_t* gPixels = nullptr;
int gPitch = 0;
int gPitch4 = 0;
int gWidth = 0;
int gHeight = 0;
float gInvWidth = 0.0f;
float gInvHeight = 0.0f;
int gClipX0 = 0;
int gClipY0 = 0;
int gClipX1 = 0;
int gClipY1 = 0;
int gFullScreen = 0;
int gFrame = 0;
uint32_t gFrameTimes[64];
uint32_t gLastPresent = 0;
int gRedShift[256];
int gGreenShift[256];
int gBlueShift[256];
Str gCaption;
Str gIconPath;
const char* const* gArrowCursor = nullptr;
const char* const* gHandCursor = nullptr;
Str gMatteName;      // 0x4818ec: a matte to use instead of <name>_Matte.png (empty)

namespace
{
const char* const kExtensions[2] = {".png", ".jpg"};  // 0x4818dc
bool gHaveScreen = false;  // 0x4818ac: the screen surface

// The cursors made so far (0x480b28: ten pairs of the art and its cursor, the latest first)
struct CachedCursor
{
	const char* const* mArt = nullptr;
	uint8_t mData[128];
	uint8_t mMask[128];
	int mHotX = 0;
	int mHotY = 0;
};
CachedCursor gCursors[10];
bool gCursorsInit = false;  // 0x4818c8

std::vector<const char*> gArrowRows;
std::vector<const char*> gHandRows;
}

// 0x421e50
double framesPerSecond()
{
	if (gFrame < 2)
		return 1.0;
	int aLast = gFrame - 1;
	aLast = aLast & -(int)(aLast >= 0);
	int aFirst = aLast - 0x3f;
	aFirst = aFirst & -(int)(aFirst >= 0);
	int aTime = (int)(gFrameTimes[aLast % 64] - gFrameTimes[aFirst % 64]);
	double anAverage = (double)aTime / (double)(aLast - aFirst + 1);
	if (anAverage <= (double)0.1f)
		anAverage = (double)0.1f;
	return (double)ftol(1000.0 / anAverage * 10.0) * (double)0.1f;
}

// 0x421f00
void drawRect(float theX, float theY, float theW, float theH, uint32_t theColor, int theThickness)
{
	float t = (float)theThickness;
	fillRect(theX, theY, theW, (float)(t * (double)gInvHeight), theColor);
	float aH = (float)(t * (double)gInvHeight);
	fillRect(theX, (float)((double)theY + theH - aH), theW, aH, theColor);
	double aT = t * (double)gInvHeight;
	fillRect(theX, (float)(aT + theY), (float)(t * (double)gInvWidth), (float)(theH - (aT + aT)), theColor);
	aT = t * (double)gInvHeight;
	float aW = (float)(t * (double)gInvWidth);
	fillRect((float)((double)theX + theW - aW), (float)(aT + theY), aW, (float)(theH - (aT + aT)), theColor);
}

// 0x421ff0
int frameCount()
{
	return gFrame;
}

// 0x422000. The original retried a file it could not open up to 80 times, 80 ms apart.
bool loadSurface(const Str& thePath, const Str& theType, HostSurface& theSurface)
{
	(void)theType;
	std::vector<uint8_t> aData;
	Str aName = baseName(thePath);
	int aHandle = gPak != nullptr ? gPak->openEntry(aName) : -1;
	if (aHandle != -1)
	{
		int aSize = gPak->size(aName);
		aData.resize(aSize + 100);
		gPak->read(aHandle, aData.data(), aSize);
		aData.resize(aSize);
		bool anOk = host()->decodeImage(aData, theSurface);
		gPak->closeEntry();
		return anOk;
	}
	if (!host()->userFileExists(thePath) ? !host()->readGameFile(thePath, aData) : !host()->readUserFile(thePath, aData))
		return false;
	return host()->decodeImage(aData, theSurface);
}

// 0x4221f0
void freeImage(Image* theImage)
{
	delete[] theImage->mFrames;
	delete theImage;
}

// 0x422220
void drawBevel(float theX, float theY, float theW, float theH, uint32_t theColor, uint32_t theEdgeColor, int theRaised,
	float theSize, float theCorners)
{
	float aRight = (float)((double)theX + theW);
	float aBottom = (float)((double)theY + theH);
	int aLines = ftol((double)theSize + (double)0.98f);
	int anInset = ftol(theSize) - 1;
	anInset = anInset & -(int)(anInset >= 0);
	double anInset2 = (double)(anInset * 2);
	double anInset1 = (double)anInset;
	fillRect((float)(anInset1 * gInvWidth + theX), (float)((double)gInvHeight * anInset1 + theY),
		(float)(theW - anInset2 * gInvWidth), (float)(theH - anInset2 * gInvHeight), theColor);
	int aCorners = ftol(theCorners);
	if (3 < aCorners)
		aCorners = 4;
	float aFraction = (float)(1.0 - ((double)aLines - theSize));
	int i = aLines - 1;
	if (-1 < i)
	{
		int aNext = aLines;
		do
		{
			// 0x458cc0: how far each line's ends are cut in, by the corners' kind
			int aCut = exeI32(0x458cc0 + (i + aCorners * 5) * 4);
			int aShade = 0x41;
			if (i == aLines - 1)
				aShade = ftol((double)aFraction * 0.5 * 65.0);
			else if (i == aLines - 2)
				aShade = ftol(((double)aFraction + 1.0) * (double)0.5f * 0.8 * 65.0);
			uint32_t aLight;
			if (theRaised == 0)
			{
				aLight = addRGB(theEdgeColor, -aShade, -aShade, -aShade);
			}
			else
			{
				aLight = addRGB(theEdgeColor, aShade, aShade, aShade);
				aShade = -aShade;
			}
			uint32_t aDark = addRGB(theEdgeColor, aShade, aShade, aShade);
			float aCut2 = (float)((aCut + i) * 2);
			double aCut1 = (double)(aCut + i);
			fillRect((float)(aCut1 * gInvWidth + theX), (float)((double)i * gInvHeight + theY),
				(float)(theW - (double)aCut2 * gInvWidth), gInvHeight, aLight);
			double aNextF = (double)aNext;
			fillRect((float)(aCut1 * gInvWidth + theX), (float)(aBottom - aNextF * gInvHeight),
				(float)(theW - (double)aCut2 * gInvWidth), gInvHeight, aDark);
			fillRect((float)((double)i * gInvWidth + theX), (float)(aCut1 * gInvHeight + theY), gInvWidth,
				(float)(theH - (double)aCut2 * gInvHeight), aLight);
			fillRect((float)(aRight - aNextF * gInvWidth), (float)(aCut1 * gInvHeight + theY), gInvWidth,
				(float)(theH - (double)aCut2 * gInvHeight), aDark);
			float aCx = (float)(aNextF * gInvWidth);
			float aCy = (float)(aNextF * gInvHeight);
			if (aCorners == 1)
			{
				fillRect((float)((double)aCx + theX), (float)((double)aCy + theY), gInvWidth, gInvHeight, aLight);
				float aY2 = (float)((double)aBottom - aCy);
				fillRect((float)((double)aCx + theX), aY2, gInvWidth, gInvHeight, aLight);
				float aX2 = (float)((double)aRight - aCx);
				fillRect(aX2, (float)((double)aCy + theY), gInvWidth, gInvHeight, aLight);
				fillRect(aX2, aY2, gInvWidth, gInvHeight, aDark);
			}
			else if (1 < aCorners)
			{
				float aX1 = (float)((double)aCx + theX);
				float aY1 = (float)((double)aCy + theY);
				fillRect(aX1, aY1, (float)((double)gInvWidth + gInvWidth), gInvHeight, aLight);
				fillRect(aX1, (float)((double)gInvHeight + aY1), gInvWidth, gInvHeight, aLight);
				float aY3 = (float)(((double)aBottom - aCy) - gInvHeight);
				fillRect(aX1, aY3, (float)((double)gInvWidth + gInvWidth), gInvHeight, aDark);
				fillRect(aX1, (float)((double)aY3 - gInvHeight), gInvWidth, gInvHeight, aLight);
				float aX3 = (float)(((double)aRight - aCx) - ((double)gInvWidth + gInvWidth));
				fillRect(aX3, aY1, (float)((double)gInvWidth + gInvWidth), gInvHeight, aLight);
				fillRect((float)((double)gInvWidth + aX3), (float)((double)gInvHeight + aY1), gInvWidth, gInvHeight,
					aDark);
				fillRect(aX3, aY3, (float)((double)gInvWidth + gInvWidth), gInvHeight, aDark);
				fillRect((float)((double)gInvWidth + aX3), (float)((double)aY3 - gInvHeight), gInvWidth, gInvHeight,
					aDark);
			}
			i = i - 1;
			aNext = aNext - 1;
		} while (-1 < i);
	}
}

// 0x4227d0
void Graphics::shutdown()
{
	gHaveScreen = false;
	gPixels = nullptr;
}

// 0x422800
void setCaption(const Str& theCaption)
{
	host()->setCaption(theCaption);
}

// 0x422860. The browser and the native window take their icon from the page and the build.
void setIcon(const Str& thePath)
{
	(void)thePath;
}

// 0x422900
bool hasInputFocus()
{
	return (host()->getAppState() >> 1 & 1) != 0;
}

// 0x422910
void Graphics::setCursor(const char* const* theArt)
{
	if (!gCursorsInit)
	{
		gCursorsInit = true;
		for (int i = 0; i < 10; i++)
			gCursors[i].mArt = nullptr;
	}
	int i = 0;
	do
	{
		if (gCursors[i].mArt == theArt)
		{
			host()->setCursor(gCursors[i].mData, gCursors[i].mMask, gCursors[i].mHotX, gCursors[i].mHotY);
			return;
		}
		i = i + 1;
	} while (i < 10);
	CachedCursor aCursor;
	std::memset(aCursor.mData, 0, sizeof(aCursor.mData));
	std::memset(aCursor.mMask, 0, sizeof(aCursor.mMask));
	int aHotX = 0;
	int aHotY = 0;
	int aRow = 0;
	int aColumn = 0;
	do
	{
		do
		{
			int aByte = aColumn / 8;
			int aBit = 7 - aColumn % 8;
			switch (theArt[aRow][aColumn])
			{
			case '.':
				aCursor.mMask[aByte + aRow * 4] |= (uint8_t)(1 << aBit);
				break;
			case '0':
				aRow = 999;
				aColumn = 999;
				break;
			case 0:
				aColumn = 999;
				break;
			case 'H':
				aHotX = aColumn;
				aHotY = aRow;
				// fall through
			case 'B':
				aCursor.mData[aByte + aRow * 4] |= (uint8_t)(1 << aBit);
				aCursor.mMask[aByte + aRow * 4] |= (uint8_t)(1 << aBit);
				break;
			}
			aColumn = aColumn + 1;
		} while (aColumn < 0x20);
		aRow = aRow + 1;
		if (0x1f < aRow)
		{
			aCursor.mArt = theArt;
			aCursor.mHotX = aHotX;
			aCursor.mHotY = aHotY;
			host()->setCursor(aCursor.mData, aCursor.mMask, aHotX, aHotY);
			for (int j = 9; j > 0; j--)
				gCursors[j] = gCursors[j - 1];
			gCursors[0] = aCursor;
			return;
		}
		aColumn = 0;
	} while (true);
}

// 0x422c40
void iconify()
{
	host()->iconify();
}

// 0x422c50
void showCursor()
{
	host()->showCursor(true);
}

// 0x422c60
void hideCursor()
{
	host()->showCursor(false);
}

// 0x422c70
void buildShiftTables()
{
	int i = 0;
	do
	{
		gRedShift[i] = i << 0x10;
		gGreenShift[i] = i << 8;
		gBlueShift[i] = i;
		i = i + 1;
	} while (i < 0x100);
}

// 0x422ca0
int imageExists(const Str& theName)
{
	for (int i = 0; i < 2; i++)
	{
		Str aPath = graphicsDir(true);
		aPath += theName;
		aPath += kExtensions[i];
		if (fileExists(aPath))
			return 1;
	}
	return 0;
}

// 0x422de0
Image* loadImageSet(const Str& theName)
{
	host()->trace("LISFRM 1");
	host()->trace(theName.c_str());
	Image* anImage = new Image();
	anImage->mFrameCount = 0;
	HostSurface aSurface;
	bool aLoaded = false;
	Str aPath;
	for (int i = 0; i < 2; i++)
	{
		aPath = graphicsDir(true);
		aPath += theName;
		aPath += kExtensions[i];
		if (fileExists(aPath))
		{
			aLoaded = loadSurface(aPath, kExtensions[i], aSurface);
			break;
		}
	}
	bool aWhite = gLoadingFont >= 1;
	if (!aLoaded)
	{
		// The original formats text 0x66 with the name, drops it and goes on with no surface, which crashes; here the
		// image is left empty.
		host()->trace(("missing image " + theName).c_str());
		aSurface.mWidth = 0;
		aSurface.mHeight = 0;
		aSurface.mBytesPerPixel = 4;
		aSurface.mPitch = 0;
	}
	int aWidth = aSurface.mWidth;
	int aHeight = aSurface.mHeight;
	// The pixels as the 32-bit surface's bytes R, G, B, A
	std::vector<uint8_t> aBytes;
	if (aSurface.mBytesPerPixel == 4)
	{
		aBytes.resize((size_t)aWidth * aHeight * 4);
		for (int y = 0; y < aHeight; y++)
			std::memcpy(aBytes.data() + (size_t)y * aWidth * 4, aSurface.mPixels.data() + (size_t)y * aSurface.mPitch,
				(size_t)aWidth * 4);
	}
	else if (aSurface.mBytesPerPixel == 3)
	{
		// A 24-bit image becomes 32-bit, opaque; while a font is loading it becomes white with its green as the alpha
		aBytes.resize((size_t)aWidth * aHeight * 4);
		for (int y = 0; y < aHeight; y++)
		{
			const uint8_t* aSrc = aSurface.mPixels.data() + (size_t)y * aSurface.mPitch;
			uint8_t* aDst = aBytes.data() + (size_t)y * aWidth * 4;
			for (int x = 0; x < aWidth; x++)
			{
				if (!aWhite)
				{
					aDst[0] = aSrc[0];
					aDst[1] = aSrc[1];
					aDst[2] = aSrc[2];
					aDst[3] = 0xff;
				}
				else
				{
					aDst[0] = 0xff;
					aDst[1] = 0xff;
					aDst[2] = 0xff;
					aDst[3] = aSrc[1];
				}
				aSrc += 3;
				aDst += 4;
			}
		}
	}
	Str aMatte = graphicsDir(true);
	if (gMatteName.empty())
	{
		aMatte += theName;
		aMatte += "_Matte.png";
	}
	else
		aMatte += gMatteName;
	if (fileExists(aMatte))
	{
		HostSurface aMatteSurface;
		if (loadSurface(aMatte, ".PNG", aMatteSurface) &&
			(aMatteSurface.mBytesPerPixel == 4 || aMatteSurface.mBytesPerPixel == 3) &&
			aMatteSurface.mHeight == aHeight && aMatteSurface.mWidth == aWidth)
		{
			int aStep = aMatteSurface.mBytesPerPixel;
			int y = 0;
			if (0 < aMatteSurface.mHeight)
			{
				do
				{
					const uint8_t* aSrc = aMatteSurface.mPixels.data() + (size_t)aMatteSurface.mPitch * y + 1;
					uint8_t* aDst = aBytes.data() + (size_t)aWidth * 4 * y + 3;
					int x = aWidth;
					if (0 < x)
					{
						do
						{
							*aDst = (uint8_t)~*aSrc;
							aSrc += aStep;
							aDst += 4;
							x = x - 1;
						} while (x != 0);
					}
					y = y + 1;
				} while (y < aMatteSurface.mHeight);
			}
		}
	}
	// R and B swapped: the pixels become 0xAARRGGBB
	anImage->mSurface.resize((size_t)aWidth * aHeight);
	for (size_t i = 0; i < anImage->mSurface.size(); i++)
	{
		const uint8_t* p = aBytes.data() + i * 4;
		anImage->mSurface[i] = (uint32_t)p[3] << 24 | (uint32_t)p[0] << 16 | (uint32_t)p[1] << 8 | p[2];
	}
	anImage->mWidth = aWidth;
	anImage->mHeight = aHeight;
	anImage->mBits = 0x20;
	anImage->mPixels = anImage->mSurface.data();
	anImage->mPitch = aWidth * 4;

	Str anIms = graphicsDir(true);
	anIms += theName;
	anIms += "_ims.txt";
	int aHandle = fileOpen(anIms, false, false);
	if (aHandle == -1)
	{
		anImage->mFrameCount = 1;
		anImage->mFrames = new Frame[1];
		int i = 0;
		if (0 < anImage->mFrameCount)
		{
			do
			{
				anImage->mFrames[i].mX = 0;
				anImage->mFrames[i].mY = 0;
				anImage->mFrames[i].mWidth = aWidth;
				anImage->mFrames[i].mHeight = aHeight;
				anImage->mFrames[i].mOwner = anImage;
				i = i + 1;
			} while (i < anImage->mFrameCount);
		}
	}
	else
	{
		Str aText = fileReadText(aHandle, 100000);
		fileClose(aHandle);
		anImage->mFrameCount = 0;
		std::vector<Frame> aFrames(500);
		int aPos = 0;
		int aLen = (int)aText.size();
		int aCount = 0;
		if (0 < aLen)
		{
			while (aPos < aLen)
			{
				if (aPos < (int)aText.size() && (unsigned char)aText[aPos] <= 0x20)
				{
					aPos = aPos + 1;
					continue;
				}
				if (aLen <= aPos)
					break;
				char aChar = (char)at(aText, aPos);
				if (aPos < (int)aText.size() && aChar == '/')
				{
					bool anEnd = true;
					while (aPos < aLen)
					{
						if (aPos < (int)aText.size() && (aText[aPos] == '\r' || aText[aPos] == '\n'))
						{
							anEnd = false;
							break;
						}
						aPos = aPos + 1;
					}
					if (anEnd)
						break;
					continue;
				}
				int aNumbers = 0;
				int aNext = 0;
				if (aPos < (int)aText.size() && aChar == 'x')
				{
					if (equalsNoCase("xywh_ref", substr(aText, aPos, aPos + 8)))
					{
						aNumbers = 6;
						aNext = aPos + 8;
					}
					else if (equalsNoCase("xywh", substr(aText, aPos, aPos + 4)))
					{
						aNumbers = 4;
						aNext = aPos + 4;
					}
				}
				if (aNumbers == 0)
				{
					// As the original: the text is not passed, so a file with anything else never finishes loading
					host()->trace(("Unrecognized string in " + anIms).c_str());
					continue;
				}
				long aValues[6] = {0, 0, 0, 0, 0, 0};
				aPos = aNext;
				int k = 0;
				do
				{
					while (aPos < aLen && aPos < (int)aText.size() && (unsigned char)aText[aPos] <= 0x20)
						aPos = aPos + 1;
					int anEnd = aPos;
					if (aPos < aLen)
					{
						do
						{
							if (anEnd < (int)aText.size() && (unsigned char)aText[anEnd] <= 0x20)
								break;
							anEnd = anEnd + 1;
						} while (anEnd < aLen);
					}
					if (aPos < anEnd)
						aValues[k] = strToInt(substr(aText, aPos, anEnd));
					else
						aValues[k] = 0;
					k = k + 1;
					aPos = anEnd;
				} while (k < aNumbers);
				Frame& aFrame = aFrames[aCount];
				aFrame.mX = (int)aValues[0];
				aFrame.mY = (int)aValues[1];
				aFrame.mWidth = (int)aValues[2];
				aFrame.mHeight = (int)aValues[3];
				aFrame.mOwner = anImage;
				aCount = aCount + 1;
				anImage->mFrameCount = aCount;
			}
		}
		anImage->mFrames = new Frame[anImage->mFrameCount > 0 ? anImage->mFrameCount : 1];
		for (int i = 0; i < anImage->mFrameCount; i++)
			anImage->mFrames[i] = aFrames[i];
	}
	return anImage;
}

// 0x423c90
void Graphics::present(int theCount, const float* theRects)
{
	int16_t aRects[40];
	host()->getTicks();
	if (theCount != 0)
	{
		int i = 0;
		if (0 < theCount)
		{
			double aW = (double)gWidth;
			double aH = (double)gHeight;
			const float* r = theRects;
			do
			{
				aRects[i * 4] = (int16_t)ftol(aW * r[0]);
				aRects[i * 4 + 1] = (int16_t)ftol(aH * r[1]);
				aRects[i * 4 + 2] = (int16_t)ftol(aW * r[2]);
				aRects[i * 4 + 3] = (int16_t)ftol(aH * r[3]);
				i = i + 1;
				r = r + 4;
			} while (i < theCount);
		}
	}
	if (theCount < 1)
		host()->updateRects(0, nullptr);
	else
		host()->updateRects(theCount, aRects);
	gPixels = host()->getScreenPixels();
	uint32_t aNow = host()->getTicks();
	gFrameTimes[gFrame % 64] = aNow;
	gFrame = gFrame + 1;
	if ((host()->getAppState() & SDL_APPINPUTFOCUS) != 0)
		mFocusFrame = gFrame;
	float aRate = 80.0f;
	if (gFullScreen == 0)
		aRate = 30.0f;
	if (gPlatform->isIdle() != 0)
		aRate = 0.75f;
	int aLimit = gPlatform->mFrameLimit;
	if (aLimit != 0 && !((double)aRate < (double)aLimit))
		aRate = (float)aLimit;
	float aFrameTime = (float)(1000.0 / aRate);
	int aWait = ftol((double)aFrameTime - (double)(int)(host()->getTicks() - gLastPresent));
	bool aDo = true;
	if ((double)aRate < 60.0 && aWait <= 2)
		aWait = 2;
	else if (aWait < 10000)
	{
		if (aWait < 1)
			aDo = false;
	}
	else
		aWait = 10000;
	// 0x40e250, the game's say on waiting, always 1
	if (aDo)
		waitUntil((int)host()->getTicks() + aWait, 1);
	gLastPresent = host()->getTicks();
}

// 0x424000
void Graphics::setVideoMode(int theWidth, int theHeight, int theFullScreen, int theForce)
{
	host()->trace("SetVideoMode 0");
	if (gPlatform->mDesktopWidth < 0x321 || gPlatform->mDesktopHeight < 0x259)
		theFullScreen = 1;
	if (isLowColour() != 0)
		theFullScreen = 1;
	if (gHaveScreen && theWidth == gWidth && theHeight == gHeight && theFullScreen == gFullScreen && theForce == 0)
		return;
	mModeFrame = gFrame;
	mModeTime = host()->getTicks();
	host()->trace("SetVideoMode 1");
	host()->setFullScreen(theFullScreen != 0);
	gHaveScreen = true;
	host()->trace("SetVideoMode 2");
	mModeTime = host()->getTicks();
	if (theFullScreen == 0)
	{
		if (gPlatform == nullptr)
			moveWindowDefault();
		else
		{
			// slot 15: where the window goes, kept on the desktop
			int aX;
			int aY;
			gPlatform->getWindowPos(&aX, &aY);
			if (aX != 0 || aY != 0)
			{
				int aDesktopW;
				int aDesktopH;
				int aBits;
				getDesktop(&aDesktopW, &aDesktopH, &aBits);
				int aMaxX = aDesktopW - theWidth - 5;
				if (aMaxX <= aX)
					aX = aMaxX;
				int aMaxY = aDesktopH - 0x37 - theHeight;
				if (aMaxY <= aY)
					aY = aMaxY;
				moveWindow(aX & -(int)(aX >= 1), aY & -(int)(aY >= 1));
			}
		}
	}
	gPixels = host()->getScreenPixels();
	gWidth = theWidth;
	gHeight = theHeight;
	gPitch = theWidth * 4;
	gInvWidth = (float)(1.0 / (double)theWidth);
	gPitch4 = gPitch >> 2;
	gClipX1 = theWidth - 1;
	gClipY1 = theHeight - 1;
	gClipX0 = 0;
	gClipY0 = 0;
	gFullScreen = theFullScreen;
	gInvHeight = (float)(1.0 / (double)theHeight);
	fillRect(0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff);
	present(0, nullptr);
	fillRect(0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff);
	present(0, nullptr);
	updateWindowMenu();
}

// 0x424320
Graphics::Graphics(int theWidth, int theHeight, int theFullScreen)
{
	host()->trace("GM Init 1");
	buildShiftTables();
	mFloatTextFrame = 0;
	mModeTime = 0;
	mModeFrame = 0;
	mFocusFrame = 0;
	host()->trace("GM Init 2");
	host()->trace("GM Init 4");
	gArrowRows = exeStringArray(0x473ee8, 32);
	gHandRows = exeStringArray(0x473f68, 32);
	gArrowCursor = gArrowRows.data();
	gHandCursor = gHandRows.data();
	if (!gIconPath.empty())
		setIcon(gIconPath);
	host()->trace("GM Init 5");
	setVideoMode(theWidth, theHeight, theFullScreen, 0);
	if (!gCaption.empty())
		setCaption(gCaption);
}

// 0x424520
void fillRoundRect(float theX, float theY, float theW, float theH, uint32_t theColor, int theRadius)
{
	int x0 = ftol((double)theX * gWidth);
	int y0 = ftol((double)theY * gHeight);
	int x1 = ftol(((double)theX + theW) * gWidth);
	int y1 = ftol(((double)theY + theH) * gHeight);
	bool aClip = x0 < gClipX0 || y0 < gClipY0 || gClipX1 < x1 || gClipY1 < y1;
	uint32_t anAlpha = theColor >> 0x18;
	if (anAlpha == 0)
		return;
	uint32_t aGreen = theColor >> 8 & 0xff;
	uint32_t aBlue = theColor & 0xff;
	int r = theRadius - 2;
	int aLeft = x0 + r + 1;
	int aTop = y0 + r + 1;
	int aRight = x1 - r - 1;
	int aBottom = y1 - r - 1;
	uint32_t anInv = 0xff - anAlpha;
	float aLimit = (float)((double)r + (double)0.5f);
	for (int y = y0; y <= y1; y++)
	{
		for (int x = x0; x <= x1; x++)
		{
			if (aClip && (x < gClipX0 || y < gClipY0 || gClipX1 < x || gClipY1 < y))
				continue;
			double aDistance = 0.0;
			if (y <= aTop)
			{
				if (x <= aLeft)
					aDistance = distance((float)x, (float)y, (float)aLeft, (float)aTop);
				else if (aRight <= x)
					aDistance = distance((float)x, (float)y, (float)aRight, (float)aTop);
			}
			else if (aBottom <= y)
			{
				if (x <= aLeft)
					aDistance = distance((float)x, (float)y, (float)aLeft, (float)aBottom);
				else if (aRight <= x)
					aDistance = distance((float)x, (float)y, (float)aRight, (float)aBottom);
			}
			if (aDistance <= (double)aLimit)
			{
				uint32_t* p = (uint32_t*)((uint8_t*)gPixels + gPitch * y) + x;
				uint32_t c = *p;
				uint32_t aR = ((c >> 0x10 & 0xff) * anInv + (theColor >> 0x10 & 0xff) * anAlpha) << 8 & 0xffff00ff;
				aR = (aR | ((c >> 8 & 0xff) * anInv + aGreen * anAlpha)) & 0xffffff00;
				*p = aR | ((c & 0xff) * anInv + aBlue * anAlpha) >> 8;
			}
		}
	}
}

// 0x4247b0
void fillTriangle(float theX1, float theY1, float theX2, float theY2, float theX3, float theY3, uint32_t theColor)
{
	int x1 = ftol((double)theX1 * gWidth);
	int x2 = ftol((double)theX2 * gWidth);
	int x3 = ftol((double)gWidth * theX3);
	int y1 = ftol((double)theY1 * gHeight);
	int y2 = ftol((double)theY2 * gHeight);
	int y3 = ftol((double)gHeight * theY3);
	if (y2 < y1 && y2 < y3)
	{
		std::swap(y2, y1);
		std::swap(x2, x1);
	}
	else if (y3 < y1)
	{
		std::swap(y3, y1);
		std::swap(x3, x1);
	}
	if (x3 < x2)
	{
		std::swap(y3, y2);
		std::swap(x3, x2);
	}
	if (y1 == y2 && x1 < x2)
	{
		std::swap(y2, y1);
		std::swap(x2, x1);
	}
	if (y1 == y3 && x3 < x1)
	{
		std::swap(y3, y1);
		std::swap(x3, x1);
	}
	int aBottom = y2;
	if (y2 <= y3)
		aBottom = y3;
	if (aBottom == y1)
		return;
	int aStepA = 1;
	if (0 < y2 - y1)
		aStepA = y2 - y1;
	aStepA = ((x2 - x1) * 0x10000) / aStepA;
	int aStepB = 1;
	if (0 < y3 - y1)
		aStepB = y3 - y1;
	aStepB = ((x3 - x1) * 0x10000) / aStepB;
	uint32_t anAlpha = theColor >> 0x18;
	int anA = x1 << 0x10;
	int aB = anA;
	if (anAlpha == 0)
		return;
	uint32_t anInv = 0xff - anAlpha;
	bool aClip = x1 < gClipX0 + 1 || gClipX1 < x1 + 1 || y1 < gClipY0 + 1 || gClipY1 < y1 + 1 || x2 < gClipX0 + 1 ||
		gClipX1 < x2 + 1 || y2 < gClipY0 + 1 || gClipY1 < y2 + 1 || x3 < gClipX0 + 1 || gClipX1 < x3 + 1 ||
		y3 < gClipY0 + 1 || gClipY1 < y3 + 1;
	for (int y = y1; y <= aBottom; y++)
	{
		if (y == y2)
		{
			aStepA = 1;
			if (0 < y3 - y2)
				aStepA = y3 - y2;
			aStepA = ((x3 - x2) * 0x10000) / aStepA;
			anA = x2 << 0x10;
		}
		else
			anA = anA + aStepA;
		if (y == y3)
		{
			aStepB = 1;
			if (0 < y2 - y3)
				aStepB = y2 - y3;
			aStepB = ((x2 - x3) * 0x10000) / aStepB;
			aB = x3 << 0x10;
		}
		else
			aB = aB + aStepB;
		int aSpanLeft = anA >> 0x10;
		int aSpanRight = aB >> 0x10;
		if (aClip)
		{
			if (y < gClipY0 || gClipY1 < y)
				continue;
			if (aSpanLeft < gClipX0)
				aSpanLeft = gClipX0;
			if (gClipX1 <= aSpanRight)
				aSpanRight = gClipX1;
		}
		uint32_t* aPixelPtr = (uint32_t*)((uint8_t*)gPixels + gPitch * y) + aSpanLeft;
		if (anAlpha < 0xff)
		{
			if (aSpanLeft <= aSpanRight)
			{
				int n = (aSpanRight - aSpanLeft) + 1;
				do
				{
					uint32_t c = *aPixelPtr;
					*aPixelPtr = ((c >> 0x10 & 0xff) * anInv + (theColor >> 0x10 & 0xff) * anAlpha & 0xffff00) << 8 |
						((c >> 8 & 0xff) * anInv + (theColor >> 8 & 0xff) * anAlpha & 0xffffff00) |
						(c & 0xff) * anInv + (theColor & 0xff) * anAlpha >> 8;
					aPixelPtr = aPixelPtr + 1;
					n = n - 1;
				} while (n != 0);
			}
		}
		else if (aSpanLeft <= aSpanRight)
		{
			for (int n = (aSpanRight - aSpanLeft) + 1; n != 0; n--)
			{
				*aPixelPtr = theColor;
				aPixelPtr = aPixelPtr + 1;
			}
		}
	}
}

// 0x424c00 (0x41f5b0, a fill of 32-bit words, is inline in its opaque branch)
void fillRect(float theX, float theY, float theW, float theH, uint32_t theColor)
{
	double aW = (double)gWidth;
	int x0 = ftol((double)theX * aW + (double)0.01f);
	float aH = (float)gHeight;
	int y0 = ftol((double)aH * theY + (double)0.01f);
	int x1 = ftol(((double)theX + theW) * aW + (double)0.01f - 1.0);
	int y1 = ftol(((double)theY + theH) * aH + (double)0.01f - 1.0);
	if (x0 < gClipX0)
		x0 = gClipX0;
	if (y0 < gClipY0)
		y0 = gClipY0;
	if (gClipX1 < x1)
		x1 = gClipX1;
	if (gClipY1 < y1)
		y1 = gClipY1;
	if (x1 < x0 || y1 < y0)
		return;
	uint32_t a = theColor >> 0x18;
	if (a == 0)
		return;
	int aCount = x1 - x0;
	if ((int)a < 0xff)
	{
		uint32_t r = (theColor >> 0x10 & 0xff) * a;
		uint32_t g = (theColor >> 8 & 0xff) * a;
		uint32_t b = (theColor & 0xff) * a;
		uint32_t anInv = 0xff - a;
		aCount = aCount + 1;
		for (int y = y0; y <= y1; y++)
		{
			uint8_t* p = (uint8_t*)gPixels + gPitch * y + x0 * 4;
			for (int i = aCount; i != 0; i--)
			{
				uint32_t c = (p[2] * anInv + r) << 8 & 0xffff00ff;
				c = (c | (p[1] * anInv + g)) & 0xffffff00;
				uint32_t aB = ((*(uint32_t*)p & 0xff) * anInv + b) >> 8;
				*(uint32_t*)p = c | aB;
				p += 4;
			}
		}
		return;
	}
	aCount = aCount + 1;
	for (int y = y0; y <= y1; y++)
	{
		uint32_t* p = (uint32_t*)((uint8_t*)gPixels + gPitch * y) + x0;
		for (int i = 0; i < aCount; i++)
			p[i] = theColor;
	}
}

// 0x427a60
int screenWidth()
{
	return gWidth;
}

// 0x427a70
int screenHeight()
{
	return gHeight;
}

// 0x427a80
int isFullScreen()
{
	return gFullScreen;
}

// 0x427a90
double pixelWidth()
{
	return gInvWidth;
}

// 0x427aa0
double pixelHeight()
{
	return gInvHeight;
}

} // namespace bonnies
