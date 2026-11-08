#include "AbyssModeDialog.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "GameText.h"
#include "ListWidget.h"
#include "ScrollbarWidget.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "ImageFont.h"
#include "Image.h"
#include "Common.h"
#include <stdlib.h>

using namespace Sexy;

// Not translated (forwarders to Dialog's own functions; nothing to override): 0x437ca0 (slot 7, SetColor), 0x438e60
// (slot 16, Draw), 0x438340 (slot 17, Update) and MoneyDialog's constructor 0x4382c0, destructor body 0x438310, deleting
// destructor 0x438f70 and Draw 0x438330 (see the header)

// 0x55fcb8: the list's five colours (background, outline, text, hilite, select)
static int gAbyssListColors[5][3] =
{
	{128, 128, 128},
	{0, 0, 0},
	{255, 255, 255},
	{255, 200, 86},
	{100, 100, 100}
};

// 0x43e7a0: MoneyDialog (0x4382c0) first. Two buttons (ok / cancel), header "CONTINUE?", no lines or footer.
AbyssModeDialog::AbyssModeDialog(WinWSApp* theApp, UserProfile* theUser) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_ABYSS_MODE, true, GT(0x0052d1ec), "", "",
		Dialog::BUTTONS_OK_CANCEL),
	mApp(theApp),
	mList(NULL),
	mScrollbar(NULL),
	mSelectedDepth(0)
{
	mTextAlign = -1;
	mSelectedDepth = 0;
	SetHeaderFont(mApp->mFontRockwell13Bold);
	SetLinesFont(mApp->mFontRockwell13);
	SetButtonFont(mApp->mFontRockwell13);
	Dialog::SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));

	// the original also reads a value of the profile manager here (a call of Font::GetHeight, folded with a plain
	// "return [this+0xc]", on mApp->mProfileMgr) and drops the result: not ported

	mList = new ListWidget(0, mApp->mFontRockwell13, this);
	mList->SetColors(gAbyssListColors, 5);
	mList->mDrawOutline = true;
	mList->mFont = mApp->mFontRockwell13;
	mList->mJustify = 1;

	mScrollbar = new ScrollbarWidget(0x2593, mList);
	mScrollbar->mHorizontal = false;
	mList->mScrollbar = mScrollbar;

	ResetList();

	// a line for each full 400 feet of the best depth; the scroll bar's maximum counts them (but not the first line)
	int aBestDepth = (theUser->mBestDepth / 400) * 400;
	int aLineDepth = 400;
	if (aBestDepth >= 400)
	{
		do
		{
			mList->AddLine(StrFormat(GTC(0x0052ec24), aLineDepth), false);
			aLineDepth += 400;
			mScrollbar->mMaxValue = mScrollbar->mMaxValue + 1.0;
		}
		while (aLineDepth <= aBestDepth);
	}
}

// 0x438ea0 (body; the scalar deleting destructor is 0x4391b0): the scroll bar goes first, then the list
AbyssModeDialog::~AbyssModeDialog()
{
	delete mScrollbar;
	delete mList;
}

// 0x43dbd0: empties the list (the original frees the line vector) and puts the first line in; the constructor's only call
void AbyssModeDialog::ResetList()
{
	if (mList != NULL)
	{
		mList->mLines.clear();
		mList->AddLine(GT(0x0052e570), false);
		MarkDirty();
	}
}

// 0x4379f0 slot 50: room for five lines of the list under the base's height
int AbyssModeDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog2004::GetPreferredHeight(theWidth);
	if (mList != NULL)
		return mList->mFont->GetHeight() * 5 + aHeight + 0x10;
	return aHeight;
}

// 0x437a30 slot 1: the first line (the beginning) is selected
void AbyssModeDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	if (mList != NULL)
	{
		theWidgetManager->AddWidget(mList);
		theWidgetManager->AddWidget(mScrollbar);
		mList->SetSelect(0);
	}
}

// 0x437a80 slot 2
void AbyssModeDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	if (mList != NULL)
	{
		theWidgetManager->RemoveWidget(mList);
		theWidgetManager->RemoveWidget(mScrollbar);
	}
}

// 0x437ac0 slot 13: the list (five lines of the font height + 2) sits above the buttons, the scroll bar at its right
// edge; the two buttons are laid out again with 0x10 pixels at the left and a gap of 0x20 between them
void AbyssModeDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);

	int aLineHeight = mList->mFont->GetHeight();
	int aListHeight = (aLineHeight + 2) * 5;
	int aListY = mYesButton->mY - aListHeight;
	mList->Resize(mX + 0x28, aListY, mWidth - 0x50, aListHeight);
	mScrollbar->ResizeScrollbar(mWidth + mX - 0x28, aListY, 0x19, aListHeight);

	if (mYesButton != NULL)
	{
		if (mNoButton != NULL)
		{
			int aButtonWidth = ((mWidth - mContentInsets.mRight - mContentInsets.mLeft) - 0x30) / 2;
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mX + mContentInsets.mLeft + 0x10, ((mY - aButtonHeight) - mContentInsets.mBottom) + mHeight,
				aButtonWidth, aButtonHeight);
			mNoButton->Resize(mX + aButtonWidth + mContentInsets.mLeft + 0x20,
				((mY - aButtonHeight) - mContentInsets.mBottom) + mHeight, aButtonWidth, aButtonHeight);
			return;
		}
		if (mYesButton != NULL)
		{
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mX + mContentInsets.mLeft, ((mHeight - mContentInsets.mBottom) + mY) - aButtonHeight,
				(mWidth - mContentInsets.mRight) - mContentInsets.mLeft, aButtonHeight);
		}
	}
}

// 0x437c20 (ButtonListener slot 3; SelectUserDialog's and DeleteUserDialog's are this function too): the click goes to
// the app; a press of any other button (there is none) plays the button sample
void AbyssModeDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + ID_YES_2004) && (theId != mId + ID_NO_2004))
		mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x43a890 (ButtonListener slot 4): the yes button takes the depth of the selected line (the number before " feet"; the
// first line has none and gives 0), then the click goes to the app
void AbyssModeDialog::ButtonDepress(int theId)
{
	if ((theId == mId + ID_YES_2004) && (mList != NULL) && (mList->mSelectIdx >= 0))
	{
		unsigned int aLineCount = mList->mLines.size();
		if ((unsigned int)mList->mSelectIdx < aLineCount)
		{
			std::string aLine = mList->mLines[mList->mSelectIdx];
			std::string aNumber = aLine.substr(0, aLine.find(GTC(0x0052e568), 0));
			mSelectedDepth = atol(aNumber.c_str());
		}
	}
	Dialog2004::ButtonDepress(theId);
}

// 0x43a750 (ListListener slot 0): a click on another line selects it (with the button sample); the first line sets the
// depth 0; a double click on a depth line sets that depth
void AbyssModeDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if ((mList != NULL) && (mList->mId == theId))
	{
		if (theIdx != mList->mSelectIdx)
		{
			mApp->PlaySample(SOUND_BUTTONCLICK);
			mList->SetSelect(theIdx);
		}

		std::string aLine = mList->mLines[theIdx];
		if (aLine == GT(0x0052e570))
			mSelectedDepth = 0;
		else if (theClickCount == 2)
		{
			std::string aNumber = aLine.substr(0, aLine.find(GTC(0x0052e568), 0));
			mSelectedDepth = atol(aNumber.c_str());
		}
	}
}
