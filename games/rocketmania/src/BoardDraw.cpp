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
#include "Font.h"
#include "ImageFont.h"
#include "GameText.h"

#include <cmath>
#include <cstdio>
#include <cstring>

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

	// the chest and the coins under it (not in strategy mode), then the specials that move (0x41d670: not translated yet)
	bool aBars = false;
	if (mState == 0 || mState == 7 || m4e0 != NULL || mState == 4 || mState == 5 || mState == 1 || mState == 6)
	{
		aBars = true;
		if (mApp->mMode != 2)
		{
			g->DrawImage(anImages[IMAGE_CHEST0 + ChestImage()], 0x89, 0x181);
			DrawCoins(g);
		}
	}
	if (aBars)
	{
		// the small count at the left: the matches left (strategy mode) or the rockets the level still needs
		char aBuf[32];
		g->SetFont(mApp->mFonts[FONT_DOMO11_BOLD]);
		g->SetColor(Color(0xff, 0xb6, 0x2b));
		int aNumber;
		int aRight;
		if (mApp->mMode == 2)
		{
			aNumber = m108;
			aRight = 0x30;
		}
		else
		{
			aNumber = m74 - m374;
			if (aNumber < 0)
				aNumber = 0;
			aRight = 0x23;
		}
		sprintf(aBuf, "%d", aNumber);
		g->DrawString(aBuf, aRight - g->GetFont()->StringWidth(aBuf), 0xc3);
	}

	// the score, a digit at a time in cells 13 pixels wide
	g->SetColor(Color(m2e4 == mScore ? 0xc0 : 0xde, 10, 10));
	if ((mState == 4 || mState == 5) && 0x32 < mUpdateCnt % 100)
		g->SetColor(Color(0xff, 0x14, 0x14));
	g->SetFont(mApp->mFonts[FONT_DOMO14_BOLD]);
	{
		char aBuf[32];
		sprintf(aBuf, "%d", m2e4);
		int aLength = (int) strlen(aBuf);
		int aX = 0x65 - ((8 - aLength) * 0xd) / 2;
		for (int anIndex = aLength; anIndex > 0; anIndex--)
		{
			g->DrawString(SexyString(1, aBuf[anIndex - 1]), aX, 0x62);
			aX -= 0xd;
		}
	}
	// (0x429230: the launch in progress is not translated yet)

	if (mApp->mMode != 2)
	{
		char aBuf[32];
		sprintf(aBuf, GTC(0x004e6654), mApp->mBoard->m100 ? mLevel + 2 : mLevel + 1);
		g->SetColor(Color(0xff, 0xff, 0x80, 200));
		g->SetFont(mApp->mFonts[FONT_ARIAL8_BOLD]);
		g->DrawString(aBuf, 0x40 - g->GetFont()->StringWidth(aBuf) / 2, 0x82);
	}
	g->SetFont(mApp->mFonts[FONT_ARIAL8_BOLD]);
	{
		SexyString aName = RocketApp::ModeName(mApp->mMode) + " " + RocketApp::DifficultyName(mApp->mDifficulty);
		int aY;
		if (mApp->mMode == 2)
		{
			g->SetColor(Color(0xff, 0xff, 0x80, 0xb4));
			aY = 0x8a;
		}
		else
		{
			g->SetColor(Color(0xff, 0xff, 0x80, 0xa0));
			aY = 0x92;
		}
		g->DrawString(aName, 0x40 - g->GetFont()->StringWidth(aName) / 2, aY);
	}

	// the highlight of the tile under the pointer
	if (mState == 0 && CellX(0) <= m4f4 && m4f4 < CellX(mCols) && CellY(0) <= m4f8 && m4f8 < CellY(mRows))
	{
		int aColumn = (m4f4 - CellX(0)) / 0x2a;
		int aRow = (m4f8 - CellY(0)) / 0x2a;
		if (mGrid[aRow][aColumn] != NULL)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImage(anImages[IMAGE_HIGHLIGHT], CellX(0) + aColumn * 0x2a, CellY(0) + aRow * 0x2a);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
	// (the frozen-time bar of classic mode 0x41f360, the effects 0x41abb0 and the talking head's bubble 0x414000 are not
	// translated yet)

	// the hint that points at the first tile to turn
	if (!m498 && !mApp->mNoHints)
	{
		int aRow = -1;
		if (mApp->mMode == 0)
		{
			bool aFull = true;
			for (int aColumn = 0; aColumn < 6; aColumn++)
				if (mGrid[8][aColumn] == NULL)
					aFull = false;
			if (aFull)
				aRow = 8;
		}
		else
			aRow = 5;
		if (aRow >= 0)
		{
			int aX = CellX(2) + 0x15;
			int aY = CellY(aRow);
			if (mUpdateCnt % 100 < 0x32)
			{
				Image* anArrow = anImages[IMAGE_HINTONPIECE];
				g->SetColorizeImages(true);
				g->SetColor(Color(255, 255, 255, 255));
				g->DrawImage(anArrow, aX - anArrow->GetWidth() / 2, (aY + 0x15) - anArrow->GetHeight() / 2);
				g->SetColorizeImages(false);
			}
			Image* aBubble = anImages[IMAGE_HINTBUBBLE];
			g->DrawImage(aBubble, aX - aBubble->GetWidth() / 2, ((aY + 0x15) - aBubble->GetHeight()) - 0xc);
		}
	}

	// the banners
	if (0 < m43c && (mLevel != 0 || mApp->mNoHints))
	{
		g->SetColor(Color(255, 255, 255));
		g->SetFont(mApp->mFonts[FONT_DOMO36_BOLD]);
		SexyString aReady = GT(0x004e6640);
		DrawWiggle(g, aReady, 0, 0, 0, 0, 2);
		DrawWiggle(g, aReady, 5, 0x40, 0x40, 0, 2);
		DrawWiggle(g, aReady, 10, 0x80, 0x80, 0, 2);
		DrawWiggle(g, aReady, 15, 0xff, 0xff, 0, 2);
	}
	if (mState == 1)
	{
		SexyString anOver = GT(m460 == 1 ? 0x004e6634 : m460 == 2 ? 0x004e6624 : 0x004e6614);
		g->SetFont(mApp->mFonts[FONT_DOMO36_BOLD]);
		DrawWiggle(g, anOver, 0, 0, 0, 0, 5);
		DrawWiggle(g, anOver, 5, 0x40, 0, 0, 5);
		DrawWiggle(g, anOver, 10, 0x80, 0, 0, 5);
		DrawWiggle(g, anOver, 15, 0xff, 0, 0, 5);
	}
	else if (mState == 4)
	{
		g->SetColor(Color(0xff, 0, 0));
		g->SetFont(mApp->mFonts[FONT_DOMO36_BOLD]);
		SexyString aUp = GT(0x004e6608);
		DrawWiggle(g, aUp, 0, 0, 0x1a, 0, 0);
		DrawWiggle(g, aUp, 5, 0x40, 0x34, 0, 0);
		DrawWiggle(g, aUp, 10, 0x80, 0x69, 0, 0);
		DrawWiggle(g, aUp, 15, 0xff, 0xff, 0xff, 0);
		if (m4d9 && mUpdateCnt % 100 < 0x50)
		{
			g->SetFont(mApp->mFonts[FONT_DOMO22_BOLD]);
			g->SetColor(Color(0, 0, 0));
			DrawOutlined(g, GT(0x004e65f8), -5, 2, 0x44, 0x42, 0x49);
			g->SetColor(Color(0xff, 0xff, 0xc8));
			SexyString aText = GT(0x004e65f8);
			g->DrawString(aText, CellX(3) - g->GetFont()->StringWidth(aText) / 2, 0x41);
		}
	}
	if (mState == 0 && 0 < m4b0)
	{
		g->SetFont(mApp->mFonts[FONT_DOMO36_BOLD]);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		DrawWiggle(g, GT(0x004e65f0), 0, 0xff, 0xff, 0xff, 10);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	// (the dimmed screen and the pause window, m350 > 0: not translated yet)
	if (mApp->mMode == 1 && mState == 0 && 0 < m4bc && m4bc < 500 && m4bc % 100 < 0x3c)
	{
		g->SetFont(mApp->mFonts[FONT_DOMO22_BOLD]);
		g->SetColor(Color(0, 0, 0));
		DrawOutlined(g, GT(0x004e65e4), -3, 2, 0x41, 0x3f, 0x44);
		g->SetColor(Color(0xff, 0x78, 0xff));
		SexyString aText = GT(0x004e65e4);
		g->DrawString(aText, CellX(3) - g->GetFont()->StringWidth(aText) / 2, 0x3e);
	}
	if (mApp->mMode == 2 && mState == 0 && 0 < m10c && m350 == 0 && m10c < 500 && m10c % 100 < 0x32)
	{
		g->SetColor(Color(0x80, 0x3c, 0x80));
		SexyString aText = GT(0x004e65d4);
		int aX = CellX(3) - g->GetFont()->StringWidth(aText) / 2;
		g->DrawString(aText, aX - 2, 0x28);
		g->DrawString(aText, aX + 2, 0x28);
		g->DrawString(aText, aX, 0x26);
		g->DrawString(aText, aX, 0x2a);
		g->SetColor(Color(0xff, 0x78, 0xff));
		g->DrawString(aText, aX - 1, 0x27);
	}
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

// 0x41f240: which of the chest's five pictures shows the coins collected
int Board::ChestImage()
{
	int aCoins = m8c[7];
	if (aCoins == 0)
		return 0;
	if (aCoins < 5)
		return 1;
	if (aCoins < 0xf)
		return 2;
	return (0x1d < aCoins) + 3;
}

// 0x421b80: the number of coins under the chest, and its label
void Board::DrawCoins(Graphics* g)
{
	char aBuf[32];
	g->SetColor(Color(0xed, 0xf8, 0xe8));
	g->SetFont(mApp->mFonts[FONT_DOMO14_BOLD]);
	sprintf(aBuf, "%d", m8c[7]);
	g->DrawString(aBuf, 0xa7 - g->GetFont()->StringWidth(aBuf) / 2, 0x1bf);
	g->SetColor(Color(0xd9, 0xe4, 0xd4));
	g->SetFont(mApp->mFonts[FONT_ARIAL8]);
	g->DrawString(GT(0x004e5080), 0x81, 0x1d4);
}

// 0x4219a0: a banner's text, centred over the board, wobbling with the update count (each call of a banner has its own
// phase and colour, so the copies trail each other)
void Board::DrawWiggle(Graphics* g, const SexyString& theText, int thePhase, int theRed, int theGreen, int theBlue, int theAmplitude)
{
	g->SetColor(Color(theRed, theGreen, theBlue));
	int aHalf = g->GetFont()->StringWidth(theText) / 2;
	double anAngle = (double) (mUpdateCnt + thePhase) * 0.06666666666666667;
	int aX = Ftol((double) (CellX(3) - aHalf) + cos(anAngle) * (double) theAmplitude);
	int aY = Ftol(sin(anAngle) * 10.0 + 220.0);
	g->DrawString(theText, aX, aY);
}

// the dark copies behind a text on the board: left and right at one height, above and below at others (the caller sets
// the colour; the text itself is drawn by the caller on top)
void Board::DrawOutlined(Graphics* g, const SexyString& theText, int theLeft, int theRight, int theSideY, int theAboveY, int theBelowY)
{
	int aX = CellX(3) - g->GetFont()->StringWidth(theText) / 2;
	g->DrawString(theText, aX + theLeft, theSideY);
	g->DrawString(theText, aX + theRight, theSideY);
	g->DrawString(theText, aX, theAboveY);
	g->DrawString(theText, aX, theBelowY);
}
