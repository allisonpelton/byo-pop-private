#include "Render.h"
#include "platform/ImageLoader.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <type_traits>

using namespace engine;
using namespace wordharmony;

namespace
{

// A PNG mask whose pixels end in alpha once the original's reader has set it up: grey with alpha, RGBA, or a palette
// with a tRNS chunk (colour type at byte 25, in IHDR)
bool maskHasAlpha(const std::vector<uint8_t>& theMask)
{
	if (theMask.size() < 33 || std::memcmp(theMask.data() + 1, "PNG", 3) != 0)
		return true;
	int aType = theMask[25];
	if (aType == 4 || aType == 6)
		return true;
	if (aType != 3)
		return false;
	size_t aPos = 8;
	while (aPos + 8 <= theMask.size())
	{
		const uint8_t* aChunk = theMask.data() + aPos;
		size_t aLength = ((size_t)aChunk[0] << 24) | (aChunk[1] << 16) | (aChunk[2] << 8) | aChunk[3];
		if (std::memcmp(aChunk + 4, "tRNS", 4) == 0)
			return true;
		if (std::memcmp(aChunk + 4, "IDAT", 4) == 0)
			return false;
		aPos += 12 + aLength;
	}
	return false;
}

}

// 0x45e760, 0x45ea10: the image read from a PNG and its mask (the platform decodes both; the mask is read below)
ScreenImage* wordharmony::decodeImage(const std::vector<uint8_t>& theData, const std::vector<uint8_t>* theMask)
{
	int aWidth = 0;
	int aHeight = 0;
	std::vector<uint32_t> aBits;
	bool aHasAlpha = false;
	if (!Platform::DecodeImage(theData, aWidth, aHeight, aBits, &aHasAlpha) || aWidth <= 0 || aHeight <= 0)
		return nullptr;
	ScreenImage* anImage = new ScreenImage(aWidth, aHeight);
	anImage->mBits.swap(aBits);
	if (theMask != nullptr)
	{
		// (0x45ea10, 0x4651c0) the last channel of each of the mask's pixels is the alpha. The reader makes an RGB mask
		// and a palette one without transparency grey (png_set_rgb_to_gray, libpng's default weights) and expands a
		// palette with it to RGBA, so a mask with alpha gives its alpha and any other its grey level.
		int aMaskWidth = 0;
		int aMaskHeight = 0;
		std::vector<uint32_t> aMaskBits;
		if (Platform::DecodeImage(*theMask, aMaskWidth, aMaskHeight, aMaskBits, nullptr))
		{
			bool aMaskAlpha = maskHasAlpha(*theMask);
			for (int y = 0; y < aHeight && y < aMaskHeight; y++)
			{
				for (int x = 0; x < aWidth && x < aMaskWidth; x++)
				{
					uint32_t aMask = aMaskBits[y * aMaskWidth + x];
					uint32_t anAlpha;
					if (aMaskAlpha)
						anAlpha = aMask >> 24;
					else
					{
						uint32_t r = aMask >> 16 & 0xff;
						uint32_t g = aMask >> 8 & 0xff;
						uint32_t b = aMask & 0xff;
						anAlpha = (r == g && r == b) ? r : (6968 * r + 23434 * g + 2366 * b) >> 15;
					}
					uint32_t& aPixel = anImage->mBits[y * aWidth + x];
					aPixel = (aPixel & 0x00ffffff) | (anAlpha << 24);
				}
			}
		}
		anImage->setAlphaMode(ALPHA_BLEND, 0);
	}
	else if (aHasAlpha)
		anImage->setAlphaMode(ALPHA_TEST, 0);
	else
		anImage->setAlphaMode(ALPHA_NONE, 0);
	return anImage;
}

namespace
{

// port (for speed): the pixel functions are inlined into the loops of the blit they are called from, where they were
// a call and a switch a pixel
#if defined(__GNUC__) || defined(__clang__)
#define BLIT_INLINE inline __attribute__((always_inline))
#else
#define BLIT_INLINE inline
#endif

// Each channel is (source * alpha + destination * (255 - alpha)) / 255. Port (for speed): red and blue are worked out
// together in the two 16-bit lanes of a word, and a sum x of at most 65025 divided by 255 as (x + 1 + (x >> 8)) >> 8, which
// is the same for every x up to 65534 (checked against the division for every alpha, every pair of channel values and 400
// million random pixels)
BLIT_INLINE uint32_t blend(uint32_t theDest, uint32_t theRGB, int theAlpha)
{
	if (theAlpha >= 255)
		return 0xff000000 | (theRGB & 0x00ffffff);
	if (theAlpha <= 0)
		return theDest;
	uint32_t anAlpha = (uint32_t)theAlpha;
	uint32_t anInverse = 255 - anAlpha;
	uint32_t rb = (theRGB & 0x00ff00ff) * anAlpha + (theDest & 0x00ff00ff) * anInverse;
	rb = ((rb + 0x00010001 + ((rb >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;
	uint32_t g = ((theRGB >> 8) & 0xff) * anAlpha + ((theDest >> 8) & 0xff) * anInverse;
	g = (g + 1 + (g >> 8)) >> 8;
	return 0xff000000 | rb | (g << 8);
}

inline int clampByte(int theValue)
{
	return theValue < 0 ? 0 : (theValue > 255 ? 255 : theValue);
}

// The pixel operations of the four blitters
enum
{
	OP_COPY,    // 0x45c070: opaque
	OP_TEST,    // 0x45c780: the pixels above the threshold (blended by their alpha in the port: the original's
	            // filtered copy is not reproduced)
	OP_BLEND,   // 0x45ce90: alpha times the opacity
	OP_OFFSET   // 0x45d5a0: the colour offset added, then blended
};

struct PixelOp
{
	int mOp;
	int mThreshold;
	int mOpacity;  // 0-255
	int mOffsetR, mOffsetG, mOffsetB;
};

// 0x45c780 and 0x45ce90: the pixel loops of the alpha-test and blend blitters, with the scanline helpers they share
// (0x4579c0, 0x4581c0, 0x458230, 0x458560, 0x458cb0). The operation is a template parameter (port, for speed: the
// blit picks it once and its pixel loops have no switch); a pixel the blend would leave as it is is not written.
template <int OP>
BLIT_INLINE void applyPixel(const PixelOp& theOp, uint32_t& theDest, uint32_t theSrc)
{
	int anAlpha = (int)(theSrc >> 24);
	if (OP == OP_COPY)
		theDest = 0xff000000 | (theSrc & 0x00ffffff);
	else if (OP == OP_TEST)
	{
		if (anAlpha > theOp.mThreshold)
			theDest = blend(theDest, theSrc, anAlpha);
	}
	else if (OP == OP_BLEND)
	{
		int aBlendAlpha = anAlpha * theOp.mOpacity / 255;
		if (aBlendAlpha > 0)
			theDest = blend(theDest, theSrc, aBlendAlpha);
	}
	else
	{
		int r = clampByte((int)((theSrc >> 16) & 0xff) + theOp.mOffsetR);
		int g = clampByte((int)((theSrc >> 8) & 0xff) + theOp.mOffsetG);
		int b = clampByte((int)(theSrc & 0xff) + theOp.mOffsetB);
		int aBlendAlpha = anAlpha * theOp.mOpacity / 255;
		if (aBlendAlpha > 0)
			theDest = blend(theDest, (uint32_t)((r << 16) | (g << 8) | b), aBlendAlpha);
	}
}

// Calls theBody with the operation as a std::integral_constant, so that the body is compiled once for each
template <class Body>
void withOp(int theOp, Body&& theBody)
{
	switch (theOp)
	{
	case OP_COPY:
		theBody(std::integral_constant<int, OP_COPY>());
		break;
	case OP_TEST:
		theBody(std::integral_constant<int, OP_TEST>());
		break;
	case OP_BLEND:
		theBody(std::integral_constant<int, OP_BLEND>());
		break;
	default:
		theBody(std::integral_constant<int, OP_OFFSET>());
		break;
	}
}

}

// port (for speed): retained drawing. A frame is a list of draw calls, and the value of a pixel depends only on the calls
// that cover it, in order, on the black the screen starts the frame as, and on the images' pixels (which never change once
// decoded). So the calls are recorded as they come (drawImage and drawLine only record), and at the frame's end (finish)
// the screen is taken in tiles: a tile whose calls are the same as the last frame's keeps its pixels, and every other tile is
// made black and has its calls drawn again, clipped to the tile. The pixels are those the original's drawing gives, call
// for call: each pixel is worked out by the same code from the same inputs. Two calls are the same when they would write the
// same pixels, not only when their numbers are the same: a position that moved by a fraction of a pixel, in a call that
// reads the same source column and row for each pixel as before, is the same call (the columns and rows are worked out once
// for the call, as the original's loops work them out for each of its rows).
namespace
{

const int TILE = 32;  // the tiles are TILE x TILE pixels (the last row and column of them are cut by the screen's edge)

enum
{
	BLIT_IMAGE,
	BLIT_LINE
};

// Everything that decides the pixels one draw call writes (a record compared with the last frame's, so the members are
// all 32 bits wide, with no padding between them). A line keeps its ends in mSrc*, its colour in mThreshold and its alpha in
// mOpacity.
struct Blit
{
	int32_t mKind;
	uint32_t mImage;  // the source image's serial number (ScreenImage::mSerial)
	int32_t mSrcLeft, mSrcTop, mSrcRight, mSrcBottom;
	float mX, mY;  // the position with the state's translation, snapped as the state says
	float mRotation, mScaleX, mScaleY;
	int32_t mPivotX, mPivotY;
	int32_t mOp, mThreshold, mOpacity, mOffsetR, mOffsetG, mOffsetB;
	int32_t mLeft, mTop, mRight, mBottom;  // the pixels the call can write (a part of the screen)
};

static_assert(sizeof(Blit) == 23 * 4, "Blit is compared as bytes, so it has no padding");

// What an unrotated image call reads for each pixel it can write, worked out once for the call: the source column of each
// destination column (mLeft to mRight) and the source row of each destination row (mTop to mBottom), -1 where the call
// leaves the pixel. The numbers are in the frame's mMapData.
struct BlitMaps
{
	int32_t mColumns;  // where the columns start in the map data, or -1 for a call without maps (rotated, or a line)
	int32_t mRows;
	int32_t mFirst;    // the first and last destination column (counted from mLeft) that read the source
	int32_t mLast;
	bool mRun;         // the columns that read the source follow one another (an unscaled, unmirrored call): rows are runs
};

// The calls of a frame
struct Frame
{
	std::vector<Blit> mBlits;
	std::vector<const ScreenImage*> mImages;  // the image of each call, alive for this frame only (the next one compares serial numbers)
	std::vector<BlitMaps> mMaps;
	std::vector<int> mMapData;

	void clear()
	{
		mBlits.clear();
		mImages.clear();
		mMaps.clear();
		mMapData.clear();
	}
};

PixelOp blitOp(const Blit& theBlit)
{
	PixelOp anOp;
	anOp.mOp = theBlit.mOp;
	anOp.mThreshold = theBlit.mThreshold;
	anOp.mOpacity = theBlit.mOpacity;
	anOp.mOffsetR = theBlit.mOffsetR;
	anOp.mOffsetG = theBlit.mOffsetG;
	anOp.mOffsetB = theBlit.mOffsetB;
	return anOp;
}

// The part of the screen an image draw call can write; false when it writes nothing. The rectangles are those the pixel loops of
// renderBlit run over (the position, scale and rotation as the loops take them).
bool blitBounds(Blit& theBlit, int theScreenWidth, int theScreenHeight)
{
	float aRotation = theBlit.mRotation;
	int aWidth = theBlit.mSrcRight - theBlit.mSrcLeft;
	int aHeight = theBlit.mSrcBottom - theBlit.mSrcTop;
	float aPivotX = (float)theBlit.mPivotX;
	float aPivotY = (float)theBlit.mPivotY;
	if (std::fabs(aRotation) < 2.3841858e-07f)
	{
		float aSX = std::fabs(theBlit.mScaleX);
		float aSY = std::fabs(theBlit.mScaleY);
		if (aSX < 1e-6f || aSY < 1e-6f)
			return false;
		float aLeft = theBlit.mX - aPivotX * aSX;
		float aTop = theBlit.mY - aPivotY * aSY;
		float aRight = aLeft + aWidth * aSX;
		float aBottom = aTop + aHeight * aSY;
		theBlit.mLeft = std::max(0, (int)std::ceil(aLeft - 0.5f));
		theBlit.mTop = std::max(0, (int)std::ceil(aTop - 0.5f));
		theBlit.mRight = std::min(theScreenWidth, (int)std::ceil(aRight - 0.5f));
		theBlit.mBottom = std::min(theScreenHeight, (int)std::ceil(aBottom - 0.5f));
	}
	else
	{
		if (std::fabs(theBlit.mScaleX) < 1e-6f || std::fabs(theBlit.mScaleY) < 1e-6f)
			return false;
		float aCos = std::cos(aRotation);
		float aSin = std::sin(aRotation);
		float aCorners[4][2] = {{0, 0}, {(float)aWidth, 0}, {0, (float)aHeight}, {(float)aWidth, (float)aHeight}};
		float aMinX = 1e9f, aMinY = 1e9f, aMaxX = -1e9f, aMaxY = -1e9f;
		for (int i = 0; i < 4; i++)
		{
			float u = (aCorners[i][0] - aPivotX) * theBlit.mScaleX;
			float v = (aCorners[i][1] - aPivotY) * theBlit.mScaleY;
			float aX = aCos * u - aSin * v + theBlit.mX;
			float aY = aSin * u + aCos * v + theBlit.mY;
			aMinX = std::min(aMinX, aX);
			aMaxX = std::max(aMaxX, aX);
			aMinY = std::min(aMinY, aY);
			aMaxY = std::max(aMaxY, aY);
		}
		theBlit.mLeft = std::max(0, (int)std::floor(aMinX));
		theBlit.mTop = std::max(0, (int)std::floor(aMinY));
		theBlit.mRight = std::min(theScreenWidth, (int)std::ceil(aMaxX));
		theBlit.mBottom = std::min(theScreenHeight, (int)std::ceil(aMaxY));
	}
	return theBlit.mLeft < theBlit.mRight && theBlit.mTop < theBlit.mBottom;
}

// An unrotated call's maps (0x45c070's unrotated path), appended to the frame's map data. The column each destination column reads
// is the float mapping of the original's loop; a mirrored scale reads the image's columns the other way round. False when the
// call reads nothing (no column or no row falls inside the source), and so writes nothing.
bool makeMaps(const Blit& theBlit, const ScreenImage& theImage, Frame& theFrame, BlitMaps& theMaps)
{
	float aSX = std::fabs(theBlit.mScaleX);
	float aSY = std::fabs(theBlit.mScaleY);
	float aLeft = theBlit.mX - (float)theBlit.mPivotX * aSX;
	float aTop = theBlit.mY - (float)theBlit.mPivotY * aSY;
	int aWidth = theBlit.mSrcRight - theBlit.mSrcLeft;
	int aHeight = theBlit.mSrcBottom - theBlit.mSrcTop;
	int aSrcX = std::max(theBlit.mSrcLeft, 0);
	int aSrcY = std::max(theBlit.mSrcTop, 0);
	int aSrcRight = std::min(theBlit.mSrcRight, theImage.mWidth);
	int aSrcBottom = std::min(theBlit.mSrcBottom, theImage.mHeight);
	int aColumnCount = theBlit.mRight - theBlit.mLeft;
	int aRowCount = theBlit.mBottom - theBlit.mTop;
	size_t aBase = theFrame.mMapData.size();
	theFrame.mMapData.resize(aBase + aColumnCount + aRowCount);
	int* aColumns = theFrame.mMapData.data() + aBase;
	int* aRows = aColumns + aColumnCount;
	int aFirst = -1;
	int aLast = -1;
	bool aRun = true;
	for (int i = 0; i < aColumnCount; i++)
	{
		int u = (int)((theBlit.mLeft + i + 0.5f - aLeft) / aSX);
		if (u >= aWidth)
			u = aWidth - 1;
		if (theBlit.mScaleX < 0)
			u = aWidth - 1 - u;
		u += theBlit.mSrcLeft;
		int aColumn = (u < aSrcX || u >= aSrcRight) ? -1 : u;
		aColumns[i] = aColumn;
		if (aColumn < 0)
			continue;
		if (aFirst < 0)
			aFirst = i;
		else if (aLast != i - 1 || aColumn != aColumns[i - 1] + 1)
			aRun = false;
		aLast = i;
	}
	bool aAnyRow = false;
	for (int i = 0; i < aRowCount; i++)
	{
		int v = (int)((theBlit.mTop + i + 0.5f - aTop) / aSY);
		if (v >= aHeight)
			v = aHeight - 1;
		if (theBlit.mScaleY < 0)
			v = aHeight - 1 - v;
		v += theBlit.mSrcTop;
		aRows[i] = (v < aSrcY || v >= aSrcBottom) ? -1 : v;
		aAnyRow = aAnyRow || aRows[i] >= 0;
	}
	if (aFirst < 0 || !aAnyRow)
	{
		theFrame.mMapData.resize(aBase);
		return false;
	}
	theMaps.mColumns = (int32_t)aBase;
	theMaps.mRows = (int32_t)(aBase + aColumnCount);
	theMaps.mFirst = aFirst;
	theMaps.mLast = aLast;
	theMaps.mRun = aRun;
	return true;
}

// Whether the call i of the frame A writes the pixels the call j of the frame B writes, as the same function of the pixels
// under it
bool sameCall(const Frame& theA, size_t i, const Frame& theB, size_t j)
{
	const Blit& aFirstBlit = theA.mBlits[i];
	const Blit& aSecondBlit = theB.mBlits[j];
	if (std::memcmp(&aFirstBlit, &aSecondBlit, sizeof(Blit)) == 0)
		return true;
	const BlitMaps& aFirstMaps = theA.mMaps[i];
	const BlitMaps& aSecondMaps = theB.mMaps[j];
	if (aFirstMaps.mColumns < 0 || aSecondMaps.mColumns < 0)
		return false;
	// two unrotated image calls: the same image, operation and pixels, reading the same source column and row for each
	if (aFirstBlit.mImage != aSecondBlit.mImage || aFirstBlit.mOp != aSecondBlit.mOp ||
		aFirstBlit.mThreshold != aSecondBlit.mThreshold || aFirstBlit.mOpacity != aSecondBlit.mOpacity ||
		aFirstBlit.mOffsetR != aSecondBlit.mOffsetR || aFirstBlit.mOffsetG != aSecondBlit.mOffsetG ||
		aFirstBlit.mOffsetB != aSecondBlit.mOffsetB || aFirstBlit.mLeft != aSecondBlit.mLeft ||
		aFirstBlit.mTop != aSecondBlit.mTop || aFirstBlit.mRight != aSecondBlit.mRight ||
		aFirstBlit.mBottom != aSecondBlit.mBottom)
		return false;
	return aFirstMaps.mFirst == aSecondMaps.mFirst && aFirstMaps.mLast == aSecondMaps.mLast &&
		aFirstMaps.mRun == aSecondMaps.mRun &&
		std::memcmp(theA.mMapData.data() + aFirstMaps.mColumns, theB.mMapData.data() + aSecondMaps.mColumns,
			(size_t)(aFirstBlit.mRight - aFirstBlit.mLeft + aFirstBlit.mBottom - aFirstBlit.mTop) * sizeof(int)) == 0;
}

// Draws one recorded image call into the screen, only the pixels inside the clip rectangle (a part of the screen).
// 0x45e2d0, 0x45c070 and the other blitters: the position with the state's translation, snapped (truncated) as the state
// says; the source rectangle placed with its pivot at the position, scaled and rotated about it. Without a rotation a
// negative scale mirrors the image inside the rectangle the positive scale would cover (as 0x45c070's unrotated path does).
// (0x457930 and 0x457940 set the blit's flags and 0x4589e0 compares two floats: translated inline)
void renderBlit(uint32_t* theBits, int theScreenWidth, const Blit& theBlit, const BlitMaps& theMaps, const int* theMapData,
	const ScreenImage* theImage, int theClipLeft, int theClipTop, int theClipRight, int theClipBottom)
{
	const PixelOp anOp = blitOp(theBlit);
	int aX0 = std::max(theBlit.mLeft, theClipLeft);
	int aY0 = std::max(theBlit.mTop, theClipTop);
	int aX1 = std::min(theBlit.mRight, theClipRight);
	int aY1 = std::min(theBlit.mBottom, theClipBottom);
	if (aX0 >= aX1 || aY0 >= aY1)
		return;
	const uint32_t* aSrcBits = theImage->mBits.data();
	int aSrcPitch = theImage->mWidth;

	if (theMaps.mColumns >= 0)
	{
		const int* aColumns = theMapData + theMaps.mColumns;  // from theBlit.mLeft
		const int* aRows = theMapData + theMaps.mRows;        // from theBlit.mTop
		int aFirst = std::max(theMaps.mFirst, aX0 - theBlit.mLeft);
		int aLast = std::min(theMaps.mLast, aX1 - 1 - theBlit.mLeft);
		if (aFirst > aLast)
			return;
		bool aRun = theMaps.mRun;
		withOp(anOp.mOp, [&](auto theTag)
		{
			constexpr int OP = decltype(theTag)::value;
			for (int dy = aY0; dy < aY1; dy++)
			{
				int v = aRows[dy - theBlit.mTop];
				if (v < 0)
					continue;
				uint32_t* aDest = theBits + dy * theScreenWidth + theBlit.mLeft;
				const uint32_t* aRow = aSrcBits + v * aSrcPitch;
				if (aRun)
				{
					uint32_t* __restrict d = aDest + aFirst;
					const uint32_t* __restrict s = aRow + aColumns[aFirst];
					int aRunLength = aLast - aFirst + 1;
					for (int i = 0; i < aRunLength; i++)
						applyPixel<OP>(anOp, d[i], s[i]);
				}
				else
				{
					for (int i = aFirst; i <= aLast; i++)
					{
						if (aColumns[i] >= 0)
							applyPixel<OP>(anOp, aDest[i], aRow[aColumns[i]]);
					}
				}
			}
		});
		return;
	}

	// Rotated: translate(-pivot), scale, rotate, translate(position) (0x45c070's matrix); each screen pixel's centre
	// is mapped back into the source
	float aRotation = theBlit.mRotation;
	float x = theBlit.mX;
	float y = theBlit.mY;
	int aSrcX = std::max(theBlit.mSrcLeft, 0);
	int aSrcY = std::max(theBlit.mSrcTop, 0);
	int aSrcRight = std::min(theBlit.mSrcRight, theImage->mWidth);
	int aSrcBottom = std::min(theBlit.mSrcBottom, theImage->mHeight);
	int aWidth = theBlit.mSrcRight - theBlit.mSrcLeft;
	int aHeight = theBlit.mSrcBottom - theBlit.mSrcTop;
	float aPivotX = (float)theBlit.mPivotX;
	float aPivotY = (float)theBlit.mPivotY;
	float aCos = std::cos(aRotation);
	float aSin = std::sin(aRotation);
	float anInvX = 1.0f / theBlit.mScaleX;
	float anInvY = 1.0f / theBlit.mScaleY;
	withOp(anOp.mOp, [&](auto theTag)
	{
		constexpr int OP = decltype(theTag)::value;
		for (int dy = aY0; dy < aY1; dy++)
		{
			uint32_t* aDest = theBits + dy * theScreenWidth;
			float aRelY = dy + 0.5f - y;
			for (int dx = aX0; dx < aX1; dx++)
			{
				float aRelX = dx + 0.5f - x;
				// the inverse rotation, then the inverse scale
				float u = (aCos * aRelX + aSin * aRelY) * anInvX + aPivotX;
				float v = (-aSin * aRelX + aCos * aRelY) * anInvY + aPivotY;
				if (u < 0 || v < 0 || u >= aWidth || v >= aHeight)
					continue;
				int aU = (int)u + theBlit.mSrcLeft;
				int aV = (int)v + theBlit.mSrcTop;
				if (aU < aSrcX || aU >= aSrcRight || aV < aSrcY || aV >= aSrcBottom)
					continue;
				applyPixel<OP>(anOp, aDest[dx], aSrcBits[aV * aSrcPitch + aU]);
			}
		}
	});
}

// Draws one recorded line (slot 7, 0x45e180): a one-pixel line in the state's colour and opacity, only the pixels inside the
// clip rectangle
void renderLine(uint32_t* theBits, int theScreenWidth, const Blit& theBlit, int theClipLeft, int theClipTop, int theClipRight,
	int theClipBottom)
{
	int x0 = theBlit.mSrcLeft;
	int y0 = theBlit.mSrcTop;
	int x1 = theBlit.mSrcRight;
	int y1 = theBlit.mSrcBottom;
	uint32_t aColor = (uint32_t)theBlit.mThreshold;
	int anAlpha = theBlit.mOpacity;
	int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int anError = dx + dy;
	while (true)
	{
		if (x0 >= theClipLeft && y0 >= theClipTop && x0 < theClipRight && y0 < theClipBottom)
			theBits[y0 * theScreenWidth + x0] = blend(theBits[y0 * theScreenWidth + x0], aColor, anAlpha);
		if (x0 == x1 && y0 == y1)
			break;
		int e2 = 2 * anError;
		if (e2 >= dy)
		{
			anError += dy;
			x0 += sx;
		}
		if (e2 <= dx)
		{
			anError += dx;
			y0 += sy;
		}
	}
}

}

// The frame's draw calls and the screen's tiles (the last frame's too, to tell which tiles changed)
struct ScreenGraphics::Retained
{
	Frame mFrame;
	Frame mPrev;
	std::vector<std::vector<uint32_t>> mTiles;  // each tile's calls: indices into mFrame, in order
	std::vector<std::vector<uint32_t>> mPrevTiles;
	bool mHasPrev;
	std::vector<ScreenGraphics::Rectangle> mRedrawn;  // what the last finish drew
	Retained() : mHasPrev(false) {}
};

// port (for speed): the rectangles the last finish drew
const std::vector<ScreenGraphics::Rectangle>& ScreenGraphics::redrawn() const
{
	return mRetained->mRedrawn;
}

// port (for speed): the number the next image has
uint32_t ScreenImage::nextSerial()
{
	static uint32_t sSerial = 0;
	return ++sSerial;
}

ScreenGraphics::ScreenGraphics(uint32_t* theBits, int theWidth, int theHeight)
	: mBits(theBits), mWidth(theWidth), mHeight(theHeight), mRetained(new Retained)
{
	int aTiles = ((theWidth + TILE - 1) / TILE) * ((theHeight + TILE - 1) / TILE);
	mRetained->mTiles.resize(aTiles);
	mRetained->mPrevTiles.resize(aTiles);
}

// port (for speed): the retained frames go with the screen
ScreenGraphics::~ScreenGraphics()
{
	delete mRetained;
}

// port (for speed): the start of a frame: the screen is black, and nothing is drawn (the calls are recorded, and finish draws them)
void ScreenGraphics::clear()
{
	mRetained->mFrame.clear();
}

// port (for speed): the end of a frame: the tiles whose calls differ from the last frame's are drawn. Returns whether any pixel may
// have changed.
bool ScreenGraphics::finish()
{
	Retained& aRetained = *mRetained;
	const Frame& aNowFrame = aRetained.mFrame;
	const Frame& aBeforeFrame = aRetained.mPrev;
	std::vector<Rectangle>& aRedrawn = aRetained.mRedrawn;
	aRedrawn.clear();
	if (aRetained.mHasPrev && aNowFrame.mBlits.size() == aBeforeFrame.mBlits.size())
	{
		bool aSame = true;
		for (size_t i = 0; aSame && i < aNowFrame.mBlits.size(); i++)
			aSame = sameCall(aNowFrame, i, aBeforeFrame, i);
		if (aSame)
			return false;  // the same calls, the same screen
	}

	int aTilesX = (mWidth + TILE - 1) / TILE;
	int aTilesY = (mHeight + TILE - 1) / TILE;
	for (std::vector<uint32_t>& aTile : aRetained.mTiles)
		aTile.clear();
	for (size_t i = 0; i < aNowFrame.mBlits.size(); i++)
	{
		const Blit& aBlit = aNowFrame.mBlits[i];
		int aTX1 = (aBlit.mRight - 1) / TILE;
		int aTY1 = (aBlit.mBottom - 1) / TILE;
		for (int aTY = aBlit.mTop / TILE; aTY <= aTY1; aTY++)
			for (int aTX = aBlit.mLeft / TILE; aTX <= aTX1; aTX++)
				aRetained.mTiles[aTY * aTilesX + aTX].push_back((uint32_t)i);
	}

	bool aChanged = false;
	for (int aTY = 0; aTY < aTilesY; aTY++)
	{
		size_t aRowStart = aRedrawn.size();  // this row's rectangles; the ones before are from the rows above
		for (int aTX = 0; aTX < aTilesX; aTX++)
		{
			const std::vector<uint32_t>& aNow = aRetained.mTiles[aTY * aTilesX + aTX];
			const std::vector<uint32_t>& aBefore = aRetained.mPrevTiles[aTY * aTilesX + aTX];
			bool aSame = aRetained.mHasPrev && aNow.size() == aBefore.size();
			for (size_t k = 0; aSame && k < aNow.size(); k++)
				aSame = sameCall(aNowFrame, aNow[k], aBeforeFrame, aBefore[k]);
			if (aSame)
				continue;
			aChanged = true;
			int aLeft = aTX * TILE;
			int aTop = aTY * TILE;
			int aRight = std::min(aLeft + TILE, mWidth);
			int aBottom = std::min(aTop + TILE, mHeight);
			if (aRedrawn.size() > aRowStart && aRedrawn.back().mX + aRedrawn.back().mWidth == aLeft)
				aRedrawn.back().mWidth += aRight - aLeft;
			else
				aRedrawn.push_back({aLeft, aTop, aRight - aLeft, aBottom - aTop});
			for (int y = aTop; y < aBottom; y++)
				std::fill(mBits + y * mWidth + aLeft, mBits + y * mWidth + aRight, 0xff000000);
			for (uint32_t anIndex : aNow)
			{
				const Blit& aBlit = aNowFrame.mBlits[anIndex];
				if (aBlit.mKind == BLIT_LINE)
					renderLine(mBits, mWidth, aBlit, aLeft, aTop, aRight, aBottom);
				else
					renderBlit(mBits, mWidth, aBlit, aNowFrame.mMaps[anIndex], aNowFrame.mMapData.data(), aNowFrame.mImages[anIndex],
						aLeft, aTop, aRight, aBottom);
			}
		}
		// a run with the span of a rectangle that ends where this row begins is joined to it
		for (size_t i = aRowStart; i < aRedrawn.size();)
		{
			bool aJoined = false;
			for (size_t j = 0; j < aRowStart && !aJoined; j++)
			{
				Rectangle& anAbove = aRedrawn[j];
				if (anAbove.mX == aRedrawn[i].mX && anAbove.mWidth == aRedrawn[i].mWidth &&
					anAbove.mY + anAbove.mHeight == aRedrawn[i].mY)
				{
					anAbove.mHeight += aRedrawn[i].mHeight;
					aJoined = true;
				}
			}
			if (aJoined)
				aRedrawn.erase(aRedrawn.begin() + i);
			else
				i++;
		}
	}
	std::swap(aRetained.mFrame, aRetained.mPrev);
	aRetained.mTiles.swap(aRetained.mPrevTiles);
	aRetained.mHasPrev = true;
	return aChanged;
}

// The image's call recorded: the position with the state's translation, snapped (truncated) as the state says, and the
// pixel operation the state and the image choose (0x45e2d0)
void ScreenGraphics::drawImage(Image* theImage, const IntRect& theSrc, float x, float y)
{
	ScreenImage* anImage = static_cast<ScreenImage*>(theImage);
	if (anImage == nullptr || theSrc.isEmpty())
		return;
	float aRotation = getRotation();
	float aScaleX = getScaleX();
	float aScaleY = getScaleY();
	float anOpacity = getOpacity();
	const RenderState& aState = state();
	x = x + aState.mTranslation.x;
	y = y + aState.mTranslation.y;
	int aSnap = getSnap();
	if (aSnap == SNAP_BOTH || aSnap == SNAP_Y)
		y = (float)(int)y;
	if (aSnap == SNAP_BOTH || aSnap == SNAP_X)
		x = (float)(int)x;

	Blit aBlit;
	std::memset(&aBlit, 0, sizeof(aBlit));
	aBlit.mKind = BLIT_IMAGE;
	aBlit.mImage = anImage->mSerial;
	aBlit.mSrcLeft = theSrc.left;
	aBlit.mSrcTop = theSrc.top;
	aBlit.mSrcRight = theSrc.right;
	aBlit.mSrcBottom = theSrc.bottom;
	aBlit.mX = x;
	aBlit.mY = y;
	aBlit.mRotation = aRotation;
	aBlit.mScaleX = aScaleX;
	aBlit.mScaleY = aScaleY;
	aBlit.mPivotX = anImage->getPivotX();
	aBlit.mPivotY = anImage->getPivotY();
	aBlit.mThreshold = anImage->getAlphaThreshold();
	aBlit.mOpacity = clampByte((int)(anOpacity * 255.0f));
	const Color& anOffset = getColorOffset();
	if (2.3841858e-07f <= std::fabs(anOffset.r) || 2.3841858e-07f <= std::fabs(anOffset.g) ||
		2.3841858e-07f <= std::fabs(anOffset.b))
	{
		aBlit.mOp = OP_OFFSET;
		aBlit.mOffsetR = (int)(anOffset.r * 255.0f);
		aBlit.mOffsetG = (int)(anOffset.g * 255.0f);
		aBlit.mOffsetB = (int)(anOffset.b * 255.0f);
	}
	else if (anImage->getAlphaMode() != ALPHA_BLEND && std::fabs(anOpacity - 1.0f) < 2.3841858e-07f)
		aBlit.mOp = anImage->getAlphaMode() == ALPHA_TEST ? OP_TEST : OP_COPY;
	else
		aBlit.mOp = OP_BLEND;
	if (aBlit.mOp != OP_COPY && aBlit.mOp != OP_TEST && aBlit.mOpacity == 0)
		return;
	if (!blitBounds(aBlit, mWidth, mHeight))
		return;
	BlitMaps aMaps;
	aMaps.mColumns = aMaps.mRows = -1;
	aMaps.mFirst = aMaps.mLast = 0;
	aMaps.mRun = false;
	Frame& aFrame = mRetained->mFrame;
	if (std::fabs(aRotation) < 2.3841858e-07f && !makeMaps(aBlit, *anImage, aFrame, aMaps))
		return;
	aFrame.mBlits.push_back(aBlit);
	aFrame.mImages.push_back(anImage);
	aFrame.mMaps.push_back(aMaps);
}

// Slot 7 (0x45e180): a one-pixel line (with the translation) in the state's colour and opacity, recorded
void ScreenGraphics::drawLine(const Point& theFrom, const Point& theTo)
{
	const RenderState& aState = state();
	Blit aBlit;
	std::memset(&aBlit, 0, sizeof(aBlit));
	aBlit.mKind = BLIT_LINE;
	aBlit.mSrcLeft = (int)(theFrom.x + aState.mTranslation.x);
	aBlit.mSrcTop = (int)(theFrom.y + aState.mTranslation.y);
	aBlit.mSrcRight = (int)(theTo.x + aState.mTranslation.x);
	aBlit.mSrcBottom = (int)(theTo.y + aState.mTranslation.y);
	aBlit.mThreshold = (int)(uint32_t)(clampByte((int)(aState.mColor.r * 255)) << 16 |
		clampByte((int)(aState.mColor.g * 255)) << 8 | clampByte((int)(aState.mColor.b * 255)));
	aBlit.mOpacity = clampByte((int)(aState.mOpacity * 255));
	aBlit.mLeft = std::max(0, std::min(aBlit.mSrcLeft, aBlit.mSrcRight));
	aBlit.mTop = std::max(0, std::min(aBlit.mSrcTop, aBlit.mSrcBottom));
	aBlit.mRight = std::min(mWidth, std::max(aBlit.mSrcLeft, aBlit.mSrcRight) + 1);
	aBlit.mBottom = std::min(mHeight, std::max(aBlit.mSrcTop, aBlit.mSrcBottom) + 1);
	if (aBlit.mLeft >= aBlit.mRight || aBlit.mTop >= aBlit.mBottom)
		return;
	BlitMaps aMaps;
	aMaps.mColumns = aMaps.mRows = -1;
	aMaps.mFirst = aMaps.mLast = 0;
	aMaps.mRun = false;
	Frame& aFrame = mRetained->mFrame;
	aFrame.mBlits.push_back(aBlit);
	aFrame.mImages.push_back(nullptr);
	aFrame.mMaps.push_back(aMaps);
}
