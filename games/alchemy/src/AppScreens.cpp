// The screen flow of WinAlchApp: the selector, the board, the help text, the score screens and the dialogs, and the
// handlers of the buttons (Alchemy.exe 0x410cb9-0x412017, 0x413eea). The registration, update-check, internet-score
// and "more games" dialogs of the original (ids 5, 6, 8, 9, 10, 0xd-0xf, 0x11 and up) belong to the unregistered game
// and its online services (GAME_NOTES.md, "What the original has and the port leaves out", lists their functions): the copy this port
// comes from acts as registered
// and the port has no online features, so they are not translated.

#include "WinAlchApp.h"
#include "Board.h"
#include "IntroScreen.h"
#include "Sidebar.h"
#include "BoardOverlay.h"
#include "Wizard.h"
#include "HelpWidget.h"
#include "HighScores.h"
#include "Dialogs.h"
#include "WidgetManager.h"
#include "Dialog.h"
#include "Checkbox.h"

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

using namespace Sexy;

// 0x4114ff: the wizard's message of three lines (header, text, footer) as a dialog
void WinAlchApp::GiveHelp(const SexyString& theHeader, const SexyString& theLines, const SexyString& theFooter)
{
	DoDialog(DIALOG_HINT, true, theHeader, theLines, theFooter, Dialog::BUTTONS_FOOTER);
}

// 0x410d2a (slot 71): mutes or unmutes (when the count says it must), then the sound button follows the mute count
void WinAlchApp::SetMute(bool wantMuted)
{
	if (wantMuted)
	{
		if (mMuteCount == 0)
			Mute(false);
	}
	else if (mMuteCount == 1)
		Unmute(false);

	UpdateSoundButton();
}

// 0x410cb9 (slot 63): a modal dialog freezes the board's clock
void WinAlchApp::ModalOpen()
{
	if (mBoard != NULL)
		mBoard->Pause(mTimeTrial);
}

// 0x410cd3 (slot 64)
void WinAlchApp::ModalClose()
{
	if (mBoard != NULL)
		mBoard->Unpause(mTimeTrial);
}

// 0x4111fa (slot 61): after the last dialog is gone the board gets the keyboard back
bool WinAlchApp::KillDialog(int theDialogId)
{
	if (SexyApp::KillDialog(theDialogId))
	{
		if (mDialogMap.empty())
			mWidgetManager->SetFocus(mBoard);
		return true;
	}
	return false;
}

// 0x410dc3: the all-time score table
void WinAlchApp::DoHighScoreDialog()
{
	Dialog* aDialog = new HighScoreDialog(this);
	aDialog->Resize(0xb5, 0x32, 0x1ac, aDialog->GetPreferredHeight(0x1ac));
	AddDialog(DIALOG_HIGH_SCORES, aDialog);
}

// 0x410e37: the help text over the board; the effects overlay stays on top of it, and shows the arrow
void WinAlchApp::DoHelp()
{
	KillDialog(DIALOG_OPTIONS);
	if (mHelp == NULL)
	{
		mHelp = new HelpWidget(this);
		mWidgetManager->AddWidget(mHelp);
		mHelp->Resize(BOARD_X, 0, BOARD_WIDTH, SCREEN_HEIGHT);
		mWidgetManager->PutBehind(mHelp, mBoardOverlay);
		mBoardOverlay->mShowHelpArrow = true;
		mBoardOverlay->MarkDirty();
	}
}

// 0x410ee0
void WinAlchApp::RemoveHelpWidget()
{
	if (mHelp != NULL)
	{
		mWidgetManager->RemoveWidget(mHelp);
		SafeDeleteWidget(mHelp);
		mHelp = NULL;
		mBoardOverlay->mShowHelpArrow = false;
		mBoardOverlay->MarkDirty();
	}
}

// 0x410f28: the options dialog (gold buttons, black text)
void WinAlchApp::DoOptions()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	aDialog->Resize(0xcd, 0x32, 0x17c, aDialog->GetPreferredHeight(0x17c));
	AddDialog(DIALOG_OPTIONS, aDialog);
}

// 0x4110bf: a score of the table is the player's: asks for the name
void WinAlchApp::DoRecordScoreDialog(const SexyString& theLines)
{
	RecordScoreDialog* aDialog = new RecordScoreDialog(this, theLines);
	aDialog->Resize(0xfa, 0x20, 0x13c, aDialog->GetPreferredHeight(0x13c));
	AddDialog(DIALOG_RECORD_SCORE, aDialog);
}

// 0x411138: the score table of the game that ended replaces the board's view
void WinAlchApp::DoGameOver()
{
	mBoard->SetVisible(false);

	mHighScores = new HighScoreWidget(this);
	mHighScores->Resize(BOARD_X, 0, BOARD_WIDTH, SCREEN_HEIGHT);
	mWidgetManager->AddWidget(mHighScores);
	mWidgetManager->BringToFront(mBoard->mWizard);
}

// 0x41122e: a new board replaces the selector, the score table and the old board
void WinAlchApp::NewGame()
{
	RemoveHelpWidget();
	mBeforeFirstGame = false;

	if (mHighScores != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScores);
		SafeDeleteWidget(mHighScores);
		mHighScores = NULL;
	}
	if (mIntroScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mIntroScreen);
		SafeDeleteWidget(mIntroScreen);
		mIntroScreen = NULL;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}

	mBoard = new Board(this);
	mBoard->Resize(BOARD_X, 0, BOARD_WIDTH, SCREEN_HEIGHT);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);

	mSidebar->mScoreAnim = 0;
	mSidebar->MarkDirty();
}

// 0x411357: the selector replaces the board and the score table
void WinAlchApp::ShowIntroScreen()
{
	RemoveHelpWidget();

	if (mHighScores != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScores);
		SafeDeleteWidget(mHighScores);
		mHighScores = NULL;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}

	mTutorialMode = false;

	mIntroScreen = new IntroScreen(this);
	mIntroScreen->Resize(BOARD_X, 0, BOARD_WIDTH, SCREEN_HEIGHT);
	mWidgetManager->AddWidget(mIntroScreen);
	mWidgetManager->BringToBack(mIntroScreen);
	mWidgetManager->BringToFront(mBoardOverlay);
}

// 0x411d4f
void WinAlchApp::TryConfirmLoad()
{
	if (Board::HasSavedGame())
		DoDialog(DIALOG_LOAD_GAME, true, GetString("CONFIRM_LOAD1"), GetString("CONFIRM_LOAD2"), "", Dialog::BUTTONS_YES_NO);
}

// 0x411e2a: the new game button: the selector comes up at once when no game is being played
void WinAlchApp::ConfirmNewGame()
{
	if (mIntroScreen == NULL)
	{
		if (mBoard == NULL || mBoard->mState == Board::STATE_GAME_OVER)
		{
			mTutorialMode = false;
			ShowIntroScreen();
		}
		else
			DoDialog(DIALOG_NEW_GAME, true, GetString("CONFIRM_NEW1"), GetString("CONFIRM_NEW2"), "", Dialog::BUTTONS_YES_NO);
	}
}

// 0x411f27
void WinAlchApp::ConfirmTutorial()
{
	if (mBoard == NULL || mBoard->mState == Board::STATE_GAME_OVER)
	{
		mTutorialMode = true;
		NewGame();
	}
	else
		DoDialog(DIALOG_TUTORIAL, true, GetString("CONFIRM_TUTORIAL1"), GetString("CONFIRM_TUTORIAL2"), "", Dialog::BUTTONS_YES_NO);
}

// 0x412017: the quit button; a game with progress is mentioned
void WinAlchApp::ConfirmQuit()
{
	SexyString aLines = GetString("CONFIRM_QUIT1");
	if (mBoard != NULL && mBoard->IsInProgress())
		aLines += GetString("CONFIRM_QUIT2");
	DoDialog(DIALOG_QUIT, true, GetString("QUIT"), aLines, "", Dialog::BUTTONS_YES_NO);
}

// 0x410db6: every button clicks
void WinAlchApp::ButtonPress(int theId)
{
	PlaySample(SOUND_CLICK);
}

// 0x413eea: the panel's buttons (ids 10..16) and the dialogs' (2000 + id yes, 3000 + id no)
void WinAlchApp::ButtonDepress(int theId)
{
	int anId = theId % 10000;
	if (anId >= 2000 && anId < 3000)
	{
		int aDialogId = anId - 2000;
		switch (aDialogId)
		{
		case DIALOG_NEW_GAME:
			KillDialog(DIALOG_NEW_GAME);
			mTutorialMode = false;
			ShowIntroScreen();
			break;
		case DIALOG_QUIT:
			KillDialog(DIALOG_QUIT);
			Shutdown();
			break;
		case DIALOG_OPTIONS:
		{
			OptionsDialog* aDialog = (OptionsDialog*) GetDialog(DIALOG_OPTIONS);
			bool wantWindowed = !aDialog->mFullscreenBox->IsChecked();
			if (mIsWindowed != wantWindowed)
				SwitchScreenMode(wantWindowed);
			EnableCustomCursors(aDialog->mCursorsBox->IsChecked());
			KillDialog(DIALOG_OPTIONS);
			break;
		}
		case DIALOG_RECORD_SCORE:
		{
			RecordScoreDialog* aDialog = (RecordScoreDialog*) GetDialog(DIALOG_RECORD_SCORE);
			mHighScores->NameEntered(aDialog->GetName());
			mLastScoreName = aDialog->GetName();
#ifdef __EMSCRIPTEN__
			SiteScoresChanged(this);
#endif
			KillDialog(DIALOG_RECORD_SCORE);
			break;
		}
		case DIALOG_TUTORIAL:
			KillDialog(DIALOG_TUTORIAL);
			mTutorialMode = true;
			NewGame();
			break;
		case DIALOG_LOAD_GAME:
			KillDialog(DIALOG_LOAD_GAME);
			NewGame();
			mBoard->LoadGame();
			Board::DeleteSavedGame();
			break;
		default:
			KillDialog(aDialogId);
			break;
		}
	}
	else if (anId >= 3000 && anId < 4000)
	{
		int aDialogId = anId - 3000;
		if (aDialogId == DIALOG_LOAD_GAME)
		{
			KillDialog(DIALOG_LOAD_GAME);
			Board::DeleteSavedGame();
		}
		else
			KillDialog(aDialogId);
	}

	switch (theId)
	{
	case BUTTON_NEW_GAME:
		ConfirmNewGame();
		break;
	case BUTTON_OPTIONS:
		DoOptions();
		break;
	case BUTTON_FORGE:
		if (mBoard != NULL)
			mBoard->Discard();
		break;
	case BUTTON_QUIT:
		ConfirmQuit();
		break;
	case BUTTON_HINT:
		if (mBoard != NULL)
			mBoard->ShowPossibleMoves();
		break;
	case BUTTON_SOUND:
		SetMute(mMuteCount == 0);
		PlaySample(SOUND_CLICK);
		break;
	}
}
