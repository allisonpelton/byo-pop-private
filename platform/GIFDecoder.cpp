// The GIF decoder (see GIFDecoder.h). Each step follows stb_image 2.30's (third_party/stb/stb_image.h): stbi__gif_header,
// stbi__gif_load_next for the first frame, stbi__process_gif_raster and stbi__out_gif_code, with the same reads past the end
// (a zero byte), the same table of 8192 codes and the same refusals, which are left to stb. What differs is only how: a
// code's pixels are read off its chain into a buffer instead of by recursion, and they go straight into the 32-bit pixels
// instead of through stb's three frame buffers and a copy.

#include "GIFDecoder.h"

#include <climits>
#include <cstring>
#include <memory>

namespace Platform
{

namespace
{

// stb_image's reading from memory: a byte past the end reads as 0 and leaves the position; a skip may pass the end
struct Reader
{
	const uint8_t* mData;
	size_t mSize;
	size_t mPos;

	int Get8()
	{
		return (mPos < mSize) ? mData[mPos++] : 0;
	}
	int Get16()
	{
		int aLow = Get8();
		return aLow + (Get8() << 8);
	}
	void Skip(int theCount)
	{
		if (theCount < 0)
			mPos = mSize;
		else
			mPos += (size_t) theCount;
	}
};

struct Code
{
	int16_t mPrefix;   // the code whose pixels come before this one's last, -1 for a single pixel
	uint8_t mFirst;    // the first pixel's colour index
	uint8_t mSuffix;   // the last pixel's colour index
	uint16_t mLength;  // how many pixels (stb walks the prefixes instead)
	int32_t mPrefixAt; // where in the picture the prefix's pixels were written in one piece when this code was made, or -1
};

const int kCodeCount = 8192;  // stb's table (stbi__gif's codes[8192]); a stream that needs more fails there

struct Decoder
{
	Reader mReader;
	int mWidth = 0, mHeight = 0;                  // the logical screen
	int mFlags = 0, mBackground = 0;
	int mExtensionFlags = 0, mTransparent = -1;  // the Graphic Control Extension's, as stb keeps them
	uint8_t mPalette[256][4];                     // blue, green, red, alpha, as stb keeps them
	uint8_t mLocalPalette[256][4];
	const uint8_t (*mColors)[4] = nullptr;
	uint32_t mPixels[256];                        // mColors as the pixels stb writes (MakePixels)
	Code mCodes[kCodeCount];
	uint8_t mString[kCodeCount];
	uint32_t* mBits = nullptr;
	std::vector<uint8_t> mReached;                // pixels the frame reached (stb's history), kept only for the background fill
	// the frame's rectangle and the next pixel, in pixels and rows (stb's are in bytes), with the interlace's step and passes
	int mStartX = 0, mStartY = 0, mEndX = 0, mEndY = 0, mX = 0, mY = 0, mStep = 1, mPasses = 0;
	int32_t mLastAt = -1;                         // where the last code's pixels went in one piece, or -1 (PutCode)

	// The palettes start as stb's (zeros: an index past a palette's end is a transparent pixel). The codes need not: one is
	// read only after the raster's start or the code that makes it has set it, as in stb.
	Decoder()
	{
		memset(mPalette, 0, sizeof(mPalette));
		memset(mLocalPalette, 0, sizeof(mLocalPalette));
	}

	void ReadPalette(uint8_t thePalette[256][4], int theCount, int theTransparent)
	{
		for (int i = 0; i < theCount; i++)
		{
			thePalette[i][2] = (uint8_t) mReader.Get8();
			thePalette[i][1] = (uint8_t) mReader.Get8();
			thePalette[i][0] = (uint8_t) mReader.Get8();
			thePalette[i][3] = (theTransparent == i) ? 0 : 255;
		}
	}

	// The frame's colours as pixels. stb leaves a transparent pixel (alpha 128 or less: the palette's alphas are 0 or 255)
	// as it was, 0, and a first frame reaches no pixel twice, so writing 0 for one comes to the same.
	void MakePixels()
	{
		for (int i = 0; i < 256; i++)
		{
			const uint8_t* aColor = mColors[i];
			mPixels[i] = (aColor[3] > 128) ?
				((uint32_t) aColor[3] << 24) | ((uint32_t) aColor[2] << 16) | ((uint32_t) aColor[1] << 8) | aColor[0] : 0;
		}
	}

	// stb's step to the next pixel at the end of a row, with the interlace's passes
	void NextRow()
	{
		mX = mStartX;
		mY += mStep;
		while ((mY >= mEndY) && (mPasses > 0))
		{
			mStep = 1 << mPasses;
			mY = mStartY + (mStep >> 1);
			mPasses--;
		}
	}

	// stbi__out_gif_code for one pixel
	inline void Put(int theIndex)
	{
		if (mY >= mEndY)
			return;
		size_t aPos = (size_t) mY * mWidth + mX;
		if (!mReached.empty())
			mReached[aPos] = 1;
		mBits[aPos] = mPixels[theIndex];
		mX++;
		if (mX >= mEndX)
			NextRow();
	}

	// Writes a code's pixels. One that fits in the row goes straight in: its prefix's pixels copied from where they were
	// written when the code was made (a code is the code before it and one pixel more), else read off its chain. mLastAt is
	// where this code went, for the code the next one makes.
	void PutCode(int theCode)
	{
		const Code& aThis = mCodes[theCode];
		int aLength = aThis.mLength;
		if ((mY < mEndY) && (mX + aLength <= mEndX))
		{
			size_t aPos = (size_t) mY * mWidth + mX;
			uint32_t* anOut = mBits + aPos;
			if (aThis.mPrefixAt >= 0)
			{
				const uint32_t* aPrefix = mBits + aThis.mPrefixAt;
				for (int i = 0; i < aLength - 1; i++)
					anOut[i] = aPrefix[i];
				anOut[aLength - 1] = mPixels[aThis.mSuffix];
			}
			else
			{
				int aCode = theCode;
				for (int i = aLength - 1; i >= 0; i--)
				{
					anOut[i] = mPixels[mCodes[aCode].mSuffix];
					aCode = mCodes[aCode].mPrefix;
				}
			}
			if (!mReached.empty())
				memset(&mReached[aPos], 1, (size_t) aLength);
			mLastAt = (int32_t) aPos;
			mX += aLength;
			if (mX >= mEndX)
				NextRow();
			return;
		}
		mLastAt = -1;
		int aCode = theCode;
		for (int i = aLength - 1; i >= 0; i--)
		{
			mString[i] = mCodes[aCode].mSuffix;
			aCode = mCodes[aCode].mPrefix;
		}
		for (int i = 0; i < aLength; i++)
			Put(mString[i]);
	}

	// stbi__process_gif_raster
	bool ReadRaster()
	{
		int aMinSize = mReader.Get8();
		if (aMinSize > 12)
			return false;
		int aClear = 1 << aMinSize;
		bool isFirst = true;
		int aCodeSize = aMinSize + 1;
		int aCodeMask = (1 << aCodeSize) - 1;
		int32_t aBits = 0;
		int aValidBits = 0;
		for (int i = 0; i < aClear; i++)
		{
			mCodes[i].mPrefix = -1;
			mCodes[i].mFirst = (uint8_t) i;
			mCodes[i].mSuffix = (uint8_t) i;
			mCodes[i].mLength = 1;
			mCodes[i].mPrefixAt = -1;
		}
		int anAvail = aClear + 2;
		int anOldCode = -1;
		int aBlockLeft = 0;
		for (;;)
		{
			if (aValidBits < aCodeSize)
			{
				if (aBlockLeft == 0)
				{
					aBlockLeft = mReader.Get8();
					if (aBlockLeft == 0)
						return true;
				}
				aBlockLeft--;
				aBits |= (int32_t) mReader.Get8() << aValidBits;
				aValidBits += 8;
				continue;
			}
			int aCode = aBits & aCodeMask;
			aBits >>= aCodeSize;
			aValidBits -= aCodeSize;
			if (aCode == aClear)
			{
				aCodeSize = aMinSize + 1;
				aCodeMask = (1 << aCodeSize) - 1;
				anAvail = aClear + 2;
				anOldCode = -1;
				isFirst = false;
			}
			else if (aCode == aClear + 1)
			{
				mReader.Skip(aBlockLeft);
				while ((aBlockLeft = mReader.Get8()) > 0)
					mReader.Skip(aBlockLeft);
				return true;
			}
			else if (aCode <= anAvail)
			{
				if (isFirst)
					return false;  // stb: "no clear code"
				if (anOldCode >= 0)
				{
					Code& aNew = mCodes[anAvail++];
					if (anAvail > kCodeCount)
						return false;  // stb: "too many codes"
					aNew.mPrefix = (int16_t) anOldCode;
					aNew.mFirst = mCodes[anOldCode].mFirst;
					aNew.mSuffix = (aCode == anAvail) ? aNew.mFirst : mCodes[aCode].mFirst;
					aNew.mLength = (uint16_t) (mCodes[anOldCode].mLength + 1);
					aNew.mPrefixAt = mLastAt;  // the old code's pixels, the last ones written
				}
				else if (aCode == anAvail)
					return false;  // stb: "illegal code in raster"
				PutCode(aCode);
				if (((anAvail & aCodeMask) == 0) && (anAvail <= 0x0FFF))
				{
					aCodeSize++;
					aCodeMask = (1 << aCodeSize) - 1;
				}
				anOldCode = aCode;
			}
			else
				return false;  // stb: "illegal code in raster"
		}
	}

	// stbi__gif_header and stbi__gif_load_next's first frame
	bool Decode(std::vector<uint32_t>& theBits)
	{
		if ((mReader.Get8() != 'G') || (mReader.Get8() != 'I') || (mReader.Get8() != 'F') || (mReader.Get8() != '8'))
			return false;
		int aVersion = mReader.Get8();
		if (((aVersion != '7') && (aVersion != '9')) || (mReader.Get8() != 'a'))
			return false;
		mWidth = mReader.Get16();
		mHeight = mReader.Get16();
		mFlags = mReader.Get8();
		mBackground = mReader.Get8();
		mReader.Get8();  // the aspect ratio
		mTransparent = -1;
		// stb's check of the frame buffers' size; an empty screen is left to stb as well
		if ((mWidth == 0) || (mHeight == 0) || ((long long) 4 * mWidth * mHeight > INT_MAX))
			return false;
		if (mFlags & 0x80)
			ReadPalette(mPalette, 2 << (mFlags & 7), -1);
		theBits.assign((size_t) mWidth * mHeight, 0);
		mBits = &theBits[0];
		if (mBackground > 0)
			mReached.assign((size_t) mWidth * mHeight, 0);

		for (;;)
		{
			int aTag = mReader.Get8();
			if (aTag == 0x2C)  // an image descriptor
			{
				int aX = mReader.Get16(), aY = mReader.Get16(), aW = mReader.Get16(), aH = mReader.Get16();
				if ((aX + aW > mWidth) || (aY + aH > mHeight))
					return false;  // stb: "bad Image Descriptor"
				mStartX = aX;
				mStartY = aY;
				mEndX = aX + aW;
				mEndY = aY + aH;
				mX = mStartX;
				mY = (aW == 0) ? mEndY : mStartY;
				int aLocalFlags = mReader.Get8();
				if (aLocalFlags & 0x40)
				{
					mStep = 8;
					mPasses = 3;
				}
				else
				{
					mStep = 1;
					mPasses = 0;
				}
				if (aLocalFlags & 0x80)
				{
					ReadPalette(mLocalPalette, 2 << (aLocalFlags & 7), (mExtensionFlags & 0x01) ? mTransparent : -1);
					mColors = mLocalPalette;
				}
				else if (mFlags & 0x80)
					mColors = mPalette;
				else
					return false;  // stb: "missing color table"
				MakePixels();
				if (!ReadRaster())
					return false;
				if (mBackground > 0)
				{
					// stb copies its palette entry (blue, green, red) into a pixel it reads as red, green, blue
					mPalette[mBackground][3] = 255;
					const uint8_t* aColor = mPalette[mBackground];
					uint32_t aPixel = 0xFF000000u | ((uint32_t) aColor[0] << 16) | ((uint32_t) aColor[1] << 8) | aColor[2];
					for (size_t i = 0; i < theBits.size(); i++)
						if (mReached[i] == 0)
							theBits[i] = aPixel;
				}
				return true;
			}
			else if (aTag == 0x21)  // an extension
			{
				int aLength;
				if (mReader.Get8() == 0xF9)  // the Graphic Control Extension
				{
					aLength = mReader.Get8();
					if (aLength == 4)
					{
						mExtensionFlags = mReader.Get8();
						mReader.Get16();  // the delay
						if (mTransparent >= 0)
							mPalette[mTransparent][3] = 255;
						if (mExtensionFlags & 0x01)
						{
							mTransparent = mReader.Get8();
							mPalette[mTransparent][3] = 0;
						}
						else
						{
							mReader.Skip(1);
							mTransparent = -1;
						}
					}
					else
					{
						// stb skips the block and goes on to the next tag without reading the end of the sub-blocks
						mReader.Skip(aLength);
						continue;
					}
				}
				while ((aLength = mReader.Get8()) != 0)
					mReader.Skip(aLength);
			}
			else
				return false;  // the end of the stream before a frame (stb gives no picture), or stb's "unknown code"
		}
	}
};

}

bool DecodeGIF(const uint8_t* theData, size_t theSize, int& theWidth, int& theHeight, std::vector<uint32_t>& theBits, bool* theHasAlpha)
{
	// about 80 KB of tables: on the heap, not the stack
	std::unique_ptr<Decoder> aDecoder(new Decoder());
	aDecoder->mReader.mData = theData;
	aDecoder->mReader.mSize = theSize;
	aDecoder->mReader.mPos = 0;
	std::vector<uint32_t> aBits;
	if (!aDecoder->Decode(aBits))
		return false;
	theWidth = aDecoder->mWidth;
	theHeight = aDecoder->mHeight;
	if (theHasAlpha)
	{
		bool hasAlpha = false;
		for (uint32_t aPixel : aBits)
		{
			if ((aPixel >> 24) != 0xFF)
			{
				hasAlpha = true;
				break;
			}
		}
		*theHasAlpha = hasAlpha;
	}
	theBits.swap(aBits);
	return true;
}

}
