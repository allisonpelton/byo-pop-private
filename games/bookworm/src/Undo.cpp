#include "Undo.h"
#include "Board.h"
#include "BookWormApp.h"
#include "Effect.h"
#include "ModFeatures.h"
#include "OverlayWidget.h"
#include "Res.h"
#include "Solver.h"
#include "WordBalloon.h"

using namespace Sexy;

UndoStack::UndoStack(Board* theBoard) :
	mBoard(theBoard),
	mPending(false)
{
}

int UndoStack::Count()
{
	// a level-up leaves the earlier level's moves behind
	while ((!mSnapshots.empty()) && (mSnapshots.front().mLevel != mBoard->mLevel))
		mSnapshots.erase(mSnapshots.begin());
	return (int)mSnapshots.size();
}

void UndoStack::Push()
{
	Snapshot aSnapshot;
	if (!mBoard->SaveGameSnapshot(mBoard->mApp->mGameMode, &aSnapshot.mRecord))
		return;
	aSnapshot.mBoardRand = *mBoard->mRand;
	aSnapshot.mGameRand = *gGameRand;
	Board::GetGlobalState(&aSnapshot.mLastLetterIdx, &aSnapshot.mLastRefillLevel);
	aSnapshot.mNextBonusWord = mBoard->mApp->mBonusWord;
	aSnapshot.mLevel = mBoard->mLevel;

	Count();
	mSnapshots.push_back(aSnapshot);
	int aDepth = ModFeatureInt("undo", "depth", 10, 1, 100);
	if ((int)mSnapshots.size() > aDepth)
		mSnapshots.erase(mSnapshots.begin(), mSnapshots.end() - aDepth);
}

// In play, not paused, the tiles resting and the eaten word's tiles arrived at Lex (the record has no
// effects: putting it back drops them)
bool UndoStack::BoardReady()
{
	return (Solver::BoardAtRest(mBoard)) && (mBoard->mOverlay != NULL) &&
		(!mBoard->mOverlay->HasEffects(EFFECT_FLYING_PIECES)) && (mBoard->mApp->GetDialogCount() == 0);
}

void UndoStack::RequestUndo()
{
	if ((mBoard->mState != BOARD_PLAYING) || (mBoard->mPauseCount > 0) || (Count() == 0))
	{
		mPending = false;
		mBoard->mApp->PlaySample(SOUND_BUZZER);
		return;
	}
	mPending = true;
	Update();
}

void UndoStack::Update()
{
	if (!mPending)
		return;
	if ((mBoard->mState != BOARD_PLAYING) || (Count() == 0))
	{
		mPending = false;
		return;
	}
	if (!BoardReady())
		return;
	mPending = false;
	Restore();
}

void UndoStack::Restore()
{
	Snapshot aSnapshot = mSnapshots.back();
	mSnapshots.pop_back();

	// the clocks, the pause count and the held Ctrl key are now's
	int aLevelSeconds = mBoard->mLevelSeconds;
	int aTotalSeconds = mBoard->mTotalSeconds;
	int aLevelSecondsBanked = mBoard->mLevelSecondsBanked;
	int aPauseCount = mBoard->mPauseCount;
	bool aCtrlDown = mBoard->mCtrlDown;
	int aModWordsPlayed = mBoard->mModWordsPlayed;

	if (!mBoard->ReadSavedGame(aSnapshot.mRecord))
	{
		mBoard->mApp->PlaySample(SOUND_BUZZER);
		return;
	}
	*mBoard->mRand = aSnapshot.mBoardRand;
	*gGameRand = aSnapshot.mGameRand;
	Board::SetGlobalState(aSnapshot.mLastLetterIdx, aSnapshot.mLastRefillLevel);
	mBoard->mApp->mBonusWord = aSnapshot.mNextBonusWord;
	mBoard->mLevelSeconds = aLevelSeconds;
	mBoard->mTotalSeconds = aTotalSeconds;
	mBoard->mLevelSecondsBanked = aLevelSecondsBanked;
	mBoard->mPauseCount = aPauseCount;
	mBoard->mCtrlDown = aCtrlDown;
	mBoard->AddModWordsAllTime(mBoard->mModWordsPlayed - aModWordsPlayed);  // the undone word's count too

	// the record was made in Submit, with the word selected; Lex is a new one, so nothing would count down
	// the undone word's definition balloon (and the mouse resting on it would hold it up): it fades now
	std::for_each(mBoard->mSelection.begin(), mBoard->mSelection.end(), Piece::Deselect);
	mBoard->ClearWord();
	if ((mBoard->mOverlay != NULL) && (mBoard->mOverlay->mBalloon != NULL))
		mBoard->mOverlay->mBalloon->Show(0);
	mBoard->ClearPopups();  // the undone word's "+points"
	if (mBoard->mSolver != NULL)
		mBoard->mSolver->Reset();
	mBoard->mApp->PlaySample(SOUND_APPEAR2);
	mBoard->MarkDirty();
}
