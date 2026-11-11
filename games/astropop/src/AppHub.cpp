// AstroPopApp::ButtonDepress (0x431ac0), the hub every button and every box of the game reports to. A button of the
// game's own screens has an id of its own (0x898 .. 0xfb6). A box made by MakeDialog has an id from 100 to 124 and its
// buttons report 2000 + id (the first button: Yes or OK) and 3000 + id (the second: No or Cancel); the original also
// accepts the ids with 10000 or 20000 added (the update check made them). Offsets in the comments are the original's.
//
// Left out (registration, outbound links and the update check are not in the port, docs/GAME_NOTES.md): the register
// screen (buttons 0x44c and 0x84a, the box 0x7a), PopCap's web link (0xce4), the update check (the boxes 0x71 and 0x72
// and the ids 12114 and 22114 of the update flow).

#include "AstroPopApp.h"
#include "AstroDialog.h"
#include "Board.h"
#include "Checkbox.h"
#include "GameText.h"
#include "MainMenu.h"
#include "PowerDialog.h"
#include "UserProfile.h"
#include "WidgetManager.h"
#include <SDL.h>

using namespace Sexy;

// 0x409080: the colour set of the boxes now
int AstroPopApp::GetDialogSet()
{
	if (mBoard != NULL)
		return mBoard->mMode + 2;
	if (mMainMenu == NULL)
		return 0;
	return 1;
}

// 0x408d70: a box goes and the game, if one runs, goes on
void AstroPopApp::CloseDialog(int theId)
{
	KillDialog(theId);
	mOpenDialogs--;
	if (mBoard != NULL)
		mBoard->Pause(false, true);
}

// 0x4291c0: the box that explains the pilot's weapon goes (PowerDialog). Its tick box says whether it is shown again: the
// answer is kept in the profile (setting 0x2a + the classic pilot, 1 for shown). The pictures made for the box go with it
// (the original frees them here, the port in ~PowerDialog). The game goes on.
void AstroPopApp::ClosePowerDialog()
{
	mOpenDialogs--;
	PowerDialog* aDialog = (PowerDialog*) GetDialog(0x7b);
	bool aChecked = aDialog->mCheckbox->IsChecked();
	mUser->SetInt(mClassicPilot + 0x2a, aChecked ? 0 : 1);
	mUser->Save();
	KillDialog(0x7b);
	if (mBoard != NULL)
	{
		mBoard->Pause(false, true);
		mWidgetManager->SetFocus(mBoard);
	}
}

// The confirmation of the button that leaves a game (0xfa8): the words depend on the mode.
static void AskToEndGame(AstroPopApp* theApp)
{
	Board* aBoard = theApp->mBoard;
	int aType = (aBoard != NULL) ? aBoard->m1a60 : 0;
	std::string aHeader = GT(0x005638a4);
	std::string aLines;
	if (aType == 1)
	{
		aLines = GT(0x5638e0);
		if (aBoard->mField193C == 1)
			aLines = GT(0x5638b0);
	}
	else if (aType != 0)
		aLines = GT(0x563830);
	else
	{
		aLines = GT(0x563870);
		aHeader = GT(0x00563860);
	}
	theApp->MakeDialog(theApp->GetDialogSet(), false, 0x6a, -1, -1, 1, aHeader, aLines, "Yes", "No", true);
}

// The OK of the message box (0x6c) returns the keyboard to the Board, or to the player-name box when that is open
static void FocusAfterMessage(AstroPopApp* theApp)
{
	if (theApp->mBoard == NULL || theApp->mOpenDialogs != 0)
	{
		Dialog* aDialog = theApp->GetDialog(0x6b);
		if (aDialog != NULL)
			theApp->mWidgetManager->SetFocus(aDialog);
		return;
	}
	theApp->mWidgetManager->SetFocus(theApp->mBoard);
}

// What follows a box that sent the player back to the selector: the selector comes up (but not in a game of type 0), and
// the Board draws itself again
static void BackToSelector(AstroPopApp* theApp)
{
	if (theApp->mBoard == NULL)
		return;     // (the original reads the Board here: a box that goes to the selector has one)
	if (theApp->mBoard->m1a60 != 0)
		theApp->ShowMainMenu();
	theApp->mBoard->RedrawAfterDialog();
}

// The score of the checkpoint is put into a new classic game that goes on from it (0x848 and 0xfa4)
static void RestoreCheckpointScore(AstroPopApp* theApp)
{
	int aScore = 0;
	theApp->mUser->GetInt(theApp->mClassicPilot + 0x6b, &aScore);
	theApp->mBoard->mScore = aScore;
	theApp->mBoard->m1970 = aScore;
	theApp->mBoard->m1bd8 = theApp->mBoard->m1bdc;
}

// 0x431ac0 (and 0x432730, its entry through the ButtonListener part)
void AstroPopApp::ButtonDepress(int theId)
{
	int aId = theId;
	if (theId >= 20001)
		aId = theId - 20000;
	if (theId > 10000 && theId < 20000)
		aId = theId - 10000;

	int aYes = aId - 2000;
	int aNo = aId - 3000;

	if (aYes >= 100 && aYes <= 0x7c)
	{
		// The first button of a box. The switch is on the id as it came: the ids of the update flow (12114, 22114) and the
		// other ids with 10000 or 20000 added do nothing in the port.
		switch (theId)
		{
		case 0x834:     // 100: restart the level (type 1) or the demonstration (type 0)
			CloseDialog(theId - 2000);
			if (mBoard != NULL && mBoard->m1a60 == 1)
			{
				mBoard->LoadLevel(true);
				mWidgetManager->SetFocus(mBoard);
			}
			else if (mBoard != NULL && mBoard->m1a60 == 0)
			{
				mWidgetManager->RemoveWidget(mBoard);
				SafeDeleteWidget(mBoard);
				LoadPilotImages(0);
				mBoard = new Board(this, &mBus, 0, 1, 1, false, true, false);
				mBoard->Resize(0, 0, 0x280, 0x1e0);
				mWidgetManager->AddWidget(mBoard);
			}
			break;
		case 0x836:     // 102: the options box is accepted
			ApplyOptions(false);
			break;
		case 0x83a:     // 106: END GAME? / END MISSION? yes
			CloseDialog(0x6a);
			if (GetDialog(0x66) != NULL)
				ApplyOptions(true);
			mUser->Save();
			// (the original writes "Quitting level" to its trace log here)
			ShowMainMenu();
			if (mBoard != NULL)
				mBoard->RedrawAfterDialog();
			break;
		case 0x83b:     // 107: the name of a new player is entered
			NewUserOk();
			break;
		case 0x83c:     // 108: the message box is closed
			CloseDialog(0x6c);
			FocusAfterMessage(this);
			break;
		case 0x83d:     // 109: the player is chosen from the list
			UserSelectOk();
			break;
		case 0x83e:     // 110: asks whether to delete the player
			DeleteUser(false);
			break;
		case 0x83f:     // 111: the player is deleted
			DeleteUser(true);
			break;
		case 0x840:     // 112: go on from the checkpoint / play the second mode again
			KillDialog(0x70);
			mOpenDialogs--;
			if (mBoard != NULL && mBoard->m1a60 == 1)
				StartClassic(true, false);
			else
				StartSurvivalGame(mSurvivalPilot);
			break;
		case 0x844:     // 116: continue the saved game
			mFlag65b = true;
			CloseDialog(0x74);
			mFlag6fc = false;
			StartGame(true);
			break;
		case 0x845:     // 117: the hints are on again (and then the box goes like 103 .. 105)
		{
			mUser->ResetHintsShown();
			mUser->SetInt(0x2a, 1);
			mUser->SetInt(0x2b, 1);
			mUser->SetInt(0x2c, 1);
			mUser->SetInt(0x2d, 1);
			mUser->SetInt(0x73, 1);
			if (mBoard != NULL)
				mBoard->m1c08 = true;
			mUser->EnableHints();
			mUser->Save();
			int aValue = 1;
			mUser->GetInt(0x73, &aValue);
			mFlag6fb = (aValue == 1);
			CloseDialog(theId - 2000);
			break;
		}
		case 0x837:     // 103
		case 0x838:     // 104
		case 0x839:     // 105
			CloseDialog(theId - 2000);
			break;
		case 0x846:     // 118
		case 0x847:     // 119
			KillDialog(theId == 0x846 ? 0x76 : 0x77);
			mOpenDialogs--;
			if (mBoard != NULL)
				mBoard->TakeFocus();
			break;
		case 0x848:     // 120: the checkpoint game starts over from its level
		{
			CloseDialog(0x78);
			int aBest = 0;
			mUser->GetInt(mClassicPilot + 8, &aBest);
			mUser->SetInt(0x71, 0);
			StartClassicGame(((aBest + ((aBest >> 31) & 3)) >> 2) * 4 + 1, false, false);
			RestoreCheckpointScore(this);
			break;
		}
		case 0x849:     // 121
			CloseDialog(0x79);
			break;
		case 0x84b:     // 123: the power of the pilot is used
			ClosePowerDialog();
			if (mBoard != NULL)
				mBoard->BeginPlay();
			break;
		case 0x84c:     // 124: leave the game
			CloseDialog(0x7c);
			Shutdown();
			break;
		}
		// (0x841 / 0x842: the update check, 0x84a: the register screen; out of the port)
		return;
	}

	if (aNo >= 100 && aNo <= 0x7c)
	{
		// The second button of a box
		int aRaw = theId - 3000;
		switch (aNo)
		{
		case 0x6a:      // END GAME? / END MISSION?: no
		case 0x75:
		case 0x7c:      // the quit box: no
			CloseDialog(aRaw);
			break;
		case 0x6b:      // the name box: cancel (a first player cannot cancel)
		case 0x6e:
		case 0x6f:
			if (aRaw == 0x6b && mUserNames.empty())
			{
				ShowLowMessageDialog(GT(0x00563954), GT(0x563964));
				return;
			}
			mOpenDialogs--;
			KillDialog(aRaw);
			if (mMainMenu != NULL)
				mWidgetManager->SetFocus(mMainMenu);
			break;
		case 0x6d:      // the player list: the same as its first button
			UserSelectOk();
			break;
		case 0x70:      // the checkpoint box: no
			KillDialog(0x70);
			mOpenDialogs--;
			BackToSelector(this);
			break;
		case 0x74:      // the saved game is dropped
			CloseDialog(0x74);
			mUser->ResetHintsShown();
			mUser->ResetGame();
			mUser->Save();
			mFlag6fc = false;
			StartGame(false);
			break;
		case 0x78:
			CloseDialog(0x78);
			StartClassicGame(1, false, false);
			break;
		}
		// (0x71 / 0x72 and the ids of the update flow, 0x7a: the register box; out of the port)
		return;
	}

	switch (theId)
	{
	case 0x898:     // the title's button
		OnClickToPlay();
		break;
	case 0xfa1:     // the options button of the game
		ShowOptions(true);
		break;
	case 0xfa8:     // the end game button
		AskToEndGame(this);
		break;
	case 0xfa2:     // the OK of a hint
		if (mBoard != NULL)
			mBoard->DismissHint();
		break;
	case 0xfa4:     // play again
		if (mBoard == NULL)
			break;
		if (mBoard->m1a60 != 1)
		{
			StartSurvivalGame(mSurvivalPilot);
			mFlag6fc = true;
			break;
		}
		mUser->SetInt(0x71, 0);
		StartClassic(true, true);
		{
			int aBest = 0;
			mUser->GetInt(mClassicPilot + 8, &aBest);
			if (((aBest + ((aBest >> 31) & 3)) >> 2) * 4 > 3)
				RestoreCheckpointScore(this);
		}
		mFlag6fc = true;
		break;
	case 0xfa5:     // back to the selector
		BackToSelector(this);
		break;
	case 0xfa6:     // the help button
		ShowHelp();
		break;
	case 0xfa7:
		if (mBoard != NULL)
			mBoard->CloseOverlay();
		break;
	case 0xfb4:     // the full-screen help is closed
		CloseFullScreenHelp();
		break;
	case 0xfb6:     // the story screen is closed
		SafeDeleteWidget(mScreen654);
		mWidgetManager->RemoveWidget(mScreen654);
		mScreen654 = NULL;
		if (mBoard == NULL)
			ShowMainMenu();
		break;
	}
	// (0x44c: the register screen, 0xce4: PopCap's web link; out of the port)
}
