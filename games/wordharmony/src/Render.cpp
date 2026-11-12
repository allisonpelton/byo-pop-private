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

// port (performance pass): the pixel functions are inlined into the loops of the blit they are called from, where they were
// a call and a switch a pixel
#if defined(__GNUC__) || defined(__clang__)
#define BLIT_INLINE inline __attribute__((always_inline))
#else
#define BLIT_INLINE inline
#endif

// Each channel is (source * alpha + destination * (255 - alpha)) / 255. Port (performance pass): red and blue are worked out
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
// (0x4579c0, 0x4581c0, 0x458230, 0x458560, 0x458cb0). The operation is a template parameter (port, performance pass: the
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

// The source column each destination column of an unrotated blit reads, -1 where the blit leaves it
std::vector<int> sBlitColumns;

}

// port (performance pass): the frame's clear is made when something is drawn that does not cover the screen, or at the end of
// the frame (finish), not at its start: most frames begin with an opaque copy of a full-screen picture, which writes every
// pixel and would leave the 1.9 MB of the clear's stores for nothing
void ScreenGraphics::clear()
{
	mCleared = true;
}

void ScreenGraphics::finish()
{
	if (!mCleared)
		return;
	mCleared = false;
	std::fill(mBits, mBits + (size_t)mWidth * mHeight, 0xff000000);
}

// 0x45e2d0: the position with the state's translation, snapped (truncated) as the state says; the source rectangle
// placed with its pivot at the position, scaled and rotated about it. Without a rotation a negative scale mirrors the
// image inside the rectangle the positive scale would cover (as 0x45c070's unrotated path does).
// (0x457930 and 0x457940 set the blit's flags and 0x4589e0 compares two floats: translated inline)
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

	PixelOp anOp;
	anOp.mThreshold = anImage->getAlphaThreshold();
	anOp.mOpacity = clampByte((int)(anOpacity * 255.0f));
	anOp.mOffsetR = anOp.mOffsetG = anOp.mOffsetB = 0;
	const Color& anOffset = getColorOffset();
	if (2.3841858e-07f <= std::fabs(anOffset.r) || 2.3841858e-07f <= std::fabs(anOffset.g) ||
		2.3841858e-07f <= std::fabs(anOffset.b))
	{
		anOp.mOp = OP_OFFSET;
		anOp.mOffsetR = (int)(anOffset.r * 255.0f);
		anOp.mOffsetG = (int)(anOffset.g * 255.0f);
		anOp.mOffsetB = (int)(anOffset.b * 255.0f);
	}
	else if (anImage->getAlphaMode() != ALPHA_BLEND && std::fabs(anOpacity - 1.0f) < 2.3841858e-07f)
		anOp.mOp = anImage->getAlphaMode() == ALPHA_TEST ? OP_TEST : OP_COPY;
	else
		anOp.mOp = OP_BLEND;
	if (anOp.mOp != OP_COPY && anOp.mOp != OP_TEST && anOp.mOpacity == 0)
		return;

	int aSrcX = std::max(theSrc.left, 0);
	int aSrcY = std::max(theSrc.top, 0);
	int aSrcRight = std::min(theSrc.right, anImage->mWidth);
	int aSrcBottom = std::min(theSrc.bottom, anImage->mHeight);
	int aWidth = theSrc.right - theSrc.left;
	int aHeight = theSrc.bottom - theSrc.top;
	float aPivotX = (float)anImage->getPivotX();
	float aPivotY = (float)anImage->getPivotY();
	const uint32_t* aSrcBits = anImage->mBits.data();
	int aSrcPitch = anImage->mWidth;

	if (std::fabs(aRotation) < 2.3841858e-07f)
	{
		float aSX = std::fabs(aScaleX);
		float aSY = std::fabs(aScaleY);
		if (aSX < 1e-6f || aSY < 1e-6f)
			return;
		float aLeft = x - aPivotX * aSX;
		float aTop = y - aPivotY * aSY;
		float aRight = aLeft + aWidth * aSX;
		float aBottom = aTop + aHeight * aSY;
		int aX0 = std::max(0, (int)std::ceil(aLeft - 0.5f));
		int aY0 = std::max(0, (int)std::ceil(aTop - 0.5f));
		int aX1 = std::min(mWidth, (int)std::ceil(aRight - 0.5f));
		int aY1 = std::min(mHeight, (int)std::ceil(aBottom - 0.5f));
		if (aX0 >= aX1 || aY0 >= aY1)
			return;
		// port (performance pass): the column each destination column reads is worked out once for the blit, not for each
		// pixel of each row (the same float expression); the columns inside the source that follow one another (any blit
		// not mirrored or scaled) are then copied or blended as a run
		int aCount = aX1 - aX0;
		sBlitColumns.resize(aCount);
		int aFirst = -1;
		int aLast = -1;
		bool aRun = true;
		for (int i = 0; i < aCount; i++)
		{
			int u = (int)((aX0 + i + 0.5f - aLeft) / aSX);
			if (u >= aWidth)
				u = aWidth - 1;
			if (aScaleX < 0)
				u = aWidth - 1 - u;
			u += theSrc.left;
			int aColumn = (u < aSrcX || u >= aSrcRight) ? -1 : u;
			sBlitColumns[i] = aColumn;
			if (aColumn < 0)
				continue;
			if (aFirst < 0)
				aFirst = i;
			else if (aLast != i - 1 || aColumn != sBlitColumns[i - 1] + 1)
				aRun = false;
			aLast = i;
		}
		if (aFirst < 0)
			return;
		const int* aColumns = sBlitColumns.data();
		// the source row each destination row reads (the float mapping of 0x45c070), -1 where the blit leaves it
		auto aSourceRow = [&](int dy)
		{
			int v = (int)((dy + 0.5f - aTop) / aSY);
			if (v >= aHeight)
				v = aHeight - 1;
			if (aScaleY < 0)
				v = aHeight - 1 - v;
			v += theSrc.top;
			return (v < aSrcY || v >= aSrcBottom) ? -1 : v;
		};
		if (mCleared)
		{
			// the screen's clear is not made yet: an opaque copy that writes every pixel of the screen makes it unneeded
			bool aCovers = anOp.mOp == OP_COPY && aX0 == 0 && aX1 == mWidth && aY0 == 0 && aY1 == mHeight &&
				aFirst == 0 && aLast == aCount - 1;
			for (int i = 0; aCovers && !aRun && i < aCount; i++)
				aCovers = aColumns[i] >= 0;
			for (int dy = aY0; aCovers && dy < aY1; dy++)
				aCovers = aSourceRow(dy) >= 0;
			if (aCovers)
				mCleared = false;
			else
				finish();
		}
		withOp(anOp.mOp, [&](auto theTag)
		{
			constexpr int OP = decltype(theTag)::value;
			for (int dy = aY0; dy < aY1; dy++)
			{
				int v = aSourceRow(dy);
				if (v < 0)
					continue;
				uint32_t* aDest = mBits + dy * mWidth + aX0;
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
	if (std::fabs(aScaleX) < 1e-6f || std::fabs(aScaleY) < 1e-6f)
		return;
	float c = std::cos(aRotation);
	float s = std::sin(aRotation);
	// forward: X = c*sx*(u-px) - s*sy*(v-py) + x, Y = s*sx*(u-px) + c*sy*(v-py) + y
	float aCorners[4][2] = {{0, 0}, {(float)aWidth, 0}, {0, (float)aHeight}, {(float)aWidth, (float)aHeight}};
	float aMinX = 1e9f, aMinY = 1e9f, aMaxX = -1e9f, aMaxY = -1e9f;
	for (int i = 0; i < 4; i++)
	{
		float u = (aCorners[i][0] - aPivotX) * aScaleX;
		float v = (aCorners[i][1] - aPivotY) * aScaleY;
		float aX = c * u - s * v + x;
		float aY = s * u + c * v + y;
		aMinX = std::min(aMinX, aX);
		aMaxX = std::max(aMaxX, aX);
		aMinY = std::min(aMinY, aY);
		aMaxY = std::max(aMaxY, aY);
	}
	int aX0 = std::max(0, (int)std::floor(aMinX));
	int aY0 = std::max(0, (int)std::floor(aMinY));
	int aX1 = std::min(mWidth, (int)std::ceil(aMaxX));
	int aY1 = std::min(mHeight, (int)std::ceil(aMaxY));
	float anInvX = 1.0f / aScaleX;
	float anInvY = 1.0f / aScaleY;
	finish();
	withOp(anOp.mOp, [&](auto theTag)
	{
		constexpr int OP = decltype(theTag)::value;
		for (int dy = aY0; dy < aY1; dy++)
		{
			uint32_t* aDest = mBits + dy * mWidth;
			float aRelY = dy + 0.5f - y;
			for (int dx = aX0; dx < aX1; dx++)
			{
				float aRelX = dx + 0.5f - x;
				// the inverse rotation, then the inverse scale
				float u = (c * aRelX + s * aRelY) * anInvX + aPivotX;
				float v = (-s * aRelX + c * aRelY) * anInvY + aPivotY;
				if (u < 0 || v < 0 || u >= aWidth || v >= aHeight)
					continue;
				int aU = (int)u + theSrc.left;
				int aV = (int)v + theSrc.top;
				if (aU < aSrcX || aU >= aSrcRight || aV < aSrcY || aV >= aSrcBottom)
					continue;
				applyPixel<OP>(anOp, aDest[dx], aSrcBits[aV * aSrcPitch + aU]);
			}
		}
	});
}

// Slot 5: the rectangle (with the translation) in the state's colour and opacity
void ScreenGraphics::fillRect(const Rect& theRect)
{
	finish();
	const RenderState& aState = state();
	int aX0 = std::max(0, (int)(theRect.left + aState.mTranslation.x));
	int aY0 = std::max(0, (int)(theRect.top + aState.mTranslation.y));
	int aX1 = std::min(mWidth, (int)(theRect.right + aState.mTranslation.x));
	int aY1 = std::min(mHeight, (int)(theRect.bottom + aState.mTranslation.y));
	uint32_t aColor = (uint32_t)(clampByte((int)(aState.mColor.r * 255)) << 16 |
		clampByte((int)(aState.mColor.g * 255)) << 8 | clampByte((int)(aState.mColor.b * 255)));
	int anAlpha = clampByte((int)(aState.mOpacity * 255));
	for (int y = aY0; y < aY1; y++)
		for (int x = aX0; x < aX1; x++)
			mBits[y * mWidth + x] = blend(mBits[y * mWidth + x], aColor, anAlpha);
}

// Slot 6: the same added to the screen
void ScreenGraphics::fillRectAdditive(const Rect& theRect)
{
	finish();
	const RenderState& aState = state();
	int aX0 = std::max(0, (int)(theRect.left + aState.mTranslation.x));
	int aY0 = std::max(0, (int)(theRect.top + aState.mTranslation.y));
	int aX1 = std::min(mWidth, (int)(theRect.right + aState.mTranslation.x));
	int aY1 = std::min(mHeight, (int)(theRect.bottom + aState.mTranslation.y));
	int r = (int)(aState.mColor.r * aState.mOpacity * 255);
	int g = (int)(aState.mColor.g * aState.mOpacity * 255);
	int b = (int)(aState.mColor.b * aState.mOpacity * 255);
	for (int y = aY0; y < aY1; y++)
	{
		for (int x = aX0; x < aX1; x++)
		{
			uint32_t& p = mBits[y * mWidth + x];
			p = 0xff000000 | clampByte((int)((p >> 16) & 0xff) + r) << 16 | clampByte((int)((p >> 8) & 0xff) + g) << 8 |
				clampByte((int)(p & 0xff) + b);
		}
	}
}

// Slot 7: a one-pixel line (with the translation) in the state's colour and opacity
void ScreenGraphics::drawLine(const Point& theFrom, const Point& theTo)
{
	finish();
	const RenderState& aState = state();
	int x0 = (int)(theFrom.x + aState.mTranslation.x);
	int y0 = (int)(theFrom.y + aState.mTranslation.y);
	int x1 = (int)(theTo.x + aState.mTranslation.x);
	int y1 = (int)(theTo.y + aState.mTranslation.y);
	uint32_t aColor = (uint32_t)(clampByte((int)(aState.mColor.r * 255)) << 16 |
		clampByte((int)(aState.mColor.g * 255)) << 8 | clampByte((int)(aState.mColor.b * 255)));
	int anAlpha = clampByte((int)(aState.mOpacity * 255));
	int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int anError = dx + dy;
	while (true)
	{
		if (x0 >= 0 && y0 >= 0 && x0 < mWidth && y0 < mHeight)
			mBits[y0 * mWidth + x0] = blend(mBits[y0 * mWidth + x0], aColor, anAlpha);
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
