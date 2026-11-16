// ThunderballAppDialogs.cpp: the part of ThunderballApp that makes the game's dialogs and answers their buttons. The original
// keeps its tweakable numbers in ModVal calls (each returns the number it was compiled with); they are named constants here,
// each with the line of ThunderballApp.cpp its call names.

#include "Dialog.h"
#include "NotTranslated.h"
#include "ThunderCommon.h"
#include "ThunderDialog.h"
#include "ThunderballApp.h"

using namespace Sexy;

static const int NEW_DIALOG_LEAST_WIDTH = 350;		// the width a dialog is given (0x15e) before it is placed
static const int CONFIRM_QUIT_X = 235;				// line 191
static const int CONFIRM_QUIT_Y = 280;				// line 192
static const int SCROLL_FROM_ABOVE = 1;
static const int SCROLL_FROM_BELOW = -1;

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

// 0x4302a0: the dialogs that are not translated yet name their place
void ThunderballApp::DialogButtonDepress(int theDialogId, int theButtonId)
{
	bool aYes = theButtonId == Dialog::ID_YES;
	switch (theDialogId)
	{
	case DIALOG_CONFIRM_QUIT:
		FinishConfirmQuitDialog(aYes);
		return;
	case DIALOG_CONFIRM_DELETE_USER:
	case DIALOG_CONFIRM_MAIN_MENU:
	case DIALOG_CONFIRM_RESTART_LEVEL:
	case DIALOG_CONFIRM_NEW_CHALLENGE:
	case DIALOG_CONFIRM_RESTART_ADVENTURE:
	case DIALOG_CREATE_USER:
	case DIALOG_NAME_ERROR:
	case DIALOG_NAME_ERROR_2:
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
