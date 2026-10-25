// The app's game flow: starting a game, the dialogs, the end (Atomica.exe 0x405b10-0x406240).

#include "AtomicaApp.h"
#include <cstdio>
#include "Board.h"
#include "Grid.h"
#include "Screens.h"
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

// 0x405bd0 / 0x405c40 / 0x405d00: the options, the help and the menu boxes
// port: the dialogs are not ported yet
void AtomicaApp::ShowOptions()
{
	printf("options dialog (not ported)");
}

void AtomicaApp::ShowHelp()
{
	printf("help dialog (not ported)");
}

void AtomicaApp::ShowMenuDialog()
{
	// port: until the menu box is ported the button gives up the game
	EndGame();
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
	(void) theGameOver;
	// port: until the menu is ported, a new game starts at once
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	StartGame();
}

// 0x406200
void AtomicaApp::ShowGameOver()
{
	if (mB3ea0)
		ShowMainMenu(false);
	else
		ShowMainMenu(true);
}

// 0x432240: the level-up box
// port: not ported yet; the next level comes at once
void AtomicaApp::ShowLevelUp()
{
	if (mBoard != NULL)
		mBoard->NextLevel();
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
