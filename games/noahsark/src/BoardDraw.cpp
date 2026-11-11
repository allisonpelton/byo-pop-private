// The Board, part 5 of 6: drawing (WinNoah.exe 0x4092a0, Board::Draw, and its helpers 0x408810, 0x408ce0, 0x409180,
// 0x40bfa0, 0x40c180, 0x40c460, 0x40c8a0, 0x407fa0).

#include "Board.h"
#include "NoahApp.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

namespace
{

// Text with a black outline one pixel around, then the text itself in white or the given colour
void DrawOutlined(Graphics* g, Font* theFont, const SexyString& theText, int theX, int theY, const Color& theColor,
	const Color& theOutline = Color(0, 0, 0))
{
	g->SetFont(theFont);
	g->SetColor(theOutline);
	for (int aOffsetX = -1; aOffsetX < 2; aOffsetX++)
		for (int aOffsetY = -1; aOffsetY < 2; aOffsetY++)
			g->DrawString(theText, theX + aOffsetX, theY + aOffsetY);
	g->SetColor(theColor);
	g->DrawString(theText, theX, theY);
}

Color Lighter(const Color& theColor)
{
	return Color(std::min(255, theColor.mRed + 0x1e), std::min(255, theColor.mGreen + 0x1e), std::min(255, theColor.mBlue + 0x1e));
}

}

// 0x408ce0: the fish (a strip of 20 swimming frames of 80x80, the turn on the third row)
void Board::DrawFish(Graphics* g)
{
	for (size_t i = 0; i < mFish.size(); i++)
	{
		Fish* aFish = mFish[i];
		Graphics aClip(*g);
		aClip.ClipRect(aFish->mX, aFish->mY, 0x3c, 0x35);
		int aX, aY;
		if (!aFish->mTurning)
		{
			aY = (aFish->mDX < 0) ? aFish->mY : aFish->mY - 0x50;
			aX = aFish->mX - (aFish->mFrame % 0x14) * 0x50;
		}
		else
		{
			aY = aFish->mY - 0xa0;
			aX = aFish->mX - aFish->mTurnFrame * 0x50;
		}
		aClip.DrawImage(mApp->mImages[IMAGE_FISH], aX, aY);
	}
}

// 0x409180
void Board::DrawBubbles(Graphics* g)
{
	for (size_t i = 0; i < mBubbles.size(); i++)
	{
		Bubble* aBubble = mBubbles[i];
		Graphics aClip(*g);
		aClip.ClipRect((int) aBubble->mX, (int) aBubble->mY, 0xf, 0xf);
		int aOffset = (aBubble->mY <= (double) (mWaterY + 0x14)) ? 0 : -0xf;
		aClip.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		aClip.DrawImage(mApp->mImages[IMAGE_BUBBLES], (int) aBubble->mX + aOffset, (int) aBubble->mY);
		aClip.SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x408810
void Board::DrawSparkles(Graphics* g)
{
	for (size_t i = 0; i < mSparkles.size(); i++)
	{
		Sparkle* aSparkle = mSparkles[i];
		int aY = (aSparkle->mHeight + aSparkle->mY) - aSparkle->mFrame;
		Graphics aClip(*g);
		aClip.SetColor(Color(0xff, 0xff, 0xff, std::max(0, std::min(0xff, aSparkle->mLife))));
		aClip.SetColorizeImages(true);
		aClip.ClipRect(aSparkle->mX, aY, 0xf, 0xf);
		int aFrame = abs((aSparkle->mFrame / 5) % 4);
		aClip.DrawImage(mApp->mImages[IMAGE_HEARTS], aSparkle->mX, aY - aFrame * 0xf);
		aClip.SetColorizeImages(false);
	}
}

// 0x407fa0: the floating scores, in the colour of the animal with a dark outline, and a star per combo step
void Board::DrawTexts(Graphics* g)
{
	for (size_t i = 0; i < mTexts.size(); i++)
	{
		FloatingText* aFloating = mTexts[i];
		g->SetFont(mApp->mFonts[FONT_COLORLESS]);
		int aAlpha = std::min(aFloating->mAlpha, 0xff);
		SexyString aText = aFloating->mText;
		int aWidth = g->GetFont()->StringWidth(aText);
		int aX = (0x1e - aWidth) / 2 + aFloating->mX;
		g->SetColor(Color(0xdc, 0xdc, 0xdc, aAlpha));
		for (int aOffsetX = -1; aOffsetX < 2; aOffsetX++)
			for (int aOffsetY = -1; aOffsetY < 2; aOffsetY++)
				g->DrawString(aText, aX + aOffsetX, aFloating->mY + aOffsetY);
		g->SetColor(Color(aFloating->mColor.mRed, aFloating->mColor.mGreen, aFloating->mColor.mBlue, aAlpha));
		g->DrawString(aText, aX, aFloating->mY);

		g->SetColorizeImages(true);
		g->SetColor(Color(aFloating->mColor.mRed, aFloating->mColor.mGreen, aFloating->mColor.mBlue, aAlpha));
		int aStarX = aFloating->mX + 7;
		int aStarY = aFloating->mY;
		int aStars = aFloating->mStars;
		int aMode = mApp->mGameMode;
		if ((aMode != GAMEMODE_STAMPEDE) && (aMode != GAMEMODE_PUZZLE))
		{
			Image* aStar = mApp->mImages[IMAGE_STAR];
			while (aStars > 0)
			{
				if (aStars >= 4)
				{
					g->DrawImage(aStar, aFloating->mX - 0xb, aStarY);
					g->DrawImage(aStar, aStarX + 1 - 7 + 7, aStarY + 8);
					g->DrawImage(aStar, aStarX + 0xd - 7 + 7, aStarY + 8);
					g->DrawImage(aStar, aStarX + 0x19 - 7 + 7, aStarY + 8);
					aStars -= 4;
					aStarY += 8;
				}
				else if (aStars == 3)
				{
					g->DrawImage(aStar, aStarX - 0xc, aStarY);
					g->DrawImage(aStar, aStarX, aStarY);
					g->DrawImage(aStar, aStarX + 0xc, aStarY);
					aStars = 0;
				}
				else if (aStars == 2)
				{
					g->DrawImage(aStar, aFloating->mX + 1, aStarY);
					g->DrawImage(aStar, aFloating->mX + 0xd, aStarY);
					aStars = 0;
				}
				else
				{
					g->DrawImage(aStar, aFloating->mX + 7, aStarY);
					aStars = 0;
				}
			}
		}
		g->SetColorizeImages(false);
	}
}

// 0x40bfa0: the level number beside "level" under the board's frame
void Board::DrawLevelNumber(Graphics* g)
{
	int aBaseX = CellLeft(0);
	int aWidth = 0x38;
	int aLevel = mApp->mLevel;
	for (int aRest = aLevel; aRest > 9; aRest /= 10)
		aWidth += 10;
	int aX = ((aBaseX - 10) - aWidth) / 2 - 10 + aWidth;
	g->SetColorizeImages(true);
	g->SetColor(Color(0, 0, 0));
	for (; aLevel > 9; aLevel /= 10)
	{
		Graphics aClip(*g);
		aClip.ClipRect(aX, 0x135, 0xc, 0x12);
		aClip.DrawImage(mApp->mImages[IMAGE_LEVELNUMBERS], aX, (aLevel % 10) * -0x12 + 0x134);
		aX -= 10;
	}
	Graphics aClip(*g);
	aClip.ClipRect(aX, 0x135, 0xc, 0x12);
	aClip.DrawImage(mApp->mImages[IMAGE_LEVELNUMBERS], aX, (aLevel % 10) * -0x12 + 0x134);
	g->DrawImage(mApp->mImages[IMAGE_LEVELTEXT], aX - 0x30, 0x135);
	g->SetColorizeImages(false);
}

// 0x40c180: the score in the left panel, with thousands separated by commas
void Board::DrawScore(Graphics* g)
{
	int aScore = mApp->mShownScore;
	char aBuffer[256];
	char aPart[256];
	int aLow = aScore % 1000;
	aScore /= 1000;
	const char* aFormat = "%d";
	if ((aScore < 1) || (aLow > 99))
		aFormat = "%d";
	else if (aLow < 10)
		aFormat = "00%d";
	else
		aFormat = "0%d";
	snprintf(aBuffer, sizeof(aBuffer), aFormat, aLow);
	while (aScore > 0)
	{
		aLow = aScore % 1000;
		snprintf(aPart, sizeof(aPart), "%s", aBuffer);
		aScore /= 1000;
		if ((aScore < 1) || (aLow > 99))
			aFormat = "%d,%s";
		else if (aLow < 10)
			aFormat = "00%d,%s";
		else
			aFormat = "0%d,%s";
		snprintf(aBuffer, sizeof(aBuffer), aFormat, aLow, aPart);
	}
	g->SetFont(mApp->mFonts[FONT_SCORE]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	SexyString aText = aBuffer;
	int aWidth = g->GetFont()->StringWidth(aText);
	g->DrawString(aText, (CellLeft(0) - 10 - aWidth) / 2, 0x58);
	g->DrawImage(mApp->mImages[IMAGE_POINTSTEXT], CellLeft(0) - 0x28, 0x4e);
}

// The trail: straight pieces between two neighbours, bends with a tinted overlay, an arrow at the end.
// theNodes[theFrom .. theTo-1] are the cells (0x40c8a0 and 0x40c460 draw this from the preview and the pairs).
void Board::DrawTrailNodes(Graphics* g, const std::vector<PathNode*>& theNodes, int theFrom, int theTo, const Color& theTint)
{
	NoahApp* anApp = mApp;
	for (int aPass = 0; aPass < 2; aPass++)
	{
		g->SetColor((aPass == 0) ? Color(0, 0, 0) : theTint);
		for (int i = theFrom; i < theTo; i++)
		{
			if ((i < 1) || (i >= (int) theNodes.size() - 1))
				continue;
			PathNode* aPrev = theNodes[i - 1];
			PathNode* aCur = theNodes[i];
			PathNode* aNext = theNodes[i + 1];
			int aX = CellLeft(aCur->mCellX);
			int aY = CellTop(aCur->mCellY);
			if (aPrev->mCellX == aNext->mCellX)
				g->DrawImage(anApp->mImages[(aNext->mCellY < aPrev->mCellY) ? IMAGE_TRAILUD : IMAGE_TRAILDU], aX, aY);
			else if (aPrev->mCellY == aNext->mCellY)
				g->DrawImage(anApp->mImages[(aNext->mCellX < aPrev->mCellX) ? IMAGE_TRAILRL : IMAGE_TRAILLR], aX, aY);
			else
			{
				int aDX = aNext->mCellX + aPrev->mCellX - 2 * aCur->mCellX;
				int aDY = aNext->mCellY + aPrev->mCellY - 2 * aCur->mCellY;
				int aBase, aOver;
				if ((aDX < 0) && (aDY < 0))
				{
					aBase = IMAGE_TRAILUL;
					aOver = IMAGE_TRAILUL_OVER;
				}
				else if ((aDX > 0) && (aDY < 0))
				{
					aBase = IMAGE_TRAILRU;
					aOver = IMAGE_TRAILRU_OVER;
				}
				else if ((aDX < 0) && (aDY > 0))
				{
					aBase = IMAGE_TRAILLD;
					aOver = IMAGE_TRAILLD_OVER;
				}
				else
				{
					aBase = IMAGE_TRAILDR;
					aOver = IMAGE_TRAILDR_OVER;
				}
				g->DrawImage(anApp->mImages[aBase], aX, aY);
				g->SetColorizeImages(true);
				g->DrawImage(anApp->mImages[aOver], aX, aY);
				g->SetColorizeImages(false);
			}
		}
	}
}

// part of 0x40c8a0 and 0x40c460: the arrow on a trail's last cell, pointing the way of the last step
void Board::DrawTrailArrow(Graphics* g, PathNode* theLast, PathNode* thePrev, const Color& theTint)
{
	int aX = CellLeft(theLast->mCellX);
	int aY = CellTop(theLast->mCellY);
	g->SetColor(theTint);
	int aImage;
	if (theLast->mCellX < thePrev->mCellX)
	{
		aX += 1;
		aImage = IMAGE_TRAILARROWLEFT;
	}
	else if (thePrev->mCellX < theLast->mCellX)
	{
		aX -= 1;
		aImage = IMAGE_TRAILARROWRIGHT;
	}
	else if (theLast->mCellY < thePrev->mCellY)
	{
		aY += 1;
		aImage = IMAGE_TRAILARROWUP;
	}
	else
	{
		aY -= 1;
		aImage = IMAGE_TRAILARROWDOWN;
	}
	g->DrawImage(mApp->mImages[aImage], aX, aY);
}

// 0x40c8a0: the path preview while an animal is picked, or the path of a pair being dragged
void Board::DrawPreview(Graphics* g, bool theSecondPass)
{
	int aMode = mApp->mGameMode;
	if ((aMode == GAMEMODE_PUZZLE) || (aMode == GAMEMODE_STAMPEDE))
		return;
	g->SetColorizeImages(false);
	if (mLink.size() < 2)
	{
		if (!theSecondPass)
		{
			if (mTrail.size() >= 2)
			{
				Color aTint(0, 0, 0);
				if (mSelected != NULL)
					aTint = Lighter(AnimalColor(mSelected->mType));
				DrawTrailArrow(g, mTrail.back(), mTrail[mTrail.size() - 2], aTint);
				if (mTrail.size() > 2)
					DrawTrailNodes(g, mTrail, 1, (int) mTrail.size() - 1, aTint);
			}
		}
	}
	else if (!theSecondPass)
	{
		for (size_t i = 0; i < mLink.size(); i++)
		{
			Piece* aPiece = mLink[i];
			if ((aPiece != NULL) && (aPiece->mPath.size() >= 2) && aPiece->mFacingRight)
			{
				Color aTint = Lighter(AnimalColor(aPiece->mType));
				DrawTrailArrow(g, aPiece->mPath.back(), aPiece->mPath[aPiece->mPath.size() - 2], aTint);
				if (aPiece->mPath.size() > 2)
					DrawTrailNodes(g, aPiece->mPath, 1, (int) aPiece->mPath.size() - 1, aTint);
			}
		}
	}
}

// 0x40c460: the part of a walking pair's trail that is still ahead of its first animal
void Board::DrawWalkingTrails(Graphics* g)
{
	g->SetColorizeImages(false);
	for (int x = 0; x < GRID_SIZE; x++)
	{
		for (int y = 0; y < GRID_SIZE; y++)
		{
			Piece* aPiece = mGrid[x][y];
			if (!aPiece->mMoving || !aPiece->mFacingRight || aPiece->mPath.size() < 3)
				continue;
			Color aTint = Lighter(AnimalColor(aPiece->mType));
			int aTo = (int) aPiece->mPath.size();
			if (aPiece->mPartner != NULL)
				aTo -= aPiece->mPartner->mPathIndex;
			int aFrom = aPiece->mPathIndex;
			if (aFrom < 1)
				aFrom = 1;
			DrawTrailNodes(g, aPiece->mPath, aFrom, std::min(aTo, (int) aPiece->mPath.size() - 1), aTint);
		}
	}
}

// part of 0x4092a0: the outlined caption in the middle of the board ("Get Ready!", "No More Moves!", "PAUSED")
void Board::DrawBanner(Graphics* g, const SexyString& theText)
{
	g->SetFont(mApp->mFonts[FONT_BIG]);
	int aWidth = g->GetFont()->StringWidth(theText);
	int aX = (0x1d6 - aWidth) / 2 - 10 + CellLeft(0);
	DrawOutlined(g, mApp->mFonts[FONT_BIG], theText, aX, 0xff, Color(0xff, 0xff, 0xff));
}

// 0x4092a0
void Board::Draw(Graphics* g)
{
	NoahApp* anApp = mApp;
	ImageTable& anImages = anApp->mImages;

	// the sky (rain, or the lightning flash)
	Image* aSky = mLightning ? anImages[IMAGE_LIGHTNING] : anImages[IMAGE_RAIN1 + (mBackdropTick & 1)];
	g->DrawImage(aSky, 0, 0);

	// the flood: the waves in a colour that follows the danger, then the body of water
	int aSurface = mWaterY - 0xf;
	bool aDanger = (mWaterY < mArkLevelY + 0x14) && ((mState == BOARD_PLAYING) || (mState == BOARD_SWAP) ||
		(mState == BOARD_SWAP_BACK) || (mState == BOARD_SETTLE));
	Color aWave;
	if (mStormTimer < 1)
		aWave = aDanger ? Color(std::max(0, mDangerPulse - 0x48), 0, 0) : Color(0, 0, 0x80);
	else
		aWave = Color(0x82, 0x82, 0xcd);
	g->SetColor(aWave);
	g->SetColorizeImages(true);
	for (int x = 0; x < mWidth; x += 300)
	{
		Graphics aClip(*g);
		aClip.ClipRect(x, aSurface, 300, 0x1e);
		aClip.DrawImage(anImages[IMAGE_WAVES], x + (((x / 300) + mBackWavePhase) % 0x1e) * -300, aSurface);
	}
	g->SetColor(aDanger ? Color(std::max(0, mDangerPulse - 0x48), 0, 0) : Color(0, 0, 0x80));
	g->FillRect(0, aSurface + 0x1e, mWidth, mHeight);
	g->SetColorizeImages(false);

	g->DrawImage(anImages[IMAGE_MOUNTAIN], 0, 0);

	// the ark, and Noah
	g->DrawImage(anImages[IMAGE_ARKRAMP_1 + std::min(mArkStage, 4)], mArkX, mArkY);
	{
		int aNoah = -1, aNX = 0, aNY = 0;
		if (!mNoahWorried)
		{
			if (!mArkLeaving && (mState != BOARD_WON))
			{
				aNoah = mNoahSprite + mNoahFrame;
				aNX = mArkX - 1;
				aNY = mArkY + 0x65;
			}
			else if (mArkStage == 0)
			{
				aNoah = IMAGE_NOAH_ASCEND10;
				aNX = mArkX + 9;
				aNY = mArkY + 0x4e;
			}
		}
		else
		{
			// worried Noah climbs the ramp; once the level is won or the ark is leaving, only the last climbing frame
			// shows, and only while the ark has not started to rise
			if (mArkLeaving || (mState == BOARD_WON))
			{
				if (mArkStage == 0)
				{
					aNoah = IMAGE_NOAH_ASCEND10;
					aNX = mArkX + 9;
					aNY = mArkY + 0x4e;
				}
			}
			else
			{
				aNoah = std::min<int>(mNoahFrame + IMAGE_NOAH_ASCEND1, IMAGE_NOAH_ASCEND10);
				aNX = mArkX + 9;
				aNY = mArkY + 0x4e;
			}
		}
		if (aNoah >= 0)
			g->DrawImage(anImages[aNoah], aNX, aNY);
	}
	{
		Graphics aClip(*g);
		aClip.ClipRect(mArkX + 0x75, mArkY, 0x22, 0xf);
		aClip.DrawImage(anImages[IMAGE_FLAG], mArkX + 0x75, mArkY - (mAnimTick % 8) * 0xf);
	}

	// the foreground waves, ten layers one pixel lower and five to the left, brighter each time
	for (int i = 0; i < 10; i++)
	{
		int aShift = i * 5;
		Color aColor;
		if (mStormTimer < 1)
		{
			if ((mWaterY < mArkLevelY + 0x14) && ((mState == BOARD_PLAYING) || (mState == BOARD_SWAP) ||
				(mState == BOARD_SWAP_BACK) || (mState == BOARD_SETTLE)))
				aColor = Color(std::min(0xff, mDangerPulse + aShift), aShift, aShift);   // 0x4092a0: the pulse itself, only the back wave takes 0x48 off
			else
				aColor = Color(std::min(0xff, 0x2c + aShift), std::min(0xff, 0x45 + aShift), 0xb3);
		}
		else
			aColor = Color(std::min(0xff, 0xb4 + aShift), std::min(0xff, 0xb4 + aShift), 0xff);
		g->SetColor(aColor);
		g->SetColorizeImages(true);
		for (int x = 0; x < mWidth; x += 300)
		{
			Graphics aClip(*g);
			aClip.ClipRect(x - aShift, mWaterY + i, 300, 0x1e);
			aClip.DrawImage(anImages[IMAGE_WAVES], (x + (((x / 300) + mFrontWavePhase) % 0x1e) * -300) - aShift, mWaterY + i);
		}
		g->SetColorizeImages(false);
	}
	if (mStormTimer < 1)
	{
		if ((mWaterY < mArkLevelY + 0x14) && ((mState == BOARD_PLAYING) || (mState == BOARD_SWAP) ||
			(mState == BOARD_SWAP_BACK) || (mState == BOARD_SETTLE)))
			g->SetColor(Color(std::min(0xff, mDangerPulse + 0x2d), 0x2d, 0x2d));
		else
			g->SetColor(Color(0x54, 0x6d, 0xb3));
		g->FillRect(0, mWaterY + 0x27, mWidth, (mHeight - mWaterY) - 0x1e);
	}
	else
	{
		g->SetColor(Color(0xe1, 0xe1, 0xff));
		g->FillRect(0, mWaterY + 0x1e, mWidth, 0xaf);
	}

	DrawFish(g);
	DrawBubbles(g);
	g->DrawImage(anImages[IMAGE_NOAHSARKTEXT], 3, 7);
	g->DrawImage(anImages[IMAGE_FRAME], CellLeft(0) - 10, CellTop(0) - 10);
	DrawPreview(g, false);

	if (!mPaused && anApp->mHasFocus)
	{
		// the animals and obstacles that stand still (the walking, leaving and abandoned are drawn later)
		for (int x = 0; x < GRID_SIZE; x++)
		{
			for (int y = 0; y < GRID_SIZE; y++)
			{
				Piece* aPiece = mGrid[x][y];
				if (aPiece == NULL)
					continue;
				int aX = CellLeft(x) + aPiece->mOffsetX;
				int aY = CellTop(y) + aPiece->mOffsetY;
				int aType = aPiece->mType;
				bool aQuiet = !aPiece->mRemoved && !aPiece->mFading && !aPiece->mMoving;
				if ((aType == PIECE_CRATE) || (aType == PIECE_BLOCK))
				{
					if ((aPiece->mAlpha < 0xff) && (mState == BOARD_LEVEL_DONE))
					{
						aX += (int) (sin(y * 0.5 + mUpdateCnt * 0.06666666666666667) * (64.0f - aPiece->mAlpha * 0.25f));   // 0x409d1e: the sway grows as the piece fades
						aY += 14;
					}
					g->DrawImage(anImages[(aType == PIECE_CRATE) ? IMAGE_CRATE : IMAGE_BLOCK], aX, aY);
					continue;
				}
				if ((aType == PIECE_EMPTY) || !aQuiet)
					continue;
				if ((aPiece->mAlpha < 0xff) && (mState == BOARD_LEVEL_DONE))
				{
					aX += (int) (sin(y * 0.5 + mUpdateCnt * 0.06666666666666667) * (64.0f - aPiece->mAlpha * 0.25f));   // 0x409d1e: the sway grows as the piece fades
					aY += 14;
				}
				Graphics aClip(*g);
				aClip.ClipRect(aX, aY, CELL_SIZE, CELL_SIZE);
				if (aPiece->mAlpha < 0xff)
				{
					aClip.SetColorizeImages(true);
					aClip.SetColor(Color(0xff, 0xff, 0xff, std::max(0, aPiece->mAlpha)));
				}
				else if (aPiece->mFlash)
				{
					aClip.SetColorizeImages(true);
					aClip.SetColor(Color(mFlashPulse, mFlashPulse, mFlashPulse));
				}
				bool aHover = (mSelected == NULL) && (CellAtX(mMouseX) == x) && (CellAtY(mMouseY) == y) &&
					(anApp->mGameMode != GAMEMODE_STAMPEDE) && (x >= 0) && (y >= 0);
				bool aInLink = std::find(mLink.begin(), mLink.end(), aPiece) != mLink.end();
				int aRow;
				if ((!aPiece->mSelected || (mState != BOARD_GET_READY)) && !aPiece->mFlash)
				{
					if (aHover)
						aRow = 4;
					else if ((aPiece == mSelected) || aPiece->mSelected || aInLink)
						aRow = 6;
					else if (anApp->mNextLevelWidget == NULL)
						aRow = aPiece->mBlink;
					else
						aRow = 0;
				}
				else
					aRow = 5;
				aClip.DrawImage(anImages[IMAGE_ANIMALS], aX - aType * CELL_SIZE, aY - aRow * CELL_SIZE);
				aClip.SetColorizeImages(false);
			}
		}
		// the Stampede's waiting row
		if ((anApp->mGameMode == GAMEMODE_STAMPEDE) && (mState == BOARD_PLAYING))
		{
			for (int x = 0; x < GRID_SIZE; x++)
			{
				int aX = CellLeft(x);
				int aY = CellTop(GRID_SIZE) + mNextRow[x]->mOffsetY;
				Graphics aClip(*g);
				aClip.ClipRect(aX, aY, CELL_SIZE, CELL_SIZE);
				aClip.DrawImage(anImages[IMAGE_ANIMALS], aX - mNextRow[x]->mType * CELL_SIZE, aY - CELL_SIZE * 2 + CELL_SIZE * 2 - CELL_SIZE * 0);
			}
		}
	}
	else
	{
		DrawBanner(g, GT(0x00494340));
	}

	DrawWalkingTrails(g);

	// the animals that walk, are taken in or are abandoned
	for (int x = 0; x < GRID_SIZE; x++)
	{
		for (int y = 0; y < GRID_SIZE; y++)
		{
			Piece* aPiece = mGrid[x][y];
			if ((aPiece == NULL) || (aPiece->mType == PIECE_EMPTY))
				continue;
			int aType = aPiece->mType;
			if (!aPiece->mRemoved && !aPiece->mFading && !aPiece->mMoving)
				continue;
			if (aPiece->mMoving && !aPiece->mRemoved && !aPiece->mFading)
			{
				// the cell the animal has walked to (it stays in its grid slot until it is taken in), not the slot's
				int aX = CellLeft(aPiece->mX) + aPiece->mOffsetX;
				int aY = CellTop(aPiece->mY) + aPiece->mOffsetY;
				if (!aPiece->mFacingRight)
				{
					aX += 6;
					aY += 6;
				}
				Graphics aClip(*g);
				aClip.ClipRect(aX, aY, CELL_SIZE, CELL_SIZE);
				if (aPiece->mAlpha < 0xff)
				{
					aClip.SetColorizeImages(true);
					aClip.SetColor(Color(0xff, 0xff, 0xff, std::max(0, aPiece->mAlpha)));
				}
				else if (aPiece->mFlash)
				{
					aClip.SetColorizeImages(true);
					aClip.SetColor(Color(mFlashPulse, mFlashPulse, mFlashPulse));
				}
				aClip.DrawImage(anImages[IMAGE_ANIMALS], aX - aType * CELL_SIZE, aY);
				aClip.SetColorizeImages(false);
				continue;
			}
			// leaving for the ark (shrinking, shaded) or abandoned
			int aSize = (int) aPiece->mScale;
			int aX = (int) aPiece->mPixelX;
			int aY = (int) aPiece->mPixelY;
			if (!aPiece->mFacingRight && !aPiece->mFading)
			{
				aX += 6;
				aY += 6;
			}
			int aAlpha = std::max(0, std::min(0xff, aPiece->mAlpha));
			g->SetColor(Color(10, 10, 10, aAlpha));
			g->FillRect(aX + 1, aY + 1, aSize, aSize);
			Graphics aClip(*g);
			aClip.ClipRect(aX, aY, aSize, aSize);
			if (aPiece->mAlpha < 0xff)
			{
				aClip.SetColorizeImages(true);
				aClip.SetColor(Color(0xff, 0xff, 0xff, aAlpha));
			}
			if (aPiece->mRemoved)
			{
				// the animal is stretched into a picture of its own with the framework's fast stretch (0x40a6f9 does so for
				// the translucent ones, the opaque ones go straight to the screen, which a software surface stretches the same way)
				MemoryImage aStretched(mApp);
				aStretched.Create(aSize, aSize);
				{
					Graphics aStretch(&aStretched);
					aStretch.SetFastStretch(true);
					aStretch.DrawImage(anImages[IMAGE_ANIMALS], Rect(0, 0, aSize, aSize), Rect(aType * CELL_SIZE, 0xfa, CELL_SIZE, CELL_SIZE));
				}
				aClip.DrawImage(&aStretched, aX, aY);
			}
			else if (aPiece->mFading)
			{
				aClip.DrawImage(anImages[IMAGE_ANIMALS], aX - aType * CELL_SIZE, aY - 0x15e);
			}
			aClip.SetColorizeImages(false);
		}
	}

	DrawScore(g);
	DrawSparkles(g);
	DrawTexts(g);
	if (anApp->mBonusRound || (anApp->mGameMode != GAMEMODE_PUZZLE))
		DrawLevelNumber(g);
	DrawModNote(g);

	if (mComboTextTimer > 0)
	{
		SexyString aText = FormatText(GT(0x00494334), mShownCombo);
		g->SetFont(anApp->mFonts[FONT_SMALL]);
		g->SetColor(Color(0xff, 0xff, 0xff));
		int aWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, (CellLeft(0) - 10 - aWidth) / 2, 0x7b);
	}

	// the how-to-play box
	{
		g->SetFont(anApp->mFonts[FONT_GARAMOND_TINY]);
		g->SetColor(Color(0xff, 0xff, 0xff, std::min(0xff, mHintAlpha)));
		g->SetColorizeImages(true);
		g->DrawImage(anImages[IMAGE_SCROLL], -1, 0x145);
		g->SetColorizeImages(false);
		g->SetColor(Color(0, 0, 0, std::min(0xff, mHintAlpha)));
		for (int i = 0; i < 3; i++)
		{
			const SexyString& aText = HintLine(mHintIndex, i);
			int aWidth = g->GetFont()->StringWidth(aText);
			g->DrawString(aText, (CellLeft(0) - 10 - aWidth) / 2, 0x15b + i * 0xf);
		}
	}

	DrawPreview(g, true);

	if (!mPaused && anApp->mHasFocus)
	{
		int aMode = anApp->mGameMode;
		if (((mState == BOARD_NO_MOVES) || mNoMoves) && (anApp->mHighScoreWidget == NULL))
			DrawBanner(g, GT(0x00494258));
		else if (mState == BOARD_ABANDON)
			DrawBanner(g, GT(0x00494258));
		else if (mState == BOARD_GET_READY)
		{
			if (mShowGetReady)
				DrawBanner(g, GT(0x0049424c));
			Graphics aClip(*g);
			aClip.ClipRect(0x91, 0, 0x1d6, 7);
			aClip.DrawImage(anImages[IMAGE_RAIN1], 0, 0);
		}
		if (((mState == BOARD_WON) || (mState == BOARD_LEVEL_DONE)) && BoardCleared() && (aMode == GAMEMODE_ACTION))
		{
						const SexyString& aSpeed = GT(0x00494244);
			const SexyString& aBonus = GT(0x0049423c);
			g->SetFont(anApp->mFonts[FONT_GARAMOND_SMALL]);
			g->SetColor(Color(0, 0, 0, 0x96));
			for (int aOffsetX = -1; aOffsetX < 2; aOffsetX++)
				for (int aOffsetY = -1; aOffsetY < 2; aOffsetY++)
				{
					g->DrawString(aSpeed, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aSpeed)) / 2 + aOffsetX, 0xfa + aOffsetY);
					g->DrawString(aBonus, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aBonus)) / 2 + aOffsetX, 0x110 + aOffsetY);
					SexyString aBuffer = FormatText(GT(0x00494234), mBonusPoints);
					g->DrawString(aBuffer, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aBuffer)) / 2 - g->GetFont()->StringWidth(GT(0x00494230)) / 2 + aOffsetX, 0x126 + aOffsetY);
				}
			g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
			g->DrawString(aSpeed, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aSpeed)) / 2, 0xfa);
			g->DrawString(aBonus, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aBonus)) / 2, 0x110);
			SexyString aBuffer = FormatText(GT(0x00494234), mBonusPoints);
			g->DrawString(aBuffer, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aBuffer)) / 2 - g->GetFont()->StringWidth(GT(0x00494230)) / 2, 0x126);
		}
	}
}
