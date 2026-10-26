// How the screens of Mummy Maze Deluxe follow each other: WinMM.exe 0x41b160-0x420fxx (the application's helpers and its
// button handler). See MummyApp.h and docs/GAME_NOTES.md.

#include "MummyApp.h"
#include "Board.h"
#include "Screens.h"
#include "Dialogs.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Dialog.h"
#include "Checkbox.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// a dialog with the game's colours and fonts (0x41aa90) whose text is named by its address in the exe
void MummyApp::DoMessage(int theId, const SexyString& theHeader, const SexyString& theLines, int theMode, int theX, int theY, int theW)
{
	Dialog* aDialog = NewDialog(theId, true, theHeader, theLines, theMode == 3 ? GT(0x004955b4) : SexyString(""), theMode);
	aDialog->Resize(theX, theY, theW, aDialog->GetPreferredHeight(theW));
	AddDialog(theId, aDialog);
}

// 0x41e970: the title screen is done: the menu scrolls down
void MummyApp::TitleDone()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;
	ShowMainMenu(0, false);
}

// 0x41b160: the main menu in place of whatever else is on the screen
void MummyApp::ShowMainMenu(int theEntry, bool theFade)
{
	mAdvPyramid = 0;
	mAdvChamber = 0;
	if (mMainMenu != NULL)
		return;
	Widget** aSlots[] = {(Widget**) &mHighScores, (Widget**) &mGamePanel, (Widget**) &mLevelComplete, (Widget**) &mBoard,
		(Widget**) &mVictory, (Widget**) &mSidePanel, (Widget**) &mHighScoreBack};
	for (size_t i = 0; i < sizeof(aSlots) / sizeof(aSlots[0]); i++)
	{
		if (*aSlots[i] != NULL)
		{
			mWidgetManager->RemoveWidget(*aSlots[i]);
			SafeDeleteWidget(*aSlots[i]);
			*aSlots[i] = NULL;
		}
	}
	mMainMenu = new MainMenu(this, theEntry);
	mMainMenu->Resize(0, 0, 0x280, 0x1e0);
	mWidgetManager->AddWidget(mMainMenu);
	if (theFade)
		SetMusicOrder(1, true);
}

// 0x41c340, 0x41b4b0, 0x41c300
void MummyApp::RemoveBoard()
{
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
}

void MummyApp::RemoveGamePanel()
{
	if (mGamePanel != NULL)
	{
		mWidgetManager->RemoveWidget(mGamePanel);
		SafeDeleteWidget(mGamePanel);
		mGamePanel = NULL;
	}
}

// 0x41c380
void MummyApp::RemoveGameOverScroll()
{
	if (mHighScoreBack != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreBack);
		SafeDeleteWidget(mHighScoreBack);
		mHighScoreBack = NULL;
	}
}

// 0x41b7b0: the level-complete screen goes; the next board comes unless the world map takes over
void MummyApp::RemoveLevelComplete()
{
	if (mLevelComplete != NULL)
	{
		mWidgetManager->RemoveWidget(mLevelComplete);
		SafeDeleteWidget(mLevelComplete);
		mLevelComplete = NULL;
	}
	if (mWorldMap == NULL)
		StartBoardScreen();
}

void MummyApp::RemoveWorldMap()
{
	if (mWorldMap != NULL)
	{
		mWidgetManager->RemoveWidget((Widget*) mWorldMap);
		SafeDeleteWidget((Widget*) mWorldMap);
		mWorldMap = NULL;
	}
}

// 0x41e6a0: the side panel and the board of a new game
void MummyApp::StartBoardScreen()
{
	if (mHighScores != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScores);
		SafeDeleteWidget(mHighScores);
		mHighScores = NULL;
	}
	if (mMainMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mMainMenu);
		SafeDeleteWidget(mMainMenu);
		mMainMenu = NULL;
	}
	if (mSidePanel != NULL)
	{
		mWidgetManager->RemoveWidget(mSidePanel);
		SafeDeleteWidget(mSidePanel);
		mSidePanel = NULL;
	}
	RemoveBoard();
	mSidePanel = new SidePanel(this);
	mSidePanel->Resize(0, 0, 0x96, 0x1e0);
	mWidgetManager->AddWidget(mSidePanel);
	mSidePanel->MarkDirty();
	mBoard = new Board(this);
	mBoard->Resize(0x96, 0, 0x1ea, 0x1e0);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToFront(mBoard);
	mWidgetManager->SetFocus(mBoard);
}

// 0x41b4f0: the level is complete: the screen with the score and the way on
void MummyApp::OnLevelComplete()
{
	mBoard->mState = STATE_DONE;
	mLevelComplete = new LevelCompleteScreen(this);
	mLevelComplete->Resize(0x96, 0, 0x1ea, 0x1e0);
	mWidgetManager->AddWidget(mLevelComplete);
	mWidgetManager->SetFocus(mLevelComplete);
}

// 0x4112b0, case 3: the link after a level or on the ending board
void MummyApp::OnContinueLink()
{
	if (mGameMode == MODE_ADVENTURE_DONE)
	{
		if (mAdvPyramid < 0xe)
		{
			LoadAdventure(mCurrentSlot, false);
			RemoveBoard();
			return;
		}
	}
	else
	{
		if (mGameMode != MODE_CLASSIC_DONE)
			return;
		if (mClassicPyramid < 0xe)
		{
			mClassicChamber = 0;
			mClassicPyramid = mClassicPyramid + 1;
			mGameMode = MODE_CLASSIC;
			StartBoardScreen();
			return;
		}
	}
	ShowVictory();
}

// 0x41c410: classic mode: continue the game that was saved, or start a new one
void MummyApp::StartClassicOrContinue()
{
	if (!ClassicSaveExists())
	{
		StartBoardScreen();
		mScore = 0;
		return;
	}
	DoMessage(DIALOG_CONTINUE, GT(0x00494068), GT(0x00494074), 1, 0xa2, 0x52, 0x13c);
}

// 0x41c3c0: the tutorial
void MummyApp::StartTutorial()
{
	if (mMainMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mMainMenu);
		SafeDeleteWidget(mMainMenu);
		mMainMenu = NULL;
	}
	if (mBoard != NULL)
	{
		if (mGameMode == MODE_CLASSIC || mGameMode == MODE_CLASSIC_DONE)
			mBoard->SaveClassic();
		mBoard->mState = STATE_LOST;
	}
	mGameMode = MODE_TUTORIAL;
	StartBoardScreen();
	mScore = 0;
}

// 0x41cc30
void MummyApp::ShowQuitDialog()
{
	DoMessage(DIALOG_QUIT, "", GT(0x00494140), 1, 0xa2, 0x52, 0x13c);
}

// 0x41cfb0: the way back to the menu, with the question that goes with the state of the game
void MummyApp::ShowQuitToMainDialog()
{
	if (mHighScores != NULL)
	{
		KillDialog(DIALOG_QUIT_TO_MAIN);
		if (mWorldMap != NULL)
			RemoveWorldMap();
		ShowMainMenu(2, true);
	}
	SexyString aLines;
	if (mVictory != NULL)
		aLines = GT(0x004942d4);
	else if (mLevelComplete == NULL)
	{
		if (mBoard == NULL)
			aLines = (mGameMode != 2 && mGameMode != 3) ? GT(0x004941e0) : GT(0x00494228);
		else if (mGameMode < 2)
			aLines = (mBoard->mShowingSolution) ? GT(0x00494290) : GT(0x00494228);
		else
			aLines = (mGameMode != 2 && mGameMode != 3) ? GT(0x004941e0) : GT(0x00494228);
	}
	else
		aLines = GT(0x00494290);
	DoMessage(DIALOG_QUIT_TO_MAIN, "", aLines, 1, 0xd9, 0x75, 0x164);
	Dialog* aDialog = GetDialog(DIALOG_QUIT_TO_MAIN);
	if (aDialog != NULL)
		aDialog->mDialogHeader = GT(0x004942c4);
}

// 0x41f3e0: the name for a high score
void MummyApp::ShowRecordScore(const SexyString& theLines)
{
	RecordScoreDialog* aDialog = new RecordScoreDialog(this, theLines);
	aDialog->Resize(0xfa, 0x20, 0x13c, aDialog->GetPreferredHeight(0x13c));
	AddDialog(DIALOG_RECORD_SCORE, aDialog);
}

// 0x41eab0: the options
void MummyApp::ShowOptions()
{
	DoMessage(DIALOG_OPTIONS, GT(0x004941a8), "", 3, 0x99, 0x1a, 0x1e0);
}

// 0x41b5b0: the tips (not ported yet)
void MummyApp::ShowTip(int theWhich)
{
	(void) theWhich;
}

void MummyApp::ShowAdventureSlots()
{
}

void MummyApp::LoadAdventure(int theSlot, bool theFlag)
{
	(void) theSlot;
	(void) theFlag;
}

void MummyApp::OnAdventureDeath()
{
}

void MummyApp::OnWorldMapButton()
{
}

void MummyApp::ShowVictory()
{
	ShowMainMenu(2, true);
}

// 0x41b2f0: the panel with undo, try again, save and quit and abandon hope
void MummyApp::ShowGamePanel()
{
	if (mGamePanel == NULL && mBoard != NULL)
	{
		if (mBoard->mShowingSolution)
		{
			// the solution has played: the high scores, with the game-over scroll over them
			mHighScores = new HighScoreScreen(this);
			mHighScores->Resize(0x96, 0, 0x1ea, 0x1e0);
			mWidgetManager->AddWidget(mHighScores);
			mHighScoreBack = new GameOverScroll(this);
			mHighScoreBack->Resize(0, 0, 0x280, 0x1e0);
			mWidgetManager->AddWidget(mHighScoreBack);
			SetMusicOrder(0x4c, true);
			return;
		}
		mGamePanel = new GamePanel(this);
		mGamePanel->mScrollState = 0;
		mGamePanel->Resize(0, 0, 0x280, 0x1e0);
		mWidgetManager->AddWidget(mGamePanel);
	}
}

// 0x4201e0: the answers of the dialogs
void MummyApp::ButtonDepress(int theId)
{
	if (theId >= 3000 && theId < 4000)
	{
		int anId = theId - 3000;
		if (anId == DIALOG_CONTINUE)
		{
			KillDialog(anId);
			Board::DeleteClassicSave();
			StartBoardScreen();
			mScore = 0;
			return;
		}
		KillDialog(anId);
		return;
	}
	if (theId < 2000 || theId >= 3000)
		return;
	switch (theId)
	{
	case 2000 + DIALOG_NEW_GAME:
		KillDialog(DIALOG_NEW_GAME);
		StartBoardScreen();
		mScore = 0;
		break;
	case 2000 + DIALOG_STARTED_TUTORIAL:
		KillDialog(DIALOG_STARTED_TUTORIAL);
		KillDialog(DIALOG_OPTIONS);
		StartTutorial();
		break;
	case 2000 + DIALOG_QUIT:
		KillDialog(DIALOG_QUIT);
		Shutdown();
		break;
	case 2000 + DIALOG_QUIT_TO_MAIN:
		KillDialog(DIALOG_QUIT_TO_MAIN);
		if (mBoard != NULL)
		{
			if (mGameMode == MODE_CLASSIC || mGameMode == MODE_CLASSIC_DONE)
				mBoard->SaveClassic();
			mBoard->mState = STATE_LOST;
		}
		ShowMainMenu(2, true);
		break;
	case 2000 + DIALOG_OPTIONS:
		KillDialog(DIALOG_OPTIONS);
		break;
	case 2000 + DIALOG_RECORD_SCORE:
	{
		RecordScoreDialog* aDialog = (RecordScoreDialog*) GetDialog(DIALOG_RECORD_SCORE);
		if (mGameMode == 3)
		{
			if (mVictory != NULL)
				mVictory->SetName(aDialog->mNameEdit->mString);
		}
		else if (mHighScores != NULL)
			mHighScores->SetName(aDialog->mNameEdit->mString);
		mLastScoreName = aDialog->mNameEdit->mString;
		KillDialog(DIALOG_RECORD_SCORE);
		break;
	}
	case 2000 + DIALOG_CONTINUE:
		KillDialog(DIALOG_CONTINUE);
		LoadClassic();
		if (mScore != 0)
			mTimesPlayed++;
		Board::DeleteClassicSave();
		StartBoardScreen();
		break;
	default:
		KillDialog(theId - 2000);
		break;
	}
}
