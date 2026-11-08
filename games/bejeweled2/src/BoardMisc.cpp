// BoardMisc.cpp: the tips that the classic games show, the start of the hyperspace jump and of the whirlpool, and the
// small functions around the end of a level.
//   CheckForHints 0x59cb4b, DoHyperspace 0x59d20d, DoWhirlpool 0x59d37a, UpdateSecretUnlocked 0x59da14,
//   EndInterfaceRestore 0x59dae4, ShowPuzzleHint 0x59db5a, ShowPlanetCount 0x59de98, CreateFontGlowImages 0x5a0c93.

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "GalaxyParser.h"
#include "EffectOverlay.h"
#include "PuzzleStateOverlay.h"
#include "Hyperspace.h"
#include "Points.h"
#include "Dialog.h"
#include "PodDialog.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include "DialogButton.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "Common.h"
#include <math.h>

using namespace Sexy;

// the dialog of a tip says what a special gem is: it sits beside the gem, below it for a gem in the upper half of the board
// and above it otherwise, and its buttons read "Don't Show Again" and "Continue"
static void PlaceHintDialog(Board* theBoard, Dialog* theDialog, Piece* thePiece)
{
	theDialog->mYesButton->mLabel = "Don't Show Again";
	theDialog->mNoButton->mLabel = "Continue";

	WinDMApp* anApp = gApp;
	int aRow = !theBoard->mFlipped ? thePiece->mRow : 7 - thePiece->mRow;
	double aPieceY = (double)anApp->ScaleNum(thePiece->mY, 0.0f);
	int aY;
	if (aRow < 4)
		aY = FTol((double)anApp->ScaleNum(100, 0) + aPieceY);
	else
		aY = FTol(aPieceY - (double)anApp->ScaleNum(300, 16));
	int aX = anApp->ScaleNum(20, 0) + theDialog->mX;
	theDialog->Move(aX, aY);
}

// 0x59cb4b: the first power gem and the first hyper cube that the player makes come with a tip, once each for the profile
// (the profile's hints 1 and 2) and once for the game (m194d, m194e); the power gem comes first
void Board::CheckForHints()
{
	if (mIsPuzzle)
		return;

	Piece* aHyperCube = NULL;
	Piece* aPowerGem = NULL;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL)
			{
				if (aPiece->mType == 9)
					aHyperCube = aPiece;
				else if (aPiece->m70)
					aPowerGem = aPiece;
			}
		}
	}

	bool aWide = mApp->mWidth == 640;
	if (aPowerGem == NULL || m194d)
	{
		if (aHyperCube != NULL && !m194e)
		{
			if (mApp->mProfile->WantsHint(2))
			{
				ClearPoints();
				m1954 = 0;
				mHintPiece = aHyperCube;
				Dialog* aDialog = mApp->DoDialog(0x27, true, "HYPER CUBE",
					"You created a HYPER CUBE by merging 5 gems of the same color.  Swap it with a gem to clear all gems of that color onscreen!",
					"", 1);
				mApp->SetupDialog(aDialog, aWide ? 500 : 600);
				PlaceHintDialog(this, aDialog, aHyperCube);
			}
			m194e = true;
		}
	}
	else
	{
		if (mApp->mProfile->WantsHint(1))
		{
			ClearPoints();
			m1954 = 0;
			mHintPiece = aPowerGem;
			const char* aText;
			if (aWide)
			{
				if (!m194c)
					aText = "You created a Power Gem by merging 4 gems of the same color.  This new gem will explode when matched, scoring extra points!";
				else
					aText = "Create Power Gems by merging 4 gems of the same color.  These new gems will explode when matched, scoring extra points!";
			}
			else
			{
				if (!m194c)
					aText = "You created a Power Gem by merging 4 gems of the same color.\nThis new gem will explode when matched, scoring extra points!";
				else
					aText = "Create Power Gems by merging 4 gems of the same color.\nThese new gems will explode when matched, scoring extra points!";
			}
			Dialog* aDialog = mApp->DoDialog(0x26, true, "POWER GEM", aText, "", 1);
			mApp->SetupDialog(aDialog, aWide ? 500 : 600);
			PlaceHintDialog(this, aDialog, aPowerGem);
		}
		m194d = true;
	}
}

// the score texts go (the original forgets them: they were never deleted)
void Board::ClearPoints()
{
	for (size_t i = 0; i < mPoints.size(); i++)
		delete mPoints[i];
	mPoints.clear();
}

// 0x59d20d: the jump through hyperspace after a level: the widget that draws it goes over the board; before the last
// planet of a galaxy the music fades out and the finale is lined up
void Board::DoHyperspace()
{
	if (mHyperspace != NULL)
	{
		mWidgetManager->RemoveWidget(mHyperspace);
		delete mHyperspace;
		mHyperspace = NULL;
	}
	if (mIsPuzzle && !mIsSecret)
	{
		WinDMApp* anApp = mApp;
		if (m15f6c == mApp->mGalaxyParser->GetPlanetCount() - 1)
		{
			m191c = true;
			anApp->mMusicInterface->FadeOut(0, true, 0.003);
			mApp->mPendingMusicOffset = mApp->GetInteger("MUSICOFFSET_(finale_name)");
			mApp->mPendingMusicDelay = mApp->GetInteger("MUSIC_FINALE_DELAY");
		}
	}
	mHyperspace = new Hyperspace(mApp);
	mHyperspace->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHyperspace);
}

// 0x59d37a: the board is swept into the whirlpool at the end of a level. With 3D acceleration the screen is a 48 by 48
// mesh of points around the middle of the 1024 by 768 picture (u and v are the place in the picture, the angle and the
// distance are those from the middle) that UpdateWhirlpool turns; without it the hyperspace jump starts at once
void Board::DoWhirlpool()
{
	KillTutorialButtons();
	bool a3D = mApp->Is3DAccelerated();
	mApp->mProfile->DisableHint(0);
	if (mHyperspace != NULL)
	{
		mWidgetManager->RemoveWidget(mHyperspace);
		delete mHyperspace;
		mHyperspace = NULL;
	}
	if (mPuzzleOverlay != NULL)
		mPuzzleOverlay->mDomeWidget->SetVisible(false);
	mMenuButton->SetVisible(false);
	mHintButton->SetVisible(false);
	if (mThirdButton != NULL)
		mThirdButton->SetVisible(false);
	mSelectedPiece = NULL;
	if (m1728 != NULL)
		m1728->Release();

	if (!a3D)
	{
		DoHyperspace();
	}
	else
	{
		m15fb0 = 1.0f;
		for (int aRowPos = 0; aRowPos < 0x9000; aRowPos += 0x300)
		{
			int aRow = aRowPos / 0x300;
			float aY = (float)(aRowPos / 0x2f);
			float aV = aY / 768.0f;
			float aDY = aY - 384.0f;
			float aDY2 = aDY * aDY;
			for (int aColPos = 0; aColPos < 0xc000; aColPos += 0x400)
			{
				WhirlPoint* aPoint = &mWhirlMesh[aRow][aColPos / 0x400];
				float aX = (float)(aColPos / 0x2f);
				aPoint->mX = aX;
				aPoint->mY = aY;
				aPoint->mZ = 0;
				aPoint->mRHW = 0;
				aPoint->mColor = 0;
				aPoint->mU = aX * 0.0009765625f;
				aPoint->mV = aV;
				float aDX = aX - 512.0f;
				aPoint->mAngle = (float)((double)(float)atan2((double)aDY, (double)aDX) + 6.28318);
				float aDist2 = aDX * aDX + aDY2;
				aPoint->mRadius = (float)sqrt((double)aDist2);    // (Ghidra calls the exe's sqrtf wrapper "logf")
			}
		}
		m15da0 = 0x5a;
		m15d9c = 0.0f;
		m15da4 = 0x50;
		m15df0 = 100;
	}
	m15de0 = 0.0;
	m15db0.SetInVal(0.0);
	m15da8 = 1.0f;
	m15f90 = 0.0;
	m15fa8 = false;
	m15f84 = 0;
	mSelectedPiece = NULL;
	m15f98 = 0.0;
	m15fa0 = 0.0;
	mState = STATE_WHIRLPOOL;
	m15de8 = true;
	mApp->ClearUpdateBacklog();
	m16028 = 0;
	m15e4c = 0;
	m15e94 = 0;
}

// 0x59da14: the game that the player has just won a secret mode in says so, once
void Board::UpdateSecretUnlocked()
{
	if (mSecretModeUnlocked)
		return;
	if (mApp->mGameMode < 4)
	{
		if (mApp->mProfile->IsModeUnlocked(mApp->mGameMode + 4))
		{
			mApp->DoDialog(0x25, true, "MODE UNLOCKED",
				"You have unlocked a secret game mode!\n\nThe new game mode can now be accessed from the main menu.", "", 3);
			mSecretModeUnlocked = true;
		}
	}
}

// 0x59dae4: the screen is back after the level change: the game is saved and the buttons come back
void Board::EndInterfaceRestore()
{
	SaveGame();
	UpdateSecretUnlocked();
	if (!mIsPuzzle)
	{
		mState = STATE_NEW;
		mDropDelay = 0x19;
	}
	else
	{
		mState = STATE_IDLE;
	}
	if (mPuzzleOverlay != NULL)
		mPuzzleOverlay->mDomeWidget->SetVisible(true);
	mMenuButton->SetVisible(true);
	mHintButton->SetVisible(true);
	if (mThirdButton != NULL)
		mThirdButton->SetVisible(true);
}

// 0x59db5a: the hint button of a puzzle: the next move of the solution when the puzzle is solvable from here, else the
// dialogs that say so (or offer to undo to the last position that was solvable)
void Board::ShowPuzzleHint()
{
	if (IsBoardEmpty())
		return;

	unsigned int aMoves[64];
	int aMoveCount = FindPuzzleValidMoves(aMoves, 0x40);
	if (aMoveCount == 0)
	{
		m1918++;
		if (!mSolutionValid)
			mApp->DoDialog(0x25, true, "No Hints", "There are no hints available for this puzzle.", "", 3);
		else
			mApp->DoDialog(0x16, true, "No Longer Solvable",
				"Do you want to UNDO to the last point the puzzle could be solved from?", "", 1);
	}
	else
	{
		if (m15ea9)
		{
			ShowNextPuzzleMove();
			return;
		}
		m1918++;
		mApp->DoDialog(0x18, true, "Still Solvable!",
			"This puzzle is solvable from this point.\nDo you want a hint on a move to make?", "", 1);
	}
}

// 0x59de98: "N PLANETS LEFT" over the board when a new planet starts
void Board::ShowPlanetCount()
{
	int aPuzzleNum = mApp->mProfile->GetPuzzleProgress()->mPuzzleNum;
	int aLeft = (mApp->mGalaxyParser->GetPlanetCount() - aPuzzleNum / 5) - 1;
	if (aLeft < 2)
		mEffectOverlay->DoStrechyText("FINAL PLANET!", 0x78);
	else
		mEffectOverlay->DoStrechyText(StrFormat("%d PLANETS LEFT", aLeft), 0x78);
	mEffectOverlay->m9c = (float)((double)mEffectOverlay->m9c * 0.55);
}

// 0x5a0c93: the level text (the level number, the score) is drawn once into the glow image that the board then draws
// with the glow
void Board::CreateFontGlowImages()
{
	mFontGlowImage->Clear();
	Graphics g(mFontGlowImage);
	int aTop;
	int aLeft;
	if (!mIsPuzzle || mIsSecret)
	{
		aTop = (mIsEndless ? -7 : 0) + 0x2a;
		aLeft = (mIsEndless ? -7 : 0) + 0x16;
	}
	else
	{
		aTop = 0x1b;
		aLeft = 0;
	}
	int aY = -mApp->ScaleNum(aTop + 0x59, 0);
	int aX = -mApp->ScaleNum(aLeft + 0x61, 0);
	g.Translate(aX, aY);
	DrawLevelText(&g);
}
