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

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Res.h"
#include "Graphics.h"
#include "DDImage.h"
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

		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece == NULL)
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

		Piece* aCursorPieces[3] = {mSelectedPiece, mSwapPiece1, mSwapPiece2};
		for (int i = 0; i < 3; i++)
		{
			if (aCursorPieces[i] != NULL)
				aScaledG.DrawImage(mApp->mImages[IMAGE_TARGET], (int)(aCursorPieces[i]->mX - 2.0), (int)(aCursorPieces[i]->mY - 2.0), Rect((mCursorFrame % 6) * 52, 0, 52, 52));
		}

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
