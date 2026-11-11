// Retro64's images (0x423980..0x425d60), pixel formats, palettes, the sine tables and the pixel helpers
// 0x42d540..0x42e6a0. The 24- and 32-bit destinations the blit wrappers also handle are not translated
// (R64Blit.h says why): a wrapper whose destination is not 16-bit draws nothing here.

#include "R64.h"
#include "R64Blit.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace r64
{

namespace
{

int gCosTable[361]; // 0x709a08
int gSinTable[361]; // 0x709fb0

void InitComponent(uint32_t theMask, uint32_t& theMaskOut, int& theBits, int& theShift, int& theLoss)
{
	theMaskOut = theMask;
	theBits = 0;
	theShift = 0;
	uint32_t aBits = theMask;
	if ((theMask & 1) == 0)
	{
		do
		{
			aBits = 0;
			if (theMask == 0)
				break;
			theShift++;
			theMask >>= 1;
			aBits = theMask;
		} while ((theMask & 1) == 0);
	}
	if ((aBits & 1) != 0)
	{
		int n = 0;
		do
		{
			aBits >>= 1;
			n++;
		} while ((aBits & 1) != 0);
		theBits = n;
	}
	theLoss = 8 - theBits;
}

inline bool Is16(const PixelFormat* theFmt)
{
	return theFmt->mBpp == 16;
}

inline int DegIndex(int theAngle)
{
	if (theAngle < 0)
		return 360 - (-theAngle) % 360;
	return theAngle % 360;
}

} // namespace

// 0x4012d0 (and 0x42e790). An 8-bit format gets the masks 0xff0000, 0xff00 and 0xff.
void PixelFormat_Init(PixelFormat* theFmt, int theBpp, uint32_t theRedMask, uint32_t theGreenMask, uint32_t theBlueMask)
{
	if (theBpp == 8)
	{
		theRedMask = 0xff0000;
		theGreenMask = 0xff00;
		theBlueMask = 0xff;
	}
	theFmt->mBpp = theBpp;
	InitComponent(theRedMask, theFmt->mRMask, theFmt->mRBits, theFmt->mRShift, theFmt->mRLoss);
	InitComponent(theGreenMask, theFmt->mGMask, theFmt->mGBits, theFmt->mGShift, theFmt->mGLoss);
	InitComponent(theBlueMask, theFmt->mBMask, theFmt->mBBits, theFmt->mBShift, theFmt->mBLoss);
}

// 0x4013d0
Palette::Palette()
{
	std::memset(mRgb, 0, sizeof(mRgb));
	mP16 = (uint16_t*)mConv;
	mP32 = (uint32_t*)mConv;
	std::memset(mConv, 0, sizeof(mConv));
}

// 0x42be80
void InitSinCos()
{
	int i = 0;
	do
	{
		double a = ((double)i * 3.14159) / 180.0;
		gCosTable[i] = (int)(std::cos(a) * 1024.0);
		gSinTable[i] = (int)(std::sin(a) * 1024.0);
		i++;
	} while (i < 0x169);
}

// 0x42bef0
float SinDeg(int theAngle)
{
	return (float)(gSinTable[DegIndex(theAngle)] * 0.0009765625);
}

// 0x42bf50
float CosDeg(int theAngle)
{
	return (float)(gCosTable[DegIndex(theAngle)] * 0.0009765625);
}

// 0x42bfb0
int ClipBlit(int theClipL, int theClipT, int theClipR, int theClipB, int x, int y, int w, int h, int* theOutX, int* theOutY,
	int* theOutSx, int* theOutSy, int* theOutW, int* theOutH)
{
	int aRight = x + -1 + w;
	int aBottom = y + -1 + h;
	if (x <= theClipR && theClipL <= aRight && y <= theClipB && theClipT <= aBottom)
	{
		*theOutSx = 0;
		*theOutSy = 0;
		*theOutW = w;
		*theOutH = h;
		if (x < theClipL)
		{
			*theOutSx = *theOutSx + (theClipL - x);
			*theOutW = *theOutW - *theOutSx;
			x = theClipL;
		}
		if (y < theClipT)
		{
			*theOutSy = *theOutSy + (theClipT - y);
			*theOutH = *theOutH - *theOutSy;
			y = theClipT;
		}
		if (theClipR < aRight)
			*theOutW = *theOutW + (theClipR - aRight);
		if (theClipB < aBottom)
			*theOutH = *theOutH + (theClipB - aBottom);
		*theOutX = x;
		*theOutY = y;
		if (0 < *theOutW && 0 < *theOutH)
			return 1;
	}
	return 0;
}

// 0x42c0f0 (each colour is truncated: the original's fistp runs with the rounding control set to chop, or 0xc00)
void Palette_Gradient(Palette* thePalette, int theFrom, int theTo, uint8_t theR0, uint8_t theG0, uint8_t theB0, uint8_t theR1,
	uint8_t theG1, uint8_t theB1)
{
	double aSteps = (double)std::abs(theTo - theFrom) + 1.0;
	double aRed = theR0;
	double aDr = (double)theR1 - aRed;
	double aGreen = theG0;
	double aDg = (double)theG1 - aGreen;
	bool anUp = theFrom < theTo;
	double aBlue = theB0;
	double aDb = (double)theB1 - aBlue;
	for (int i = 0; i < aSteps; i++)
	{
		thePalette->mRgb[theFrom * 4] = (uint8_t)(int)aRed;
		thePalette->mRgb[theFrom * 4 + 1] = (uint8_t)(int)aGreen;
		thePalette->mRgb[theFrom * 4 + 2] = (uint8_t)(int)aBlue;
		theFrom = theFrom + (anUp ? 1 : -1);
		aRed = aRed + aDr / aSteps;
		aGreen = aGreen + aDg / aSteps;
		aBlue = aBlue + aDb / aSteps;
	}
}

// 0x423980. The original leaves +0x3c and +0x40 alone; the port's are 0.
void Image_Init(Image* theImg)
{
	theImg->mBits = nullptr;
	theImg->mW = 0;
	theImg->mH = 0;
	theImg->mClip[0] = 0;
	theImg->mClip[1] = 0;
	theImg->mClip[2] = 0;
	theImg->mClip[3] = 0;
	theImg->mPitch = 0;
	theImg->mOrigW = 0;
	theImg->mOrigH = 0;
	theImg->mXOff = 0;
	theImg->mYOff = 0;
	theImg->mDataSize = 0;
	theImg->mHandleX = 0;
	theImg->mHandleY = 0;
	theImg->mPrev = nullptr;
	theImg->mNext = nullptr;
	PixelFormat_Init(&theImg->mFmt, 8, 0, 0, 0);
	theImg->mPalette = nullptr;
	theImg->mRawPalette = nullptr;
	theImg->mDefPalette = nullptr;
	std::memset(theImg->mName, 0, sizeof(theImg->mName));
	std::memcpy(theImg->mName, "None", 4);
	theImg->mHasMask = 0;
	theImg->mAsk = nullptr;
	theImg->mAskX = 0;
	theImg->mAskY = 0;
	theImg->mDefMask = nullptr;
	theImg->mRle = nullptr;
	theImg->mFileName = nullptr;
	theImg->mNotOwned = 0;
	theImg->mPaletteNumberPadding = 0;
	theImg->mPaletteNumber = 0;
	theImg->mIsMask = 0;
	theImg->mAskNamed = 0;
	theImg->mAutoMask = nullptr;
	theImg->mBankDataSize = 0;
	theImg->mBankPitch = 0;
	PixelFormat_Init(&theImg->mBankFmt, 8, 0, 0, 0);
}

// 0x423a50
void Image_SetMidHandle(Image* theImg)
{
	theImg->mHandleX = theImg->mOrigW / 2;
	theImg->mHandleY = theImg->mOrigH / 2;
}

// 0x423a70
int Image_DrawX(const Image* theImg)
{
	return theImg->mXOff - theImg->mHandleX;
}

// 0x423a80
int Image_DrawY(const Image* theImg)
{
	return theImg->mYOff - theImg->mHandleY;
}

// 0x423b00
void Image_SetPalette(Image* theImg, Palette* thePal)
{
	if (thePal == nullptr)
		thePal = theImg->mDefPalette;
	theImg->mPalette = thePal;
}

// 0x423b20. The old bits are freed whoever owns them, as in the original.
int Image_ConvertTo(Image* theImg, const PixelFormat* theFmt, const uint8_t* theRgb)
{
	PixelFormat aFmt = *theFmt;
	int aPitch = (aFmt.mBpp >> 3) * theImg->mW;
	int aRem = aPitch % 4;
	if (aRem != 0)
		aPitch = aPitch + (4 - aRem);
	uint8_t* aBits = (uint8_t*)std::malloc((size_t)theImg->mH * aPitch);
	if (aBits != nullptr)
	{
		if (ConvertPixels(&aFmt, aBits, aPitch, 0, 0, &theImg->mFmt, theRgb, theImg->mBits, theImg->mPitch, 0, 0, theImg->mW, theImg->mH) !=
			0)
		{
			std::free(theImg->mBits);
			theImg->mPitch = aPitch;
			theImg->mDataSize = theImg->mH * aPitch;
			theImg->mFmt = aFmt;
			theImg->mBits = aBits;
			return 1;
		}
		std::free(aBits);
	}
	return 0;
}

// 0x423bf0
void Image_FreeRle(Image* theImg)
{
	if (theImg->mRle != nullptr)
	{
		std::free(theImg->mRle);
		theImg->mRle = nullptr;
	}
}

// 0x425470: whether column x of the mask has a pixel that is not 0
int Image_MaskColumnSet(const Image* theImg, int x)
{
	const Image* aMask = theImg->mAsk;
	if (aMask == nullptr)
		return 0;
	int i = 0;
	if (0 < aMask->mH)
	{
		const uint8_t* p = aMask->mBits + x;
		do
		{
			if (*p != 0)
				return 1;
			i++;
			p += aMask->mPitch;
		} while (i < aMask->mH);
	}
	return 0;
}

// 0x4254c0: an image whose bits belong to a bank keeps everything
void Image_Free(Image* theImg)
{
	if (theImg->mNotOwned == 0)
	{
		if (theImg->mRle != nullptr)
		{
			std::free(theImg->mRle);
			theImg->mRle = nullptr;
		}
		if (theImg->mBits != nullptr)
			std::free(theImg->mBits);
		if (theImg->mFileName != nullptr)
			std::free(theImg->mFileName);
		Image_Init(theImg);
	}
}

// 0x425520. Image_Free resets the format to 8-bit first, so a null fmt makes an 8-bit image.
void Image_Create(Image* theImg, int theWidth, int theHeight, const PixelFormat* theFmt)
{
	if (theImg->mNotOwned == 0)
	{
		Image_Free(theImg);
		if (theFmt != nullptr)
			theImg->mFmt = *theFmt;
		theImg->mClip[2] = theWidth + -1;
		int aPitch = (theImg->mFmt.mBpp >> 3) * theWidth;
		theImg->mClip[3] = theHeight + -1;
		int aRem = aPitch % 4;
		theImg->mBits = nullptr;
		theImg->mW = theWidth;
		theImg->mH = theHeight;
		theImg->mClip[0] = 0;
		theImg->mClip[1] = 0;
		theImg->mPitch = aPitch;
		if (aRem != 0)
			theImg->mPitch = (aPitch - aRem) + 4;
		theImg->mOrigW = theWidth;
		theImg->mOrigH = theHeight;
		theImg->mXOff = 0;
		theImg->mYOff = 0;
		theImg->mBits = (uint8_t*)std::malloc((size_t)theImg->mPitch * theHeight);
		std::memset(theImg->mBits, 0, (size_t)theImg->mPitch * theImg->mH);
		theImg->mDataSize = theImg->mPitch * theImg->mH;
	}
}

// 0x4255d0
void Image_CreateCopy(Image* theImg, const uint8_t* theSrc, int theSrcPitch, int w, int h, const PixelFormat* theFmt)
{
	if (theImg->mNotOwned == 0)
	{
		Image_Create(theImg, w, h, theFmt);
		Copy8(theImg->mBits, 0, 0, theImg->mPitch, 0, theSrc, 0, 0, theSrcPitch, 0, (theImg->mFmt.mBpp >> 3) * w, h);
	}
}

// 0x425630
void Image_BuildRle(Image* theImg)
{
	if (theImg->mRle != nullptr)
	{
		std::free(theImg->mRle);
		theImg->mRle = nullptr;
	}
	if (theImg->mFmt.mBpp == 8)
		theImg->mRle = Rle_Build(theImg->mBits, theImg->mPitch, theImg->mH);
}

// 0x425d00
void Image_Destroy(Image* theImg)
{
	Image* anAutoMask = theImg->mAutoMask;
	if (anAutoMask != nullptr)
	{
		Image_Destroy(anAutoMask);
		delete anAutoMask;
		theImg->mAutoMask = nullptr;
	}
	if (theImg->mRle != nullptr)
	{
		std::free(theImg->mRle);
		theImg->mRle = nullptr;
	}
	if (theImg->mNotOwned == 0)
		Image_Free(theImg);
}

// 0x425d60
void Image_Copy(Image* theImg, const Image* theSrc)
{
	if (theImg->mNotOwned == 0)
		Image_CreateCopy(theImg, theSrc->mBits, theSrc->mPitch, theSrc->mW, theSrc->mH, &theSrc->mFmt);
}

// 0x423c20
void Image_Blit(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x, int y)
{
	int aDstX, aDstY, aSrcX, aSrcY, w, h;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], (theImg->mXOff - theImg->mHandleX) + x, (theImg->mYOff - theImg->mHandleY) + y,
			theImg->mW, theImg->mH, &aDstX, &aDstY, &aSrcX, &aSrcY, &w, &h) == 0)
		return;
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (0 < w && 0 < h)
	{
		if (theFmt->mBpp == 8)
		{
			if (theImg->mFmt.mBpp == 8)
				Copy8(theDst, aDstX, aDstY, theDstPitch, 0, theImg->mBits, aSrcX, aSrcY, theImg->mPitch, 0, w, h); // 0x4295f0
		}
		else if (theImg->mFmt.mBpp == 8)
		{
			if (theImg->mPalette != nullptr && Is16(theFmt)) // 0x429cb0
				Pal16(theDst, theDstPitch, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aSrcX, aSrcY, w, h);
		}
		else if (Is16(theFmt)) // 0x429680
			Copy16(theDst, theDstPitch, aDstX, aDstY, theImg->mBits, theImg->mPitch, aSrcX, aSrcY, w, h);
	}
}

// 0x423f50. The runs are used only when the whole image is drawn.
void Image_BlitMasked(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x, int y)
{
	int aDstX, aDstY, aSrcX, aSrcY, w, h;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], (theImg->mXOff - theImg->mHandleX) + x, (theImg->mYOff - theImg->mHandleY) + y,
			theImg->mW, theImg->mH, &aDstX, &aDstY, &aSrcX, &aSrcY, &w, &h) == 0)
		return;
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (!(0 < w && 0 < h))
		return;
	if (theFmt->mBpp == 8)
	{
		if (theImg->mFmt.mBpp == 8)
			Copy8Key(theDst, aDstX, aDstY, theDstPitch, 0, theImg->mBits, aSrcX, aSrcY, theImg->mPitch, 0, w, h); // 0x429630
	}
	else if (theImg->mHasMask == 1)
	{
		const uint8_t* aRle = theImg->mAsk->mRle;
		if (aRle == nullptr || theImg->mAskX != 0 || theImg->mAskY != 0 || w != theImg->mW || h != theImg->mH)
		{
			if (theImg->mFmt.mBpp != 8)
			{
				if (Is16(theFmt)) // 0x429750
					Copy16Mask(theDst, theDstPitch, aDstX, aDstY, theImg->mBits, theImg->mPitch, aSrcX, aSrcY, theImg->mAsk->mBits, theImg->mAsk->mPitch,
						theImg->mAskX, theImg->mAskY, w, h);
				return;
			}
			if (theImg->mPalette != nullptr)
			{
				if (Is16(theFmt)) // 0x429d90
					Pal16Mask(theDst, theDstPitch, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aSrcX, aSrcY, theImg->mAsk->mBits,
						theImg->mAsk->mPitch, theImg->mAskX, theImg->mAskY, w, h);
				return;
			}
		}
		else
		{
			if (theImg->mFmt.mBpp != 8)
			{
				if (Is16(theFmt)) // 0x429af0
					Rle16Copy(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, aRle);
				return;
			}
			if (theImg->mPalette != nullptr)
			{
				if (Is16(theFmt)) // 0x42a0d0
					RlePal16Copy(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aRle);
				return;
			}
		}
	}
	else if (theImg->mFmt.mBpp == 8 && theImg->mPalette != nullptr)
	{
		if (Is16(theFmt)) // 0x429d90
			Pal16Mask(theDst, theDstPitch, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aSrcX, aSrcY, theImg->mBits, theImg->mPitch, 0, 0,
				w, h);
	}
}

// 0x4241f0: the colour through the image's mask (an 8-bit image without one is its own)
void Image_BlitFill(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x, int y,
	uint32_t theColour)
{
	int aDstX, aDstY, aSrcX, aSrcY, w, h;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], (theImg->mXOff - theImg->mHandleX) + x, (theImg->mYOff - theImg->mHandleY) + y,
			theImg->mW, theImg->mH, &aDstX, &aDstY, &aSrcX, &aSrcY, &w, &h) == 0)
		return;
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (0 < w && 0 < h && theFmt->mBpp != 8)
	{
		const uint8_t* aMask;
		int aMaskPitch, aMaskX, aMaskY;
		if (theImg->mHasMask == 1)
		{
			const uint8_t* aRle = theImg->mAsk->mRle;
			if (aRle != nullptr && theImg->mAskX == 0 && theImg->mAskY == 0 && w == theImg->mW && h == theImg->mH)
			{
				if (Is16(theFmt)) // 0x429b30
					Rle16Fill(theDst, theDstPitch, theFmt, aDstX, aDstY, aRle, (uint16_t)theColour);
				return;
			}
			aMaskY = theImg->mAskY;
			aMaskX = theImg->mAskX;
			aMaskPitch = theImg->mAsk->mPitch;
			aMask = theImg->mAsk->mBits;
		}
		else
		{
			if (theImg->mFmt.mBpp != 8)
				return;
			aMask = theImg->mBits;
			aMaskPitch = theImg->mPitch;
			aMaskY = 0;
			aMaskX = 0;
		}
		if (Is16(theFmt)) // 0x429860
			Fill16Mask(theDst, theDstPitch, theFmt, aDstX, aDstY, aSrcX, aSrcY, aMask, aMaskPitch, aMaskX, aMaskY, w, h, (uint16_t)theColour);
	}
}

// 0x424360
void Image_BlitFillAlpha(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x, int y,
	uint32_t theColour, uint32_t theAlpha)
{
	int aDstX, aDstY, aSrcX, aSrcY, w, h;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], (theImg->mXOff - theImg->mHandleX) + x, (theImg->mYOff - theImg->mHandleY) + y,
			theImg->mW, theImg->mH, &aDstX, &aDstY, &aSrcX, &aSrcY, &w, &h) == 0)
		return;
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (0 < w && 0 < h && theFmt->mBpp != 8)
	{
		const uint8_t* aMask;
		int aMaskPitch, aMaskX, aMaskY;
		if (theImg->mHasMask == 1)
		{
			const uint8_t* aRle = theImg->mAsk->mRle;
			if (aRle != nullptr && theImg->mAskX == 0 && theImg->mAskY == 0 && w == theImg->mW && h == theImg->mH)
			{
				if (Is16(theFmt)) // 0x429b70
					Rle16FillAlpha(theDst, theDstPitch, theFmt, aDstX, aDstY, aRle, (uint16_t)theColour, theAlpha & 0xff);
				return;
			}
			aMaskY = theImg->mAskY;
			aMaskX = theImg->mAskX;
			aMaskPitch = theImg->mAsk->mPitch;
			aMask = theImg->mAsk->mBits;
		}
		else
		{
			if (theImg->mFmt.mBpp != 8)
				return;
			aMask = theImg->mBits;
			aMaskPitch = theImg->mPitch;
			aMaskY = 0;
			aMaskX = 0;
		}
		if (Is16(theFmt)) // 0x4298a0
			Fill16MaskAlpha(theDst, theDstPitch, theFmt, aDstX, aDstY, aSrcX, aSrcY, aMask, aMaskPitch, aMaskX, aMaskY, w, h, (uint16_t)theColour,
				(uint8_t)theAlpha);
	}
}

// 0x4244f0, 0x424700, 0x424cc0 share one shape: only images with a mask draw
namespace
{

enum MaskedKind
{
	BLEND,  // 0x4244f0
	ALPHA,  // 0x424700
	ADD     // 0x424cc0
};

void BlitMaskedKind(MaskedKind theKind, Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip,
	int x, int y)
{
	int aDstX, aDstY, aSrcX, aSrcY, w, h;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], (theImg->mXOff - theImg->mHandleX) + x, (theImg->mYOff - theImg->mHandleY) + y,
			theImg->mW, theImg->mH, &aDstX, &aDstY, &aSrcX, &aSrcY, &w, &h) == 0)
		return;
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (!(0 < w && 0 < h && theFmt->mBpp != 8 && theImg->mHasMask == 1))
		return;
	const uint8_t* aRle = theImg->mAsk->mRle;
	const uint8_t* aMaskPixel = theImg->mAsk->mBits;
	int aMp = theImg->mAsk->mPitch;
	if (aRle == nullptr || theImg->mAskX != 0 || theImg->mAskY != 0 || w != theImg->mW || h != theImg->mH)
	{
		if (theImg->mFmt.mBpp == 8)
		{
			if (theImg->mPalette != nullptr && Is16(theFmt))
			{
				if (theKind == BLEND) // 0x429eb0
					PalBlend16Mask(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aSrcX, aSrcY, aMaskPixel, aMp,
						theImg->mAskX, theImg->mAskY, w, h);
				else if (theKind == ALPHA) // 0x429ef0
					PalAlpha16Mask(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aSrcX, aSrcY, aMaskPixel, aMp,
						theImg->mAskX, theImg->mAskY, w, h);
				else // 0x429f70
					PalAdd16Mask(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aSrcX, aSrcY, aMaskPixel, aMp,
						theImg->mAskX, theImg->mAskY, w, h);
			}
		}
		else if (Is16(theFmt))
		{
			if (theKind == BLEND) // 0x4298e0
				Blend16Mask(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, aSrcX, aSrcY, aMaskPixel, aMp, theImg->mAskX, theImg->mAskY, w,
					h);
			else if (theKind == ALPHA) // 0x429920
				Alpha16Mask(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, aSrcX, aSrcY, aMaskPixel, aMp, theImg->mAskX, theImg->mAskY, w,
					h);
			else // 0x4299a0
				Add16Mask(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, aSrcX, aSrcY, aMaskPixel, aMp, theImg->mAskX, theImg->mAskY, w, h);
		}
	}
	else if (theImg->mFmt.mBpp != 8)
	{
		if (Is16(theFmt))
		{
			if (theKind == BLEND) // 0x429bb0
				Rle16Blend(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, aRle);
			else if (theKind == ALPHA) // 0x429bf0
				Rle16Alpha(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, aRle);
			else // 0x429c70
				Rle16Add(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, aRle);
		}
	}
	else if (theImg->mPalette != nullptr && Is16(theFmt))
	{
		if (theKind == BLEND) // 0x42a110
			RlePal16Blend(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aRle);
		else if (theKind == ALPHA) // 0x42a150
			RlePal16Alpha(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aRle);
		else // 0x42a1d0
			RlePal16Add(theDst, theDstPitch, theFmt, aDstX, aDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aRle);
	}
}

// 0x424a90's faded alpha blend, with the runs when the whole image is drawn (theWhole)
void BlitAlphaFadeAt(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, int theSrcX, int theSrcY,
	int theWidth, int theHeight, bool theWhole, uint32_t theAlpha)
{
	const uint8_t* aRle = theImg->mAsk->mRle;
	const uint8_t* aMaskPixel = theImg->mAsk->mBits;
	int aMp = theImg->mAsk->mPitch;
	if (aRle == nullptr || theImg->mAskX != 0 || theImg->mAskY != 0 || !theWhole)
	{
		if (theImg->mFmt.mBpp == 8)
		{
			if (theImg->mPalette != nullptr && Is16(theFmt)) // 0x429f30
				PalAlpha16MaskFade(theDst, theDstPitch, theFmt, theDstX, theDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, theSrcX, theSrcY, aMaskPixel, aMp,
					theImg->mAskX, theImg->mAskY, theWidth, theHeight, (int)theAlpha);
		}
		else if (Is16(theFmt)) // 0x429960
			Alpha16MaskFade(theDst, theDstPitch, theFmt, theDstX, theDstY, theImg->mBits, theImg->mPitch, theSrcX, theSrcY, aMaskPixel, aMp, theImg->mAskX, theImg->mAskY, theWidth,
				theHeight, (int)theAlpha);
	}
	else if (theImg->mFmt.mBpp != 8)
	{
		if (Is16(theFmt)) // 0x429c30
			Rle16AlphaFade(theDst, theDstPitch, theFmt, theDstX, theDstY, theImg->mBits, theImg->mPitch, aRle, theAlpha);
	}
	else if (theImg->mPalette != nullptr && Is16(theFmt)) // 0x42a190
		RlePal16AlphaFade(theDst, theDstPitch, theFmt, theDstX, theDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, aRle, theAlpha);
}

} // namespace

void Image_BlitBlend(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x, int y)
{
	BlitMaskedKind(BLEND, theImg, theDst, theDstPitch, theFmt, theClip, x, y);
}

void Image_BlitAlpha(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x, int y)
{
	BlitMaskedKind(ALPHA, theImg, theDst, theDstPitch, theFmt, theClip, x, y);
}

void Image_BlitAdd(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x, int y)
{
	BlitMaskedKind(ADD, theImg, theDst, theDstPitch, theFmt, theClip, x, y);
}

// 0x424a90
void Image_BlitAlphaFade(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x, int y,
	uint32_t theAlpha)
{
	int aDstX, aDstY, aSrcX, aSrcY, w, h;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], (theImg->mXOff - theImg->mHandleX) + x, (theImg->mYOff - theImg->mHandleY) + y,
			theImg->mW, theImg->mH, &aDstX, &aDstY, &aSrcX, &aSrcY, &w, &h) == 0)
		return;
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (0 < w && 0 < h && theFmt->mBpp != 8 && theImg->mHasMask == 1)
		BlitAlphaFadeAt(theImg, theDst, theDstPitch, theFmt, aDstX, aDstY, aSrcX, aSrcY, w, h, w == theImg->mW && h == theImg->mH, theAlpha);
}

// 0x425010: a part of the image, opaque and unclipped
void Image_BlitRect(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, int theSrcX, int theSrcY,
	int theWidth, int theHeight)
{
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (theFmt->mBpp == 8)
	{
		if (theImg->mFmt.mBpp == 8)
			Copy8(theDst, theDstX, theDstY, theDstPitch, 0, theImg->mBits, theSrcX, theSrcY, theImg->mPitch, 0, theWidth, theHeight); // 0x4295f0
	}
	else if (theImg->mFmt.mBpp == 8)
	{
		if (theImg->mPalette != nullptr && Is16(theFmt)) // 0x429cb0
			Pal16(theDst, theDstPitch, theDstX, theDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, theSrcX, theSrcY, theWidth, theHeight);
	}
	else if (Is16(theFmt)) // 0x429680
		Copy16(theDst, theDstPitch, theDstX, theDstY, theImg->mBits, theImg->mPitch, theSrcX, theSrcY, theWidth, theHeight);
}

// 0x4250f0: a part of the image through its mask, unclipped
void Image_BlitSubRect(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, int theSrcX, int theSrcY,
	int theWidth, int theHeight)
{
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (theFmt->mBpp == 8)
	{
		if (theImg->mFmt.mBpp == 8)
			Copy8Key(theDst, theDstX, theDstY, theDstPitch, 0, theImg->mBits, theSrcX, theSrcY, theImg->mPitch, 0, theWidth, theHeight); // 0x429630
	}
	else if (theImg->mHasMask == 1)
	{
		if (theImg->mFmt.mBpp != 8)
		{
			if (Is16(theFmt)) // 0x429750
				Copy16Mask(theDst, theDstPitch, theDstX, theDstY, theImg->mBits, theImg->mPitch, theSrcX, theSrcY, theImg->mAsk->mBits, theImg->mAsk->mPitch,
					theImg->mAskX, theImg->mAskY, theWidth, theHeight);
			return;
		}
		if (theImg->mPalette != nullptr && Is16(theFmt)) // 0x429d90
			Pal16Mask(theDst, theDstPitch, theDstX, theDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, theSrcX, theSrcY, theImg->mAsk->mBits,
				theImg->mAsk->mPitch, theImg->mAskX, theImg->mAskY, theWidth, theHeight);
	}
	else if (theImg->mFmt.mBpp == 8 && theImg->mPalette != nullptr && Is16(theFmt)) // 0x429d90
		Pal16Mask(theDst, theDstPitch, theDstX, theDstY, theImg->mBits, theImg->mPitch, theImg->mPalette, theSrcX, theSrcY, theImg->mBits, theImg->mPitch, 0, 0, theWidth, theHeight);
}

// 0x4252f0: rotated by angle degrees about (x, y), the image's middle
void Image_BlitRotated(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int x, int y, int theAngle,
	const int* theClip)
{
	if (theFmt == nullptr)
		theFmt = &theImg->mFmt;
	if (theFmt->mBpp == 8)
	{
		if (theImg->mFmt.mBpp == 8)
			Rotate8(theDst, theDstPitch, theImg->mBits, theImg->mPitch, theImg->mW, theImg->mH, x, y, theAngle, theClip);
	}
	else if (theImg->mHasMask == 1)
	{
		if (theImg->mFmt.mBpp != 8)
		{
			if (Is16(theFmt)) // 0x4299e0
				Rotate16(theDst, theDstPitch, theImg->mBits, theImg->mPitch, theImg->mW, theImg->mH, theImg->mAsk->mBits, theImg->mAsk->mPitch,
					theImg->mAskX, theImg->mAskY, x, y, theAngle, theClip);
			return;
		}
		if (theImg->mPalette != nullptr && Is16(theFmt)) // 0x429fb0
			PalRotate16(theDst, theDstPitch, theImg->mBits, theImg->mPitch, theImg->mPalette, theImg->mW, theImg->mH, theImg->mAsk->mBits,
				theImg->mAsk->mPitch, theImg->mAskX, theImg->mAskY, x, y, theAngle, theClip);
	}
	else if (theImg->mFmt.mBpp == 8 && theImg->mPalette != nullptr && Is16(theFmt)) // 0x429fb0
		PalRotate16(theDst, theDstPitch, theImg->mBits, theImg->mPitch, theImg->mPalette, theImg->mW, theImg->mH, theImg->mBits, theImg->mPitch, 0, 0, x, y,
			theAngle, theClip);
}

// 0x42d540
void Fill8(uint8_t* theDst, int theDstPitch, int l, int t, int r, int b, int theValue)
{
	uint8_t* d = theDst + theDstPitch * t + l;
	if (t <= b)
	{
		int n = (b - t) + 1;
		do
		{
			std::memset(d, theValue, (size_t)((r - l) + 1));
			d += theDstPitch;
			n--;
		} while (n != 0);
	}
}

// 0x42e020
void ExtractAlpha(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch, int theSrcX, int theSrcY, int theWidth,
	int theHeight)
{
	const uint8_t* aSrcPixel = theSrc + theSrcPitch * theSrcY + theSrcX * 4;
	uint8_t* aDstPixel = theDst + theDstPitch * theDstY + theDstX;
	if (0 < theHeight)
	{
		int aRows = theHeight;
		do
		{
			const uint8_t* aPs = aSrcPixel;
			uint8_t* aPd = aDstPixel;
			int n = theWidth;
			if (0 < theWidth)
			{
				do
				{
					*aPd = aPs[3];
					n--;
					aPs += 4;
					aPd++;
				} while (n != 0);
			}
			aSrcPixel += theSrcPitch;
			aDstPixel += theDstPitch;
			aRows--;
		} while (aRows != 0);
	}
}

// 0x42e0a0. The original passes the pixel through globals between a reader per source depth (0x42de00 8-bit,
// 0x42de40 16-bit, 0x42dea0 24/32-bit) and a writer per destination depth (0x42def0, 0x42df50, 0x42dfd0).
int ConvertPixels(const PixelFormat* theDstFmt, uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const PixelFormat* theSrcFmt,
	const uint8_t* theRgb, const uint8_t* theSrc, int theSrcPitch, int theSrcX, int theSrcY, int theWidth, int theHeight)
{
	uint32_t aSrcBpp = (uint32_t)theSrcFmt->mBpp;
	uint32_t aDstBpp = (uint32_t)theDstFmt->mBpp;
	if (aSrcBpp != 8 && aSrcBpp != 0x10 && aSrcBpp != 0x18 && aSrcBpp != 0x20)
		return 0;
	if (aDstBpp != 0x10 && aDstBpp != 0x18 && aDstBpp != 0x20)
		return 0;
	uint8_t aRed = 0, aGreen = 0, aBlue = 0;
	auto aRead = [&](const uint8_t* thePixel)
	{
		if (aSrcBpp == 8)
		{
			aRed = theRgb[*thePixel * 4];
			aGreen = theRgb[*thePixel * 4 + 1];
			aBlue = theRgb[*thePixel * 4 + 2];
			return;
		}
		uint32_t v;
		if (aSrcBpp == 0x10)
			v = *(const uint16_t*)thePixel;
		else
			std::memcpy(&v, thePixel, 4); // 24-bit reads a whole word too
		aRed = (uint8_t)(((theSrcFmt->mRMask & v) >> (theSrcFmt->mRShift & 0x1f)) << (theSrcFmt->mRLoss & 0x1f));
		aGreen = (uint8_t)(((theSrcFmt->mGMask & v) >> (theSrcFmt->mGShift & 0x1f)) << (theSrcFmt->mGLoss & 0x1f));
		aBlue = (uint8_t)(((theSrcFmt->mBMask & v) >> (theSrcFmt->mBShift & 0x1f)) << (theSrcFmt->mBLoss & 0x1f));
	};
	auto aWrite = [&](uint8_t* thePixel)
	{
		uint32_t aValue = (((uint32_t)aBlue >> (theDstFmt->mBLoss & 0x1f)) << (theDstFmt->mBShift & 0x1f)) |
			(((uint32_t)aGreen >> (theDstFmt->mGLoss & 0x1f)) << (theDstFmt->mGShift & 0x1f)) |
			(((uint32_t)aRed >> (theDstFmt->mRLoss & 0x1f)) << (theDstFmt->mRShift & 0x1f));
		if (aDstBpp == 0x10)
			*(uint16_t*)thePixel = (uint16_t)aValue;
		else if (aDstBpp == 0x18)
		{
			thePixel[0] = (uint8_t)aValue;
			thePixel[1] = (uint8_t)(aValue >> 8);
			thePixel[2] = (uint8_t)(aValue >> 0x10);
		}
		else
			std::memcpy(thePixel, &aValue, 4);
	};
	uint8_t* aDstPixel = theDst + theDstPitch * theDstY + (aDstBpp >> 3) * theDstX;
	const uint8_t* aSrcPixel = theSrc + theSrcPitch * theSrcY + (aSrcBpp >> 3) * theSrcX;
	if (0 < theHeight)
	{
		int aRows = theHeight;
		do
		{
			const uint8_t* aPs = aSrcPixel;
			uint8_t* aPd = aDstPixel;
			if (0 < theWidth)
			{
				int n = theWidth;
				do
				{
					aRead(aPs);
					aPs += aSrcBpp >> 3;
					aWrite(aPd);
					aPd += aDstBpp >> 3;
					n--;
				} while (n != 0);
			}
			aSrcPixel += theSrcPitch;
			aDstPixel += theDstPitch;
			aRows--;
		} while (aRows != 0);
	}
	return 1;
}

// 0x42e480. A first pass sizes the buffer, a second writes it; each row is a state machine over the pixels: 0
// counts skipped (0) pixels, 1 blended ones (stored), 2 solid (0xff) ones, and every change of state starts a new
// word-aligned count. A row ends with its runs completed by zero counts. Word alignment is by address in the
// original; malloc's buffers are aligned, so the port aligns by offset.
uint8_t* Rle_Build(const uint8_t* theBits, int theWidth, int theHeight)
{
	const uint8_t* aRow = theBits;
	uint32_t aSize = 0xc;
	if (0 < theHeight)
	{
		int aRows = theHeight;
		do
		{
			uint32_t aState = 0;
			aSize = aSize + 4;
			int i = 0;
			const uint8_t* aPixel = aRow;
			uint32_t aPrev = 0;
			if (theWidth < 1)
				aSize = aSize + (2 - aState) * 4;
			else
			{
				do
				{
					while (true)
					{
						if (aState == 0)
						{
							if (*aPixel != 0)
								aState = 1;
						}
						else if (aState == 1)
						{
							if (*aPixel == 0 || *aPixel == 0xff)
								aState = 2;
							else
								aSize = aSize + 1;
						}
						else if (aState == 2 && *aPixel != 0xff)
							aState = 0;
						if (theWidth <= i || aState == aPrev)
							break;
						if ((aSize & 3) != 0)
							aSize = aSize + (4 - (aSize & 3));
						aSize = aSize + 4;
						aPrev = aState;
					}
					i++;
					aPixel++;
				} while (i < theWidth);
				if (aState < 2)
					aSize = aSize + (2 - aState) * 4;
			}
			aRow = aRow + theWidth;
			aRows--;
		} while (aRows != 0);
	}
	uint32_t aBufSize = aSize + 4;
	uint8_t* aBuf = (uint8_t*)std::malloc(aBufSize);
	if (aBuf == nullptr)
		return nullptr;
	std::memset(aBuf, 0, aBufSize);
	RleHeader* aHeader = (RleHeader*)aBuf;
	aHeader->mSize = aBufSize;
	aHeader->mWidth = (uint32_t)theWidth;
	aHeader->mHeight = (uint32_t)theHeight;
	auto anAlign = [&](uint32_t theAt) { return (theAt & 3) != 0 ? theAt + (4 - (theAt & 3)) : theAt; };
	auto aWord = [&](uint32_t theAt) -> uint32_t& { return *(uint32_t*)(aBuf + theAt); };
	uint32_t anAt = 0xc;
	aRow = theBits;
	if (0 < theHeight)
	{
		int aRows = theHeight;
		do
		{
			uint32_t aCount = anAlign(anAt);
			aWord(aCount) = 0;
			anAt = aCount + 4;
			uint32_t aState = 0;
			int i = 0;
			uint32_t aPrev = 0;
			const uint8_t* aPixel = aRow;
			if (theWidth < 1)
			{
				int n = 2 - (int)aState;
				do
				{
					anAt = anAlign(anAt);
					aWord(anAt) = 0;
					anAt += 4;
					n--;
				} while (n != 0);
			}
			else
			{
				do
				{
					while (true)
					{
						if (aState == 0)
						{
							if (*aPixel == 0)
								aWord(aCount) = aWord(aCount) + 1;
							else
								aState = 1;
						}
						else if (aState == 1)
						{
							if (*aPixel == 0 || *aPixel == 0xff)
								aState = 2;
							else
							{
								aWord(aCount) = aWord(aCount) + 1;
								aBuf[anAt] = *aPixel;
								anAt++;
							}
						}
						else if (aState == 2)
						{
							if (*aPixel == 0xff)
								aWord(aCount) = aWord(aCount) + 1;
							else
								aState = 0;
						}
						if (theWidth <= i || aState == aPrev)
							break;
						aCount = anAlign(anAt);
						aWord(aCount) = 0;
						anAt = aCount + 4;
						aPrev = aState;
					}
					i++;
					aPixel++;
				} while (i < theWidth);
				if (aState < 2)
				{
					int n = 2 - (int)aState;
					do
					{
						anAt = anAlign(anAt);
						aWord(anAt) = 0;
						anAt += 4;
						n--;
					} while (n != 0);
				}
			}
			aRow = aRow + theWidth;
			aRows--;
		} while (aRows != 0);
	}
	return aBuf;
}

} // namespace r64
