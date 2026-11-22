// ThunderballAppUsers.cpp: the part of ThunderballApp that asks for and chooses the player: the certificate of the first run, the
// list of players ("WHO ARE YOU?") and its rename and delete questions. The original keeps its tweakable numbers in ModVal calls
// (each returns the number it was compiled with); they are named constants here, each with the line of ThunderballApp.cpp its
// call names.

#include "Common.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "MainMenu.h"
#include "NewUserDialog.h"
#include "ThunderCommon.h"
#include "ThunderDialog.h"
#include "ThunderballApp.h"
#include "UserDialog.h"
#include "WidgetManager.h"

using namespace Sexy;

static const int SCROLL_FROM_ABOVE = 1;
static const int CERTIFICATE_X = 30;				// line 202: where the certificate of the first run comes to rest, when the main
static const int CERTIFICATE_Y = 30;				// line 203: menu is up
static const int USER_DIALOG_WIDTH = 400;			// the list of players is placed with this width
static const int USER_DIALOG_X = 150;				// line 200
static const int USER_DIALOG_Y = 60;				// line 201

// 0x40c3b0: the certificate that asks for the first player's name. With a profile in use it can be cancelled.
void ThunderballApp::DoCreateUserDialog()
{
	KillDialog(DIALOG_CREATE_USER);
	NewUserDialog* aDialog = new NewUserDialog(this, false, true, mCurrentPlayer != nullptr);
	int aWidth = aDialog->mWidth;
	int aHeight = aDialog->GetPreferredHeight(aWidth);
	if (GetDialogCount() == 0)
		aDialog->Resize((mWidth - aWidth) / 2, (mHeight - aHeight) / 2, aWidth, aHeight);
	else
		PositionDialog(aDialog, aWidth, false, -1);
	if (mMainMenu != nullptr)
	{
		aDialog->mX = CERTIFICATE_X;
		aDialog->mY = CERTIFICATE_Y;
	}
	aDialog->DoScroll(SCROLL_FROM_ABOVE);
	AddDialog(DIALOG_CREATE_USER, aDialog);
}

// 0x427830: OK (theYes) or Cancel of the new-user dialog. A name that is empty or taken brings up a message and keeps the dialog.
void ThunderballApp::FinishCreateUserDialog(bool theYes)
{
	NewUserDialog* aDialog = static_cast<NewUserDialog*>(GetDialog(DIALOG_CREATE_USER));
	if (aDialog == nullptr)
		return;

	std::string aName = aDialog->GetName();
	bool aNameIsEmpty = aName.empty();
	if (!theYes && mCurrentPlayer != nullptr)
	{
		// cancelled with a profile to go back to
		DoScrollOff(DIALOG_CREATE_USER);
		return;
	}
	if (!theYes || aNameIsEmpty)
	{
		// there is no profile to go back to, or OK with no name
		DoDialogScroll(DIALOG_NAME_ERROR, true, "Enter Your Name",
					   "Please enter your name to create a new user profile for storing high score data and game progress.", "OK",
					   Dialog::BUTTONS_FOOTER);
		return;
	}

	PlayerInfo* aProfile = mProfileMgr.AddProfile(aName);
	if (aProfile == nullptr)
	{
		DoDialogScroll(DIALOG_NAME_ERROR, true, "Name Conflict",
					   "The name you entered is already being used.  Please enter a unique player name.", "OK",
					   Dialog::BUTTONS_FOOTER);
		return;
	}

	mProfileMgr.Save();
	if (mCurrentPlayer != nullptr)
		mCurrentPlayer->SaveIfDirty();
	mCurrentPlayer = aProfile;
	if (mStatsMgr.mName.empty())
		mStatsMgr.mName = aProfile->mName;
	mWidgetManager->MarkAllDirty();
	if (!aDialog->mCertificate)
	{
		DoScrollOff(DIALOG_USER);
		DoScrollOff(DIALOG_CREATE_USER);
	}
	else
		aDialog->StartStamp();
	if (mMainMenu != nullptr)
	{
		mMainMenu->SyncPlayerInfo();
		mWidgetManager->SetFocus(mMainMenu);
	}
}

// 0x408500: the message about the name is closed and the name's box has the keys again
void ThunderballApp::FinishNameErrorDialog(int theDialogId)
{
	DoScrollOff(theDialogId);
	NewUserDialog* aDialog = static_cast<NewUserDialog*>(GetDialog(theDialogId != DIALOG_NAME_ERROR ? DIALOG_RENAME_USER : DIALOG_CREATE_USER));
	if (aDialog != nullptr)
		mWidgetManager->SetFocus(aDialog->mEditWidget);
}

// 0x40c2e0: the list of players
void ThunderballApp::DoUserDialog()
{
	KillDialog(DIALOG_USER);
	UserDialog* aDialog = new UserDialog(this);
	PositionDialog(aDialog, USER_DIALOG_WIDTH, false, -1);
	aDialog->mX = USER_DIALOG_X;
	aDialog->mY = USER_DIALOG_Y;
	aDialog->DoScroll(SCROLL_FROM_ABOVE);
	AddDialog(DIALOG_USER, aDialog);
}

// 0x427720: OK (theYes) or Cancel of the list. OK makes the line chosen the profile in use.
void ThunderballApp::FinishUserDialog(bool theYes)
{
	UserDialog* aDialog = static_cast<UserDialog*>(GetDialog(DIALOG_USER));
	if (aDialog == nullptr)
		return;

	if (theYes)
	{
		if (mCurrentPlayer != nullptr)
			mCurrentPlayer->SaveIfDirty();
		PlayerInfo* aProfile = mProfileMgr.GetProfile(aDialog->GetSelName());
		if (aProfile != nullptr)
		{
			mCurrentPlayer = aProfile;
			mWidgetManager->MarkAllDirty();
			if (mMainMenu != nullptr)
				mMainMenu->SyncPlayerInfo();
			CheckMaxStage();
		}
	}
	if (mMainMenu != nullptr)
		mWidgetManager->SetFocus(mMainMenu);
	DoScrollOff(DIALOG_USER);
}

// 0x40c4d0: the box for a new name for the player called theName
void ThunderballApp::DoRenameUserDialog(const std::string& theName)
{
	KillDialog(DIALOG_RENAME_USER);
	NewUserDialog* aDialog = new NewUserDialog(this, true, false, true);
	PositionDialog(aDialog, aDialog->mWidth, false, -1);
	aDialog->SetName(theName);
	aDialog->DoScroll(SCROLL_FROM_ABOVE);
	AddDialog(DIALOG_RENAME_USER, aDialog);
}

// 0x427b90: OK (theYes) or Cancel of the rename. An empty name does nothing (the box stays); a name that is taken says so.
void ThunderballApp::FinishRenameUserDialog(bool theYes)
{
	if (!theYes)
	{
		DoScrollOff(DIALOG_RENAME_USER);
		return;
	}

	UserDialog* aUserDialog = static_cast<UserDialog*>(GetDialog(DIALOG_USER));
	NewUserDialog* aNameDialog = static_cast<NewUserDialog*>(GetDialog(DIALOG_RENAME_USER));
	if (aUserDialog == nullptr || aNameDialog == nullptr)
		return;

	std::string anOldName = aUserDialog->GetSelName();
	std::string aNewName = aNameDialog->GetName();
	if (aNewName.empty())
		return;

	if (mCurrentPlayer != nullptr)
		mCurrentPlayer->SaveIfDirty();
	bool aRenamedTheCurrent = mProfileMgr.GetProfile(anOldName) == mCurrentPlayer;
	if (!mProfileMgr.RenameProfile(anOldName, aNewName))
	{
		DoDialogScroll(DIALOG_NAME_ERROR_2, true, "Name Conflict",
					   "The name you entered is already being used.  Please enter a unique player name.", "OK",
					   Dialog::BUTTONS_FOOTER);
		return;
	}

	mProfileMgr.Save();
	if (aRenamedTheCurrent)
		mCurrentPlayer = mProfileMgr.GetProfile(aNewName); // its entry in the list is a new one
	aUserDialog->FinishRenameUser(aNewName);
	mWidgetManager->MarkAllDirty();
	DoScrollOff(DIALOG_RENAME_USER);
}

// 0x41d260: "Delete User?" for the player called theName
void ThunderballApp::DoConfirmDeleteUserDialog(const std::string& theName)
{
	KillDialog(DIALOG_CONFIRM_DELETE_USER);
	std::string aLines = StrFormat("This will permanently remove '%s' from the player roster!", theName.c_str());
	Dialog* aDialog = SexyAppBase::DoDialog(DIALOG_CONFIRM_DELETE_USER, true, "Delete User?", aLines, "", Dialog::BUTTONS_YES_NO);
	static_cast<ThunderDialog*>(aDialog)->DoScroll(SCROLL_FROM_ABOVE);
	aDialog->mYesButton->mLabel = "Delete";
	aDialog->mNoButton->mLabel = "Cancel";
}

// 0x41d3d0: Delete (theYes) or Cancel. The player chosen in the list goes (with the files), the next one up the list is chosen,
// and with nobody left the certificate comes up to make a new one.
void ThunderballApp::FinishConfirmDeleteUserDialog(bool theYes)
{
	DoScrollOff(DIALOG_CONFIRM_DELETE_USER);
	if (!theYes)
		return;
	UserDialog* aDialog = static_cast<UserDialog*>(GetDialog(DIALOG_USER));
	if (aDialog == nullptr)
		return;

	std::string aCurrentName = mCurrentPlayer != nullptr ? mCurrentPlayer->mName : std::string();
	std::string aDeletedName = aDialog->GetSelName();
	if (aDeletedName == aCurrentName)
		mCurrentPlayer = nullptr;
	mProfileMgr.DeleteProfile(aDeletedName);
	aDialog->FinishDeleteUser();

	if (mCurrentPlayer == nullptr)
	{
		// the line that is now chosen, or any one
		mCurrentPlayer = mProfileMgr.GetProfile(aDialog->GetSelName());
		if (mCurrentPlayer == nullptr)
			mCurrentPlayer = mProfileMgr.GetAnyProfile();
	}
	mProfileMgr.Save();
	if (mCurrentPlayer == nullptr)
		DoCreateUserDialog();
	mWidgetManager->MarkAllDirty();
	if (mMainMenu != nullptr)
		mMainMenu->SyncPlayerInfo();
}

// 0x405aa0: the furthest stage and level reached, as far as the profiles in use have got
void ThunderballApp::CheckMaxStage()
{
	if (mCurrentPlayer == nullptr)
		return;

	if (mMaxStage < mCurrentPlayer->mMaxStage)
	{
		mMaxStage = mCurrentPlayer->mMaxStage;
		mMaxLevel = mCurrentPlayer->mMaxLevel;
		return;
	}
	if (mCurrentPlayer->mMaxStage == mMaxStage && mMaxLevel < mCurrentPlayer->mMaxLevel)
		mMaxLevel = mCurrentPlayer->mMaxLevel;
}
