// The pictures that the loading procedure of Rocket Mania Deluxe makes from the loaded ones (RocketManiaGame
// 0x42bdb0, its second half, 0x42c0b0-0x42d0a0): the checkbox halves, the pop-up bubble turned around, the magic
// rocket in four hues, the 140 pipe tiles (five rows of four, each in seven colours) and the talking head with
// each of its mouths. See docs/GAME_NOTES.md.

#include "RocketApp.h"
#include "Graphics.h"
#include "MemoryImage.h"

#include <cmath>

using namespace Sexy;

// 0x416fd0: red, green and blue as 0 to 255 to hue (degrees, -1 for no hue), saturation and value (0 to 1)
static void RGBToHSV(double r, double g, double b, double* h, double* s, double* v)
{
	double aR = r * 0.00392156862745098;
	double aG = g * 0.00392156862745098;
	double aB = b * 0.00392156862745098;

	double aMax = aB;
	if (aB < aG)
		aMax = aG;
	double aTop = aR;
	if (aR <= aMax)
	{
		aTop = aG;
		if (aG <= aB)
			aTop = aB;
	}
	double aMin = aB;
	if (aG < aB)
		aMin = aG;
	double aBottom = aR;
	if (aMin <= aR)
	{
		aBottom = aB;
		if (aG < aB)
			aBottom = aG;
	}
	double aDelta = aTop - aBottom;

	*v = aTop;
	if (aTop != 0.0)
	{
		*s = aDelta / aTop;
		if (aDelta / aTop != 0.0)
		{
			// (when no channel equals the top one, which only a NaN does, the original scales what *h held)
			if (aR == aTop)
				*h = (aG - aB) / aDelta;
			else if (aG == aTop)
				*h = (aB - aR) / aDelta + 2.0;
			else if (aB == aTop)
				*h = (aR - aG) / aDelta + 4.0;

			*h = *h * 60.0;
			if (*h >= 0.0)
				return;
			*h = *h + 360.0;
			return;
		}
	}
	else
		*s = 0.0;

	*h = -1.0;
}

// 0x417140: hue (degrees, -1 for none), saturation and value to red, green and blue as 0 to 255
static void HSVToRGB(double h, double s, double v, double* r, double* g, double* b)
{
	double aR, aG, aB;
	if (s == 0.0)
	{
		if (h == -1.0)
			aR = aG = aB = v;
		else
			aR = aG = aB = 0.0;
	}
	else
	{
		if (h == 360.0)
			h = 0.0;
		int aSector = (int) floor(h * 0.016666666666666666);
		double aFraction = h * 0.016666666666666666 - (double) aSector;
		double aP = (1.0 - s) * v;
		double aQ = (1.0 - s * aFraction) * v;
		double aT = (1.0 - (1.0 - aFraction) * s) * v;
		aR = aP;
		aG = aP;
		aB = aP;
		switch (aSector)
		{
		case 0:
			aR = v;
			aG = aT;
			break;
		case 1:
			aR = aQ;
			aG = v;
			break;
		case 2:
			aG = v;
			aB = aT;
			break;
		case 3:
			aG = aQ;
			aB = v;
			break;
		case 4:
			aR = aT;
			aB = v;
			break;
		case 5:
			aR = v;
			aB = aQ;
			break;
		default:
			aR = v;
			aG = v;
			aB = v;
			break;
		}
	}
	*r = aR * 255.0;
	*g = aG * 255.0;
	*b = aB * 255.0;
}

// 0x46dc69: mirror every row of the picture (the original casts to a memory image and swaps the pixels in place)
static void FlipHorizontal(MemoryImage* theImage)
{
	ulong* aRow = theImage->GetBits();
	int aWidth = theImage->mWidth;
	for (int y = 0; y < theImage->mHeight; y++)
	{
		ulong* aLeft = aRow;
		ulong* aRight = aRow + aWidth - 1;
		for (int aCount = aWidth >> 1; aCount > 0; aCount--)
		{
			ulong aPixel = *aLeft;
			*aLeft = *aRight;
			*aRight = aPixel;
			aRight--;
			aLeft++;
		}
		aRow += aWidth;
	}
	theImage->BitsChanged();
}

// 0x46dcef: mirror every column
static void FlipVertical(MemoryImage* theImage)
{
	ulong* aColumn = theImage->GetBits();
	int aWidth = theImage->mWidth;
	int aHeight = theImage->mHeight;
	for (int x = aWidth; x > 0; x--)
	{
		ulong* aBottom = aColumn + (aHeight - 1) * aWidth;
		ulong* aTop = aColumn;
		for (int aCount = aHeight >> 1; aCount > 0; aCount--)
		{
			ulong aPixel = *aTop;
			*aTop = *aBottom;
			*aBottom = aPixel;
			aBottom -= aWidth;
			aTop += aWidth;
		}
		aColumn++;
	}
	theImage->BitsChanged();
}

// the hues that the rocket's four pictures are turned by (0x4e6d60)
static const double gRocketHueShifts[4] = {20.0, 100.0, 210.0, 320.0};

// the hues that the pipe tiles' colours 1 to 5 are set to (the table built on the stack at 0x42cd9e); colour 0 keeps
// the picture's hue and colour 6 is a grey
static const double gTileHues[7] = {-1.0, 45.75, 28.75, 0.25, 337.25, 100.0, -1.0};

// the second half of 0x42bdb0
void RocketApp::GenerateImages()
{
	// the checkbox picture holds the two states next to each other
	mCheckUnchecked = CopyImage(mImages[IMAGE_CHECKBOXES], Rect(0, 0, 24, 24));
	mCheckChecked = CopyImage(mImages[IMAGE_CHECKBOXES], Rect(24, 0, 24, 24));

	// the pop-up bubble mirrored both ways
	Image* aBubble = mImages[IMAGE_POPUPBUBBLE];
	mBubbleFlipH = CopyImage(aBubble, Rect(0, 0, aBubble->GetWidth(), aBubble->GetHeight()));
	FlipHorizontal(mBubbleFlipH);
	mBubbleFlipV = CopyImage(aBubble, Rect(0, 0, aBubble->GetWidth(), aBubble->GetHeight()));
	FlipVertical(mBubbleFlipV);

	// the magic rocket in four hues
	Image* aRocket = mImages[IMAGE_ROCKETX];
	for (int aHue = 0; aHue < 4; aHue++)
	{
		MemoryImage* anImage = new MemoryImage(this);
		anImage->Create(aRocket->GetWidth(), aRocket->GetHeight());
		anImage->SetImageMode(true, true);
		{
			Graphics g(anImage);
			g.DrawImage(aRocket, 0, 0);
		}
		mRocketHues[aHue] = anImage;

		ulong* aBits = anImage->GetBits();
		int aCount = anImage->GetWidth() * anImage->GetHeight();
		for (int i = 0; i < aCount; i++)
		{
			ulong aPixel = aBits[i];
			double aHueAngle, aSaturation, aValue;
			RGBToHSV((double) (aPixel & 0xff), (double) ((aPixel >> 8) & 0xff), (double) ((aPixel >> 16) & 0xff),
				&aHueAngle, &aSaturation, &aValue);
			if (aHueAngle != -1.0)
			{
				aHueAngle = aHueAngle - gRocketHueShifts[aHue];
				if (aHueAngle < 0.0)
					aHueAngle = aHueAngle + 360.0;
				double aR, aG, aB;
				HSVToRGB(aHueAngle, aSaturation, aValue, &aR, &aG, &aB);
				aBits[i] = (((int) aB << 8 | (int) aG) << 8) | (aPixel & 0xff000000) | (int) aR;
			}
		}
		anImage->BitsChanged();
		mCompletedLoadingThreadTasks++;
	}

	// the pipe tiles: for each of the pipes picture's 5 x 4 pieces, seven pictures of the tile's background with the
	// piece drawn over it in seven colours
	for (int aRow = 0; aRow < NUM_PIECE_ROWS; aRow++)
	{
		for (int aColumn = 0; aColumn < NUM_PIECE_COLUMNS; aColumn++)
		{
			MemoryImage* aPiece = new MemoryImage(this);
			aPiece->Create(42, 42);
			aPiece->SetImageMode(true, true);
			Graphics aPieceGraphics(aPiece);

			for (int aVariant = 0; aVariant < NUM_PIECE_VARIANTS; aVariant++)
			{
				MemoryImage* aTile = new MemoryImage(this);
				aTile->Create(42, 42);
				aTile->SetImageMode(true, true);
				Graphics aTileGraphics(aTile);
				aTileGraphics.DrawImage(aRow == 4 ? mImages[IMAGE_TILE] : mImages[IMAGE_TILE2], 0, 0);

				aPiece->Clear();
				aPieceGraphics.DrawImage(mImages[IMAGE_PIPES], 0, 0, Rect(aColumn * 42, aRow * 42, 42, 42));
				mPieces[aRow][aColumn][aVariant] = aTile;

				ulong* aBits = aPiece->GetBits();
				int aCount = aPiece->GetWidth() * aPiece->GetHeight();
				if (aVariant == 6)
				{
					// the grey: the brightest channel of every pixel
					for (int i = 0; i < aCount; i++)
					{
						ulong aPixel = aBits[i];
						ulong aLow = aPixel & 0xff;
						ulong aMiddle = (aPixel >> 8) & 0xff;
						ulong aHigh = (aPixel >> 16) & 0xff;
						ulong aBrightest = aHigh;
						if (aBrightest < aMiddle)
							aBrightest = aMiddle;
						if (aLow <= aBrightest)
						{
							aBrightest = aMiddle;
							if (aMiddle <= aHigh)
								aBrightest = aHigh;
						}
						else
							aBrightest = aLow;
						aBits[i] = ((aBrightest << 8 | aBrightest) << 8) | (aPixel & 0xff000000) | aBrightest;
					}
				}
				else
				{
					for (int i = 0; i < aCount; i++)
					{
						ulong aPixel = aBits[i];
						double aHueAngle, aSaturation, aValue;
						RGBToHSV((double) (aPixel & 0xff), (double) ((aPixel >> 8) & 0xff),
							(double) ((aPixel >> 16) & 0xff), &aHueAngle, &aSaturation, &aValue);
						if (aHueAngle != -1.0)
						{
							if (aVariant != 0)
								aHueAngle = gTileHues[aVariant];
							double aR, aG, aB;
							HSVToRGB(aHueAngle, aSaturation, aValue, &aR, &aG, &aB);
							// the first and third channels come back in the other order than they went in
							aBits[i] = (((int) aR << 8 | (int) aG) << 8) | (aPixel & 0xff000000) | (int) aB;
						}
					}
				}
				aPiece->BitsChanged();
				aTileGraphics.DrawImage(aPiece, 0, 0);
				mCompletedLoadingThreadTasks++;
			}
			delete aPiece;
		}
	}

	// the talking head with each of its seven mouths; the last two also keep the fifth mouth
	Image* aHead = mImages[IMAGE_TALKINGHEAD];
	int aHeadWidth = aHead->GetWidth();
	int aHeadHeight = aHead->GetHeight();
	for (int aFrame = 0; aFrame < 7; aFrame++)
	{
		MemoryImage* anImage = new MemoryImage(this);
		anImage->Create(aHeadWidth, aHeadHeight);
		anImage->SetImageMode(true, true);
		Graphics g(anImage);
		g.DrawImage(aHead, 0, 0);
		g.DrawImage(mImages[IMAGE_TALKINGHEADMOUTHS], 0, 0, Rect(0, aHeadHeight * aFrame, aHeadWidth, aHeadHeight));
		if (aFrame == 5 || aFrame == 6)
			g.DrawImage(mImages[IMAGE_TALKINGHEADMOUTHS], 0, 0, Rect(0, aHeadHeight * 4, aHeadWidth, aHeadHeight));
		mHeadFrames[aFrame] = anImage;
		mCompletedLoadingThreadTasks++;
	}

	mHeadRotate1 = mImages[IMAGE_HEADROTATE1];
	mCompletedLoadingThreadTasks++;
	mHeadRotate2 = mImages[IMAGE_HEADROTATE2];
	mCompletedLoadingThreadTasks++;
}
