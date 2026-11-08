// The screens the application opens and closes: the game selector and, until each is translated, stubs for the
// screens it leads to. A stub logs "not translated" so a test run shows that a button was reached.

#include "AstroPopApp.h"
#include "AstroDialog.h"
#include "CheckpointDialog.h"
#include "NewUserDialog.h"
#include "EditWidget.h"
#include "Particle.h"
#include "Board.h"
#include "GameText.h"
#include "LoaderScreen.h"
#include "CharSelector.h"
#include "MainMenu.h"
#include "XcfFile.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include <SDL.h>

using namespace Sexy;

// 0x411490: the game selector replaces whatever screen is up
void AstroPopApp::ShowMainMenu()
{
	if (mBoard != NULL)
	{
		mEngineLoop->Stop();
		mHyperspaceLoop->Stop();
	}
	if (mScreen650 != NULL)
	{
		mWidgetManager->RemoveWidget(mScreen650);
		SafeDeleteWidget(mScreen650);
		mScreen650 = NULL;
		// The original also tells the resource manager's page counter (0x48f860, "MoreGames") that the page is gone;
		// that is PopCap's ad statistics and is left out.
	}

	mMainMenu = new MainMenu(this);
	mMainMenu->Resize(0, 0, 640, 480);
	mWidgetManager->AddWidget(mMainMenu);
	mWidgetManager->SetFocus(mMainMenu);
	PlaySong(5);

	if (mBoard != NULL)
	{
		mBoard->mHasAlpha = true;
		mBoard->mHasTransparencies = true;
		mWidgetManager->BringToFront(mBoard);
	}
}

// 0x412320: the running game is taken off the screen and deleted (when it is safe), and the pointer is cleared. (The
// original also tells its page counter, 0x48f860 "GameVolatile": PopCap's ad statistics, left out.)
void AstroPopApp::CloseBoard()
{
	mWidgetManager->RemoveWidget(mBoard);
	SafeDeleteWidget(mBoard);
	mBoard = NULL;
}

bool AstroPopApp::IsLeftButtonDown()
{
	return (mWidgetManager->mDownButtons & 1) != 0;
}

// 0x4319b0: the title's button. The loader screen goes, the level descriptor and the profile come.
// (The original also tells its page counter, 0x48f860 "Init": PopCap's ad statistics, left out. It allocates the 43
// lists at +0x62c here, which the Board uses; they come with the Board.)
void AstroPopApp::OnClickToPlay()
{
	mWidgetManager->RemoveWidget(mLoaderScreen);
	SafeDeleteWidget(mLoaderScreen);
	mLoaderScreen = NULL;
	LoadDescriptor();
	SelectStartUser();
}

// 0x42c150: data\descriptor.xcf holds the settings of the levels (DropTime, NumRowsToPass, the chances of the bricks...).
// The original first lists the 28 key names it expects and checks the file's signature; the port does neither.
void AstroPopApp::LoadDescriptor()
{
	mDescriptor = new XcfFile();
	int aResult = mDescriptor->Parse("data\\descriptor.xcf");
	if (aResult != 0)
	{
		Popup(StrFormat(GTC(0x562d50), aResult, mDescriptor->mLine));
		mShutdown = true;
	}
}

// 0x411880: the player of the last session (registry Users\LastUser), else the first player, else none
std::string AstroPopApp::GetLastUserName()
{
	std::string aName;
	if (!RegistryReadString("Users\\LastUser", &aName))
	{
		if (!mUserNames.empty())
			aName = mUserNames[0];
		else
			aName = "";
	}
	return aName;
}

// 0x4298c0: a profile that could not be read is made new again under its name and number
void AstroPopApp::ResetFailedUserLoad()
{
	std::string aName;
	mUser->GetString(PROFILE_NAME, &aName);
	int aNumber = -1;
	mUser->GetInt(PROFILE_NUMBER, &aNumber);
	mUser->InitNew(aName, aNumber);
}

// 0x4285d0: the profile's checks, and the flags the game reads from its settings
void AstroPopApp::ValidateUser()
{
	int aValue = 0;
	if (!mUser->GetInt(0x11, &aValue))
		aValue = 1;
	mFlag658 = aValue != 0;

	aValue = 1;
	if (!mUser->GetInt(0x13, &aValue))
		aValue = 1;
	mFlag6f9 = aValue != 0;

	// the unlock flags must be one of four known values and agree with their check value
	int anUnlock = -1;
	int aCheck = -1;
	bool aBad = !mUser->GetInt(PROFILE_UNLOCK_A, &anUnlock);
	if (!aBad)
	{
		aBad = !mUser->GetInt(PROFILE_UNLOCK_B, &aCheck);
		if (!aBad && anUnlock == aCheck - 0x193 &&
			(anUnlock == 0x44 || anUnlock == 0x13 || anUnlock == 0x16 || anUnlock == 0x41))
			aBad = false;
		else
			aBad = true;
	}
	if (aBad)
	{
		if (mLoaderScreen == NULL && mMainMenu == NULL)
			Popup(GT(0x562b78));
		else
			ShowMessageDialog("ERROR", GT(0x562ab0));
		mUser->SetInt(PROFILE_UNLOCK_A, 0x13);
		mUser->SetInt(PROFILE_UNLOCK_B, 0x1a6);
		mUser->Save();
	}

	mFlag658 = false;
	aValue = 0;
	mUser->GetInt(0x72, &aValue);
	mFlag6fa = aValue == 1;
	aValue = 1;
	mUser->GetInt(0x73, &aValue);
	mFlag6fb = aValue == 1;
}

// 0x4300c0: the profile of the last player is loaded and the selector shown; with no players the selector shows and
// the new-player dialog opens over it
void AstroPopApp::SelectStartUser()
{
	if (mUserNames.empty())
	{
		ShowMainMenu();
		ShowNewUserDialog();
		return;
	}

	std::string aName = GetLastUserName();
	bool aFound = false;
	for (size_t i = 0; i < mUserNames.size(); i++)
	{
		if (mUserNames[i] == aName)
		{
			aFound = true;
			break;
		}
	}
	if (!aFound)
		aName = mUserNames[0];

	bool aLoadFailed = false;
	mUser = new UserProfile();
	mUser->SetString(PROFILE_NAME, aName);
	RegistryWriteString("Users\\LastUser", aName);
	if (!mUser->Load(NULL, NULL, false))
	{
		aLoadFailed = true;
		ResetFailedUserLoad();
	}
	ShowMainMenu();
	if (mUser != NULL && (!mUser->Save() || !mUser->mValid))
		ShowMessageDialog("ERROR", GT(0x563520));
	if (mUser != NULL)
		ValidateUser();
	if (aLoadFailed)
		ShowMessageDialog("Error", GT(0x563470));
}

void AstroPopApp::Fn412730()
{
	SDL_Log("AstroPop: Fn412730 not translated yet (0x412730)");
}

void AstroPopApp::ShowThemeDialog()
{
	SDL_Log("AstroPop: ShowThemeDialog not translated yet (0x40c490)");
}

// 0x411660: the pilot choice screen replaces whatever is up. A running game stays under it (on the screen, but out of play).
// (The original also tells its page counter, 0x48f8a0 with an empty name: PopCap's ad statistics, left out.)
void AstroPopApp::ShowCharSelector(int theMode)
{
	PlaySong(5);
	if (mBoard != NULL)
	{
		mEngineLoop->Stop();
		mHyperspaceLoop->Stop();
	}
	if (mMainMenu != NULL)
		CloseMainMenu();

	mScreen650 = new CharSelector(this, theMode);
	mScreen650->Resize(0, 0, 0x280, 0x1e0);
	mWidgetManager->AddWidget(mScreen650);
	mWidgetManager->SetFocus(mScreen650);
	if (mBoard != NULL)
	{
		mBoard->mHasAlpha = true;
		mBoard->mHasTransparencies = true;
		mWidgetManager->BringToFront(mBoard);
	}
	StartLoopSound(6);
}

void AstroPopApp::ShowHallOfFame()
{
	SDL_Log("AstroPop: ShowHallOfFame not translated yet (0x4288a0)");
}

void AstroPopApp::ShowFullScreenHelp()
{
	SDL_Log("AstroPop: ShowFullScreenHelp not translated yet (0x422db0)");
}

// 0x422c00: the box that asks whether to leave the game, with one of the ten goodbye lines of the constructor
void AstroPopApp::ShowQuitDialog()
{
	StartLoopSound(-1);
	PlaySample(SOUND_GOODBYE);
	std::string aLines;
	int aDraw = (int) AstroRand();
	if (!mGoodbyeMessages.empty())
		aLines = mGoodbyeMessages[((unsigned int) (aDraw % 500000000)) % mGoodbyeMessages.size()];
	AstroDialog* aDialog = MakeDialog(1, false, 0x7c, -1, -1, 1, "QUIT GAME?", aLines, "Yes", "No", true);
	int aWidth = aDialog->mWidth;
	int aHeight = aDialog->mHeight;
	aDialog->Resize(0x140 - aWidth / 2, 0xf0 - aHeight / 2, aWidth, aHeight);
}

void AstroPopApp::ShowUserSelect()
{
	SDL_Log("AstroPop: ShowUserSelect not translated yet (0x422870)");
}

void AstroPopApp::ShowOptions(bool theFromGame)
{
	SDL_Log("AstroPop: ShowOptions(%d) not translated yet (0x40bd70)", theFromGame ? 1 : 0);
}

void AstroPopApp::ShowContinueDialog(const std::string& thePilot, int theLevel)
{
	SDL_Log("AstroPop: the CONTINUE? dialog (pilot %s, level %d) not translated yet (0x411ba0)", thePilot.c_str(), theLevel);
}

// 0x40bc00: the box where a new player types a name, centred a little low
void AstroPopApp::ShowNewUserDialog()
{
	mOpenDialogs++;
	NewUserDialog* aDialog = new NewUserDialog();
	StyleDialog(aDialog);
	int aHeight = aDialog->GetPreferredHeight(0x15e);
	aDialog->Resize(0x91, 0x118 - aHeight / 2, 0x15e, aHeight);
	AddDialog(0x6b, aDialog);
	mWidgetManager->SetFocus(aDialog->mEdit);
}

// 0x40c3e0: the starting point box for a player who has passed a checkpoint (the colour set of the pilot of the mode)
void AstroPopApp::ShowCheckpointDialog(int theLevel)
{
	CheckpointDialog* aDialog = new CheckpointDialog(mClassicPilot + 2, this);
	aDialog->mLevel = theLevel;
	StyleDialog(aDialog);
	mOpenDialogs++;
	aDialog->Resize(0x91, 0x58, 0x15e, 0x131);
	AddDialog(0x78, aDialog);
}

// Not translated yet: each logs once so a test run shows that the button was reached
#define APP_NOT_TRANSLATED(name, addr) 	do { static bool aLogged = false; if (!aLogged) { aLogged = true; SDL_Log("AstroPop: " name " not translated yet (" addr ")"); } } while (0)

void AstroPopApp::ShowHelp()
{
	APP_NOT_TRANSLATED("ShowHelp", "0x40bea0");
}

void AstroPopApp::CloseFullScreenHelp()
{
	APP_NOT_TRANSLATED("CloseFullScreenHelp", "0x428fb0");
}

void AstroPopApp::ApplyOptions(bool theClose)
{
	(void) theClose;
	APP_NOT_TRANSLATED("ApplyOptions", "0x428a10");
}

void AstroPopApp::UserSelectOk()
{
	APP_NOT_TRANSLATED("UserSelectOk", "0x430f00");
}

void AstroPopApp::DeleteUser(bool theConfirmed)
{
	(void) theConfirmed;
	APP_NOT_TRANSLATED("DeleteUser", "0x431310");
}
