// The screens the application opens and closes: the game selector and, until each is translated, stubs for the
// screens it leads to. A stub logs "not translated" so a test run shows that a button was reached.

#include "AstroPopApp.h"
#include "AstroDialog.h"
#include "CheckpointDialog.h"
#include "NewUserDialog.h"
#include "OptionsDialog.h"
#include "PowerDialog.h"
#include "UpgradeDialog.h"
#include "HelpDialog.h"
#include "Checkbox.h"
#include "UserProfile.h"
#include "EditWidget.h"
#include "Particle.h"
#include "Board.h"
#include "GameText.h"
#include "LoaderScreen.h"
#include "DDImage.h"
#include "DialogButton.h"
#include "MemoryImage.h"
#include "Res.h"
#include "CharSelector.h"
#include "MainMenu.h"
#include "XcfFile.h"
#include "SoundInstance.h"
#include "CreditsScreen.h"
#include "FullScreenHelp.h"
#include "HallOfFame.h"
#include "ResourceManager.h"
#include "Checkbox.h"
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
	if (mCharSelector != NULL)
	{
		mWidgetManager->RemoveWidget(mCharSelector);
		SafeDeleteWidget(mCharSelector);
		mCharSelector = NULL;
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
	mTutorialOn = aValue != 0;

	aValue = 1;
	if (!mUser->GetInt(0x13, &aValue))
		aValue = 1;
	mHintsOn = aValue != 0;

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

	mTutorialOn = false;
	aValue = 0;
	mUser->GetInt(0x72, &aValue);
	mColorblind = aValue == 1;
	aValue = 1;
	mUser->GetInt(0x73, &aValue);
	mShowHelpAtStart = aValue == 1;
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

// 0x412730: the box of the warp screen that tells that the pilot's weapon has been upgraded (the upgrade itself is made
// by the Board before the warp), with the name of the upgrade the level brings
void AstroPopApp::ShowUpgradeDialog()
{
	UpgradeDialog* aDialog = new UpgradeDialog(this, mBoard->mMode, mBoard->GetUpgradeSetting(), mBoard->GetUpgradeText());
	StyleDialog(aDialog);
	mOpenDialogs++;
	aDialog->mYesButton->mLabel = "OK";
	aDialog->Resize(0xab, 0x4e, 0x1ae, 0x145);
	AddDialog(0x76, aDialog);
}

// 0x40c490: the box that explains the weapon of the game's pilot (PowerDialog), in the colour set of the mode (+ 2). The
// first move of a game opens it unless the player has asked not to see it (profile setting 0x2a + the mode); the game waits
// meanwhile. The box is 430 wide for the two pilots that have two pictures, 590 for the other two (the original asks a table
// for these widths, its tunable numbers, where nothing overrides them).
void AstroPopApp::ShowPowerDialog()
{
	if (mBoard == NULL)
		return;     // port: only the Board opens the box
	int aMode = mBoard->mMode;
	int aSet = aMode + 2;
	Color anInsetColor;
	Color aButtonColor;
	if (aSet >= 0 && aSet <= 5)
	{
		anInsetColor = GetDialogInsetColor(aSet);
		aButtonColor = GetDialogColor(aSet);
	}
	MemoryImage* anInsetImage = NULL;
	MemoryImage* aStripImage = NULL;
	MakeDialogImages(anInsetColor, aButtonColor, &anInsetImage, &aStripImage, false);
	Image* aCheckImage = CreateColorizedImage(IMAGE_TH_CHECKBOX, Color(0xff, 0xff, 0xff));

	PowerDialog* aDialog = new PowerDialog(this, aMode, anInsetImage, aStripImage, aCheckImage);
	StyleDialog(aDialog);
	mOpenDialogs++;
	int aWidth = (aMode == 0 || aMode == 3) ? 0x1ae : 0x24e;
	int aHeight = aDialog->GetPreferredHeight(aWidth);
	aDialog->Resize(0x140 - aWidth / 2, 0xf0 - aHeight / 2, aWidth, aHeight);
	mBoard->Pause(true, false);
	AddDialog(0x7b, aDialog);
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

	mCharSelector = new CharSelector(this, theMode);
	mCharSelector->Resize(0, 0, 0x280, 0x1e0);
	mWidgetManager->AddWidget(mCharSelector);
	mWidgetManager->SetFocus(mCharSelector);
	if (mBoard != NULL)
	{
		mBoard->mHasAlpha = true;
		mBoard->mHasTransparencies = true;
		mWidgetManager->BringToFront(mBoard);
	}
	StartLoopSound(6);
}

// 0x4288a0: the pictures of the hall of fame are loaded and the screen comes over the selector. (The original reads the
// clock, 0x4ce790, which the load of the pictures made it lose.)
void AstroPopApp::ShowHallOfFame()
{
	mResourceManager->StartLoadResources("HighScores");
	while (mResourceManager->LoadNextResource())
	{
	}
	if (!mResourceManager->HadError() && ExtractHighScoresResources(mResourceManager))
	{
		mHallScreen = new HallOfFame(this);
		mHallScreen->Resize(0, 0, 0x280, 0x1e0);
		mWidgetManager->AddWidget(mHallScreen);
		mWidgetManager->SetFocus(mHallScreen);
		return;
	}

	ShowResourceError(false);
	mLoadingFailed = true;
	Shutdown();
}

// 0x411a90: the hall of fame goes (its pictures stay loaded). (The original also tells its page counter, 0x48f860
// "HighScores" and 0x48f8a0 with an empty name: PopCap's ad statistics, left out.)
void AstroPopApp::CloseHallOfFame()
{
	mWidgetManager->RemoveWidget(mHallScreen);
	SafeDeleteWidget(mHallScreen);
	mHallScreen = NULL;
}

// 0x422db0: a game opens with the picture of how to play (once the player has not turned it off): the selector goes, the
// pictures of it are loaded and the game waits (Pause) while it is up. (The original also tells its page counter, 0x48f8a0:
// PopCap's ad statistics, left out, and reads the clock, 0x4ce790, which the load of the pictures made it lose.)
void AstroPopApp::ShowFullScreenHelp()
{
	CloseMainMenu();
	mResourceManager->StartLoadResources("FullScreenHelp");
	while (mResourceManager->LoadNextResource())
	{
	}
	if (!mResourceManager->HadError() && ExtractFullScreenHelpResources(mResourceManager))
	{
		if (mBoard != NULL)
			mBoard->Pause(true, false);
		mHelpScreen = new FullScreenHelp(this);
		mHelpScreen->Resize(0, 0, 0x280, 0x1e0);
		mWidgetManager->AddWidget(mHelpScreen);
		return;
	}

	ShowResourceError(false);
	mLoadingFailed = true;
	Shutdown();
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
	AstroDialog* aDialog = MakeDialog(1, false, 0x7c, -1, -1, 1, GT(0x005622b8), aLines, "Yes", "No");
	int aWidth = aDialog->mWidth;
	int aHeight = aDialog->mHeight;
	aDialog->Resize(0x140 - aWidth / 2, 0xf0 - aHeight / 2, aWidth, aHeight);
}

// 0x40bd70 (its tail 0x40be20, shared with 0x40bea0, pauses the game and runs the box): the options box, in the colour set of the game's mode (+ 2) or, over the selector, 1. A game on pauses.
void AstroPopApp::ShowOptions(bool theFromGame)
{
	mOpenDialogs++;
	int aSet = (mBoard == NULL) ? 1 : mBoard->mMode + 2;
	OptionsDialog* aDialog = new OptionsDialog(mHintsOn, aSet, theFromGame);
	StyleDialog(aDialog);
	aDialog->mHasAlpha = true;
	aDialog->mHasTransparencies = true;
	int aHeight = aDialog->GetPreferredHeight(0x1b8);
	aDialog->Resize(100, 0xf0 - aHeight / 2, 0x1b8, aHeight);
	if (mBoard != NULL)
		mBoard->Pause(true, false);
	AddDialog(0x66, aDialog);
}

// 0x446410 (the classic button, with a saved game): the saved game would be lost by a new one, so the box asks. Continue
// is its first button (the hub's 0x844), New Game the second (the hub's 0x74).
void AstroPopApp::ShowContinueDialog(const std::string& thePilot, int theLevel)
{
	MakeDialog(1, false, 0x74, 0x177, 0xe6, 1, GT(0x005649f0), StrFormat(GTC(0x564a00), thePilot.c_str(), theLevel), "Continue",
			   GT(0x00564b3c));
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

// 0x40bea0 (and the tail 0x40be20): the "how to play" box, in the colour set of the game's mode (+ 2) or, over the selector, 1. A game on pauses.
void AstroPopApp::ShowHelp()
{
	mOpenDialogs++;
	int aSet = (mBoard == NULL) ? 1 : mBoard->mMode + 2;
	HelpDialog* aDialog = new HelpDialog(aSet, this);
	StyleDialog(aDialog);
	aDialog->mHasAlpha = true;
	aDialog->mHasTransparencies = true;
	int aHeight = aDialog->GetPreferredHeight(0x1c2);
	if (mBoard != NULL)
		mBoard->Pause(true, false);
	aDialog->Resize(0x5f, 0xf0 - aHeight / 2, 0x1c2, aHeight);
	AddDialog(0x79, aDialog);
}

void AstroPopApp::CloseFullScreenHelp()
{
	FullScreenHelp* aHelp = (FullScreenHelp*) mHelpScreen;
	mShowHelpAtStart = !aHelp->mNoMoreCheck->IsChecked();
	mWidgetManager->RemoveWidget(mHelpScreen);
	SafeDeleteWidget(mHelpScreen);
	mHelpScreen = NULL;
	mUser->SetInt(0x73, mShowHelpAtStart ? 1 : 0);
	mUser->Save();
	if (mBoard != NULL)
		mBoard->Pause(false, false);
}

// 0x428a10: the box's settings take effect and are saved with the player; the box goes. In a game the game goes on (theClose:
// the box was closed with its own button, which the game notes in mKeepLoopSoundsMuted).
void AstroPopApp::ApplyOptions(bool theClose)
{
	mOpenDialogs--;
	OptionsDialog* aDialog = (OptionsDialog*) GetDialog(0x66);
	SwitchScreenMode(!aDialog->mFullscreenCheck->IsChecked(), aDialog->mAccelCheck->IsChecked());
	EnableCustomCursors(aDialog->mCursorCheck->IsChecked());
	mHintsOn = aDialog->mHintsCheck->IsChecked();
	mUser->SetInt(0x13, mHintsOn ? 1 : 0);
	mColorblind = aDialog->mColorblindCheck->IsChecked();
	mUser->SetInt(0x72, mColorblind ? 1 : 0);
	mUser->Save();
	KillDialog(0x66);
	if (mBoard != NULL)
	{
		if (theClose)
			mBoard->mKeepLoopSoundsMuted = true;
		mBoard->Pause(false, true);
		mWidgetManager->SetFocus(mBoard);
	}
}

// 0x428f10: the credits come over the options box (which goes, when no game is on) and the selector; the button of the
// screen (0xfb6) takes them away again
void AstroPopApp::ShowCredits()
{
	CloseMainMenu();
	if (mBoard == NULL)
		ApplyOptions(false);
	mCreditsScreen = new CreditsScreen(this);
	mCreditsScreen->Resize(0, 0, 0x280, 0x1e0);
	mWidgetManager->AddWidget(mCreditsScreen);
}

