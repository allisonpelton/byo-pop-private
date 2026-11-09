// UserDialogs.cpp: CreateUserDialog, UserDialog and PuzzlePackSelector (UserDialogs.h).
#include "UserDialogs.h"

#include "EditWidget.h"
#include "Graphics.h"
#include "ListWidget.h"
#include "ScrollbarWidget.h"
#include "Res.h"
#include "WidgetManager.h"
#include "WinDMApp.h"

#include <cctype>

using namespace Sexy;

// The colors of the lists (the exe's data at 0x66bc0c and 0x66b5ac): background, outline, text, highlight, selection.
static int sListColors[5][3] = {{74, 1, 95}, {0, 0, 0}, {255, 255, 255}, {255, 255, 255}, {213, 9, 163}};

// ---- CreateUserDialog

// 0x5aed7c: a PodDialog (id 27, or 29 to rename) with an edit field for the name
CreateUserDialog::CreateUserDialog(bool isRename) :
	PodDialog(gApp, isRename ? 29 : 27, true, isRename ? "RENAME USER" : "NEW USER", "Please enter your name.", "", Dialog::BUTTONS_OK_CANCEL)
{
	// the colors of the edit field (red, green, blue, alpha): clear background and outline, light text, white
	// highlight with black text on it (the exe's data at 0x66b118)
	static int sEditColors[5][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}};
	mEdit = new EditWidget(0, this);
	mEdit->SetColors(sEditColors, 5);
	mEdit->SetFont(FONT_CONTINUUM_MED_14, nullptr);
	mEdit->mMaxChars = 12;
	mEdit->mMaxPixels = 200;
}

// 0x5aea42
CreateUserDialog::~CreateUserDialog()
{
	delete mEdit;
}

// 0x5aea99
void CreateUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	PodDialog::Resize(theX, theY, theWidth, theHeight);
	mEdit->Resize(mApp->ScaleNum(0x28, 0) + mX, mApp->ScaleNum(0x6e, 3) + mY, mWidth - mApp->ScaleNum(0x50, 0), mApp->ScaleNum(0x20, 0));
}

// 0x5aeb1b
int CreateUserDialog::GetPreferredHeight(int theWidth)
{
	return PodDialog::GetPreferredHeight(theWidth) + mApp->ScaleNum(0x28, 0);
}

// 0x5aeb45
void CreateUserDialog::AddedToManager(WidgetManager* theManager)
{
	PodDialog::AddedToManager(theManager);
	theManager->AddWidget(mEdit);
	theManager->SetFocus(mEdit);
}

// 0x5aeb73
void CreateUserDialog::RemovedFromManager(WidgetManager* theManager)
{
	PodDialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mEdit);
}

// 0x5aeb96: Enter in the edit field presses the OK button
void CreateUserDialog::EditWidgetText(int theId, const SexyString& theString)
{
	ButtonDepress(ID_OK);
}

// 0x5aeba9: letters, digits and the space
bool CreateUserDialog::AllowChar(int theId, SexyChar theChar)
{
	return isalnum((unsigned char)theChar) != 0 || theChar == ' ';
}

// 0x5aec61
void CreateUserDialog::Draw(Graphics* g)
{
	PodDialog::Draw(g);
	Rect aRect(mEdit->mX - mX, mEdit->mY - mY, mEdit->mWidth, mEdit->mHeight);
	int aGrow = mApp->ScaleNum(4, 0);
	aRect = Rect(aRect.mX - aGrow, aRect.mY - aGrow, aRect.mWidth + aGrow * 2, aRect.mHeight + aGrow * 2);
	DrawEditBox(g, aRect);
}

// 0x5aeccc: the name as typed, with the runs of spaces turned into one space and no space at either end
SexyString CreateUserDialog::GetName()
{
	SexyString aName;
	char aPrev = ' ';
	for (int i = 0; i < (int)mEdit->mString.size(); i++) {
		char c = mEdit->mString[i];
		if (c == ' ') {
			if (aPrev != ' ')
				aName.push_back(' ');
		} else {
			aName.push_back(c);
		}
		aPrev = c;
	}
	if (!aName.empty() && aName[aName.size() - 1] == ' ')
		aName.resize(aName.size() - 1);
	return aName;
}

// 0x5aebfc
void CreateUserDialog::SetName(const SexyString& theName)
{
	mEdit->SetText(theName, true);
	mEdit->mCursorPos = (int)theName.size();
	mEdit->mHilitePos = 0;
}

// ---- UserDialog

// 0x5db642
UserDialog::UserDialog() :
	PodDialog(gApp, 32, true, "Who are you?", "", "", Dialog::BUTTONS_OK_CANCEL)
{
	WinDMApp* anApp = gApp;
	mList = new ListWidget(0, FONT_CONTINUUM_MED_14, this);
	mScrollbar = new ScrollbarWidget(0, mList);
	mRenameButton = new PodDialogButton(IMAGE_DIALOG_BUTTON, IMAGE_DIALOG_BUTTON_GLOW, 0, this);
	mRenameButton->SetFont(FONT_CONTINUUM_MED_14);
	mRenameButton->mLabel = "RENAME";
	mDeleteButton = new PodDialogButton(IMAGE_DIALOG_BUTTON, IMAGE_DIALOG_BUTTON_GLOW, 1, this);
	mDeleteButton->SetFont(FONT_CONTINUUM_MED_14);
	mDeleteButton->mLabel = "DELETE";
	mList->mScrollbar = mScrollbar;
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mList->mItemHeight = anApp->ScaleNum(0x14, 2);
	mList->SetColors(sListColors, 5);
	mList->mDrawOutline = true;
	mList->AddLine("(Create a New User)", false);
	if (!anApp->mProfile->mName.empty())
		mList->SetSelect(mList->AddLine(anApp->mProfile->mName, false));
	const std::vector<std::string>& aNames = anApp->mProfile->mNames;
	for (int i = 0; i < (int)aNames.size(); i++) {
		if (aNames[i] != anApp->mProfile->mName)
			mList->AddLine(aNames[i], false);
	}
	// the scrollbar is only there when the names do not fit
	if (!((aNames.size() > 4 || mApp->mWidth != 0x280) && (aNames.size() > 5 || mApp->mWidth == 0x280)))
		mScrollbar->SetVisible(false);
}

// 0x5db2e3
UserDialog::~UserDialog()
{
	delete mList;
	delete mScrollbar;
	delete mRenameButton;
	delete mDeleteButton;
}

// 0x5db36a
void UserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	PodDialog::Resize(theX, theY, theWidth, theHeight);
	int aLeft = mApp->ScaleNum(0x28, 0);
	int aTop = mApp->ScaleNum(0x51, 2);
	int aMargin = mApp->ScaleNum(0x50, 0);
	int aListHeight = mApp->ScaleNum(0x80, 0);
	int aScrollWidth = mScrollbar->mVisible ? 0x10 : 0;
	int aListWidth = (mWidth - aMargin) - aScrollWidth;
	mList->Resize(aLeft + mX, aTop + mY, aListWidth, aListHeight);
	mScrollbar->ResizeScrollbar(aListWidth + aLeft + mX, aTop + mY, aScrollWidth, aListHeight);
	mRenameButton->Layout(LAY_SameLeft | LAY_Above | LAY_SameHeight | LAY_SameWidth, mYesButton, 0, mApp->ScaleNum(-13, 0), 0, 0);
	mDeleteButton->Layout(LAY_SameLeft | LAY_Above | LAY_SameHeight | LAY_SameWidth, mNoButton, 0, mApp->ScaleNum(-13, 0), 0, 0);
}

// 0x5db47c
int UserDialog::GetPreferredHeight(int theWidth)
{
	return PodDialog::GetPreferredHeight(theWidth) + mApp->ScaleNum(0xda, 0);
}

// 0x5db4a9
void UserDialog::AddedToManager(WidgetManager* theManager)
{
	PodDialog::AddedToManager(theManager);
	theManager->AddWidget(mList);
	theManager->AddWidget(mScrollbar);
	theManager->AddWidget(mDeleteButton);
	theManager->AddWidget(mRenameButton);
}

// 0x5db4f3
void UserDialog::RemovedFromManager(WidgetManager* theManager)
{
	PodDialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mList);
	theManager->RemoveWidget(mScrollbar);
	theManager->RemoveWidget(mDeleteButton);
	theManager->RemoveWidget(mRenameButton);
}

// 0x5db53d: the selected line is gone: the one above it (but never the "create" line) is selected
void UserDialog::FinishDeleteUser()
{
	int aSelected = mList->mSelectIdx;
	mList->RemoveLine(aSelected);
	aSelected--;
	if (aSelected < 1)
		aSelected = 1;
	if (mList->GetLineCount() > 1)
		mList->SetSelect(aSelected);
}

// 0x5db584
void UserDialog::FinishRenameUser(const SexyString& theNewName)
{
	int aSelected = mList->mSelectIdx;
	if (aSelected > 0)
		mList->SetLine(aSelected, theNewName);
}

// 0x5db5ae: the first line creates a user, a click on another selects it and a double click picks it
void UserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	WinDMApp* anApp = gApp;
	if (theIdx == 0) {
		anApp->DoCreateUserDialog();
	} else {
		mList->SetSelect(theIdx);
		if (theClickCount == 2)
			anApp->FinishUserDialog(true);
	}
}

// 0x5db9ca: the selected name ("" for the create line)
SexyString UserDialog::GetSelName()
{
	if (mList->mSelectIdx > 0) {
		if (mList->mSelectIdx < mList->GetLineCount())
			return mList->GetStringAt(mList->mSelectIdx);
	}
	return "";
}

// 0x5dba24: the rename and delete buttons (ids 0 and 1) work on the selected name
void UserDialog::ButtonDepress(int theId)
{
	PodDialog::ButtonDepress(theId);
	WinDMApp* anApp = gApp;
	SexyString aName = GetSelName();
	if (!aName.empty()) {
		if (theId == 0)
			anApp->DoRenameUserDialog(aName);
		else if (theId == 1)
			anApp->DoConfirmDeleteUserDialog(aName);
	}
}

// ---- PuzzlePackSelector

// 0x5d1115
PuzzlePackSelector::PuzzlePackSelector() :
	PodDialog(gApp, 34, true, "Select Puzzle Pack", "", "PLAY", Dialog::BUTTONS_OK_CANCEL)
{
	WinDMApp* anApp = gApp;
	mYesButton->mLabel = "PLAY";
	mNoButton->mLabel = "CANCEL";
	mList = new ListWidget(0, FONT_CONTINUUM_MED_14, this);
	mScrollbar = new ScrollbarWidget(0, mList);
	mList->mScrollbar = mScrollbar;
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mList->mItemHeight = anApp->ScaleNum(0x14, 2);
	mList->SetColors(sListColors, 5);
	mList->mDrawOutline = true;
	if (!anApp->mProfile->mPuzzlePack.empty())
		mList->SetSelect(mList->AddLine(anApp->mProfile->mPuzzlePack, false));
	for (int i = 0; i < (int)anApp->mPuzzlePacks.size(); i++) {
		if (anApp->mPuzzlePacks[i] != anApp->mProfile->mPuzzlePack)
			mList->AddLine(anApp->mPuzzlePacks[i], false);
	}
	if (anApp->mPuzzlePacks.size() < 5)
		mScrollbar->SetVisible(false);
}

// 0x5d0f09
PuzzlePackSelector::~PuzzlePackSelector()
{
	delete mList;
	delete mScrollbar;
}

// 0x5d0f70
void PuzzlePackSelector::Resize(int theX, int theY, int theWidth, int theHeight)
{
	PodDialog::Resize(theX, theY, theWidth, theHeight);
	int aLeft = mApp->ScaleNum(0x28, 0);
	int aTop = mApp->ScaleNum(0x52, -1);
	int aMargin = mApp->ScaleNum(0x50, 0);
	int aListHeight = mApp->ScaleNum(0x5a, 10);
	int aScrollWidth = mScrollbar->mVisible ? 0x10 : 0;
	int aListWidth = (mWidth - aMargin) - aScrollWidth;
	mList->Resize(aLeft + mX, aTop + mY, aListWidth, aListHeight);
	mScrollbar->ResizeScrollbar(aListWidth + aLeft + mX, aTop + mY, aScrollWidth, aListHeight);
}

// 0x5d1029
int PuzzlePackSelector::GetPreferredHeight(int theWidth)
{
	return PodDialog::GetPreferredHeight(theWidth) + mApp->ScaleNum(0x46, 0);
}

// 0x5d1053
void PuzzlePackSelector::AddedToManager(WidgetManager* theManager)
{
	PodDialog::AddedToManager(theManager);
	theManager->AddWidget(mList);
	theManager->AddWidget(mScrollbar);
}

// 0x5d1083
void PuzzlePackSelector::RemovedFromManager(WidgetManager* theManager)
{
	PodDialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mList);
	theManager->RemoveWidget(mScrollbar);
}

// 0x5d10b3: a click selects the pack. A double click presses the yes button's id (1000) in the app, which ignores it (the
// app only answers the ids 2000+n and 3000+n), as the original does.
void PuzzlePackSelector::ListClicked(int theId, int theIdx, int theClickCount)
{
	WinDMApp* anApp = gApp;
	mList->SetSelect(theIdx);
	if (theClickCount == 2)
		anApp->ButtonDepress(mYesButton->mId);
}

// 0x5d13ae
SexyString PuzzlePackSelector::GetSelectedPack()
{
	if (mList->mSelectIdx < mList->GetLineCount())
		return mList->GetStringAt(mList->mSelectIdx);
	return "";
}
