// UserDialog.cpp: see UserDialog.h. The original keeps its tweakable numbers in ModVal calls (each returns the number it was
// compiled with); they are named constants here, each with the line of UserDialog.cpp its call names. One is 0 and switches a
// part off, which is left out: line 935 is the flag of a first line "(Create a New User)" in the list (a field at +0x198 of
// the original, read in FinishDeleteUser, FinishRenameUser and ListClicked); without it the first line is the profile in use.

#include "UserDialog.h"

#include "Color.h"
#include "Common.h"
#include "ListWidget.h"
#include "ScrollbarWidget.h"
#include "ThunderButton.h"
#include "ThunderCommon.h"
#include "ThunderballApp.h"
#include "WidgetManager.h"

#include <algorithm>

using namespace Sexy;

static const char* const DIALOG_HEADER = "WHO ARE YOU?";
static const char* const CREATE_BUTTON_LABEL = "Create New User"; // line 936
static const int LIST_INSET_X = 10;				// line 937: the list is this far inside the dialog's content area
static const int LIST_INSET_WIDTH = 20;			// line 938: and this much narrower
static const int LIST_HEIGHT = 155;				// line 939
static const int SCROLLBAR_WIDTH = 16;			// a literal of the original
static const int FRAME_LEFT = -8;				// line 940: the frame is this far outside the list, to the left
static const int FRAME_TOP = -8;				// line 941: and above
static const int FRAME_HEIGHT_EXTRA = 16;		// line 942: and this much taller
static const int FRAME_WIDTH_EXTRA = 8;			// line 943: and this far past the scrollbar's right edge
static const int RENAME_BUTTON_TOP = -5;		// line 944: the buttons stand above OK and Cancel; this is the gap
static const int DELETE_BUTTON_TOP = -5;		// line 945
static const int CREATE_BUTTON_TOP = -5;		// line 946
static const int EXTRA_HEIGHT = 255;			// line 947: what the dialog is taller than the framework's

// 0x4ac970
UserDialog::UserDialog(ThunderballApp* theApp)
	: ThunderDialog(DIALOG_USER, true, DIALOG_HEADER, "", "", BUTTONS_OK_CANCEL)
{
	mApp = theApp;
	mHangsOnChain = true;
	mList = MakeListWidget(0, this);
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mFrame = new ThunderFrame();
	mScrollbar = new ScrollbarWidget(0, mList);
	mRenameButton = MakeButton(BUTTON_RENAME, this, "Rename", nullptr);
	mDeleteButton = MakeButton(BUTTON_DELETE, this, "Delete", nullptr);
	mCreateButton = MakeButton(BUTTON_CREATE, this, CREATE_BUTTON_LABEL, nullptr);
	mList->mScrollbar = mScrollbar;

	// the profile in use comes first and is the one chosen, then the others
	PlayerInfo* aCurrentPlayer = theApp->mCurrentPlayer;
	if (aCurrentPlayer != nullptr)
		mList->SetSelect(mList->AddLine(aCurrentPlayer->mName, false));
	for (ProfileMgr::ProfileMap::iterator aProfile = theApp->mProfileMgr.mProfiles.begin();
		 aProfile != theApp->mProfileMgr.mProfiles.end(); ++aProfile)
	{
		if (aCurrentPlayer == nullptr || aProfile->second.mName != aCurrentPlayer->mName)
			mList->AddLine(aProfile->second.mName, false);
	}

	AddWidget(mList);
	AddWidget(mScrollbar);
	AddWidget(mRenameButton);
	AddWidget(mDeleteButton);
	AddWidget(mCreateButton);
	AddWidget(mFrame);
}

// 0x497dd0: the children go with the dialog
UserDialog::~UserDialog()
{
	RemoveAllWidgets(true, false);
}

// 0x4ace40
std::string UserDialog::GetSelName() const
{
	if (mList->mSelectIdx >= 0 && mList->mSelectIdx < mList->GetLineCount())
		return mList->GetStringAt(mList->mSelectIdx);
	return std::string();
}

// 0x4acec0
void UserDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);
	std::string aName = GetSelName();
	if (theId == BUTTON_RENAME)
	{
		if (!aName.empty())
			mApp->DoRenameUserDialog(aName);
	}
	else if (theId == BUTTON_DELETE)
	{
		if (!aName.empty())
			mApp->DoConfirmDeleteUserDialog(aName);
	}
	else if (theId == BUTTON_CREATE)
		mApp->DoCreateUserDialog();
}

// 0x497e50
void UserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	// the list, as wide as the dialog's content area less a little
	int aListX = LIST_INSET_X + (GetLeft() - mX);
	int aListY = GetTop() - mY;
	int aListWidth = GetWidth() - LIST_INSET_WIDTH;
	mList->Resize(aListX, aListY, aListWidth, LIST_HEIGHT);

	// the scrollbar takes the list's right edge when the names do not all fit
	mScrollbar->SetVisible(mScrollbar->GetThumbSize() != 0);
	if (mScrollbar->mVisible)
		mList->Resize(aListX, aListY, aListWidth - SCROLLBAR_WIDTH, LIST_HEIGHT);
	mScrollbar->ResizeScrollbar(aListWidth - SCROLLBAR_WIDTH + aListX, aListY, SCROLLBAR_WIDTH, LIST_HEIGHT);

	// the frame around both, the three buttons above OK and Cancel
	mFrame->Layout(LAY_SameHeight | LAY_SameLeft | LAY_SameTop, mList, FRAME_LEFT, FRAME_TOP, 0, FRAME_HEIGHT_EXTRA);
	mFrame->Layout(LAY_GrowToRight, mScrollbar, 0, 0, FRAME_WIDTH_EXTRA, 0);
	mRenameButton->Layout(LAY_SameSize | LAY_SameLeft | LAY_Above, mYesButton, -mX, RENAME_BUTTON_TOP - mY, 0, 0);
	mDeleteButton->Layout(LAY_SameSize | LAY_SameLeft | LAY_Above, mNoButton, -mX, DELETE_BUTTON_TOP - mY, 0, 0);
	mCreateButton->Layout(LAY_SameHeight | LAY_SameLeft | LAY_Above, mRenameButton, 0, CREATE_BUTTON_TOP, 0, 0);
	mCreateButton->Layout(LAY_GrowToRight, mDeleteButton, 0, 0, 0, 0);
}

// 0x4980a0
int UserDialog::GetPreferredHeight(int theWidth)
{
	return Dialog::GetPreferredHeight(theWidth) + EXTRA_HEIGHT;
}

// 0x4980d0
void UserDialog::CheckShowScrollbar()
{
	if ((mScrollbar->GetThumbSize() != 0) != mScrollbar->mVisible)
		Resize(mX, mY, mWidth, mHeight);
}

// 0x498120
void UserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->SetFocus(mList);
}

// 0x498150
void UserDialog::FinishDeleteUser()
{
	int aDeleted = mList->mSelectIdx;
	mList->RemoveLine(aDeleted);
	int aNewSelect = std::max(aDeleted - 1, 0);
	if (mList->GetLineCount() > 0)
		mList->SetSelect(aNewSelect);
	CheckShowScrollbar();
}

// 0x4981b0
void UserDialog::FinishRenameUser(const std::string& theNewName)
{
	if (mList->mSelectIdx >= 0)
		mList->SetLine(mList->mSelectIdx, theNewName);
}

// 0x498200
void UserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	mList->SetSelect(theIdx);
	if (theClickCount == 2)
		mApp->FinishUserDialog(true);
}
