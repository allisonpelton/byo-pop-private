#include "PuzzleImages.h"
#include "PixelusApp.h"
#include "Grid.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include <math.h>
#include <algorithm>

using namespace Sexy;

// 0x4e9590 and 0x4e9730: RGB (0-255) to hue (0-6), saturation and value (0-1) and back
static void RgbToHsv(double theRed, double theGreen, double theBlue, double* theHue, double* theSaturation, double* theValue)
{
	double aMax = std::max(theRed, std::max(theGreen, theBlue));
	double aMin = std::min(theRed, std::min(theGreen, theBlue));
	double aDelta = aMax - aMin;
	*theValue = aMax / 255.0;
	*theSaturation = aMax > 0 ? aDelta / aMax : 0;
	*theHue = 0;
	if (aDelta <= 0)
		return;
	if (aMax == theRed)
		*theHue = (theGreen - theBlue) / aDelta;
	else if (aMax == theGreen)
		*theHue = 2 + (theBlue - theRed) / aDelta;
	else
		*theHue = 4 + (theRed - theGreen) / aDelta;
	if (*theHue < 0)
		*theHue += 6;
}

// port: a helper of the port (HsvToRgb)
static void HsvToRgb(double theHue, double theSaturation, double theValue, double* theRed, double* theGreen, double* theBlue)
{
	int i = (int) floor(theHue) % 6;
	double f = theHue - floor(theHue);
	double p = theValue * (1 - theSaturation), q = theValue * (1 - theSaturation * f), t = theValue * (1 - theSaturation * (1 - f));
	switch (i)
	{
	case 0: *theRed = theValue; *theGreen = t; *theBlue = p; break;
	case 1: *theRed = q; *theGreen = theValue; *theBlue = p; break;
	case 2: *theRed = p; *theGreen = theValue; *theBlue = t; break;
	case 3: *theRed = p; *theGreen = q; *theBlue = theValue; break;
	case 4: *theRed = t; *theGreen = p; *theBlue = theValue; break;
	default: *theRed = theValue; *theGreen = p; *theBlue = q; break;
	}
	*theRed *= 255; *theGreen *= 255; *theBlue *= 255;
}

// 0x426910
bool Sexy::LoadPuzzlePicture(PixelusApp* theApp, Grid* theGrid)
{
	theGrid->mHasPicture = false;
	if (theGrid->mName.empty())
		return false;
	DDImage* anImage = theApp->GetImage("images\\levels\\" + theGrid->mName, false);
	if (anImage == NULL)
		return false;
	const unsigned int* aBits = (const unsigned int*) anImage->GetBits();
	if (aBits == NULL || anImage->GetWidth() < theGrid->mCols || anImage->GetHeight() < theGrid->mRows)
	{
		delete anImage;
		return false;
	}

	int aWidth = anImage->GetWidth();
	for (int r = 0; r < theGrid->mRows; r++)
		for (int c = 0; c < theGrid->mCols; c++)
		{
			unsigned int aPixel = aBits[r * aWidth + c];
			double h, s, v, red, green, blue;
			RgbToHsv((aPixel >> 16) & 0xff, (aPixel >> 8) & 0xff, aPixel & 0xff, &h, &s, &v);
			s = std::min(1.0, s * 1.33);
			v = std::min(1.0, v * 1.1);
			HsvToRgb(h, s, v, &red, &green, &blue);
			theGrid->mPicture[r][c] = 0xff000000 | ((unsigned int) red << 16) | ((unsigned int) green << 8) | (unsigned int) blue;
		}
	theGrid->mHasPicture = true;
	delete anImage;
	return true;
}

// the mini Grid::Draw (0x422ab0 with the mini flag, the second and third arguments 1 0 for the shape, 1 1 for the picture).
// The picture colours every cell by the puzzle's gif. The shape draws the back of each cell that holds no tile at rest (a
// wanted cell's, with the sticky spot over it where there is one), then the tiles (0x42ed50): a block is blue-grey, a tile on
// a cell nobody wants red, on a wanted cell pale cyan (the colours at 0x542d30, 0x542d40 and the grid's +0x20)
MemoryImage* Sexy::MakeMiniImage(PixelusApp* theApp, const Grid& theGrid, bool thePicture)
{
	const int CELL = 6;
	MemoryImage* anImage = new MemoryImage(theApp);
	anImage->Create(theGrid.mCols * CELL, theGrid.mRows * CELL);
	anImage->SetImageMode(true, true);

	Graphics g(anImage);
	if (thePicture && theGrid.mHasPicture)
	{
		g.SetColorizeImages(true);
		for (int r = 0; r < theGrid.mRows; r++)
			for (int c = 0; c < theGrid.mCols; c++)
			{
				unsigned int aColor = theGrid.mPicture[r][c];
				g.SetColor(Color((aColor >> 16) & 0xff, (aColor >> 8) & 0xff, aColor & 0xff));
				g.DrawImage(theApp->mImages[IMAGE_GRAYTILE_6X6], c * CELL, r * CELL);
			}
		return anImage;
	}

	for (int r = 0; r < theGrid.mRows; r++)
		for (int c = 0; c < theGrid.mCols; c++)
		{
			const Grid::Cell& aCell = theGrid.mCells[r][c];
			if (aCell.mObject != NULL && aCell.mObject->mState == 0)
				continue;
			int aX = c * CELL, aY = r * CELL;
			g.DrawImage(theApp->mImages[aCell.mWanted ? IMAGE_TILEBACK_WANTTILE_6X6 : IMAGE_TILEBACK_6X6], aX, aY);
			if (aCell.mStart)
				g.DrawImage(theApp->mImages[IMAGE_STICKYSPOT_6X6], aX, aY);
		}
	g.SetColorizeImages(true);
	for (size_t i = 0; i < theGrid.mObjects.size(); i++)
	{
		const GridObject* anObject = theGrid.mObjects[i];
		if (!theGrid.InGrid(anObject->mRow, anObject->mCol))
			continue;
		const Grid::Cell& aCell = theGrid.mCells[anObject->mRow][anObject->mCol];
		Color aColor = aCell.mFixed ? Color(142, 170, 192) : (aCell.mWanted ? Color(203, 255, 255) : Color(255, 80, 80));
		g.SetColor(aColor);
		g.DrawImage(theApp->mImages[IMAGE_GRAYTILE_6X6], anObject->mCol * CELL, anObject->mRow * CELL);
	}
	return anImage;
}

MemoryImage* Sexy::MakeSolvedImage(PixelusApp* theApp, const Grid& theGrid)
{
	const int CELL = Grid::CELL_SIZE;
	MemoryImage* anImage = new MemoryImage(theApp);
	anImage->Create(theGrid.mCols * CELL, theGrid.mRows * CELL);
	anImage->SetImageMode(true, true);

	Graphics g(anImage);
	for (int r = 0; r < theGrid.mRows; r++)
		for (int c = 0; c < theGrid.mCols; c++)
		{
			if (!theGrid.mCells[r][c].mWanted && !theGrid.mHasPicture)
				continue;
			unsigned int aColor = theGrid.mHasPicture ? theGrid.mPicture[r][c] : 0xffcccccc;
			g.SetColorizeImages(true);
			g.SetColor(Color((aColor >> 16) & 0xff, (aColor >> 8) & 0xff, aColor & 0xff));
			g.DrawImage(theApp->mImages[IMAGE_GRAYTILE_20X20], c * CELL, r * CELL);
		}
	return anImage;
}
