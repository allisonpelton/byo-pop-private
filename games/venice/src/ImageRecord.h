// The engine's picture records: a picture (a file under gfx) shared by name between the objects that draw it, with the table of
// frames when the picture is a sheet of animation frames.

#ifndef VENICE_IMAGERECORD_H
#define VENICE_IMAGERECORD_H

#include "CursorList.h"

#include <cstdint>

namespace Sexy
{
class DDImage;
}

// One frame of a picture sheet, in the sheet's pixels
struct FrameRect
{
	float mLeft;
	float mTop;
	float mRight;
	float mBottom;
};

// 0x13c bytes in the original (0x42c4e0 clears one)
class ImageRecord
{
public:
	ImageRecord* mNext = nullptr;
	ImageRecord* mPrev = nullptr;
	char mName[256] = {};
	float mWidth = 0; // the picture's size, or a text object's glyph's
	float mHeight = 0;
	float mFrameDuration = 0; // in 4 ms ticks
	int mFrameCount = 1;
	float mFrameWidth = 0;
	float mFrameHeight = 0;
	Sexy::DDImage* mImage = nullptr;
	FrameRect* mFrameRects = nullptr; // null for a picture that is not a sheet
	int mStride = 0; // bytes in a row of the picture's pixels
	int mReferenceCount = 1;
	uint32_t* mPixels = nullptr; // set by LoadPixels

	ImageRecord() = default;
	~ImageRecord(); // 0x42c5c0
	bool Load(const char* theName, int theFrameWidth, int theFrameHeight, int theFrameCount,
		int theFramesPerSecond); // 0x42c650
	bool LoadPixels(); // 0x42c7d0: the picture's pixels, for the text engine's glyph measuring
	bool BuildFrames(int theFrameWidth, int theFrameHeight, int theFrameCount, int theFramesPerSecond); // 0x42c810
};

extern CursorList<ImageRecord> gImageRecords; // 0x9a2808

// 0x42c990: the record of the picture of this name (one more user of it), made when there is none
ImageRecord* AcquireImageRecord(const char* theName, int theFrameWidth, int theFrameHeight, int theFrameCount,
	int theFramesPerSecond);
// 0x42ca40: takes the record out of the list and frees it
void ReleaseImageRecord(ImageRecord* theRecord);

#endif
