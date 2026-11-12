// The engine's picture records (0x42c3f0-0x42cc10).

#include "ImageRecord.h"

#include "DDImage.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "VeniceApp.h"
#include "VeniceGlobals.h"

#include <cstdio>
#include <cstring>

CursorList<ImageRecord> gImageRecords;

// 0x42c5c0
ImageRecord::~ImageRecord()
{
	delete mImage;
	mImage = nullptr;
	delete[] mFrameRects;
	mFrameRects = nullptr;
}

// 0x42c650: loads the picture; a sheet of frames gets its table
bool ImageRecord::Load(const char* theName, int theFrameWidth, int theFrameHeight, int theFrameCount,
	int theFramesPerSecond)
{
	Sexy::DDImage* anImage = gApp->GetImage(theName, true);
	bool aLoaded = anImage != nullptr;
	if (!aLoaded) {
		fprintf(stderr, "ERROR: Failed to load image %s\n", theName);
		mImage = nullptr;
	} else {
		mImage = anImage;
		strcpy(mName, theName);
		mFrameWidth = (float)mImage->GetWidth();
		mWidth = mFrameWidth;
		mFrameHeight = (float)mImage->GetHeight();
		mHeight = mFrameHeight;
	}
	if (theFrameWidth != 0) {
		mFrameWidth = (float)theFrameWidth;
		mFrameHeight = (float)theFrameHeight;
		BuildFrames(theFrameWidth, theFrameHeight, theFrameCount, theFramesPerSecond);
	}
	return aLoaded;
}

// 0x42c7d0
bool ImageRecord::LoadPixels()
{
	mStride = mImage->GetWidth() * 4;
	mPixels = (uint32_t*)mImage->GetBits();
	return mPixels != nullptr;
}

// 0x42c810: the frames are read from the sheet a row at a time
bool ImageRecord::BuildFrames(int theFrameWidth, int theFrameHeight, int theFrameCount, int theFramesPerSecond)
{
	mFrameRects = new FrameRect[theFrameCount];
	mFrameCount = theFrameCount;
	if (theFramesPerSecond != 0) {
		mFrameDuration = 1000.0f / (float)theFramesPerSecond;
		mFrameDuration = mFrameDuration / 4.0f;
	}
	int aTop = 0;
	int aFrame = 0;
	while ((float)(aTop + theFrameHeight) <= mHeight && aFrame < theFrameCount) {
		int aLeft = 0;
		for (; (float)(aLeft + theFrameWidth) <= mWidth && aFrame < theFrameCount; aFrame++) {
			mFrameRects[aFrame].mLeft = (float)aLeft;
			mFrameRects[aFrame].mRight = (float)(aLeft + theFrameWidth);
			mFrameRects[aFrame].mTop = (float)aTop;
			mFrameRects[aFrame].mBottom = (float)(aTop + theFrameHeight);
			aLeft = aLeft + theFrameWidth;
		}
		aTop = aTop + theFrameHeight;
	}
	return true;
}

// 0x42c990
ImageRecord* AcquireImageRecord(const char* theName, int theFrameWidth, int theFrameHeight, int theFrameCount,
	int theFramesPerSecond)
{
	bool aMore = gImageRecords.Begin();
	while (aMore) {
		if (strcmp(theName, gImageRecords.mCursor->mName) == 0) {
			gImageRecords.mCursor->mReferenceCount++;
			return gImageRecords.mCursor;
		}
		aMore = gImageRecords.Next();
	}
	gImageRecords.Append(new ImageRecord());
	bool aLoaded = gImageRecords.mCursor->Load(theName, theFrameWidth, theFrameHeight, theFrameCount,
		theFramesPerSecond);
	return aLoaded ? gImageRecords.mCursor : nullptr;
}

// 0x42ca40
void ReleaseImageRecord(ImageRecord* theRecord)
{
	bool aMore = gImageRecords.Begin();
	while (aMore) {
		if (gImageRecords.mCursor == theRecord) {
			gImageRecords.RemoveCursor();
			return;
		}
		aMore = gImageRecords.Next();
	}
	if (theRecord != nullptr)
		delete theRecord;
}
