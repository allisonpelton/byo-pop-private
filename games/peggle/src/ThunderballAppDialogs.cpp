// ThunderballAppDialogs.cpp: the part of ThunderballApp that makes the game's dialogs and answers their buttons. The original
// keeps its tweakable numbers in ModVal calls (each returns the number it was compiled with); they are named constants here,
// each with the line of ThunderballApp.cpp its call names.

#include "Dialog.h"
#include "EditWidget.h"
#include "MainMenu.h"
#include "NewUserDialog.h"
#include "NotTranslated.h"
#include "ThunderCommon.h"
#include "ThunderDialog.h"
#include "ThunderballApp.h"
#include "WidgetManager.h"

using namespace Sexy;

static const int NEW_DIALOG_LEAST_WIDTH = 350;		// the width a dialog is given (0x15e) before it is placed
static const int CONFIRM_QUIT_X = 235;				// line 191
static const int CONFIRM_QUIT_Y = 280;				// line 192
static const int SCROLL_FROM_ABOVE = 1;
static const int SCROLL_FROM_BELOW = -1;
static const int CERTIFICATE_X = 30;				// lines 202 and 203: where the certificate of the first run comes to rest, when
static const int CERTIFICATE_Y = 30;				// the main menu is up

// 0x40c0a0
Dialog* ThunderballApp::NewDialog(int theDialogId, bool isModal, const std::string& theDialogHeader,
								  const std::string& theDialogLines, const std::string& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new ThunderDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	PositionDialog(aDialog, NEW_DIALOG_LEAST_WIDTH, false, -1);
	return aDialog;
}

// 0x405f20
Dialog* ThunderballApp::DoDialogScroll(int theDialogId, bool isModal, const std::string& theDialogHeader,
									   const std::string& theDialogLines, const std::string& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = SexyAppBase::DoDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	static_cast<ThunderDialog*>(aDialog)->DoScroll(SCROLL_FROM_ABOVE);
	return aDialog;
}

// 0x405f60
void ThunderballApp::DoScrollOff(int theDialogId)
{
	Dialog* aDialog = GetDialog(theDialogId);
	if (aDialog != nullptr)
		static_cast<ThunderDialog*>(aDialog)->DoScrollOff(true);
}

// 0x41c960
void ThunderballApp::DoConfirmQuitDialog()
{
	Dialog* aDialog = DoDialogScroll(DIALOG_CONFIRM_QUIT, true, "Quit Peggle?", "Are you sure you want to\nquit the game?", "",
									 Dialog::BUTTONS_YES_NO);
	aDialog->mX = CONFIRM_QUIT_X;
	aDialog->mY = CONFIRM_QUIT_Y;
	ThunderDialog* aThunderDialog = static_cast<ThunderDialog*>(aDialog);
	aThunderDialog->DoScroll(SCROLL_FROM_BELOW);
	aThunderDialog->mHangsOnChain = false;
}

// 0x4084a0
void ThunderballApp::FinishConfirmQuitDialog(bool theYes)
{
	if (theYes)
	{
		KillDialog(DIALOG_CONFIRM_QUIT);
		Shutdown();
	}
	else
		DoScrollOff(DIALOG_CONFIRM_QUIT);
}

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

// 0x4302a0: the dialogs that are not translated yet name their place
void ThunderballApp::DialogButtonDepress(int theDialogId, int theButtonId)
{
	bool aYes = theButtonId == Dialog::ID_YES;
	switch (theDialogId)
	{
	case DIALOG_CONFIRM_QUIT:
		FinishConfirmQuitDialog(aYes);
		return;
	case DIALOG_CREATE_USER:
		FinishCreateUserDialog(aYes);
		return;
	case DIALOG_NAME_ERROR:
	case DIALOG_NAME_ERROR_2:
		FinishNameErrorDialog(theDialogId);
		return;
	case DIALOG_CONFIRM_DELETE_USER:
	case DIALOG_CONFIRM_MAIN_MENU:
	case DIALOG_CONFIRM_RESTART_LEVEL:
	case DIALOG_CONFIRM_NEW_CHALLENGE:
	case DIALOG_CONFIRM_RESTART_ADVENTURE:
	case 0xd:
	case DIALOG_TIP:
	case DIALOG_HIGH_SCORE_ENTRY:
	case DIALOG_OPTIONS:
	case DIALOG_RENAME_USER:
	case DIALOG_USER:
	case 0x1b:
	case 0x22:
		NOT_TRANSLATED("ThunderballApp::DialogButtonDepress for this dialog");
	default:
		DoScrollOff(theDialogId);
		break;
	}
}

// 0x40c140: the options dialog goes over the screen that was just put up
void ThunderballApp::BringDialogsToFront()
{
	if (GetDialogCount() != 0)
		NOT_TRANSLATED("ThunderballApp::BringDialogsToFront with a dialog up (brings the options dialog to the front)");
}

// 0x405c30
void ThunderballApp::PauseBoard(bool thePause)
{
	if (mBoard != nullptr)
		NOT_TRANSLATED("ThunderballApp::PauseBoard with a board up");
}

// 0x405b50
void ThunderballApp::ResetTwoPlayerStats()
{
	mUnknown880 = 0;
	mUnknown87c = 0;
	mUnknown888 = 0;
	mUnknown884 = 0;
	mUnknown890 = 0;
	mUnknown88c = 0;
	mUnknown894 = -1;
	mUnknown898 = 0;
	mTwoPlayerStats[0].Reset();
	mTwoPlayerStats[1].Reset();
}

// 0x41c840
void ThunderballApp::FinishOptionsDialog(bool theApply, bool theScrollOff)
{
	if (GetDialog(DIALOG_OPTIONS) != nullptr)
		NOT_TRANSLATED("ThunderballApp::FinishOptionsDialog with the options dialog up");
}

// The dialogs that are not translated yet
void ThunderballApp::DoOptionsDialog()
{
	NOT_TRANSLATED("ThunderballApp::DoOptionsDialog");
}

void ThunderballApp::DoUserDialog()
{
	NOT_TRANSLATED("ThunderballApp::DoUserDialog");
}
