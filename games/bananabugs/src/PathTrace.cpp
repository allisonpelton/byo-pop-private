#include "PathTrace.h"

#include "DDImage.h"
#include "GameApp.h"
#include "ObjBase.h"

using namespace Sexy;

// What a pixel's colour means (ClassifyPixel's answers): the colour's red and green channels and its blue
static const int PIXEL_NOTHING = 0;
static const int PIXEL_BLUE = 1;
static const int PIXEL_GREEN = 2;
static const int PIXEL_OTHER = 3;
static const int PIXEL_START = 4;                // white
static const int CHANNEL_FULL = 0xff;

// The points' angles are the average of the angles to the neighbours either side of each point over this window: the ones this
// many before and after it
static const int SMOOTH_WINDOW = 12;

// 0x454a30: the app the picture is loaded through (the rest of the object is as the memory had it: the port clears it)
PathTrace::PathTrace()
{
	mCount = 0;
	for (int i = 0; i < PATH_MAX_POINTS; i++) {
		mRaw[i].mKind = 0;
		mRaw[i].mX = 0;
		mRaw[i].mY = 0;
		mX[i] = 0.0f;
		mY[i] = 0.0f;
		mAngle[i] = 0.0f;
		mPointKind[i] = 0;
	}
	mApp = ObjBase::sApp;
}

// 0x454a40: what the pixel's colour means: 4 for white, 3 for a colour with red and blue full (not white) or with any red
// else, 2 for no red and some green, 1 for no red or green and some blue, 0 for black or a place outside the picture
int PathTrace::ClassifyPixel(int theColumn, int theRow, int theWidth, const ulong* thePixels, int theHeight)
{
	if (theColumn > -1 && theColumn < theWidth && theRow > -1 && theRow < theHeight) {
		unsigned int aPixel = (unsigned int)thePixels[theRow * theWidth + theColumn];
		unsigned int aRed = (aPixel >> 16) & 0xff;
		unsigned int aGreen = (aPixel >> 8) & 0xff;
		unsigned int aBlue = aPixel & 0xff;
		if (aBlue == CHANNEL_FULL && aRed == CHANNEL_FULL) {
			if (aGreen == CHANNEL_FULL)
				return PIXEL_START;
		} else if (aRed == 0) {
			if (aGreen != 0)
				return PIXEL_GREEN;
			return aBlue != 0 ? PIXEL_BLUE : PIXEL_NOTHING;
		}
		return PIXEL_OTHER;
	}
	return PIXEL_NOTHING;
}

// 0x454ab0: the picture is read (by the app's loader, and thrown away after): each pixel that means something is a raw
// point, the start (white) swapped to the front; then from the start each next point is the first raw point (in order) that
// touches the last one, by a side or a corner, so the points are in the order of the line. Each point's angle is that of
// the line from the point before it to the one after (up is a quarter turn); the angles are averaged over 12 points either
// way; the points' places are made relative to the start. A bug of the original: a raw point that touches no other leaves the
// loop searching for ever (port: the search stops, and the path is as long as it got); and the original does not look whether
// the picture holds more than 5040 points (port: the rest are left)
void PathTrace::Load(const char* theName)
{
	mCount = 0;
	DDImage* anImage = mApp->GetImage(theName, true);
	ulong* aBits = anImage->GetBits();
	int aWidth = anImage->GetWidth();
	int aHeight = anImage->GetHeight();
	for (int aRow = 0; aRow < aHeight; aRow++) {
		for (int aColumn = 0; aColumn < aWidth; aColumn++) {
			int aKind = ClassifyPixel(aColumn, aRow, aWidth, aBits, aHeight);
			if (aKind > 0 && mCount < PATH_MAX_POINTS) {
				mRaw[mCount].mX = aColumn;
				mRaw[mCount].mY = aRow;
				mRaw[mCount].mKind = aKind;
				if (aKind == PIXEL_START && mCount != 0) {
					RawPixel aFirst = mRaw[0];
					mRaw[0] = mRaw[mCount];
					mRaw[mCount] = aFirst;
				}
				mCount++;
			}
		}
	}
	delete anImage;
	if (mCount <= 0)
		return;

	int aLastX = mRaw[0].mX;
	int aLastY = mRaw[0].mY;
	mPointKind[0] = mRaw[0].mKind;
	mRaw[0].mKind = 0;
	mX[0] = (float)aLastX;
	mY[0] = (float)aLastY;
	int anOrdered = 1;
	while (anOrdered < mCount) {
		bool aFound = false;
		for (int i = 0; i < mCount; i++) {
			if (mRaw[i].mKind == 0)
				continue;
			int aDeltaX = mRaw[i].mX - aLastX;
			if (aDeltaX < 0)
				aDeltaX = -aDeltaX;
			int aDeltaY = mRaw[i].mY - aLastY;
			if (aDeltaY < 0)
				aDeltaY = -aDeltaY;
			bool aTouches;
			if (aDeltaX == 1)
				aTouches = aDeltaY < 2;
			else
				aTouches = aDeltaY == 1 && aDeltaX < 2;
			if (aTouches) {
				aLastX = mRaw[i].mX;
				aLastY = mRaw[i].mY;
				mX[anOrdered] = (float)aLastX;
				mY[anOrdered] = (float)aLastY;
				mPointKind[anOrdered] = mRaw[i].mKind;
				mRaw[i].mKind = 0;
				anOrdered++;
				aFound = true;
				break;
			}
		}
		if (!aFound) {
			// port (hang): the original goes round again for ever
			mCount = anOrdered;
			break;
		}
	}

	static float sRawAngle[PATH_MAX_POINTS];
	for (int i = 0; i < mCount; i++) {
		int aBefore = i - 1;
		if (aBefore < 0)
			aBefore = 0;
		int anAfter = i + 1;
		if (anAfter >= mCount)
			anAfter = mCount - 1;
		sRawAngle[i] = FloatAtan2(mY[aBefore] - mY[anAfter], mX[anAfter] - mX[aBefore]);
	}
	for (int i = 0; i < mCount; i++) {
		int aFrom = i - SMOOTH_WINDOW;
		if (aFrom < 0)
			aFrom = 0;
		int aTo = i + SMOOTH_WINDOW;
		if (aTo >= mCount)
			aTo = mCount - 1;
		float aSum = 0.0f;
		for (int j = aFrom; j <= aTo; j++)
			aSum = sRawAngle[j] + aSum;
		mAngle[i] = (float)((double)aSum / (double)(aTo - aFrom + 1));
	}

	float aStartX = mX[0];
	float aStartY = mY[0];
	for (int i = 0; i < mCount; i++) {
		mX[i] = mX[i] - aStartX;
		mY[i] = mY[i] - aStartY;
	}
}
