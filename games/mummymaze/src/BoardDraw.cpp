// The board's drawing: WinMM.exe 0x408780-0x408e50, 0x4095f0-0x40e0e0, 0x410660, 0x410860. See Board.h.
// Pixel positions: column x starts at PixelX(x) = cell * x + 0x36, row y at PixelY(y) = cell * y + 0x50; the whole board is
// drawn translated by (10, 0). The sprite strips hold one picture per cell in a row (animation frames) and one row of
// cells per direction.

#include "Board.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Screens.h"
#include "MummyApp.h"
#include "Compat2004.h"
#include "Dialogs.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

#include "GameLevels.h"
#include "Tables.inc"
#include "TutorialMazes.inc"

#define IMG(theId) (mApp->mImages[theId])

// a picture clipped to a rectangle (the original makes a Graphics from the parent, clips it and draws)
void Board::DrawClipped(Graphics* g, Image* theImage, int theX, int theY, int theClipX, int theClipY, int theClipW, int theClipH)
{
	Graphics gc(*g);
	gc.ClipRect(theClipX, theClipY, theClipW, theClipH);
	gc.DrawImage(theImage, theX, theY);
}

// A dialog-like frame of a nine-patch picture (the dialog picture, IMAGE_DIALOG) of a given size, as the message boxes of
// the board draw it (0x408780): the corners, the tiled edges and the tiled middle
void DrawNinePatch(Graphics* g, Image* theImage, int theW, int theH)
{
	int aW = theImage->GetWidth() / 3;
	int aH = theImage->GetHeight() / 3;
	g->DrawImage(theImage, 0, 0, Rect(0, 0, aW, aH));
	int aRight = theW - aW;
	g->DrawImage(theImage, aRight, 0, Rect(aW * 2, 0, aW, aH));
	int aBottom = theH - aH;
	g->DrawImage(theImage, 0, aBottom, Rect(0, aH * 2, aW, aH));
	g->DrawImage(theImage, aRight, aBottom, Rect(aW * 2, aH * 2, aW, aH));

	Graphics aTop(*g);
	int aInnerW = theW - aW * 2;
	aTop.ClipRect(aW, 0, aInnerW, theH);
	int aCols = (aRight - 1) / aW;
	int aX = aW;
	for (int i = 0; i < aCols; i++)
	{
		aTop.DrawImage(theImage, aX, 0, Rect(aW, 0, aW, aH));
		aTop.DrawImage(theImage, aX, aBottom, Rect(aW, aH * 2, aW, aH));
		aX += aW;
	}
	Graphics aSide(*g);
	int aInnerH = theH - aH * 2;
	aSide.ClipRect(0, aH, theW, aInnerH);
	int aRows = (aBottom - 1) / aH;
	int aY = aH;
	for (int i = 0; i < aRows; i++)
	{
		aSide.DrawImage(theImage, 0, aY, Rect(0, aH, aW, aH));
		aSide.DrawImage(theImage, aRight, aY, Rect(aW * 2, aH, aW, aH));
		aY += aH;
	}
	Graphics aMiddle(*g);
	aMiddle.ClipRect(aW, aH, aInnerW, aInnerH);
	aX = aW;
	for (int i = 0; i < aCols; i++)
	{
		aY = aH;
		for (int j = 0; j < aRows; j++)
		{
			aMiddle.DrawImage(theImage, aX, aY, Rect(aW, aH, aW, aH));
			aY += aH;
		}
		aX += aW;
	}
}

// 0x408780: the "Solving Puzzle" box
void Board::DrawSolving(int x, int y, int w, int h, Graphics* g)
{
	g->Translate(x, y);
	DrawNinePatch(g, IMG(IMAGE_DIALOG), w, h);
	Insets aInsets(0x18, 0x1e, 0x18, 0x18);
	int aTop = aInsets.mTop;
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0x66, 0x5c, 0x38));
	Rect aRect(aInsets.mLeft + 2, aTop, w - aInsets.mRight - aInsets.mLeft - 4, 0);
	WriteWordWrapped(g, aRect, GT(0x00491e1c), -1, 0);
	aTop = aTop + 10 + mApp->mHeaderFont->GetHeight();
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0x2e, 0x14, 0));
	Rect aRect2(aInsets.mLeft + 2, aTop, w - aInsets.mRight - aInsets.mLeft - 4, 0);
	WriteWordWrapped(g, aRect2, GT(0x00491e08), -1, 0);
	g->Translate(-x, -y);
}

// 0x408e50: the tutorial's box under the maze: the text of the step. The original builds the part's title ("TUTORIAL
// Part 1: The Basics", chosen by the game mode: 0x491e28, 0x491e44, 0x491e60) into a local string and never draws it
// (the function has one text write, in font 1, and does not touch the header font): the box has no title. Both width
// tests (under 0x35c, under 200) measure the step's text.
void Board::DrawTutorialText(int x, int y, int w, int h, Graphics* g)
{
	if (mState != STATE_READY)
		return;
	if (mUpdateCnt - mTimeBase < 0xa5)
		return;
	g->Translate(x, y);
	DrawNinePatch(g, IMG(IMAGE_DIALOG), w, h);
	Insets aInsets(0x1a, 0x18, 0x18, 0x18);
	int aTop = aInsets.mTop;
	const SexyString& aText = GT(kTutorialText[mTutorialStep]);
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0x2e, 0x14, 0));
	int aMove = kTutorialMove[mTutorialStep];
	if (mApp->mFont1->StringWidth(aText) < 0x35c && aMove != 5 && aMove != 6)
		aTop += 10;
	if (mApp->mFont1->StringWidth(aText) < 200)
		aTop += 10;
	Rect aRect(aInsets.mLeft + 2, aTop, w - aInsets.mRight - aInsets.mLeft - 4, 0);
	WriteWordWrapped(g, aRect, aText, -1, (aMove == 5 || aMove == 6) ? -1 : 0);
	g->Translate(-x, -y);
}

// 0x410660: the move that a click or the mouse over a cell would make: an arrow, or a circle for waiting
void Board::DrawMoveHint(int x1, int y1, int x2, int y2, Graphics* g)
{
	int aMode = mApp->mGameMode;
	if (aMode == 2 || aMode == 3)
		return;
	if (x2 < 0 || x2 > 10 || y2 < 0 || y2 > 10)
		return;
	if (!CanMove(x1, y1, x2, y2, true, mGateClosed))
		return;
	if (aMode > 3)
		return;
	if (mFade2 < 1.0)
		return;
	Graphics gc(*g);
	gc.ClipRect(PixelX(x2), PixelY(y2), mCellSize, mCellSize);
	int aSize = SizeIndex();
	int aX, aY;
	Image* anImage = IMG(IMAGE_ARROWS6 + aSize);
	if (x2 - x1 == -1)
	{
		aY = PixelY(y2) + mCellSize * -2;
		aX = PixelX(x2);
	}
	else if (x2 == x1)
	{
		if (y2 - y1 == -1)
			aY = PixelY(y2) + mCellSize * -3;
		else if (y2 - y1 == 1)
			aY = PixelY(y2);
		else
		{
			aY = PixelY(y2);
			aX = PixelX(x2);
			gc.DrawImage(IMG(IMAGE_CIRCLE6 + aSize), aX, aY);
			return;
		}
		aX = PixelX(x2);
	}
	else
	{
		if (x2 - x1 != 1)
			return;
		aY = PixelY(y2) - mCellSize;
		aX = PixelX(x2);
	}
	gc.DrawImage(anImage, aX, aY);
}

// 0x410860: the tutorial's arrow on the neighbour the step wants the explorer to move to
void Board::DrawTutorialArrow(int x, int y, Graphics* g)
{
	if (mState != STATE_READY)
		return;
	int aDX = 0;
	int aDY = 0;
	switch (kTutorialMove[mTutorialStep])
	{
	case 0:
		aDY = -1;
		break;
	case 1:
		aDX = -1;
		break;
	case 2:
		aDX = 1;
		break;
	case 3:
		aDY = 1;
		break;
	case 4:
		break;
	default:
		return;
	}
	Graphics gc(*g);
	int aCX = x + aDX;
	int aCY = y + aDY;
	gc.ClipRect(PixelX(aCX), PixelY(aCY), mCellSize, mCellSize);
	int aSize = SizeIndex();
	Image* anArrows = IMG(IMAGE_ARROWS6 + aSize);
	if (aDX == -1)
		gc.DrawImage(anArrows, PixelX(aCX), PixelY(aCY) + (2 - mCellSize) * 2);
	else if (aDX == 0)
	{
		if (aDY == -1)
			gc.DrawImage(anArrows, PixelX(aCX), PixelY(y - 1) + mCellSize * -3);
		else if (aDY == 1)
			gc.DrawImage(anArrows, PixelX(aCX), PixelY(y + 1));
		else
			gc.DrawImage(IMG(IMAGE_CIRCLE6 + aSize), PixelX(aCX), PixelY(aCY));
	}
	else if (aDX == 1)
		gc.DrawImage(anArrows, PixelX(aCX), PixelY(aCY) - mCellSize);
}

// 0x40d130: a mummy (theFull: with the floor tile that is revealed under it while it drops in)
void Board::DrawMummy(Graphics* g, int theIndex, bool theFull)
{
	Mummy* m = mMummies[theIndex];
	if (m == NULL || !m->mAlive)
		return;
	int aSize = SizeIndex();
	int aCell = mCellSize;
	if (mFade2 < 1.0 && theFull)
		g->DrawImage(IMG(IMAGE_BACKWALL6 + aSize), PixelX(m->mX), PixelY(m->mY));
	int aPX = (int) ((double) PixelX(m->mX) + (double) (m->mTargetX - m->mX) * m->mProgress);
	int aPY = (int) ((double) PixelY(m->mY) + (double) (m->mTargetY - m->mY) * m->mProgress + (1.0 - mFade2) * (double) aCell);
	Graphics gc(*g);
	int aDrop = (int) ((1.0 - mFade2) * (double) aCell);
	int aClipX = aPX;
	int aClipY = aPY;
	int aClipH = aCell;
	if (mFade2 != 1.0)
	{
		aClipH = aCell - aDrop;
		aClipY = PixelY(m->mY) + aDrop;
		aClipX = PixelX(m->mX);
	}
	gc.ClipRect(aClipX, aClipY, aCell, aClipH);
	if (mFade2 < 1.0 && theFull)
	{
		Rect aSrc(aCell * m->mX, aCell * m->mY, aCell, aCell - aDrop);
		g->DrawImage(IMG(IMAGE_FLOOR6 + aSize), PixelX(m->mX), PixelY(m->mY) + aDrop, aSrc);
		bool aTrap = (m->mX == mTrapX[0] && m->mY == mTrapY[0]) || (m->mX == mTrapX[1] && m->mY == mTrapY[1]);
		if (aTrap)
		{
			Rect aSrc2(0, 0, aCell, aCell - aDrop);
			g->DrawImage(IMG(IMAGE_TRAP6 + aSize), PixelX(m->mX), PixelY(m->mY) + aDrop, aSrc2);
		}
		if (mState != STATE_ENDING_FADE)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0, 0, 0, (int) ((1.0 - mFade2) * 255.0)));
		}
		Rect aSrc3(aCell * m->mX, aCell * m->mY, aCell, aCell - aDrop);
		g->DrawImage(IMG(IMAGE_FLOOR6 + aSize), PixelX(m->mX), PixelY(m->mY) + aDrop, aSrc3);
		if (aTrap)
		{
			Rect aSrc4(0, 0, aCell, aCell - aDrop);
			g->DrawImage(IMG(IMAGE_TRAP6 + aSize), PixelX(m->mX), PixelY(m->mY) + aDrop, aSrc4);
		}
		if (mState != STATE_ENDING_FADE)
			g->SetColorizeImages(false);
	}
	int aKind = aSize;
	if (mRed)
		aKind += 3;
	if (!m->mFighting || m->mX != mExplorer->mX || m->mY != mExplorer->mY)
	{
		Image* anImage;
		int aXOff, aY;
		if (!m->mIdle || mState != STATE_READY)
		{
			int aDir = m->mDir;
			int aWant = m->mWantDir;
			if (aDir == aWant)
			{
				aY = aPY - aDir * aCell;
				int aFrame = (int) (m->mProgress * (1.0 / 6.0));
				aXOff = (aFrame % 5) * aCell;
			}
			else
			{
				int aTurn = m->mTurn;
				if (aTurn < 0)
				{
					aDir--;
					aTurn += 4;
				}
				else if (aTurn > 3)
				{
					aDir++;
					aTurn -= 4;
				}
				if (aDir < 0)
					aDir = 3;
				else if (aDir > 3)
					aDir = 0;
				aXOff = aCell * aTurn;
				aY = aPY - aCell * aDir;
			}
			anImage = IMG(IMAGE_MUMMY6 + aKind);
		}
		else
		{
			aXOff = kMummyFightX[m->mIdleFrame / 5 + m->mIdleKind * 0xff] * aCell;
			aY = aPY;
			anImage = IMG(IMAGE_WHITEDANCE6 + aKind + m->mIdleKind * 6);
		}
		gc.DrawImage(anImage, aPX - aXOff, aY);
	}
	if (mFade2 < 1.0)
	{
		if (mState != STATE_ENDING_FADE)
		{
			gc.SetColorizeImages(true);
			gc.SetColor(Color(0, 0, 0, (int) ((1.0 - mFade2) * 255.0)));
		}
		int aY = aPY - m->mDir * aCell;
		int aFrame = (int) (m->mProgress * (1.0 / 6.0));
		gc.DrawImage(IMG(IMAGE_MUMMY6 + aKind), aPX - (aFrame % 5) * aCell, aY);
		if (mState != STATE_ENDING_FADE)
			gc.SetColorizeImages(false);
	}
	if (m->mFighting)
	{
		int aX = PixelX(m->mX);
		int aY = PixelY(m->mY);
		Graphics gf(*g);
		int aH = aCell;
		if ((mCells[m->mX][m->mY] & WALL_SOUTH) != 0 && m->mY != mSize - 1)
			aH = aCell - 10;
		gf.ClipRect(aX, aY, aCell, aH);
		if (m->mFighting && m->mX == mExplorer->mX && m->mY == mExplorer->mY)
			gf.DrawImage(IMG(IMAGE_WHITEFIGHT6 + aKind), aX, aY);
		int aFrame = m->mFightFrame;
		gf.DrawImage(IMG(IMAGE_DUST6 + aSize), aX - ((aFrame + ((aFrame >> 31) & 3)) >> 2) * aCell, aY);
		if (m->mFightFrame < 0x76)
		{
			for (int i = 0; i < 4; i++)
				gf.DrawImage(IMG(IMAGE_STAR), mDust[0][i] + aX, mDust[1][i] + aY);
		}
	}
}

// 0x40d910: the explorer
void Board::DrawExplorer(Graphics* g)
{
	if (mState == STATE_DONE)
		return;
	if (mDeathTimer > 0x117)
		return;
	Explorer* e = mExplorer;
	if (e == NULL)
		return;
	if (mState != STATE_ENTER)
	{
		for (int i = 0; i < 2; i++)
		{
			Mummy* m = mMummies[i];
			if (m != NULL && m->mAlive && m->mX == e->mX && m->mY == e->mY)
				return;
		}
	}
	int aSize = SizeIndex();
	int aCell = mCellSize;
	int aPX = (int) ((double) PixelX(e->mX) + (double) (e->mTargetX - e->mX) * e->mProgress);
	int aPY = (int) ((double) PixelY(e->mY) + (double) (e->mTargetY - e->mY) * e->mProgress);
	int aMode = mApp->mGameMode;
	if (aMode == 2 || aMode == 3)
		aPX -= 0x1e;
	if (mState == STATE_ENTER)
	{
		if (e->mTargetX < e->mX)
			aPX = (int) ((double) PixelX(e->mX) - e->mProgress);
		else if (e->mX < e->mTargetX)
			aPX = (int) ((double) PixelX(e->mX) + e->mProgress);
		if (e->mTargetY < e->mY)
			aPY = (int) ((double) PixelY(e->mY) - e->mProgress);
		else if (e->mY < e->mTargetY)
			aPY = (int) ((double) PixelY(e->mY) + e->mProgress);
	}
	Graphics gc(*g);
	int aClipH = aCell;
	if (!((mCells[e->mX][e->mY] & WALL_SOUTH) == 0 || e->mTargetY == mSize - 1 || mState != STATE_LEAVE || mExitStyle != 1))
		aClipH = aCell - 10;
	gc.ClipRect(aPX, aPY, aCell, aClipH);
	Image* anImage = NULL;
	int aDrawX = aPX;
	int aDrawY = aPY;
	bool aDraw = true;
	if (mState == STATE_LEAVE)
	{
		if (mExitStyle == 0)
		{
			int aFrame = (mDeathTimer - 0x28);
			aFrame = (aFrame + ((aFrame >> 31) & 3)) >> 2;
			if (aFrame < 0)
				aFrame = 0;
			else if (aFrame > 0xf)
				aFrame = 0xf;
			aDrawX = aPX - aFrame * aCell;
			anImage = IMG(IMAGE_FREAKOUT6 + aSize);
		}
		else if (mExitStyle == 1)
		{
			int aFall = mDeathTimer - 0x1e;
			int aStep = mDeathTimer / 10;
			if (aStep > 3)
				aStep = 3;
			gc.DrawImage(IMG(IMAGE_EXPFALL6 + aSize), aPX - aStep * aCell, ((aFall < 0) ? 0 : aFall) + aPY);
			// the edge of the pit on top of him
			Rect aSrc(0, aCell / 2, aCell - 5, aCell / 2);
			if (((mExplorer->mX + mExplorer->mY) & 1) == 0)
				gc.DrawImage(IMG(IMAGE_FLOORDARK6 + aSize), PixelX(mExplorer->mX), PixelY(mExplorer->mY) + 1 + aCell / 2, aSrc);
			else
				gc.DrawImage(IMG(IMAGE_FLOORLIGHT6 + aSize), PixelX(mExplorer->mX), PixelY(mExplorer->mY) + 1 + aCell / 2, aSrc);
			aDraw = false;
		}
		else
			goto L_normal;
	}
	else
	{
	L_normal:
		if (mDeathTimer2 < 0)
		{
			if (!e->mIdle || mState != STATE_READY)
			{
				aDrawY = aPY - e->mDir * aCell;
				int aFrame = (int) (e->mProgress * (1.0 / 6.0));
				aDrawX = aPX - (aFrame % 5) * aCell;
				anImage = IMG(IMAGE_EXPLORER6 + aSize);
			}
			else
			{
				aDrawX = aPX - kExplorerIdleX[e->mIdleFrame / 5 + e->mIdleKind * 0xff] * aCell;
				anImage = IMG(IMAGE_READMAP6 + aSize + e->mIdleKind * 3);
			}
		}
		else
		{
			int aFrame = mDeathTimer2 / 5;
			if (aFrame > 0x11)
				aFrame = 0x11;
			aDrawX = aPX - aFrame * aCell;
			anImage = IMG(IMAGE_STUNG6 + aSize);
		}
	}
	if (aDraw && anImage != NULL)
		gc.DrawImage(anImage, aDrawX, aDrawY);
}

// 0x40de00: the scorpion, or the sting scene
void Board::DrawScorpion(Graphics* g)
{
	int aSize = SizeIndex();
	int aCell = mCellSize;
	Scorpion* s = mScorpion;
	if (s->mStinging)
	{
		int aX = PixelX(s->mX);
		int aY = PixelY(s->mY);
		Graphics gc(*g);
		int aH = aCell;
		if (!((mCells[s->mX][s->mY] & WALL_SOUTH) == 0 || s->mY == mSize - 1))
			aH = aCell - 10;
		gc.ClipRect(aX, aY, aCell, aH);
		int aFrame = s->mStingFrame;
		gc.DrawImage(IMG(IMAGE_DUST6 + aSize), aX - ((aFrame + ((aFrame >> 31) & 3)) >> 2) * aCell, aY);
		if (s->mStingFrame < 0x76)
		{
			for (int i = 0; i < 4; i++)
				gc.DrawImage(IMG(IMAGE_STAR), mDust[0][i] + aX, mDust[1][i] + aY);
		}
		return;
	}
	if (!s->mAlive)
		return;
	int aX = PixelX(s->mX) + (s->mToX - s->mX) * s->mProgress;
	int aY = PixelY(s->mY) - 1 + (s->mToY - s->mY) * s->mProgress;
	Graphics gc(*g);
	gc.ClipRect(aX, aY, aCell, aCell);
	if (!mRed)
	{
		if (mDark)
		{
			gc.SetColorizeImages(true);
			gc.SetColor(Color(0xff, 0xff, 0xff, 0x96));
		}
	}
	else
	{
		gc.SetColorizeImages(true);
		if (!mDark)
			gc.SetColor(Color(0xe6, 10, 0));
		else
			gc.SetColor(Color(0xff, 0x32, 0x28, 0x96));
	}
	gc.DrawImage(IMG(IMAGE_SCORPION6 + aSize), aX - ((s->mProgress / 6) % 5) * aCell, aY - s->mDir * aCell);
	if (mRed || mDark)
		gc.SetColorizeImages(false);
}

// 0x40cb70: the dark pyramids: all but a window around the explorer is black, the rest of the figures are dimly seen
void Board::DrawDark(Graphics* g)
{
	int aSize = SizeIndex();
	int aWin = (aSize == 0) ? 0xf0 : ((aSize == 1) ? 200 : 0xa0);
	Explorer* e = mExplorer;
	int aPX = (int) ((double) PixelX(e->mX) + (double) (e->mTargetX - e->mX) * e->mProgress);
	int aPY = (int) ((double) PixelY(e->mY) + (double) (e->mTargetY - e->mY) * e->mProgress);
	{
		Graphics gc(*g);
		gc.ClipRect(0, 5, mWidth - 5, mHeight - 10);
		gc.SetColor(Color(0, 0, 0));
		gc.FillRect(0, 0, aPX, mHeight);
		gc.FillRect(aPX + aWin, 0, (mWidth - aPX) - aWin, mHeight);
		gc.FillRect(0, 0, mWidth, aPY);
		gc.FillRect(0, aPY + aWin, mWidth, (mHeight - aPY) - aWin);
	}
	{
		Graphics gc(*g);
		int aH = (mHeight - aPY) - 5;
		if (aWin < aH)
			aH = aWin;
		int aW = (mWidth - aPX) - 5;
		if (aWin < aW)
			aW = aWin;
		int aTop = 5;
		if (4 < aPY)
			aTop = aPY;
		gc.ClipRect(aPX < 0 ? 0 : aPX, aTop, aW, aH);
		gc.DrawImage(IMG(IMAGE_DARK6 + aSize), aPX, aPY - ((mUpdateCnt / 10) % 3) * aWin);
	}
	{
		Graphics gc(*g);
		gc.Translate(10, 0);
		gc.SetColor(Color(0xff, 0xff, 0xff, 0x4b));
		gc.SetColorizeImages(true);
		DrawMummy(&gc, 0, false);
		DrawMummy(&gc, 1, false);
		DrawScorpion(&gc);
		gc.SetColorizeImages(false);
		gc.Translate(-10, 0);
	}
	for (int y = 0; y < mSize; y++)
	{
		for (int x = 0; x < mSize; x++)
		{
			int aFlags = mCells[x][y];
			Graphics gc(*g);
			gc.Translate(10, 0);
			gc.SetColorizeImages(true);
			gc.SetColor(Color(0xff, 0xff, 0xff, 0x96));
			if (aFlags & EXIT_NORTH)
			{
				DrawClipped(&gc, IMG(IMAGE_STAIRS6 + aSize), PixelX(x), PixelY(0) - 0x42, PixelX(x), PixelY(0) - 0x42, mCellSize, 0x42);
			}
			if (aFlags & EXIT_SOUTH)
			{
				DrawClipped(&gc, IMG(IMAGE_STAIRS6 + aSize), PixelX(x) + (-0x34 - mCellSize), PixelY(y + 1), PixelX(x), PixelY(y + 1),
					mCellSize, 0x42);
			}
			if (aFlags & EXIT_WEST)
			{
				DrawClipped(&gc, IMG(IMAGE_STAIRS6 + aSize), (-0x1a - mCellSize) * 2, PixelY(y), 0, PixelY(y), 0x34, 0x42);
			}
			if (aFlags & EXIT_EAST)
			{
				DrawClipped(&gc, IMG(IMAGE_STAIRS6 + aSize), PixelX(x + 1) - mCellSize, PixelY(y), PixelX(x + 1), PixelY(y), 0x34, 0x42);
			}
			gc.SetColorizeImages(false);
			gc.Translate(-10, 0);
		}
	}
}

// 0x40c950: the ending scene's treasure
void Board::DrawEnding(Graphics* g)
{
	{
		Graphics gc(*g);
		gc.ClipRect(0, 0, 0x1e7, 0x1dd);
		gc.DrawImage(IMG(IMAGE_NEXTLEVEL), -5, 5);
	}
	g->DrawImage(IMG(IMAGE_FINDTREASURE), (mWidth - 400) / 2, (mHeight - 0x18d) / 2);
	g->SetColor(Color(0xff, 0xff, 0xff, mEndAlpha));
	g->SetColorizeImages(true);
	g->DrawImage(IMG(IMAGE_FINDTREASUREGLYPHS), (mWidth - 400) / 2, (mHeight - 0x18d) / 2);
	g->SetColorizeImages(false);
	Graphics gc(*g);
	gc.ClipRect((mWidth - 400) / 2 + 0x9c, (mHeight - 0x18d) / 2 + 0x34, 0x85, 0x7f);
	int aTreasure;
	if (mApp->mGameMode == MODE_ADVENTURE_DONE)
	{
		aTreasure = mApp->mAdvPyramid;
		if (aTreasure > 0xe)
			aTreasure = 0xe;
		else
		{
			if (aTreasure == 9)
				aTreasure = 10;
			else if (aTreasure == 10)
				aTreasure = 0xb;
			else if (aTreasure == 0xb)
				aTreasure = 9;
		}
	}
	else
	{
		aTreasure = mApp->mClassicPyramid;
		if (aTreasure == 9)
			aTreasure = 10;
		else if (aTreasure == 10)
			aTreasure = 0xb;
		else if (aTreasure == 0xb)
			aTreasure = 9;
	}
	gc.DrawImage(IMG(IMAGE_NICETREASURESTRIP), (mWidth - 400) / 2 + 0x9c, (mHeight - 0x18d) / 2 + aTreasure * -0x7f + 0x34);
}

// the walls' pieces: a wall picture is clipped to the piece and drawn at the piece's place
#define WALL(g, x, y, w, h, imgX, imgY) DrawClipped(g, IMG(IMAGE_WALLS6 + aSize), imgX, imgY, x, y, w, h)

// 0x4095f0: the maze: the floor, the stairs, the walls row by row with the figures standing in their rows
void Board::DrawBoard(Graphics* g, bool theWithExplorer)
{
	g->DrawImage(IMG(IMAGE_BACKDROP), -0x96, 0);
	int aT = mDeathTimer;
	if (!((aT < 0x78) || (0x9a < aT) || (mExitStyle != 0)))
	{
		int aShake = aT % 10 - 5;
		g->Translate(aShake, aShake);
		Graphics gc(*g);
		gc.ClipRect(0, 8, 0x1e3, 0x1d0);
		gc.DrawImage(IMG(IMAGE_BACKDROP), -0x96, 0);
	}
	else if (!((aT < 0xbe) || (0xdb < aT) || (mExitStyle != 1)))
	{
		int aShake = aT % 6 - 3;
		g->Translate(aShake, aShake);
		Graphics gc(*g);
		gc.ClipRect(0, 8, 0x1e3, 0x1d0);
		gc.DrawImage(IMG(IMAGE_BACKDROP), -0x96, 0);
	}
	g->Translate(10, 0);
	int aSize = SizeIndex();
	int aCell = mCellSize;

	// the torches over the floor's top edge, unless the exit is under them
	bool aLeftTorch = !(((mCells[1][0] & EXIT_NORTH) && (mSize == 6 || mSize == 8 || mSize == 10)) ||
		((mCells[2][0] & EXIT_NORTH) && mSize == 8));
	if (aLeftTorch)
	{
		g->DrawImage(IMG(IMAGE_TORCH), 0x8b, 0x35);
		Graphics g1(*g);
		g1.SetDrawMode(1);
		g1.ClipRect(0x7e, 0x1d, 0x24, 0x24);
		g1.DrawImage(IMG(IMAGE_GLOW), mTorchFrame * -0x24 + 0x7e, 0x1d);
		Graphics g2(*g);
		g2.SetDrawMode(1);
		g2.ClipRect(0x8a, 0x25, 0xc, 0x12);
		g2.DrawImage(IMG(IMAGE_FLAME), mTorchFrame * -0xc + 0x8a, 0x25);
	}
	bool aRightTorch = !(((mCells[4][0] & EXIT_NORTH) && mSize == 6) || ((mCells[5][0] & EXIT_NORTH) && mSize == 8) ||
		((mCells[6][0] & EXIT_NORTH) && mSize == 8) || ((mCells[7][0] & EXIT_NORTH) && mSize == 10));
	if (aRightTorch)
	{
		g->DrawImage(IMG(IMAGE_TORCH), 0x132, 0x35);
		Graphics g1(*g);
		g1.SetDrawMode(1);
		g1.ClipRect(0x125, 0x1d, 0x24, 0x24);
		g1.DrawImage(IMG(IMAGE_GLOW), mTorchFrame * -0x24 + 0x125, 0x1d);
		Graphics g2(*g);
		g2.SetDrawMode(1);
		g2.ClipRect(0x131, 0x25, 0xc, 0x12);
		g2.DrawImage(IMG(IMAGE_FLAME), mTorchFrame * -0xc + 0x131, 0x25);
	}

	// the floor and the stairs at the exits
	g->DrawImage(IMG(IMAGE_FLOOR6 + aSize), 0x36, 0x51);
	for (int y = 0; y < mSize; y++)
	{
		for (int x = 0; x < mSize; x++)
		{
			int aFlags = mCells[x][y];
			Graphics gc(*g);
			if (aFlags & EXIT_NORTH)
			{
				DrawClipped(&gc, IMG(IMAGE_STAIRS6 + aSize), PixelX(x), PixelY(0) - 0x42, PixelX(x), PixelY(0) - 0x42, aCell, 0x42);
			}
			if (aFlags & EXIT_SOUTH)
			{
				DrawClipped(&gc, IMG(IMAGE_STAIRS6 + aSize), PixelX(x) + (-0x34 - aCell), PixelY(y + 1), PixelX(x), PixelY(y + 1), aCell, 0x42);
			}
			if (aFlags & EXIT_WEST)
			{
				DrawClipped(&gc, IMG(IMAGE_STAIRS6 + aSize), (-0x1a - aCell) * 2, PixelY(y), 0, PixelY(y), 0x34, 0x42);
			}
			if (aFlags & EXIT_EAST)
			{
				DrawClipped(&gc, IMG(IMAGE_STAIRS6 + aSize), PixelX(x + 1) - aCell, PixelY(y), PixelX(x + 1), PixelY(y), 0x34, 0x42);
			}
		}
	}

	// the skull traps and their sparkles
	for (int y = 0; y < mSize; y++)
	{
		for (int x = 0; x < mSize; x++)
		{
			int aShift = 0;
			if (mExitStyle == 1 && mDeathTimer > 0 && x == mExplorer->mX && y == mExplorer->mY)
				aShift = mDeathTimer * 3;
			Graphics gc(*g);
			gc.ClipRect(PixelX(x), PixelY(y) + 1, aCell, aCell);
			bool aIsTrap0 = (x == mTrapX[0] && y == mTrapY[0]);
			bool aIsTrap1 = (x == mTrapX[1] && y == mTrapY[1]);
			if (aIsTrap0 || aIsTrap1)
			{
				gc.DrawImage(IMG(IMAGE_BACKWALL6 + aSize), PixelX(x), PixelY(y));
				gc.DrawImage(IMG(IMAGE_TRAP6 + aSize), PixelX(x) + aShift, PixelY(y) + 1);
				if (((x + y) & 1) == 0)
					gc.DrawImage(IMG(IMAGE_FLOORDARK6 + aSize), PixelX(x), PixelY(y) + 1);
				else
					gc.DrawImage(IMG(IMAGE_FLOORLIGHT6 + aSize), PixelX(x), PixelY(y) + 1);
			}
			if ((aIsTrap0 && mTrapAnim[0] > -1) || (aIsTrap1 && mTrapAnim[1] > -1))
			{
				int aFrame = aIsTrap0 ? mTrapAnim[0] : mTrapAnim[1];
				int aOff1 = 0xd, aOff2 = 0x1b, aYOff = 0x13, aSpark = 0x14;
				if (aSize == 1)
				{
					aOff2 = 0x14;
					aOff1 = 10;
					aYOff = 0xe;
					aSpark = 0xf;
				}
				else if (aSize == 2)
				{
					aOff1 = 9;
					aOff2 = 0x11;
					aYOff = 0xc;
					aSpark = 10;
				}
				Rect aSrc(aSpark * aFrame, 0, aSpark, aSpark);
				gc.DrawImage(IMG(IMAGE_TRAPSPARKLE6 + aSize), PixelX(x) + aOff1 + aShift, PixelY(y) + 1 + aYOff, aSrc);
				gc.DrawImage(IMG(IMAGE_TRAPSPARKLE6 + aSize), PixelX(x) + aOff2 + aShift, PixelY(y) + 1 + aYOff, aSrc);
			}
		}
	}

	// the ending scene's stairs and treasure chest
	int aMode = mApp->mGameMode;
	if (aMode == 2 || aMode == 3)
	{
		Graphics gc(*g);
		gc.ClipRect(PixelX(2) + 0x1e, PixelY(6), 0x3c, 0x42);
		gc.DrawImage(IMG(IMAGE_STAIRS6), PixelX(2) - 0x52, PixelY(6));
		g->DrawImage(IMG(IMAGE_TREASURECHEST), PixelX(2) - 0xf, PixelY(2) - 0xf);
	}

	// the walls, row by row, with the figures in their rows
	Graphics gm(*g);
	gm.ClipRect(0x32, 0x42, 0x16e, 0x177);
	for (int y = -2; y < mSize + 2; y++)
	{
		for (int x = 0; x < mSize; x++)
		{
			if (mState == STATE_READY && x >= 0 && x < NUM_HOVER && y >= 0 && y < NUM_HOVER && 0 < mHover[x][y])
				DrawMoveHint(mExplorer->mX, mExplorer->mY, x, y, g);
			if (x == mClickX && y == mClickY)
				DrawMoveHint(mExplorer->mTargetX, mExplorer->mTargetY, x, y, g);
			if (mApp->mGameMode > 3 && x == mExplorer->mX && y == mExplorer->mY)
				DrawTutorialArrow(mExplorer->mX, mExplorer->mY, g);
			int aFlags = (y >= 0 && y < MAX_SIZE) ? mCells[x][y] : 0;
			if (y < 0 || mSize - 1 <= y)
			{
				if (y == mSize - 1)
				{
					if ((aFlags & WALL_EAST) && x < mSize - 1)
					{
						int aW = 0xc, aH = 0x4b, aImgOff = 0x54, aYOff = 0;
						if (mSize == 8)
						{
							aImgOff = 0x45;
							aH = 0x3c;
						}
						if (mSize == 10)
						{
							aW = 8;
							aH = 0x2e;
							aImgOff = 0x34;
							aYOff = 5;
						}
						WALL(&gm, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff, aW, aH, ((x + 1) * aCell - aImgOff) + 0x33,
							aCell * y + 0x42 + aYOff);
					}
				}
			}
			else
			{
				if ((aFlags & WALL_EAST) && x < mSize - 1)
				{
					int aW = 0xc, aH = 0x4e, aYOff = 0;
					if (mSize == 8)
						aH = 0x3f;
					if (mSize == 10)
					{
						aW = 8;
						aH = 0x30;
						aYOff = 5;
					}
					WALL(&gm, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff, aW, aH, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff);
				}
				if ((aFlags & WALL_SOUTH) && y < mSize - 1 && (mGateX != x || mGateY != y))
				{
					int aClipW = 0x48, aClipH = 0x12, aImgOff = 0xc, aYOff = 0;
					if (mSize == 8)
						aClipW = 0x39;
					if (mSize == 10)
					{
						aImgOff = 8;
						aYOff = 5;
						aClipW = 0x2c;
						aClipH = 0xc;
					}
					WALL(&gm, aCell * x + 0x33, (y + 1) * aCell + 0x42 + aYOff, aClipW, aClipH, (aCell * x - aImgOff) + 0x33,
						(y + 1) * aCell + 0x42 + aYOff);
				}
				if (x == mGateX && y == mGateY)
				{
					int aClipW = 0x48, aClipH = 0x12, aYOff = 0;
					if (mSize == 8)
						aClipW = 0x39;
					if (mSize == 10)
					{
						aClipW = 0x2c;
						aClipH = 0xc;
						aYOff = 5;
					}
					DrawClipped(&gm, IMG(IMAGE_GATE6 + aSize), (aCell * x - mGateAnim * aClipW) + 0x33, (y + 1) * aCell + 0x42 + aYOff,
						aCell * x + 0x33, (y + 1) * aCell + 0x42 + aYOff, aClipW, aClipH);
				}
			}
		}
		// the key, the figures of this row
		if (mKeyY == y)
		{
			Graphics gk(*g);
			int aKX = PixelX(mKeyX) + (aCell - 0x24) / 2;
			int aKY = PixelY(mKeyY) + (aCell - 0x24) / 2;
			gk.ClipRect(aKX, aKY, 0x24, 0x24);
			gk.DrawImage(IMG(IMAGE_KEY), aKX + mKeyFrame * -0x24, aKY);
		}
		if (mScorpion->mAlive)
		{
			int aRow = mScorpion->mToY;
			if (aRow < mScorpion->mY)
				aRow = mScorpion->mY;
			if (aRow == y)
				DrawScorpion(g);
		}
		for (int i = 0; i < 2; i++)
		{
			Mummy* m = mMummies[i];
			if (m != NULL)
			{
				int aRow = m->mY;
				if (m->mY <= m->mTargetY)
					aRow = m->mTargetY;
				if (aRow == y && m->mAlive)
					DrawMummy(g, i, true);
			}
		}
		if (theWithExplorer)
		{
			int aRow = mExplorer->mTargetY;
			if (aRow < mExplorer->mY)
				aRow = mExplorer->mY;
			if (aRow == y)
				DrawExplorer(g);
		}
		Scorpion* s = mScorpion;
		if (s->mAlive && s->mStinging && s->mToX == mExplorer->mX && s->mToY == mExplorer->mY)
		{
			int aRow = s->mY;
			if (s->mY <= s->mToY)
				aRow = s->mToY;
			if (aRow == y)
				DrawScorpion(g);
		}
		// while he falls into a pit the walls east of the row cover him again
		if (mDeathTimer > 0 && mExitStyle == 1 && mSize > 0)
		{
			for (int x = 1; x <= mSize; x++)
			{
				int aFlags2 = (y >= 0 && y < MAX_SIZE && x - 1 < MAX_SIZE) ? mCells[x - 1][y] : 0;
				if (y < 0 || mSize - 1 <= y)
				{
					if (y == mSize - 1 && (aFlags2 & WALL_EAST) && x - 1 < mSize - 1)
					{
						int aW = 0xc, aH = 0x4b, aImgOff = 0x54, aYOff = 0;
						if (mSize == 8)
						{
							aImgOff = 0x45;
							aH = 0x3c;
						}
						if (mSize == 10)
						{
							aW = 8;
							aH = 0x2e;
							aImgOff = 0x34;
							aYOff = 5;
						}
						WALL(&gm, aCell * x + 0x33, aCell * y + 0x42 + aYOff, aW, aH, (aCell * x - aImgOff) + 0x33, aCell * y + 0x42 + aYOff);
					}
				}
				else if ((aFlags2 & WALL_EAST) && x - 1 < mSize - 1)
				{
					int aW = 0xc, aH = 0x4e, aYOff = 0;
					if (mSize == 8)
						aH = 0x3f;
					if (mSize == 10)
					{
						aW = 8;
						aH = 0x30;
						aYOff = 5;
					}
					WALL(&gm, aCell * x + 0x33, aCell * y + 0x42 + aYOff, aW, aH, aCell * x + 0x33, aCell * y + 0x42 + aYOff);
				}
			}
		}
	}
	// the second loop: the trap's block, and the near half of the walls over the figures
	DrawNearWalls(&gm, g);
	g->Translate(-10, 0);
	if (mApp->mGameMode > 3)
		DrawTutorialText(0, PixelX(5) + 0x11, mWidth, (mHeight - PixelX(5)) - 0x11, g);
}

// the skull trap's block that falls on the explorer (the first part of the second loop of 0x4095f0): 0x36 x 0x5a at 6 x 6, a
// sheet of frames; shown from the 0x29th update of the death, then one frame per four updates after the 0x81st
void Board::DrawTrapBlock(Graphics* g, int x, int y)
{
	if (mDeathTimer < 0 || mExitStyle != 0)
		return;
	bool aOnTrap0 = x == mTrapX[0] && y == mTrapY[0] && mExplorer->mX == mTrapX[0] && mExplorer->mY == mTrapY[0];
	bool aOnTrap1 = x == mTrapX[1] && y == mTrapY[1] && mExplorer->mX == mTrapX[1] && mExplorer->mY == mTrapY[1];
	if (!aOnTrap0 && !aOnTrap1)
		return;
	int aW = 0x36, aH = 0x5a;
	if (mSize == 8)
	{
		aW = 0x27;
		aH = 0x41;
	}
	else if (mSize == 10)
	{
		aW = 0x20;
		aH = 0x35;
	}
	int aPY = PixelY(y);
	Graphics gc(*g);
	gc.ClipRect(PixelX(x) + (mCellSize - aW) / 2, aPY, aW, aH);
	int aX;
	if (mDeathTimer < 0x82)
	{
		if (mDeathTimer <= 0x28)
			return;
		aX = PixelX(x) + (mCellSize - aW) / 2;
	}
	else
	{
		int aStep = mDeathTimer - 0x81;
		if (aStep > 0x3f)
			aStep = 0x3f;
		aX = PixelX(x) + (mCellSize - aW) / 2 - ((aStep + ((aStep >> 31) & 3)) >> 2) * aW;
	}
	gc.DrawImage(IMG(IMAGE_BLOCK6 + SizeIndex()), aX, aPY);
}

// the near half of the walls (the east walls 6 wide, the south walls and the gate cut short), drawn over the figures: the end of
// DrawFront (0x40b2f0) and the second loop of DrawBoard (0x4095f0). Inside the caller's translation, clipped to the maze.
void Board::DrawNearWalls(Graphics* theClip, Graphics* theBlockGraphics)
{
	int aSize = SizeIndex();
	int aCell = mCellSize;
	Graphics& gm = *theClip;
	for (int y = -2; y < mSize + 2; y++)
	{
		for (int x = 0; x < mSize; x++)
		{
			if (theBlockGraphics != NULL)
				DrawTrapBlock(theBlockGraphics, x, y);
			int aFlags = (y >= 0 && y < MAX_SIZE) ? mCells[x][y] : 0;
			if (y < 0 || mSize - 1 <= y)
			{
				if (y == mSize - 1 && (aFlags & WALL_EAST) && x < mSize - 1)
				{
					int aW = 6, aH = 0x4b, aImgOff = 0x54, aYOff = 0;
					if (mSize == 8)
					{
						aImgOff = 0x45;
						aH = 0x3c;
					}
					if (mSize == 10)
					{
						aW = 4;
						aH = 0x2e;
						aImgOff = 0x34;
						aYOff = 5;
					}
					WALL(&gm, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff, aW, aH, ((x + 1) * aCell - aImgOff) + 0x33,
						aCell * y + 0x42 + aYOff);
				}
			}
			else
			{
				if ((aFlags & WALL_EAST) && x < mSize - 1)
				{
					int aW = 6, aH = 0x43, aYOff = 0;
					if (mSize == 8)
						aH = 0x34;
					if (mSize == 10)
					{
						aW = 4;
						aH = 0x29;
						aYOff = 5;
					}
					WALL(&gm, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff, aW, aH, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff);
				}
				if ((aFlags & WALL_SOUTH) && y < mSize - 1 && (mGateX != x || mGateY != y))
				{
					int aClipW = 0x42, aClipH = 10, aImgOff = 0xc, aYOff = 0;
					if (mSize == 8)
						aClipW = 0x33;
					if (mSize == 10)
					{
						aClipW = 0x28;
						aClipH = 9;
						aImgOff = 8;
						aYOff = 5;
					}
					WALL(&gm, aCell * x + 0x33, (y + 1) * aCell + 0x42 + aYOff, aClipW, aClipH, (aCell * x - aImgOff) + 0x33,
						(y + 1) * aCell + 0x42 + aYOff);
				}
				if (x == mGateX && y == mGateY)
				{
					int aClipW = 0x48, aClipH = 0xc, aYOff = 0;
					if (mSize == 8)
						aClipW = 0x39;
					if (mSize == 10)
					{
						aClipW = 0x2c;
						aClipH = 9;
						aYOff = 5;
					}
					DrawClipped(&gm, IMG(IMAGE_GATE6 + aSize), (aCell * x - mGateAnim * aClipW) + 0x33, (y + 1) * aCell + 0x42 + aYOff,
						aCell * x + 0x33, (y + 1) * aCell + 0x42 + aYOff, aClipW, aClipH);
				}
			}
		}
	}
}

// 0x40b2f0: the walls drawn again over the figures, half as wide on the near side
void Board::DrawFront(Graphics* g)
{
	g->Translate(10, 0);
	int aSize = SizeIndex();
	int aCell = mCellSize;
	Graphics gm(*g);
	gm.ClipRect(0x32, 0x42, 0x16e, 0x177);
	for (int i = 0; i < 2; i++)
		if (mMummies[i] != NULL && mMummies[i]->mAlive)
			DrawMummy(g, i, true);
	for (int y = -2; y < mSize + 2; y++)
	{
		for (int x = 0; x < mSize; x++)
		{
			int aFlags = (y >= 0 && y < MAX_SIZE) ? mCells[x][y] : 0;
			if (y < 0 || mSize - 1 <= y)
			{
				if (y == mSize - 1 && (aFlags & WALL_EAST) && x < mSize - 1)
				{
					int aW = 0xc, aH = 0x4b, aImgOff = 0x54, aYOff = 0;
					if (mSize == 8)
					{
						aImgOff = 0x45;
						aH = 0x3c;
					}
					if (mSize == 10)
					{
						aW = 8;
						aH = 0x2e;
						aImgOff = 0x34;
						aYOff = 5;
					}
					WALL(&gm, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff, aW, aH, ((x + 1) * aCell - aImgOff) + 0x33,
						aCell * y + 0x42 + aYOff);
				}
			}
			else
			{
				if ((aFlags & WALL_EAST) && x < mSize - 1)
				{
					int aW = 0xc, aH = 0x4e, aYOff = 0;
					if (mSize == 8)
						aH = 0x3f;
					if (mSize == 10)
					{
						aW = 8;
						aH = 0x30;
						aYOff = 5;
					}
					WALL(&gm, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff, aW, aH, (x + 1) * aCell + 0x33, aCell * y + 0x42 + aYOff);
				}
				if ((aFlags & WALL_SOUTH) && y < mSize - 1 && (mGateX != x || mGateY != y))
				{
					int aClipW = 0x48, aClipH = 0x12, aImgOff = 0xc, aYOff = 0;
					if (mSize == 8)
						aClipW = 0x39;
					if (mSize == 10)
					{
						aImgOff = 8;
						aYOff = 5;
						aClipW = 0x2c;
						aClipH = 0xc;
					}
					WALL(&gm, aCell * x + 0x33, (y + 1) * aCell + 0x42 + aYOff, aClipW, aClipH, (aCell * x - aImgOff) + 0x33,
						(y + 1) * aCell + 0x42 + aYOff);
				}
				if (x == mGateX && y == mGateY)
				{
					int aClipW = 0x48, aClipH = 0x12, aYOff = 0;
					if (mSize == 8)
						aClipW = 0x39;
					if (mSize == 10)
					{
						aClipW = 0x2c;
						aClipH = 0xc;
						aYOff = 5;
					}
					DrawClipped(&gm, IMG(IMAGE_GATE6 + aSize), (aCell * x - mGateAnim * aClipW) + 0x33, (y + 1) * aCell + 0x42 + aYOff,
						aCell * x + 0x33, (y + 1) * aCell + 0x42 + aYOff, aClipW, aClipH);
				}
			}
		}
		int aRow = mExplorer->mY;
		if (mExplorer->mTargetY > aRow)
			aRow = mExplorer->mTargetY;
		if (aRow == y)
			DrawExplorer(g);
	}
	// the second pass: the near half of the walls once more, over the figures
	DrawNearWalls(&gm);
	g->Translate(-10, 0);
	if (mApp->mGameMode > 3)
		DrawTutorialText(0, PixelX(5) + 0x11, mWidth, (mHeight - PixelX(5)) - 0x11, g);
}

// 0x40bf70
void Board::Draw(Graphics* g)
{
	if (mState == STATE_DONE)
		return;
	int aMode = mApp->mGameMode;
	if (mFade >= 1.0)
	{
		if (mState == STATE_ENDING_FADE || mState == STATE_ENDING)
		{
			g->SetColor(Color(0, 0, 0));
			g->FillRect(0, 0, mWidth, mHeight);
			double aAlpha = mEndFade < 0.0 ? 0.0 : mEndFade;
			g->SetColor(Color(0xff, 0xff, 0xff, (int) aAlpha));
			g->SetColorizeImages(true);
		}
		DrawBoard(g, true);
		if (mFade2 < 1.0 && mState != STATE_ENDING_FADE)
			DrawFront(g);
		if (mState == STATE_ENDING_FADE || mState == STATE_ENDING)
			g->SetColorizeImages(false);
		if (mFade >= 1.0 && mApp->mShowClock && aMode < 2 && mState != STATE_LOST && !mShowingSolution)
		{
			g->SetFont(mApp->mFont1);
			g->SetColor(Color(0, 0xff, 0));
			int aTime = ((int) (mApp->mElapsedBase - mTimeBase) + mUpdateCnt) / 100;
			char aBuf[256];
			sprintf(aBuf, (aTime % 60) < 10 ? "%d:0%d" : "%d:%d", aTime / 60, aTime % 60);
			SexyString aStr = aBuf;
			g->DrawString(aStr, (mWidth - mApp->mFont1->StringWidth(aStr)) - 0x11, 0x19);
		}
	}
	else if (mState == STATE_ENTER)
	{
		g->DrawImage(IMG(IMAGE_BACKDROP), -0x96, 0);
		g->SetColor(Color(0, 0, 0));
		g->FillRect(3, 3, mWidth - 6, mHeight - 6);
		g->Translate(10, 0);
		DrawExplorer(g);
		g->Translate(-10, 0);
	}
	else
	{
		g->SetColor(Color(0, 0, 0));
		g->FillRect(0, 0, mWidth, mHeight);
		if (!mDarkImageValid)
		{
			mDarkImage = new MemoryImage(mApp);
			mDarkImage->Create(mWidth, mHeight);
			mDarkImage->SetImageMode(true, true);
			mDarkImage->SetVolatile(true);
			Graphics gi(mDarkImage);
			DrawBoard(&gi, true);
			DrawFront(&gi);
			mDarkImageValid = true;
		}
		// the light grows around the explorer as the board fades in
		int aRadius = (int) (mFade * 1400.0);
		if (aRadius == 0)
			aRadius = 1;
		int aScale = 0x10000 / aRadius;
		int aCX = PixelX(mExplorer->mX) - (int) ((double) mCellSize * -0.75);
		int aCY = PixelY(mExplorer->mY) - (int) ((double) mCellSize * -0.5);
		if (mDarkImageValid)
		{
			ulong* aBits = mDarkImage->GetBits();
			int aW = mDarkImage->GetWidth();
			int aH = mDarkImage->GetHeight();
			int aRowTerm = aScale * 3;
			for (int y = 3; y < aH - 6; y++)
			{
				int aMY = ((aRowTerm - aCY * aScale) >> 8) + 0x80;
				int aColTerm = aScale * 3;
				ulong* aPix = aBits + aW * y + 3;
				for (int x = 3; x < aW - 6; x++)
				{
					int aMX = ((aColTerm - aCX * aScale) >> 8) + 0x80;
					ulong anAlpha;
					if (aMX < 0 || aMX > 0xff || aMY < 0 || aMY > 0xff)
						anAlpha = 0;
					else
						anAlpha = mApp->mLightMask[aMY][aMX];
					*aPix = (anAlpha << 24) | (*aPix & 0xffffff);
					aPix++;
					aColTerm += aScale;
				}
				aRowTerm += aScale;
			}
			mDarkImage->BitsChanged();
			g->DrawImage(mDarkImage, 0, 0);
		}
		int anAlpha = (int) ((1.0 - (double) aRadius * 0.015625) * 255.0);
		if (0 < anAlpha)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
			g->Translate(10, 0);
			DrawExplorer(g);
			g->Translate(-10, 0);
			g->SetColorizeImages(false);
		}
	}
	if (aMode != 2 && aMode != 3 && mDark)
		DrawDark(g);
	if (mState == STATE_SOLVING)
		DrawSolving(0x91, 0xb4, 200, 0x78, g);
	if (mState == STATE_ENDING)
		DrawEnding(g);
	if (!mShowingSolution)
	{
		if (!mUnsolvable)
			return;
		g->SetFont(mApp->mFont1);
		g->SetColor(Color(0xff, 0, 0));
		g->DrawString(GT(0x00491e80), 0x11, 0x19);
		return;
	}
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0, 0xff, 0));
	g->DrawString(GT(0x00491ea0), 0x11, 0x19);
}
