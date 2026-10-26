// port(variant): Draw (0x4cbc70) for the boards that need more than it draws (games/bejeweled/docs/CODE_MODS_DESIGN.md
// 5.6), a Board member. Not part of the original game. Draw dispatches here on VariantFlags::mDraw.
//
// Stage 7: a copy of Draw for boards of any size and shape. Its board layer (the gems with their glow crossfades and
// shrink frames, the hint sparkles, the target cursor, the tutorial arrows) goes through a Graphics that scales the
// logical positions by mLayout.mScale about the board window's corner (203, 16); the score popups are drawn at their
// screen positions in the unscaled font; the vortex, the banner, the hole mask and the pause text are centred on the
// window's centre (406, 219) whatever the board's logical centre. The tutorial arrows only with a tutorial move
// (mTutorialCol >= 0). Unscaled boards take no scale branch, so on the original 8x8 this draws Draw's pixels
// (-forcevariant's screenshots check it).
//
// Stage 7b (4.14): the dirt cells under the gems (DrawDirt) and the dust of a cleared one over them (DrawDirtDust).

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Res.h"
#include "Graphics.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "ImageFont.h"

#include <algorithm>

using namespace Sexy;

// Board.cpp's DrawSparkle
static void DrawSparkle(Graphics* g, Image* theSparkleImage, int theFrame, int theX, int theY)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->DrawImage(theSparkleImage, theX, theY, Rect(theFrame * 40, 0, 40, 40));
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

void Board::DrawVariant(Graphics* g)
{
	int aCenterX = BOARD_CENTER_X;
	int aCenterY = BOARD_CENTER_Y;

	g->DrawImage(mBackgroundImage, 0, 0);
	if (mBlackHoleLevel > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mBlackHoleLevel * 255.0)));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[IMAGE_BLACKHOLE], aCenterX - 128, aCenterY - 128, Rect(mBlackHoleFrame * 256, 0, 256, 256));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	if (mUserPauseCount == 0)
	{
		// The board layer in logical space: aScaledG unclipped (sparkles, cursor, arrows), aBoardG clipped (gems)
		Graphics aScaledG(*g);
		if (mLayout.mScale != 1.0)
			aScaledG.SetScale((float) mLayout.mScale, (float) mLayout.mScale, (float) BoardLayout::VIEW_X, (float) BoardLayout::VIEW_Y);
		Graphics aBoardG(aScaledG);
		aBoardG.ClipRect(mLayout.mClipX, mLayout.mClipY, mLayout.mClipWidth, mLayout.mClipHeight); // the board's own rectangle (4.16): (194, 12, 428, 416) on 8x8
		if (mVariant.mDirt)
			DrawDirt(&aBoardG);

		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if ((aPiece == NULL) || (mVariant.mMoves && DrawsSlideCell(aCol, aRow))) // a sliding line: DrawMovesUnder
					continue;

				int aSrcX = aPiece->mSpinFrame * 52 + 2;
				int aSrcY = aPiece->mColor * 104;
				if ((aPiece->mGlowing) && (aPiece->mSpinFrame == 0) && (!aPiece->mDying))
				{
					Image* aGemSheet = mApp->mImages[IMAGE_GEMSHEET];
					DDImage* aGlowImage = mApp->CreateCrossfadeImage(aGemSheet, Rect(aSrcX, aSrcY, GEM_SIZE, GEM_SIZE),
						aGemSheet, Rect(aPiece->mGlowFrame * 52 + aSrcX, aSrcY + 52, GEM_SIZE, GEM_SIZE), aPiece->mGlowAlpha);
					aBoardG.DrawImage(aGlowImage, (int)aPiece->mX, (int)aPiece->mY);
					delete aGlowImage;
				}
				else if (aPiece->mDying)
					aBoardG.DrawImage(mApp->mGemShrinkImages[aPiece->mColor][aPiece->mDyingFrame], (int)aPiece->mX, (int)aPiece->mY);
				else
					aBoardG.DrawImage(mApp->mImages[IMAGE_GEMSHEET], (int)aPiece->mX, (int)aPiece->mY, Rect(aSrcX, aSrcY, GEM_SIZE, GEM_SIZE));
			}
		}

		if (mVariant.mMoves)
			DrawMovesUnder(&aBoardG);

		Image* aSparkleImage = mApp->mImages[IMAGE_SPARKLE];
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if ((aPiece == NULL) || (aPiece->mSparkleCount <= 0))
					continue;

				if (aPiece->mSparkleCount >= 16)
					DrawSparkle(&aScaledG, aSparkleImage, aPiece->mSparkleFrame % 14, (int)(aPiece->mX + 13.0), (int)(aPiece->mY - 7.0));
				if (aPiece->mSparkleCount >= 8)
				{
					int aFrame = (aPiece->mSparkleFrame - 8) % 14;
					if (aFrame >= 0)
						DrawSparkle(&aScaledG, aSparkleImage, aFrame, (int)(aPiece->mX - 10.0), (int)(aPiece->mY + 3.0));
				}
				int aFrame = (aPiece->mSparkleFrame - 16) % 14;
				if (aFrame >= 0)
					DrawSparkle(&aScaledG, aSparkleImage, aFrame, (int)(aPiece->mX + 5.0), (int)(aPiece->mY + 19.0));
				if (gGameConfig.mAnyModEnabled)
				{
					int aModFrame = (aPiece->mSparkleFrame - 24) % 14;
					if (aModFrame >= 0)
						DrawSparkle(&aScaledG, aSparkleImage, aModFrame, (int)(aPiece->mX + 24.0), (int)(aPiece->mY + 6.0));
				}
			}
		}

		if (mVariant.mDirt)
			DrawDirtDust(&aScaledG);

		Piece* aCursorPieces[3] = {mSelectedPiece, mSwapPiece1, mSwapPiece2};
		for (int i = 0; i < 3; i++)
		{
			if (aCursorPieces[i] != NULL)
				aScaledG.DrawImage(mApp->mImages[IMAGE_TARGET], (int)(aCursorPieces[i]->mX - 2.0), (int)(aCursorPieces[i]->mY - 2.0), Rect((mCursorFrame % 6) * 52, 0, 52, 52));
		}
		if (mVariant.mMoves)
			DrawMovesOver(&aScaledG);

		g->SetColor(Color::White);
		for (std::list<ScorePopup>::iterator anItr = mScorePopups.begin(); anItr != mScorePopups.end(); ++anItr)
		{
			g->SetFont(mApp->mAlphaDigitsFont);
			g->SetColor(Color(255, 255, 255, std::min(255, anItr->mLife * 255 / 18)));
			SexyString aPoints = StrFormat("%d", anItr->mPoints);
			int aScreenX = (int)mLayout.ToScreenX(anItr->mX);
			int aScreenY = (int)mLayout.ToScreenY(anItr->mY);
			g->DrawString(aPoints, aScreenX - mPointsFont->StringWidth(aPoints) / 2, mPointsFont->GetAscent() / 2 + aScreenY);
		}

		if (mShowTutorial && (mTutorialCol >= 0))
		{
			aScaledG.DrawImage(mApp->mImages[IMAGE_SWAPPER1], GetColX(mTutorialCol) + 3, GetRowY(mTutorialRow) + 38);
			int aHandCol = mTutorialCol;
			if ((mSelectedPiece != NULL) && (mSelectedPiece->mCol == mTutorialCol) && (mSelectedPiece->mRow == mTutorialRow))
				aHandCol = mTutorialCol2;
			aScaledG.DrawImage(mApp->mImages[IMAGE_SWAPPER2], GetColX(aHandCol) - 5, GetRowY(mTutorialRow) - 42 + mTutorialBob);
		}

		if (mState == STATE_LEVEL_WIPE)
		{
			Graphics aWipeG(*g);
			aWipeG.ClipRect(0, mWipeY - 8, mWidth, 16);
			aWipeG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			for (int i = 0; i < (mWidth + mElectOffset + 127) / 128; i++)
				aWipeG.DrawImage(mApp->mImages[IMAGE_ELECT], i * 128 - mElectOffset, mWipeY - 8);
			aWipeG.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			aWipeG.DrawImage(mApp->mImages[IMAGE_ALPHABACK], 0, 0);
		}

		if (mShowingText)
		{
			ColorizeText();
			g->DrawImage(mTextColorImage, aCenterX - mTextColorImage->GetWidth() / 2, aCenterY - mTextColorImage->GetHeight() / 2 - mHeight / 6);
		}

		if (mBlackHoleLevel > 0.0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int)(mBlackHoleLevel * 255.0)));
			Image* aHoleMask = mApp->mImages[IMAGE_HOLEMASK];
			g->DrawImage(aHoleMask, aCenterX - aHoleMask->GetWidth() / 2, aCenterY - aHoleMask->GetHeight() / 2);
			g->SetColorizeImages(false);
		}
	}
	else if ((mHasFocus) || (!mApp->mHasFocus))
	{
		g->SetFont(mApp->mHeaderFont);
		g->SetColor(Color::White);
		SexyString aText = mApp->GetString("GAME_PAUSED");
		g->DrawString(aText, aCenterX - g->GetFont()->StringWidth(aText) / 2, aCenterY - g->GetFont()->GetHeight() / 2);
		aText = mApp->GetString("CLICK_HERE_TO_CONTINUE");
		g->DrawString(aText, aCenterX - g->GetFont()->StringWidth(aText) / 2, aCenterY + g->GetFont()->GetHeight());
	}

	g->SetFont(mApp->mScoreFont);
	SexyString aScore = StrFormat("%d", mDisplayScore);
	g->SetColor(Color(173, 231, 255));
	g->DrawString(aScore, 93 - mScoreFont->StringWidth(aScore) / 2, 106);

	g->DrawImage(mApp->mTimerBarImages[mBarColor], 237, 444, Rect(0, 0, mBarWidth, 25));

	SexyString aLevel = StrFormat("%d", mDisplayLevel);
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color::Black);
	g->DrawString(aLevel, 606 - g->GetFont()->StringWidth(aLevel) / 2, 465);

	if (mMoveScoreAlpha > 0.0)
	{
		SexyString aMoveScore = StrFormat("+%d", mMoveScore);
		g->SetFont(mApp->mHeaderFont);
		g->SetColor(Color(255, 255, 255, (int)(mMoveScoreAlpha * 255.0)));
		g->DrawString(aMoveScore, 91 - g->GetFont()->StringWidth(aMoveScore) / 2 - g->GetFont()->CharWidth('+') / 3, 350);
	}

	if (mHintGlowAlpha > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mHintGlowAlpha * 255.0)));
		g->DrawImage(mApp->mImages[IMAGE_OVER_HINT], 64, 303);
		g->SetColorizeImages(false);
	}
}

// ---------------------------------------------------------------------------------------------------
// Dirt (stage 7b, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.14)

// The crack lines of the dirt art, in a cell's 51 x 51 px: a cell with n layers shows the first n
static const int kCrackPoints = 4;
static const int kCracks[BoardLayout::MAX_DIRT_LAYERS][kCrackPoints][2] =
{
	{{8, 10}, {18, 20}, {16, 30}, {24, 40}},
	{{42, 8}, {34, 18}, {38, 26}, {35, 31}},
	{{10, 44}, {20, 36}, {30, 40}, {33, 45}},
	{{44, 40}, {36, 32}, {40, 22}, {45, 18}},
	{{25, 6}, {27, 16}, {22, 24}, {25, 28}},
	{{6, 26}, {14, 28}, {18, 34}, {14, 38}},
	{{30, 46}, {32, 38}, {28, 30}, {31, 26}},
	{{46, 28}, {40, 30}, {36, 40}, {39, 45}},
	{{16, 14}, {24, 12}, {30, 20}, {29, 24}}
};

static void PutCrackPixel(ulong* theBits, int theX, int theY, ulong theColor)
{
	if ((theX >= 0) && (theX < Board::CELL_SIZE) && (theY >= 0) && (theY < Board::CELL_SIZE))
		theBits[theY * Board::CELL_SIZE + theX] = theColor;
}

// The cracks of a dirt cell with theLayers layers (1..9): dark lines with a light edge below them, on a clear cell;
// the same for every level and board, made once
static Image* DirtCracks(WinDMApp* theApp, int theLayers)
{
	static MemoryImage* sCracks[BoardLayout::MAX_DIRT_LAYERS] = {};
	MemoryImage*& anImage = sCracks[theLayers - 1];
	if (anImage == NULL)
	{
		anImage = new MemoryImage(theApp);
		anImage->Create(Board::CELL_SIZE, Board::CELL_SIZE);
		ulong* aBits = anImage->GetBits();
		std::fill(aBits, aBits + Board::CELL_SIZE * Board::CELL_SIZE, (ulong) 0);
		for (int aPass = 0; aPass < 2; aPass++)
		{
			for (int aCrack = 0; aCrack < theLayers; aCrack++)
			{
				for (int i = 0; i + 1 < kCrackPoints; i++)
				{
					int aX0 = kCracks[aCrack][i][0], aY0 = kCracks[aCrack][i][1];
					int aX1 = kCracks[aCrack][i + 1][0], aY1 = kCracks[aCrack][i + 1][1];
					int aSteps = std::max(std::abs(aX1 - aX0), std::abs(aY1 - aY0));
					for (int k = 0; k <= aSteps; k++)
					{
						int aX = aX0 + (aX1 - aX0) * k / std::max(aSteps, 1);
						int aY = aY0 + (aY1 - aY0) * k / std::max(aSteps, 1);
						// the light edge first, the dark line over it
						if (aPass == 0)
							PutCrackPixel(aBits, aX + 1, aY + 1, 0x90D8B888);
						else
							PutCrackPixel(aBits, aX, aY, 0xFF2A1A10);
					}
				}
			}
		}
		anImage->BitsChanged();
	}
	return anImage;
}

// A mod's dirt tile (images/dirttile), looked up once; NULL without one
static Image* ModDirtTile(WinDMApp* theApp)
{
	static bool isLookedUp = false;
	static Image* aDirtTile = NULL;
	if (!isLookedUp)
	{
		isLookedUp = true;
		aDirtTile = theApp->GetImage("images\\dirttile", true);
	}
	return aDirtTile;
}

// The dirt cells, under the gems: the level's checker tile A or B tinted brown (a mod's images/dirttile in its
// place, when there is one), and a crack for each layer left. The tiles are made once per level
void Board::DrawDirt(Graphics* g)
{
	if (!mLayout.HasDirt())
		return;
	Image* aModTile = ModDirtTile(mApp);
	if ((aModTile == NULL) && (mDirtTilesLevel != mLevel))
	{
		for (int i = 0; i < 2; i++)
		{
			delete mDirtTiles[i];
			MemoryImage aTexture(mApp);
			aTexture.Create(CELL_SIZE, CELL_SIZE);
			Graphics aTextureG(&aTexture);
			aTextureG.DrawImage(mApp->mImages[IMAGE_CHECKERTEXTURES], 0, 0, Rect((mLevel % 5) * 102 + i * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE));
			mDirtTiles[i] = mApp->CreateColorizedImage(&aTexture, Color(150, 100, 60));
		}
		mDirtTilesLevel = mLevel;
	}
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			if (!mLayout.IsDirt(aCol, aRow))
				continue;
			int aX = GetColX(aCol) - 1;
			int aY = GetRowY(aRow);
			Image* aTile = (aModTile != NULL) ? aModTile : mDirtTiles[(aRow + aCol) % 2];
			g->DrawImage(aTile, aX, aY, Rect(0, 0, std::min((int) CELL_SIZE, aTile->GetWidth()), std::min((int) CELL_SIZE, aTile->GetHeight())));
			g->DrawImage(DirtCracks(mApp, mLayout.mDirt[aRow][aCol]), aX, aY);
		}
	}
}

// How long a cleared dirt cell's dust shows
static const int DIRT_DUST_TICKS = 40;

// A grain of dust: a light tan disc with a brown rim, 8 x 8 px, made once
static Image* DirtGrain(WinDMApp* theApp)
{
	static MemoryImage* sGrain = NULL;
	if (sGrain == NULL)
	{
		sGrain = new MemoryImage(theApp);
		sGrain->Create(8, 8);
		ulong* aBits = sGrain->GetBits();
		for (int y = 0; y < 8; y++)
		{
			for (int x = 0; x < 8; x++)
			{
				int aDist = (2 * x - 7) * (2 * x - 7) + (2 * y - 7) * (2 * y - 7); // 4 x the squared distance from the centre
				aBits[y * 8 + x] = (aDist <= 26) ? 0xFFE8CFA0 : (aDist <= 50) ? 0xFF8A6038 : 0;
			}
		}
		sGrain->BitsChanged();
	}
	return sGrain;
}

// The dust of the dirt cells cleared lately, over the gems: twelve grains fly out from the cell's centre, sink a little,
// shrink and fade (Graphics::FillRect would ignore the board's scale, so they are images)
void Board::DrawDirtDust(Graphics* g)
{
	while (!mDirtDust.empty() && (mUpdateCnt - mDirtDust.front().mStartTick >= DIRT_DUST_TICKS))
		mDirtDust.erase(mDirtDust.begin());
	if (mDirtDust.empty())
		return;
	static const int kDirs[12][2] = {{0, -10}, {5, -9}, {9, -5}, {10, 0}, {9, 5}, {5, 9}, {0, 10}, {-5, 9}, {-9, 5}, {-10, 0}, {-9, -5}, {-5, -9}};
	Image* aGrain = DirtGrain(mApp);
	g->SetColorizeImages(true);
	for (size_t i = 0; i < mDirtDust.size(); i++)
	{
		const DirtDust& aDust = mDirtDust[i];
		int anAge = mUpdateCnt - aDust.mStartTick;
		g->SetColor(Color(255, 255, 255, 255 - anAge * 255 / DIRT_DUST_TICKS));
		int aCenterX = GetColX(aDust.mCol) + 25;
		int aCenterY = GetRowY(aDust.mRow) + 25;
		int aSize = 10 - anAge / 8;
		for (int k = 0; k < 12; k++)
		{
			// grains alternate between two speeds, so the puff is no ring
			int aReach = 6 + anAge * ((k % 2 == 0) ? 7 : 5) / 4;
			int aX = aCenterX + kDirs[k][0] * aReach / 10 - aSize / 2;
			int aY = aCenterY + kDirs[k][1] * aReach / 10 - aSize / 2 + anAge * anAge / 80;
			g->DrawImage(aGrain, Rect(aX, aY, aSize, aSize), Rect(0, 0, 8, 8));
		}
	}
	g->SetColorizeImages(false);
}
