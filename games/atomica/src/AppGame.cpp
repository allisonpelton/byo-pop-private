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

// the hints that were shown (the original keeps them in app+0x406c) are shown again
void AtomicaApp::ResetHints()
{
	if ((mBoard != NULL) && (mBoard->mGrid != NULL))
		for (int i = 0; i < 0xff; i++)
			mBoard->mGrid->mHintShown[i] = false;
	for (int i = 0; i < 0xff; i++)
		mHintsShown[i] = false;
}

// the high scores box (0x41ea40)
// port: not ported yet
void AtomicaApp::ShowHighScores()
{
	puts("high scores box (not ported)");
}

// a game of the given mode starts (from the menu)
void AtomicaApp::StartNewGame(int theMode, int theExpert)
{
	CloseMenu();
	mMode = theMode;
	mExpertMode = theExpert;
	mPauseCount = 0;
	StartGame();
}

// port: the saved game is a file in the user's folder
bool AtomicaApp::HasSavedGame()
{
	Buffer aBuffer;
	return ReadBufferFromFile("SAVEGAME.DAT", &aBuffer) && (aBuffer.GetDataLen() > 8);
}

// 0x405fc0
void AtomicaApp::SaveGame()
{
	if (mBoard == NULL)
		return;
	FILE* aFile = tmpfile();
	if (aFile == NULL)
		return;
	fwrite(&mMode, 4, 1, aFile);
	fwrite(&mExpertMode, 4, 1, aFile);
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

// 0x406060: the saved game goes on
void AtomicaApp::ResumeGame()
{
	Buffer aBuffer;
	if (!ReadBufferFromFile("SAVEGAME.DAT", &aBuffer) || (aBuffer.GetDataLen() <= 8))
		return;
	CloseMenu();
	FILE* aFile = tmpfile();
	if (aFile == NULL)
		return;
	fwrite(aBuffer.GetDataPtr(), 1, (size_t) aBuffer.GetDataLen(), aFile);
	rewind(aFile);
	fread(&mMode, 4, 1, aFile);
	fread(&mExpertMode, 4, 1, aFile);
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

// 0x405d70: the main menu (theGameOver: after a game)
void AtomicaApp::ShowMainMenu(bool theGameOver)
{
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	CloseMenu();
	MainMenu* aMenu = new MainMenu(this, theGameOver);
	aMenu->Resize(0, 0, mWidth, mHeight);
	mMenu = aMenu;
	mWidgetManager->AddWidget(aMenu);
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
