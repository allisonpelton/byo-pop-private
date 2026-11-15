#include "NcMovingDialog.h"

#include <cmath>

#include "Application.h"
#include "Color.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Insets.h"
#include "KeyCodes.h"
#include "NcButton.h"
#include "Res.h"

using namespace Sexy;

namespace
{

// What a box that nobody closed answers; the original leaves it at a value no button gives
const int NO_CLOSE_RESULT = (int) 0xc4653601;

// What the escape key answers when it does nothing
const int NO_ESCAPE_RESULT = -1;

// How far below the screen a box that is leaving has to get before it counts as gone
const int LEAVE_MARGIN = 100;

}

// 0x468870
NcMovingDialog::NcMovingDialog(int theHeight, Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
	const std::string& theDialogHeader, const std::string& theDialogLines, const std::string& theDialogFooter, int theButtonMode) :
	Dialog(theComponentImage, theButtonComponentImage, theId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode)
{
	mClosePending = false;
	mEscapeResult = NO_ESCAPE_RESULT;
	mRestY = (gApp->mHeight - theHeight) / 2;
	mCloseResult = NO_CLOSE_RESULT;
	mLeaving = false;
	// the box starts one height above the screen: the fall to its resting place is a quarter of the speed squared
	mSlide = -sqrt((double) (mRestY + theHeight) * 4.0);
	gApp->PlaySample(SOUND_UI_MOVING_DIALOG_WOOSH);
}

// 0x468980: down from above while the speed is negative (it grows by theFrac a call, and the fall is a quarter of its square),
// and, once it has been told to leave, up again by the square of the speed growing the other way
void NcMovingDialog::UpdateF(float theFrac)
{
	Dialog::UpdateF(theFrac);

	if (mSlide < 0.0)
	{
		double aSlide = theFrac + mSlide;
		if (!(aSlide < 0.0))
			aSlide = 0.0;
		mSlide = aSlide;
		Move(mX, ToInt((double) mRestY - aSlide * aSlide * 0.25));
		return;
	}

	if (mSlide > 0.0 || mLeaving)
	{
		if (ToInt(mSlide * mSlide + (double) mRestY) < gApp->mHeight + LEAVE_MARGIN)
		{
			mSlide = theFrac + mSlide;
			Move(mX, ToInt(mSlide * mSlide + (double) mRestY));
		}
		else if (mClosePending)
		{
			Closed();
			mClosePending = false;
		}
	}
}

// 0x468b40
void NcMovingDialog::KeyDown(KeyCode theKey)
{
	Widget::KeyDown(theKey);
	if (theKey == KEYCODE_ESCAPE && mEscapeResult != NO_ESCAPE_RESULT)
	{
		mCloseResult = mEscapeResult;
		mLeaving = true;
		mClosePending = true;
	}
}

// 0x468bb0: the click of the two buttons the dialog makes itself
void NcMovingDialog::ButtonPress(int theId)
{
	if (theId == mId + 2000 || theId == mId + 3000)
		gApp->PlaySample(SOUND_UI_BUTTON_CLICK);
}

// 0x468b80
void NcMovingDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);
	mCloseResult = theId;
	mLeaving = true;
	mClosePending = true;
}

// 0x468c30: nothing sounds while the box is still on its way in
void NcMovingDialog::ButtonMouseEnter(int theId)
{
	if (mSlide == 0.0)
		gApp->PlaySample(SOUND_UI_BUTTON_MOUSE_OVER);
}

// 0x44e210
void NcMovingDialog::Closed()
{
	gApp->DialogClosed(mId, mCloseResult);
}

// 0x468aa0
void NcMovingDialog::CloseWith(int theResult)
{
	mCloseResult = theResult;
	mLeaving = true;
	mClosePending = true;
}

// 0x468b30
void NcMovingDialog::SetEscapeResult(int theResult)
{
	mEscapeResult = theResult;
}

// 0x468bf0
bool NcMovingDialog::IsMoving() const
{
	return mSlide != 0.0;
}

// 0x468ac0
int NcMovingDialog::GetContentX() const
{
	return mContentInsets.mLeft + mBackgroundInsets.mLeft + mX;
}

// 0x468ae0
int NcMovingDialog::GetContentY() const
{
	return mContentInsets.mTop + mBackgroundInsets.mTop + 0x36 + mY;
}

// 0x468b00
int NcMovingDialog::GetContentWidth() const
{
	return mWidth - mContentInsets.mRight - mContentInsets.mLeft - mBackgroundInsets.mRight - mBackgroundInsets.mLeft;
}

// 0x41d210: the original goes on to put the box at the top of the screen, under another box that is up; the callers all move it at
// once, so that part is left out
void Sexy::LayoutDialog(Dialog* theDialog, int theMinWidth)
{
	theDialog->SetHeaderFont(FONT_PTBANANASPLIT27);
	theDialog->SetLinesFont(FONT_EUROFURENCE14);
	theDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	theDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0, 255));

	int aWidth = theMinWidth;
	int aNeededWidth = FONT_ARIAL10B->StringWidth(theDialog->mDialogHeader) + theDialog->mContentInsets.mRight +
		theDialog->mContentInsets.mLeft + theDialog->mBackgroundInsets.mRight + 0x14 + theDialog->mBackgroundInsets.mLeft;
	if (aWidth < aNeededWidth)
		aWidth = aNeededWidth;

	theDialog->Resize(0, 0, aWidth, theDialog->GetPreferredHeight(aWidth));
	if (theDialog->mYesButton != nullptr)
		SetupButtonCels(theDialog->mYesButton, 3);
	if (theDialog->mNoButton != nullptr)
		SetupButtonCels(theDialog->mNoButton, 3);
}
