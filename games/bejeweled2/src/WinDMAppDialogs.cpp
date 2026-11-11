// WinDMAppDialogs.cpp: the dialogs of WinDMApp: how they are made and set up (NewDialog, SetupDialog, AddDialog), what
// the buttons of every dialog do (ButtonDepress: 2000 + the dialog's id for yes and OK, 3000 + the id for no and cancel)
// and the Do... / Finish... pairs that open a dialog and act on its answer. The original's registration, update check and
// announcement dialogs, and its "more games" screen, are not ported.
#include "WinDMApp.h"

#include "Board.h"
#include "Dialog.h"
#include "EditWidget.h"
#include "FancyBackdrop.h"
#include "HelpScreen.h"
#include "HighScoreNameDialog.h"
#include "HighScoreWidget.h"
#include "Insets.h"
#include "MainMenu.h"
#include "OptionsDialog.h"
#include "PodDialog.h"
#include "Profile.h"
#include "Res.h"
#include "UserDialogs.h"
#include "WidgetManager.h"
#include "SiteProfiles.h"

using namespace Sexy;

// the dialogs of the user list, which several functions share (WinDMApp.h, DialogId)
static const int DIALOG_ENTER_NAME_ID = 0x29;

// 0x5dbf24: the fonts, the margins and the place of a dialog; theWidth is in the 1024x768 mode's pixels
void WinDMApp::SetupDialog(Dialog* theDialog, int theWidth)
{
	theDialog->SetButtonFont(FONT_CONTINUUM_MED_14);
	theDialog->SetHeaderFont(FONT_CONTINUUM_MED_18);
	theDialog->SetLinesFont(FONT_2);
	theDialog->mContentInsets = Insets(ScaleNum(0x22, 0), ScaleNum(0x10, 0), ScaleNum(0x22, 0), ScaleNum(0x22, 0));
	theDialog->mSpaceAfterHeader = ScaleNum(0x1c, 0);

	int aCenterX;
	if (mBoard == nullptr || mBoard->mScreenState != 1)
		aCenterX = mWidth / 2;
	else
		aCenterX = ScaleNum(0x270, 0);
	int aHeight = theDialog->GetPreferredHeight(ScaleNum(theWidth, 0));
	theDialog->Resize(aCenterX - ScaleNum(theWidth, 0) / 2, 0x3c, ScaleNum(theWidth, 0), aHeight);
}

// 0x5dce51: every dialog of the game is a PodDialog
Dialog* WinDMApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
	const SexyString& theDialogFooter, int theButtonMode)
{
	PodDialog* aDialog = new PodDialog(this, theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	SetupDialog(aDialog, 0x1a4);
	return aDialog;
}

// 0x5dc03b: a modal dialog takes the keyboard focus
void WinDMApp::AddDialog(int theDialogId, Dialog* theDialog)
{
	SexyApp::AddDialog(theDialogId, theDialog);
	if (theDialog->mIsModal)
		mWidgetManager->SetFocus(theDialog);
}

// a dialog with one button, its text "OK" (the profile errors, the name conflicts)
Dialog* WinDMApp::ShowMessage(int theDialogId, const std::string& theHeader, const std::string& theLines, int theButtonMode)
{
	return DoDialog(theDialogId, true, theHeader, theLines, "OK", theButtonMode);
}

// 0x5e1ed9: a button of a dialog was pressed and let go
void WinDMApp::ButtonDepress(int theId)
{
	int aMod = theId % 10000;
	if (aMod < 2000 || aMod >= 4000)
		return;

	if (aMod >= 3000)
	{
		// the no and cancel buttons (and the Escape key)
		int aDialogId = theId - 3000;
		switch (aDialogId)
		{
		case 30:
			FinishConfirmDeleteUserDialog(false);
			return;
		case 3:
			static_cast<OptionsDialog*>(GetDialog(DIALOG_OPTIONS))->FinishOptionVerify(false);
			break;
		case 17:
		{
			// "Load Game?": NO starts a new game and loses the saved one; the cancel button goes back to the menu
			PodDialog* aDialog = static_cast<PodDialog*>(GetDialog(17));
			bool anEscaped = aDialog->mEscaped;
			KillDialog(17);
			if (!anEscaped)
			{
				DoNewGame();
				mBoard->DeleteSavedGame();
			}
			else
				mMainMenu->CancelNewGame();
			return;
		}
		case 27:
			FinishCreateUserDialog(false);
			return;
		case 29:
			FinishRenameUserDialog(false);
			return;
		case 32:
			FinishUserDialog(false);
			return;
		case 34:
			FinishPuzzlePackSelector(false);
			return;
		case 35:
			FinishPuzzleRatingDialog(false);
			return;
		case 38:
		case 39:
			mBoard->mHintPiece = 0;
			break;
		case 40:
			FinishHelpScreen(false);
			return;
		default:
			break;
		}
		KillDialog(aDialogId);
		return;
	}

	// the yes and OK buttons
	int aDialogId = theId - 2000;
	switch (aDialogId)
	{
	case 0:
		mBoard->NewGame();
		break;
	case 1:
		PlaySample(SOUND_GOODBYE);
		mMainMenu->StartShutdownTimer();
		break;
	case 2:
		FinishOptionsDialog(true);
		return;
	case 3:
		static_cast<OptionsDialog*>(GetDialog(DIALOG_OPTIONS))->FinishOptionVerify(true);
		break;
	case 10:
	{
		// the name of a new high score
		Dialog* aDialog = GetDialog(10);
		const SexyString& aName = static_cast<HighScoreNameDialog*>(aDialog)->mEdit->mString;
		mHighScoreWidget->SetHighScoreName(aName);
		mLastScoreName = aName;
		break;
	}
	case 17:
		KillDialog(17);
		DoNewGame();
		mBoard->LoadGame();
		return;
	case 22:
		KillDialog(22);
		mBoard->UndoUntilSolvable();
		return;
	case 23:
		KillDialog(23);
		mBoard->TryShowHint();
		return;
	case 24:
		KillDialog(24);
		mBoard->ShowNextPuzzleMove();
		return;
	case 25:
	case 26:
	case 38:
	case 39:
	{
		// a one-time explanation: unless the player closed it with Escape, it does not come again
		PodDialog* aDialog = static_cast<PodDialog*>(GetDialog(aDialogId));
		if (!aDialog->mEscaped)
		{
			int aHint;
			if (aDialogId == 26)
				aHint = 5;
			else if (aDialogId == 25)
				aHint = 4;
			else if (aDialogId == 38)
				aHint = 1;
			else
				aHint = 2;
			mProfile->DisableHint(aHint);
		}
		mBoard->mHintPiece = 0;
		break;
	}
	case 27:
		FinishCreateUserDialog(true);
		return;
	case 29:
		FinishRenameUserDialog(true);
		return;
	case 30:
		FinishConfirmDeleteUserDialog(true);
		return;
	case 32:
		FinishUserDialog(true);
		return;
	case 33:
		KillDialog(33);
		mHighScoreWidget->ResetHighScores();
		return;
	case 34:
		FinishPuzzlePackSelector(true);
		return;
	case 35:
		FinishPuzzleRatingDialog(true);
		return;
	case 40:
		FinishHelpScreen(static_cast<HelpScreen*>(GetDialog(40))->mNeverShowAgain);
		return;
	case 41:
	{
		// "Enter Name" was answered: back to the name of the new user
		KillDialog(DIALOG_ENTER_NAME_ID);
		CreateUserDialog* aDialog = static_cast<CreateUserDialog*>(GetDialog(27));
		if (aDialog != nullptr)
			mWidgetManager->SetFocus(aDialog->mEdit);
		return;
	}
	default:
		break;
	}
	KillDialog(aDialogId);
}

// 0x5dc2ed
void WinDMApp::DoUserDialog()
{
	KillDialog(DIALOG_USER_LIST);
	UserDialog* aDialog = new UserDialog();
	SetupDialog(aDialog, 0x1e0);
	AddDialog(DIALOG_USER_LIST, aDialog);
}

// 0x5dc352
void WinDMApp::DoCreateUserDialog()
{
	KillDialog(DIALOG_CREATE_USER);
	CreateUserDialog* aDialog = new CreateUserDialog(false);
	SetupDialog(aDialog, 0x17c);
	AddDialog(DIALOG_CREATE_USER, aDialog);
#ifdef __EMSCRIPTEN__
	SiteFillNewUser(aDialog->mEdit);	// the site's player, when the page's setting says so (SiteProfiles.h)
#endif
	mWidgetManager->SetFocus(aDialog->mEdit);
}

// 0x5dc3c9
void WinDMApp::DoRenameUserDialog(const std::string& theOldName)
{
	KillDialog(DIALOG_RENAME_USER);
	CreateUserDialog* aDialog = new CreateUserDialog(true);
	SetupDialog(aDialog, 0x17c);
	AddDialog(DIALOG_RENAME_USER, aDialog);
	aDialog->SetName(theOldName);
	mWidgetManager->SetFocus(aDialog->mEdit);
}

// 0x5dc1f6
void WinDMApp::DoPuzzlePackSelector()
{
	KillDialog(DIALOG_PUZZLE_PACKS);
	PuzzlePackSelector* aDialog = new PuzzlePackSelector();
	SetupDialog(aDialog, 0x1e0);
	AddDialog(DIALOG_PUZZLE_PACKS, aDialog);
}

// 0x5df7bc: "Who are you?" was answered
void WinDMApp::FinishUserDialog(bool theOk)
{
	UserDialog* aDialog = static_cast<UserDialog*>(GetDialog(DIALOG_USER_LIST));
	if (aDialog == nullptr)
		return;
	if (theOk)
	{
		if (!mProfile->LoadProfile(aDialog->GetSelName()))
			ShowMessage(DIALOG_MESSAGE, "PROFILE ERROR", "There was an error loading that profile. It may need to be deleted and recreated.", 3);
		mWidgetManager->MarkAllDirty();
		if (mMainMenu != nullptr)
			mMainMenu->SyncProfile();
#ifdef __EMSCRIPTEN__
		SiteProfilesChanged(this);
#endif
	}
	KillDialog(DIALOG_USER_LIST);
}

// 0x5df8cb: the name of a new user was entered, or the dialog was cancelled
void WinDMApp::FinishCreateUserDialog(bool theOk)
{
	CreateUserDialog* aDialog = static_cast<CreateUserDialog*>(GetDialog(DIALOG_CREATE_USER));
	if (aDialog == nullptr)
		return;
	std::string aName = aDialog->GetName();

	// without any user the game wants a name
	if (mProfile->mName.empty() && (!theOk || aName.empty()))
	{
		ShowMessage(DIALOG_ENTER_NAME_ID, "Enter Name",
			"Enter your name to create a new user profile for storing high score data and games in progress.", 3);
		return;
	}
	if (theOk && aName.empty())
	{
		ShowMessage(DIALOG_ENTER_NAME_ID, "Enter Name",
			"Enter your name to create a new user profile for storing high score data and games in progress.", 3);
		return;
	}

	if (!theOk)
	{
		KillDialog(DIALOG_CREATE_USER);
		return;
	}
	if (!mProfile->CreateProfile(aName))
	{
		ShowMessage(DIALOG_MESSAGE, "Name Conflict", "The name you entered is already being used.  Please enter a unique player name.", 3);
		return;
	}
	if (mProfile->mNames.size() == 1 && mMainMenu != nullptr)
		mMainMenu->mWelcomeTimer = 20;
	KillDialog(DIALOG_USER_LIST);
	KillDialog(DIALOG_CREATE_USER);
	mWidgetManager->MarkAllDirty();
	if (mMainMenu != nullptr)
		mMainMenu->SyncProfile();
#ifdef __EMSCRIPTEN__
	SiteProfilesChanged(this);
#endif
}

// 0x5dfae8
void WinDMApp::DoConfirmDeleteUserDialog(const std::string& theName)
{
	KillDialog(DIALOG_DELETE_USER);
	DoDialog(DIALOG_DELETE_USER, true, "Are You Sure?", StrFormat("This will permanently remove '%s' from the player roster!", theName.c_str()), "", 1);
}

// 0x5dda71
void WinDMApp::FinishConfirmDeleteUserDialog(bool theYes)
{
	KillDialog(DIALOG_DELETE_USER);
	if (!theYes)
		return;
	UserDialog* aDialog = static_cast<UserDialog*>(GetDialog(DIALOG_USER_LIST));
	if (aDialog == nullptr)
		return;

	mProfile->DeleteProfile(aDialog->GetSelName());
	aDialog->FinishDeleteUser();
	mProfile->GetAnyProfile();
	mProfile->WriteProfileList();
	WriteToRegistry();
	if (mProfile->mName.empty())
		DoCreateUserDialog();
	mWidgetManager->MarkAllDirty();
	if (mMainMenu != nullptr)
		mMainMenu->SyncProfile();
#ifdef __EMSCRIPTEN__
	SiteProfilesChanged(this);
#endif
}

// 0x5dfb97
void WinDMApp::FinishRenameUserDialog(bool theOk)
{
	if (!theOk)
	{
		KillDialog(DIALOG_RENAME_USER);
		return;
	}
	UserDialog* aUserDialog = static_cast<UserDialog*>(GetDialog(DIALOG_USER_LIST));
	CreateUserDialog* aDialog = static_cast<CreateUserDialog*>(GetDialog(DIALOG_RENAME_USER));
	if (aUserDialog == nullptr || aDialog == nullptr)
		return;

	std::string anOldName = aUserDialog->GetSelName();
	std::string aNewName = aDialog->GetName();
	if (anOldName.empty() || aNewName.empty())
	{
		KillDialog(DIALOG_RENAME_USER);
		return;
	}
	bool isCurrent = strcasecmp(mProfile->mName.c_str(), anOldName.c_str()) == 0;
	if (!mProfile->RenameProfile(anOldName, aNewName))
	{
		ShowMessage(DIALOG_MESSAGE, "Name Conflict", "The name you entered is already being used.  Please enter a unique player name.", 3);
		return;
	}
	if (isCurrent)
		mProfile->LoadProfile(aNewName);
	aUserDialog->FinishRenameUser(aNewName);
	mWidgetManager->MarkAllDirty();
	if (mMainMenu != nullptr)
		mMainMenu->SyncProfile();
	KillDialog(DIALOG_RENAME_USER);
#ifdef __EMSCRIPTEN__
	SiteProfilesChanged(this);
#endif
}

// 0x5df3d7: the one-line instructions of the first game
void WinDMApp::ShowInstructionsDialog()
{
	Dialog* aDialog = NewDialog(DIALOG_MESSAGE, false, "", GetString("INSTRUCTIONDIALOG"), "", 0);
	aDialog->mContentInsets = Insets(0x10, 0x18, 0x10, 0x18);
	aDialog->Resize(0xc9, 0x26, 0x1a2, aDialog->GetPreferredHeight(0x1a2));
	AddDialog(DIALOG_MESSAGE, aDialog);
}

// 0x5df4d9
void WinDMApp::ShowFullInstructionsDialog()
{
	Dialog* aDialog = NewDialog(DIALOG_FULL_INSTRUCTIONS, true, GetString("FULL_INSTRUCTIONS1"), GetString("FULL_INSTRUCTIONS2"),
		GetString("DONE"), 3);
	aDialog->mTextAlign = -1;
	aDialog->Resize(0x20, 8, mWidth - 0x40, aDialog->GetPreferredHeight(mWidth - 0x40));
	AddDialog(DIALOG_FULL_INSTRUCTIONS, aDialog);
}

// 0x5df614: the display cannot show the game in a window
void WinDMApp::ShowColorDepthDialog()
{
	DoDialog(DIALOG_FULL_INSTRUCTIONS, true, GetString("COLOR_DEPTH_EXPLANATION1"), GetString("COLOR_DEPTH_EXPLANATION2"),
		GetString("DIALOG_BUTTON_OK"), 3);
}

// 0x5df168
void WinDMApp::TryConfirmLoad()
{
	Dialog* aDialog = DoDialog(DIALOG_LOAD_GAME, true, "Load Game?",
		"Do you want to continue your last game?\nIf you press NO you will start a new game, but your old game will be lost.", "", 1);
	static_cast<PodDialog*>(aDialog)->SetHasCancel(true);
	SetupDialog(aDialog, 0x1cc);
}

// 0x5e119e
void WinDMApp::ConfirmNewGame()
{
	if (mBoard->mState == 7)
		NewGame();
	else
		DoDialog(DIALOG_NEW_GAME, true, GetString("NEWGAMEDIALOG1"), GetString("NEWGAMEDIALOG2"), "", 1);
}

// 0x5df216
void WinDMApp::ConfirmResetHighScores()
{
	Dialog* aDialog = DoDialog(DIALOG_RESET_HIGH_SCORES, true, "Reset Scores?",
		"Are you sure you want to reset the high score table?\n\nThis will remove all high scores that have been recorded.", "", 1);
	SetupDialog(aDialog, (mWidth == 640 ? 0x28 : 0) + 400);
}

// 0x5df2d0
void WinDMApp::ConfirmQuit()
{
	std::string aLines = GetString("CONFIRMQUIT1");
	if (mBoard != nullptr && mBoard->CanPlay())
		aLines += GetString("CONFIRMQUIT2");
	DoDialog(DIALOG_QUIT, true, "Quit?", aLines, "", 1);
}

// 0x5e0aa4: a game of the mode the menu chose
void WinDMApp::NewGame()
{
	if (mGameMode == 2)
	{
		ValidatePuzzlePack();
		if (mPuzzlePacks.size() != 1)
		{
			DoPuzzlePackSelector();
			return;
		}
		LoadGalaxyData();
	}
	else
	{
		if (mGameMode == 3)
		{
			DoNewGame();
			mBoard->LoadGame();
			return;
		}
		if (Board::HasSavedGame())
		{
			TryConfirmLoad();
			return;
		}
	}
	DoNewGame();
}

// 0x5e15ea: a puzzle pack was chosen, or the dialog was cancelled
void WinDMApp::FinishPuzzlePackSelector(bool theOk)
{
	PuzzlePackSelector* aDialog = static_cast<PuzzlePackSelector*>(GetDialog(DIALOG_PUZZLE_PACKS));
	if (aDialog == nullptr)
		return;
	if (!theOk)
	{
		mMainMenu->CancelNewGame();
		mMainMenu->mState = 1;
	}
	else
	{
		mProfile->mPuzzlePack = aDialog->GetSelectedPack();
		LoadGalaxyData();
		DoNewGame();
	}
	KillDialog(DIALOG_PUZZLE_PACKS);
}
