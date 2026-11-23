// The board's part of the tutorial boxes and help dialogs (see HintDialog.h for the dialogs): the flags that remember what was seen, the queue
// of boxes, and when a box is put up.
#include "Board.h"

#include "Bej3App.h"
#include "HintDialog.h"
#include "Profile.h"

#include <algorithm>

namespace Sexy {

// slot 131, 0x73c4c0 (bejeweled3): the box of that bit has not been seen, boxes are not turned off, no level's flash is on (+0x1188) and the time is
// not up (+0xe28)
bool Board::CanShowHint(int theHintId) const
{
	return (mHintFlags & (1u << theHintId)) == 0 && (mHintFlags & HINTS_OFF) == 0 && mCheckerFlash == 0.0f && !IsTimeUp();
}

// slot 213, 0x743f40 (bejeweled3): the mode's start asks for its help. The id is kept (the options dialog's HOW TO PLAY button reads it); when its
// bit is unseen and boxes are not off, the game waits 100 updates (UpdateTick) and the help is queued
void Board::RequestHelp(int theHelpId)
{
	mHelpId = theHelpId;
	if ((mHintFlags & (1u << theHelpId)) == 0 && (mHintFlags & HINTS_OFF) == 0) {
		mHelpHold = 100;
		HintRequest aRequest;
		aRequest.mHintId = theHelpId;
		aRequest.mPieceId = HELP_REQUEST;
		mHintQueue.push_back(aRequest);
	}
}

// 0x745bf0 (bejeweled3): the box of a gem is queued, and its bit is set in the board's flags and, in a game the player plays, in the profile's (so a
// box is shown once, as soon as it is asked for). port: the exe's record also holds a copy of the move for the box's REPLAY button
void Board::QueueHint(const Piece* thePiece, int theHintId)
{
	HintRequest aRequest;
	aRequest.mHintId = theHintId;
	aRequest.mPieceId = thePiece->mId;
	mHintQueue.push_back(aRequest);
	SetHintFlag(theHintId, true);
}

// 0x73b990 (bejeweled3)
void Board::SetHintFlag(int theHintId, bool theOn)
{
	const uint32_t aBit = 1u << theHintId;
	if (theOn)
		mHintFlags |= aBit;
	else
		mHintFlags &= ~aBit;
	if (!mDemonstration && gApp->mProfile != nullptr) {
		if (theOn)
			gApp->mProfile->mHintFlags |= aBit;
		else
			gApp->mProfile->mHintFlags &= ~aBit;
	}
}

bool Board::HintDialogUp() const
{
	return gApp->GetDialog(HINT_DIALOG_ID) != nullptr;
}

// 0x765d50 (bejeweled3): when no hint box or help dialog is up, the game goes on and boxes are not off, the first request of the queue is put up: a
// help dialog (the entry goes), or the box of a gem (the entry stays until the box's button is pressed: DialogButtonDepress). port: the exe also ends the speed bonus (+0x1371, 0x756100) when a box shows; the port has none. A request whose gem
// has gone is dropped and its bit freed. When the time is up the queue is emptied. port: the pulse of the gem the box points at (the curve at +0x26c8
// and its drawing, 0x756cb0) is not made
void Board::CheckForTutorialDialogs()
{
	if (gApp->GetDialog(HINT_DIALOG_ID) != nullptr || gApp->GetDialog(HELP_DIALOG_ID) != nullptr)
		return;
	if (mGameOverTicks != 0 || (mHintFlags & HINTS_OFF) != 0) {
		mHintQueue.clear();
		return;
	}
	while (!mHintQueue.empty()) {
		if (IsTimeUp()) {
			mHintQueue.clear();
			break;
		}
		const HintRequest aRequest = mHintQueue.front();
		if (aRequest.mPieceId == HELP_REQUEST) {
			HelpDialog* aDialog = new HelpDialog(aRequest.mHintId, (int)mHintQueue.size());
			gApp->AddDialog(aDialog);
			mHintQueue.erase(mHintQueue.begin());
			break;
		}
		Piece* aPiece = FindPieceById(aRequest.mPieceId);
		if (aPiece != nullptr) {
			HintDialog* aDialog = new HintDialog(aRequest.mHintId);
			gApp->AddDialog(HINT_DIALOG_ID, aDialog);
			aDialog->mDialogListener = this;
			// 1200ths of the art size: a box over a gem of the top four rows hangs below it (the gem's cell, 128, and 15 more), else it stands above it
			const int aArt = gApp->mArtRes;
			const int aX = aArt * (GetGridX() + 0x200) / 0x4b0 - aDialog->mWidth / 2;
			int aY;
			if (aPiece->mRow < 4)
				aY = (int)((float)(GetGridY() + 128 + 15) + aPiece->mY) * aArt / 0x4b0;
			else
				aY = (int)(((float)(GetGridY() - 15) + aPiece->mY) * (float)aArt / (float)0x4b0) - aDialog->mHeight;
			aDialog->Move(aX, aY);
			break;
		}
		SetHintFlag(aRequest.mHintId, false);
		mHintQueue.erase(mHintQueue.begin());
	}
}

}  // namespace Sexy
