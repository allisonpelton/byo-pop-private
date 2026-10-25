#ifndef __BIGMONEY_HDPIXELS_H__
#define __BIGMONEY_HDPIXELS_H__

// port: HD art support for the game's direct pixel edits. Not in the original.
//
// A mod's HD image ("name@2x.png", see framework/DDImage.h) keeps its 1x pixels, which the game edits
// exactly as the original does, and carries HD pixels the GPU draws. The framework repeats software
// draws on HD pixels by itself, but an edit of GetBits() makes them stale and the image falls back to
// its 1x pixels. The few places where Big Money edits pixels directly (the bright coins, the grey
// selector illustrations, the "Deluxe" badge, the level-up squash and the stats fade) repeat the edit on
// a copy of the HD pixels through these helpers and put it back, so HD art keeps its detail. Without HD
// art nothing changes.

#include "MemoryImage.h"

#include <stdint.h>
#include <vector>

namespace Sexy
{

// A working copy of an image's HD pixels
struct HDPixels
{
	std::vector<uint32_t> mBits;
	int mWidth = 0;
	int mHeight = 0;
	int mScale = 0;

	bool IsValid() const { return !mBits.empty(); }
	uint32_t* Row(int theY) { return &mBits[(size_t) theY * mWidth]; }
	const uint32_t* Row(int theY) const { return &mBits[(size_t) theY * mWidth]; }
};

// The image's HD pixels when they are valid and exactly mScale times its size (else invalid)
inline HDPixels GetHDPixels(MemoryImage* theImage)
{
	HDPixels aPixels;
	if ((theImage != NULL) && (theImage->HasMirrorableHDBits()))
	{
		aPixels.mBits = theImage->mHDBits;
		aPixels.mWidth = theImage->mHDWidth;
		aPixels.mHeight = theImage->mHDHeight;
		aPixels.mScale = theImage->mHDScale;
	}
	return aPixels;
}

// HD pixels for theImage at theScale: its current ones when they match, else its 1x pixels scaled up (a
// new image, or one whose HD pixels went stale), as MemoryImage::BeginHDMirror starts them. Call it
// before editing the 1x pixels, so pixels the edit leaves alone match.
inline HDPixels GetOrCreateHDPixels(MemoryImage* theImage, int theScale)
{
	HDPixels aPixels = GetHDPixels(theImage);
	if ((!aPixels.IsValid()) || (aPixels.mScale != theScale))
	{
		aPixels.mWidth = theImage->mWidth * theScale;
		aPixels.mHeight = theImage->mHeight * theScale;
		aPixels.mScale = theScale;
		aPixels.mBits.assign((size_t) aPixels.mWidth * aPixels.mHeight, 0);
		const ulong* aBits = theImage->GetBits();
		if (aBits != NULL)
		{
			for (int y = 0; y < aPixels.mHeight; y++)
			{
				const ulong* aRow = aBits + (y / theScale) * theImage->mWidth;
				uint32_t* anOut = aPixels.Row(y);
				for (int x = 0; x < aPixels.mWidth; x++)
					anOut[x] = (uint32_t) aRow[x / theScale];
			}
		}
	}
	return aPixels;
}

// After BitsChanged(): the edited HD pixels become the image's texture again
inline void SetHDPixels(MemoryImage* theImage, const HDPixels& thePixels)
{
	if (thePixels.IsValid())
		theImage->SetHDBits(&thePixels.mBits[0], thePixels.mWidth, thePixels.mHeight, thePixels.mScale);
}

}

#endif
