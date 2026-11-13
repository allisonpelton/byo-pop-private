// The application of Hammer Heads Deluxe (0x41e030-0x422dd0, 0x41ffd0): see Application.h. Written from our own Ghidra
// export. Everything the registration code did (the demo's level limit, the upsell, the DRM client, the "level allowed"
// check at 0x41e520) is left out: the full game is the only path.

#include "Application.h"

#include "ChallengeScreen.h"
#include "Common.h"
#include "Compat2004.h"
#include "Creature.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "LevelRules.h"
#include "MusicInterface.h"
#include "NcButton.h"
#include "Object.h"
#include "Particles.h"
#include "ResourceManager.h"
#include "SDLMusicInterface.h"
#include "SavedGames.h"
#include "Screens.h"
#include "ShopScreen.h"
#include "Stats.h"
#include "UserManager.h"
#include "WidgetManager.h"

#include <SDL.h>

using namespace Sexy;

Application* Sexy::gApp = nullptr;

namespace
{

// The words of the cheats (0x54d9d0 fills the original's table in this order; the last one is empty)
const char* const kCheatWords[Application::NUM_CHEATS] = {"nomissdamage", "noawaydamage", "superpower", "jumpto5", "jumpto10",
	"jumpto15", "jumpto20", "jumpto25", ""};

// The music files, by track (0x590908)
const char* const kMusicFiles[4] = {"sounds\\Music - Loading and Menu.ogg", "sounds\\Music - In-Game.ogg",
	"sounds\\Music - High Score.ogg", "sounds\\Music - Bonus Game.ogg"};

// The pictures that get a glow copy while the game loads (the 34 resource numbers at 0x590918, in that order)
Image* const* GetGlowSources(int* theCount)
{
	static Image* aSources[34];
	aSources[0] = IMAGE_GNOME_1;
	aSources[1] = IMAGE_GNOME_FEMALE;
	aSources[2] = IMAGE_GNOME_BOMB;
	aSources[3] = IMAGE_GNOME_BUBBLEHEAD;
	aSources[4] = IMAGE_GNOME_HIDE;
	aSources[5] = IMAGE_GNOME_ARNOLD;
	aSources[6] = IMAGE_GNOME_BOUNCING;
	aSources[7] = IMAGE_GNOME_FORK;
	aSources[8] = IMAGE_PLAYER_HAMMER_2;
	aSources[9] = IMAGE_PLAYER_HAMMER_3;
	aSources[10] = IMAGE_PLAYER_HAMMER_4;
	aSources[11] = IMAGE_HEART;
	aSources[12] = IMAGE_SHOP_HEART;
	aSources[13] = IMAGE_SHOP_HEART_BIG;
	aSources[14] = IMAGE_SHOP_HEART_EXTRA;
	aSources[15] = IMAGE_SHOP_FLAME;
	aSources[16] = IMAGE_SHOP;
	aSources[17] = IMAGE_MOLE_X5;
	aSources[18] = IMAGE_CUPIDO;
	aSources[19] = IMAGE_GNOME_SILVER;
	aSources[20] = IMAGE_LUCKY_CLOVER;
	aSources[21] = IMAGE_GNOME_BONUS_STRAWBERRY;
	aSources[22] = IMAGE_GNOME_BONUS_ICECREAM;
	aSources[23] = IMAGE_GNOME_BONUS_DUCK;
	aSources[24] = IMAGE_GNOME_RABBIT;
	aSources[25] = IMAGE_GNOME_KING;
	aSources[26] = IMAGE_GNOME_NAPOLEON;
	aSources[27] = IMAGE_GNOME_AVIATOR;
	aSources[28] = IMAGE_CHEST;
	aSources[29] = IMAGE_COIN;
	aSources[30] = IMAGE_GNOME_POWERUP;
	aSources[31] = IMAGE_GNOME_SWITCHER;
	aSources[32] = IMAGE_GNOME_SAVE;
	aSources[33] = IMAGE_CHALLENGE;
	*theCount = 34;
	return aSources;
}

// The repeated "take a widget off the manager, delete it, forget it" of the original
template <class T>
void DropWidget(T*& theWidget)
{
	gApp->mWidgetManager->RemoveWidget(theWidget);
	gApp->SafeDeleteWidget(theWidget);
	theWidget = nullptr;
}

}

//////////////////////////////////////////////////////////////////////////
// Construction

// 0x41ffd0
Application::Application()
{
	gApp = this;

	mParticleDefs = nullptr;
	for (int i = 0; i < NUM_CHEATS; i++)
		mCheatFlags[i] = false;
	mScreenWidth = -1;
	mScreenHeight = -1;
	mQuietSpawn = false;
	mLevelUpPending = false;
	mGameWonPending = false;
	mGameScreen = nullptr;
	mShopScreen = nullptr;
	mShopOpening = false;
	mChallengeScreen = nullptr;
	mChallengeOpening = false;
	mChallengeLeaving = false;
	mLoadingScreen = nullptr;
	mResurrectScreen = nullptr;
	mStartLevel = 0;
	mGameWon = nullptr;
	mHowToPlay = nullptr;
	mPauseCount = 0;
	mMainMenu = nullptr;
	mTrophyRoom = nullptr;
	mQuitting = false;
	mLevelIntro = nullptr;
	mFade = 0.0;
	mZooming = false;
	mZoomWipe = nullptr;
	mGameMode = GAME_MODE_CLASSIC;
	mFocusedUpdate = -1;
	mUserManager = nullptr;

	mProdName = "HammerHeads";
	mTitle = "Hammer Heads " + mProductVersion;
	mRegKey = "PopCap\\Hammer Heads";
	mWidth = 800;
	mHeight = 600;
	mAutoEnable3D = true;
	mVSyncUpdates = true;

	mParticleRand = new ParticleRand();
}

// 0x420400
Application::~Application()
{
	if (mResourceManager != nullptr)
		mResourceManager->DeleteResources("");

	delete mUserManager;
	mUserManager = nullptr;
	delete mParticleDefs;
	mParticleDefs = nullptr;
	delete mParticleRand;
	mParticleRand = nullptr;
}

// 0x4205f0 slot 45
void Application::Init()
{
	// the desktop's size (the original asks for the full-screen size of the work area)
	{
		SDL_Rect aBounds;
		if (SDL_GetDisplayUsableBounds(0, &aBounds) == 0)
		{
			mScreenWidth = aBounds.w;
			mScreenHeight = aBounds.h;
		}
		else
		{
			mScreenWidth = mWidth;
			mScreenHeight = mHeight;
		}
	}

	SexyApp::Init();
	LoadResourceManifest();		// 0x4e5780

	// port: the 2004 SexyApp::Init seeds the C library generator and Rand()'s generator (see AstroPopApp::Init)
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());
	{
		MTRand aRand;
		SeedMTRand2004(aRand, (unsigned long) mRandSeed);
		uint32_t aState[MTRAND_N];
		int anIndex;
		aRand.GetState(aState, &anIndex);
		SetRandMTState(aState, anIndex);
	}

	if (!mResourceManager->LoadResources("Init") || !ExtractInitResources(mResourceManager))
	{
		mLoadingFailed = true;
		ShowResourceError(true);
		return;
	}

	SetCursorImages();

	if (!mResourceManager->LoadResources("LoadingScreen") || !ExtractLoadingScreenResources(mResourceManager))
	{
		mLoadingFailed = true;
		ShowResourceError(true);
		return;
	}

	mMusicInterface->LoadMusic(MUSIC_MENU, kMusicFiles[MUSIC_MENU]);
	CreateLoadingScreen();

	// the loading bar counts the three groups and 0x25 more tasks: the 34 glow pictures and the 3 songs
	mNumLoadingThreadTasks = mResourceManager->GetNumResources("MainMenu") + mResourceManager->GetNumResources("Gui") +
		mResourceManager->GetNumResources("Game");
	mNumLoadingThreadTasks += 0x25;

	mUserManager = new UserManager();
}

// 0x41e5b0: the game's own cursors
void Application::SetCursorImages()
{
	SetCursorImage(CURSOR_POINTER, IMAGE_CUSTOM_POINTER);
	SetCursorImage(CURSOR_HAND, IMAGE_CUSTOM_HAND);
	SetCursorImage(CURSOR_DRAGGING, IMAGE_CUSTOM_DRAGGING);
	SetCursorImage(CURSOR_TEXT, IMAGE_CUSTOM_TEXT);
}

// 0x420950 slot 22 (runs in one go in the port)
void Application::LoadingThreadProc()
{
	for (int aGroup = 0; aGroup < 3; aGroup++)
	{
		const char* aName = aGroup == 0 ? "Game" : aGroup == 1 ? "Gui" : "MainMenu";
		mResourceManager->StartLoadResources(aName);
		while (mResourceManager->LoadNextResource())
		{
			mCompletedLoadingThreadTasks++;
			if (mShutdown)
				return;
			mLoadingScreen->MarkDirty();
		}

		bool aOk = !mResourceManager->HadError();
		if (aOk)
		{
			if (aGroup == 0)
				aOk = ExtractGameResources(mResourceManager);
			else if (aGroup == 1)
				aOk = ExtractGuiResources(mResourceManager);
			else
				aOk = ExtractMainMenuResources(mResourceManager);
		}
		if (!aOk)
		{
			ShowResourceError(false);
			mLoadingFailed = true;
			Shutdown();
			return;
		}
	}

	// the glow copies of the gnomes and of the shop's pictures
	int aCount = 0;
	Image* const* aSources = GetGlowSources(&aCount);
	for (int i = 0; i < aCount; i++)
	{
		mCompletedLoadingThreadTasks++;
		GetGlowImage(aSources[i]);
	}

	// the other three songs (the first one is loaded in Init)
	for (int aTrack = MUSIC_GAME; aTrack <= MUSIC_BONUS; aTrack++)
	{
		mMusicInterface->LoadMusic(aTrack, kMusicFiles[aTrack]);
		mCompletedLoadingThreadTasks++;
	}
}

// 0x41e660 slot 16: the loading is done, the play button comes up
void Application::LoadingThreadCompleted()
{
	if (!mLoadingFailed)
	{
		mLoadingScreen->LoadingFinished();
		mLoadingScreen->MarkDirty();
		mParticleDefs = new ParticleDefs();
	}
}

// 0x4220d0 slot 97: the loading screen's wipe is over
void Application::LoadingScreenDone()
{
	DropWidget(mLoadingScreen);
	CreateMainMenu();
	if (mUserManager->GetCurrentUser() == nullptr)
	{
		KillDialog(DIALOG_CREATE_USER);
		MakeCreateUserDialog(false);
	}
}

// 0x41f040 slot 8
void Application::UpdateFrames()
{
	SexyApp::UpdateFrames();
	if (mQuitting)
		Shutdown();
}

// 0x41f420 slot 20
void Application::ShutdownHook()
{
	if (mMainMenu != nullptr)
		DropWidget(mMainMenu);
	if (mLoadingScreen != nullptr)
		DropWidget(mLoadingScreen);
	if (mGameScreen != nullptr)
		DropWidget(mGameScreen);
	if (mShopScreen != nullptr)
		DropWidget(mShopScreen);
	if (mChallengeScreen != nullptr)
		DeleteChallengeScreen();
	if (mResurrectScreen != nullptr)
		DropWidget(mResurrectScreen);
	if (mGameWon != nullptr)
		DropWidget(mGameWon);
	if (mLevelIntro != nullptr)
		DropWidget(mLevelIntro);
	if (mHowToPlay != nullptr)
		DropWidget(mHowToPlay);
	if (mZoomWipe != nullptr)
		EndZoomWipe();
}

//////////////////////////////////////////////////////////////////////////
// State

bool Application::GetCheatFlag(int theIndex) const
{
	return mCheatFlags[theIndex];
}

// 0x41f210
void Application::ToggleCheat(int theIndex)
{
	mCheatFlags[theIndex] = !mCheatFlags[theIndex];
	if (mCheatFlags[theIndex])
		PlaySample(SOUND_VOICE_CHEAT_CHEATER);
	else
		PlaySample(SOUND_VOICE_CHEAT_HONEST);
}

// 0x41f1f0
void Application::ClearCheats()
{
	for (int i = 0; i < NUM_CHEATS; i++)
		mCheatFlags[i] = false;
}

// 0x41f150
bool Application::IsShopOpen(bool theIncludeOpening) const
{
	if (!theIncludeOpening)
		return mShopScreen != nullptr;
	return mShopScreen != nullptr || mShopOpening;
}

// 0x41f1a0
bool Application::IsChallengeOpen(bool theIncludeOpening) const
{
	if (!theIncludeOpening)
		return mChallengeScreen != nullptr;
	return (mChallengeScreen != nullptr || mChallengeOpening) && !mChallengeLeaving;
}

// 0x41ebc0
bool Application::IsPaused() const
{
	return mPauseCount > 0;
}

// 0x41ebf0
bool Application::IsMenuOpen()
{
	return GetDialog(DIALOG_GAME_MENU) != nullptr || GetDialog(DIALOG_LEAVE_GAME) != nullptr;
}

// 0x41ec20
bool Application::IsReadyGoOpen()
{
	return GetDialog(DIALOG_READY_GO) != nullptr;
}

// 0x41f100
bool Application::IsBonusRoundOpen()
{
	return GetDialog(DIALOG_BONUS_ROUND) != nullptr;
}

// 0x41f290
bool Application::HasFewRestartPoints()
{
	User* aUser = mUserManager->GetCurrentUser();
	if (aUser == nullptr)
		return true;
	return aUser->GetRestartPointCount(0) < 3;
}

// 0x41f2d0
bool Application::IsToughUnlocked()
{
	User* aUser = mUserManager->GetCurrentUser();
	if (aUser == nullptr)
		return false;
	return aUser->mToughUnlocked;
}

//////////////////////////////////////////////////////////////////////////
// The music

// 0x41cfb0: a track plays in place of the others (the loading screen's fades in slowly)
void Application::StartMusic(int theTrack, bool theSlowFade)
{
	bool aWasPlaying = mMusicInterface->IsPlaying(theTrack);
	mMusicInterface->FadeOutAll(true, 0.004);
	mMusicInterface->FadeIn(theTrack, aWasPlaying ? -1 : 0, theSlowFade ? 0.002 : 1.0, false);
}

// 0x41cf80
void Application::FadeOutMusic()
{
	mMusicInterface->FadeOutAll(true, 0.004);
}

//////////////////////////////////////////////////////////////////////////
// Dialogs

// 0x41ee10 slot 25: every box of the game is a moving dialog that comes down from the top of the screen
Dialog* Application::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
	const SexyString& theDialogFooter, int theButtonMode)
{
	int aWidth = (int) ((double) mWidth * 0.7);

	// a dialog of the framework's own, to ask how tall the text makes the box
	Dialog* aMeasure = new Dialog(IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, theDialogId, isModal, theDialogHeader, theDialogLines,
		theDialogFooter, theButtonMode);
	aMeasure->SetHeaderFont(FONT_PTBANANASPLIT27);
	aMeasure->SetLinesFont(FONT_EUROFURENCE14);
	aMeasure->mContentInsets = Insets(0x26, 0x3c, 0x26, 0x37);
	int aHeight = aMeasure->GetPreferredHeight(aWidth);
	delete aMeasure;

	NcMovingDialog* aDialog = new NcMovingDialog(aHeight, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, theDialogId, isModal, theDialogHeader,
		theDialogLines, theDialogFooter, theButtonMode);
	if (aDialog->mYesButton != nullptr)
		SetupButtonCels(aDialog->mYesButton, 3);
	if (aDialog->mNoButton != nullptr)
		SetupButtonCels(aDialog->mNoButton, 3);
	aDialog->SetHeaderFont(FONT_PTBANANASPLIT27);
	aDialog->SetLinesFont(FONT_EUROFURENCE14);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0, 255));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(255, 255, 255, 255));
	aDialog->mContentInsets = Insets(0x30, 0x3c, 0x30, 0x37);
	aDialog->Resize((mWidth - aWidth) / 2, -aHeight, aWidth, aHeight);
	return aDialog;
}

// 0x41f260 slot 69: the dialog takes the focus
void Application::AddDialog(int theDialogId, Dialog* theDialog)
{
	SexyApp::AddDialog(theDialogId, theDialog);
	mWidgetManager->SetFocus(theDialog);
}

// 0x41eda0
void Application::CenterDialog(Dialog* theDialog)
{
	if (theDialog != nullptr)
		theDialog->Resize(mWidth / 2 - theDialog->mWidth / 2, mHeight / 2 - theDialog->mHeight / 2, theDialog->mWidth, theDialog->mHeight);
}

// 0x422ac0 (the DialogListener's slot 1): a button of a dialog was let go
void Application::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theButtonId == Dialog::ID_YES)
	{
		switch (theDialogId)
		{
		case 2: case 4: case 5: case 6: case 8: case 9: case 10: case 0xe: case 0xf: case 0x10: case 0x12: case 0x13: case 0x14:
		case 0x15: case 0x16: case 0x17:
			return;
		case DIALOG_LEAVE_GAME:
			LeaveConfirmed();
			return;
		case DIALOG_CONTINUE:
			DoZoomWipe(WipeContinueGame, WIPE_HAMMER);
			return;
		default:
			KillDialog(theDialogId);
			return;
		}
	}
	else if (theButtonId == Dialog::ID_NO)
	{
		switch (theDialogId)
		{
		case 4: case 5: case 6: case 9: case 0xe: case 0xf:
			return;
		case DIALOG_LEAVE_GAME:
			PauseGame(true);
			return;
		case DIALOG_CONTINUE:
			DoZoomWipe(WipeNewGame, WIPE_HAMMER);
			return;
		default:
			KillDialog(theDialogId);
			return;
		}
	}
}

// 0x4222a0 slot 98: a moving dialog was closed (theResult is the button that closed it, or 999 for the escape key)
void Application::DialogClosed(int theDialogId, int theResult)
{
	if (theResult == Dialog::ID_YES)
	{
		switch (theDialogId)
		{
		case DIALOG_CREATE_USER:
			CreateUserDialogDone();
			return;
		case DIALOG_CREATE_ANOTHER_USER:
			CreateAnotherUserDialogDone();
			return;
		case DIALOG_ERROR: case 10: case DIALOG_CONTINUE: case 0x14: case 0x15: case DIALOG_TOUGH_UNLOCKED: case DIALOG_GAME_WON:
			KillDialog(theDialogId);
			return;
		case DIALOG_DELETE_USER:
			DeleteSelectedUser(true);
			return;
		case DIALOG_LEAVE_GAME:
			KillDialog(DIALOG_LEAVE_GAME);
			return;
		case DIALOG_HIGH_SCORES:
			KillDialog(DIALOG_HIGH_SCORES);
			return;
		case DIALOG_BONUS_ROUND:
			KillDialog(theDialogId);
			if (mChallengeScreen != nullptr)
				mChallengeScreen->StartChallenge();
			return;
		}
	}
	else if (theResult == Dialog::ID_NO)
	{
		switch (theDialogId)
		{
		case DIALOG_CREATE_USER:
			CreateUserDialogDone();
			return;
		case DIALOG_CREATE_ANOTHER_USER:
			CreateAnotherUserDialogDone();
			return;
		case DIALOG_DELETE_USER:
			DeleteSelectedUser(false);
			return;
		case DIALOG_LEAVE_GAME:
			KillDialog(DIALOG_LEAVE_GAME);
			PauseGame(false);
			return;
		case DIALOG_CONTINUE:
			KillDialog(DIALOG_CONTINUE);
			return;
		}
	}
	else if (theResult == 999 && theDialogId == DIALOG_CONTINUE)
	{
		KillDialog(DIALOG_CONTINUE);
		if (mMainMenu != nullptr)
		{
			mWidgetManager->SetFocus(mMainMenu);
			mMainMenu->SetPlayButtonImage(IMAGE_MAINMENU_PLAY1TOUGH);
		}
	}
}

// 0x41f060
void Application::ShowUserDialog()
{
	KillDialog(DIALOG_USER);
	MakeUserDialog();
}

// 0x41f070
void Application::ReopenCreateUserDialog()
{
	KillDialog(DIALOG_CREATE_USER);
	MakeCreateUserDialog(false);
}

// 0x41f090
void Application::CreateUserDialogDone()
{
	KillDialog(DIALOG_CREATE_USER);
	mWidgetManager->MarkAllDirty();
}

// 0x41f0b0
void Application::ShowCreateAnotherUserDialog(const std::string& theText)
{
	KillDialog(DIALOG_CREATE_ANOTHER_USER);
	SetCreateUserText(MakeCreateUserDialog(true), theText);
}

// 0x41f0d0
void Application::CreateAnotherUserDialogDone()
{
	mWidgetManager->MarkAllDirty();
	KillDialog(DIALOG_CREATE_ANOTHER_USER);
}

// 0x41f980: the player list was left
void Application::UserDialogClosed(bool theSelect)
{
	Dialog* aListDialog = GetDialog(DIALOG_USER);
	if (aListDialog == nullptr)
		return;

	if (theSelect)
	{
		User* aUser = mUserManager->FindUser(GetUserDialogSelection(aListDialog));
		if (aUser != nullptr)
		{
			mUserManager->SetCurrentUser(aUser->mId);
			mWidgetManager->MarkAllDirty();
		}
	}
	KillDialog(DIALOG_USER);
	if (mMainMenu != nullptr)
		mMainMenu->UserChanged();
}

// 0x421480: "Are You Sure?" before a player is removed from the roster
void Application::DeleteUserDialog(const std::string& theName)
{
	KillDialog(DIALOG_DELETE_USER);
	Dialog* aDialog = DoDialog(DIALOG_DELETE_USER, true, "Are You Sure?",
		StrFormat("This will permanently remove '%s' from the player roster!", theName.c_str()), "", Dialog::BUTTONS_YES_NO);
	aDialog->mYesButton->mLabel = "Yes";
	aDialog->mNoButton->mLabel = "No";
}

// 0x4215f0: the answer to it
void Application::DeleteSelectedUser(bool theDelete)
{
	KillDialog(DIALOG_DELETE_USER);
	if (!theDelete)
		return;

	Dialog* aListDialog = GetDialog(DIALOG_USER);
	if (aListDialog == nullptr)
		return;

	User* aUser = mUserManager->FindUser(GetUserDialogSelection(aListDialog));
	mUserManager->DeleteUser(aUser->mId);

	User* aCurrent = mUserManager->GetCurrentUser();
	RefreshUserDialog(aListDialog, aCurrent != nullptr ? aCurrent->mName : std::string());
	mUserManager->SaveRoster();

	if (mUserManager->GetCurrentUser() == nullptr)
	{
		KillDialog(DIALOG_CREATE_USER);
		MakeCreateUserDialog(false);
	}
	mWidgetManager->MarkAllDirty();
}

// 0x421790: a saved game of the mode on screen is offered
bool Application::ShowContinueDialog()
{
	User* aUser = mUserManager->GetCurrentUser();
	if (!aUser->HasSavedGame())
		return false;

	std::string aText = "Would you like to continue your last game?\nIf you do not continue now, the game will be lost.\n\n";
	if (mGameMode == GAME_MODE_CLASSIC)
		aText += StrFormat("Level %d\nScore: %d", aUser->GetSavedGameLevel(), aUser->GetSavedGameScore());
	else if (mGameMode == GAME_MODE_TOUGH_COOKIE)
		aText += StrFormat("Tough Cookie!\nLevel %d\nScore: %d", aUser->GetSavedGameLevel(), aUser->GetSavedGameScore());
	else
		aText += StrFormat("Time: %s\nScore: %d", aUser->GetSavedGameTime().c_str(), aUser->GetSavedGameScore());

	Dialog* aDialog = DoDialog(DIALOG_CONTINUE, true, "Continue?", aText, "", Dialog::BUTTONS_YES_NO);
	aDialog->Resize(aDialog->mX, aDialog->mY, (int) ((double) mWidth * 0.7), aDialog->mHeight);
	CenterDialog(aDialog);
	aDialog->mYesButton->mLabel = "Continue";
	aDialog->mNoButton->mLabel = "New Game";
	NcMovingDialog* aMoving = dynamic_cast<NcMovingDialog*>(aDialog);
	if (aMoving != nullptr)
		aMoving->SetEscapeResult(999);
	return true;
}

// 0x421ac0
void Application::DoBonusRoundDialog()
{
	Dialog* aDialog = DoDialog(DIALOG_BONUS_ROUND, true, "Bonus Round!",
		"Be quick and grab all the coins.\n\nA miss will not damage your health.\n\nGet all coins without missing for a bonus!",
		"Start Bonus Round!", Dialog::BUTTONS_FOOTER);
	aDialog->SetLinesFont(FONT_EUROFURENCE14BOLD);
	aDialog->Resize(aDialog->mX, aDialog->mY, (int) ((double) mWidth * 0.75), aDialog->mHeight);
	CenterDialog(aDialog);
}

// 0x422500: "Leave Game?", or the way out at once when there is no game to lose
void Application::DoLeaveGameDialog()
{
	if (mGameScreen == nullptr || !mGameScreen->IsGameInProgress())
	{
		EndGameMenu();
		if (mShopScreen == nullptr && mChallengeScreen == nullptr)
			PauseGame(true);
		DoZoomWipe(WipeToMainMenu, WIPE_HAMMER);
		return;
	}

	Dialog* aDialog = DoDialog(DIALOG_LEAVE_GAME, true, "Leave Game?", "Do you want to return to the main menu?\nYour progress will be saved.",
		"", Dialog::BUTTONS_YES_NO);
	aDialog->mYesButton->mLabel = "Yes";
	aDialog->mNoButton->mLabel = "No";
	aDialog->Resize(aDialog->mX, aDialog->mY, (int) ((double) mWidth * 0.6), aDialog->mHeight);
	CenterDialog(aDialog);
}

// 0x422260: "Yes" of the leave box
void Application::LeaveConfirmed()
{
	EndGameMenu();
	if (mShopScreen == nullptr && mChallengeScreen == nullptr)
		PauseGame(true);
	DoZoomWipe(WipeToMainMenu, WIPE_HAMMER);
}

// 0x41fec0: the "Ready?" box that waits for the player
void Application::DoReadyGoDialog(bool thePlaySound, const std::string& theHeader, const std::string& theText)
{
	PauseGame(true);
	MakeReadyGoDialog(theHeader, theText);
	if (thePlaySound)
		PlaySample(SOUND_VOICE_BONUS_READY);
}

// 0x41fdc0: the score of the player goes into the table; the table is shown when the player has made it
bool Application::ShowHighScores(bool theShow)
{
	HighScoreDisplayDialog* aTable = MakeHighScoreDisplay();
	aTable->Prepare();
	User* aUser = mUserManager->GetCurrentUser();
	aTable->RecordScore(aUser != nullptr ? aUser->mName : std::string());
	aTable->MarkDirty();

	if (GetDialog(DIALOG_HIGH_SCORES) == nullptr || !theShow)
	{
		KillDialog(aTable);
		aTable = nullptr;
	}
	if (GetDialog(DIALOG_HIGH_SCORES) != nullptr && !theShow)
		KillDialog(DIALOG_HIGH_SCORES);

	if (aTable != nullptr)
	{
		aTable->mShown = true;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// The pause count

// 0x41f790
void Application::PauseGame(bool thePause)
{
	if (!thePause)
	{
		if (mPauseCount == 0)
			return;		// the original only writes a debug line here

		mPauseCount--;
		if (mPauseCount == 0 && mGameScreen != nullptr)
		{
			mFocusedUpdate = mGameScreen->mUpdateCnt;
			mGameScreen->mMenuButton->SetDisabled(false);
			mGameScreen->ShowCrosshair();
			mWidgetManager->SetFocus(mGameScreen);
		}
		return;
	}

	mPauseCount++;
	if (mPauseCount == 1 && mGameScreen != nullptr)
	{
		mGameScreen->mMenuButton->SetDisabled(true);
		mGameScreen->UpdateHover();
	}
}

// 0x41f890
void Application::ShowGameMenu()
{
	if (mPauseCount < 1)
		PauseGame(true);
	MakeGameMenu();
}

// 0x41f8c0
void Application::EndGameMenu()
{
	KillDialog(DIALOG_GAME_MENU);
	if (mShopScreen == nullptr && mChallengeScreen == nullptr)
		PauseGame(false);

	if (mShopScreen != nullptr)
		mWidgetManager->SetFocus(mShopScreen);
	else if (mChallengeScreen != nullptr)
		mWidgetManager->SetFocus(mChallengeScreen);
	else if (mGameScreen != nullptr)
		mWidgetManager->SetFocus(mGameScreen);
}

// 0x41fb00
void Application::GotFocus()
{
	if (mPauseCount > 1)
		PauseGame(false);

	if (mGameScreen != nullptr)
	{
		if (mPauseCount < 1)
			mGameScreen->ShowCrosshair();
		else
			mGameScreen->UpdateHover();
	}
	if (mChallengeScreen != nullptr)
		mChallengeScreen->GotFocus();
}

// 0x41fb60
void Application::LostFocus()
{
	PauseGame(true);
	if (mChallengeScreen != nullptr)
		mChallengeScreen->LostFocus();
}

// 0x41fbf0: the intro page of a level was left
void Application::LevelIntroDone()
{
	PauseGame(false);
	PauseGame(false);
	mLevelIntro = nullptr;
	if (mGameScreen != nullptr)
		mWidgetManager->SetFocus(mGameScreen);
}

//////////////////////////////////////////////////////////////////////////
// The zoom wipe

// 0x41eab0
void Application::DoZoomWipe(WipeCallback theCallback, int theKind)
{
	ZoomWipeWidget* aWipe = new ZoomWipeWidget();
	mWidgetManager->AddWidget(aWipe);
	mWidgetManager->SetFocus(aWipe);

	Image* aTo = nullptr;
	Image* aFrom = nullptr;
	switch (theKind)
	{
	case WIPE_HAMMER:
		aTo = IMAGE_ZOOMWIPEB_HAMMER;
		aFrom = IMAGE_ZOOMWIPEA_HAMMER;
		break;
	case WIPE_TROPHY:
		aTo = IMAGE_ZOOMWIPEB_TROPHY;
		aFrom = IMAGE_ZOOMWIPEA_TROPHY;
		break;
	case WIPE_LEVEL_UP:
		aTo = IMAGE_ZOOMWIPELEVELUPB;
		aFrom = IMAGE_ZOOMWIPELEVELUPA;
		break;
	case WIPE_SHOP:
		aTo = IMAGE_ZOOMWIPEB_SHOP;
		aFrom = IMAGE_ZOOMWIPEA_SHOP;
		break;
	}
	aWipe->Setup(aFrom, aTo, Color(0xff, 0xac, 0x1d), theCallback);
	mZoomWipe = aWipe;
	mZooming = true;
}

// 0x41f380: the wipe is over
void Application::EndZoomWipe()
{
	mZooming = false;
	mWidgetManager->RemoveWidget(mZoomWipe);
	SafeDeleteWidget(mZoomWipe);

	if (mQuitting)
	{
		if (mMainMenu != nullptr)
		{
			DropWidget(mMainMenu);
			mZoomWipe = nullptr;
			return;
		}
		if (mGameScreen != nullptr)
		{
			KillDialog(DIALOG_GAME_MENU);
			KillDialog(2);
			DeleteGameScreen();
		}
	}
	mZoomWipe = nullptr;
}

void Application::WipeToMainMenu()
{
	gApp->MainMenuFromGameScreen();
}

void Application::WipeToShop()
{
	gApp->ShopDone();
}

void Application::WipeToGame()
{
	gApp->ChallengeDone();
}

void Application::WipeNextLevel()
{
	gApp->NextLevelStarts();
}

// 0x41fd00
void Application::WipeGameWon()
{
	gApp->mGameWonPending = false;
	gApp->PauseGame(true);
	gApp->mGameWon = MakeGameWon();
}

// 0x422a10: "New Game" of the continue box, once the screen is covered
void Application::WipeNewGame()
{
	gApp->KillDialog(DIALOG_CONTINUE);
	if (gApp->mMainMenu != nullptr)
		DropWidget(gApp->mMainMenu);
	if (gApp->mGameMode == GAME_MODE_CLASSIC || gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
		gApp->ResurrectOrPlay(false);
	else
		gApp->CreateGame(false);
}

// 0x422950: "Continue" of the continue box, once the screen is covered
void Application::WipeContinueGame()
{
	gApp->KillDialog(DIALOG_CONTINUE);
	if (gApp->mMainMenu != nullptr)
		DropWidget(gApp->mMainMenu);
	gApp->CreateGameScreen(true, false);
}

// 0x41f310
void Application::WipeOpenShop()
{
	gApp->OpenShopScreen();
}

// 0x41f330
void Application::WipeOpenChallenge()
{
	gApp->OpenChallengeScreen();
}

//////////////////////////////////////////////////////////////////////////
// Making and taking down the screens

// 0x41e6e0: the game screen, then the game it shows: a new one, a saved one, or one from a restart point
void Application::CreateGameScreen(bool theContinue, bool theFromRestartPoint)
{
	mGameScreen = new GameScreen();
	mGameScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mGameScreen);
	mWidgetManager->SetFocus(mGameScreen);

	if (!theContinue)
	{
		if (!theFromRestartPoint)
			mGameScreen->Start(mGameMode);
		else
			mUserManager->GetCurrentUser()->ResumeFromRestartPoint(mStartLevel);
	}
	else
		mUserManager->GetCurrentUser()->LoadSavedGame();

	if (GetDialog(DIALOG_READY_GO) != nullptr)
		mWidgetManager->BringToFront(GetDialog(DIALOG_READY_GO));

	StartMusic(MUSIC_GAME, false);
}

// 0x420ff0
void Application::CreateGame(bool theContinue)
{
	bool aFromRestartPoint = !theContinue && mStartLevel != 0;
	CreateGameScreen(theContinue, aFromRestartPoint);
	if (theContinue)
		return;

	User* aUser = mUserManager->GetCurrentUser();
	if (mStartLevel == 0)
	{
		bool aMadeReadyBox = CreateLevelIntro();
		if (aUser->mShowHowToPlay)
			ShowHowToPlay(true);
		else if (aMadeReadyBox)
			PlaySample(SOUND_VOICE_BONUS_READY);
	}
	else
	{
		mStartLevel = 0;
		CreateLevelIntro();
		if (aUser->mShowHowToPlay)
			ShowHowToPlay(true);
	}
	aUser->ClearSavedGame();
}

// 0x420c80: the page that shows what is new in the level, or the "Ready?" box
bool Application::CreateLevelIntro()
{
	mLevelIntro = MakeLevelIntro(mGameScreen);
	if (mLevelIntro != nullptr)
	{
		PauseGame(true);
		mLevelIntro->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mLevelIntro);
		mWidgetManager->SetFocus(mLevelIntro);
		return false;
	}

	if (mGameMode != GAME_MODE_MARATHON)
		DoReadyGoDialog(false, "Ready?", "");
	else
		DoReadyGoDialog(false, "Marathon", "Conserve your health!\n\nGnomes will not drop hearts,\nbut you can buy health\nat the shop.");
	return true;
}

// 0x420dc0
void Application::CreateMainMenu()
{
	mMainMenu = new MainMenu();
	mMainMenu->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mMainMenu);
	mWidgetManager->SetFocus(mMainMenu);
	StartMusic(MUSIC_MENU, false);

	User* aUser = mUserManager->GetCurrentUser();
	if (aUser != nullptr && aUser->mToughUnlocked && !aUser->mToughAnnounced)
	{
		Dialog* aDialog = NewDialog(DIALOG_TOUGH_UNLOCKED, true, "A real challenge...",
			"You have unlocked Tough Cookie,\na new extra difficult game mode!", "OK", Dialog::BUTTONS_FOOTER);
		LayoutInfoDialog(aDialog, 400);
		int aDialogWidth = aDialog->mWidth;
		int aDialogHeight = aDialog->GetPreferredHeight(aDialogWidth);
		aDialog->Resize((mWidth - aDialogWidth) / 2, -aDialogHeight, aDialogWidth, aDialogHeight);
		AddDialog(DIALOG_TOUGH_UNLOCKED, aDialog);
		aUser->mToughAnnounced = true;
	}
}

// 0x41e9f0
void Application::CreateLoadingScreen()
{
	mLoadingScreen = new LoadingScreen();
	mLoadingScreen->Resize(0, 0, mWidth, mHeight);
	mLoadingScreen->Begin();
	mWidgetManager->AddWidget(mLoadingScreen);
	StartMusic(MUSIC_MENU, true);
}

// 0x41e810
void Application::CreateShopScreen()
{
	mShopScreen = new ShopScreen();
	mShopScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mShopScreen);
	mWidgetManager->SetFocus(mShopScreen);
}

// 0x41e8b0
void Application::CreateChallengeScreen()
{
	mChallengeScreen = new ChallengeScreen();
	mChallengeScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mChallengeScreen);
	mWidgetManager->SetFocus(mChallengeScreen);
	StartMusic(MUSIC_BONUS, false);
}

// 0x41f130
void Application::OpenShopScreen()
{
	CreateShopScreen();
	mShopScreen->Opened();
}

// 0x41f180
void Application::OpenChallengeScreen()
{
	CreateChallengeScreen();
	mChallengeScreen->Opened(0);
}

// 0x41f640
void Application::CreateTrophyRoom(int theMode, const std::vector<int>& theTrophies)
{
	mTrophyRoom = new TrophyRoomWidget(theMode, theTrophies);
	mTrophyRoom->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTrophyRoom);
	StartMusic(MUSIC_HIGH_SCORE, false);
}

// 0x41f700: the trophy room from the main menu
void Application::ShowTrophyRoomFromMenu()
{
	DropWidget(mMainMenu);
	CreateTrophyRoom(0, std::vector<int>());
}

// 0x41ed40: the high score table from the main menu
void Application::ShowHighScoreTableFromMenu()
{
	DropWidget(mMainMenu);
	MakeHighScoreDisplay()->mShown = true;
	StartMusic(MUSIC_HIGH_SCORE, false);
}

// 0x41e960
void Application::DeleteGameScreen()
{
	mWidgetManager->RemoveWidget(mGameScreen);
	SafeDeleteWidget(mGameScreen);
	mGameScreen = nullptr;
}

// 0x41e9a0
void Application::DeleteChallengeScreen()
{
	mChallengeOpening = false;
	mChallengeLeaving = false;
	mWidgetManager->RemoveWidget(mChallengeScreen);
	SafeDeleteWidget(mChallengeScreen);
	mChallengeScreen = nullptr;
	StartMusic(MUSIC_GAME, false);
}

// 0x41ec90: the screens of a game
void Application::RemoveAllScreens()
{
	if (mGameScreen != nullptr)
		DropWidget(mGameScreen);
	if (mShopScreen != nullptr)
		DropWidget(mShopScreen);
	if (mChallengeScreen != nullptr)
		DeleteChallengeScreen();
	if (mResurrectScreen != nullptr)
		DropWidget(mResurrectScreen);
}

// 0x41ec30, 0x41ec50: the how-to-play movie waits for the shared SWF player and is left out
void Application::ShowHowToPlay(bool)
{
}

void Application::HideHowToPlay()
{
}

//////////////////////////////////////////////////////////////////////////
// What the buttons and the wipes lead to

// 0x421150: the wipe back to the menu has covered the screen
void Application::MainMenuFromGameScreen()
{
	KillDialog(DIALOG_LEAVE_GAME);
	PauseGame(false);
	RemoveAllScreens();
	if (GetStats()->Get(STAT_HEALTH) != 0.0 || !ShowHighScores(true))
		CreateMainMenu();
	ObjectManager::Get()->Clear();
}

// 0x4211c0
void Application::TrophyRoomToMainMenu()
{
	DropWidget(mTrophyRoom);
	CreateMainMenu();
}

// 0x4213d0
void Application::HighScoresToMainMenu()
{
	KillDialog(DIALOG_HIGH_SCORE_TABLE);
	CreateMainMenu();
}

// 0x4213f0: the game was won and the player leaves the page that says so
void Application::GameEndToMainMenu()
{
	DropWidget(mGameWon);
	PauseGame(false);
	GetStats()->Set(STAT_HEALTH, 0.0);
	RemoveAllScreens();
	if (!ShowHighScores(true))
		CreateMainMenu();
	ObjectManager::Get()->Clear();
}

// 0x421c30
void Application::GameWonToMainMenu()
{
	KillDialog(DIALOG_GAME_WON);
	PauseGame(false);
	RemoveAllScreens();
	CreateMainMenu();
	ObjectManager::Get()->Clear();
	if (mLevelIntro != nullptr)
		DropWidget(mLevelIntro);
}

// 0x422460
void Application::GameOverDone()
{
	KillDialog(DIALOG_GAME_OVER);
	DoZoomWipe(WipeToMainMenu, WIPE_HAMMER);
}

// 0x422bb0: "try again" after a game over
void Application::RetryFromGameOver()
{
	KillDialog(DIALOG_GAME_OVER);
	if (mPauseCount > 0)
		PauseGame(false);
	RemoveAllScreens();
	ShowHighScores(false);
	ObjectManager::Get()->Clear();
	GetRulesSheet(true);
	GetStats()->Reset();
	if (!ShowContinueDialog())
		DoZoomWipe(WipeNewGame, WIPE_HAMMER);
}

// 0x422a80: a game of a mode is chosen on the main menu
void Application::NewGame(int theMode)
{
	mGameMode = theMode;
	GetRulesSheet(true);
	GetStats()->Reset();
	if (!ShowContinueDialog())
		DoZoomWipe(WipeNewGame, WIPE_HAMMER);
}

// 0x422180: the choice of a restart point, or the game at once when there is no choice
void Application::ResurrectOrPlay(bool theFlag)
{
	User* aUser = mUserManager->GetCurrentUser();
	if (aUser->GetRestartPointCount(mGameMode) > 1)
	{
		mResurrectScreen = new ResurrectScreenWidget(theFlag);
		mResurrectScreen->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mResurrectScreen);
		mWidgetManager->SetFocus(mResurrectScreen);
		return;
	}
	CreateGame(false);
}

// 0x422140: a restart point was chosen
void Application::ResurrectDone()
{
	DropWidget(mResurrectScreen);
	CreateGame(false);
}

// 0x4212f0: the player goes back to the game from the restart point choice
void Application::ResurrectBackToGame()
{
	PauseGame(false);
	DropWidget(mResurrectScreen);
	mWidgetManager->SetFocus(mGameScreen);
	DoReadyGoDialog(true, "Ready?", "");
}

// 0x422480
void Application::LeaveShop()
{
	mShopOpening = false;
	DoZoomWipe(WipeToShop, WIPE_SHOP);
}

// 0x4224a0
void Application::LeaveChallenge()
{
	mChallengeOpening = false;
	mChallengeLeaving = true;
	DoZoomWipe(WipeToGame, WIPE_HAMMER);
}

// 0x421ca0: the shop is gone from the screen
void Application::ShopDone()
{
	mShopScreen->Closing();
	DropWidget(mShopScreen);
	mWidgetManager->SetFocus(mGameScreen);
	DoReadyGoDialog(true, "Ready?", "");
}

// 0x421d70: the bonus round is gone from the screen
void Application::ChallengeDone()
{
	mChallengeOpening = false;
	mChallengeScreen->Closing();
	DeleteChallengeScreen();
	if (mGameScreen != nullptr)
		mWidgetManager->SetFocus(mGameScreen);
	DoReadyGoDialog(true, "Ready?", "");
}

// 0x421e70: the level-up wipe covers the screen: the next level starts
void Application::NextLevelStarts()
{
	mLevelUpPending = false;
	GameScreen* aGameScreen = mGameScreen;
	aGameScreen->NextLevel();
	aGameScreen->Start(aGameScreen->mMode);
	CreateLevelIntro();
	if (mLevelIntro == nullptr)
		mWidgetManager->SetFocus(aGameScreen);
}

// 0x422770: what the removal of a game screen's pieces set off
void Application::ProcessPending()
{
	if ((mShopOpening && mShopScreen == nullptr) || (mChallengeOpening && mChallengeScreen == nullptr))
		ObjectManager::Get()->DeactivateAll();
	if (mChallengeLeaving)
		ChallengeDone();
	if (mLevelUpPending)
		NextLevelStarts();
	if (mGameWonPending)
	{
		mGameWonPending = false;
		PauseGame(true);
		mGameWon = MakeGameWon();
	}

	User* aUser = mUserManager->GetCurrentUser();
	aUser->StoreGameSnapshot();
	aUser->SaveProfile();
}

// 0x422860: the level was finished
void Application::LevelFinished()
{
	if (mGameMode == GAME_MODE_CLASSIC || mGameMode == GAME_MODE_TOUGH_COOKIE)
	{
		bool aLastLevel = GetRulesSheet()->IsLastRow(mGameScreen->mLevel - 1);
		PlaySample(SOUND_STING_LEVEL_UP);
		if (aLastLevel)
		{
			DoZoomWipe(WipeGameWon, WIPE_HAMMER);
			mGameWonPending = true;
			return;
		}
		DoZoomWipe(WipeNextLevel, WIPE_LEVEL_UP);
		mLevelUpPending = true;
	}
	else if (mGameMode == GAME_MODE_MARATHON)
		mGameScreen->NextLevel();
}

// 0x41fa70
void Application::DoGameOver()
{
	PauseGame(true);
	MakeGameOverDialog();
}

// 0x41fa90
void Application::DoShopScreen()
{
	PauseGame(true);
	mShopOpening = true;
	DoZoomWipe(WipeOpenShop, WIPE_SHOP);
}

// 0x41fac0
void Application::DoChallengeScreen()
{
	PauseGame(true);
	mChallengeOpening = true;
	mChallengeLeaving = false;
	DoZoomWipe(WipeOpenChallenge, WIPE_HAMMER);
}

// 0x41f950
void Application::GameToTrophyRoom(const std::vector<int>& theTrophies)
{
	PauseGame(true);
	CreateTrophyRoom(1, theTrophies);
}

void Application::Quit()
{
	mQuitting = true;
}

//////////////////////////////////////////////////////////////////////////
// The cheat words

// 0x421ec0: a letter was typed; a word that is complete switches its cheat, one that could still be completed waits, and a
// letter that fits none of them starts the typing over with itself
void Application::KeyChar(char theChar)
{
	mTyped += theChar;

	bool aCouldComplete = false;
	bool aSwitched = false;
	for (int i = 0; i < NUM_CHEATS; i++)
	{
		std::string aWord = kCheatWords[i];
		if (mTyped == aWord)
		{
			ToggleCheat(i);
			aSwitched = true;
		}
		else if (aWord.compare(0, mTyped.size(), mTyped) == 0)
			aCouldComplete = true;
	}

	if (aSwitched)
	{
		mTyped = "";
		return;
	}
	if (!aCouldComplete)
		mTyped = std::string(1, theChar);
}
