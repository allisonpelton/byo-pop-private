// The board's drawing and the statistics panel of a cleared board (see Board.h).

#include "Board.h"
#include "WinAlchApp.h"
#include "Piece.h"
#include "Wizard.h"
#include "BoardOverlay.h"
#include "Sidebar.h"
#include "DialogButton.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "Font.h"
#include "ImageFont.h"
#include "WidgetManager.h"
#include <stdio.h>
#include <string>

using namespace Sexy;

// Part of Board::Draw (0x404923): a flash of light over a cell, as bright as the animation is far along (0..100; the full
// colour at 75)
static Color FlashColor(int thePercent)
{
	int aRed = (thePercent * 0xff) / 0x4b;
	if (aRed > 0xfe)
		aRed = 0xff;
	int aGreen = (thePercent * 200) / 0x4b;
	if (aGreen > 0xfe)
		aGreen = 0xff;
	int aBlue = (thePercent << 6) / 0x4b;
	if (aBlue > 0xfe)
		aBlue = 0xff;
	return Color(aRed, aGreen, aBlue);
}

// 0x404923
void Board::Draw(Graphics* g)
{
	if (mCoverCount < 1)
		g->DrawImage(mBgImage, 0, 0);
	else
		g->DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(WinAlchApp::BOARD_X, 0, WinAlchApp::BOARD_WIDTH, WinAlchApp::SCREEN_HEIGHT));

	// the level bar: a gem for each board cleared, the second row of gems after the seventh
	if (mBoardsCleared < 8)
		g->DrawImage(mApp->mImages[IMAGE_GEMS1], 0x15, 8, Rect(0, 0, (mBoardsCleared * 0x1bd) / 7, 0x1d));
	else
	{
		g->DrawImage(mApp->mImages[IMAGE_GEMS1], 0x15, 8);
		int aGems = mBoardsCleared - 7;
		if (aGems > 6)
			aGems = 7;
		g->DrawImage(mApp->mImages[IMAGE_GEMS2], 0x15, 8, Rect(0, 0, (aGems * 0x1bd) / 7, 0x1d));
	}

	char aBuffer[32];
	g->SetFont(mApp->mLevelFont);
	g->SetColor(Color(0, 0, 0));
	sprintf(aBuffer, "%d", mBoardsCleared + 1);
	g->DrawString(aBuffer, 0xfc, 0x1d5);

	// paused (time trial): the board is covered
	if (mCoverCount != 0)
	{
		if (mApp->mDialogMap.empty())
		{
			g->SetFont(mApp->mAFont1);
			g->SetColor(Color(255, 255, 255));
			int aMidX = mWidth / 2;
			int aMidY = mHeight / 2;

			SexyString aText = mApp->GetString("GAME_PAUSED");
			g->DrawString(aText, aMidX - g->GetFont()->StringWidth(aText) / 2, aMidY - g->GetFont()->GetHeight() / 2);
			aText = mApp->GetString("CLICK_HERE_TO_CONTINUE");
			g->DrawString(aText, aMidX - g->GetFont()->StringWidth(aText) / 2, g->GetFont()->GetHeight() + aMidY);
		}
		return;
	}

	// the game is over: the pieces shrink
	if (mState == STATE_ENDING)
	{
		for (int aRow = 0; aRow < ROWS; aRow++)
			for (int aCol = 0; aCol < COLS; aCol++)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == NULL)
					continue;

				int aX = aPiece->mX - mX;
				int aY = aPiece->mY - mY;
				if (!aPiece->mSkull)
				{
					g->SetColorizeImages(true);
					g->SetColor(Color(gPieceColors[aPiece->mColor][0], gPieceColors[aPiece->mColor][1],
						gPieceColors[aPiece->mColor][2]));
					g->DrawImage(mApp->mShapeImages[aPiece->mShrink][aPiece->mShape], aX, aY);
					g->SetColorizeImages(false);
				}
				else
					g->DrawImage(mApp->mShapeImages[aPiece->mShrink][22], aX, aY);
			}
	}

	bool isPieceShown = mCurPiece != NULL && mCurPiece->mVisible && mState == STATE_PLAYING;

	// the hint: every cell the piece fits in glows
	if (mCurPiece != NULL && !mCurPiece->mBomb && mHintOn)
	{
		for (int aRow = 0; aRow < ROWS; aRow++)
			for (int aCol = 0; aCol < COLS; aCol++)
				if (mValid[aRow][aCol])
				{
					g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					g->SetColor(Color(0x18, 0x18, 0x18));
					g->FillRect(GetColX(aCol), GetRowY(aRow), CELL, CELL);
					g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				}
	}

	// the cell the piece is over
	if (mHoverX != -1 && isPieceShown)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(0x18, 0x18, 0x18));
		g->FillRect(GetColX(mHoverX), GetRowY(mHoverY), CELL, CELL);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	if (mState == STATE_CLEARING)
	{
		// the full column and row light up
		for (int i = 0; i < 2; i++)
		{
			Rect aRect;
			if (i == 0)
			{
				if (mClearCol == -1)
					continue;
				aRect = Rect(GetColX(mClearCol), GetRowY(0), CELL, ROWS * CELL);
			}
			else
			{
				if (mClearRow == -1)
					continue;
				aRect = Rect(GetColX(0), GetRowY(mClearRow), COLS * CELL, CELL);
			}
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(FlashColor(mAnimPct));
			g->FillRect(aRect.mX, aRect.mY, aRect.mWidth, aRect.mHeight);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
	else if (mState == STATE_WON || mState == STATE_NEXT_BOARD)
	{
		// the cells that go flash
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(FlashColor(mAnimPct));
		for (int aRow = 0; aRow < ROWS; aRow++)
			for (int aCol = 0; aCol < COLS; aCol++)
				if (mFlash[aRow][aCol])
					g->FillRect(GetColX(aCol), GetRowY(aRow), CELL, CELL);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	// the tile that is lighting up, with its piece on top (the piece's colour pulses toward white)
	if (mLightOn)
	{
		g->DrawImage(mLightOut, GetColX(mLightCol), GetRowY(mLightRow));
		Piece* aPiece = mGrid[mLightRow][mLightCol];
		if (aPiece != NULL)
		{
			if (!aPiece->mSkull)
			{
				int aBlue = (int) (gPieceColors[aPiece->mColor][2] * (1.0 - mLightMix) + mLightMix * 255.0);
				int aGreen = (int) (gPieceColors[aPiece->mColor][1] * (1.0 - mLightMix) + mLightMix * 255.0);
				int aRed = (int) (gPieceColors[aPiece->mColor][0] * (1.0 - mLightMix) + mLightMix * 255.0);
				g->SetColorizeImages(true);
				g->SetColor(Color(aRed, aGreen, aBlue));
				g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], GetColX(mLightCol) + 1, GetRowY(mLightRow) + 1,
					Rect(aPiece->mShape * WinAlchApp::SYMBOL_SIZE, 0, WinAlchApp::SYMBOL_SIZE, WinAlchApp::SYMBOL_SIZE));
				g->SetColorizeImages(false);
			}
			else
				g->DrawImage(mApp->mImages[IMAGE_NULLBLOCK], GetColX(mLightCol), GetRowY(mLightRow));
		}
	}

	// the explosion of a bomb
	if (mBombOn)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[IMAGE_BIGEXPLOSION], mBombX, mBombY, Rect(mBombFrame * 0x47, 0, 0x47, 100));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	if (mState == STATE_NEXT_BOARD)
	{
		// the next board's tiles fade in
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, (int) (mNextBoardFade * 255.0)));
		int aLevel = (mBoardsCleared + 1) % 9;
		if (aLevel > 0)
			g->DrawImage(mApp->mImages[IMAGE_LEVEL2 + aLevel - 1], GRID_X, GRID_Y);
		else
			g->DrawImage(mApp->mImages[IMAGE_BACKDROP], GRID_X, GRID_Y, Rect(0xa0, 0x27, 0x1d4, 0x1a0));
		g->SetColorizeImages(false);
	}
	else if (mState == STATE_GAME_OVER)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, (int) (mGameOverFade * 255.0)));
		g->DrawImage(mApp->mImages[IMAGE_GAMEOVER], 0x72, 0xa4);
		g->SetColorizeImages(false);
	}
}

// 0x40400d: the board is cleared: the statistics are written on the board, and a button starts the next one. The text
// has a black outline of 24 copies around it.
void Board::ShowBoardComplete()
{
	mState = STATE_BOARD_CLEARED;
	ShowWizard();

	DialogButton* aButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 0, this);
	mButton = aButton;
	aButton->SetFont(mApp->mFont1);
	aButton->Resize(mX + 0x20, mY + mHeight - 0x50, mWidth - 0x40, 0x20);
	aButton->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY_NEXT_BOARD");
	aButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xe4, 0x37));
	mWidgetManager->AddWidget(aButton);

	Graphics g(mBgImage);
	Rect aRect(0x20, 0x5c, mWidth - 0x40, mHeight - 0x40);
	static const int anOffsets[5] = {-2, -1, 1, 2, 0};

	char aBuffer[64];
	for (int i = 0; i < 5; i++)
	{
		int aDY = anOffsets[i];
		for (int j = 0; j < 5; j++)
		{
			int aDX = anOffsets[j];
			bool isLast = (i == 4 && j == 4);

			// the title
			g.SetFont(mApp->mAFont1);
			g.SetColor(Color(0, 0, 0));
			if (isLast)
				g.SetColor(Color(0xff, 0xdc, 0x60));
			SexyString aTitle;
			if (mBoardsCleared == 0)
				aTitle = mApp->GetString("BOARD_CLEARED");
			else
			{
				sprintf(aBuffer, mApp->GetString("BOARDS_CLEARED_D").c_str(), mBoardsCleared + 1);
				aTitle = aBuffer;
			}
			g.DrawString(aTitle, (aDX - g.GetFont()->StringWidth(aTitle) / 2) + 0x118, aDY + 0xa8);

			// the statistics
			g.SetFont(mApp->mFont1);
			if (isLast)
				g.SetColor(Color(0xff, 0xa0, 0x3c));
			sprintf(aBuffer, "%2d:%02d", mTotalSeconds / 60, mTotalSeconds % 60);
			DrawColumn(&g, aRect, aDX, aDY + 0x6e, mApp->GetString("CURRENT_GAME_TIME"), aBuffer);
			sprintf(aBuffer, "%d", mBoardsCleared + 1);
			DrawColumn(&g, aRect, aDX, aDY + 0x82, mApp->GetString("BOARDS_CLEARED"), aBuffer);
			sprintf(aBuffer, "%d", mLongestRun);
			DrawColumn(&g, aRect, aDX, aDY + 0x96, mApp->GetString("LONGEST_RUN_WITHOUT_DISCARD"), aBuffer);
			sprintf(aBuffer, "%d", mScore);
			DrawColumn(&g, aRect, aDX, aDY + 0xaa, mApp->GetString("CURRENT_SCORE"), aBuffer);

			// the rank and the score of the next one
			if (isLast)
				g.SetColor(Color(0xfc, 100, 0x34));
			SexyString aRankLabel = mApp->GetString("CURRENT_RANKING");
			SexyString aNextLabel = mApp->GetString("NEXT_RANK_AT");
			DrawColumn(&g, aRect, aDX, aDY + 0xd2, aRankLabel, aNextLabel);
			int aRankLabelWidth = g.GetFont()->StringWidth(aRankLabel);
			int aNextLabelWidth = g.GetFont()->StringWidth(aNextLabel);

			if (isLast)
				g.SetColor(Color(0xfa, 0xbb, 0x66));
			int aRankPad = (aRankLabelWidth - g.GetFont()->StringWidth(mRankName)) / 2;
			if (aRankPad < 0)
				aRankPad = 0;
			g.DrawString(mRankName, aDX + 0x30 + aRankPad, g.GetFont()->GetAscent() + 0x142 + aDY);

			SexyString aNext = "-";
			if (mNextRank != 0)
			{
				sprintf(aBuffer, "%d", mNextRank);
				aNext = aBuffer;
			}
			int aNextWidth = g.GetFont()->StringWidth(aNext);
			int aNextX = ((aNextLabelWidth - aNextWidth) / 2 - aNextLabelWidth) - 0x10 + aRect.mWidth;
			int aRightX = (aRect.mWidth - aNextWidth) - 0x10;
			if (aRightX <= aNextX)
				aNextX = aRightX;
			g.DrawString(aNext, aDX + 0x20 + aNextX, g.GetFont()->GetAscent() + 0x142 + aDY);
		}
	}
}
