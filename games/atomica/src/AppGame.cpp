// The app's game flow: starting a game, the dialogs, the end (Atomica.exe 0x405b10-0x406240).

#include "AtomicaApp.h"
#include <cstdio>
#include <vector>
#include "Board.h"
#include "Grid.h"
#include "Screens.h"
#include "Menu.h"
#include "Dialogs.h"
#include "Buffer.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x405b10: a game starts
void AtomicaApp::StartGame()
{
	if (mBoard == NULL)
	{
		mBoard = new Board(this);
		mBoard->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mBoard);
		mBoard->Start();
	}
	mB3ea0 = false;
}

// 0x405bd0: the quit box
void AtomicaApp::ShowQuit()
{
	if (mQuitDialog == NULL)
	{
		QuitDialog* aDialog = new QuitDialog(this);
		mQuitDialog = aDialog;
		mWidgetManager->AddWidget(aDialog);
	}
}

// 0x405c40: "confirm new game"
void AtomicaApp::ShowConfirm()
{
	if (mConfirmDialog == NULL)
	{
		ConfirmDialog* aDialog = new ConfirmDialog(this);
		mConfirmDialog = aDialog;
		mWidgetManager->AddWidget(aDialog);
	}
}

// 0x405d00: the game options
void AtomicaApp::ShowOptions()
{
	if (mOptionsDialog == NULL)
	{
		OptionsDialog* aDialog = new OptionsDialog(this);
		mOptionsDialog = aDialog;
		mWidgetManager->AddWidget(aDialog);
	}
}

// 0x424430 (button 9): the hints that were shown (the original keeps them in app+0x406c) are shown again; the ones this
// game has shown already stay shown
void AtomicaApp::ResetHints()
{
	for (int i = 0; i < 0xff; i++)
		mHintsShown[i] = false;
}

// 0x422d40: the high scores box
void AtomicaApp::ShowHighScores()
{
	if (mHighScoresBox == NULL)
	{
		HighScoresBox* aBox = new HighScoresBox(this);
		mHighScoresBox = aBox;
		aBox->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(aBox);
		mWidgetManager->BringToFront(aBox);
		mWidgetManager->BringToFront(aBox->mReturn);
	}
}

// 0x41e440: a score that earns a place or the best of the day gets the entry box
void AtomicaApp::ShowHighScoreEntry(int theScore)
{
	if (mScoreDialog == NULL)
	{
		mSndNewHighScore->Play();
		HighScoreDialog* aDialog = new HighScoreDialog(this, theScore);
		mScoreDialog = aDialog;
		mWidgetManager->AddWidget(aDialog);
	}
}

// 0x405ed0: HS.CFG holds the two tables, 208 bytes each (5 scores, 5 names, the best of the day, its name and its date)
void AtomicaApp::ReadHighScores()
{
	Buffer aBuffer;
	if (ReadBufferFromFile("HS.CFG", &aBuffer) && (aBuffer.GetDataLen() >= 2 * (int) (4 * 5 + 150 + 4 + 30 + 4)))
	{
		mScoreTables[0].Read(&aBuffer);
		mScoreTables[1].Read(&aBuffer);
	}
	else
	{
		mScoreTables[0].Reset();
		mScoreTables[1].Reset();
	}
}

// 0x405f30
void AtomicaApp::WriteHighScores()
{
	Buffer aBuffer;
	mScoreTables[0].Write(&aBuffer);
	mScoreTables[1].Write(&aBuffer);
	WriteBufferToFile("HS.CFG", &aBuffer);
}

// port: the saved game is a file in the user's folder
bool AtomicaApp::HasSavedGame()
{
	Buffer aBuffer;
	return ReadBufferFromFile("SAVEGAME.DAT", &aBuffer) && (aBuffer.GetDataLen() > 8);
}

// 0x405fc0: the saved game holds the mode and the board (the original does not save the expert flag)
void AtomicaApp::SaveGame()
{
	if (mBoard == NULL)
		return;
	FILE* aFile = tmpfile();
	if (aFile == NULL)
		return;
	fwrite(&mMode, 4, 1, aFile);
	mBoard->Save(aFile);
	long aSize = ftell(aFile);
	rewind(aFile);
	std::vector<unsigned char> aData((size_t) aSize);
	if (aSize > 0)
		fread(&aData[0], 1, (size_t) aSize, aFile);
	fclose(aFile);
	Buffer aBuffer;
	if (aSize > 0)
		aBuffer.WriteBytes(&aData[0], (int) aSize);
	WriteBufferToFile("SAVEGAME.DAT", &aBuffer);
}

void AtomicaApp::DeleteSavedGame()
{
	Buffer aBuffer;
	WriteBufferToFile("SAVEGAME.DAT", &aBuffer);
}

// 0x406060 / 0x406020: the saved game goes on (the menu closes itself when its doors are open)
void AtomicaApp::ResumeGame()
{
	Buffer aBuffer;
	if (!ReadBufferFromFile("SAVEGAME.DAT", &aBuffer) || (aBuffer.GetDataLen() <= 8))
		return;
	FILE* aFile = tmpfile();
	if (aFile == NULL)
		return;
	fwrite(aBuffer.GetDataPtr(), 1, (size_t) aBuffer.GetDataLen(), aFile);
	rewind(aFile);
	fread(&mMode, 4, 1, aFile);
	mPauseCount = 0;
	if (mBoard == NULL)
	{
		mBoard = new Board(this);
		mBoard->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mBoard);
		mBoard->Start();
		mBoard->Load(aFile);
	}
	fclose(aFile);
	mB3ea0 = false;
	DeleteSavedGame();
}

// 0x405cb0: the player gives up
void AtomicaApp::EndGame()
{
	if ((mBoard != NULL) && (mBoard->mGrid != NULL) && !mBoard->mGrid->mLevelOver)
	{
		mBoard->mGrid->mLevelOver = true;
		mBoard->mGrid->mOverTick = 0x7d;
		mB3ea0 = true;
	}
}

// 0x405d70: the main menu (theGameOver: after a game). The title screen or the board stays under it until its doors have shut
// (0x405e20 closes them); a game in progress waits meanwhile.
void AtomicaApp::ShowMainMenu(bool theGameOver)
{
	CloseMenu();
	if ((mBoard != NULL) && !theGameOver)
		mPauseCount++;
	MainMenu* aMenu = new MainMenu(this, theGameOver);
	aMenu->Resize(0, 0, mWidth, mHeight);
	mMenu = aMenu;
	mWidgetManager->AddWidget(aMenu);
}

// 0x405e20: the title screen and the board go
void AtomicaApp::CloseBoard()
{
	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		SafeDeleteWidget(mTitleScreen);
		mTitleScreen = NULL;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	mPauseCount = 0;
}

// 0x406200
void AtomicaApp::ShowGameOver()
{
	DeleteSavedGame();
	if (mB3ea0)
		ShowMainMenu(false);
	else
		ShowMainMenu(true);
}

// 0x432240: the level-up box (the next level begins when its button is pressed)
void AtomicaApp::ShowLevelUp()
{
	if ((mBoard != NULL) && (mLevelUpDialog == NULL))
	{
		LevelUpDialog* aDialog = new LevelUpDialog(this, mBoard);
		mLevelUpDialog = aDialog;
		mWidgetManager->AddWidget(aDialog);
	}
}

// 0x405e90
void AtomicaApp::CloseMenu()
{
	if (mMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mMenu);
		SafeDeleteWidget(mMenu);
		mMenu = NULL;
	}
}
