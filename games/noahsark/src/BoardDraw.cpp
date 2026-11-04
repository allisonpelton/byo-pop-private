// Board.cpp, part 5: drawing (WinNoah.exe 0x4092a0 Board::vf17 and its helpers 0x408810, 0x408ce0, 0x409180,
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
	for (int dx = -1; dx < 2; dx++)
		for (int dy = -1; dy < 2; dy++)
			g->DrawString(theText, theX + dx, theY + dy);
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
		aClip.SetColorizeImages(true);
		aClip.DrawImage(mApp->mImages[IMAGE_BUBBLES], (int) aBubble->mX + aOffset, (int) aBubble->mY);
		aClip.SetColorizeImages(false);
	}
}

// 0x408810
void Board::DrawSparkles(Graphics* g)
{
	for (size_t i = 0; i < mSparkles.size(); i++)
	{
		Sparkle* s = mSparkles[i];
		int aY = (s->mHeight + s->mY) - s->mFrame;
		Graphics aClip(*g);
		aClip.SetColor(Color(0xff, 0xff, 0xff, std::max(0, std::min(0xff, s->mLife))));
		aClip.SetColorizeImages(true);
		aClip.ClipRect(s->mX, aY, 0xf, 0xf);
		int aFrame = abs((s->mFrame / 5) % 4);
		aClip.DrawImage(mApp->mImages[IMAGE_HEARTS], s->mX, aY - aFrame * 0xf);
		aClip.SetColorizeImages(false);
	}
}

// 0x407fa0: the floating scores, in the colour of the animal with a dark outline, and a star per combo step
void Board::DrawTexts(Graphics* g)
{
	for (size_t i = 0; i < mTexts.size(); i++)
	{
		FloatingText* t = mTexts[i];
		g->SetFont(mApp->mFonts[FONT_COLORLESS]);
		int aAlpha = std::min(t->mAlpha, 0xff);
		SexyString aText = t->mText;
		int aWidth = g->GetFont()->StringWidth(aText);
		int aX = (0x1e - aWidth) / 2 + t->mX;
		g->SetColor(Color(0xdc, 0xdc, 0xdc, aAlpha));
		for (int dx = -1; dx < 2; dx++)
			for (int dy = -1; dy < 2; dy++)
				g->DrawString(aText, aX + dx, t->mY + dy);
		g->SetColor(Color(t->mColor.mRed, t->mColor.mGreen, t->mColor.mBlue, aAlpha));
		g->DrawString(aText, aX, t->mY);

		g->SetColorizeImages(true);
		g->SetColor(Color(t->mColor.mRed, t->mColor.mGreen, t->mColor.mBlue, aAlpha));
		int aStarX = t->mX + 7;
		int aStarY = t->mY;
		int aStars = t->mStars;
		int aMode = mApp->mGameMode;
		if ((aMode != GAMEMODE_STAMPEDE) && (aMode != GAMEMODE_PUZZLE))
		{
			Image* aStar = mApp->mImages[IMAGE_STAR];
			while (aStars > 0)
			{
				if (aStars >= 4)
				{
					g->DrawImage(aStar, t->mX - 0xb, aStarY);
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
					g->DrawImage(aStar, t->mX + 1, aStarY);
					g->DrawImage(aStar, t->mX + 0xd, aStarY);
					aStars = 0;
				}
				else
				{
					g->DrawImage(aStar, t->mX + 7, aStarY);
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
	for (int n = aLevel; n > 9; n /= 10)
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
			Piece* p = mLink[i];
			if ((p != NULL) && (p->mPath.size() >= 2) && p->mFacingRight)
			{
				Color aTint = Lighter(AnimalColor(p->mType));
				DrawTrailArrow(g, p->mPath.back(), p->mPath[p->mPath.size() - 2], aTint);
				if (p->mPath.size() > 2)
					DrawTrailNodes(g, p->mPath, 1, (int) p->mPath.size() - 1, aTint);
			}
		}
	}
}

// 0x40c460: the part of a walking pair's trail that is still ahead of its first animal
void Board::DrawWalkingTrails(Graphics* g)
{
	g->SetColorizeImages(false);
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* p = mGrid[x][y];
			if (!p->mMoving || !p->mFacingRight || p->mPath.size() < 3)
				continue;
			Color aTint = Lighter(AnimalColor(p->mType));
			int aTo = (int) p->mPath.size();
			if (p->mPartner != NULL)
				aTo -= p->mPartner->mPathIndex;
			int aFrom = p->mPathIndex;
			if (aFrom < 1)
				aFrom = 1;
			DrawTrailNodes(g, p->mPath, aFrom, std::min(aTo, (int) p->mPath.size() - 1), aTint);
		}
	}
}

// the outlined caption in the middle of the board ("Get Ready!", "No More Moves!", "PAUSED")
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
	Image** anImages = anApp->mImages;

	// the sky (rain, or the lightning flash)
	Image* aSky = mLightning ? anImages[IMAGE_LIGHTNING] : anImages[IMAGE_RAIN1 + (mBackdropTick & 1)];
	g->DrawImage(aSky, 0, 0);

	// the flood: the waves in a colour that follows the danger, then the body of water
	int aSurface = mWaterY - 0xf;
	bool aDanger = (mWaterY < mArkLevelY + 0x14) && ((mState == BOARD_PLAYING) || (mState == BOARD_SWAP) ||
		(mState == BOARD_SWAP_BACK) || (mState == BOARD_SETTLE));
	Color aWave;
	if (mStormTimer < 1)
		aWave = aDanger ? Color(std::max(0, mPulseA - 0x48), 0, 0) : Color(0, 0, 0x80);
	else
		aWave = Color(0x82, 0x82, 0xcd);
	g->SetColor(aWave);
	g->SetColorizeImages(true);
	for (int x = 0; x < mWidth; x += 300)
	{
		Graphics aClip(*g);
		aClip.ClipRect(x, aSurface, 300, 0x1e);
		aClip.DrawImage(anImages[IMAGE_WAVES], x + (((x / 300) + mWavePhaseA) % 0x1e) * -300, aSurface);
	}
	g->SetColor(aDanger ? Color(std::max(0, mPulseA - 0x48), 0, 0) : Color(0, 0, 0x80));
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
				aColor = Color(std::min(0xff, std::max(0, mPulseA - 0x48) + aShift), aShift, aShift);
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
			aClip.DrawImage(anImages[IMAGE_WAVES], (x + (((x / 300) + mWavePhaseB) % 0x1e) * -300) - aShift, mWaterY + i);
		}
		g->SetColorizeImages(false);
	}
	if (mStormTimer < 1)
	{
		if ((mWaterY < mArkLevelY + 0x14) && ((mState == BOARD_PLAYING) || (mState == BOARD_SWAP) ||
			(mState == BOARD_SWAP_BACK) || (mState == BOARD_SETTLE)))
			g->SetColor(Color(std::min(0xff, mPulseA + 0x2d), 0x2d, 0x2d));
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
		for (int x = 0; x < 9; x++)
		{
			for (int y = 0; y < 9; y++)
			{
				Piece* p = mGrid[x][y];
				if (p == NULL)
					continue;
				int aX = CellLeft(x) + p->mOffsetX;
				int aY = CellTop(y) + p->mOffsetY;
				int aType = p->mType;
				bool aQuiet = !p->mRemoved && !p->mFading && !p->mMoving;
				if ((aType == PIECE_BLOCK) || (aType == PIECE_CRATE))
				{
					if ((p->mAlpha < 0xff) && (mState == BOARD_LEVEL_DONE))
					{
						aX += (int) (sinf(y * 0.5f + mUpdateCnt * 0.0667f) * 20.0f);
						aY += 14;
					}
					// the original's first group of obstacles (0x401ce0, the ones the port numbers 0x10) shows block.jpg, its second the crate
					g->DrawImage(anImages[(aType == PIECE_BLOCK) ? IMAGE_CRATE : IMAGE_BLOCK], aX, aY);
					continue;
				}
				if ((aType == PIECE_EMPTY) || !aQuiet)
					continue;
				if ((p->mAlpha < 0xff) && (mState == BOARD_LEVEL_DONE))
				{
					aX += (int) (sinf(y * 0.5f + mUpdateCnt * 0.0667f) * 20.0f);
					aY += 14;
				}
				Graphics aClip(*g);
				aClip.ClipRect(aX, aY, 0x32, 0x32);
				if (p->mAlpha < 0xff)
				{
					aClip.SetColorizeImages(true);
					aClip.SetColor(Color(0xff, 0xff, 0xff, std::max(0, p->mAlpha)));
				}
				else if (p->mFlash)
				{
					aClip.SetColorizeImages(true);
					aClip.SetColor(Color(mPulseB, mPulseB, mPulseB));
				}
				bool aHover = (mSelected == NULL) && (CellAtX(mMouseX) == x) && (CellAtY(mMouseY) == y) &&
					(anApp->mGameMode != GAMEMODE_STAMPEDE) && (x >= 0) && (y >= 0);
				bool aInLink = std::find(mLink.begin(), mLink.end(), p) != mLink.end();
				int aRow;
				if ((!p->mSelected || (mState != BOARD_GET_READY)) && !p->mFlash)
				{
					if (aHover)
						aRow = 4;
					else if ((p == mSelected) || p->mSelected || aInLink)
						aRow = 6;
					else if (anApp->mNextLevelWidget == NULL)
						aRow = p->mBlink;
					else
						aRow = 0;
				}
				else
					aRow = 5;
				aClip.DrawImage(anImages[IMAGE_ANIMALS], aX - aType * 0x32, aY - aRow * 0x32);
				aClip.SetColorizeImages(false);
			}
		}
		// the Stampede's waiting row
		if ((anApp->mGameMode == GAMEMODE_STAMPEDE) && (mState == BOARD_PLAYING))
		{
			for (int x = 0; x < 9; x++)
			{
				int aX = CellLeft(x);
				int aY = CellTop(9) + mNextRow[x]->mOffsetY;
				Graphics aClip(*g);
				aClip.ClipRect(aX, aY, 0x32, 0x32);
				aClip.DrawImage(anImages[IMAGE_ANIMALS], aX - mNextRow[x]->mType * 0x32, aY - 0x32 * 2 + 0x32 * 2 - 0x32 * 0);
			}
		}
	}
	else
	{
		DrawBanner(g, GT(0x00494340));
	}

	DrawWalkingTrails(g);

	// the animals that walk, are taken in or are abandoned
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* p = mGrid[x][y];
			if ((p == NULL) || (p->mType == PIECE_EMPTY))
				continue;
			int aType = p->mType;
			if (!p->mRemoved && !p->mFading && !p->mMoving)
				continue;
			if (p->mMoving && !p->mRemoved && !p->mFading)
			{
				// the cell the animal has walked to (it stays in its grid slot until it is taken in), not the slot's
				int aX = CellLeft(p->mX) + p->mOffsetX;
				int aY = CellTop(p->mY) + p->mOffsetY;
				if (!p->mFacingRight)
				{
					aX += 6;
					aY += 6;
				}
				Graphics aClip(*g);
				aClip.ClipRect(aX, aY, 0x32, 0x32);
				if (p->mAlpha < 0xff)
				{
					aClip.SetColorizeImages(true);
					aClip.SetColor(Color(0xff, 0xff, 0xff, std::max(0, p->mAlpha)));
				}
				else if (p->mFlash)
				{
					aClip.SetColorizeImages(true);
					aClip.SetColor(Color(mPulseB, mPulseB, mPulseB));
				}
				aClip.DrawImage(anImages[IMAGE_ANIMALS], aX - aType * 0x32, aY - 6 * 0x32);
				aClip.SetColorizeImages(false);
				continue;
			}
			// leaving for the ark (shrinking, shaded) or abandoned
			int aSize = (int) p->mScale;
			int aX = (int) p->mPixelX;
			int aY = (int) p->mPixelY;
			if (!p->mFacingRight && !p->mFading)
			{
				aX += 6;
				aY += 6;
			}
			int aAlpha = std::max(0, std::min(0xff, p->mAlpha));
			g->SetColor(Color(10, 10, 10, aAlpha));
			g->FillRect(aX + 1, aY + 1, aSize, aSize);
			Graphics aClip(*g);
			aClip.ClipRect(aX, aY, aSize, aSize);
			if (p->mAlpha < 0xff)
			{
				aClip.SetColorizeImages(true);
				aClip.SetColor(Color(0xff, 0xff, 0xff, aAlpha));
			}
			if (p->mRemoved)
			{
				if (p->mAlpha < 0xff)
				{
					Rect aSrc(aType * 0x32, 0xfa, 0x32, 0x32);
					aClip.DrawImage(anImages[IMAGE_ANIMALS], Rect(aX, aY, aSize, aSize), aSrc);
				}
				else
				{
					Rect aSrc(aType * 0x32, 0xfa, 0x32, 0x32);
					aClip.DrawImage(anImages[IMAGE_ANIMALS], Rect(aX, aY, aSize, aSize), aSrc);
				}
			}
			else if (p->mFading)
			{
				aClip.DrawImage(anImages[IMAGE_ANIMALS], aX - aType * 0x32, aY - 0x15e);
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

	if (mComboShown > 0)
	{
		SexyString aText = FormatText(GT(0x00494334), mComboShownAt);
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
			char aBuffer[64];
			const SexyString& aSpeed = GT(0x00494244);
			const SexyString& aBonus = GT(0x0049423c);
			g->SetFont(anApp->mFonts[FONT_GARAMOND_SMALL]);
			g->SetColor(Color(0, 0, 0, 0x96));
			for (int dx = -1; dx < 2; dx++)
				for (int dy = -1; dy < 2; dy++)
				{
					g->DrawString(aSpeed, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aSpeed)) / 2 + dx, 0xfa + dy);
					g->DrawString(aBonus, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aBonus)) / 2 + dx, 0x110 + dy);
					snprintf(aBuffer, sizeof(aBuffer), "%d", mBonusPoints);
					g->DrawString(aBuffer, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aBuffer)) / 2 + dx, 0x126 + dy);
				}
			g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
			g->DrawString(aSpeed, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aSpeed)) / 2, 0xfa);
			g->DrawString(aBonus, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aBonus)) / 2, 0x110);
			snprintf(aBuffer, sizeof(aBuffer), "%d", mBonusPoints);
			g->DrawString(aBuffer, (CellLeft(0) - 10 - g->GetFont()->StringWidth(aBuffer)) / 2, 0x126);
		}
	}
}
