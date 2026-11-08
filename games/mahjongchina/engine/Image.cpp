#include "Image.h"

#include "App.h"
#include "Blit.h"
#include "Display.h"
#include "Host.h"

#include "platform/ImageLoader.h"
#include "stb_image.h"

#include <cstring>
#include <string>
#include <vector>

namespace jb
{

uint8_t* gScreen = nullptr;
int gScreenPitch = 0;

// 0x5d8f38..0x5d8f44: the blit's temporary buffer, grown as needed
static uint8_t* gBlitBuffer = nullptr;
static int gBlitPitch = 0;
static int gBlitWidth = 0;
static int gBlitHeight = 0;

// 0x42ce90
void FreeBlitBuffer()
{
	if (gBlitBuffer != nullptr)
	{
		blit::Free(gBlitBuffer);
		gBlitBuffer = nullptr;
		gBlitWidth = 0;
		gBlitHeight = 0;
	}
}

// 0x42cec0: room for the image scaled (and 4 more pixels each way), cleared to 0 when theClear
static void EnsureBlitBuffer(int theWidth, int theHeight, int theScaleX, int theScaleY, bool theClear)
{
	int aWidth = MulDiv(theWidth, theScaleX, 100) + 4;
	int aHeight = MulDiv(theHeight, theScaleY, 100) + 4;
	if (gBlitWidth < aWidth || gBlitHeight < aHeight)
	{
		if (aWidth < 0x80)
			aWidth = 0x80;
		if (aHeight < 0x80)
			aHeight = 0x80;
		if (aWidth < gBlitWidth)
			aWidth = gBlitWidth;
		if (aHeight < gBlitHeight)
			aHeight = gBlitHeight;
		FreeBlitBuffer();
		gBlitWidth = aWidth;
		gBlitHeight = aHeight;
		gBlitBuffer = blit::Malloc32(aWidth, aHeight, &gBlitPitch);
	}
	if (theClear)
	{
		uint8_t aZero[4] = {0, 0, 0, 0};
		blit::Set4(aZero, gBlitBuffer, gBlitPitch, aWidth, aHeight);
	}
}

// 0x42ce00
Image::~Image()
{
	Release();
}

// 0x42cf80
void Image::Release()
{
	mPrepared = false;
	FreePixels();
	if (mOwnsBits && mBits != nullptr)
	{
		blit::Free(mBits);
		mBits = nullptr;
		mOwnsBits = false;
	}
	if (mDisplay != nullptr)
		mDisplay->Untrack(this);
}

// 0x42cfc0
bool Image::LoadResource(int theId, Module theModule)
{
	Release();
	uint32_t aSize = 0;
	int aType = 0;
	const uint8_t* aData = GetHost()->FindResource(theModule, nullptr, 2, theId & 0xffff, &aSize);
	if (aData == nullptr)
	{
		aData = GetHost()->FindResource(theModule, "JPG", 0, theId & 0xffff, &aSize);
		if (aData == nullptr)
		{
			aData = GetHost()->FindResource(theModule, "TGA", 0, theId & 0xffff, &aSize);
			if (aData == nullptr)
			{
				aData = GetHost()->FindResource(theModule, "PNG", 0, theId & 0xffff, &aSize);
				if (aData == nullptr)
					return false;
				aType = 3;
			}
			else
				aType = 2;
		}
		else
			aType = 1;
	}
	return Decode(aData, aSize, aType);
}

// 0x42d080
bool Image::Decode(const uint8_t* theData, uint32_t theSize, int theType)
{
	switch (theType)
	{
	case 0:
		return DecodeBmp(theData);
	case 1:
		if (theData[0] == 0xff && theData[1] == 0xd8)
			return DecodeJpeg(theData, theSize);
		break;
	case 2:
		return DecodeTga(theData, (int)theSize);
	case 3:
		if (theData[0] == 0x89 && theData[1] == 'P' && theData[2] == 'N' && theData[3] == 'G')
			return DecodePng(theData, theSize);
		break;
	}
	return false;
}

static int32_t ReadI32(const uint8_t* p)
{
	return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}

static uint16_t ReadU16(const uint8_t* p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

// 0x42d110: the pixels stay in the resource, after the header (biSize bytes; no palette)
bool Image::DecodeBmp(const uint8_t* theData)
{
	mBits = (uint8_t*)theData + ReadI32(theData);
	mWidth = ReadI32(theData + 4);
	mHeight = ReadI32(theData + 8);
	mBitCount = ReadU16(theData + 0xe);
	int aPitch = (mBitCount >> 3) * mWidth;
	int aRest = aPitch % 4;
	if (aRest != 0)
		aPitch = aPitch - aRest + 4;
	mPitch = aPitch;
	return true;
}

// 0x42d160: to 24 bits, B G R, the rows bottom-up. The original decodes with the exe's libjpeg (0x438000..,
// islow); libjpeg 6b, as the original does (Platform::LoadImageFromMemory).
bool Image::DecodeJpeg(const uint8_t* theData, uint32_t theSize)
{
	mBitCount = 0x18;
	int aWidth, aHeight, aComp;
	uint8_t* aRgb = Platform::LoadImageFromMemory(theData, (int)theSize, &aWidth, &aHeight, &aComp, 3);
	if (aRgb == nullptr)
		return false;
	mHeight = aHeight;
	mWidth = aWidth;
	AllocBits();
	int aLine = 0;
	if (aLine < mHeight)
	{
		do
		{
			const uint8_t* aSrc = aRgb + (size_t)aLine * aWidth * 3;
			aLine = aLine + 1;
			uint8_t* aDst = mBits + (size_t)(mHeight - aLine) * mPitch;
			int x = 0;
			if (0 < mWidth)
			{
				do
				{
					aDst[0] = aSrc[2];
					aDst[1] = aSrc[1];
					aDst[2] = aSrc[0];
					aDst = aDst + 3;
					x = x + 1;
					aSrc = aSrc + 3;
				} while (x < mWidth);
			}
		} while (aLine < mHeight);
	}
	stbi_image_free(aRgb);
	return true;
}

// 0x42d320: an uncompressed true-colour TGA (type 2) stays in the resource; the others are read as RLE. The rows
// are kept in the file's order (bottom-up for the game's).
bool Image::DecodeTga(const uint8_t* theData, int theSize)
{
	mWidth = ReadU16(theData + 0xc);
	mHeight = ReadU16(theData + 0xe);
	uint8_t aBits = theData[0x10];
	mBitCount = aBits;
	if (theData[2] == 2)
	{
		mBits = (uint8_t*)theData + theData[0] + 0x12;
		mPitch = (aBits >> 3) * mWidth;
	}
	AllocBits();
	uint8_t aState = 0xff;
	const uint8_t* aSrc = theData + theData[0] + 0x12;
	int aLeft = theSize - theData[0] - 0x12;
	int y = 0;
	if (mHeight != 0)
	{
		do
		{
			DecodeTgaRow(aSrc, mBits + (size_t)mPitch * y, mWidth, aState, aLeft);
			y = y + 1;
		} while (y < mHeight);
	}
	return aLeft == 0;
}

// 0x42d3f0
bool Image::DecodeTgaRow(const uint8_t*& theSrc, uint8_t* theDst, int theWidth, uint8_t& theState, int& theLeft)
{
	int aBytes = mBitCount / 8;
	int x = 0;
	if (0 < mWidth)
	{
		do
		{
			uint8_t aHeader = theState;
			if (aHeader == 0xff)
			{
				aHeader = *theSrc;
				theSrc = theSrc + 1;
				theLeft = theLeft - 1;
			}
			else
				theState = 0xff;
			int aCount;
			if ((aHeader & 0x80) == 0)
			{
				aCount = (uint8_t)(aHeader + 1);
				if (theWidth < aCount + x)
				{
					theState = (uint8_t)((x - theWidth) + aHeader + 1 - 1);
					aCount = (uint8_t)(theWidth - x);
				}
				int aLen = aCount * aBytes;
				theLeft = theLeft - aLen;
				if (theLeft < 0)
					return false;
				memcpy(theDst, theSrc, aLen);
				theDst = theDst + aLen;
				theSrc = theSrc + aLen;
			}
			else
			{
				aCount = (uint8_t)(aHeader + 0x81);
				if (theWidth < aCount + x)
				{
					theState = (uint8_t)((x - theWidth) + aHeader + 0x81 + 0x7f);
					aCount = (uint8_t)(theWidth - x);
				}
				if (aCount != 0)
				{
					int i = aCount;
					do
					{
						memcpy(theDst, theSrc, aBytes);
						theDst = theDst + aBytes;
						i = i - 1;
					} while (i != 0);
				}
				if (theState == 0xff)
				{
					theSrc = theSrc + aBytes;
					theLeft = theLeft - aBytes;
				}
			}
			x = x + aCount;
		} while (x < mWidth);
	}
	return true;
}

// 0x42d580: RGB8 to 24 bits, RGBA8 to 32 bits (other formats as 24-bit), B G R (A), the rows bottom-up. The
// original decodes with the exe's own PNG decoder (0x444f50); PNG is lossless, so stb_image gives the same pixels.
bool Image::DecodePng(const uint8_t* theData, uint32_t theSize)
{
	int aWidth, aHeight, aComp;
	if (!stbi_info_from_memory(theData, (int)theSize, &aWidth, &aHeight, &aComp))
		return false;
	mWidth = aWidth;
	mHeight = aHeight;
	mBitCount = 0x18;
	if (theSize > 0x19 && theData[0x18] == 8 && theData[0x19] == 6)
		mBitCount = 0x20;
	int aBytes = mBitCount / 8;
	uint8_t* aRgb = Platform::LoadImageFromMemory(theData, (int)theSize, &aWidth, &aHeight, &aComp, aBytes);
	if (aRgb == nullptr)
		return false;
	AllocBits();
	for (int y = 0; y < mHeight; y++)
	{
		const uint8_t* aSrc = aRgb + (size_t)y * aWidth * aBytes;
		uint8_t* aDst = mBits + (size_t)(mHeight - 1 - y) * mPitch;
		for (int x = 0; x < mWidth; x++)
		{
			aDst[0] = aSrc[2];
			aDst[1] = aSrc[1];
			aDst[2] = aSrc[0];
			if (aBytes == 4)
				aDst[3] = aSrc[3];
			aDst = aDst + aBytes;
			aSrc = aSrc + aBytes;
		}
	}
	stbi_image_free(aRgb);
	return true;
}

// 0x42d670
bool Image::LoadFile(const char* thePath)
{
	Release();
	size_t aLen = strlen(thePath);
	const char* anExt = aLen < 4 ? "" : thePath + aLen - 4;
	int aType;
	if (StrICmp(anExt, ".bmp") == 0)
		aType = 0;
	else if (StrICmp(anExt, ".jpg") == 0)
		aType = 1;
	else if (StrICmp(anExt, ".png") == 0)
		aType = 3;
	else if (StrICmp(anExt, ".tga") == 0)
		aType = 2;
	else
	{
		std::string aPath = thePath;
		if (LoadFile((aPath + ".bmp").c_str()))
			return true;
		if (LoadFile((aPath + ".jpg").c_str()))
			return true;
		if (LoadFile((aPath + ".png").c_str()))
			return true;
		return LoadFile((aPath + ".tga").c_str());
	}
	FILE* aFile = fopen(thePath, "rb");
	if (aFile == nullptr)
		return false;
	if (aType == 0)
	{
		bool aResult = LoadBmpFile(aFile);
		fclose(aFile);
		return aResult;
	}
	fseek(aFile, 0, SEEK_END);
	long aSize = ftell(aFile);
	fseek(aFile, 0, SEEK_SET);
	std::vector<uint8_t> aData((size_t)(aSize > 0 ? aSize : 0));
	bool aResult = false;
	if (aSize > 0 && fread(aData.data(), 1, (size_t)aSize, aFile) == (size_t)aSize)
		aResult = Decode(aData.data(), (uint32_t)aSize, aType);
	fclose(aFile);
	return aResult;
}

// 0x42d940: the file header and BITMAPINFOHEADER (0x3a bytes read), then the pixels at bfOffBits
bool Image::LoadBmpFile(FILE* theFile)
{
	uint8_t aHeader[0x3a];
	if (fread(aHeader, 1, sizeof(aHeader), theFile) != sizeof(aHeader))
		return false;
	if (ReadU16(aHeader) != 0x4d42)
		return false;
	if (fseek(theFile, ReadI32(aHeader + 10), SEEK_SET) != 0)
		return false;
	mHeight = ReadI32(aHeader + 0x16);
	mWidth = ReadI32(aHeader + 0x12);
	mBitCount = ReadU16(aHeader + 0x1c);
	AllocBits();
	int aPitch = (mBitCount * mWidth) / 8;
	int aRest = aPitch % 4;
	if (aRest != 0)
		aPitch = aPitch - aRest + 4;
	mPitch = aPitch;
	if (mBits == nullptr)
		return false;
	fread(mBits, 1, (size_t)mHeight * aPitch, theFile);
	return true;
}

// 0x42da20
void Image::AllocBits()
{
	if (mBitCount == 0x18)
		mBits = blit::Malloc24(mWidth, mHeight, &mPitch);
	else if (mBitCount == 0x20)
	{
		mBits = blit::Malloc32(mWidth, mHeight, &mPitch);
		mOwnsBits = true;
		return;
	}
	mOwnsBits = true;
}

// 0x42da70
void Image::SetColourKey(uint32_t theKey)
{
	mKeyed = true;
	mKey = ((theKey & 0xff) << 16) | (theKey & 0xff00) | ((theKey >> 16) & 0xff);
}

// 0x42daa0
void Image::GetInfo(ImageInfo& theInfo)
{
	theInfo.mWidth = mWidth;
	theInfo.mHeight = mHeight;
	theInfo.mBits = mBitCount;
	theInfo.mKeyed = mKeyed;
	theInfo.mKey = ((mKey & 0xff) << 16) | (mKey & 0xff00) | ((mKey >> 16) & 0xff);
}

// 0x42dae0 (the accelerated display's textured quads are not ported)
bool Image::Draw(int theX, int theY, const Rect& theSrc, int theScaleX, int theScaleY, int theAlpha)
{
	if (mWidth == 0 || mHeight == 0)
		return false;
	if (mFlipped)
		FlipRows();
	if (!EnsurePrepared())
		return false;
	return Blit(theX, theY, theSrc, theScaleX, theScaleY, theAlpha, gScreen, gScreenPitch);
}

// 0x42dcd0
bool Image::Blit(int theX, int theY, const Rect& theSrc, int theScaleX, int theScaleY, int theAlpha,
	uint8_t* theDst, int theDstPitch)
{
	int aDstOffsetY = theY * theDstPitch;
	uint8_t* aDst = theDst + theX * 4 + aDstOffsetY;
	int aWidth = theSrc.right - theSrc.left;
	int aHeight = theSrc.bottom - theSrc.top;
	const uint8_t* aSrc = mPixels + mPixelPitch * theSrc.top + theSrc.left * 4;
	int aSrcPitch = mPixelPitch;
	if (theScaleX != 100 || theScaleY != 100)
	{
		EnsureBlitBuffer(aWidth, aHeight, theScaleX, theScaleY, true);
		aSrc = gBlitBuffer;
		aSrcPitch = gBlitPitch;
		aWidth = MulDiv(aWidth, theScaleX, 100);
		aHeight = MulDiv(aHeight, theScaleY, 100);
		blit::ResizeNearest(mPixels, mWidth, mHeight, mPixelPitch, theSrc.left, theSrc.top, theSrc.right,
			theSrc.bottom, gBlitBuffer, gBlitPitch, aWidth, aHeight,
			(double)aWidth / (double)(theSrc.right - theSrc.left),
			(double)aHeight / (double)(theSrc.bottom - theSrc.top));
	}
	int aY = theY;
	bool aClip;
	if (theX < 0)
	{
		aSrc = aSrc - theX * 4;
		aDst = aDst - theX * 4;
		aWidth = aWidth + theX;
		theX = 0;
		aClip = true;
	}
	else
		aClip = theY < 0 || 800 < theX + aWidth || 600 < aHeight + theY;
	if (aClip)
	{
		if (theY < 0)
		{
			theY = 0;
			aSrc = aSrc - aSrcPitch * aY;
			aDst = aDst - aDstOffsetY;
			aHeight = aHeight + aY;
		}
		if (800 < theX + aWidth)
			aWidth = 800 - theX;
		if (600 < aHeight + theY)
			aHeight = 600 - theY;
		if (aWidth < 1 || aHeight < 1)
			return true;
	}
	// Port: the original's keyed copy (0x42e960) loops once more than it should for an empty unclipped rect;
	// nothing is drawn for one here
	if (aWidth < 1 || aHeight < 1)
		return true;
	if (mBitCount == 0x20)
	{
		if (theAlpha == 0xff)
		{
			blit::Blend(aSrc, aSrcPitch, aDst, theDstPitch, aDst, theDstPitch, aWidth, aHeight, 6);
			return true;
		}
	}
	else if (!mKeyed)
	{
		if (theAlpha != 0xff)
		{
			blit::AlphaCompC(aSrc, aSrcPitch, theAlpha, aDst, theDstPitch, 0xff, aDst, theDstPitch, aWidth,
				aHeight);
			return true;
		}
		blit::CopyRgb(aSrc, aSrcPitch, aDst, theDstPitch, aWidth, aHeight);
		return true;
	}
	else if (theAlpha == 0xff)
	{
		// 0x42e960, 0x438fc0: the pixels that are not the key, opaque
		int y = aHeight;
		do
		{
			const uint32_t* s = (const uint32_t*)aSrc;
			uint32_t* d = (uint32_t*)aDst;
			int n = aWidth;
			do
			{
				if ((*s & 0xff000000) != 0)
					*d = *s | 0xff000000;
				s = s + 1;
				d = d + 1;
				n = n - 1;
			} while (n != 0);
			aSrc = aSrc + aSrcPitch;
			aDst = aDst + theDstPitch;
			y = y - 1;
		} while (y != 0);
		return true;
	}
	// 32 bits or keyed, with alpha: the pixels times the alpha in the temporary buffer, then over
	if (theScaleX == 100 && theScaleY == 100)
		EnsureBlitBuffer(aWidth, aHeight, 100, 100, false);
	uint8_t anAlpha[4] = {(uint8_t)theAlpha, (uint8_t)theAlpha, (uint8_t)theAlpha, (uint8_t)theAlpha};
	blit::MulC(aSrc, aSrcPitch, anAlpha, gBlitBuffer, gBlitPitch, aWidth, aHeight);
	blit::Blend(gBlitBuffer, gBlitPitch, aDst, theDstPitch, aDst, theDstPitch, aWidth, aHeight, 6);
	return true;
}

// 0x42e3a0
bool Image::EnsurePrepared()
{
	if (!mPrepared)
		return Prepare();
	return true;
}

// 0x42e8b0
bool Image::Prepare()
{
	if (mBits == nullptr)
		return false;
	if (mPrepared && mPixels != nullptr)
		return true;
	mPrepared = false;
	mPixels = blit::Malloc32(mWidth, mHeight, &mPixelPitch);
	ConvertRows(mBits, mPitch, mPixels + (mHeight - 1) * mPixelPitch, mPixelPitch, 0x20, mWidth, mHeight);
	if (mBitCount == 0x20)
		blit::Premultiply(mPixels, mPixelPitch, mWidth, mHeight);
	mPrepared = true;
	return true;
}

// 0x42e940
void Image::FreePixels()
{
	mPrepared = false;
	if (mPixels != nullptr)
	{
		blit::Free(mPixels);
		mPixels = nullptr;
	}
}

// 0x42e9a0 (the 16-bit targets of the accelerated display are not ported)
void Image::ConvertRows(const uint8_t* theSrc, int theSrcPitch, uint8_t* theDst, int theDstPitch, int theDstBits,
	int theWidth, int theHeight)
{
	if (theDstBits != 0x20)
		return;
	if (mBitCount == 0x20)
	{
		do
		{
			// 0x438fa0
			memcpy(theDst, theSrc, (size_t)theWidth * 4);
			theSrc = theSrc + theSrcPitch;
			theDst = theDst - theDstPitch;
			theHeight = theHeight - 1;
		} while (theHeight != 0);
		return;
	}
	if (mBitCount == 0x18)
	{
		if (!mKeyed)
		{
			do
			{
				// 0x438ef0
				const uint8_t* s = theSrc;
				uint32_t* d = (uint32_t*)theDst;
				int n = theWidth;
				do
				{
					*d = (uint32_t)s[0] | ((uint32_t)s[1] << 8) | ((uint32_t)s[2] << 16) | 0xff000000;
					s = s + 3;
					d = d + 1;
					n = n - 1;
				} while (n != 0);
				theSrc = theSrc + theSrcPitch;
				theDst = theDst - theDstPitch;
				theHeight = theHeight - 1;
			} while (theHeight != 0);
			return;
		}
		do
		{
			// 0x438f20
			const uint8_t* s = theSrc;
			uint32_t* d = (uint32_t*)theDst;
			int n = theWidth;
			do
			{
				uint32_t aPixel = (uint32_t)s[0] | ((uint32_t)s[1] << 8) | ((uint32_t)s[2] << 16);
				if (aPixel == mKey)
					*d = 0;
				else
					*d = aPixel | 0xff000000;
				s = s + 3;
				d = d + 1;
				n = n - 1;
			} while (n != 0);
			theSrc = theSrc + theSrcPitch;
			theDst = theDst - theDstPitch;
			theHeight = theHeight - 1;
		} while (theHeight != 0);
	}
}

// 0x42eb60
void Image::Create(int theWidth, int theHeight, Display* theDisplay)
{
	mHeight = theHeight;
	mCreated = true;
	mWidth = theWidth;
	mBitCount = 0x20;
	AllocBits();
	mDisplay = theDisplay;
}

// 0x42eb90
void Image::BeginDrawing()
{
	FreePixels();
	FlipRows();
	mFlipped = true;
}

// 0x42ebb0
void Image::Fill(uint32_t theColour)
{
	FreePixels();
	uint8_t aValue[4] = {(uint8_t)(theColour >> 16), (uint8_t)(theColour >> 8), (uint8_t)theColour,
		(uint8_t)(theColour >> 24)};
	blit::Set4(aValue, mBits, mPitch, mWidth, mHeight);
	mFlipped = true;
}

// 0x42ec10
void Image::FlipRows()
{
	mFlipped = false;
	std::vector<uint8_t> aRow((size_t)mPitch);
	int y = 0;
	if (0 < mHeight / 2)
	{
		do
		{
			uint8_t* aTop = mBits + (size_t)mPitch * y;
			uint8_t* aBottom = mBits + (size_t)(mHeight - y - 1) * mPitch;
			memcpy(aRow.data(), aTop, mPitch);
			memcpy(aTop, aBottom, mPitch);
			memcpy(aBottom, aRow.data(), mPitch);
			y = y + 1;
		} while (y < mHeight / 2);
	}
}

// 0x42ecd0
bool Image::LoadDll(int theId)
{
	if (LoadResource(theId, Module::Dll))
	{
		Prepare();
		return true;
	}
	return false;
}

// 0x42ed00
bool Image::LoadDllKeyed(int theId, uint32_t theKey)
{
	SetColourKey(theKey);
	return LoadDll(theId);
}

// 0x42ed20
bool Image::DrawTo(Image* theTarget, int theX, int theY, const Rect& theSrc, int theScaleX, int theScaleY,
	int theAlpha)
{
	if (mWidth != 0 && mHeight != 0)
		return Blit(theX, theY, theSrc, theScaleX, theScaleY, theAlpha, theTarget->mBits, theTarget->mPitch);
	return false;
}

// 0x42ed70: B G R rows (0x438880 drops the alpha), bottom-up as the DIB is
bool Image::SaveBmp(const char* thePath)
{
	if (mFlipped)
		FlipRows();
	FILE* aFile = fopen(thePath, "wb");
	if (aFile == nullptr)
		return false;
	int aPitch = mWidth * 3;
	if (aPitch % 4 != 0)
		aPitch = aPitch + (4 - aPitch % 4);
	std::vector<uint8_t> aPixels((size_t)aPitch * mHeight, 0);
	for (int y = 0; y < mHeight; y++)
	{
		const uint8_t* s = mBits + (size_t)y * mPitch;
		uint8_t* d = aPixels.data() + (size_t)y * aPitch;
		for (int x = 0; x < mWidth; x++)
		{
			d[0] = s[0];
			d[1] = s[1];
			d[2] = s[2];
			s = s + mBitCount / 8;
			d = d + 3;
		}
	}
	uint8_t aHeader[0x36] = {};
	uint32_t aSize = (uint32_t)aPixels.size();
	auto Put32 = [&](int o, uint32_t v) {
		aHeader[o] = (uint8_t)v;
		aHeader[o + 1] = (uint8_t)(v >> 8);
		aHeader[o + 2] = (uint8_t)(v >> 16);
		aHeader[o + 3] = (uint8_t)(v >> 24);
	};
	aHeader[0] = 'B';
	aHeader[1] = 'M';
	Put32(2, aSize + 0x36);
	Put32(10, 0x36);
	Put32(14, 0x28);
	Put32(18, (uint32_t)mWidth);
	Put32(22, (uint32_t)mHeight);
	aHeader[26] = 1;
	aHeader[28] = 0x18;
	bool aResult = fwrite(aHeader, 1, 0x0e, aFile) == 0x0e && fwrite(aHeader + 0x0e, 1, 0x28, aFile) == 0x28 &&
		fwrite(aPixels.data(), 1, aSize, aFile) == aSize;
	fclose(aFile);
	return aResult;
}

// 0x42dff0 (software display)
void ScreenFillRect(const Rect* theRect, uint32_t theColour)
{
	Rect aScreen;
	SetRect(aScreen, 0, 0, 800, 600);
	Rect aClipped;
	const Rect* aRect = &aScreen;
	if (theRect != nullptr)
	{
		IntersectRect(aClipped, *theRect, aScreen);
		aRect = &aClipped;
	}
	uint8_t aValue[4] = {(uint8_t)(theColour >> 16), (uint8_t)(theColour >> 8), (uint8_t)theColour, 0};
	blit::SetRgb(aValue, gScreen + aRect->top * gScreenPitch + aRect->left * 4, gScreenPitch,
		aRect->right - aRect->left, aRect->bottom - aRect->top);
}

// 0x42e120 (software display)
void ScreenFillRectAlpha(Rect& theRect, int theAlpha, uint32_t theColour)
{
	if (theAlpha < 0)
		theAlpha = 0;
	else if (0xff < theAlpha)
		theAlpha = 0xff;
	uint32_t aColour = theColour & 0xffffff;
	Rect aScreen;
	SetRect(aScreen, 0, 0, 800, 600);
	IntersectRect(theRect, theRect, aScreen);
	uint8_t* aDst = gScreen + theRect.top * gScreenPitch + theRect.left * 4;
	int aWidth = theRect.right - theRect.left;
	int aHeight = theRect.bottom - theRect.top;
	uint8_t aRest = (uint8_t)(0xff - theAlpha);
	uint8_t aDarken[4] = {aRest, aRest, aRest, 0};
	blit::MulCRgb(aDst, gScreenPitch, aDarken, aDst, gScreenPitch, aWidth, aHeight);
	if (aColour != 0)
	{
		uint8_t anAdd[4] = {(uint8_t)MulDiv(aColour >> 16, theAlpha, 0xff),
			(uint8_t)MulDiv((aColour >> 8) & 0xff, theAlpha, 0xff), (uint8_t)MulDiv(theColour & 0xff, theAlpha, 0xff),
			0};
		blit::AddCRgb(anAdd, aDst, gScreenPitch, aWidth, aHeight);
	}
}

} // namespace jb
