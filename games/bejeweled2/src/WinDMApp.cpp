// WinDMApp.cpp: Sexy::WinDMApp (WinDMApp.h), the application class of Bejeweled 2 Deluxe: start-up, the loading of the
// resources, the screen modes and the registry. The dialogs are in WinDMAppDialogs.cpp, the gem images in
// WinDMAppImages.cpp.
#include "WinDMApp.h"

#include "BackdropsParser.h"
#include "Board.h"
#include "Buffer.h"
#include "ButtonWidget.h"
#include "CustomResourceManager.h"
#include "EffectOverlay.h"
#include "Dialog.h"
#include "FancyBackdrop.h"
#include "GalaxyParser.h"
#include "HighScoreWidget.h"
#include "MainMenu.h"
#include "MemoryImage.h"
#include "ReloadDisplay.h"
#include "DDImage.h"
#include "DDInterface.h"
#include "SDLMusicInterface.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "TitleScreen.h"
#include "WorkerThread.h"
#include "WidgetManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <algorithm>
#include <set>

using namespace Sexy;

WinDMApp* Sexy::gApp = nullptr;   // 0x67ccd8

// 0x5e1b49, 0x5dbac6 (BackdropColors' constructor, run for each of the 50 entries: both colors white, here the loop below)
WinDMApp::WinDMApp()
{
	gApp = this;
	mBoard = nullptr;
	mGalaxyParser = new GalaxyParser();
	mProfile = new Profile();
	mWorker = new WorkerThread();
	mBackdropsParser = new BackdropsParser(this);
	for (BackdropColors& c : mBackdropColors)
		c.mColor1 = c.mColor2 = Color(255, 255, 255, 255);
	mPendingMusicOffset = -1;
	mPendingMusicDelay = 0;
	mGameMode = 0;
	mPuzzleFlag = false;
	mInitialMenuCleared = false;
	mAutoEnable3D = true;
	mCurrentSong = 0;
	for (int& s : mModeSeconds)
		s = 0;
	mTitleScreen = nullptr;
	mFancyBackdrop = nullptr;
	mMainMenu = nullptr;
	mReloadPending = false;
	mShrunkenGems = nullptr;
	mHighScoreWidget = nullptr;
	mBombifiedStillStrip = nullptr;
	mWasInGame = false;
	mReloadDisplay = nullptr;
	mReloading = false;
	mShrinkLoads = 0;
	mShrinkFill = 0;
	m3dImagesLoaded = false;
	mNormBonus = mEndlessBonus = mTimedBonus = 0;
	mNormBonusMult = mNormPMBase = mNormPMMult = 0.0;
	mEndlessBonusMult = mEndlessPMBase = 0.0;
	mTimedBonusMult = mTimedBonusAccel = mTimedPMBase = mTimedPMMult = 0.0;
	mGraceSeconds = mGraceSecondsMult = mGraceSecondsMax = 0.0;
	mWantWindowed = false;
	mTestGoto = -1;
	mTestUnlock = false;

	mProdName = "Bejeweled2";
	// port: the original's SexyAppBase ctor reads this from the exe's version resource (ProductVersion)
	mProductVersion = "1.1";
	mTitle = "Bejeweled 2 Deluxe " + mProductVersion;
	mRegKey = "PopCap\\Bejeweled2";

	// the size of the screen: the original asks GetSystemMetrics for the client area of a full-screen window
	mScreenWidth = 1920;
	mScreenHeight = 1080;
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == 0) {
		SDL_DisplayMode aMode;
#ifdef __EMSCRIPTEN__
		// port: in a browser SDL's usable bounds are the page's inner size (window.innerWidth and innerHeight: the frame the
		// game is shown in, 640 by 480 as the site's Desktop 95 window starts), not a screen's, and the page scales the
		// game's canvas to the room it has. So the screen is the display itself, and never less than a 1024 by 768 window
		// needs: the Options dialog's and SwitchScreenMode's tests for a desktop too small for the windowed game, or for
		// high resolution in a window, are about a window that cannot be scaled. With the page's 640 the game refused to
		// leave full screen ("Your desktop resolution is not high enough...", the user, 2026-11-10)
		if (SDL_GetDesktopDisplayMode(0, &aMode) == 0) {
			mScreenWidth = aMode.w;
			mScreenHeight = aMode.h;
		}
		if (mScreenWidth < 1280)
			mScreenWidth = 1280;
		if (mScreenHeight < 1024)
			mScreenHeight = 1024;
#else
		SDL_Rect aBounds;
		if (SDL_GetDisplayUsableBounds(0, &aBounds) == 0 && aBounds.w > 0 && aBounds.h > 0) {
			mScreenWidth = aBounds.w;
			mScreenHeight = aBounds.h;
		} else if (SDL_GetDesktopDisplayMode(0, &aMode) == 0) {
			mScreenWidth = aMode.w;
			mScreenHeight = aMode.h;
		}
#endif
	}
	mWidth = 1024;
	mHeight = 768;
	mCustomCursorsEnabled = false;
	mIsDiamondMine = false;
	mHighRes = true;
	mAlwaysUpdate = false;

	// the base class's resource manager gives way to the game's
	delete mResourceManager;
	mResourceManager = new CustomResourceManager(this);
	mMusicVolume = 0.35;
	mSfxVolume = 0.85;
}

// 0x5e3476
WinDMApp::~WinDMApp()
{
	delete mGalaxyParser;
	delete mProfile;
	delete mBackdropsParser;
	delete mWorker;
}

// 0x5dbb94: the 1024x768 layout in 640x480 pixels
int WinDMApp::ScaleNum(int theValue, int theOffset)
{
	if (mWidth != 1024)
		theValue = (theValue * 640) / 1024 + theOffset;   // C division, as the original's shift and correction
	return theValue;
}

// 0x5dbbbf
int WinDMApp::ScaleNumUp(int theValue, int theOffset)
{
	if (mWidth != 1024)
		theValue = (theValue * 640 + 639) / 1024 + theOffset;
	return theValue;
}

// 0x5dbb64
float WinDMApp::ScaleNum(float theValue, float theOffset)
{
	if (mWidth != 1024)
		theValue = theValue * 640.0f * 0.0009765625f + theOffset;
	return theValue;
}

// 0x5dbb3f
double WinDMApp::ScaleNum(double theValue, double theOffset)
{
	if (mWidth != 1024)
		theValue = theValue * 640.0 * 0.0009765625 + theOffset;
	return theValue;
}

// 0x5e2a19
void WinDMApp::Init()
{
	SexyApp::Init();
	LoadResourceManifest();

	// port: the original moves a Windows Vista user's old profiles (users\) into the application data folder here
	mProfile->ReadProfileList();
	std::string aLastProfile;
	if (RegistryReadString("LastProfileName", &aLastProfile))
		mProfile->LoadProfile(aLastProfile);

	LoadProperties("properties\\config.xml", true, false);
	LoadProperties("properties\\music.xml", true, false);
	Buffer aBackdropsBuffer;
	ReadBufferFromFile("properties\\backdrops.xml", &aBackdropsBuffer, false);
	mBackdropsParser->Parse(std::string((const char*)aBackdropsBuffer.GetDataPtr(), aBackdropsBuffer.GetDataLen()));

	mNormBonus = (int)GetDouble("NormBonus");
	mNormBonusMult = GetDouble("NormBonusMult");
	mNormPMBase = GetDouble("NormPMBase");
	mNormPMMult = GetDouble("NormPMMult");
	mTimedBonus = (int)GetDouble("TimedBonus");
	mTimedBonusMult = GetDouble("TimedBonusMult");
	mTimedBonusAccel = GetDouble("TimedBonusAccel");
	mTimedPMBase = GetDouble("TimedPMBase");
	mTimedPMMult = GetDouble("TimedPMMult");
	mEndlessBonus = (int)GetDouble("EndlessBonus");
	mEndlessBonusMult = GetDouble("EndlessBonusMult");
	mEndlessPMBase = GetDouble("EndlessPMBase");
	mGraceSeconds = GetDouble("GraceSeconds");
	mGraceSecondsMult = GetDouble("GraceSecondsMult");
	mGraceSecondsMax = GetDouble("GraceSecondsMax");
	LoadProperties();

	DIALOG_YES_STRING = GetString("DIALOG_BUTTON_YES");
	DIALOG_NO_STRING = GetString("DIALOG_BUTTON_NO");
	DIALOG_OK_STRING = GetString("DIALOG_BUTTON_OK");
	DIALOG_CANCEL_STRING = GetString("DIALOG_BUTTON_CANCEL");

	// port: the original tests the MaxPlays, MaxExecutions and MaxTime limits of the trial here
	BuildBackdropList();
	BuildPuzzlePackList();

	mResourceManager->LoadResources("LoadScreen");
	if (!ExtractLoadScreenResources(mResourceManager))
		ShowResourceError(true);
	mResourceManager->LoadResources("Init");
	if (!ExtractInitResources(mResourceManager))
		ShowResourceError(true);
	mResourceManager->LoadResources("NonResize");
	if (!ExtractNonResizeResources(mResourceManager))
		ShowResourceError(true);
	mResourceManager->LoadResources("InitialMainMenu");
	if (!ExtractInitialMainMenuResources(mResourceManager))
		ShowResourceError(true);

	mFancyBackdrop = new FancyBackdrop(this);
	mFancyBackdrop->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mFancyBackdrop);
	mFancyBackdrop->SetVisible(false);

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);

	mMusicInterface->LoadMusic(0, "music\\BeyondNetwork.mo3");
	mMusicInterface->PlayMusic(0, GetInteger("MUSICOFFSET_LOADING"), false);

	SetCursorImage(0, IMAGE_CURSOR_POINTER);
	SetCursorImage(1, IMAGE_CURSOR_HAND);
	SetCursorImage(2, IMAGE_CURSOR_DRAGGING);
	SetCursorImage(3, IMAGE_CURSOR_TEXT);
	// port: the original checks for an update here when the player turned it on (CheckForUpdates)
}

// 0x5e2531: the backdrops of images\backdrops, in the order of backdrops.lst, then the jpg files that it lacks
void WinDMApp::BuildBackdropList()
{
	std::vector<uint8_t> aList;
	if (Platform::VFS::Get().ReadAll("images/backdrops/backdrops.lst", aList)) {
		std::string aText(aList.begin(), aList.end());
		size_t aPos = 0;
		while (aPos < aText.size()) {
			size_t anEnd = aText.find('\n', aPos);
			if (anEnd == std::string::npos)
				anEnd = aText.size();
			std::string aLine = Trim(aText.substr(aPos, anEnd - aPos));
			if (!aLine.empty())
				mBackdrops.push_back(aLine);
			aPos = anEnd + 1;
		}
	}
	std::set<std::string> aMissing;
	for (const std::string& anEntry : Platform::ListDirectory(Platform::VFS::Get().GetBaseDir() + "/images/backdrops")) {
		if (anEntry.size() < 4 || StringToLower(anEntry.substr(anEntry.size() - 4)) != ".jpg")
			continue;
		std::string aName = anEntry;
		size_t aDot = aName.find('.');
		if (aDot != std::string::npos)
			aName = aName.substr(0, aDot);
		if (std::find(mBackdrops.begin(), mBackdrops.end(), aName) == mBackdrops.end())
			aMissing.insert(aName);
	}
	mBackdrops.insert(mBackdrops.end(), aMissing.begin(), aMissing.end());
	// port: the original writes the list back to backdrops.lst (the game's folder is not ours to write)
}

// 0x5e2832: the folders of puzzles\ that hold a galaxy.xml, sorted
void WinDMApp::BuildPuzzlePackList()
{
	std::set<std::string> aPacks;
	std::string aDir = Platform::VFS::Get().GetBaseDir() + "/puzzles";
	for (const std::string& anEntry : Platform::ListDirectory(aDir)) {
		if (!Platform::IsDirectory(aDir + "/" + anEntry) || anEntry == "CVS")
			continue;
		if (FileExists("puzzles\\" + anEntry + "\\galaxy.xml"))
			aPacks.insert(anEntry);
	}
	for (const std::string& aPack : aPacks)
		mPuzzlePacks.push_back(aPack);
}

// 0x5dd641: the profile's puzzle pack is one of the packs that exist, else the first
void WinDMApp::ValidatePuzzlePack()
{
	if (std::find(mPuzzlePacks.begin(), mPuzzlePacks.end(), mProfile->mPuzzlePack) == mPuzzlePacks.end() && !mPuzzlePacks.empty())
		mProfile->mPuzzlePack = mPuzzlePacks[0];
}

// 0x5e0957
void WinDMApp::LoadGalaxyData()
{
	ValidatePuzzlePack();
	Buffer aBuffer;
	bool anOk = ReadBufferFromFile("puzzles\\" + mProfile->mPuzzlePack + "\\galaxy.xml", &aBuffer, false);
	if (!anOk)
		mGalaxyParser->Fail("Unable to open galaxy.xml file");
	mGalaxyParser->mPuzzlePack = mProfile->mPuzzlePack;
	mGalaxyParser->ParseBuffer(aBuffer);
	if (mGalaxyParser->HasFailed())
		Popup("Galaxy Parsing Failed: " + mGalaxyParser->GetErrorText());
}

// 0x5dbdba
void WinDMApp::KillBoard()
{
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = nullptr;
	}
}

// 0x5dbbef: the main menu and the fancy backdrop behind it
void WinDMApp::KillMainMenu()
{
	if (mFancyBackdrop != nullptr) {
		mWidgetManager->RemoveWidget(mFancyBackdrop);
		SafeDeleteWidget(mFancyBackdrop);
		mFancyBackdrop = nullptr;
	}
	if (mMainMenu != nullptr) {
		mWidgetManager->RemoveWidget(mMainMenu);
		SafeDeleteWidget(mMainMenu);
		mMainMenu = nullptr;
	}
}

// 0x5dbc4e
void WinDMApp::DoNewGame()
{
	bool aHadHighScores = false;
	if (mHighScoreWidget != nullptr) {
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		SafeDeleteWidget(mHighScoreWidget);
		mHighScoreWidget = nullptr;
		aHadHighScores = true;
	}
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}
	mBoard = new Board(this);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);
	if (mMainMenu == nullptr) {
		mBoard->ShowNow();
		if (!aHadHighScores)
			goto done;
	} else {
		mMainMenu->mState = 5;
	}
	mBoard->SetVisible(false);
	mBoard->mEffectOverlay->DoPortal();
	ClearUpdateBacklog(false);
	mBoard->mEffectOverlay->mClearBacklogAfterPortal = true;
	PlaySample(SOUND_MAIN_GAME_START);
done:
	if (mGameMode == 2)
		mPuzzleFlag = false;
}

// 0x5dcd44: a board that a cheat changed the pictures of (mImagesChanged) is deleted at once and the pictures are loaded again; any
// other stays behind the menu (it slides out and is saved, so that its game can be resumed)
void WinDMApp::DoMainMenu()
{
	if (mBoard != nullptr && mBoard->mImagesChanged) {
		mReloadPending = true;
		KillBoard();
	}
	if (mMainMenu == nullptr) {
		mMainMenu = new MainMenu(this);
		if (mInitialMenuCleared)
			mMainMenu->mBackdropClear = true;
		mMainMenu->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mMainMenu);
	}
}

// 0x5de1a6
void WinDMApp::DoBoardShow()
{
	mBoard->SetVisible(true);
	if (mGameMode != 2 && mProfile->WantsHint(3) && mBoard->mNoMoveYet) {
		mBoard->HideNow();
		DoHelpScreen();
	} else {
		mBoard->Show();
	}
	KillMainMenu();
	ClearInitialMainMenu();
}

// 0x5de14d: the pictures of the first main menu are not needed any more
void WinDMApp::ClearInitialMainMenu()
{
	if (!mInitialMenuCleared) {
		mResourceManager->DeleteResources("InitialMainMenu");
		mInitialMenuCleared = true;
	}
}

// 0x5dec6d
void WinDMApp::KillTitleScreen()
{
	SafeDeleteWidget(mTitleScreen);
	mWidgetManager->RemoveWidget(mTitleScreen);
	mTitleScreen = nullptr;
	mResourceManager->DeleteResources("LoadScreen");
}

// 0x5decdc: the title screen is done with its show; the main menu opens
void WinDMApp::LoadingCompleted()
{
	mTitleScreen->mLoadingCompleted = true;
	mMusicInterface->FadeOut(mCurrentSong, true, 0.006);
	mPendingMusicOffset = GetInteger("MUSICOFFSET_MAIN_MENU");
	DoMainMenu();
	mMainMenu->Show();
	mFancyBackdrop->mShowing = true;
	mFancyBackdrop->SetVisible(true);
	int aSound = SOUND_WELCOME_BACK;
	if (mProfile->mName.empty() && !mProfile->GetAnyProfile()) {
		mMainMenu->mUserDialogTimer = 125;
		aSound = SOUND_WELCOME;
	}
	PlaySample(aSound);
	mMainMenu->SyncProfile();
}

// 0x5de207: the sounds and the pictures that the loading screen waits for
void WinDMApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = mResourceManager->GetNumResources("Sounds") + mResourceManager->GetNumResources("LoadingThread") + 10;
	bool is3d = Is3DAccelerated();
	if (is3d)
		mNumLoadingThreadTasks += mResourceManager->GetNumResources("3dImages");

	mResourceManager->StartLoadResources("LoadingThread");
	while (mResourceManager->LoadNextResource())
		mCompletedLoadingThreadTasks++;
	if (!mResourceManager->HadError()) {
		bool anOk = true;
		if (Is3DAccelerated()) {
			mResourceManager->StartLoadResources("3dImages");
			while (mResourceManager->LoadNextResource())
				mCompletedLoadingThreadTasks++;
			if (mResourceManager->HadError())
				anOk = false;
			else
				m3dImagesLoaded = true;
		}
		if (anOk) {
			mResourceManager->StartLoadResources("Sounds");
			while (mResourceManager->LoadNextResource())
				mCompletedLoadingThreadTasks++;
			if (!mResourceManager->HadError()) {
				if (is3d) {
					mResourceManager->StartLoadResources("3dImages");
					while (mResourceManager->LoadNextResource())
						mCompletedLoadingThreadTasks++;
					if (mResourceManager->HadError())
						anOk = false;
				}
				if (anOk && ExtractLoadingThreadResources(mResourceManager) && ExtractSoundsResources(mResourceManager) &&
					(!is3d || Extract3dImagesResources(mResourceManager))) {
					mCompletedLoadingThreadTasks += 2;
					MakeShrunkenGems();
					mCompletedLoadingThreadTasks++;
					SDL_Log("Loading completed: %d out of %d", mCompletedLoadingThreadTasks, mNumLoadingThreadTasks);
					return;
				}
			}
		}
	}
	ShowResourceError(false);
	mLoadingFailed = true;
}

// 0x5ddc91: the resources that a reload makes again
void WinDMApp::DeleteReloadResources()
{
	// port: the original waits for its worker thread (WaitForTask) here; the port has none
	delete mBombifiedStillStrip;
	mBombifiedStillStrip = nullptr;
	if (mShrunkenGems != nullptr) {
		for (int aColor = 0; aColor < 7; aColor++)
			for (int aFrame = 0; aFrame < 6; aFrame++)
				delete mShrunkenGems[aColor][aFrame];
		mShrunkenGems = nullptr;   // (the arrays themselves are the original's leak)
	}
	if (!mInitialMenuCleared)
		mResourceManager->DeleteResources("InitialMainMenu");
	mResourceManager->DeleteResources("Init");
	mResourceManager->DeleteResources("LoadingThread");
	mResourceManager->DeleteResources("3dImages");
}

// 0x5dddd9: the resource groups of a reload (the original's reload thread)
void WinDMApp::ReloadThreadProc()
{
	DeleteReloadResources();
	const char* aGroups[4] = {"Init", "LoadingThread", "3dImages", "InitialMainMenu"};
	m3dImagesLoaded = false;
	for (int i = 0; i < 4; i++) {
		if (i == 2 && !Is3DAccelerated())
			continue;
		if (i == 3 && mInitialMenuCleared)
			continue;
		mResourceManager->StartLoadResources(aGroups[i]);
		while (mResourceManager->LoadNextResource())
			mCompletedLoadingThreadTasks++;
		bool anOk = !mResourceManager->HadError();
		if (anOk && i == 0)
			anOk = ExtractInitResources(mResourceManager);
		if (anOk && i == 1)
			anOk = ExtractLoadingThreadResources(mResourceManager);
		if (anOk && i == 2)
			anOk = Extract3dImagesResources(mResourceManager);
		if (anOk && i == 3)
			anOk = ExtractInitialMainMenuResources(mResourceManager);
		if (!anOk) {
			ShowResourceError(false);
			mLoadingFailed = true;
			return;
		}
		if (i == 2)
			m3dImagesLoaded = true;
	}
	MakeShrunkenGems();
}

// 0x5e0479: the display changes (hi-res or not, 3D or not) and the pictures are loaded again at the new size
void WinDMApp::DoReloading(bool wantWindowed, bool is3d)
{
	if (!mReloading) {
		if (mBoard == nullptr) {
			mWasInGame = false;
		} else {
			mBoard->mNoMoveYet = false;
			mWasInGame = mBoard->CanPlay();
			KillBoard();
		}
	}
	mReloading = false;
	if (!mLoadingThreadCompleted) {
		// port: the original waits for its loading thread here; the port's runs on the main thread
		LoadingThreadProcStub(this);
		LoadingCompleted();
	}
	mCompletedLoadingThreadTasks = 0;
	mNumLoadingThreadTasks = mResourceManager->GetNumResources("Init") + 6 + mResourceManager->GetNumResources("LoadingThread");
	if (Is3DAccelerated())
		mNumLoadingThreadTasks += mResourceManager->GetNumResources("3dImages");
	if (!mInitialMenuCleared)
		mNumLoadingThreadTasks += mResourceManager->GetNumResources("InitialMainMenu");
	mYieldMainThread = true;
	while (!mDialogMap.empty())
		KillDialog(mDialogMap.begin()->first);
	KillTitleScreen();
	KillMainMenu();
	KillHighScoreWidget();
	if (mReloadDisplay != nullptr) {
		SafeDeleteWidget(mReloadDisplay);
		mWidgetManager->RemoveWidget(mReloadDisplay);
		mReloadDisplay = nullptr;
	}
	if (!mHighRes) {
		mWidth = 640;
		mHeight = 480;
	} else {
		mWidth = 1024;
		mHeight = 768;
	}
	mWidgetManager->mWidth = mWidth;
	mWidgetManager->mHeight = mHeight;
	mIsWindowed = !wantWindowed;
	SwitchScreenMode(wantWindowed, is3d);
	mReloadDisplay = new ReloadDisplay(this);
	mReloadDisplay->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mReloadDisplay);
	mYieldMainThread = false;
	mReloading = true;   // port: the reload runs in ReloadDisplay::Update, once the display has been drawn
}

// 0x5e0777: the reload is done
void WinDMApp::DoneReloading()
{
	SafeDeleteWidget(mReloadDisplay);
	mWidgetManager->RemoveWidget(mReloadDisplay);
	mReloadDisplay = nullptr;
	if (!mWasInGame) {
		DoMainMenu();
		if (mProfile->mName.empty() && !mProfile->GetAnyProfile())
			mMainMenu->mUserDialogTimer = 50;
		if (!mInitialMenuCleared) {
			mFancyBackdrop = new FancyBackdrop(this);
			mFancyBackdrop->Resize(0, 0, mWidth, mHeight);
			mWidgetManager->AddWidget(mFancyBackdrop);
			mWidgetManager->BringToBack(mFancyBackdrop);
		} else {
			delete mMainMenu->mBackdropImage;
			mMainMenu->mBackdropImage = static_cast<MemoryImage*>(static_cast<CustomResourceManager*>(mResourceManager)->GetCachedImage(
				StrFormat("images\\backdrops\\%s", mBackdrops[0].c_str())));
			mMainMenu->mBackdropClear = false;
		}
	} else {
		DoNewGame();
		if (!mBoard->LoadGame())
			mBoard->mDropDelay = 20;
		mBoard->InitialBackdropProcessing();
		mBoard->mStartCountdown = 0;
		mBoard->mUpdateCnt = 60000;
	}
	// port: the original shows the "no more plays" dialog here when the trial has run out
}

// 0x5dbe1f: the 3D test found no acceleration: the game runs at 640x480 in 16 bits
void WinDMApp::Done3dTesting()
{
	if (!Is3DAccelerated() && mHighRes) {
		mWidth = 640;
		mHeight = 480;
		mFullscreenBits = 16;
		mHighRes = false;
		if (mWantWindowed && mWidth < mScreenWidth && mHeight < mScreenHeight)
			mIsWindowed = true;
	}
}

// 0x5e0b03: the display changes between windowed and full screen: a hi-res game that the screen cannot hold windowed
// reloads at 640x480, and the other way round
void WinDMApp::SwitchScreenMode(bool wantWindowed)
{
	if (!mForceFullscreen) {
		if (!wantWindowed) {
			if (!mHighRes && Is3DAccelerated()) {
				mHighRes = true;
				DoReloading(false, true);
				return;
			}
		} else if (mHighRes && mScreenWidth <= 1024) {
			mHighRes = false;
			DoReloading(true, Is3DAccelerated());
			return;
		}
	}
	SexyApp::SwitchScreenMode(wantWindowed);
}

// port: a picture that CustomResourceManager loads for the 640 by 480 game comes at five eighths of its size, as the copy
// that the original keeps of it in the user's folder is (CustomResourceManager::DoLoadImage, 0x5b4a69)
DDImage* WinDMApp::GetImage(const std::string& theFileName, bool commitBits)
{
	if (mShrinkLoads == 0)
		return SexyApp::GetImage(theFileName, commitBits);
	DDImage* anImage = SexyApp::GetImage(theFileName, false);
	if (anImage != nullptr)
		CustomResourceManager::ShrinkImage(anImage, mShrinkFill, commitBits);
	return anImage;
}

// 0x5e0b6f
void WinDMApp::PreDisplayHook()
{
	mWantWindowed = mIsWindowed;
	if (!mHighRes) {
		mWidth = 640;
		mHeight = 480;
	} else {
		mWidth = 1024;
		mHeight = 768;
	}
	mIsDiamondMine = GetBoolean("IsDiamondMine", false);
	if (mIsDiamondMine)
		mTitle = GetString("Title", "Diamond Mine 2 Deluxe") + " " + mProductVersion;
}

// 0x5de7bb
void WinDMApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	mProfile->WriteProfileList();
	RegistryWriteString("LastScoreName", mLastScoreName);
	if (mLoaded)
		RegistryWriteString("LastProfileName", mProfile->mName);
	RegistryWriteBoolean("HighResolution", mHighRes);
	RegistryWriteBoolean("AlwaysUpdate", mAlwaysUpdate);
	RegistryWriteInteger("FullscreenBits", mFullscreenBits);
	RegistryWriteInteger("GameMode", mGameMode);
	for (int i = 0; i < 4; i++)
		RegistryWriteInteger(StrFormat("ModeSeconds%d", i), mModeSeconds[i]);
}

// 0x5de9ba
void WinDMApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	RegistryReadString("LastScoreName", &mLastScoreName);
	RegistryReadBoolean("HighResolution", &mHighRes);
	RegistryReadBoolean("AlwaysUpdate", &mAlwaysUpdate);
	RegistryReadInteger("FullscreenBits", &mFullscreenBits);
	RegistryReadInteger("GameMode", &mGameMode);
	if (mGameMode < 0 || mGameMode > 4)
		mGameMode = 0;
	for (int i = 0; i < 4; i++)
		RegistryReadInteger(StrFormat("ModeSeconds%d", i), &mModeSeconds[i]);
}

// 0x5ddf3a
void WinDMApp::Shutdown()
{
	if (mShutdown)
		return;
	// port: the original waits for the reload thread here, and adds the minutes of this run to the trial's counter
	mReloading = false;
	if (mBoard != nullptr && mBoard->CanPlay()) {
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
		mBoard = nullptr;
	}
	mProfile->WriteProfile();
	if (mReloadDisplay != nullptr) {
		mWidgetManager->RemoveWidget(mReloadDisplay);
		delete mReloadDisplay;
		mReloadDisplay = nullptr;
	}
	DeleteReloadResources();
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
		mBoard = nullptr;
	}
	while (!mButtonList.empty()) {
		mWidgetManager->RemoveWidget(mButtonList.front());
		delete mButtonList.front();
		mButtonList.pop_front();
	}
	if (mHighScoreWidget != nullptr) {
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		delete mHighScoreWidget;
		mHighScoreWidget = nullptr;
	}
	if (mFancyBackdrop != nullptr) {
		mWidgetManager->RemoveWidget(mFancyBackdrop);
		delete mFancyBackdrop;
		mFancyBackdrop = nullptr;
	}
	if (mMainMenu != nullptr) {
		mWidgetManager->RemoveWidget(mMainMenu);
		delete mMainMenu;
		mMainMenu = nullptr;
	}
	if (mTitleScreen != nullptr) {
		mWidgetManager->RemoveWidget(mTitleScreen);
		delete mTitleScreen;
		mTitleScreen = nullptr;
	}
	SexyApp::Shutdown();
}

// 0x5e13f3
void WinDMApp::UpdateFrames()
{
	// port: the original's 640 by 480 game (no 3D) draws into a software screen, where a fast stretch steps its source
	// position before it reads and a rotation is the game's own bilinear code; the framework's software paths do the same
	// (as in Twist's 2D game), where the GPU's would sample at other places and show the big text half a pixel off. The 3D game
	// keeps the GPU's, as the original's Direct3D does. The software screen is also one surface that keeps the last frame: the
	// 2D whirlpool opens over the menu's last picture, which neither the menu nor the board draws once it has started.
	if (mDDInterface != NULL)
	{
		mDDInterface->mSoftwareStretch = !Is3DAccelerated();
		mDDInterface->mSoftwareTransforms = !Is3DAccelerated();
		mDDInterface->mKeepScreen = !Is3DAccelerated();
	}
	SexyApp::UpdateFrames();
	if (mTraceMode)
		TraceUpdate();
	if (mTestGoto != -1)
		TestAdvanceGoto();
	if (mReloadPending) {
		DoReloading(mIsWindowed, Is3DAccelerated());
		mReloadPending = false;
	}
	if (mPendingMusicOffset != -1) {
		// the title music has faded out (the song's volume is 0): the main menu's music starts after a short wait
		SDLMusicInterface* aMusic = dynamic_cast<SDLMusicInterface*>(mMusicInterface);
		if (aMusic != nullptr) {
			auto aSong = aMusic->mSongs.find(0);
			if (aSong != aMusic->mSongs.end() && aSong->second.mVolume == 0.0) {
				if (mPendingMusicDelay < 1) {
					mMusicInterface->PlayMusic(0, mPendingMusicOffset, false);
					mPendingMusicOffset = -1;
				} else {
					mPendingMusicDelay--;
				}
			}
		}
	}
	// port: the original shows the announcement of a new version here (DoAnnounceDialog)
}

// 0x5dbe86
void WinDMApp::PlaySample(int theSoundNum)
{
	PlaySample(theSoundNum, 0);
}

// 0x5dbe9a: the sounds of a game in progress change pitch as the game allows
void WinDMApp::PlaySample(int theSoundNum, int thePan)
{
	if (mSoundManager == nullptr)
		return;
	SoundInstance* anInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (anInstance == nullptr)
		return;
	if (mBoard != nullptr) {
		double aPitch;
		bool aChange = true;
		if (mBoard->mIsTwilight)
			aPitch = -2.0;
		else if (mBoard->mIsHyper)
			aPitch = (theSoundNum == SOUND_WHIRLPOOL) ? 12.0 : 1.0;
		else
			aChange = false;
		if (aChange)
			anInstance->AdjustPitch(aPitch);
	}
	if (thePan != 0)
		anInstance->SetPan(thePan);
	anInstance->Play(false, true);
}

// 0x5dd6e2: a dialog opens over a game in progress: the game pauses
void WinDMApp::ModalOpen()
{
	if (mBoard != nullptr) {
		bool aFlag = false;
		if (mBoard->mIsAction) {
			for (const auto& aDialog : mDialogMap)
				if (aDialog.second->mId < 0x25)
					aFlag = true;
		}
		mBoard->Pause(aFlag);
	}
}

// 0x5dc069
void WinDMApp::ModalClose()
{
	if (mBoard != nullptr)
		mBoard->Unpause(mBoard->mIsAction);
}

// 0x5dc086
void WinDMApp::GotFocus()
{
	if (mBoard != nullptr)
		mBoard->Unpause(mBoard->mIsAction);
}

// 0x5dc0a3
void WinDMApp::LostFocus()
{
	if (mBoard != nullptr)
		mBoard->Pause(mBoard->mIsAction);
}

// 0x5dc0c0
void WinDMApp::DoHighScoreWidget()
{
	PlaySample(SOUND_PANEL_SLIDE);
	mHighScoreWidget = new HighScoreWidget(this);
	mHighScoreWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHighScoreWidget);
}

// 0x5dc140
void WinDMApp::KillHighScoreWidget()
{
	if (mHighScoreWidget != nullptr) {
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		SafeDeleteWidget(mHighScoreWidget);
		mHighScoreWidget = nullptr;
	}
}
