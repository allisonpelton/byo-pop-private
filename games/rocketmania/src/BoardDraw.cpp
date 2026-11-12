// The Board's drawing (RocketManiaGame 0x41f540 and the helpers 0x41d420-0x41ec20). First part: the background, the grid's
// tiles, the tiles that wait above the board, the pieces that fall, the edge pieces, the specials, the classic mode's
// lanterns and the number of a launch. Positions are ftol(mX) and ftol(mY) of a piece (the first __ftol of a pair is y).

#include "Board.h"
#include "RocketApp.h"
#include "TalkingHead.h"
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
#include <cstdlib>
#include <cstring>

using namespace Sexy;

static int Ftol(double theValue)
{
	return (int) theValue;
}

// 0x41f540, with the two small counts at the left inline: 0x41ef20 (the matches left, strategy mode) and 0x41f0b0 (the rockets
// the level still needs)
void Board::Draw(Graphics* g)
{
	if (mFc == 1)
		return;
	// port: the original returns here once the dimming of a paused game is complete (m358 == 1.0) and leaves the last frame
	// in its back buffer; this port's screen does not keep it (the screenshot is black), so the frame is drawn once more
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
	// the effects of the layer behind the board (0x41abb0 with the flag set)
	mEffects->Draw(g, true);
	g->DrawImage(anImages[IMAGE_COLUMN], 0, 0);
	mApp->mHead->DrawHole(g);
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

	// the chest and the coins under it (not in strategy mode), then the specials
	bool aBars = false;
	if (mState == 0 || m4e0 != NULL || mState == 4 || mState == 5 || mState == 1 || mState == 6)
	{
		aBars = true;
		if (mApp->mMode != 2)
		{
			g->DrawImage(anImages[IMAGE_CHEST0 + ChestImage()], 0x89, 0x181);
			DrawCoins(g);
		}
	}
	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); ++anItr)
		DrawSpecial(g, *anItr);
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
	DrawLaunchInProgress(g);    // 0x429230

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
	if (mApp->mMode == 1)       // 0x42f6f0
		DrawNightBar(g);
	mEffects->Draw(g, false);
	mTip->Draw(g);      // 0x414000

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
	// the dimmed screen and the pause window (m350 > 0)
	if (0 < m350)
	{
		if (mApp->GetDialog(0xb) == NULL)
		{
			mApp->mHead->SkipDraw();
			g->SetColor(Color(0, 0, 0, Ftol(m358 * 210.0)));
			g->SetColorizeImages(true);
			g->FillRect(0, 0, 0x280, 0x1e0);
			g->SetColorizeImages(false);
		}
		if (mApp->GetDialog(8) == NULL && mApp->GetDialog(0) == NULL && mApp->GetDialog(1) == NULL)
		{
			g->SetColor(mApp->mDialogColors[0]);
			g->SetFont(mApp->mFonts[FONT_VERDANA18_BOLD]);
			g->DrawImageBox(Rect(0x94, 0x9b, 0x159, 0xaa), anImages[IMAGE_DIALOG]);
			SexyString aPaused = GT(0x004e50bc);
			g->DrawString(aPaused, 0x140 - g->GetFont()->StringWidth(aPaused) / 2, 0xd5);
			g->SetColor(mApp->mDialogColors[1]);
			g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
			SexyString aClick = GT(0x004e50a4);
			g->DrawString(aClick, 0x140 - g->GetFont()->StringWidth(aClick) / 2, 0x102);
			SexyString aResume = GT(0x004e5090);
			g->DrawString(aResume, 0x140 - g->GetFont()->StringWidth(aResume) / 2, 0x116);
		}
	}
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
// once the flame has passed. A tile that was hit (|m80| > 0.005) is drawn turned (0x41e630), a tile of the top rows gets the
// warning overlay (0x41e690), and a tile that was moved leaves ghosts along the way it came.
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
		if (0.005 < fabs(thePiece->m80))
		{
			DrawTilted(g, thePiece);
			return;
		}
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
		DrawTileOverlay(g, thePiece, theHint);
		return;
	}
	if (0.005 < fabs(thePiece->m80))
		DrawTilted(g, thePiece);
	else
	{
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
		DrawTileOverlay(g, thePiece, theHint);
	}

	// the ghosts of a tile that was moved: one grey picture for each cell it passed, fading with the shake that is left
	if (thePiece->mac != 0)
	{
		int aSteps = abs(thePiece->mRow - thePiece->mb0) + abs(thePiece->mColumn - thePiece->mb4);
		g->SetColorizeImages(true);
		int anAge = 0;
		for (int aStep = 0; aStep < aSteps; aStep++)
		{
			int anAlpha = thePiece->mac - anAge;
			if (10 < anAlpha)
			{
				g->SetColor(Color(255, 255, 255, anAlpha));
				int aGhostY, aGhostX;
				if (aStep == aSteps - 1)
				{
					aGhostY = CellY(thePiece->mb0);
					aGhostX = CellX(thePiece->mb4);
				}
				else
				{
					aGhostY = CellY(thePiece->mRow - aStep - 1);
					aGhostX = CellX(thePiece->mColumn);
				}
				g->DrawImage(aVariants[6], aGhostX, aGhostY);
			}
			anAge += 0x14;
		}
		g->SetColorizeImages(false);
	}
}

// 0x41e630: a tile that was hit and still swings (|m80| > 0.005) is drawn turned by that angle, in the plain grey picture
void Board::DrawTilted(Graphics* g, Piece* thePiece)
{
	MemoryImage** aVariants = mApp->mPieces[thePiece->mKind][thePiece->mRotation];
	int aY = Ftol(thePiece->mY);
	int aX = Ftol(thePiece->mX);
	g->DrawImageRotated(aVariants[6], aX, aY, thePiece->m80, NULL);
}

// 0x41e690: the warning over a tile of the top rows: the tile of the second row glows (hint 1 or 2), the one of the first row
// (hint 2) blinks between the two pictures every 50 updates
void Board::DrawTileOverlay(Graphics* g, Piece* thePiece, int theHint)
{
	Image* anImage;
	if (thePiece->mRow == 1 && (theHint == 1 || theHint == 2))
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		anImage = mApp->mImages[IMAGE_HIGHLIGHTWARNING];
	}
	else
	{
		if (thePiece->mRow != 0)
			return;
		if (theHint != 2)
			return;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		if (mUpdateCnt % 100 < 0x32)
			anImage = mApp->mImages[IMAGE_HIGHLIGHTDANGER];
		else
			anImage = mApp->mImages[IMAGE_HIGHLIGHTWARNING];
	}
	int aY = Ftol(thePiece->mY);
	int aX = Ftol(thePiece->mX);
	g->DrawImage(anImage, aX, aY);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
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

// 0x41e530: the next tile (arcade mode). It slides down from 42 pixels above its place as the fall timer runs (it stops 32
// pixels down when the column's top cell is taken), always dim. The decompilation drops the x87 code that computes the slide.
void Board::DrawSpare(Graphics* g, Piece* thePiece)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(100, 100, 100));
	int aFallen = Ftol((double) m2e0 / (double) m444 * 42.0);
	if (mGrid[0][thePiece->mColumn] != NULL && 0x20 < aFallen)
		aFallen = 0x20;
	int aWarning = m44c % 100;
	if (m44c < 500)
		g->SetColor(Color(0xff - aWarning, 99 - aWarning, 99 - aWarning));
	g->DrawImage(mApp->mPieces[thePiece->mKind][thePiece->mRotation][6], Ftol(thePiece->mX), Ftol((double) aFallen + thePiece->mY - 42.0));
	g->SetColorizeImages(false);
}

// 0x41d620: a rocket's picture (the magic rocket of the strategy mode in four hues, else the plain rockets), at x, y
void Board::DrawRocket(Graphics* g, int theIndex, int theX, int theY)
{
	static_assert(IMAGE_ROCKET01 == 53, "the rocket pictures follow image 52");
	if (mApp->mMode == 2)
		g->DrawImage(mApp->mRocketHues[theIndex], theX, theY);
	else
		g->DrawImage(mApp->mImages[IMAGE_ROCKET01 + theIndex], theX, theY);
}

// 0x41ec20: the fuses on the left and the rocket launchers on the right. A fuse is a frame of the ten-frame flame strip,
// and the frame depends on the row's height, so that the fuses do not flicker in step. A rocket fades and slides in over its first 155
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
		// the frame is centred on the 42-wide cell, then moved up and left by the original's two constants (7 and 8)
		g->DrawImage(aStrip, Ftol((double) ((0x2a - aWidth / 10) / 2) + thePiece->mX - 7.0),
			Ftol((double) ((0x2a - aHeight) / 2) + thePiece->mY - 8.0), Rect(aFrame * (aWidth / 10), 0, aWidth / 10, aHeight));
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
		// the rocket slides in from down and to the right: 0.2 and 0.4 of a pixel for each of the 155 updates it has left
		int aLeft = 0x9b - anAge;
		int aY = Ftol((double) aLeft * 0.2 + thePiece->mY - 77.0);
		int aX = Ftol((double) aLeft * 0.4 + thePiece->mX + 1.0);
		DrawRocket(g, thePiece->mIndex, aX, aY);
		g->SetColorizeImages(false);
		return;
	}
	int aY = Ftol(thePiece->mY - 77.0);
	int aX = Ftol(thePiece->mX + 1.0);
	DrawRocket(g, thePiece->mIndex, aX, aY);
	g->SetColorizeImages(false);
}

// 0x41d420: the pieces that move about, first pass. A falling tile is drawn in its plain picture: the original sets the
// colour 120, 100, 120 but never turns the colouring of images on, so the colour touches only what is drawn later without
// one of its own; a flash is a picture strip's frame (kind 2 the 42-wide dust strip, the others the match-flame strip,
// frame m7c).
void Board::DrawMover(Graphics* g, Piece* thePiece)
{
	int aType = thePiece->mType;
	if (aType == 0 || aType == 2)
	{
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

// 0x41d670's drawing of a special's picture stretched about its centre by theScale (SetFastStretch on; theOdd makes the
// width and height odd, as the original does for the clock and the bonuses)
void Board::DrawStretched(Graphics* g, Image* theImage, double theScale, Piece* thePiece, bool theOdd)
{
	int aWidth = theImage->GetWidth();
	int aHeight = theImage->GetHeight();
	int aStretchedWidth = Ftol((double) aWidth * theScale);
	int aStretchedHeight = Ftol((double) aHeight * theScale);
	if (theOdd)
	{
		aStretchedWidth |= 1;
		aStretchedHeight |= 1;
	}
	g->SetFastStretch(true);
	int aX = Ftol((double) (aWidth / 2 - aStretchedWidth / 2) + thePiece->mX);
	int aY = Ftol((double) (aHeight / 2 - aStretchedHeight / 2) + thePiece->mY - 1.0);
	g->DrawImage(theImage, Rect(aX, aY, aStretchedWidth, aStretchedHeight), Rect(0, 0, aWidth, aHeight));
	g->SetFastStretch(false);
}

// 0x41d670: how far down a special that rides on a tile waiting above the board is drawn, while that tile falls
int Board::SpecialDy(Piece* thePiece)
{
	int aDy = 0;
	if (mApp->mMode == 0 && thePiece->mHost != NULL)
	{
		int aColumn = thePiece->mColumn;
		if (thePiece->mHost == mTop[aColumn])
		{
			if (m448 == aColumn)
			{
				aDy = Ftol((double) m2e0 / (double) m444 * 42.0);
				if (mGrid[0][aColumn] != NULL && 0x20 < aDy)
					aDy = 0x20;
			}
		}
		else if (thePiece->mHost == mSpare)
		{
			aDy = Ftol((double) m2e0 / (double) m444 * 42.0 - 42.0);
			if (mGrid[0][aColumn] != NULL && -10 < aDy)
				aDy = -10;
		}
	}
	return aDy;
}

// 0x41d670: the specials (type 6 pieces), by kind: 0 and 1 a bomb (pulsing, and growing as it goes off), 2 and 3 a bonus
// that bounces in, 4 a flame, 5 a rocket, 6 and 7 a coin that falls into the chest, 8 a picture that fades out,
// 9 a launch's number (0x4290b0), 10 and 11 a torch whose frame shows its hits left, 12 and 13 the
// clock. Some drawings move a sparkle effect (the piece's mFlash) along with the piece until it lands.
void Board::DrawSpecial(Graphics* g, Piece* thePiece)
{
	Image** anImages = mApp->mImages;
	if (mApp->mMode == 1 || mApp->mMode == 2)
	{
		if (thePiece->mType != 6)
			return;
		if (thePiece->mHost != NULL)
		{
			for (int i = 0; i < 6; i++)
			{
				if (thePiece->mHost == mTop[i])
					return;
			}
		}
	}
	if (thePiece->mType != 6)
		return;
	switch (thePiece->mKind)
	{
	case 0:
	{
		double aScale = 1.0;
		if (mUpdateCnt % 800 < 0x32)
		{
			double aPulse = sin((double) mUpdateCnt * 0.002 * 62.83185307) * 0.36 + 1.0;
			if (1.0 <= aPulse)
				aScale = aPulse;
		}
		if (thePiece->mBorn < mUpdateCnt)
			DrawStretched(g, anImages[IMAGE_BOMB], aScale, thePiece, false);
		else
			g->DrawImage(anImages[IMAGE_BOMB], Ftol(thePiece->mX), Ftol((double) SpecialDy(thePiece) + thePiece->mY));
		break;
	}
	case 1:
		DrawStretched(g, anImages[IMAGE_BOMB],
			(double) (mUpdateCnt - thePiece->mBorn) * 0.023809523809523808 * 0.3 + 1.0, thePiece, false);
		break;
	case 2:
	case 3:
	{
		static const int sImages[5] = { IMAGE_GEM01, IMAGE_GEM02, IMAGE_GEM03, IMAGE_GEM04, IMAGE_GEM05 };
		Image* anImage = anImages[sImages[thePiece->mIndex >= 0 && thePiece->mIndex < 5 ? thePiece->mIndex : 0]];
		int anAge = thePiece->mBorn - mUpdateCnt;
		bool aStretch = false;
		double aScale = 1.0;
		if (anAge < 100)
		{
			double aGrow = 1.0 - cos((double) anAge * 0.01 * 1.57079632675);
			aScale = 0.4 <= aGrow ? aGrow : 0.4;
			aStretch = true;
		}
		else if (anAge < 600)
		{
			double aWobble = (1.05 - (double) ((anAge - 100) / 100) * 0.2) * sin((double) (anAge - 100) * 0.002 * 31.415926535) + 1.0;
			aScale = 1.0 <= aWobble ? aWobble : 1.0;
			aStretch = true;
		}
		if (aStretch && thePiece->mKind == 2)
			DrawStretched(g, anImage, aScale, thePiece, true);
		else
			g->DrawImage(anImage, Ftol(thePiece->mX), Ftol((double) SpecialDy(thePiece) + thePiece->mY));
		break;
	}
	case 4:
	{
		Image* aStrip = anImages[IMAGE_MATCHFLAME];
		int aWidth = aStrip->GetWidth() / 10;
		int aHeight = aStrip->GetHeight();
		int aFrame = CrtRand() % 10;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(aStrip, Ftol((double) ((0x2a - aWidth) / 2) + thePiece->mX), Ftol((double) ((0x2a - aHeight) / 2) + thePiece->mY),
			Rect(aFrame * aWidth, 0, aWidth, aHeight));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;
	}
	case 5:
		DrawRocket(g, thePiece->mIndex, Ftol(thePiece->mX), Ftol(thePiece->mY));
		break;
	case 6:
	case 7:
	{
		int anAge = mUpdateCnt - thePiece->mBorn;
		int aX = Ftol(thePiece->mX);
		int aY;
		if (anAge < 100)
		{
			aY = Ftol((double) anAge * (double) anAge * 0.0001 * thePiece->mY);
			if (thePiece->mFlash != NULL)
			{
				thePiece->mFlash->mX = (double) (aX + 7);
				thePiece->mFlash->mY = (double) aY;
			}
		}
		else
		{
			aY = Ftol(thePiece->mY);
			if (thePiece->mFlash != NULL)
			{
				mEffects->Remove(thePiece->mFlash);
				thePiece->mFlash = NULL;
			}
		}
		int aCoin = thePiece->mIndex == 0 ? IMAGE_COINBRONZE : thePiece->mIndex == 1 ? IMAGE_COINSILVER :
			thePiece->mIndex == 2 ? IMAGE_COINGOLD : IMAGE_COINPLATINUM;
		g->DrawImage(anImages[aCoin], aX, aY);
		int aPhase = (aX / 10 + aY / 10 + mUpdateCnt) % 800;
		if (aPhase <= 100 && 100 <= anAge)
		{
			int aShade = Ftol(sin((double) aPhase * 0.01 * 3.1415926535) * 60.0);
			g->SetColorizeImages(true);
			g->SetColor(Color(aShade, aShade, aShade));
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImage(anImages[IMAGE_COINGLOW], aX - 2, aY - 3);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(false);
		}
		break;
	}
	case 8:
	{
		int anAlpha = thePiece->mBorn - mUpdateCnt + 0xff;
		if (anAlpha < 0)
			anAlpha = 0;
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawImage(anImages[thePiece->mIndex], Ftol(thePiece->mX), Ftol(thePiece->mY));
		g->SetColorizeImages(false);
		break;
	}
	case 9:
	{
		int anAlpha = thePiece->mBorn - mUpdateCnt + 0xff;
		if (anAlpha < 0)
			anAlpha = 0;
		g->SetColorizeImages(true);
		DrawLaunchNumber(g, thePiece->mIndex, thePiece->mRow, anAlpha);
		g->SetColorizeImages(false);
		break;
	}
	case 10:
	case 11:
	{
		int aX;
		int aY;
		if (mUpdateCnt - thePiece->mBorn < 100)
		{
			int aRest = 100 - (mUpdateCnt - thePiece->mBorn);
			aX = thePiece->m40;
			double anEase = 1.0 - (double) aRest * (double) aRest * 0.0001;
			aY = Ftol((double) thePiece->m44 + anEase * (thePiece->mY - (double) thePiece->m44));
			if (thePiece->mFlash != NULL)
			{
				thePiece->mFlash->mX = (double) (aX + 7);
				thePiece->mFlash->mY = (double) aY;
			}
		}
		else
		{
			aX = Ftol(thePiece->mX);
			aY = Ftol(thePiece->mY);
			if (thePiece->mFlash != NULL)
			{
				mEffects->Remove(thePiece->mFlash);
				thePiece->mFlash = NULL;
			}
		}
		Image* aSheet = anImages[IMAGE_TORCH];
		int aWidth = aSheet->GetWidth() / 3;
		int aHeight = aSheet->GetHeight() / 3;
		int aColumn = ((mUpdateCnt % 0x2d) / 0xf + aY % 5 + aY % 3 + aX % 7) % 3;
		g->DrawImage(aSheet, aX, aY, Rect(aColumn * aWidth, (3 - thePiece->ma8) * aHeight, aWidth, aHeight));
		break;
	}
	case 12:
	case 13:
		if (mUpdateCnt % 800 < 100)
		{
			double aScale = sin((double) mUpdateCnt * 0.004 * 47.1238898025) * 0.15 + 1.0;
			if (thePiece->mKind == 12)
			{
				DrawStretched(g, anImages[IMAGE_CLOCK], aScale, thePiece, true);
				break;
			}
		}
		g->DrawImage(anImages[IMAGE_CLOCK], Ftol(thePiece->mX), Ftol((double) SpecialDy(thePiece) + thePiece->mY));
		break;
	}
}

// 0x41f2e0: one lantern of the night's bar: lantern theIndex (0 to 9, from the left) in the picture theImage, each a little
// higher or lower than the one before (the string of lanterns hangs in a curve)
void Board::DrawLantern(Graphics* g, int theImage, int theIndex)
{
	static const int sBob[10] = { 0, 2, 4, 5, 6, 6, 5, 4, 2, 0 };
	g->DrawImage(mApp->mImages[theImage], theIndex * 0x16 + 0x108, sBob[theIndex] + 0x28);
}

// 0x41f360: the night of the classic mode as ten lanterns on a wire (m318 of the night's m68 is left). While time is frozen
// (m4bc) all of them show the frozen picture. Otherwise the lanterns burn out from the left as the night goes (the lit
// picture, then the dim one that fades out over the tenth of the night that the lantern lasts, then the unlit one); the
// first one that goes out sets m468[i] (the board's update then makes the puff, 0x41f280). In the last tenth of the night the
// lanterns still lit flicker.
void Board::DrawNightBar(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_WIRE], 0x102, 0x25);
	if (0 < m4bc)
	{
		for (int i = 9; -1 < i; i--)
			DrawLantern(g, IMAGE_LANTERNFROZEN, i);
		return;
	}
	double aFraction = 1.0 - m318 / m68;
	if (0.8999999761581421 < aFraction)
	{
		int aLeft = -0x5a - Ftol(aFraction * -100.0);
		int aFlicker = Ftol(aFraction * 1000.0) % 10;
		for (int i = 9; -1 < i; i--)
		{
			DrawLantern(g, IMAGE_LANTERNOFF, i);
			if (aLeft <= i && 3 < aFlicker)
				DrawLantern(g, IMAGE_LANTERNON, i);
		}
		return;
	}
	for (int i = 9; -1 < i; i--)
	{
		if (aFraction <= (double) i * 0.1)
			DrawLantern(g, IMAGE_LANTERNON, i);
		else
		{
			double aScaled = aFraction * 10.0;
			int aTenth = Ftol(aScaled);
			if (Ftol((double) i * 0.1 * 10.0) == aTenth)
			{
				DrawLantern(g, IMAGE_LANTERNDIM, i);
				g->SetColorizeImages(true);
				g->SetColor(Color(255, 255, 255, Ftol((1.0 - ((aScaled - (double) aTenth) * 0.1) * 10.0) * 255.0)));
				DrawLantern(g, IMAGE_LANTERNON, i);
				g->SetColorizeImages(false);
			}
			else
			{
				DrawLantern(g, IMAGE_LANTERNOFF, i);
				if (m464 < i)
				{
					m464 = i;
					m468[i] = 1;
				}
			}
		}
	}
}

// 0x4290b0: the points of the launch that is being made, beside the row of its fuse, in a colour that turns from green to
// white as the chain grows (five points for each flame, scaled by the difficulty); theAlpha fades it
void Board::DrawLaunchNumber(Graphics* g, int theCount, int theRow, int theAlpha)
{
	if (0 < theAlpha)
	{
		int aPoints = theCount * 5;
		int aHue = aPoints;
		if (0xff < aHue)
			aHue = 0xff;
		unsigned long aColor = mApp->HSLToRGB(aHue, 0xff, 0x80);
		g->SetColor(Color((int) ((aColor & 0xffffff) | ((unsigned long) theAlpha << 24))));
		g->SetFont(mApp->mFonts[FONT_DOMO14_BOLD]);
		double aFactor;
		if (mApp->mDifficulty == 1)         // 0x425990
			aFactor = 1.0;
		else if (mApp->mDifficulty == 2)
			aFactor = 1.5;
		else
			aFactor = 2.0;
		char aBuf[64];
		sprintf(aBuf, "%d", Ftol(aFactor * (double) aPoints));
		g->DrawString(SexyString(aBuf), CellX(-1) - 0x1e, CellY(theRow) + 0x1c);
	}
}

// 0x429230: the launch in progress (m49c flames counted, in the row m4a0) shows its points
void Board::DrawLaunchInProgress(Graphics* g)
{
	if (m4a0 != -1 && m49c != 0)
		DrawLaunchNumber(g, m49c, m4a0, 0xff);
}

