// Board.cpp, part 7: drawing (WinSS.exe 0x406400 Draw, 0x4093f0 the framed box, 0x409840 the arrow of a hovered
// cell, 0x409d10 the pirate's gun-range marks).
//
// Sprite sheets are drawn the way the original does: a clip rectangle on the cell and the whole sheet shifted so
// that the wanted frame lies in it (a ship's sheet has 16 headings in columns and 8 animation frames in rows).

#include "Board.h"
#include "SevenSeasApp.h"
#include "Anims.h"
#include "Res.h"
#include "Compat2004.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cstdio>
#include <cstdlib>

using namespace Sexy;

// 0x4093f0: a box made of the nine parts of the wood frame image
void Board::DrawBox(int theX, int theY, int theWidth, int theHeight, Graphics* g)
{
	g->Translate(theX, theY);
	Image* aFrame = mApp->mImages[IMAGE_DIALOGWOOD];
	int aW = aFrame->GetWidth() / 3;
	int aH = aFrame->GetHeight() / 3;
	g->DrawImage(aFrame, 0, 0, Rect(0, 0, aW, aH));
	int aRight = theWidth - aW;
	g->DrawImage(aFrame, aRight, 0, Rect(aW * 2, 0, aW, aH));
	int aBottom = theHeight - aH;
	g->DrawImage(aFrame, 0, aBottom, Rect(0, aH * 2, aW, aH));
	g->DrawImage(aFrame, aRight, aBottom, Rect(aW * 2, aH * 2, aW, aH));

	// the top and bottom edges
	Graphics gTop(*g);
	int aInnerW = theWidth - aW * 2;
	gTop.ClipRect(aW, 0, aInnerW, theHeight);
	int aCountX = (aRight - 1) / aW;
	int aTileX = aW;
	int aLeft = aCountX;
	if (0 < aCountX)
	{
		do
		{
			gTop.DrawImage(aFrame, aTileX, 0, Rect(aW, 0, aW, aH));
			gTop.DrawImage(aFrame, aTileX, aBottom, Rect(aW, aH * 2, aW, aH));
			aLeft = aLeft - 1;
			aTileX = aTileX + aW;
		}
		while (aLeft != 0);
	}
	// the left and right edges
	Graphics gSide(*g);
	int aInnerH = theHeight - aH * 2;
	gSide.ClipRect(0, aH, theWidth, aInnerH);
	int aCountY = (aBottom - 1) / aH;
	int aTileY = aH;
	aLeft = aCountY;
	if (0 < aCountY)
	{
		do
		{
			gSide.DrawImage(aFrame, 0, aTileY, Rect(0, aH, aW, aH));
			gSide.DrawImage(aFrame, aRight, aTileY, Rect(aW * 2, aH, aW, aH));
			aLeft = aLeft - 1;
			aTileY = aTileY + aH;
		}
		while (aLeft != 0);
	}
	// the middle
	Graphics gMid(*g);
	gMid.ClipRect(aW, aH, aInnerW, aInnerH);
	if (0 < aCountX)
	{
		int aCols = aCountX;
		int aX = aW;
		do
		{
			if (0 < aCountY)
			{
				int aRows = aCountY;
				int aY = aH;
				do
				{
					gMid.DrawImage(aFrame, aX, aY, Rect(aW, aH, aW, aH));
					aY = aY + aH;
					aRows = aRows - 1;
				}
				while (aRows != 0);
			}
			aX = aX + aW;
			aCols = aCols - 1;
		}
		while (aCols != 0);
	}
	g->Translate(-theX, -theY);
}

// 0x409840: the arrow on a cell next to the ship that a click would sail to (or the broadside's range, when it is
// the ship's own cell); theFromX/Y is the ship, theX/Y the cell
void Board::DrawCellArrow(int theFromX, int theFromY, int theX, int theY, bool theWithRange, Graphics* g)
{
	if ((theX < 0) || (10 < theX) || (theY < 0) || (10 < theY))
		return;
	if (!CellFree(theX, theY))
		return;
	Graphics g2(*g);
	g2.ClipRect(CellToPixel(theX), CellToPixel(theY), CELL_SIZE, CELL_SIZE);
	Image* anImage = NULL;
	int aX = 0;
	int aY = 0;
	bool aOwnCell = false;
	if (theX - theFromX == -1)
	{
		if (theY - theFromY == -1)
		{
			aY = CellToPixel(theY) + 4;
			aX = CellToPixel(theX) + 4;
			anImage = mApp->mArrows[0];
		}
		else if (theY == theFromY)
		{
			aY = CellToPixel(theY) + 4;
			aX = CellToPixel(theX);
			anImage = mApp->mArrows[1];
		}
		else
		{
			if (theY - theFromY != 1)
				return;
			aY = CellToPixel(theY) - 4;
			aX = CellToPixel(theX) + 4;
			anImage = mApp->mArrows[2];
		}
	}
	else if (theX == theFromX)
	{
		if (theY - theFromY == -1)
		{
			aY = CellToPixel(theY);
			aX = CellToPixel(theX);
			anImage = mApp->mArrows[3];
		}
		else
		{
			if (theY == theFromY)
				aOwnCell = true;
			else
			{
				if (theY - theFromY != 1)
					return;
				aY = CellToPixel(theY) - 4;
				aX = CellToPixel(theX);
				anImage = mApp->mArrows[5];
			}
		}
		if (!aOwnCell)
		{
			g2.DrawImage(anImage, aX + 4, aY);
		}
	}
	else
	{
		if (theX - theFromX != 1)
			return;
		if (theY - theFromY == -1)
		{
			aY = CellToPixel(theY) + 4;
			aX = CellToPixel(theX);
			anImage = mApp->mArrows[6];
		}
		else if (theY == theFromY)
		{
			aY = CellToPixel(theY) + 4;
			aX = CellToPixel(theX) - 4;
			anImage = mApp->mArrows[7];
		}
		else
		{
			if (theY - theFromY != 1)
				return;
			aY = CellToPixel(theY) - 4;
			aX = CellToPixel(theX);
			anImage = mApp->mArrows[8];
		}
	}
	if (aOwnCell)
	{
		// the cell of the ship: the range of the broadside in each direction, as a pulsing colour
		for (int aDX = -1; aDX < 2; aDX++)
		{
			for (int aDY = -1; aDY < 2; aDY++)
			{
				int aRange = FireRange(mPlayerX, mPlayerY, mHeading, mPlayerX + aDX, mPlayerY + aDY);
				if (0 < aRange)
				{
					Graphics g3(*g);
					g3.SetColorizeImages(true);
					int aPhase = abs(mUpdateCnt % 200);
					int aGreen;
					if (aPhase * 2 < 200)
						aGreen = aPhase * 2 + 0x37;
					else
						aGreen = aPhase * -2 + 0x1c7;
					g3.SetColor(Color(0xff, aGreen, 0));
					int aReach;
					if (!mApp->mTutorial)
						aReach = (mCellFlash[theY][theX] - mUnused_95008) * 3;
					else
						aReach = mCellFlash[theY][theX] * 3;
					if (aReach > aRange * CELL_SIZE)
						aReach = aRange * CELL_SIZE;
					g3.ClipRect(CellToPixel(theX) + aReach * aDX, CellToPixel(theY) + aReach * aDY, CELL_SIZE, CELL_SIZE);
					g3.DrawImage(mApp->mImages[IMAGE_CROSSHAIR], CellToPixel(theX) + 8 + aReach * aDX,
						CellToPixel(theY) + 8 + aReach * aDY);
					g3.SetColorizeImages(false);
				}
			}
		}
	}
	if (theWithRange)
		DrawGunRange(theX, theY, g);
}

// 0x409d10: on the easiest level the squares where a pirate ship's guns reach are marked
void Board::DrawGunRange(int theX, int theY, Graphics* g)
{
	if ((mApp->mDifficulty < 2) && !mApp->mTutorial)
	{
		for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
		{
			Ship* aShip = *anItr;
			if (aShip->mType == 0)
			{
				int aDX = (aShip->mX < theX) ? 1 : 0;
				if (theX < aShip->mX)
					aDX = aDX - 1;
				int aDY = (aShip->mY < theY) ? 1 : 0;
				if (theY < aShip->mY)
					aDY = aDY - 1;
				Graphics g2(*g);
				g2.ClipRect(CellToPixel(aDX + aShip->mX) + aDX * -0xe + 0xc, CellToPixel(aShip->mY + aDY) + aDY * -0xe + 0xc, 0xe, 0xe);
				g2.DrawImage(mApp->mImages[IMAGE_ENEMYARROWS], CellToPixel(aDX + aShip->mX) - 2, CellToPixel(aShip->mY + aDY) - 2);
			}
		}
	}
}

// 0x406400
void Board::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;

	// the sea, scrolling in two directions
	int aWaveY = (int) mWaveB;
	int aWaveX = (int) mWaveA;
	g->DrawImage(anImages[IMAGE_WATER], aWaveX, aWaveY);
	g->DrawImage(anImages[IMAGE_WATER], aWaveX - 500, aWaveY);
	g->DrawImage(anImages[IMAGE_WATER], aWaveX, aWaveY - 500);
	g->DrawImage(anImages[IMAGE_WATER], aWaveX - 500, aWaveY - 500);
	g->DrawImage(anImages[IMAGE_BACKDROP], 0, 0, Rect(0xad, 0, 0x1d3, 0x1e0));
	g->Translate(0, 0xc);

	if (mBonus)
	{
		DrawBonus(g);
		return;
	}

	// the fade of the whole board at the start of a level, and at the game's end
	if (mState == STATE_LEVEL_END)
	{
		g->SetColorizeImages(true);
		int anAlpha = (mGameStartCnt - mUpdateCnt) + 0xff;
		if (anAlpha < 0)
			anAlpha = 0;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
	}
	else
	{
		int aSince = mUpdateCnt - mLevelStartCnt;
		if (aSince < 0x50)
		{
			int anAlpha = (int) ((double) aSince * 255.0 / 80.0);
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		}
	}

	// animations drawn behind the grid (the wakes), additive
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (std::list<AnimInstance*>::iterator anItr = mAnims.begin(); anItr != mAnims.end(); ++anItr)
	{
		AnimInstance* anAnim = *anItr;
		if (anAnim->mFlag)
		{
			AnimDef* aDef = anAnim->mDef;
			const AnimFrame& aFrame = aDef->mFrames[anAnim->mFrameIdx];
			int aX = anAnim->mX;
			int aY = anAnim->mY;
			if (aDef->mFlagA == 0)
				aX = aX - aFrame.mW / 2;
			else if (aDef->mFlagA == 1)
				aX = aX + (1 - aFrame.mW);
			if (aDef->mFlagB == 0)
				aY = aY - aFrame.mH / 2;
			else if (aDef->mFlagB == 1)
				aY = aY + (1 - aFrame.mH);
			if ((aDef->mFlagA == 1) && (aDef->mFlagB == 0))
			{
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				aY = aY + anAnim->mFrameIdx;
			}
			else if ((aDef->mFlagA == 0) && (aDef->mFlagB == 1))
			{
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				int aShift = (int) (mWaveSpeedA * 10.0);
				aY = aY - aShift;
				aX = aX - aShift;
			}
			else if ((aDef->mFlagA == 1) && (aDef->mFlagB == 1))
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->DrawImage(aDef->mImage, aX, aY, Rect(aFrame.mX, aFrame.mY, aFrame.mW, aFrame.mH));
		}
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->DrawImage(anImages[IMAGE_GRID], -6, -0xc);

	// the whirlpools in the corners
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (std::list<Port*>::iterator anItr = mPorts.begin(); anItr != mPorts.end(); ++anItr)
	{
		Port* aPort = *anItr;
		Graphics g2(*g);
		g2.ClipRect(CellToPixel(aPort->mX), CellToPixel(aPort->mY), 0x28, 0x28);
		Image* anImage = NULL;
		int aY = 0;
		if ((!aPort->mActive || (aPort->mAppear < 0x10)) && (-1 < aPort->mAppear))
		{
			if (!aPort->mActive)
			{
				if (aPort->mAppear < 0x10)
				{
					aY = CellToPixel(aPort->mY) + aPort->mAppear * -0x28;
					anImage = anImages[IMAGE_WHIRLDISAPPEAR];
				}
			}
			else if (aPort->mAppear < 0x10)
			{
				aY = CellToPixel(aPort->mY) + aPort->mAppear * -0x28;
				anImage = anImages[IMAGE_WHIRLAPPEAR];
			}
		}
		else
		{
			aY = CellToPixel(aPort->mY) + aPort->mFrame * -0x28;
			anImage = anImages[IMAGE_WHIRL];
		}
		if (anImage != NULL)
			g2.DrawImage(anImage, CellToPixel(aPort->mX), aY);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	// the sea cells with a wreck, the islands
	for (int aX = 0; aX < GRID; aX++)
	{
		for (int aY = 0; aY < GRID; aY++)
		{
			if (mTerrain[aX][aY] < 1)
			{
				if (mVisited[aX][aY])
				{
					Graphics g2(*g);
					g2.ClipRect(CellToPixel(aX), CellToPixel(aY), 0x28, 0x28);
					g2.DrawImage(anImages[IMAGE_RUBBLE], CellToPixel(aX) + ((mUpdateCnt / 0x1e + aY + aX) % 0xc) * -0x28, CellToPixel(aY));
				}
			}
			else
			{
				Graphics g2(*g);
				g2.ClipRect(CellToPixel(aX) - 2, CellToPixel(aY) - 2, 0x2a, 0x2a);
				int aFrame = (mUpdateCnt / 0xe + aY + aX) % 4;
				g2.DrawImage(anImages[IMAGE_ISLANDS], CellToPixel(aX) + aFrame * -0x2a - 2,
					CellToPixel(aY) + mTerrain[aX][aY] * -0x2a + 0x28);
			}
		}
	}

	// the player's ship
	if (((mState == STATE_WHIRL_OUT) || (mState == STATE_WHIRL_IN)) && (mHeading == 0))
	{
		if (mState == STATE_WHIRL_IN)
		{
			Graphics g2(*g);
			g2.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			int aY = CellToPixel(mPlayerY) + 4 + (mDestY - mPlayerY) * mMoveProgress;
			g2.ClipRect(CellToPixel(mPlayerX) - 0xc + (mDestX - mPlayerX) * mMoveProgress, aY, 0x40, 0x20);
			g2.DrawImage(anImages[IMAGE_WAKEBRIGHT], CellToPixel(mPlayerX) - 0x3cc + (mDestX - mPlayerX) * mMoveProgress + (mShipAnim / 2) * 0x40, aY);
			g2.SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
		Graphics g3(*g);
		g3.ClipRect(CellToPixel(mPlayerX) - 5 + (mDestX - mPlayerX) * mMoveProgress, CellToPixel(mPlayerY) - 5 + (mDestY - mPlayerY) * mMoveProgress, 0x32, 0x32);
		g3.DrawImage(anImages[IMAGE_GOODSHIPWHIRL], CellToPixel(mPlayerX) + mShipAnim * -0x32 - 5, CellToPixel(mPlayerY) - 5);
	}
	else if (!mLevelOver && !mBlink)
	{
		// the gun smoke around the ship
		int aOffset = -0xd;
		for (int i = 0; i < 3; i++, aOffset += 0xd)
		{
			int aTimer = mGunFlash[i * 3];
			if (-1 < aTimer)
			{
				Graphics g2(*g);
				g2.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g2.ClipRect(CellToPixel(mPlayerX) + 10 + aOffset, CellToPixel(mPlayerY) - 0x13 + aTimer, 0x14, 0x1c);
				g2.DrawImage(anImages[IMAGE_CANNONBLAST], CellToPixel(mPlayerX) + aOffset - 0x10e + aTimer * 0x14, CellToPixel(mPlayerY) - 0x13 + aTimer);
				g2.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
		if (!mTutorialShown || (mUpdateCnt / 0x26 % 2 == 0))
		{
			Graphics g2(*g);
			g2.Translate(0, -5);
			g2.ClipRect(CellToPixel(mPlayerX) - 5 + (mDestX - mPlayerX) * mMoveProgress,
				CellToPixel(mPlayerY) - 5 + (mDestY - mPlayerY) * mMoveProgress, 0x32, 0x32);
			if (!mVisited[mPlayerX][mPlayerY])
			{
				int aFrame = (mShipAnim / 0xe) % 8;
				g2.DrawImage(anImages[IMAGE_GOODSHIP], CellToPixel(mPlayerX) - 5 + (mDestX - mPlayerX) * mMoveProgress + mHeading * -0x32,
					CellToPixel(mPlayerY) - 5 + (mDestY - mPlayerY) * mMoveProgress + aFrame * -0x32);
			}
			g2.Translate(0, 5);
		}
		aOffset = -0xd;
		for (int i = 0; i < 3; i++, aOffset += 0xd)
		{
			int aTimer = mGunFlash[i * 3 + 1];
			if (-1 < aTimer)
			{
				Graphics g2(*g);
				g2.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g2.ClipRect(CellToPixel(mPlayerX) + 10 + aOffset, CellToPixel(mPlayerY) - 0xb + aTimer, 0x14, 0x1c);
				g2.DrawImage(anImages[IMAGE_CANNONBLAST], CellToPixel(mPlayerX) + aOffset - 0x10e + aTimer * 0x14, CellToPixel(mPlayerY) - 0xb + aTimer);
				g2.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
		aOffset = -0xd;
		for (int i = 0; i < 3; i++, aOffset += 0xd)
		{
			int aTimer = mGunFlash[i * 3 + 2];
			if (-1 < aTimer)
			{
				Graphics g2(*g);
				g2.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g2.ClipRect(CellToPixel(mPlayerX) + 10 + aOffset, CellToPixel(mPlayerY) - 1 + aTimer, 0x14, 0x1c);
				g2.DrawImage(anImages[IMAGE_CANNONBLAST], CellToPixel(mPlayerX) + aOffset - 0x10e + aTimer * 0x14, CellToPixel(mPlayerY) - 1 + aTimer);
				g2.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
	}

	// the enemy ships
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
	{
		Ship* aShip = *anItr;
		int aOffset = -0xd;
		for (int i = 0; i < 3; i++, aOffset += 0xd)
		{
			int aTimer = aShip->mTrail[i * 3];
			if (-1 < aTimer)
			{
				Graphics g2(*g);
				g2.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g2.ClipRect(CellToPixel(aShip->mX) + 10 + aOffset, CellToPixel(aShip->mY) - 0x13 + aTimer, 0x14, 0x1c);
				g2.DrawImage(anImages[IMAGE_CANNONBLAST], CellToPixel(aShip->mX) + aOffset - 0x10e + aTimer * 0x14, CellToPixel(aShip->mY) - 0x13 + aTimer);
				g2.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
		Graphics g2(*g);
		g2.Translate(0, -5);
		int aPosX = CellToPixel(aShip->mX) - 5 + (aShip->mNextX - aShip->mX) * aShip->mMoveProgress;
		int aPosY = CellToPixel(aShip->mY) - 5 + (aShip->mNextY - aShip->mY) * aShip->mMoveProgress;
		if ((aShip->mType != 2) && (aShip->mType != 4))
			g2.ClipRect(aPosX, aPosY, 0x32, 0x32);
		Image* anImage = NULL;
		int aImgX = 0;
		int aImgY = 0;
		int aFrame = (aShip->mAnim / 0xe) % 8;
		switch (aShip->mType)
		{
		case 0:
			aImgY = aPosY + aFrame * -0x32;
			aImgX = aPosX + aShip->mHeading * -0x32;
			anImage = anImages[IMAGE_PIRATE];
			break;
		case 1:
			aImgY = aPosY + aFrame * -0x32;
			aImgX = aPosX + aShip->mHeading * -0x32;
			anImage = anImages[IMAGE_REDPIRATE];
			break;
		case 2:
			g2.ClipRect(aPosX, aPosY, 0x32, 0x32);
			aImgY = aPosY + aFrame * -0x32;
			aImgX = aPosX + aShip->mHeading * -0x32;
			anImage = anImages[IMAGE_MONSTER];
			break;
		case 3:
			aImgY = aPosY + aFrame * -0x32;
			aImgX = aPosX + aShip->mHeading * -0x32;
			anImage = anImages[IMAGE_BLUEPIRATE];
			break;
		case 4:
		{
			// the ghost ship bobs and glows
			int aBob = abs((aShip->mAnim / 7) % 16) - 4;
			if (4 < aBob)
				aBob = 8 - aBob;
			g2.SetColorizeImages(true);
			g2.SetColor(Color(0xff, 0xff, 0xff, 100));
			g2.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g2.DrawImage(anImages[IMAGE_LIGHT], CellToPixel(aShip->mX) - 0xf + (aShip->mNextX - aShip->mX) * aShip->mMoveProgress,
				aBob + (aShip->mNextY - aShip->mY) * aShip->mMoveProgress - 0x13 + CellToPixel(aShip->mY));
			g2.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g2.SetColor(Color(0, 0, 0, 0x7d));
			g2.ClipRect(aPosX, aBob + aPosY, 0x32, 0x32);
			g2.DrawImage(anImages[IMAGE_PIRATE], aPosX + aShip->mHeading * -0x32,
				aBob + (aShip->mNextY - aShip->mY) * aShip->mMoveProgress + (aFrame * -0x32) - 5 + CellToPixel(aShip->mY));
			g2.SetColorizeImages(false);
			anImage = NULL;
			break;
		}
		}
		if (anImage != NULL)
			g2.DrawImage(anImage, aImgX, aImgY);
		g2.Translate(0, 5);
		int aOff = -0xd;
		for (int i = 0; i < 3; i++, aOff += 0xd)
		{
			int aTimer = aShip->mTrail[i * 3 + 1];
			if (-1 < aTimer)
			{
				Graphics g3(*g);
				g3.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g3.ClipRect(CellToPixel(aShip->mX) + 10 + aOff, CellToPixel(aShip->mY) - 0xb + aTimer, 0x14, 0x1c);
				g3.DrawImage(anImages[IMAGE_CANNONBLAST], CellToPixel(aShip->mX) + aOff - 0x10e + aTimer * 0x14, CellToPixel(aShip->mY) - 0xb + aTimer);
				g3.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
		aOff = -0xd;
		for (int i = 0; i < 3; i++, aOff += 0xd)
		{
			int aTimer = aShip->mTrail[i * 3 + 2];
			if (-1 < aTimer)
			{
				Graphics g3(*g);
				g3.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g3.ClipRect(CellToPixel(aShip->mX) + 10 + aOff, CellToPixel(aShip->mY) - 1 + aTimer, 0x14, 0x1c);
				g3.DrawImage(anImages[IMAGE_CANNONBLAST], CellToPixel(aShip->mX) + aOff - 0x10e + aTimer * 0x14, CellToPixel(aShip->mY) - 1 + aTimer);
				g3.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
	}

	// the cannonballs and their shadows
	for (std::list<Shot*>::iterator anItr = mShots.begin(); anItr != mShots.end(); ++anItr)
	{
		Shot* aShot = *anItr;
		if ((-1 < aShot->mOwner) && (aShot->mOwner < 2))
		{
			g->DrawImage(anImages[IMAGE_CANNON], CellToPixel(aShot->mX) + 0x10 + (aShot->mProgress - 0x15) * aShot->mDX,
				CellToPixel(aShot->mY) + 0x10 + ((aShot->mProgress - 0x15) * aShot->mDY - aShot->mFrame));
			g->DrawImage(anImages[IMAGE_CANNONSHADOW], CellToPixel(aShot->mX) + 0x10 + (aShot->mProgress - 0x15) * aShot->mDX,
				CellToPixel(aShot->mY) + 0x20 + (aShot->mProgress - 0x15) * aShot->mDY);
		}
	}

	// the animations without the flag (explosions, splashes, sparkles)
	for (std::list<AnimInstance*>::iterator anItr = mAnims.begin(); anItr != mAnims.end(); ++anItr)
	{
		AnimInstance* anAnim = *anItr;
		if (!anAnim->mFlag)
		{
			AnimDef* aDef = anAnim->mDef;
			const AnimFrame& aFrame = aDef->mFrames[anAnim->mFrameIdx];
			int aX = anAnim->mX;
			int aY = anAnim->mY;
			if (aDef->mFlagA == 0)
				aX = aX - aFrame.mW / 2;
			else if (aDef->mFlagA == 1)
				aX = aX + (1 - aFrame.mW);
			if (aDef->mFlagB == 0)
				aY = aY - aFrame.mH / 2;
			else if (aDef->mFlagB == 1)
				aY = aY + (1 - aFrame.mH);
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImage(aDef->mImage, aX, aY, Rect(aFrame.mX, aFrame.mY, aFrame.mW, aFrame.mH));
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
	g->SetColorizeImages(false);

	// the arrows around the ship that show what a click does
	if (!mApp->mTutorial)
	{
		if ((mState == STATE_IDLE) && !mLevelComplete)
		{
			for (int aX = 0; aX < GRID; aX++)
				for (int aY = 0; aY < GRID; aY++)
					if (mUnused_95008 < mCellFlash[aX][aY])
						DrawCellArrow(mPlayerX, mPlayerY, aX, aY, true, g);
		}
	}

	// the frame with the hint text above the board
	DrawBox(0, 0, mWidth - 4, 0x54, g);
	g->SetFont(mApp->mFonts[FONT_TUT]);
	g->SetColor(Color(0xff, 0xff, 0));
	int aStep = mTutorialStep;
	DrawHintLine(g, GT(0x004a0150 + aStep * 0x100), 0xc, 0x16);
	DrawHintLine(g, GT(0x004a1050 + aStep * 0x100), 0xc, 0x2b);
	DrawHintLine(g, GT(0x004a1f50 + aStep * 0x100), 0xc, 0x40);
	switch (mTutorialStep)
	{
	case 0:
	case 3:
	case 0xb:
	case 0xc:
	case 0xd:
		g->DrawImage(anImages[IMAGE_NEXT], CellToPixel(8) + 2, CellToPixel(2) + 4);
	}
	if (mState == STATE_IDLE)
	{
		switch (mTutorialStep)
		{
		case 1:
		case 9:
			DrawCellArrow(mPlayerX, mPlayerY, mPlayerX, mPlayerY + 1, true, g);
			break;
		case 2:
		case 10:
			DrawCellArrow(mPlayerX, mPlayerY, mPlayerX, mPlayerY, true, g);
			break;
		case 4:
			DrawCellArrow(mPlayerX, mPlayerY, mPlayerX + 1, mPlayerY, true, g);
			break;
		case 5:
		case 6:
		case 7:
		case 8:
			DrawCellArrow(mPlayerX, mPlayerY, mPlayerX + 1, mPlayerY + 1, true, g);
		}
	}

	// the points that rise from the cells
	for (std::list<ScoreText*>::iterator anItr = mScoreTexts.begin(); anItr != mScoreTexts.end(); ++anItr)
	{
		ScoreText* aText = *anItr;
		if (0 < aText->mAlpha)
		{
			char aBuf[64];
			sprintf(aBuf, "%d", aText->mValue);
			g->SetColorizeImages(true);
			int anAlpha = aText->mAlpha;
			if (0xfe < anAlpha)
				anAlpha = 0xff;
			g->SetColor(Color(0xff, 0xff, 0, anAlpha));
			g->SetFont(mApp->mFonts[FONT_MT22]);
			g->DrawString(aBuf, aText->mX + 3, aText->mY);
			g->SetColorizeImages(false);
		}
	}

	// the tutorial's first box
	if (mTutorialShown)
	{
		DrawBox(CellToPixel(3) - 4, CellToPixel(2) - 5, CellToPixel(5) + 0xc, CellToPixel(2) + 10, g);
		g->SetFont(mApp->mFonts[FONT_TUT]);
		g->SetColor(Color(0xff, 0xff, 0));
		WriteCenteredLine(g, CellToPixel(2), GT(0x004a2f30));
		WriteCenteredLine(g, CellToPixel(2) + 0x12, GT(0x004a2f14));
		WriteCenteredLine(g, CellToPixel(2) + 0x23, GT(0x004a2ef8));
		WriteCenteredLine(g, CellToPixel(2) + 0x34, GT(0x004a2ee4));
		int aCnt = mUpdateCnt;
		if (aCnt < 0x262)
		{
			// the arrows tour the ship's neighbours, one after the other
			static const int kTour[8][2] = {{4, 5}, {4, 4}, {5, 4}, {6, 4}, {6, 5}, {6, 6}, {5, 6}, {4, 6}};
			int aSlot = -1;
			if (aCnt >= 0x58)
				aSlot = (aCnt - 0x58) / 0x57;
			if (aSlot > 7)
				aSlot = 7;
			if (aSlot >= 0)
				DrawCellArrow(5, 5, kTour[aSlot][0], kTour[aSlot][1], false, g);
		}
	}

	// the message after an undo that cannot be done
	if (0 < mHintTimer)
	{
		g->SetFont(mApp->mFonts[FONT_MEDIUM]);
		int anAlpha = mHintTimer;
		if (0xff < anAlpha)
			anAlpha = 0xff;
		g->SetColor(Color(0xff, 0xff, 0, anAlpha));
		// "Cannot Undo:" and what stops it (the list of the last move was cleared by that)
		static const unsigned int kReasons[4] = {0x004a2ec0, 0x004a2eac, 0x004a2ea0, 0x004a2e8c};
		WriteCenteredLine(g, mHeight - 0x41, GT(0x004a2ed4));
		if ((mHintReason >= 0) && (mHintReason < 4))
			WriteCenteredLine(g, mHeight - 0x2d, GT(kReasons[mHintReason]));
	}

	g->Translate(0, -0xc);
	if (0 < mLifeFlash)
	{
		if (mLifeFlash < 0x15)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, (int) ((double) mLifeFlash / 20.0 * 255.0)));
		}
		g->DrawImage(anImages[IMAGE_EXTRALIFE], mWidth / 2 - 0x3f, mHeight / 2 - 0x49);
		g->SetFont(mApp->mFonts[FONT_FONT1]);
		g->SetColor(Color(0xff, 0xff, 0));
		WriteCenteredLine(g, mHeight / 2 - 0x49 + 0x60, GT(0x004a2e80));
		g->SetColorizeImages(false);
	}
	if (mState == STATE_LEVEL_END)
		g->DrawImage(anImages[IMAGE_GAMEOVER], mWidth / 2 - 0x84, mHeight / 2 - 0x4b);
}

// the bonus screen (digging for the treasure; 9 x 9 squares)
void Board::DrawBonus(Graphics* g)
{
	Image** anImages = mApp->mImages;
	g->DrawImage(anImages[IMAGE_BONUSBACK], CellToPixel(0), CellToPixel(0));
	for (int aCol = 0; aCol < 9; aCol++)
	{
		for (int aRow = 1; aRow <= 9; aRow++)
		{
			int aValue = mBonusGrid[aCol][aRow - 1];
			if (aValue == 100)
			{
				g->DrawImage(anImages[IMAGE_GRAVE], CellToPixel(aCol + 1) - 5, CellToPixel(aRow) - 5);
			}
			else if (aValue == 99)
			{
				Graphics g2(*g);
				g2.ClipRect(CellToPixel(aCol + 1), CellToPixel(aRow), 0x1e, 0x1e);
				int aY = CellToPixel(aRow);
				int aX = CellToPixel(aCol + 1);
				if (mUpdateCnt - mBonusTick >= 0x32)
					aX = aX;
				else
					aX = aX - 0x1e;
				g2.DrawImage(anImages[IMAGE_BONUSCHEST], aX, aY);
			}
			else if ((aValue == -2) && (mBonusState == 2))
			{
				Graphics g2(*g);
				g2.SetColorizeImages(true);
				int anAlpha = (int) ((double) (mUpdateCnt - mBonusTick2) * 255.0 / 150.0);
				if (0xfe < anAlpha)
					anAlpha = 0xff;
				g2.SetColor(Color(0xff, 0xff, 0xff, anAlpha));
				g2.ClipRect(CellToPixel(aCol + 1), CellToPixel(aRow), 0x1e, 0x1e);
				g2.DrawImage(anImages[IMAGE_BONUSCHEST], CellToPixel(aCol + 1) - 0x1e, CellToPixel(aRow));
				g2.SetColorizeImages(false);
			}
			else if (-1 < aValue)
			{
				g->DrawImage(anImages[IMAGE_BONUSX], CellToPixel(aCol + 1) - 5, CellToPixel(aRow) - 7);
				if (0 < aValue)
				{
					g->SetFont(mApp->mFonts[FONT_FONT1]);
					g->SetColor(Color(0xff, 0, 0));
					char aBuf[32];
					sprintf(aBuf, "%d", aValue);
					SexyString aText = aBuf;
					g->DrawString(aText, (0x10 - g->GetFont()->StringWidth(aText) / 2) + CellToPixel(aCol + 1), CellToPixel(aRow) + 0x19);
				}
			}
		}
	}
	g->SetFont(mApp->mFonts[FONT_FONT1]);
	g->SetColor(Color(0xff, 0xff, 0));
	if (mBonusState == 1)
	{
		WriteCenteredLine(g, mHeight - 0x32, GT(0x004a2f70));
		WriteCenteredLine(g, mHeight - 0x1e, GT(0x004a2f50));
	}
	else if (mBonusState == 2)
	{
		WriteCenteredLine(g, mHeight - 0x32, GT(0x004a2f64));
		WriteCenteredLine(g, mHeight - 0x1e, GT(0x004a2f50));
	}
	else if (mShovelX == -1)
	{
		WriteCenteredLine(g, mHeight - 0x32, GT(0x004a3070));
		WriteCenteredLine(g, mHeight - 0x1e, GT(0x004a305c));
	}
	else
	{
		int aDistY = abs(mShovelY - mTreasureY);
		int aDistX = abs(mShovelX - mTreasureX);
		int aDist = aDistX + aDistY;
		if (aDist < 10)
		{
			if (aDist < 3)
			{
				WriteCenteredLine(g, mHeight - 0x32, GT(0x004a3028));
				WriteCenteredLine(g, mHeight - 0x1e, GT(0x004a3014));
			}
			else
			{
				int aDX = mTreasureX - mShovelX;
				int aDY = mTreasureY - mShovelY;
				WriteCenteredLine(g, mHeight - 0x32, GT(0x004a3004));
				unsigned int aText;
				if (abs(aDX) < abs(aDY))
					aText = (aDY < 0) ? 0x004a2fdc : 0x004a2fcc;      // to the north / to the south
				else if (abs(aDY) < abs(aDX))
					aText = (aDX < 0) ? 0x004a2ff8 : 0x004a2fec;      // to the west / to the east
				else if (aDX < 0)
					aText = (aDY < 0) ? 0x004a2fb8 : 0x004a2f90;      // northwest / southwest
				else
					aText = (aDY < 0) ? 0x004a2fa4 : 0x004a2f7c;      // northeast / southeast
				WriteCenteredLine(g, mHeight - 0x1e, GT(aText));
			}
		}
		else
		{
			WriteCenteredLine(g, mHeight - 0x32, GT(0x004a304c));
			WriteCenteredLine(g, mHeight - 0x1e, GT(0x004a303c));
		}
	}

	for (std::list<AnimInstance*>::iterator anItr = mAnims.begin(); anItr != mAnims.end(); ++anItr)
	{
		AnimInstance* anAnim = *anItr;
		if (!anAnim->mFlag)
		{
			AnimDef* aDef = anAnim->mDef;
			const AnimFrame& aFrame = aDef->mFrames[anAnim->mFrameIdx];
			int aX = anAnim->mX - aFrame.mW / 2;
			int aY = anAnim->mY - aFrame.mH / 2;
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImage(aDef->mImage, aX, aY, Rect(aFrame.mX, aFrame.mY, aFrame.mW, aFrame.mH));
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
	for (std::list<ScoreText*>::iterator anItr = mScoreTexts.begin(); anItr != mScoreTexts.end(); ++anItr)
	{
		ScoreText* aText = *anItr;
		if (0 < aText->mAlpha)
		{
			char aBuf[64];
			sprintf(aBuf, "%d", aText->mValue);
			g->SetColorizeImages(true);
			int anAlpha = aText->mAlpha;
			if (0xfe < anAlpha)
				anAlpha = 0xff;
			g->SetColor(Color(0xff, 0xff, 0, anAlpha));
			g->SetFont(mApp->mFonts[FONT_MT22]);
			g->DrawString(aBuf, aText->mX + 3, aText->mY);
			g->SetColorizeImages(false);
		}
	}
}

void Board::DrawHintLine(Graphics* g, const SexyString& theText, int theX, int theY)
{
	g->DrawString(theText, theX, theY);
}

// 0x40a230: how far (in cells, at the most three) the broadside of a ship at (x, y) with a heading reaches a
// cell next to it; 0 when the cell is not on the broadside
int Board::FireRange(int theX, int theY, int theHeading, int theTX, int theTY)
{
	int aDX = theTX - theX;
	int aDY = theTY - theY;
	int aResult = 0;
	if ((theHeading == 0) || (theHeading == 8))
	{
		if (aDX == -1)
		{
			if (aDY != 0)
				return 0;
		}
		else
		{
			if (aDX != 1)
				return 0;
			if (aDY != 0)
				return 0;
		}
	}
	else if ((theHeading == 2) || (theHeading == 10))
	{
		if (aDX == -1)
		{
			if (aDY != -1)
				return 0;
		}
		else
		{
			if (aDX != 1)
				return 0;
			if (aDY != 1)
				return 0;
		}
	}
	else if ((theHeading == 4) || (theHeading == 0xc))
	{
		if (aDX != 0)
			return 0;
		if ((aDY != -1) && (aDY != 1))
			return 0;
	}
	else if ((theHeading == 6) || (theHeading == 0xe))
	{
		if (aDX == -1)
		{
			if (aDY != 1)
				return 0;
		}
		else
		{
			if (aDX != 1)
				return 0;
			if (aDY != -1)
				return 0;
		}
	}
	int aCellX = aDX + theX;
	int aStep = 1;
	int aRow = aCellX * 0xb;
	int aY = theY;
	do
	{
		aY = aDY + aY;
		if (((aResult == aStep - 1) && (-1 < aCellX)) && (aCellX < 0xb) && (-1 < aY) && (aY < 0xb) &&
			(mTerrain[aCellX][aY] == 0) && (ShipAt(aCellX - aDX, aY - aDY, NULL) == NULL))
			aResult = aStep;
		aStep = aStep + 1;
		aRow = aRow + aDX * 0xb;
		aCellX = aCellX + aDX;
	}
	while (aStep < 4);
	(void) aRow;
	return aResult;
}
