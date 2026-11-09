// The application's game start: a new classic game (0x428b30), a new game of the second type (0x422ae0), the saved game
// (0x428c70), and the small helpers they share.

#include "AstroPopApp.h"
#include "Board.h"
#include "GameText.h"
#include "MainMenu.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include "ResourceManager.h"

using namespace Sexy;

// 0x40e0c0: the loop sounds stop (all but the last, the klaxon, which the Board keeps itself) and the one asked for starts
void AstroPopApp::StartLoopSound(int theIndex)
{
	for (unsigned int i = 0; i + 1 < mLoopSounds.size(); i++)
	{
		if (mLoopSounds[i] != NULL)
			mLoopSounds[i]->Stop();
	}
	if (theIndex != -1 && mLoopSounds[theIndex] != NULL)
		mLoopSounds[theIndex]->Play(false, false);
}

// 0x422380: the pictures of a pilot's ship, loaded again if the resource manager dropped them
void AstroPopApp::LoadPilotImages(int theKind)
{
	int aPilot;
	if (theKind == 1)
		aPilot = mClassicPilot;
	else if (theKind == 0)
		aPilot = mPilot6e8;
	else
		aPilot = mSurvivalPilot;

	LoadImageById(mResourceManager, aPilot + 0x173);
	LoadImageById(mResourceManager, aPilot + 0x177);
	LoadImageById(mResourceManager, aPilot + 0x17b);
	LoadImageById(mResourceManager, aPilot + 0x17f);
	if (aPilot == 1)
	{
		LoadImageById(mResourceManager, 0x183);
		LoadImageById(mResourceManager, 0x184);
	}
	else if (aPilot == 2)
	{
		LoadImageById(mResourceManager, 0x185);
		LoadImageById(mResourceManager, 0x186);
		LoadImageById(mResourceManager, 0x18a);
		LoadImageById(mResourceManager, 0x189);
	}
	else if (aPilot == 0)
		LoadImageById(mResourceManager, 0x187);
	else
		LoadImageById(mResourceManager, 0x188);
}

// 0x4113e0: the game selector goes (the original also tells its page counter, "MoreGames"... left out)
void AstroPopApp::CloseMainMenu()
{
	if (mMainMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mMainMenu);
		SafeDeleteWidget(mMainMenu);
		mMainMenu = NULL;
	}
}

// 0x4117d0: the screen over the selector goes
void AstroPopApp::CloseSelectorScreen()
{
	mWidgetManager->RemoveWidget(mScreen650);
	SafeDeleteWidget(mScreen650);
	mScreen650 = NULL;
}

// 0x428b30: a new classic game from a level. The player's hints start again, the old game (if any) goes, and the new Board
// takes the screen. The first level shows the tutorial unless theSkipTutorial or the player turned the hints off.
void AstroPopApp::StartClassicGame(int theLevel, bool theFlag, bool theSkipTutorial)
{
	if (mScreen650 != NULL)
		CloseSelectorScreen();
	mBus.Clear();
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}
	mUser->ResetHintsShown();
	mUser->Save();
	LoadPilotImages(1);

	bool aTutorial = theLevel == 1 && mFlag6f9 && !theSkipTutorial;
	mBoard = new Board(this, &mBus, 1, 1, theLevel, theFlag, true, aTutorial);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->SetFocus(mBoard);
	StartLoopSound(-1);
}

// 0x422ae0: a new game of the second type, flying the pilot given
void AstroPopApp::StartSurvivalGame(int thePilot)
{
	mSurvivalPilot = thePilot;
	mFlag6fc = false;
	if (mScreen650 != NULL)
		CloseSelectorScreen();
	mBus.Clear();
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}
	LoadPilotImages(2);

	mBoard = new Board(this, &mBus, 2, 1, 1, false, true, false);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->SetFocus(mBoard);
	StartLoopSound(-1);
}

// 0x428c70: the Play button of the selector. A game that was saved (setting 0x4f above 1) goes on when theContinue says so;
// otherwise the saved game is dropped and the pilots are shown.
void AstroPopApp::StartGame(bool theContinue)
{
	int aSavedLevel = 0;
	mUser->GetInt(0x4f, &aSavedLevel);
	if (aSavedLevel > 1 && theContinue)
	{
		int aScore = 0;
		float aFloat50 = 0;
		float aFloat52 = 0;
		int aSetting54 = 0;
		int aSetting55 = 0;
		mUser->GetInt(0x51, &aScore);
		mUser->GetFloat(0x50, &aFloat50);
		mUser->GetFloat(0x52, &aFloat52);
		mUser->GetInt(0x54, &aSetting54);
		mUser->GetInt(0x55, &aSetting55);
		int aPilot = 0;
		mUser->GetInt(0x53, &aPilot);

		// (the original also drops a saved classic game of a copy that is not registered; the port's game is the whole game)
		if ((aPilot == 2 || aPilot == 3) && !mUser->HasModeUnlocked(aPilot))
		{
			ShowMessageDialog("ERROR", GT(0x00562c48));
			mUser->SetInt(0x4f, -1);
			mUser->SetInt(0x51, -1);
			mUser->Save();
			return;
		}

		CloseMainMenu();
		mClassicPilot = aPilot;
		LoadPilotImages(1);
		mBoard = new Board(this, &mBus, 1, 0, 1, false, false, false);
		mBoard->Resize(0, 0, mWidth, mHeight);
		mBoard->RestoreGame(aScore, aSetting54, aSetting55, aFloat52, aFloat50, aSavedLevel);
		mWidgetManager->AddWidget(mBoard);
		mWidgetManager->SetFocus(mBoard);
		StartLoopSound(-1);
	}
	if (aSavedLevel < 1 || !theContinue)
	{
		CloseMainMenu();
		mUser->SetInt(0x4f, -1);
		mUser->SetInt(0x51, -1);
		mUser->Save();
		ShowCharSelector(0);
	}
}

// 0x4299b0: the pilot is chosen in the first mode. A player who has passed a checkpoint (the setting of the pilot, 8 + the
// pilot, is the best level; the checkpoints are every fourth) is asked whether to go on from it, and the answer
// comes back here with theCheckpointed set. Otherwise the game starts at the first level.
void AstroPopApp::StartClassic(bool theCheckpointed, bool theSkipTutorial)
{
	mFlag6fc = false;
	int aBest;
	mUser->GetInt(mClassicPilot + 8, &aBest);
	if (aBest < 4)
	{
		StartClassicGame(1, false, theSkipTutorial);
		return;
	}
	int aCheckpoint = ((aBest + ((aBest >> 31) & 3)) >> 2) * 4;
	if (!theCheckpointed)
		ShowCheckpointDialog(aCheckpoint);
	else
		StartClassicGame(aCheckpoint + 1, true, theSkipTutorial);
}
