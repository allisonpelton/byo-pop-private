// The tutorial: the wizard explains the rules step by step on a board that is set up for each step (see Board.h).

#include "Board.h"
#include "WinAlchApp.h"
#include "Piece.h"
#include "ButtonWidget.h"
#include "DialogButton.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"
#include "Compat2004.h"

using namespace Sexy;

// 0x402fcf: the next step. The odd steps after an action explain what happened and wait for the "next" button; the
// others set up a position and let the player try.
void Board::NextTutorial()
{
	mTutorialStep++;
	mAnimateLights = false;
	if (mButton != NULL)
	{
		if (mWidgetManager != NULL)
			mWidgetManager->RemoveWidget(mButton);
		mApp->SafeDeleteWidget(mButton);
		mButton = NULL;
	}

	Graphics g(mBgImage);
	g.DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(0x96, 0, 0x1ea, 0x1e0));

	SexyString aText;
	bool needButton = false;
	switch (mTutorialStep)
	{
	case 0:
		aText = mApp->GetString("TUTORIAL_FIRST_PIECE");
		{
			Piece* aPiece = new Piece(mApp);
			aPiece->mSkull = true;
			PutPiece(aPiece, 4, 5);
		}
		break;
	case 1:
		aText = mApp->GetString("TUTORIAL_FIRST_PIECE_DONE");
		needButton = true;
		break;
	case 2:
		aText = mApp->GetString("TUTORIAL_MORE_PIECES");
		break;
	case 3:
		aText = mApp->GetString("TUTORIAL_MORE_PIECES_DONE");
		needButton = true;
		break;
	case 4:
		{
			aText = mApp->GetString("TUTORIAL_CLEAR_ROW");
			ResetBoard();
			static const int aColors[8] = {3, 3, 3, 4, 4, 5, 5, 5};
			static const int aShapes[8] = {4, 2, 3, 3, 4, 4, 5, 6};
			for (int i = 0; i < 8; i++)
			{
				Piece* aPiece = new Piece(mApp);
				aPiece->mColor = aColors[i];
				aPiece->mShape = aShapes[i];
				PutPiece(aPiece, i, 5);
			}
		}
		break;
	case 5:
		aText = mApp->GetString("TUTORIAL_CLEAR_ROW_DONE");
		needButton = true;
		break;
	case 6:
		{
			aText = mApp->GetString("TUTORIAL_USE_FORGE");
			ResetBoard();
			Piece* aPiece = new Piece(mApp);
			aPiece->mColor = 2;
			aPiece->mShape = 2;
			PutPiece(aPiece, 4, 6);
		}
		break;
	case 7:
		aText = mApp->GetString("TUTORIAL_COOL_FORGE");
		break;
	case 8:
		aText = mApp->GetString("TUTORIAL_FORGE_DONE");
		needButton = true;
		break;
	case 9:
		{
			aText = mApp->GetString("TUTORIAL_BIG_MATCH");
			ResetBoard();
			// a block of 5 x 4 pieces (-1: none) in the rows 4 to 7, and the tiles of row 6 already lit
			static const int aColors[20] = {1, -1, 4, 2, 0, 0, 3, 3, 5, -1, -1, 1, -1, 3, 2, 2, 1, -1, 6, 4};
			static const int aShapes[20] = {0, -1, 1, 2, 6, 2, 2, 0, 7, -1, -1, 5, -1, 5, 4, 4, 2, -1, 3, 3};
			for (int k = 0; k < 20; k++)
			{
				if (aShapes[k] == -1)
					continue;
				Piece* aPiece = new Piece(mApp);
				aPiece->mShape = aShapes[k];
				aPiece->mColor = aColors[k];
				PutPiece(aPiece, 2 + k % 5, 4 + k / 5);
			}
			for (int aCol = 0; aCol < COLS; aCol++)
				mLit[6][aCol] = true;
		}
		break;
	case 10:
		aText = mApp->GetString("TUTORIAL_BIG_MATCH_DONE");
		needButton = true;
		break;
	case 0xb:
		aText = mApp->GetString("TUTORIAL_FOURWAY");
		break;
	case 0xc:
		aText = mApp->GetString("TUTORIAL_BOMB");
		break;
	case 0xd:
		aText = mApp->GetString("TUTORIAL_BOMB_DONE");
		needButton = true;
		break;
	case 0xe:
		{
			aText = mApp->GetString("TUTORIAL_DONE");
			ResetBoard();
			DialogButton* aButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 1, this);
			mButton = aButton;
			aButton->SetFont(mApp->mFont1);
			aButton->Resize(mX + 0x20, mY + mHeight - 0x50, mWidth - 0x40, 0x20);
			aButton->mLabel = mApp->GetString("CLICK_HERE_TO_START_PLAYING");
			aButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xe4, 0x37));
			mWidgetManager->AddWidget(aButton);
			mWidgetManager->PutInfront(aButton, this);
		}
		break;
	}

	// the wizard's words in a box at the top
	g.SetFont(mApp->mFont1);
	int aTextHeight = GetWordWrappedHeight2004(&g, mWidth - 0xc0, aText, -1);
	g.SetColor(Color(0, 0, 0, 0x80));
	g.FillRect(0x30, 0x3c, mWidth - 0x4c, aTextHeight + 8);
	g.SetColor(Color(0, 0, 0));
	g.DrawRect(0x30, 0x3c, mWidth - 0x4d, aTextHeight + 7);
	g.SetColor(Color(0xff, 0xdc, 0x96));
	WriteWordWrapped2004(&g, Rect(0xa0, 0x40, mWidth - 0xc0, 0), aText, -1, -1);
	g.DrawImage(mApp->mImages[IMAGE_WIZARDHEAD], -4, 0);

	mTutorialCount = 0;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			if (mGrid[aRow][aCol] != NULL || mLit[aRow][aCol])
				RedrawBackground(aCol, aRow);

	if (needButton)
	{
		ButtonWidget* aButton = new ButtonWidget(2, this);
		mButton = aButton;
		aButton->Resize(0x1ef, 0x18a, 0x72, 0x2b);
		aButton->mButtonImage = mApp->mImages[IMAGE_NEXT];
		aButton->mDoFinger = true;
		mWidgetManager->AddWidget(aButton);
		mWidgetManager->PutInfront(aButton, this);
	}

	if (mTutorialStep == 0xe || needButton)
	{
		if (mCurPiece != NULL)
		{
			mWidgetManager->RemoveWidget(mCurPiece);
			delete mCurPiece;
		}
		mCurPiece = NULL;
	}
	else
		NextMove();
	mTutorialWaiting = needButton;
	mAnimateLights = true;
}
