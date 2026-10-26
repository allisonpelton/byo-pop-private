#include "UserSelectionDialog.h"

#include <algorithm>

#include "DialogListener.h"
#include "Font.h"
#include "Image.h"
#include "JeffLib.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

using namespace JeffLib;
using namespace Sexy;

// the list's colours: background, outline, text, hilite, select and select text (0x818618)
static int gListColors[][3] = {{40, 33, 5}, {0, 0, 0}, {202, 164, 72}, {255, 255, 255}, {190, 0, 7}, {249, 233, 7}};

UserSelectionDialog::UserSelectionDialog(const Resources& theResources)
	: ListBoxDialog(theResources.mFont, theResources.mHeader, "", theResources.mDialogImage,
	                theResources.mButtonImage, theResources.mClickSound, gListColors)
{
	mNewButton = new DialogButton(theResources.mButtonImage, BUTTON_NEW, this);
	mNewButton->mLabel = theResources.mNewLabel;
	mDeleteButton = new DialogButton(theResources.mButtonImage, BUTTON_DELETE, this);
	mDeleteButton->mLabel = theResources.mDeleteLabel;
	mMaxProfiles = 0x7fffffff;

	// the pictures' columns, as OK and CANCEL show theirs
	int aCelWidth = theResources.mButtonImage->GetCelWidth();
	int aCelHeight = theResources.mButtonImage->GetCelHeight();
	DialogButton* aButtons[2] = {mNewButton, mDeleteButton};
	int i = 0;
	do {
		DialogButton* aButton = aButtons[i];
		aButton->mNormalRect = Rect(0, 0, aCelWidth, aCelHeight);
		aButton->mOverRect = Rect(aCelWidth, 0, aCelWidth, aCelHeight);
		aButton->mDownRect = Rect(aCelWidth * 2, 0, aCelWidth, aCelHeight);
		if (theResources.mButtonImage->mNumCols > 3)
			aButton->mDisabledRect = Rect(aCelWidth * 3, 0, aCelWidth, aCelHeight);
		i++;
	} while (i < 2);

	RefreshList();
	mAllowDrag = true;
}

UserSelectionDialog::~UserSelectionDialog()
{
	delete mNewButton;
	delete mDeleteButton;
}

std::string UserSelectionDialog::GetUser(size_t theIdx) const
{
	if (mUsers.empty() || theIdx >= mUsers.size())
		return std::string();
	return mUsers[theIdx];
}

void UserSelectionDialog::ClearUsers()
{
	mUsers.clear();
	mCurrentUser = "";
}

void UserSelectionDialog::SetButtonFont(Font* theFont)
{
	Dialog::SetButtonFont(theFont);
	mNewButton->SetFont(theFont);
	mDeleteButton->SetFont(theFont);
}

void UserSelectionDialog::ButtonDepress(int theId)
{
	if (theId == BUTTON_NEW) {
		mDialogListener->DialogButtonDepress(mId, BUTTON_NEW);
		return;
	}
	if (theId == BUTTON_DELETE) {
		mDialogListener->DialogButtonDepress(mId, BUTTON_DELETE);
		if (mUsers.size() < mMaxProfiles) {
			mNewButton->SetDisabled(false);
			mNewButton->SetColor(0, Color(255, 255, 255, 255));
		}
	} else {
		ListBoxDialog::ButtonDepress(theId);
	}
}

void UserSelectionDialog::RefreshList()
{
	mList->mLines.clear();

	// the others, in order
	std::vector<std::string> aOthers;
	size_t i;
	for (i = 0; i < mUsers.size(); i++) {
		if (mUsers[i] != mCurrentUser)
			aOthers.push_back(mUsers[i]);
	}
	std::sort(aOthers.begin(), aOthers.end(), StringLessNatural);

	for (i = 0; i < aOthers.size(); i++) {
		if (aOthers[i] != mCurrentUser)
			mList->AddLine(aOthers[i], false);
	}

	mScrollbar->mMaxValue = (double)(unsigned int)mUsers.size();
	mScrollbar->ClampValue();

	// the player now playing first. (The lines' colours are not cleared with the lines: each refresh adds its
	// own to the old ones, AddLine's text colours and here the footer's.)
	if (!mCurrentUser.empty()) {
		mList->mLines.insert(mList->mLines.begin(), mCurrentUser);
		mList->mLineColors.push_back(mColors[COLOR_FOOTER]);
	}
}

void UserSelectionDialog::SelectUser(const std::string& theName)
{
	size_t i = 0;
	while (true) {
		if (mList->mLines.empty() || i >= mList->mLines.size())
			return;
		if (mList->mLines[i] == theName)
			break;
		i++;
	}
	mSelected = theName;
	mList->SetSelect((int)i);
}

void UserSelectionDialog::AddUser(const std::string& theName)
{
	if (mUsers.size() < mMaxProfiles) {
		mUsers.push_back(theName);
		RefreshList();
		if (mUsers.size() < mMaxProfiles) {
			mNewButton->SetDisabled(false);
			mNewButton->SetColor(0, Color(255, 255, 255, 255));
		} else {
			mNewButton->SetDisabled(true);
			mNewButton->SetColor(0, Color(255, 255, 255, 128));
		}
		// the list is centred when the scrollbar is not there
		if (!mScrollbar->mVisible)
			mList->Move(mX + 52, mList->mY);
		else
			mList->Move(mX + 40, mList->mY);
	}
}

void UserSelectionDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	ListBoxDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNewButton);
	theWidgetManager->AddWidget(mDeleteButton);
}

void UserSelectionDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mList);
	theWidgetManager->RemoveWidget(mScrollbar);
	theWidgetManager->RemoveWidget(mNewButton);
	theWidgetManager->RemoveWidget(mDeleteButton);
}

int UserSelectionDialog::GetPreferredHeight(int theWidth)
{
	return ListBoxDialog::GetPreferredHeight(theWidth) + mYesButton->mComponentImage->GetHeight();
}

void UserSelectionDialog::Resize(const Rect& theRect)
{
	Resize(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void UserSelectionDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	ListBoxDialog::Resize(theX, theY, theWidth, theHeight);

	// NEW and DELETE side by side over OK and CANCEL
	int aWidth = (mWidth - mContentInsets.mRight - mContentInsets.mLeft - 48) / 2;
	int aHeight = mYesButton->mComponentImage->GetHeight();
	mNewButton->Resize(mX + mContentInsets.mLeft + 16, mHeight - mContentInsets.mBottom + mY - aHeight * 2 - 7, aWidth,
	                   aHeight);
	mDeleteButton->Resize(mContentInsets.mLeft + mX + aWidth + 32,
	                      mHeight - mContentInsets.mBottom + mY - aHeight * 2 - 7, aWidth, aHeight);
}

void UserSelectionDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog::SetColor(theIdx, theColor);
	DialogButton* aButtons[2] = {mNewButton, mDeleteButton};
	if (theIdx == COLOR_BUTTON_TEXT || theIdx == COLOR_BUTTON_TEXT_HILITE) {
		int i = 0;
		do {
			int aLabelColor = theIdx != COLOR_BUTTON_TEXT ? DialogButton::COLOR_LABEL_HILITE : DialogButton::COLOR_LABEL;
			aButtons[i]->SetColor(aLabelColor, theColor);
			i++;
		} while (i < 2);
	}
}

void UserSelectionDialog::MouseDrag(int x, int y)
{
	if (mAllowDrag)
		Dialog::MouseDrag(x, y);
}

void UserSelectionDialog::MouseDown(int x, int y, int theClickCount)
{
	Dialog::MouseDown(x, y, theClickCount);
	if (!mAllowDrag) {
		// the dialog's mouse down has taken the pointer for dragging
		if (gSexyAppBase->GetCursor() == CURSOR_DRAGGING)
			gSexyAppBase->SetCursor(CURSOR_POINTER);
	}
}
