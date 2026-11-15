// BoardLogic.cpp: the board's puzzle selection, the undo lists and the falling of the gems (Sexy::Board,
// 0x5a92ed-0x5a9cdf and 0x5a9ced-0x5aa2f0).

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "Res.h"
#include "GalaxyParser.h"
#include "PuzzleStateOverlay.h"
#include "EffectOverlay.h"
#include "PodDialog.h"
#include "Common.h"
#include "WidgetManager.h"
#include <math.h>
#include <stdlib.h>
#include <algorithm>

using namespace Sexy;

// port: the dialog button of the board's puzzle screens (id 400 and 401), added to the widget manager (the same five steps
// each time in the original)
PodDialogButton* Board::AddBoardButton(int theId, const char* theLabel)
{
	PodDialogButton* aButton = new PodDialogButton(IMAGE_DIALOG_BUTTON, IMAGE_DIALOG_BUTTON_GLOW, theId, this);
	aButton->mLabel = theLabel;
	aButton->SetFont(FONT_CONTINUUM_MED_14);
	aButton->mAlpha = 0.0f;
	mWidgetManager->AddWidget(aButton);
	return aButton;
}

// 0x595958: the tutorial puzzle goes on to its next step
void Board::AdvanceTutorial()
{
	mTutorialStep++;
	mTutorialMoveFade = 1.0f;
	mTutorialNextFade = 0.0f;
}

// 0x5a92ed: the puzzle of a pod (0 to 4) or, with -1, the planet's intro puzzle
void Board::SelectPuzzlePod(int thePod)
{
	KillTutorialButtons();
	mPendingPod = -1;
	GalaxyParser* aGalaxy = mApp->mGalaxyParser;
	std::string aDir = "puzzles\\" + aGalaxy->mPuzzlePack;
	PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
	int aPlanetNum = aProgress->mPuzzleNum / 5;
	Planet& aPlanet = aGalaxy->GetPlanet(aPlanetNum);
	if (thePod == -1)
	{
		mIsPlanetIntro = true;
		mStartCountdown = 0;
		mTutorialText = aPlanet.mTutorialText;
		mTutorialMoves = aPlanet.mTutorialMoves;
		mTutorialMoveFade = 0;
		mTutorialNextFade = 0;
		mTutorialStep = 0;
		LoadPuzzle(PuzzlePath(aPlanet.mTutorialFile, aDir));
	}
	else
	{
		mIsPlanetIntro = false;
		aProgress->mPuzzleNum = aPlanetNum * 5 + thePod;
		LoadPuzzle(PuzzlePath(aPlanet.GetPuzzle(thePod).mFile, aDir));
		mPuzzleOverlay->SelectPod(thePod);
	}
}

// 0x5a9485: the pod under the mouse is shown first; the puzzle loads when the same pod is chosen again
void Board::DeferredSelectPuzzlePod(int thePod)
{
	if (thePod == mPuzzleOverlay->mSelectedPod)
		SelectPuzzlePod(thePod);
	else
	{
		PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
		aProgress->mPuzzleNum = (aProgress->mPuzzleNum / 5) * 5 + thePod;
		mPuzzleOverlay->SelectPod(thePod);
		mPendingPod = thePod;
	}
}

// port: the undo list and index of the current board kind (the original keeps two, selected by mInUndoPreview)
std::list<Buffer>& Board::GetUndoList()
{
	return mUndoList[mInUndoPreview ? 0 : 1];
}

// port: the index of the undo list of the current board kind
int& Board::GetUndoIndex()
{
	return mUndoIndex[mInUndoPreview ? 0 : 1];
}

// 0x5a9508: the state before a move is pushed on the list; a move after undoing drops the states newer than the one
// the player went back to
void Board::MarkForUndo()
{
	Buffer aBuffer;
	SaveState(aBuffer);
	int& aIndex = GetUndoIndex();
	std::list<Buffer>& aList = GetUndoList();
	if (aIndex >= 0)
	{
		std::list<Buffer>::iterator anIt = aList.begin();
		for (int i = 0; i <= aIndex; i++)
			++anIt;
		aList.erase(aList.begin(), anIt);
		aIndex = -1;
	}
	aList.push_front(aBuffer);
}

// 0x5a95cf
void Board::Undo()
{
	if (mIsPlanetIntro)
		return;
	if (mState != STATE_IDLE && mState != STATE_SWAPPING && mState != STATE_SWAP_BACK && mState != STATE_COMMIT_SWAP &&
		mState != STATE_CLEARING && mState != STATE_HYPER_SWAP_DONE && mState != STATE_NEW && mState != STATE_PUZZLE_SOLVED && mState != STATE_PUZZLE_STUCK)
		return;

	KillTutorialButtons();
	mState = STATE_IDLE;
	mPodArrowShown = false;
	mSelectedPiece = NULL;
	mSwapPiece[0] = NULL;
	mSwapPiece[1] = NULL;
	int& aIndex = GetUndoIndex();
	std::list<Buffer>& aList = GetUndoList();
	if (aIndex == -1)
	{
		Buffer aBuffer;
		SaveState(aBuffer);
		aList.push_front(aBuffer);
		aIndex = 0;
	}
	if (aIndex < (int)aList.size() - 1)
	{
		aIndex++;
		std::list<Buffer>::iterator anIt = aList.begin();
		for (int i = 0; i < aIndex; i++)
			++anIt;
		LoadState(*anIt);
	}
}

// 0x59806c: there is an older state to go back to (the test of UndoUntilSolvable)
bool Board::CanUndo()
{
	return GetUndoIndex() < (int)GetUndoList().size() - 1;
}

// 0x5a96f9: go back until the position can be solved again
void Board::UndoUntilSolvable()
{
	while (mSolutionMoves == 0 && mSolutionValid)
	{
		if (!CanUndo())
			break;
		Undo();
	}
	mHintAccepted = true;
}

// 0x5a9729: the next puzzle of the secret galaxy (Cognito): the puzzles of every planet are queued once, mixed a
// little and taken one after the other
void Board::LoadNextSecretPuzzle()
{
	mPuzzleFade = 0.0f;
	if (mSecretPuzzles.empty())
	{
		mApp->LoadGalaxyData();
		GalaxyParser* aGalaxy = mApp->mGalaxyParser;
		for (int aPlanetNum = 0; aPlanetNum < aGalaxy->GetPlanetCount(); aPlanetNum++)
		{
			Planet& aPlanet = aGalaxy->GetPlanet(aPlanetNum);
			for (int aPuzzleNum = 0; aPuzzleNum < aPlanet.GetPuzzleCount(); aPuzzleNum++)
				mSecretPuzzles.push_back(PuzzlePath(aPlanet.GetPuzzle(aPuzzleNum).mFile, "puzzles\\" + aGalaxy->mPuzzlePack));
		}
		mSecretPuzzleCount = (int)mSecretPuzzles.size();
		for (int i = 0; i < mSecretPuzzleCount * 5; i++)
		{
			int aFrom = Rand() % mSecretPuzzleCount;
			int aTo = Rand() % 5 - 2 + aFrom;
			if (aTo >= 0 && aTo < mSecretPuzzleCount)
				std::swap(mSecretPuzzles[aFrom], mSecretPuzzles[aTo]);
		}
	}
	std::string aFile = mSecretPuzzles.front();
	mSecretPuzzles.erase(mSecretPuzzles.begin());
	LoadPuzzle(aFile);
	MakePuzzleCrazy();
}

// 0x5a9979: the puzzle on the board is solved
void Board::PuzzleCompleted()
{
	if (mIsSecret)
	{
		if (mHintsUsed == 0)
		{
			mSecretSolved++;
			mSecretRun++;
		}
		if (mSecretPuzzles.empty())
		{
			mPlayAgainButton = AddBoardButton(400, "Play Again");
			mMainMenuButton = AddBoardButton(401, "Main Menu");
			ResizeButtons();
			mState = STATE_SECRET_FINISHED;
			DeleteSavedGame();
		}
		else if (mSecretRun == 4)
			DoWhirlpool();
		else
			LoadNextSecretPuzzle();
		return;
	}

	int aSolvedBefore = mPuzzleOverlay->mSolvedCount;
	mPuzzleOverlay->PuzzleCompleted();
	if (aSolvedBefore != mPuzzleOverlay->mSolvedCount)
	{
		mApp->mProfile->mStat[2]++;
		if (mPuzzleOverlay->mSolvedCount == 5)
		{
			mApp->PlaySample(SOUND_WORLD_COMPLETE);
			mEffectOverlay->DoStrechyText("COMPLETE!", 0x50);
		}
		else if (mPuzzleOverlay->mSolvedCount == 4)
		{
			mApp->PlaySample(SOUND_WARP_DRIVE_READY);
			mEffectOverlay->DoStrechyText("WARP READY", 0x50);
			mEffectOverlay->mTextSizeFactor = (float)(mEffectOverlay->mTextSizeFactor * 0.8);
		}
		else
		{
			mApp->PlaySample(SOUND_PUZZLE_SOLVED);
			mEffectOverlay->DoStrechyText("SOLVED!", 0x50);
		}
	}

	PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
	if ((int)aProgress->mSolved.size() <= aProgress->mPuzzleNum)
		aProgress->mSolved.resize(aProgress->mPuzzleNum + 1, 0);
	aProgress->mSolved[aProgress->mPuzzleNum] = 1;
	if (mApp->mGalaxyParser->mPuzzlePack == "Galaxy One")
	{
		int aCount = 0;
		for (int i = 0; i < (int)aProgress->mSolved.size(); i++)
		{
			if (aProgress->mSolved[i] != 0)
				aCount++;
		}
		if (mApp->mProfile->mGalaxyOneSolved < aCount)
			mApp->mProfile->mGalaxyOneSolved = aCount;
	}
	if (mState == STATE_IDLE)
		mState = STATE_PUZZLE_SOLVED;
}

// 0x5a9ced: the gems fall; when everything has settled the board looks for sets, for a move and for the end
void Board::UpdateFalling()
{
	if (mDropDelay > 0)
	{
		mDropDelay--;
		return;
	}
	if (mScreenState != SCREEN_SHOWN)
		return;

	bool aSettled = UpdateExplodeDelays();
	int aLandCount = 0;
	int aLandX = 0;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;
			if (aPiece->mY == (float)GetRowY(aRow))
			{
				aPiece->mVY = 0;
				continue;
			}
			aSettled = false;
			aPiece->mY = aPiece->mY + aPiece->mVY;
			int aRowY = GetRowY(aRow);
			bool aLanded = mFlipped ? (aPiece->mY <= (float)aRowY) : (aPiece->mY >= (float)aRowY);
			if (aLanded)
			{
				aLandCount++;
				aLandX += 0x2a + (int)aPiece->mX;
				aPiece->mY = (float)GetRowY(aRow);
			}
			// gravity: 0.24 a step, the twilight board 0.1 (and upside down it pulls up)
			if (mFlipped)
				aPiece->mVY = aPiece->mVY - 0.1f;
			else if (mIsTwilight)
				aPiece->mVY = aPiece->mVY + 0.1f;
			else
				aPiece->mVY = (float)((double)aPiece->mVY + 0.23999999463558197);
		}
	}
	if (aLandCount > 0)
	{
		if (abs(mLastLandSoundTick - mUpdateCnt) >= 10)
		{
			mLastLandSoundTick = mUpdateCnt;
			mApp->PlaySample(SOUND_GEM_HIT, GetPanPosition(aLandX / aLandCount));
		}
	}

	if (aSettled && mState == STATE_NEW)
	{
		if (FindSets())
			mState = STATE_CLEARING;
		else
		{
			if (!mSkipBestRecords)
			{
				if (mBiggestCascade < mCascadeCount)
					mBiggestCascade = mCascadeCount;
				if (mBiggestCombo < mMoveScore)
					mBiggestCombo = mMoveScore;
			}
			bool aHasMove = FindMove(NULL, 0, true, true);
			mState = STATE_IDLE;
			if (mIsTwilight)
			{
				// the twilight board turns over after every move: the rows swap places
				mFlipped = !mFlipped;
				for (int aRow = 0; aRow < 4; aRow++)
				{
					for (int aCol = 0; aCol < 8; aCol++)
					{
						Piece* aTop = mBoard[aRow][aCol];
						Piece* aBottom = mBoard[7 - aRow][aCol];
						aBottom->mRow = aRow;
						mBoard[aRow][aCol] = aBottom;
						aTop->mRow = 7 - aRow;
						mBoard[7 - aRow][aCol] = aTop;
					}
				}
			}
			if (mIsAction)
			{
				double aGrace = (double)mMoveScore / mLevelValue * mApp->mGraceSecondsMult + mApp->mGraceSeconds;
				if (mApp->mGraceSecondsMax <= aGrace)
					aGrace = mApp->mGraceSecondsMax;
				mDrainGrace += (int)(aGrace * 100.0);
			}

			bool aBombExploded = false;
			if (mCountBombs)
			{
				for (int aRow = 0; aRow < 8; aRow++)
				{
					for (int aCol = 0; aCol < 8; aCol++)
					{
						Piece* aPiece = mBoard[aRow][aCol];
						if (aPiece != NULL && aPiece->mType == 8)
						{
							if (aPiece->mCountdown == 0)
							{
								ExplodeAt(aCol, aRow);
								aBombExploded = true;
							}
							else
								aPiece->mCountdown--;
						}
					}
				}
				mCountBombs = false;
				if (aBombExploded)
				{
					mState = STATE_CLEARING;
					mApp->PlaySample(SOUND_BOMB_EXPLODE);
				}
			}

			if (!aHasMove && !aBombExploded)
			{
				mPodArrowShown = true;
				if (mIsPuzzle)
				{
					mPodArrowPhase = 0.0;
					if (mIsPlanetIntro)
					{
						mTutorialButton1 = AddBoardButton(400, "Show Again");
						mTutorialButton2 = AddBoardButton(401, "Continue");
						ResizeButtons();
					}
					else if (IsBoardEmpty())
						PuzzleCompleted();
					else
					{
						if (mIsSecret)
						{
							mTutorialButton1 = AddBoardButton(400, "Reset Puzzle");
							ResizeButtons();
						}
						mSelectedPiece = NULL;
						mState = STATE_PUZZLE_STUCK;
					}
				}
				else if (mIsAction)
					mState = STATE_BOARD_DROPPING;
				else
				{
					mEffectOverlay->DoStrechyText("NO MOVES!", 0x50);
					mApp->PlaySample(SOUND_NO_MORE_MOVES);
					GameOver(200);
				}
			}
		}
		CheckForHints();
	}
	if (mState == STATE_IDLE && mMouseHeld)
		MouseMove(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
	MarkDirtyFull();
}
