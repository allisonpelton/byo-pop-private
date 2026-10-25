// BoardDraw.cpp: the drawing half of Board.cpp (Board::Draw and its helpers, the end-of-level button
// and the stats panel). The Board class is declared in Board.h (board_logic); behaviour spec:
// re/bigmoney/analysis/board_draw.md section 2.

#include "Board.h"
#include "PuzzleSolver.h"
#include "HDPixels.h"
#include <algorithm>
#include "Piece.h"
#include "WinBMApp.h"
#include "Res.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "Insets.h"
#include <stdio.h>
#include <math.h>

using namespace Sexy;

static Color CoinColor(const int theTable[5][3], int theColor)
{
	return Color(theTable[theColor][0], theTable[theColor][1], theTable[theColor][2]);
}

// 0x405480
void Board::Draw(Graphics* g)
{
	// The playfield clip, used for the grid pieces (with mClipExtra 0: x 175..583, y 4..470).
	Graphics aClipG(*g);
	aClipG.ClipRect(175, 4, mClipExtra + 409, 467);

	// The playfield is not drawn once the pause screen has faded in completely.
	if (mPauseFade < 1.0)
	{
		g->DrawImage(mApp->mImages[IMAGE_BACKDROP], 178, 4);
		g->SetColor(mMovesColor); // stays set on g
		g->SetColorizeImages(true);
		g->DrawImage(mApp->mImages[IMAGE_BOTTOMLINE], 156, 432);
		g->SetColorizeImages(false);

		// Glows under the highlighted group, row-major, unclipped. Neither mBlinkHidden nor mIsSpecial
		// is tested here.
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece != NULL)
				{
					int aX = (int)(aPiece->mShakeX + aPiece->mX);
					int aY = (int)(aPiece->mShakeY + aPiece->mY);
					if (aPiece->mHighlighted)
					{
						g->SetColorizeImages(true);
						g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
						g->SetColor(CoinColor(gCoinLightColors, aPiece->mColor));
						g->DrawImage(mApp->mImages[IMAGE_COINGLOW], aX - 8, aY - 8);
						g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
						g->SetColorizeImages(false);
					}
				}
			}
		}

		// Grid pieces column by column, bottom row first, then the minting row below the grid.
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			for (int aRow = mRows - 1; aRow >= 0; aRow--)
			{
				if (mGrid[aRow][aCol] != NULL)
					DrawPiece(&aClipG, mGrid[aRow][aCol], 0);
			}

			if (mNextRow[aCol] != NULL)
				DrawPiece(&aClipG, mNextRow[aCol], mMintPixels);
		}

		for (std::list<Piece*>::iterator anItr = mLoosePieces.begin(); anItr != mLoosePieces.end(); ++anItr)
		{
			Piece* aPiece = *anItr;
			DrawPiece(aPiece->mFlying ? g : &aClipG, aPiece, 0);
		}

		// Bills: mType 0 is the full-size strip, 1..4 the scaled copies (app+0x7dc + mType*4 in the
		// original = mBillImages[mType - 1]).
		for (std::list<Bill>::iterator anItr = mBills.begin(); anItr != mBills.end(); ++anItr)
		{
			Bill& aBill = *anItr;
			Image* anImage = (aBill.mType == 0) ? mApp->mImages[IMAGE_BILL] : mApp->mBillImages[aBill.mType - 1];
			int aCelX = anImage->GetWidth() * aBill.mFrame / 16;
			int aCelEndX = anImage->GetWidth() * (aBill.mFrame + 1) / 16;
			int aHeight = anImage->GetHeight();
			g->DrawImage(anImage, (int)aBill.mX, (int)aBill.mY, Rect(aCelX, 0, aCelEndX - aCelX, aHeight));
		}
	}

	if (mPauseFade > 0.0)
	{
		// Pause screen: the backdrop scrolls up by 1 px every 8 updates, tiled three times.
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mPauseFade * 255.0)));
		Image* aBackdrop = mApp->mImages[IMAGE_PAUSEBACKDROP];
		int aWidth = aBackdrop->GetWidth();
		int aHeight = aBackdrop->GetHeight();
		int anOffset = (mPauseAnim / 8) % aHeight;
		g->DrawImage(aBackdrop, mWidth - aWidth, -anOffset);
		g->DrawImage(aBackdrop, mWidth - aWidth, -anOffset + aHeight);
		g->DrawImage(aBackdrop, mWidth - aWidth, -anOffset + 2*aHeight);

		if (mPauseClickable)
		{
			// Drawn in the fading colour set above
			g->SetFont(mApp->mFont1);
			Font* aFont = g->GetFont();
			SexyString aString = mApp->GetString("GAME_PAUSED");
			g->DrawString(aString, 380 - aFont->StringWidth(aString)/2, 210);
			aString = mApp->GetString("CLICK_HERE_TO_CONTINUE");
			g->DrawString(aString, 380 - aFont->StringWidth(aString)/2, 250);
		}
		g->SetColorizeImages(false);
	}

	// HUD frame, mode label, logo, right bars
	g->DrawImage(mApp->mImages[IMAGE_LEFTBAR], 0, 0);
	g->DrawImage(mApp->mImages[IMAGE_GAMEDIFFTYPE], 35, 161,
		Rect(0, (mApp->mGameType*3 + mApp->mSkillLevel)*15 - 15, 100, 15));
	g->DrawImage(mApp->mImages[IMAGE_FLOATLOGO], 15, 8);
	g->DrawImage(mApp->mImages[IMAGE_RIGHTBAR1], mWidth - mApp->mImages[IMAGE_RIGHTBAR1]->GetWidth() + mClipExtra, 0);

	// The coin stack (meter): 6 px per coin, the fraction of the top coin raises the whole stack;
	// drawn in chunks of at most 62 coins (the height of the strip).
	double aStack = mMeterCoins - mMeterDrained;
	int aWhole = (int)aStack;
	int aStackY = (int)((1.0 - (aStack - aWhole)) * 6.0 + 440.0);
	while (aStack > 0.0)
	{
		Image* aStackImage = mApp->mImages[IMAGE_STACK];
		int aCount = (int)aStack + 1;
		if (aCount >= aStackImage->GetHeight()/6)
			aCount = aStackImage->GetHeight()/6;
		aStackY -= aCount*6;
		g->DrawImage(aStackImage, 595, aStackY, Rect(0, 0, aStackImage->GetWidth(), aCount*6));
		aStack -= aCount;
	}

	for (std::list<TubeCoin>::iterator anItr = mTubeCoins.begin(); anItr != mTubeCoins.end(); ++anItr)
	{
		TubeCoin& aCoin = *anItr;
		g->DrawImage(mApp->mImages[IMAGE_COIN], 595, (int)aCoin.mY, Rect(aCoin.mFrame*40, 0, 40, 40));
	}

	g->DrawImage(mApp->mImages[IMAGE_RIGHTBAR2], mWidth - mApp->mImages[IMAGE_RIGHTBAR2]->GetWidth() + mClipExtra, 0);

	// Score, right-aligned at x 140, no thousands separators
	g->SetColor(Color(255, 255, 255));
	g->SetFont(mApp->mScoreFont);
	SexyString aScoreString = StrFormat("$%d", mDisplayScore);
	g->DrawString(aScoreString, 140 - g->GetFont()->StringWidth(aScoreString), 110);

	g->DrawImage(mApp->mImages[IMAGE_BARBAGDISPLAY], mClipExtra + 592, 4, Rect(mMeterFlashFrame*42, 0, 42, 43));

	g->DrawImage(mApp->mImages[IMAGE_LEVEL], 589, 441);
	g->SetFont(mApp->mLevelFont);
	g->SetColor(Color(255, 255, 255));
	SexyString aLevelString = StrFormat("%d", mLevel + 1);
	g->DrawString(aLevelString, 616 - g->GetFont()->StringWidth(aLevelString)/2, 471);

	// Bag slots: banked bags (all pulse together while mBagFlash != 0), then empty holders
	for (int i = 0; i < mBagsNeeded; i++)
	{
		Point aPos;
		GetBagSlotPos(&aPos, i);
		if (i < mBagsBanked)
		{
			g->DrawImage(mApp->mImages[IMAGE_BAG], aPos.mX - 2, aPos.mY - 8, Rect(99, 0, 33, 36));
			if (mBagFlash != 0.0)
			{
				int aLevel = (int)((1.0 - fabs(mBagFlash - 0.5) * 2) * 255.0);
				g->SetColorizeImages(true);
				g->SetColor(Color(aLevel, aLevel, aLevel));
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->DrawImage(mApp->mImages[IMAGE_GLOWBAG], aPos.mX - 17, aPos.mY - 17);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColorizeImages(false);
			}
		}
		else
		{
			Image* aHolder = mApp->mImages[IMAGE_BAGHOLDER];
			g->DrawImage(aHolder, aPos.mX + (29 - aHolder->GetWidth())/2, aPos.mY + (29 - aHolder->GetHeight())/2);
		}
	}

	// Everything below only when the pause fade is exactly 0
	if (mPauseFade != 0.0)
		return;

	if (mLevelUpActive)
	{
		Image* aLevelUp = mApp->mImages[IMAGE_LEVELUP];
		if (mLevelUpBouncing)
		{
			// Rotated about the image's integer centre (w/2, h/2), as in 1.2
			g->DrawImageRotated(aLevelUp, 238, (int)mLevelUpY, mLevelUpAngle, NULL);
		}
		else
		{
			// Squash: row r of mLevelUpImage is source row (int)(r*h / mLevelUpStretch); rows whose
			// source is past the bottom are not written and keep what earlier frames left there
			// (mLevelUpImage is persistent; zero when new).
			MemoryImage* aSrcImage = dynamic_cast<MemoryImage*>(aLevelUp);
			int aHeight = aSrcImage->GetHeight();
			int aWidth = aSrcImage->GetWidth();
			if (mLevelUpImage == NULL)
			{
				mLevelUpImage = new MemoryImage(mApp);
				mLevelUpImage->Create(aWidth, aHeight);
				mLevelUpImage->SetImageMode(true, true);
				mLevelUpImage->SetVolatile(true);
			}

			ulong* aDestBits = mLevelUpImage->GetBits();
			ulong* aSrcBits = aSrcImage->GetBits();
			// port: a mod's HD sign gets the same row copies, s HD rows per 1x row (HDPixels.h)
			HDPixels aSrcHD = GetHDPixels(aSrcImage);
			HDPixels aDestHD;
			if (aSrcHD.IsValid())
				aDestHD = GetOrCreateHDPixels(mLevelUpImage, aSrcHD.mScale);
			int aDestRow = 0;
			int aSum = 0;
			for (int aRow = 0; aRow < aHeight; aRow++)
			{
				int aSrcRow = (int)(aSum / mLevelUpStretch);
				if (aSrcRow < aHeight)
				{
					ulong* aSrc = aSrcBits + aSrcRow*aWidth;
					for (int x = 0; x < aWidth; x++)
						*(aDestBits++) = *(aSrc++);
					for (int s = 0; s < aDestHD.mScale; s++)
						std::copy(aSrcHD.Row(aSrcRow * aSrcHD.mScale + s), aSrcHD.Row(aSrcRow * aSrcHD.mScale + s) + aDestHD.mWidth,
							aDestHD.Row(aDestRow * aDestHD.mScale + s));
					aDestRow++;
				}
				aSum += aHeight;
			}
			mLevelUpImage->BitsChanged();
			SetHDPixels(mLevelUpImage, aDestHD);
			g->DrawImage(mLevelUpImage, 238, (int)mLevelUpY);
		}
	}

	if (mCountdownActive)
	{
		// "3", "2", "1": each digit zooms in during the first 1/8 of its second and out in the last
		Image* aCountdown = mApp->mImages[IMAGE_COUNTDOWN];
		int aWidth = aCountdown->GetWidth();
		int aCelHeight = aCountdown->GetHeight() / 3;
		int aCenterX = 380 - aWidth/2;
		int aCel = (int)mCountdownValue;
		double aScale = (0.5 - fabs(0.5 - (mCountdownValue - aCel))) * 8.0;
		if (!(aScale < 1.0))
			aScale = 1.0;
		int aDestWidth = (int)(aWidth * aScale) | 1;
		int aDestHeight = (int)(aCelHeight * aScale) | 1;
		g->SetFastStretch(true);
		g->DrawImage(aCountdown, Rect((aWidth - aDestWidth)/2 + aCenterX, (aCelHeight - aDestHeight)/2 + 124, aDestWidth, aDestHeight),
			Rect(0, aCel*aCelHeight, aWidth, aCelHeight));
		g->SetFastStretch(false);
	}

	if (mMoveDigitActive)
	{
		// Strategy: moves-left digit, same zoom on mMoveDigitFade (1 -> 0)
		Image* aCountdown = mApp->mImages[IMAGE_COUNTDOWN];
		int aWidth = aCountdown->GetWidth();
		int aCelHeight = aCountdown->GetHeight() / 3;
		double aScale = (0.5 - fabs(mMoveDigitFade - 0.5)) * 8.0;
		int aCenterX = 380 - aWidth/2;
		if (!(aScale < 1.0))
			aScale = 1.0;
		int aDestWidth = (int)(aWidth * aScale) | 1;
		int aDestHeight = (int)(aCelHeight * aScale) | 1;
		g->SetFastStretch(true);
		g->DrawImage(aCountdown, Rect((aWidth - aDestWidth)/2 + aCenterX, (aCelHeight - aDestHeight)/2 + 124, aDestWidth, aDestHeight),
			Rect(0, mMoveDigit*aCelHeight, aWidth, aCelHeight));
		g->SetFastStretch(false);
	}

	if (mBannerVisible)
	{
		const char* aPlural = (mBagsNeeded > 1) ? "s" : "";
		char aBuffer[1024];
		// port: snprintf (the original sprintf into a stack buffer)
		snprintf(aBuffer, sizeof(aBuffer), mApp->GetString("COLLECT_X_MONEY_BAGS_TO_ADVANCE").c_str(), mBagsNeeded, aPlural);
		g->SetFont(mApp->mFont1);
		g->SetColor(Color(255, 255, 255));
		SexyString aString = aBuffer;
		int aX = 380 - g->GetFont()->StringWidth(aString)/2;
		g->DrawString(aString, aX, 180);
	}

	// Particles; the original tests mBright but both of its paths draw the same thing.
	for (std::list<Particle>::iterator anItr = mParticles.begin(); anItr != mParticles.end(); ++anItr)
	{
		Particle& aParticle = *anItr;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[IMAGE_SPARKLE], (int)(aParticle.mX - 10.0), (int)(aParticle.mY - 10.0), Rect(aParticle.mFrame*20, 0, 20, 20));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	if (mHintVisible)
	{
		// Arrow cell and the offsets of the arrow and of the text row from the hint point
		int anArrowCel, anArrowX, anArrowY, aTextX, aTextY;
		if (mHintY < 100)
		{
			anArrowCel = 1; anArrowX = -22; anArrowY = -6; aTextX = -100; aTextY = 38;      // up
		}
		else if (mHintX < 254)
		{
			anArrowCel = 4; anArrowX = -2; anArrowY = -24; aTextX = 11; aTextY = -25;       // left
		}
		else if (mHintX > 506)
		{
			anArrowCel = 2; anArrowX = -48; anArrowY = -24; aTextX = -208; aTextY = -25;    // right
		}
		else if (mHintAlt)
		{
			anArrowCel = 0; anArrowX = -40; anArrowY = -40; aTextX = -100; aTextY = -85;    // bent
		}
		else
		{
			anArrowCel = 3; anArrowX = -22; anArrowY = -47; aTextX = -100; aTextY = -92;    // down
		}

		g->DrawImage(mApp->mImages[IMAGE_INSTRUCTOARROWS], mHintX + anArrowX, mHintY + anArrowY, Rect(0, anArrowCel*50, 50, 50));
		g->DrawImage(mApp->mImages[IMAGE_INSTRUCTOTEXT], mHintX + aTextX, mHintY + aTextY, Rect(0, mHintId*50, 200, 50));
	}

	if (mState == STATE_GAME_OVER_SCORES)
		g->DrawImage(mApp->mImages[IMAGE_GAMEOVER], 221, (int)(mHighScoreSlide * 1.5));

	if ((mState == STATE_GAME_OVER || mState == STATE_GAME_OVER_SCORES) && mApp->mGameType == GAMETYPE_PUZZLE)
		g->DrawImage(mApp->mImages[IMAGE_NOMOREMOVES], 216, mNoMovesY + 80);

	if (mState == STATE_GAME_OVER_SCORES || mState == STATE_LEVEL_DONE)
	{
		if (mStatsImage == NULL)
		{
			// The panel is rendered once (ShowContinueButton deletes it for the next one)
			mStatsImage = new MemoryImage(mApp);
			mStatsImage->Create(396, 263);
			mStatsImage->SetImageMode(true, true);
			Graphics aStatsG(mStatsImage);
			DrawStatsPanel(&aStatsG);

			mStatsFadeImage = new MemoryImage(mApp);
			mStatsFadeImage->Create(396, 263);
			mStatsFadeImage->SetImageMode(true, true);
			mStatsFadeImage->SetVolatile(true);
		}

		if (mStatsDirty)
		{
			// Fade-in with a settling horizontal sine wave. Shifted rows leave |offset| pixels
			// unwritten, which keep the previous frame's values (mStatsFadeImage is persistent).
			ulong* aSrcBits = mStatsImage->GetBits();
			ulong* aDestBits = mStatsFadeImage->GetBits();
			int anAlpha = (int)(mStatsFade * 256.0);
			// port: the panel's HD pixels (drawn from a mod's HD art) get the same shifts, scaled (HDPixels.h)
			HDPixels aSrcHD = GetHDPixels(mStatsImage);
			HDPixels aDestHD;
			if (aSrcHD.IsValid())
				aDestHD = GetOrCreateHDPixels(mStatsFadeImage, aSrcHD.mScale);
			for (int y = 0; y < mStatsImage->GetHeight(); y++)
			{
				ulong* aSrc = aSrcBits + mStatsImage->GetWidth() * y;
				ulong* aDest = aDestBits + mStatsFadeImage->GetWidth() * y;
				int aCount = mStatsImage->GetWidth();
				int anOffset = (int)(sin(y * 0.08 + mStatsFade * 8.0) * (1.0 - mStatsFade) * 64.0); // fsin
				if (anOffset < 0)
				{
					aSrc += -anOffset;
					aCount += anOffset;
				}
				else
				{
					aDest += anOffset;
					aCount -= anOffset;
				}

				for (int i = 0; i < aCount; i++)
				{
					ulong aPixel = *(aSrc++);
					*(aDest++) = (((((aPixel >> 24) & 0xff) * anAlpha) >> 8) << 24) | (aPixel & 0xffffff);
				}

				for (int s = 0; s < aDestHD.mScale; s++)
				{
					int aScale = aDestHD.mScale;
					const uint32_t* aSrcHDRow = aSrcHD.Row(y * aScale + s);
					uint32_t* aDestHDRow = aDestHD.Row(y * aScale + s);
					int aCountHD = aCount * aScale;
					if (anOffset < 0)
						aSrcHDRow += -anOffset * aScale;
					else
						aDestHDRow += anOffset * aScale;
					for (int i = 0; i < aCountHD; i++)
					{
						uint32_t aPixel = aSrcHDRow[i];
						aDestHDRow[i] = (((((aPixel >> 24) & 0xff) * anAlpha) >> 8) << 24) | (aPixel & 0xffffff);
					}
				}
			}
			mStatsFadeImage->BitsChanged();
			SetHDPixels(mStatsFadeImage, aDestHD);
			mStatsDirty = false;
		}

		if (mHighScoreSlide == 0)
			g->DrawImage(mStatsFadeImage, 182, 162);
		else
			g->DrawImage(mStatsFadeImage, 182, mHighScoreSlide + 162);
	}

	// port(mods): while a replay plays, a line over the coins says so, with its speed and keys (Replay.h)
	if (mPlayer != NULL)
	{
		SexyString aText = mApp->GetString("REPLAY", _S("REPLAY")) + StrFormat(_S(" %dX"), mApp->mWatchSpeed);
		if (mPauseCount > 0)
			aText += _S(" ") + mApp->GetString("REPLAY_PAUSED", _S("PAUSED"));
		SexyString aKeys = mApp->GetString("REPLAY_KEYS", _S("KEYS 1 TO 4 SPEED, SPACE PAUSE, ESC STOP"));
		Font* aFont = mApp->mFont3;
		g->SetFont(aFont);
		g->SetColor(Color(0, 0, 0, 160));
		g->FillRect(BOARD_X, BOARD_Y, BOARD_COLS * COIN_SIZE, 44);
		g->SetColor(Color(0, 255, 0));
		g->DrawString(aText, BOARD_X + (BOARD_COLS * COIN_SIZE - aFont->StringWidth(aText)) / 2, BOARD_Y + 18);
		g->SetColor(Color(0, 170, 0));
		g->DrawString(aKeys, BOARD_X + (BOARD_COLS * COIN_SIZE - aFont->StringWidth(aKeys)) / 2, BOARD_Y + 37);
	}

	DrawSolverStatus(g); // port(mods)
}

// port(mods): the Puzzle solver's status line (Board::GetSolverStatusRect; PuzzleSolver.h)
void Board::DrawSolverStatus(Graphics* g)
{
	Rect aRect;
	SexyString aStatus;
	SexyString aHint;
	if (!GetSolverStatusRect(&aRect, &aStatus, &aHint))
		return;
	if (mPlayer != NULL)
		aRect.mY += 44; // under the replay's lines
	Font* aFont = mApp->mFont3;
	g->SetFont(aFont);
	g->SetColor(Color(0, 0, 0, 160));
	g->FillRect(aRect);
	int aY = aRect.mY + 18;
	if (!aStatus.empty())
	{
		bool isLost = (mSolver->mStatus == PuzzleSolver::STATUS_LOST);
		g->SetColor(isLost ? Color(255, 64, 64) : Color(0, 255, 0));
		g->DrawString(aStatus, aRect.mX + (aRect.mWidth - aFont->StringWidth(aStatus)) / 2, aY);
		aY += 19;
	}
	if (!aHint.empty())
	{
		g->SetColor(Color(0, 170, 0));
		g->DrawString(aHint, aRect.mX + (aRect.mWidth - aFont->StringWidth(aHint)) / 2, aY);
	}
}

// 0x402060
// theYOffset: 0, or mMintPixels for the row being minted below the grid.
void Board::DrawPiece(Graphics* g, Piece* thePiece, int theYOffset)
{
	if (thePiece->mBlinkHidden)
		return;

	if (thePiece->mIsSpecial)
	{
		if (thePiece->mSpecialType == PIECE_MONEYBAG)
		{
			Image* aBag = mApp->mImages[IMAGE_BAG];
			int aCelWidth = aBag->GetWidth() / 4;
			int aCelHeight = aBag->GetHeight();
			int aY = (int)((29 - aCelHeight)/2 + thePiece->mShakeY + theYOffset + thePiece->mY);
			int aX = (int)((29 - aCelWidth)/2 + thePiece->mShakeX + thePiece->mX);
			g->DrawImage(aBag, aX, aY, Rect(thePiece->mFrame*aCelWidth, 0, aCelWidth, aCelHeight));

			if (mApp->mGameType != GAMETYPE_PUZZLE)
			{
				// The glow ignores the shake and theYOffset
				int aLevel = (int)(fabs(mBagGlow - 0.5) * 510.0);
				g->SetColorizeImages(true);
				g->SetColor(Color(aLevel, aLevel, aLevel));
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->DrawImage(mApp->mImages[IMAGE_GLOWBAG], (int)(thePiece->mX - 17.0), (int)(thePiece->mY - 12.0));
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColorizeImages(false);
			}
		}
		else if (thePiece->mSpecialType == PIECE_DOLLAR_BOMB)
		{
			Image* aDollar = mApp->mImages[IMAGE_DOLLAR];
			int aCelWidth = aDollar->GetWidth() / 32;
			int aCelHeight = aDollar->GetHeight();
			g->SetColorizeImages(true);
			g->SetColor(CoinColor(gCoinColors, thePiece->mColor));
			int aY = (int)((29 - aCelHeight)/2 + thePiece->mShakeY + thePiece->mY + theYOffset);
			int aX = (int)((29 - aCelWidth)/2 + thePiece->mShakeX + thePiece->mX);
			g->DrawImage(aDollar, aX, aY, Rect(thePiece->mFrame*aCelWidth, 0, aCelWidth, aCelHeight));
			g->SetColorizeImages(false);
		}
		else if (thePiece->mSpecialType == PIECE_EXPLODING)
		{
			Image* anExplosion = mApp->mImages[IMAGE_EXPLOSION];
			int aCelWidth = anExplosion->GetWidth() / 15;
			int aCelHeight = anExplosion->GetHeight();
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			int aY = (int)((29 - aCelHeight)/2 + thePiece->mShakeY + thePiece->mY + theYOffset);
			int aX = (int)((29 - aCelWidth)/2 + thePiece->mShakeX + thePiece->mX);
			g->DrawImage(anExplosion, aX, aY, Rect(thePiece->mFrame*aCelWidth, 0, aCelWidth, aCelHeight));
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
		else // PIECE_POOF (and any other type)
		{
			Image* aSparkle = mApp->mImages[IMAGE_SPARKLE];
			int aCelWidth = aSparkle->GetWidth() / 14;
			int aCelHeight = aSparkle->GetHeight();
			int aY = (int)((29 - aCelHeight)/2 + thePiece->mShakeY + thePiece->mY + theYOffset);
			int aX = (int)((29 - aCelWidth)/2 + thePiece->mShakeX + thePiece->mX);
			g->DrawImage(aSparkle, aX, aY, Rect(thePiece->mFrame*aCelWidth, 0, aCelWidth, aCelHeight));
		}
		return;
	}

	Image* aCoins = mApp->mImages[IMAGE_COINS];
	if (thePiece->mImageSet == 1)
		aCoins = mApp->mImages[IMAGE_HORZCOINS];
	else if (thePiece->mHighlighted || thePiece->mDanger)
		aCoins = mApp->mBrightCoinsImage;
	else if ((IsSolverHintPiece(thePiece)) && ((mUpdateCnt / 25) % 2 == 0)) // port(mods): the solver's hint blinks
		aCoins = mApp->mBrightCoinsImage;

	int aY = (int)(thePiece->mShakeY + thePiece->mY + theYOffset);
	int aX = (int)(thePiece->mShakeX + thePiece->mX);
	g->DrawImage(aCoins, aX, aY, Rect(thePiece->mFrame*29, thePiece->mColor*29, 29, 29));
}

// 0x401e30
// Bag slot i of the HUD: rows of 3 (4 when more than 6 bags are needed), 37 px apart, each row centred.
void Board::GetBagSlotPos(Point* thePos, int theIndex)
{
	int aPerRow = (mBagsNeeded > 6) ? 4 : 3;
	int aRow = theIndex / aPerRow;
	int aCol = theIndex % aPerRow;
	int aRowCount = mBagsNeeded - aRow*aPerRow;
	if (aRowCount >= aPerRow)
		aRowCount = aPerRow;
	thePos->mX = aCol*37 - (aRowCount*37 - 4)/2 + 85;
	thePos->mY = aRow*40 + 232;
}

// 0x404950
// The level / game stats and rank panel, drawn once into mStatsImage (396x263).
void Board::DrawStatsPanel(Graphics* g)
{
	bool isGameOver = (mState == STATE_GAME_OVER_SCORES);

	g->DrawImageBox(Rect(0, 0, 396, 263), mApp->mImages[IMAGE_OLDDIALOG]);
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(255, 163, 0));
	// Both arms of the original's isGameOver test fetch GAME_TIME.
	g->DrawString(mApp->GetString("GAME_TIME"), 46, 50);
	g->DrawString(mApp->GetString("BIGGEST_COIN_COMBO"), 46, 72);
	g->DrawString(mApp->GetString("BIGGEST_BONUS"), 46, 94);
	g->DrawString(mApp->GetString("COINS_CLEARED"), 46, 116);

	Font* aFont = g->GetFont();
	g->SetColor(Color(255, 70, 39));
	SexyString aString = StrFormat("%d min %02d sec", mStatsSeconds / 60, mStatsSeconds % 60);
	g->DrawString(aString, 355 - aFont->StringWidth(aString), 50);
	aString = StrFormat("%d", isGameOver ? mGameBiggestCombo : mLevelBiggestCombo);
	g->DrawString(aString, 355 - aFont->StringWidth(aString), 72);
	aString = StrFormat("%d", isGameOver ? mGameBiggestBonus : mLevelBiggestBonus);
	g->DrawString(aString, 355 - aFont->StringWidth(aString), 94);
	aString = StrFormat("%d", isGameOver ? mGameCoinsCleared : mLevelCoinsCleared);
	g->DrawString(aString, 355 - aFont->StringWidth(aString), 116);

	// Rank: the highest threshold the score reaches; aNextRank = the threshold above it (0 at the top)
	int aNextRank = 0;
	int aRank = 0;
	for (int i = 26; i >= 0; i--)
	{
		if (mScore >= gRankThresholds[i])
		{
			aRank = i;
			break;
		}
		aNextRank = gRankThresholds[i];
	}

	g->DrawImage(mApp->mImages[IMAGE_RANKSTRIP], 53, 125, Rect(0, aRank*100, 100, 100));
	g->SetFont(mApp->mFont3);
	g->SetColor(Color(0, 212, 15));
	// port: guarded index (the original reads gRankNames[aRank] unchecked; the string table has 27)
	g->DrawString(aRank < (int)gRankNames.size() ? gRankNames[aRank] : SexyString(), 168, 167);

	if (aNextRank != 0)
	{
		g->SetColor(Color(0, 141, 0));
		char aBuffer[1024];
		// port: snprintf (the original sprintf into a stack buffer)
		snprintf(aBuffer, sizeof(aBuffer), mApp->GetString("NEXT_RANK_AT_X_PTS").c_str(), aNextRank);
		g->DrawString(aBuffer, 168, 195);
	}

	// port(mods): a game on a seed the player chose shows it (ModRules.h); "SEED" can be changed by a
	// strings mod. Unseeded games draw nothing here, as the original.
	if (mUserSeeded)
	{
		g->SetColor(Color(0, 141, 0));
		g->DrawString(mApp->GetString("BOARD_SEED", _S("SEED")) + StrFormat(_S(" %u"), (unsigned) mGameSeed), 168, 223);
	}

	// port(mods): the game-over panel of a recorded game says how to watch it, on its top edge (the high
	// scores cover its bottom) (Replay.h)
	if ((isGameOver) && (!mReplayPath.empty()) && (mPlayer == NULL))
	{
		SexyString aHint = mApp->GetString("REPLAY_PRESS_R", _S("PRESS R TO WATCH THIS GAME"));
		g->SetColor(Color(0, 212, 15));
		g->DrawString(aHint, (396 - g->GetFont()->StringWidth(aHint)) / 2, 33);
	}
}

// 0x4045e0
// End of a level (state 1) or of the game (state 6): snapshot the game time, add the
// "CLICK HERE TO PLAY ..." button and reset the panel images so Draw renders a fresh panel.
void Board::ShowContinueButton()
{
	mStatsSeconds = (timeGetTime() - mTimerTick) / 1000 + mPlaySeconds;

	mContinueButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], 0, this);
	mContinueButton->SetFont(mApp->mFont3);
	mContinueButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0, 255, 0));
	if ((mState == STATE_GAME_OVER_SCORES) && (mPlayer != NULL)) // port(mods): the end of a replay (Replay.h)
		mContinueButton->mLabel = mApp->GetString("REPLAY_END", _S("END OF REPLAY. CLICK HERE TO GO ON"));
	else if (mState == STATE_GAME_OVER_SCORES)
		mContinueButton->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY_AGAIN");
	else
		mContinueButton->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY_NEXT_LEVEL");
	mContinueButton->Resize(197, 444, 366, 27);
	mContinueButton->mMouseInsets = Insets(-10, -10, -10, -10);
	mContinueButton->mDoFinger = true;
	mWidgetManager->AddWidget(mContinueButton);

	mButtonBlink = 0;
	delete mStatsImage;
	mStatsImage = NULL;
	delete mStatsFadeImage;
	mStatsFadeImage = NULL;
	delete mLevelUpImage;
	mLevelUpImage = NULL;
	mStatsFade = 0.0;
	mStatsDirty = true;
	MarkDirty();
}
