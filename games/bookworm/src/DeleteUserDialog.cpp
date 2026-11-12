#include "DeleteUserDialog.h"
#include "SelectUserDialog.h"
#include "BookWormApp.h"
#include "UserManager.h"
#include "Res.h"
#include "GameText.h"

#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "ListWidget.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x52ea00 (the same five colours as SelectUserDialog's 0x52f788)
static int gDeleteUserListColors[5][3] =
{
	{53, 43, 29},       // background
	{0, 0, 0},          // outline
	{255, 225, 73},     // text
	{255, 255, 255},    // hilited text
	{188, 22, 22}       // selection fill
};

// 0x41ddb0 (body; scalar deleting dtor 0x41dfb0)
DeleteUserDialog::~DeleteUserDialog()
{
	// port: the original never deletes the list (0xc0 bytes leaked per dialog). It is out of the widget
	// manager by now (RemovedFromManager).
	delete mUserList;
}

// 0x41ddd0 (slot 50)
int DeleteUserDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog2004::GetPreferredHeight(theWidth);
	if (mUserList != NULL)
		return aHeight + 16 + mUserList->GetOptimalHeight();
	return aHeight;
}

// 0x41de10 (slot 13): SelectUserDialog::Resize without the ResetUserListWidget
void DeleteUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);

	// The list 16 px above the buttons as Dialog::Resize placed them, 40 px in from each side
	if (mUserList != NULL)
	{
		int aListHeight = mUserList->GetOptimalHeight();
		mUserList->Resize(mX + 40, mYesButton->mY - (aListHeight + 16), mWidth - 80, mUserList->GetOptimalHeight());
	}

	// The buttons again: 16 px in from the content edges with 16 px between them
	if (mYesButton != NULL)
	{
		if (mNoButton != NULL)
		{
			int aButtonWidth = (mWidth - mContentInsets.mRight - mContentInsets.mLeft - 48) / 2;
			int aButtonHeight = mYesButton->mComponentImage->GetHeight();
			mYesButton->Resize(mX + 16 + mContentInsets.mLeft, mHeight - mContentInsets.mBottom - aButtonHeight + mY,
				aButtonWidth, aButtonHeight);
			mNoButton->Resize(mX + mContentInsets.mLeft + 32 + aButtonWidth,
				mHeight - mContentInsets.mBottom - aButtonHeight + mY, aButtonWidth, aButtonHeight);
			return;
		}

		int aButtonHeight = mYesButton->mComponentImage->GetHeight();
		mYesButton->Resize(mX + mContentInsets.mLeft, mHeight - mContentInsets.mBottom + mY - aButtonHeight,
			mWidth - mContentInsets.mRight - mContentInsets.mLeft, aButtonHeight);
	}
}

// 0x432670 (slot 7; SelectUserDialog's, folded)
void DeleteUserDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
}

// 0x432680 (slot 1; SelectUserDialog's, folded)
void DeleteUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	if (mUserList != NULL)
		theWidgetManager->AddWidget(mUserList);
}

// 0x4326b0 (slot 2; SelectUserDialog's, folded)
void DeleteUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	if (mUserList != NULL)
		theWidgetManager->RemoveWidget(mUserList);
}

// 0x4326e0 (slot 16; SelectUserDialog's, folded): header and the warning lines
void DeleteUserDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);
}

// 0x41df60 (ButtonListener +0x58 slot 3; SelectUserDialog's, folded). Only OK and CANCEL exist, which
// click through the app, so the click here never plays.
void DeleteUserDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + ID_YES_2004) && (theId != mId + ID_NO_2004))
		mApp2->PlaySample(SOUND_BUTTONCLICK);
}

// 0x41e020: the confirmation (dialog 21, YES/NO; the app's DoDialog sets its fonts and colours)
void DeleteUserDialog::DoConfirmDeleteDialog(const std::string& theName)
{
	mApp2->DoDialog(DIALOG_CONFIRM_DELETE_USER, true, GameText(GT_ARE_YOU_SURE),
		StrFormat(GameTextC(GT_CONFIRM_DELETE_FORMAT), theName.c_str()), "", BUTTONS_YES_NO);
}

// 0x41e150 (ListListener, +0x120 slot 0): a double click asks for confirmation
void DeleteUserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if ((mUserList == NULL) || (mUserList->mId != theId))
		return;

	if (theIdx != mUserList->mSelectIdx)
	{
		mApp2->PlaySample(SOUND_BUTTONCLICK);
		mUserList->SetSelect(theIdx);
	}

	SexyString aName = mUserList->mLines[theIdx];
	if (theClickCount == 2)
		DoConfirmDeleteDialog(aName);
}

// 0x41e220 (ButtonListener +0x58 slot 4)
void DeleteUserDialog::ButtonDepress(int theId)
{
	// The app: 2020 -> KillDialog(21) (normally nothing to kill), 3020 -> KillDialog(20) (deferred delete:
	// this object stays valid here)
	Dialog2004::ButtonDepress(theId);

	if (theId == mId + ID_YES_2004)
	{
		// OK asks for confirmation; without a selection nothing happens and the dialog stays
		if ((mUserList != NULL) && (mUserList->mSelectIdx >= 0))
		{
			SexyString aName = mUserList->mLines[mUserList->mSelectIdx];
			DoConfirmDeleteDialog(aName);
		}
	}
	else if (theId == mId + ID_NO_2004)
	{
		// CANCEL refreshes the roster underneath (no resize)
		SelectUserDialog* aSelectUserDialog = (SelectUserDialog*) mApp2->GetDialog(DIALOG_SELECT_USER);
		if (aSelectUserDialog != NULL)
			aSelectUserDialog->ResetUserListWidget();
	}
}

// 0x41e370. The creator (BookWormApp::DoDeleteUserDialog) sets colour 0 (white; the lines keep the
// default yellow) and sizes it to (0, 17, 400, GetPreferredHeight(400)).
DeleteUserDialog::DeleteUserDialog(BookWormApp* theApp, std::vector<std::string> theNames) :
	MoneyDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_DELETE_USER, true,
		GameText(GT_DELETE_USER_HEADER), GameText(GT_DELETE_USER_LINES), "", BUTTONS_OK_CANCEL),
	mApp2(theApp),
	mUserList(NULL)
{
	SetHeaderFont(theApp->mFonts[FONT_VERDANA18BOLDCOLOR]);
	SetLinesFont(mApp2->mFonts[FONT_VERDANA14BOLD]);
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	SetButtonFont(mApp2->mFonts[FONT_VERDANA10BOLD]);

	mUserList = new ListWidget(USER_LIST_ID, mApp2->mFonts[FONT_VERDANA10BOLD], this);
	mUserList->SetColors(gDeleteUserListColors, 5);
	mUserList->mDrawOutline = true;
	mUserList->mJustify = ListWidget::JUSTIFY_CENTER;

	// Nothing is selected (mSelectIdx stays -1)
	for (unsigned int i = 0; i < theNames.size(); i++)
		mUserList->AddLine(theNames[i], false);
}

// 0x41e620: called only by the app on 2021 (YES in the confirmation); the app then kills dialogs 21 and 20
void DeleteUserDialog::DeleteSelectedUser()
{
	if ((mUserList != NULL) && (mUserList->mSelectIdx >= 0))
	{
		SexyString aName = mUserList->mLines[mUserList->mSelectIdx];

		// Every line equal to the name goes, straight out of the vector (colours and selection stay)
		for (SexyStringVector::iterator anItr = mUserList->mLines.begin(); anItr != mUserList->mLines.end(); )
		{
			if (*anItr == aName)
				anItr = mUserList->mLines.erase(anItr);
			else
				++anItr;
		}

		UserManager* aUserManager = mApp2->mUserManager;
		if (aUserManager != NULL)
		{
			// Deletes the registry key and, only if that worked, the user; players\<name>\ and its saved
			// games stay on disk
			aUserManager->DeleteUser(aName);

			// The first remaining name becomes the current user (no LoadUserConfig here either)
			if (aName == aUserManager->GetCurrentUserName())
			{
				// port: the original reads mLines[0] unchecked; the list always has four names left here
				aName = mUserList->mLines.empty() ? SexyString() : mUserList->mLines[0];
				if (!aName.empty())
					aUserManager->SetCurrentUser(aName);
			}
		}
	}

	// Re-fit the roster underneath: its GetPreferredHeight and Resize both refill the list, so it shrinks and
	// shows the new-user line (GT_USER_LIST_CREATE) again
	Dialog* aSelectUserDialog = mApp2->GetDialog(DIALOG_SELECT_USER);
	if (aSelectUserDialog != NULL)
	{
		int aHeight = aSelectUserDialog->GetPreferredHeight(aSelectUserDialog->mWidth);
		aSelectUserDialog->Resize(aSelectUserDialog->mX, aSelectUserDialog->mY, aSelectUserDialog->mWidth, aHeight);
	}
}
