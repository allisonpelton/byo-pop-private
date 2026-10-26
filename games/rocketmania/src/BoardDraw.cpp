// The Board's drawing (RocketManiaGame 0x41f540 and the helpers 0x41d420-0x41ec20). First part: the background, the grid's
// tiles, the tiles that wait above the board and the pieces that fall. Not translated yet: the tilted tile 0x41e630, the
// overlay 0x41e690, the edge pieces 0x41ec20, the specials and effects 0x41d670, the score and the bars (the rest of
// 0x41f540, 0x41ef20, 0x41f0b0, 0x421b80). Positions are ftol(mX) and ftol(mY) of a piece (the first __ftol of a pair is y).

#include "Board.h"
#include "RocketApp.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "MTRand.h"
#include "WidgetManager.h"

using namespace Sexy;

static int Ftol(double theValue)
{
	return (int) theValue;
}

// 0x41f540
void Board::Draw(Graphics* g)
{
	if (mFc == 1)
		return;
	if (m358 == 1.0)
		return;
	if (m350 == 0 && 0 < m4b4 && (mState == 0 || mState == 4 || mState == 5 || mState == 1))
		Resize(CrtRand() % (m4b8 * 2 + 1) - m4b8, 0, 640, 480);
	else
		Resize(0, 0, 640, 480);

	Graphics aClip(*g);
	aClip.ClipRect(0xb0, 4, 0x1d0, 0x1d5);

	Image** anImages = mApp->mImages;
	double aDawn = 1.0 - m318 / m68;
	if (aDawn <= 0.5 || m4e0 != NULL || mApp->GetDialog(0xb) != NULL)
		g->DrawImage(anImages[IMAGE_NIGHTSKY], 0x3f, 0);
	else
	{
		int anAlpha = Ftol((aDawn - 0.5) * 510.0);
		if (anAlpha < 0xfe)
		{
			g->DrawImage(anImages[IMAGE_NIGHTSKY], 0x3f, 0);
			if (4 < anAlpha)
			{
				g->SetColorizeImages(true);
				g->SetColor(Color(255, 255, 255, anAlpha));
				g->DrawImage(anImages[IMAGE_DAYSKY], 0x3f, 0);
				g->SetColorizeImages(false);
			}
		}
		else
			g->DrawImage(anImages[IMAGE_DAYSKY], 0x3f, 0);
		if (200 < anAlpha)
			g->DrawImage(anImages[IMAGE_SUN], anAlpha / 8 + 0x5f, 0x228 - anAlpha);
	}
	g->DrawImage(anImages[IMAGE_CITY], 0x74, 0x13d);
	g->DrawImage(anImages[IMAGE_GROUND], 0x74, 0x17e);
	// (the effects behind the board, 0x41abb0 with the flag set, are not translated yet)
	g->DrawImage(anImages[IMAGE_COLUMN], 0, 0);
	// (the talking head, 0x416d50, is not translated yet)
	if (mApp->mMode == 2)
		g->DrawImage(anImages[IMAGE_MATCHESLEFT], 0, 0xa6);
	g->DrawImage(anImages[IMAGE_POLELEFT], 0xc3, 0x18);
	g->DrawImage(anImages[IMAGE_POLERIGHT], 0x1ec, 0x18);

	if (mApp->mMode == 0)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			if (mTop[aColumn] != NULL)
				DrawTop(&aClip, mTop[aColumn]);
		}
		if (mSpare != NULL)
			DrawSpare(&aClip, mSpare);
	}

	for (int aColumn = 0; aColumn < mCols; aColumn++)
	{
		int aHint = 0;
		if (mApp->mMode != 1 && mApp->mMode != 2)
		{
			if (mGrid[0][aColumn] != NULL && mGrid[0][aColumn]->mType == 1)
				aHint = 2;
			else if (mGrid[1][aColumn] != NULL && mGrid[1][aColumn]->mType == 1)
				aHint = 1;
		}
		for (int aRow = mRows - 1; aRow >= 0; aRow--)
		{
			if (mGrid[aRow][aColumn] != NULL)
				DrawTile(&aClip, mGrid[aRow][aColumn], aHint);
		}
	}

	for (int aRow = 0; aRow < mRows; aRow++)
	{
		if (mLeft[aRow] != NULL)
			DrawEdge(&aClip, mLeft[aRow], aRow, 0);
		if (mRight[aRow] != NULL)
			DrawEdge(&aClip, mRight[aRow], aRow, mCols - 1);
	}

	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); ++anItr)
		DrawMover(&aClip, *anItr);
}

// 0x41e770: a tile of the grid. The colour of its picture says how it is connected: no path the plain grey one (variant 6),
// a path that reaches the left edge variant 1 (a fuse there) or 2, one that reaches only the right edge variant 3 (a rocket
// there) or 4. A burning tile fades its lit picture (variant 0) in over the grey one and afterwards shows variant 0, or 5
// once the flame has passed. Not translated: the tilted drawing (0x41e630, |m80| > 0.005), the overlay 0x41e690 and the
// ghosts of a tile that was moved.
void Board::DrawTile(Graphics* g, Piece* thePiece, int theHint)
{
	int aJitter = 0;
	if (m44c < 500 && theHint == 2 && m448 == thePiece->mColumn && mState != 1 && mState != 2 && m350 == 0)
		aJitter = CrtRand() % 3 - 1;
	if (thePiece->mType != 1 && thePiece->mType != 4)
		return;
	MemoryImage** aVariants = mApp->mPieces[thePiece->mKind][thePiece->mRotation];
	int aX = Ftol((double) aJitter + thePiece->mX);
	int aY = Ftol(thePiece->mY);
	if (thePiece->mType == 4)
	{
		if (m2ec < 0)
			g->DrawImage(aVariants[thePiece->m60 ? 5 : 0], aX, aY);
		else
		{
			g->DrawImage(aVariants[6], aX, aY);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, Ftol(255.0 - (double) m2ec * 0.015151515151515152 * 255.0)));
			g->DrawImage(aVariants[0], aX, aY);
			g->SetColorizeImages(false);
		}
		return;
	}
	int aVariant = 6;
	int aPath = thePiece->mPath;
	if (aPath != 0)
	{
		unsigned int aBits = (unsigned int) m384[aPath];
		if ((aBits & 1) != 0)
			aVariant = m3fc[aPath] ? 1 : 2;
		else if ((aBits & 2) != 0)
			aVariant = m41a[aPath] ? 3 : 4;
	}
	g->DrawImage(aVariants[aVariant], aX, aY);
}

// 0x41e3e0: a tile waiting above the board (arcade mode). The one over the column that is about to get the next tile
// brightens and sinks as the fall timer runs (it stops 32 pixels down when the column's top cell is taken); the others stay dim.
void Board::DrawTop(Graphics* g, Piece* thePiece)
{
	g->SetColorizeImages(true);
	int aOffset = 0;
	if (m448 == thePiece->mColumn)
	{
		double aFraction = (double) m2e0 / (double) m444;
		int aFallen = Ftol(42.0 * aFraction);
		if (mGrid[0][thePiece->mColumn] == NULL || aFallen < 0x21)
		{
			int aShade = Ftol(aFraction * 155.0 + 100.0);
			g->SetColor(Color(aShade, aShade, aShade));
			aOffset = aFallen;
		}
		else
		{
			g->SetColor(Color(100, 100, 100));
			aOffset = 0x20;
		}
	}
	else
		g->SetColor(Color(100, 100, 100));
	int aWarning = m44c % 100;
	if (m44c < 500)
		g->SetColor(Color(0xff - aWarning, 99 - aWarning, 99 - aWarning));
	g->DrawImage(mApp->mPieces[thePiece->mKind][thePiece->mRotation][6], Ftol(thePiece->mX), Ftol(thePiece->mY + (double) aOffset));
	g->SetColorizeImages(false);
}

// 0x41e530: the next tile (arcade mode)
void Board::DrawSpare(Graphics* g, Piece* thePiece)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(100, 100, 100));
	int aWarning = m44c % 100;
	if (m44c < 500)
		g->SetColor(Color(0xff - aWarning, 99 - aWarning, 99 - aWarning));
	g->DrawImage(mApp->mPieces[thePiece->mKind][thePiece->mRotation][6], Ftol(thePiece->mX), Ftol(thePiece->mY));
	g->SetColorizeImages(false);
}

// 0x41d620: a rocket's picture (the magic rocket of the strategy mode in four hues, else the plain rockets), at x, y
void Board::DrawRocket(Graphics* g, int theIndex, int theX, int theY)
{
	static_assert(IMAGE_ROCKET01 == 54, "the rocket pictures follow image 53");
	if (mApp->mMode == 2)
		g->DrawImage(mApp->mRocketHues[theIndex], theX, theY);
	else
		g->DrawImage(mApp->mImages[IMAGE_ROCKET01 + theIndex], theX, theY);
}

// 0x41ec20: the fuses on the left and the rocket launchers on the right. A fuse is a frame of the ten-frame flame strip,
// and the frame depends on the row's height, so that the fuses do not flicker in step. A rocket fades in over its first 155
// updates; it shows dimmer when the tile next to it does not open towards it.
void Board::DrawEdge(Graphics* g, Piece* thePiece, int theRow, int theColumn)
{
	Piece* aTile = mGrid[theRow][theColumn];
	g->SetColorizeImages(true);
	bool aConnected = false;
	if (aTile != NULL && (aTile->mType == 1 || aTile->mType == 4))
	{
		unsigned int aMask = gPipeMask[aTile->mRotation + aTile->mKind * 4];
		if ((theColumn == 0 && (aMask & 8) != 0) || (theColumn == mCols - 1 && (aMask & 2) != 0))
			aConnected = true;
	}
	g->SetColor(Color(255, 255, 255));
	if (theColumn == 0)
	{
		Image* aStrip = mApp->mImages[IMAGE_MATCHFLAME];
		int aWidth = aStrip->GetWidth();
		int aHeight = aStrip->GetHeight();
		int aY = Ftol(thePiece->mY);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aFrame = ((mUpdateCnt % 100) / 10 + aY % 7 + aY % 5 + aY % 3) % 10;
		g->DrawImage(aStrip, Ftol((double) theColumn + thePiece->mX - 7.0), Ftol((double) theColumn + thePiece->mY - 8.0),
			Rect(aFrame * (aWidth / 10), 0, aWidth / 10, aHeight));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
		return;
	}
	int anAge = mUpdateCnt - thePiece->mBorn;
	if (anAge < 155)
	{
		if (aConnected)
			g->SetColor(Color(255, 255, 255, anAge + 100));
		else
			g->SetColor(Color(170, 170, 170, anAge + 100));
		g->SetColorizeImages(true);
		int aY = Ftol((double) theColumn * 0.2 + thePiece->mY - 77.0);
		int aX = Ftol((double) theColumn * 0.4 + thePiece->mX + 1.0);
		DrawRocket(g, thePiece->mIndex, aX, aY);
		g->SetColorizeImages(false);
		return;
	}
	int aY = Ftol(thePiece->mY - 77.0);
	int aX = Ftol(thePiece->mX + 1.0);
	DrawRocket(g, thePiece->mIndex, aX, aY);
	g->SetColorizeImages(false);
}

// 0x41d420: the pieces that move about, first pass. A falling tile is drawn greyed (colour 120, 100, 120); a flash is a
// picture strip's frame (kind 2 the 42-wide dust strip, the others the match-flame strip, frame m7c).
void Board::DrawMover(Graphics* g, Piece* thePiece)
{
	int aType = thePiece->mType;
	if (aType == 0 || aType == 2)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0x78, 100, 0x78));
		g->DrawImage(mApp->mPieces[thePiece->mKind][thePiece->mRotation][6], Ftol(thePiece->mX), Ftol(thePiece->mY));
		g->SetColorizeImages(false);
	}
	else if (aType == 3)
	{
		if (thePiece->mKind == 2)
		{
			Image* aStrip = mApp->mImages[IMAGE_DUST];
			g->DrawImage(aStrip, Ftol(thePiece->mX), Ftol(thePiece->mY), Rect(thePiece->m7c * 0x2a, 0, 0x2a, 0x2a));
		}
		else
		{
			Image* aStrip = mApp->mImages[IMAGE_MATCHFLAME];
			int aWidth = aStrip->GetWidth();
			if (thePiece->mKind == 1)
			{
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0, 0x80));
			}
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImage(aStrip, Ftol(thePiece->mX), Ftol(thePiece->mY),
				Rect(thePiece->m7c * (aWidth / 0xf), 0, aWidth / 0xf, aStrip->GetHeight()));
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			if (thePiece->mKind == 1)
				g->SetColorizeImages(false);
		}
	}
}
