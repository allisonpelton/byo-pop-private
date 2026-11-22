// BlendedImage.cpp: see BlendedImage.h.

#include "BlendedImage.h"

#include "Graphics.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

#include <cmath>

using namespace Sexy;

// The copy for an offset is drawn this far to the left and above the picture's corner, less the offset's quarters (the
// original's ModValFloat at BlendedImage.cpp line 52; the call at line 51, 0.1, has its result thrown away).
static const float COPY_BASE_OFFSET = -1.0f;
static const double OFFSET_STEP = 0.25;
static const int NUM_OFFSETS = 4;

// 0x406960
BlendedImage::BlendedImage()
	: mSource(nullptr)
{
	for (int x = 0; x < NUM_OFFSETS; x++)
		for (int y = 0; y < NUM_OFFSETS; y++)
			mOffsetImages[x][y] = nullptr;
}

// 0x4069e0
BlendedImage::~BlendedImage()
{
	DeleteImages();
}

// 0x401e70
void BlendedImage::DeleteImages()
{
	for (int x = 0; x < NUM_OFFSETS; x++)
	{
		for (int y = 0; y < NUM_OFFSETS; y++)
		{
			delete mOffsetImages[x][y];
			mOffsetImages[x][y] = nullptr;
		}
	}
}

// 0x4069b0
void BlendedImage::Create(Image* theSource, const Rect& theRect)
{
	DeleteImages();
	mSource = theSource;
	mRect = theRect;
}

// 0x408ff0
void BlendedImage::Create(Image* theSource)
{
	Create(theSource, Rect(0, 0, theSource->mWidth, theSource->mHeight));
}

// 0x401d30: the picture drawn a number of quarters of a pixel to the right and down
void BlendedImage::MakeOffsetImage(int theXQuarter, int theYQuarter)
{
	MemoryImage* aImage = new MemoryImage(gSexyAppBase);
	aImage->Create(mRect.mWidth, mRect.mHeight);
	Graphics g(aImage);

	float aBase = COPY_BASE_OFFSET;
	float aX = (float)((double)(theXQuarter + 1) * OFFSET_STEP + (double)aBase);
	float aY = (float)((double)(theYQuarter + 1) * OFFSET_STEP + (double)aBase);
	g.DrawImageF(mSource, aX, aY, mRect);
	mOffsetImages[theXQuarter][theYQuarter] = aImage;
}

// 0x401eb0
void BlendedImage::Draw(Graphics* g, float theX, float theY)
{
	// the offsets are the fractions of the position, in quarters
	float aFloorX = (float)floor((double)theX);
	int aXQuarter = (int)(((double)theX - (double)aFloorX) * 4.0);
	float aFloorY = (float)floor((double)theY);
	int aYQuarter = (int)(((double)theY - (double)aFloorY) * 4.0);

	if (mOffsetImages[aXQuarter][aYQuarter] == nullptr)
		MakeOffsetImage(aXQuarter, aYQuarter);
	g->DrawImage(mOffsetImages[aXQuarter][aYQuarter], (int)theX, (int)theY);
}
