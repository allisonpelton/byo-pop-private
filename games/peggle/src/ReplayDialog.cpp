// ReplayDialog.cpp: see ReplayDialog.h. The original keeps its tweakable numbers in ModVal calls (each returns the number it was
// compiled with); they are named constants here, each with the line of ReplayDialog.cpp its call names.

#include "ReplayDialog.h"

#include "Board.h"
#include "Common.h"
#include "NotTranslated.h"
#include "Res.h"
#include "ThunderButton.h"
#include "ThunderCommon.h"
#include "WidgetManager.h"

using namespace Sexy;

static const int REPLAY_DIALOG_ID = 0x16;
static const int NUM_PANEL_BUTTONS = 4;					// Next, Prev, Save, Done
static const int REPLAY_BUTTON_X = 10;					// line 711
static const int REPLAY_BUTTON_Y = 10;					// line 712
static const int PANEL_BUTTON_X = 10;					// line 713
static const int PANEL_BUTTON_Y = 83;					// line 714: where the first of them is
static const int PANEL_BUTTON_GAP = -2;					// line 715: added to the segment picture's height between two of them
static const int PREFERRED_HEIGHT_GAP = -2;				// line 717 (the same gap, under the top picture)
static const int PREFERRED_HEIGHT_BUTTON_GAP = -2;		// line 718
static const int PREFERRED_HEIGHT_MULTIPLE_EXTRA = 0;	// line 719: added when there are several replays
static const int MOUSE_DOWN_PASSES_ON = 0;				// line 716: nonzero lets the dialog be dragged like another (it is not)
static const int DONE_BLINK_TIMES = 10;					// line 720
static const int SLOW_MO_RATE = 250;					// what the board's slow motion takes while the left button is held

// 0x4a9960
ReplayDialog::ReplayDialog(Board* theBoard)
	: ThunderDialog(REPLAY_DIALOG_ID, true, "", "", "", BUTTONS_NONE)
{
	mPriority = 0;
	mBoard = theBoard;
	mReplayButton = MakeEmbeddedButton(0, this, IMAGE_DLG_INSTANTREPLAY, false);
	AddWidget(mReplayButton);
	static const char* const aLabels[NUM_PANEL_BUTTONS] = {"Next", "Prev", "Save", "Done"};
	ThunderButton** aButtons[NUM_PANEL_BUTTONS] = {&mNextButton, &mPrevButton, &mSaveButton, &mDoneButton};
	for (int i = 0; i < NUM_PANEL_BUTTONS; i++)
	{
		*aButtons[i] = MakePurpleButton(i + 1, this, aLabels[i]);
		AddWidget(*aButtons[i]);
	}
	mSlowMoHeld = false;
	mReplayIndex = -1;
	mFastScrollOff = false;
	mShowing = false;
	SetMode(false, false, true);
}

// 0x49ab70
void ReplayDialog::SetMode(bool theShowing, bool theMultiple, bool theResetIndex)
{
	mShowing = theShowing;
	mSlowMoHeld = false;
	int aNumReplays = mBoard->mReplayCount;
	if (theResetIndex || mReplayIndex < 0 || aNumReplays <= mReplayIndex)
		mReplayIndex = -1;
	mMultiple = theMultiple;
	if (theMultiple)
	{
		if (aNumReplays < 2)
			mMultiple = false;
		else if (mReplayIndex < 0 || aNumReplays <= mReplayIndex)
			mReplayIndex = aNumReplays - 1;
	}
	mSaveButton->SetVisible(!theShowing || mBoard->mUnknownC6 || mMultiple);
	mNextButton->SetVisible(mMultiple);
	mPrevButton->SetVisible(mMultiple);
}

// 0x495b70
void ReplayDialog::SetMouseVisibility(bool theVisible)
{
	mReplayButton->mMouseVisible = theVisible;
	mNextButton->mMouseVisible = theVisible;
	mPrevButton->mMouseVisible = theVisible;
	mSaveButton->mMouseVisible = theVisible;
	mDoneButton->mMouseVisible = theVisible;
}

// 0x495d20
bool ReplayDialog::IsFast() const
{
	return mBoard->mIsOver && mBoard->mWidgetManager->IsRightButtonDown();
}

// 0x495d50
void ReplayDialog::StopBlinkingButtons()
{
	mDoneButton->mBlinkCount = 0;
}

// 0x49ad00
void ReplayDialog::BlinkDoneButton()
{
	mDoneButton->Blink(DONE_BLINK_TIMES, true);
}

// 0x495c90
int ReplayDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = PREFERRED_HEIGHT_GAP + IMAGE_DLG_REPLAYTOP->mHeight;
	ThunderButton* aButtons[NUM_PANEL_BUTTONS] = {mNextButton, mPrevButton, mSaveButton, mDoneButton};
	for (ThunderButton* aButton : aButtons)
	{
		if (aButton->mVisible)
			aHeight += PREFERRED_HEIGHT_BUTTON_GAP + IMAGE_DLG_REPLAYSEGMENT->mHeight;
	}
	if (mMultiple)
		aHeight += PREFERRED_HEIGHT_MULTIPLE_EXTRA;
	return aHeight;
}

// 0x495bb0: the buttons of the panel stand in a column, those not shown leaving no gap
void ReplayDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mReplayButton->Move(REPLAY_BUTTON_X, REPLAY_BUTTON_Y);
	int aY = PANEL_BUTTON_Y;
	ThunderButton* aButtons[NUM_PANEL_BUTTONS] = {mNextButton, mPrevButton, mSaveButton, mDoneButton};
	for (ThunderButton* aButton : aButtons)
	{
		if (aButton->mVisible)
		{
			aButton->Move(PANEL_BUTTON_X, aY);
			aY += PANEL_BUTTON_GAP + IMAGE_DLG_REPLAYSEGMENT->mHeight;
		}
	}
}

// 0x49cc30: while a replay is shown, holding the left button slows it down
void ReplayDialog::Update()
{
	ThunderDialog::Update();
	if (!IsScrolling() && mWidgetManager != nullptr)
	{
		if (mBoard->mIsOver && mWidgetManager->IsLeftButtonDown())
		{
			if (mSlowMoHeld)
				return;
			mSlowMoHeld = true;
			mBoard->SetSlowMo(true, SLOW_MO_RATE);
			return;
		}
		if (mSlowMoHeld)
		{
			mSlowMoHeld = false;
			mBoard->SetSlowMo(false, SLOW_MO_RATE);
		}
	}
}

// 0x49acc0
void ReplayDialog::MouseDown(int x, int y, int theClickCount)
{
	if (MOUSE_DOWN_PASSES_ON != 0)
		ThunderDialog::MouseDown(x, y, theClickCount);
}

// 0x495d70
void ReplayDialog::Draw(Graphics* g)
{
	NOT_TRANSLATED_DRAWING("ReplayDialog::Draw");
}

// 0x49e5d0
void ReplayDialog::DrawOverlay(Graphics* g)
{
	NOT_TRANSLATED_DRAWING("ReplayDialog::DrawOverlay");
}

// 0x49ad30: the Prev button slows the replay while it is held
void ReplayDialog::ButtonPress(int theId)
{
	if (theId == 2)
	{
		mSlowMoHeld = true;
		mBoard->SetSlowMo(true, SLOW_MO_RATE);
	}
}

// 0x49ad60
void ReplayDialog::ButtonDepress(int theId)
{
	if (IsScrolling())
		return;
	switch (theId)
	{
	case 0:
		if (!mReplayButton->mRightClicked)
		{
			NOT_TRANSLATED("Board::ShowReplay");
			return;
		}
		SetMode(mShowing, !mMultiple, false);
		NOT_TRANSLATED("Board::PositionReplayDialog");
		return;
	case 1:
	case 2:
		mReplayIndex += (theId == 1) * 2 - 1;
		if (mReplayIndex < 0)
			mReplayIndex = mBoard->mReplayCount - 1;
		if (mBoard->mReplayCount <= mReplayIndex)
			mReplayIndex = 0;
		MarkDirty();
		break;
	case 3:
		NOT_TRANSLATED("Board::DoReplayFileDialog");
		return;
	case 4:
		if (!mBoard->mUnknownC5)
		{
			NOT_TRANSLATED("Board::EndReplay");
			return;
		}
		NOT_TRANSLATED("Board::DoReplayFileDialog");
		return;
	}
}
