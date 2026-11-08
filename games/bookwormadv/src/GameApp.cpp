#include "GameApp.h"
#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

#include <SDL.h>

#include "Board.h"
#include "BookManager.h"
#include "ChapterLoadScreen.h"
#include "ConversationPanel.h"
#include "Credits.h"
#include "CurtainTransition.h"
#include "CutScene.h"
#include "FadeButton.h"
#include "FlashWidget.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Lex.h"
#include "LuaInterface.h"
#include "LuaResourceManager.h"
#include "MainMenu.h"
#include "MinigameMenu.h"
#include "MusicInterface.h"
#include "PopAnimManager.h"
#include "Profile.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SDLMusicInterface.h"
#include "SongPlayer.h"
#include "SoundPlayer.h"
#include "StatsBook.h"
#include "TitleScreen.h"
#include "TreasureScreen.h"
#include "WidgetManager.h"
#include "lauxlib.h"
#include "platform/VFS.h"

using namespace Sexy;

GameApp* Sexy::gApp = nullptr;

// The game's resource manager (vtable 0x7b4dbc): a LuaResourceManager, its two overrides compiled copies of the
// base's (the destructor and ResourceLoadedHook)
class CustomResManager : public LuaResourceManager {
public:
	using LuaResourceManager::LuaResourceManager;
};

GameApp::GameApp()
{
	gApp = this;
	mProdName = "WinBAD";
	mProductVersion = "1.0";
	mRegKey = "PopCap\\WinBAD";
	// port: the original's title was the properties' Title (InitPropertiesHook) and the version
	mTitle = "Bookworm Adventures Deluxe";
	mWidth = 800;
	mHeight = 600;
	mAutoStartLoadingThread = false;  // the title screen starts it once the curtain is closed
	mPopAnimManager = new PopAnimManager();
	mSoundPlayer = new JeffLib::SoundPlayer(this);
}

GameApp::~GameApp()
{
	if (mMainMenu != nullptr) {
		mWidgetManager->RemoveWidget(mMainMenu);
		delete mMainMenu;
	}
	if (mConversationPanel != nullptr) {
		mWidgetManager->RemoveWidget(mConversationPanel);
		delete mConversationPanel;
	}
	delete mSongPlayer;
	delete mSoundPlayer;
	for (Image* anImage : mCurtainCopies)
		delete anImage;
	delete mLex;
	delete mPopAnimManager;
	delete mProfile;
	gApp = nullptr;
}

// The development features (Vista's moved save folder, the debug keys, cheats) are left out
void GameApp::Init()
{
	mLuaInterface = new LuaInterface();
	mLua = mLuaInterface;
	RegisterClasses();
	SexyApp::Init();
	if (mShutdown)
		return;
	RegistryWriteString("LastRanVersion", mProductVersion);
	mLuaInterface->RegisterSexyFunctions();
	RegisterGameAppMethods();

	delete mResourceManager;
	mResourceManager = new CustomResManager(this, mLua);

	LoadProperties("properties/config.xml", true, false);
	LoadResourceManifest();
	LoadGameTexts();
	mNumLoadingThreadTasks = mResourceManager->GetNumResources("CommonGame") + 1;
	if (mMusicInterface != nullptr) {
		mSongPlayer = new JeffLib::SongPlayer(mMusicInterface);
		mSongPlayer->LoadSong("sounds/music.mo3", 12.0);
		// the media player's spectrum: 320 x 240. port: cleared (the original's is not until a song plays)
		mSongPlayer->mSpectrumWidth = 320;
		mSongPlayer->mSpectrumHeight = 240;
		delete[] mSongPlayer->mSpectrum;
		mSongPlayer->mSpectrum = new unsigned char[320 * 240]();
	}

	if (!mResourceManager->LoadResources("TitleAndMM"))
		ShowResourceError(true);
	if (!ExtractTitleAndMMResources(mResourceManager))
		ShowResourceError(true);
	if (!mResourceManager->LoadResources("Init"))
		ShowResourceError(true);
	if (!ExtractInitResources(mResourceManager))
		ShowResourceError(true);
	if (mShutdown)
		return;
	SetCursorImage(CURSOR_POINTER, IMAGE_CUSTOM_POINTER);
	SetCursorImage(CURSOR_HAND, IMAGE_CUSTOM_HAND);
	SetCursorImage(CURSOR_DRAGGING, IMAGE_CUSTOM_DRAGGING);
	SetCursorImage(CURSOR_TEXT, IMAGE_CUSTOM_TEXT);

	mTitleScreen = new TitleScreen(this);
	if (!mTitleScreen->InitLoadingScreen()) {
		// the original's message (GT 0x7acbc8) asked the player to reinstall the game
		Popup(GT(0x7acbc8));
		Shutdown();
		return;
	}
	mTitleScreen->Resize(0, 0, 800, 600);
	mWidgetManager->AddWidget(mTitleScreen);
	mTitleScreen->MarkDirty();

	mLuaInterface->DoFile("scripts\\main.lua");
	lua_pushboolean(mLua->L, 1);
	mLua->Call("main.SetDisableCheats", 1, 0);
	if (mShutdown)
		return;

	// main.LoadRequiredScripts: the number of loading tasks it adds, the number of books, then the resource
	// groups to load (and .xml resource files to read)
	lua_State* L = mLua->L;
	int aTop = lua_gettop(L);
	if (!mLua->Call("main.LoadRequiredScripts", 0, LUA_MULTRET))
		return;
	int aNumResults = lua_gettop(L) - aTop;
	int aTasks = aNumResults >= 1 ? (int)lua_tonumber(L, aTop + 1) : 0;
	mTotalBooks = aNumResults >= 2 ? (int)lua_tonumber(L, aTop + 2) : 0;
	if (!RegistryReadInteger("TotalBooks", &mRegTotalBooks)) {
		mRegTotalBooks = mTotalBooks;
		RegistryWriteInteger("TotalBooks", mTotalBooks);
	}
	mNumLoadingThreadTasks += aTasks;
	for (int i = 3; i <= aNumResults; i++) {
		const char* aName = lua_tostring(L, aTop + i);
		std::string aString = aName != nullptr ? aName : "";
		if (aString.find(".xml") == std::string::npos) {
			if (!aString.empty()) {
				mStartupGroups.push_back(aString);
				mNumLoadingThreadTasks += mResourceManager->GetNumResources(aString);
			}
		} else if (!mResourceManager->ParseResourcesFile(aString)) {
			ShowResourceError(true);
			return;
		}
	}
	lua_settop(L, aTop);
	// gTotalLoadCount (0x823b14) = mNumLoadingThreadTasks - 1: read by nothing the port has yet

	CreateLinkedObject(this);
}

void GameApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-chapterloadupdates") {
		mChapterLoadUpdates.clear();
		size_t aPos = 0;
		while (aPos <= theParamValue.size()) {
			size_t aComma = theParamValue.find(',', aPos);
			if (aComma == std::string::npos)
				aComma = theParamValue.size();
			if (aComma > aPos)
				mChapterLoadUpdates.push_back(atoi(theParamValue.substr(aPos, aComma - aPos).c_str()));
			aPos = aComma + 1;
		}
	} else if (theParamName == "-boardlog")
		mBoardLogForTests = theParamValue;  // port: tests with real input (TestHooks.cpp)
	else if (theParamName == "-startloadupdates")
		mStartLoadWanted = theParamValue.empty() ? -1 : atoi(theParamValue.c_str());
	else
		LuaApp::HandleCmdLineParam(theParamName, theParamValue);
}

bool GameApp::UpdateAppStep(bool* updated)
{
	// A chapter's resources load between the frames while its loading screen is up (StartLoadChapterResources);
	// with -fast all of them at once
	if (mChapterLoadScreen != nullptr && !mChapterLoadDone) {
		if (mChapterLoadWanted >= 0) {
			// -chapterloadupdates: the load spread evenly over the original's measured updates, so that the bar fills
			// as its loading thread fills it (at that thread's own pace, which no update count can give exactly)
			int anElapsed = mUpdateCount - mChapterLoadStart + 1;
			int aLimit = anElapsed >= mChapterLoadWanted ? -1 : mChapterLoadTotal * anElapsed / mChapterLoadWanted;
			mChapterLoadDone = LoadChapterResources(0, aLimit) && (mChapterLoadFailed || anElapsed > mChapterLoadWanted);
		}
		else
			mChapterLoadDone = LoadChapterResources(mFastMode ? 0 : SDL_GetTicks() + 20);
	}
	// The loading thread's next 20 ms of work, between the frames (StartLoading); with -fast all of it at once,
	// so that a test run reaches the menu at the same update on any machine
	if (mLoadingPhase != LOADING_IDLE && mLoadingPhase != LOADING_DONE && mDrawCount > 0) {
		bool aDone;
		if (mStartLoadWanted >= 0) {
			int anElapsed = mUpdateCount - mStartLoadStart + 1;
			int aLimit = anElapsed >= mStartLoadWanted ? -1 : mNumLoadingThreadTasks * anElapsed / mStartLoadWanted;
			aDone = LoadingStep(0, aLimit);  // under the limit the last task (the Lex) never runs
		} else
			aDone = LoadingStep(mFastMode ? 0 : SDL_GetTicks() + 20);
		if (aDone) {
			mLoadingThreadRunning = false;
			mLoadingThreadCompleted = true;  // DoUpdateFrames calls LoadingThreadCompleted
		}
	}
	return LuaApp::UpdateAppStep(updated);
}

void GameApp::UpdateFrames()
{
	// (SexyApp's: not LuaApp's, whose UpdateState the original called only with a development flag set)
	SexyApp::UpdateFrames();
	// (the original's music interface did this in its Update, the end of SexyAppBase::UpdateFrames)
	if (mSongPlayer != nullptr)
		mSongPlayer->Update();
	if (!mBoardLogForTests.empty())
		LogBoardForTests();
	if (!mRetriesForTests.empty())
		RunRetriesForTests();
	// port: every 100 updates the original checked the trial's limits of its marketing modes (an hour's play,
	// or the Hydra); the port is the full game
	if (mCountsTime)
		mUserUpdates++;
	mSoundPlayer->Update();
}

void GameApp::Shutdown()
{
	if (mShutdown)
		return;
	if (mProfile != nullptr)
		mProfile->Save();
	// the battle's board goes
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = nullptr;
	}
	// port: the original, in the trial's marketing modes, also asked its trial window whether to show the "buy"
	// screen here
	LuaApp::Shutdown();
}

namespace {

// the screen stops updating (the focus handlers' flag), or goes on
void StopUpdating(Widget* theWidget, bool isStopped)
{
	if (isStopped)
		theWidget->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_UPDATE;
	else
		theWidget->mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_UPDATE;
}

}  // namespace

void GameApp::GotFocus()
{
	// 0x451230: the screens update again, unless the credits are up (they stopped the ones under them)
	mCountsTime = true;
	if (mCredits != nullptr)
		StopUpdating(mCredits, false);
	if (mBookScreen != nullptr) {
		StatsBook* aBook = static_cast<StatsBook*>(mBookScreen);
		if (aBook->mMediaPlayer && aBook->mFlash != nullptr)
			aBook->mFlash->Unpause();
	}
	// port: the original also went on with the trial's "buy" screen (the app's +0x8a4) here, and in two of the
	// trial's marketing modes asked its trial window whether to leave the game; the port is the full game
	if (mCreditsCount == 0) {
		if (mBoard != nullptr)
			StopUpdating(mBoard, false);
		if (mBookManager != nullptr)
			StopUpdating(mBookManager, false);
		if (mTreasureScreen != nullptr)
			StopUpdating(mTreasureScreen, false);
		if (mMainMenu != nullptr)
			StopUpdating(mMainMenu, false);
		if (mHallOfFame != nullptr)
			StopUpdating(mHallOfFame, false);
		if (mMiniGameMenu != nullptr)
			StopUpdating(mMiniGameMenu, false);
	}
}

void GameApp::LostFocus()
{
	// 0x451160: the screens stop updating, unless the credits are up (they stopped the ones under them)
	mCountsTime = false;
	if (mCredits != nullptr)
		StopUpdating(mCredits, true);
	if (mMainMenu != nullptr)
		mMainMenu->AppLostFocus();
	if (mBookScreen != nullptr) {
		StatsBook* aBook = static_cast<StatsBook*>(mBookScreen);
		if (aBook->mMediaPlayer && aBook->mFlash != nullptr)
			aBook->mFlash->Pause();
	}
	// port: the original also stopped the trial's "buy" screen (the app's +0x8a4) here
	if (mCreditsCount == 0) {
		if (mBoard != nullptr)
			StopUpdating(mBoard, true);
		if (mBookManager != nullptr)
			StopUpdating(mBookManager, true);
		if (mTreasureScreen != nullptr)
			StopUpdating(mTreasureScreen, true);
		if (mMainMenu != nullptr && mMainMenu->mLogoY == 0)
			StopUpdating(mMainMenu, true);
		if (mHallOfFame != nullptr)
			StopUpdating(mHallOfFame, true);
		if (mMiniGameMenu != nullptr)
			StopUpdating(mMiniGameMenu, true);
	}
}

void GameApp::PlaySample(int theSoundNum)
{
	mSoundPlayer->PlaySampleMinTime(theSoundNum, 5);
}

void GameApp::PlaySample(int theSoundNum, int thePan)
{
	mSoundPlayer->PlaySamplePan(theSoundNum, thePan);
}

Dialog* GameApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
                           const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	GameDialog* aDialog = new GameDialog(theDialogId, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->Layout(348);
	return aDialog;
}

Dialog* GameApp::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
                          const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog =
		LuaApp::DoDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	if (theDialogId == 20025 || theDialogId == 10025) {
		aDialog->SetColor(Dialog::COLOR_HEADER, Color::White);
		aDialog->mContentInsets = Insets(42, 42, 42, 42);
		aDialog->mSpaceAfterHeader = 25;
		aDialog->Resize(aDialog->mX, aDialog->mY, aDialog->mWidth, aDialog->mHeight + 50);
	}
	return aDialog;
}

GameDialog* GameApp::DoConfirmDialog(int theId, const SexyString& theHeader, const SexyString& theLines,
                                     ButtonListener* theListener, const std::string& theCallback)
{
	if (theId == -1)
		theId = DIALOG_CONFIRM;
	// the battle holds under it (with its sounds, and the looping samples go silent)
	if (mBoard != nullptr) {
		mBoard->Pause(true, true);
		Unmute(true);
		mSoundPlayer->PauseLooping(true);
	}
	mDialogCallback = theCallback;
	mConfirmListener = theListener;
	GameDialog* aDialog = static_cast<GameDialog*>(DoDialog(theId, true, theHeader, theLines, _S(""), Dialog::BUTTONS_YES_NO));
	Unmute(true);
	mSoundPlayer->PauseLooping(true);
	aDialog->Resize(mWidth / 2 - aDialog->mWidth / 2, mHeight / 2 - aDialog->mHeight / 2, aDialog->mWidth,
	                aDialog->mHeight + 50);
	aDialog->SetHeaderFont(FONT_LARGE_OUTLINE);
	aDialog->SetLinesFont(FONT_MEDIUM);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color::White);
	aDialog->SetBanner(IMAGE_DIALOG_BANNER_RUSURE, 25);
	aDialog->mDialogHeader = _S(" ");  // room for the banner
	aDialog->mContentInsets.mTop -= 14;
	aDialog->mSpaceAfterHeader = 25;
	return aDialog;
}

GameDialog* GameApp::DoOkDialog(const SexyString& theHeader, const SexyString& theLines, const std::string& theCallback,
                                Image* theBanner)
{
	if (mBoard != nullptr) {
		mBoard->Pause(true, true);
		Unmute(true);
		mSoundPlayer->PauseLooping(true);
	}
	mDialogCallback = theCallback;
	GameDialog* aDialog = static_cast<GameDialog*>(
		DoDialog(DIALOG_OK, true, theHeader, theLines, GT(0x7ac5a0), Dialog::BUTTONS_FOOTER));
	Unmute(true);
	mSoundPlayer->PauseLooping(true);
	aDialog->SetHeaderFont(FONT_LARGE_OUTLINE);
	aDialog->SetLinesFont(FONT_MEDIUM);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color::White);
	aDialog->mContentInsets.mTop -= 14;
	aDialog->mSpaceAfterHeader = 25;
	if (theBanner != nullptr) {
		aDialog->mDialogHeader = _S(" ");
		aDialog->SetBanner(theBanner, 17);
	}
	int aHeight = aDialog->mHeight;
	if (aHeight > 525)
		aHeight = 525;
	aDialog->Resize(aDialog->mX, aDialog->mY, aDialog->mWidth, aHeight + 50);
	mWidgetManager->SetFocus(aDialog);
	return aDialog;
}

void GameApp::CallDialogCallback(bool theAnswer)
{
	if (mDialogCallback.empty())
		return;
	lua_State* L = mLua->L;
	int aTop = lua_gettop(L);
	lua_pushboolean(L, theAnswer);
	mLua->Call(mDialogCallback, 1, 1);
	lua_settop(L, aTop);
	mDialogCallback.clear();
}

void GameApp::ButtonPress(int theId)
{
	if (theId == 1 || theId == 11)
		PlaySample(SOUND_CLICK2);
	else
		PlaySample(SOUND_CLICK1);
}

void GameApp::ButtonDepress(int theId)
{
	// nothing while the curtain moves
	if (IsCurtainMoving())
		return;
	int anId = theId % 10000;
	if (anId >= 2000 && anId < 3000) {
		int aDialogId = theId - 2000;
		// 0x44adae: the battle's board takes the focus back first
		if (mBoard != nullptr)
			mWidgetManager->SetFocus(mBoard);
		// the pauses of the looping samples end: two for the ok and the question (both pause once more over a
		// battle), one for the options and the menu
		if (aDialogId == DIALOG_CONFIRM || aDialogId == DIALOG_OPTIONS || aDialogId == DIALOG_OK ||
		    aDialogId == DIALOG_MENU) {
			mSoundPlayer->PauseLooping(false);
			if (aDialogId == DIALOG_CONFIRM || aDialogId == DIALOG_OK)
				mSoundPlayer->PauseLooping(false);
		}
		switch (aDialogId) {
		case DIALOG_REALLY_QUIT:  // its yes is "Quit"
			mMainMenu->mReallyQuit = false;
			Shutdown();
			break;
		case DIALOG_CONFIRM:
			if (mBoard != nullptr)
				mBoard->Pause(false, true);
			CallDialogCallback(true);
			KillDialog(DIALOG_CONFIRM);
			if (mConfirmListener != nullptr)
				mConfirmListener->ButtonDepress(1018);
			break;
		case DIALOG_OK:
			if (mBoard != nullptr)
				mBoard->Pause(false, true);
			KillDialog(DIALOG_OK);
			CallDialogCallback(true);
			break;
		case DIALOG_OPTIONS:  // the options' footer button, or Escape
			CloseOptionsDialog(true);
			if (mBoard != nullptr)
				mWidgetManager->SetFocus(mBoard);
			else if (mBookManager != nullptr)
				mWidgetManager->SetFocus(mBookManager);
			break;
		case DIALOG_NEW_USER:  // the dialog hears its buttons itself (NewUserDialog::ButtonDepress)
			break;
		case DIALOG_NO_WINDOWED:  // the options' message
			KillDialog(DIALOG_NO_WINDOWED);
			break;
		case DIALOG_CONTINUE_GAME:  // the arena's saved run
			mMainMenu->ContinueArena(true);
			KillDialog(DIALOG_CONTINUE_GAME);
			break;
		case DIALOG_SKIP_MOVIE:
			if (mCutScene != nullptr)
				mCutScene->Skip();
			KillDialog(DIALOG_SKIP_MOVIE);
			break;
		case DIALOG_ARENA_WIN:  // its Continue (the original has no case of its own, but kills the dialog)
			KillDialog(DIALOG_ARENA_WIN);
			break;
		default:
			SDL_Log("GameApp: the answer %d to dialog %d is not ported yet", theId, aDialogId);
			KillDialog(aDialogId);
			break;
		}
	} else if (anId >= 3000 && anId < 4000) {
		int aDialogId = theId - 3000;
		// 0x44b20e: the battle's board takes the focus back first
		if (mBoard != nullptr)
			mWidgetManager->SetFocus(mBoard);
		if (aDialogId == DIALOG_CONFIRM || aDialogId == DIALOG_OPTIONS || aDialogId == DIALOG_OK ||
		    aDialogId == DIALOG_MENU) {
			mSoundPlayer->PauseLooping(false);
			if (aDialogId == DIALOG_CONFIRM || aDialogId == DIALOG_OK)
				mSoundPlayer->PauseLooping(false);
		} else if (aDialogId == DIALOG_REALLY_QUIT)
			mMainMenu->mReallyQuit = false;
		else if (aDialogId == DIALOG_SKIP_MOVIE) {
			if (mCutScene != nullptr)
				mCutScene->mFlash->Unpause();
		} else if (aDialogId == DIALOG_CONTINUE_GAME) {
			// a new run (the original killed the dialog here and again below)
			mProfile->ResetTimedMode();
			StartNewGameTransition();
		} else if (aDialogId != DIALOG_CONFIRM && aDialogId != DIALOG_OK)
			SDL_Log("GameApp: the answer %d to dialog %d is not ported yet", theId, aDialogId);
		KillDialog(aDialogId);
		if (aDialogId == DIALOG_CONFIRM) {
			if (mBoard != nullptr)
				mBoard->Pause(false, true);
			CallDialogCallback(false);
			if (mConfirmListener != nullptr)
				mConfirmListener->ButtonDepress(3018);
		}
		if (mBoard != nullptr)
			mWidgetManager->SetFocus(mBoard);
		else if (mBookManager != nullptr)
			mWidgetManager->SetFocus(mBookManager);
	} else {
		switch (theId) {
		case BookManager::BUTTON_BACK:
			if (mBookManager != nullptr && mBookManager->mState == BookManager::STATE_NORMAL)
				StartToMenuTransition();
			break;
		case BookManager::BUTTON_CONTINUE:
			if (mBookManager->mState == BookManager::STATE_NORMAL &&
			    mBookManager->IsContinueEnabled() && mBookManager->CallLinkedMethod("CanClickContinue", 0).Bool()) {
				// the tutorial's arrows hear a click at the corner (the whole-screen hot spot) and go
				if (mArrowHintManager != nullptr) {
					mArrowHintManager->Click(0, 0, 1);
					mArrowHintManager->DeleteAllArrowHints();
				}
				mBookManager->CallLinkedMethod("ContinueButtonPressed", 0);
				mBookManager->DisableContinueButton();
			}
			break;
		case BookManager::BUTTON_OPTIONS:
		case TreasureScreen::BUTTON_OPTIONS:
			ShowGameMenu(theId == TreasureScreen::BUTTON_OPTIONS);
			break;
		case TreasureScreen::BUTTON_BACK:
			if (mTimedMode)
				StartToMenuTransition();
			else
				mTreasureScreen->CallLinkedMethod("SlideOut", 0);
			break;
		case TreasureScreen::BUTTON_CONTINUE:
			mTreasureScreen->mPreventRightClick = true;
			StartTreasureTransition();
			break;
		default:
			SDL_Log("GameApp: button %d is not ported yet", theId);
			break;
		}
	}
	if (mTitleScreenDoneQueued && GetDialogCount() == 0) {
		mTitleScreenDoneQueued = false;
		TitleScreenDone();
	}
}

void GameApp::StartLoading()
{
	if (mLoadingThreadStarted)
		return;
	mLoadingThreadStarted = true;
	// the framework runs LoadingThreadProc in one go while this is false
	mLoadingThreadRunning = true;
	mLoadingPhase = LOADING_COMMON;
	mStartLoadStart = mUpdateCount;
	mLoadingGroup = 0;
	mLoadingGroupStarted = false;
}

// The original made a JeffLib::SexyBassMusicInterface. Its SetVolume (0x59a8c0) is the framework's
// BassMusicInterface::SetVolume, which on BASS 2.2 and later sets BASS_CONFIG_GVOL_MUSIC and _STREAM to
// volume * 100 and uses the 40 of mMaxMusicVolume only on older BASS: the game's BASS 2.3 plays the music at
// the full volume setting.
MusicInterface* GameApp::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new MusicInterface;
	SDLMusicInterface* aMusicInterface = new SDLMusicInterface();
	aMusicInterface->mMaxMusicVolume = 100;
	return aMusicInterface;
}

void GameApp::LoadingThreadProc()
{
	if (mLoadingPhase == LOADING_IDLE)
		mLoadingPhase = LOADING_COMMON;
	LoadingStep(0);
}

void GameApp::LoadingFailed()
{
	ShowResourceError(false);
	mLoadingFailed = true;
	Shutdown();
	mLoadingPhase = LOADING_DONE;
}

bool GameApp::LoadingStep(unsigned int theDeadline, int theTaskLimit)
{
	auto aTimeLeft = [theDeadline]() { return theDeadline == 0 || !SDL_TICKS_PASSED(SDL_GetTicks(), theDeadline); };
	while (mLoadingPhase != LOADING_DONE) {
		if (mShutdown) {
			mLoadingPhase = LOADING_DONE;
			break;
		}
		if (!aTimeLeft())
			return false;
		if (theTaskLimit >= 0 && mCompletedLoadingThreadTasks >= theTaskLimit)
			return false;
		switch (mLoadingPhase) {
		case LOADING_COMMON:
		case LOADING_GROUPS: {
			if (!mLoadingGroupStarted) {
				mResourceManager->StartLoadResources(mLoadingPhase == LOADING_COMMON ? std::string("CommonGame")
				                                                                     : mStartupGroups[mLoadingGroup]);
				mLoadingGroupStarted = true;
			}
			if (mResourceManager->LoadNextResource()) {
				mCompletedLoadingThreadTasks++;
				if (mTitleScreen != nullptr)
					mTitleScreen->MarkDirty();
				break;
			}
			mLoadingGroupStarted = false;
			if (mLoadingPhase == LOADING_COMMON) {
				if (mResourceManager->HadError() || !ExtractCommonGameResources(mResourceManager)) {
					LoadingFailed();
					break;
				}
				mLoadingPhase = LOADING_GROUPS;
				mLoadingGroup = 0;
			} else {
				if (mResourceManager->HadError()) {
					LoadingFailed();
					break;
				}
				mLoadingGroup++;
			}
			if (mLoadingPhase == LOADING_GROUPS && mLoadingGroup >= mStartupGroups.size())
				mLoadingPhase = LOADING_LEX;
			break;
		}
		case LOADING_LEX:
			mLex = new Lex(false);
			if (!mLex->Load("data/defs.bin", "data/compressed.txt")) {
				// the original: "ERROR: " and the product name, then it quit at once
				Popup("Could not read the word list (data/compressed.txt).");
				mLoadingFailed = true;
				Shutdown();
				mLoadingPhase = LOADING_DONE;
				break;
			}
			SDL_Log("Lex: %d words", (int)mLex->GetNumWords());
			mCompletedLoadingThreadTasks++;
			if (mTitleScreen != nullptr)
				mTitleScreen->MarkDirty();
			mLoadingPhase = LOADING_DONE;
			break;
		default:
			mLoadingPhase = LOADING_DONE;
			break;
		}
	}
	return true;
}

void GameApp::LoadingThreadCompleted()
{
	mLoadingDone = true;
	if (mLoadingChapter) {
		mLoadingChapter = false;
		return;
	}
	((LuaResourceManager*)mResourceManager)->ExportToLua();
	mLua->Call("main.LoadInitialResources", 0, 0);
	lua_pushboolean(mLua->L, 0);
	mLua->Call("main.SetBetaBuild", 1, 0);
	if (mLoadingFailed || mShutdown)
		return;
	mTitleScreen->MarkDirty();
	mTitleScreen->mLoaded = true;
	// the profiles that do not load are deleted, and the player is told
	if (!CheckProfiles() && !mProfileErrors.empty()) {
		DoOkDialog(GT(0x7a9c04), StrFormat(GT(0x7acdc0).c_str(), mProfileErrors.c_str()), "", nullptr);
		// wide enough for what failed, and put back in the middle
		GameDialog* aDialog = static_cast<GameDialog*>(GetDialog(DIALOG_OK));
		aDialog->mWidth = 600;
		aDialog->Move((mWidth - 600) / 2, (mHeight - aDialog->mHeight) / 2);
	}
	// the last player's profile, or the first there is
	std::string aName;
	if (!RegistryReadString("LastUser", &aName) || !Platform::IsFile(GetAppDataFolder() + "users/" + aName + ".bwa")) {
		aName.clear();
		for (const std::string& aFile : Platform::ListDirectory(GetAppDataFolder() + "users")) {
			if (aFile.size() > 4 && StringToLower(aFile.substr(aFile.size() - 4)) == ".bwa") {
				aName = aFile.substr(0, aFile.size() - 4);
				break;
			}
		}
	}
	if (!aName.empty())
		SelectUser(aName);
	mConversationPanel = new ConversationPanel(this);
	mWidgetManager->AddWidget(mConversationPanel);
	mConversationPanel->Resize(0, 0, mWidth, mHeight);
	mConversationPanel->SetDisabled(true);
	mConversationPanel->SetVisible(false);
	// the tutorial's arrows (0x40b2c0), over the whole screen and linked to a Lua object of their own
	mArrowHintManager = new ArrowHintManager(this);
	mWidgetManager->AddWidget(mArrowHintManager);
	mArrowHintManager->Resize(0, 0, mWidth, mHeight);
	mArrowHintManager->RegisterObject();
	// port: not yet: the "GAME LAUNCHED" statistic. The update check is left out.
	RegistryWriteInteger("TotalBooks", mTotalBooks);
}

void GameApp::TitleScreenDone()
{
	if (GetDialogCount() > 0) {
		mTitleScreenDoneQueued = true;
		return;
	}
	mCountsTime = true;
	if (HasPlayedGameBefore()) {
		mTitleToMainMenu = true;
		mWidgetManager->RemoveWidget(mTitleScreen);
		SafeDeleteWidget(mTitleScreen);
		mTitleScreen = nullptr;
		ShowMainMenu(true, false);
		mWidgetManager->BringToFront(mCurtain);
		mTransitionState = TRANSITION_OPENING;
		mCurtain->StartOpening();
		return;
	}
	StartIntro(0);
}

bool GameApp::HasPlayedGameBefore()
{
	if (mProfile == nullptr)
		return false;
	if (mTimedMode)
		return true;
	return !(mProfile->GetBool("NeedsIntroTileHelp") && mProfile->GetInt("CurrentChapter") < 2 &&
	         mProfile->GetInt("CurrentStage") < 2 && mProfile->GetInt("CurrentBook") < 2);
}

bool GameApp::CheckProfiles()
{
	mProfileErrors.clear();
	std::string aLastUser;
	bool aHasLastUser = RegistryReadString("LastUser", &aLastUser);
	bool aLastUserFound = false;
	bool anAllLoaded = true;
	std::string aUsers = GetAppDataFolder() + "users";
	for (const std::string& aFile : Platform::ListDirectory(aUsers)) {
		if (aFile.size() <= 4 || StringToLower(aFile.substr(aFile.size() - 4)) != ".bwa")
			continue;
		std::string aName = aFile.substr(0, aFile.size() - 4);
		if (aHasLastUser && StringToLower(aName) == StringToLower(aLastUser))
			aLastUserFound = true;
		Profile aProfile(aName, true);
		if (aProfile.Load())
			continue;
		// "Profile for <name>: <why>", and its files go
		anAllLoaded = false;
		mProfileErrors += StrFormat(GT(0x7acd44).c_str(), aName.c_str(), aProfile.mOutOfDateReasons.c_str());
		if (aHasLastUser && StringToLower(aName) == StringToLower(aLastUser))
			RegistryEraseValue("LastUser");
		EraseFile(aUsers + "/" + aFile);
		EraseFile(GetAppDataFolder() + "users/data/" + aName + ".dat");
	}
	if (aHasLastUser && !aLastUserFound)
		RegistryEraseValue("LastUser");
	return anAllLoaded;
}

bool GameApp::SelectUser(const std::string& theName)
{
	delete mProfile;
	mProfile = new Profile(theName, false);
	if (!mProfile->Load()) {
		std::string aReasons = mProfile->mOutOfDateReasons;
		delete mProfile;
		mProfile = nullptr;
		RegistryEraseValue("LastUser");
		// a profile that is out of date says why in a message box (the app's vf 0x78, MsgBox), any other that does not
		// load in the error dialog
		if (aReasons.empty())
			DoOkDialog(GT(0x7a9c04), StrFormat(GT(0x7acec8).c_str(), theName.c_str()), "", nullptr);
		else
			MsgBox(StrFormat(GT(0x7acf08).c_str(), aReasons.c_str()), GT(0x7acef4), 0);
		return false;
	}
	RegistryWriteString("LastUser", mProfile->mName);
#ifdef __EMSCRIPTEN__
	SiteProfilesChanged(this);	// the web page's players (framework/WebProfiles.h): a player was chosen
#endif
	mUserUpdates = 0;
	mBattleUpdates = 0;
	mMiniGameUpdates = 0;
	return true;
}

void GameApp::ShowMainMenu(bool theFromTitle, bool theToBack)
{
	bool aNew = mMainMenu == nullptr;
	if (aNew) {
		mMainMenu = new MainMenu(this, theFromTitle);
		mWidgetManager->AddWidget(mMainMenu);
		if (theFromTitle)
			mWidgetManager->BringToBack(mMainMenu);
	}
	mWidgetManager->SetFocus(mMainMenu);
	mMainMenu->SetDisabled(false);
	if (aNew)
		ShowUnlockMessages();
	if (theToBack && mHallOfFame != nullptr)
		mWidgetManager->BringToFront(mHallOfFame);
	if (mCurtain != nullptr)
		mWidgetManager->BringToFront(mCurtain);
	if (!mNoMusic && mSongPlayer != nullptr)
		mSongPlayer->PlaySong(0x8c, 0.005f, true);  // the main menu's song
	mWidgetManager->SetFocus(mMainMenu);
	ClearUpdateBacklog(false);
}

void GameApp::ShowUnlockMessages()
{
	// 0x449660
	auto aNeeds = [this](const char* theKey) {
		return mProfile != nullptr && mProfile->KeyExists(theKey) && mProfile->GetBool(theKey);
	};
	bool aBestiary = aNeeds("NeedsBestiaryDialog");
	bool aMiniGames = aNeeds("NeedsMiniGamesDialog");
	bool aArena = aNeeds("NeedsArenaDialog");
	bool aExpansionBoss = aNeeds("NeedsExpansionBossDialog");
	bool aMediaPlayer = aNeeds("NeedsMPDialog");

	// Lex speaks from a new panel, one page after another
	auto aNewPanel = [this]() {
		NewConversationPanel();
		mConversationPanel->SetDefaults(-1, Color(0, 75, 10), IMAGE_TUT_PIC_LEX, FONT_HUGE, 0);
		mConversationPanel->SetContinueTextColor(Color(0, 140, 20));
	};
	// the panel comes over the menu, and the menu's welcome when it is done. (After the end of the game it is made
	// but not shown: the credits show it.)
	auto aShowPanel = [this]() {
		if (!mNoMusic) {
			mConversationPanel->Activate(false);
			mWidgetManager->PutInfront(mConversationPanel, mMainMenu);
			mConversationPanel->mOnInactive = [this]() {
				// 0x44d340
				if (mTitleToMainMenu) {
					mTitleToMainMenu = false;
					mMainMenu->PlayReaction(MainMenu::REACTION_WELCOME);
				}
			};
		}
	};

	if (aBestiary) {
		// the tome, and what else came with it
		aNewPanel();
		mConversationPanel->AddTextPage(GT(0x7ad090));
		mConversationPanel->AddTextPage(GT(0x7ad0e0));
		if (aMiniGames) {
			mConversationPanel->AddTextPage(GT(0x7ad128));
			mProfile->SetBool("NeedsMiniGamesDialog", false);
		}
		if (aArena) {
			mConversationPanel->AddTextPage(aMiniGames ? GT(0x7ad170) : GT(0x7ad1c0));
			mProfile->SetBool("NeedsArenaDialog", false);
		}
		if (aExpansionBoss) {
			mConversationPanel->AddTextPage(!aMiniGames && !aArena ? GT(0x7ad254) : GT(0x7ad210));
			mProfile->SetBool("NeedsExpansionBossDialog", false);
		}
		aShowPanel();
		mProfile->SetBool("NeedsBestiaryDialog", false);
		mProfile->Save();
		return;
	}
	if (aMiniGames) {
		aNewPanel();
		mConversationPanel->AddTextPage(GT(0x7ad290));
		mConversationPanel->AddTextPage(GT(0x7ad2e0));
		if (aArena) {
			mConversationPanel->AddTextPage(GT(0x7ad1c0));
			mProfile->SetBool("NeedsArenaDialog", false);
		}
		if (aExpansionBoss) {
			mConversationPanel->AddTextPage(aArena ? GT(0x7ad210) : GT(0x7ad254));
			mProfile->SetBool("NeedsExpansionBossDialog", false);
		}
		aShowPanel();
		mProfile->SetBool("NeedsMiniGamesDialog", false);
		mProfile->Save();
		return;
	}

	// the arena, the expansion's bosses or the film reel: one of them
	const char* aKey;
	if (aArena) {
		aNewPanel();
		mConversationPanel->AddTextPage(GT(0x7ad308));
		mConversationPanel->AddTextPage(GT(0x7ad360));
		if (aExpansionBoss) {
			mConversationPanel->AddTextPage(GT(0x7ad254));
			mProfile->SetBool("NeedsExpansionBossDialog", false);
		}
		aKey = "NeedsArenaDialog";
	} else if (aExpansionBoss) {
		aNewPanel();
		mConversationPanel->AddTextPage(GT(0x7ad3c0));
		mConversationPanel->AddTextPage(GT(0x7ad3f4));
		aKey = "NeedsExpansionBossDialog";
	} else {
		if (!aMediaPlayer)
			return;
		aNewPanel();
		mConversationPanel->AddTextPage(GT(0x7ad420));
		mConversationPanel->AddTextPage(GT(0x7ad470));
		aKey = "NeedsMPDialog";
	}
	mProfile->SetBool(aKey, false);
	aShowPanel();
	mProfile->Save();
}

void GameApp::StartIntro(int theMode)
{
	// a player with no name yet: a profile that is not saved until the intro asks for the name
	bool aNewPlayer = false;
	if (theMode == 0 && mProfile == nullptr) {
		mProfile = new Profile("", true);
		aNewPlayer = true;
	}
	InitGameScreen(false);
	lua_pushint(mLua->L, 1);
	lua_pushboolean(mLua->L, 1);
	lua_pushboolean(mLua->L, 1);
	mBookManager->CallLinkedMethod("SelectBookNumber", 3);
	HideBookManager();
	if (aNewPlayer)
		mPlayIntro = true;
	if (!mNoMusic && mSongPlayer != nullptr)
		mSongPlayer->PlaySong(0x1b, 0.005f, true);  // the intro's song
	PlayCutScene("images/cutscenes/intro.swf", "gBookManager:StartGame", GT(0x7acfdc), true, false);
	if (mCurtain != nullptr)
		mWidgetManager->BringToFront(mCurtain);
}

bool GameApp::IsCurtainMoving() const
{
	return mCurtain != nullptr && !(mCurtain->mDone && mCurtain->mDrawn);
}

void GameApp::NewConversationPanel()
{
	if (mConversationPanel != nullptr) {
		mWidgetManager->RemoveWidget(mConversationPanel);
		// port: the original deleted it at once, though a script's callback of the panel itself can ask for this
		SafeDeleteWidget(mConversationPanel);
	}
	mConversationPanel = new ConversationPanel(this);
	mWidgetManager->AddWidget(mConversationPanel);
	mConversationPanel->Resize(0, 0, mWidth, mHeight);
}

bool GameApp::ShowingConversationPanel() const
{
	return mConversationPanel != nullptr && mConversationPanel->Active();
}

void GameApp::KeepCurtainOnTop()
{
	if (mCurtain == nullptr)
		return;
	mWidgetManager->BringToFront(mCurtain);
	// port: as the original, one more than its own priority, not the panel's
	if (ShowingConversationPanel() && mCurtain->mPriority < mConversationPanel->mPriority)
		mCurtain->mPriority++;
}

CurtainTransition* GameApp::GetCurtain()
{
	if (mCurtain == nullptr) {
		mCurtain = new CurtainTransition(this);
		mWidgetManager->AddWidget(mCurtain);
		mWidgetManager->BringToFront(mCurtain);
	}
	return mCurtain;
}

// The callback of the curtain (+0x894): called once, then forgotten
bool GameApp::CallCurtainCallback()
{
	void (*aCallback)(void*) = mCurtainCallback;
	void* anObject = mCurtainCallbackObject;
	mCurtainCallback = nullptr;
	mCurtainCallbackObject = nullptr;
	if (aCallback != nullptr)
		aCallback(anObject);
	return !mShutdown;
}

// The scripts' function the curtain was given, with no arguments, then forgotten
bool GameApp::CallCurtainLuaCallback()
{
	if (!mCurtainLuaCallback.empty()) {
		LuaLib::CallLua(mCurtainLuaCallback, 0);
		if (mShutdown)
			return false;
	}
	mCurtainLuaCallback.clear();
	return true;
}

void GameApp::CurtainDone()
{
	bool aClosed = mCurtain->mClosing;
	switch (mTransitionState) {
	case TRANSITION_TITLE_CURTAIN:
		CallCurtainCallback();
		return;

	case TRANSITION_CUTSCENE:
		if (!aClosed) {
			// open over the movie: it plays (after the new player's name, for the intro)
			if (mPlayIntro) {
				// the movie waits for the name (NewUserDialogDone lets it play)
				ShowNewUserDialog(Dialog::BUTTONS_FOOTER);
				return;
			}
			if (!mCutScene->mEnded)
				mCutScene->mFlash->Unpause();
			mCutScene->mWaitingForCurtain = false;
			return;
		}
		// closed: the movie in place of the screen
		if (mCutScene != nullptr) {
			mCutScene->SetVisible(true);
			if (mHallOfFame != nullptr)
				CloseHallOfFame();
		}
		if (!mFullCurtain) {
			if (mTitleScreen != nullptr) {
				mWidgetManager->RemoveWidget(mTitleScreen);
				SafeDeleteWidget(mTitleScreen);
				mTitleScreen = nullptr;
			} else if (mMainMenu != nullptr)
				RemoveMainMenu();
		} else {
			if (!CallCurtainCallback())
				return;
			mTransitionState = TRANSITION_OPENING;
		}
		mCurtain->StartOpening();
		ClearUpdateBacklog(false);
		return;

	case TRANSITION_OPENING:
		if (aClosed) {
			if (!CallCurtainLuaCallback())
				return;
			mCurtain->StartOpening();
			ClearUpdateBacklog(true);
			if (!mKillCutSceneWhenDone && mCutScene != nullptr && mCutScene->mEnded)
				CloseCutScene();
			return;
		}
		mFullCurtain = false;
		if (mTitleToMainMenu && !ShowingConversationPanel()) {
			mTitleToMainMenu = false;
			mMainMenu->PlayReaction(MainMenu::REACTION_WELCOME);  // 0x44d340
		}
		break;

	case TRANSITION_NEW_GAME: {
		if (mShutdown)
			return;
		if (!aClosed)
			break;
		RemoveMainMenu();
		if (mHallOfFame != nullptr)
			CloseHallOfFame();
		int aBook = mTimedMode ? -1 : mProfile->GetInt("CurrentBook");
		InitGameScreen(mTimedMode);
		if (mNukeProfile) {
			mLua->Call("_NukeProfile", 0, 0);
			mNukeProfile = false;
		}
		lua_pushint(mLua->L, aBook);
		lua_pushboolean(mLua->L, 1);
		lua_pushboolean(mLua->L, 1);
		mBookManager->CallLinkedMethod("SelectBookNumber", 3);
		mBookManager->CallLinkedMethod("DoArrowHints", 0);
		mBookManager->CallLinkedMethod("MMToCSTransitionComplete", 0);
		if (mTimedMode) {
			mBookManager->DoTreasureTransition();
			if (mProfile->KeyExists("TM_OnCheckpoint") && mProfile->GetBool("TM_OnCheckpoint")) {
				// a run saved at its checkpoint: the treasure screen as it, over the book
				mBookManager->CallLinkedMethod("InitTimedModeCheckpoint", 0);
				mTreasureScreen->CallLinkedMethod("InitTimedModeCheckpoint", 0);
			} else {
				// a new run: what the arena is, under the arena's banner
				DoOkDialog(_S(""), GT(0x7ad7d8), "", IMAGE_TM_INTRO_TITLE);
				GameDialog* aDialog = static_cast<GameDialog*>(GetDialog(DIALOG_OK));
				aDialog->SetLinesFont(FONT_LARGE);
				aDialog->mSpaceAfterHeader += 0x14;
				aDialog->mPriority = 5;
				int aWidth = aDialog->mWidth;
				aDialog->SetBanner(IMAGE_TM_INTRO_TITLE, 3);
				aWidth += 0x96;
				aDialog->Resize((mWidth - aWidth) / 2, (mHeight - aDialog->mHeight) / 2, aWidth, aDialog->mHeight);
			}
		}
		mCurtain->StartOpening();
		mWidgetManager->BringToFront(mCurtain);
		ClearUpdateBacklog(true);
		return;
	}

	case TRANSITION_SAVED_GAME:
	case TRANSITION_TREASURE:
		if (mShutdown)
			return;
		if (!aClosed)
			break;
		if (mTransitionState == TRANSITION_SAVED_GAME)
			RemoveMainMenu();
		if (!CallCurtainLuaCallback())
			return;
		mCurtain->StartOpening();
		mWidgetManager->BringToFront(mCurtain);
		ClearUpdateBacklog(true);
		HideBookManager();
		return;

	case TRANSITION_TO_MENU - 1:
	case TRANSITION_TO_MENU:
		if (!aClosed)
			break;
		mWidgetManager->RemoveWidget(mBookManager);
		if (mTreasureScreen != nullptr)
			mWidgetManager->RemoveWidget(mTreasureScreen);
		SafeDeleteWidget(mTreasureScreen);
		RemoveBookManager();
		RemoveBoard();
		mTimedMode = false;
		lua_pushboolean(mLua->L, 0);
		mLua->Call("main.SetTimedMode", 1, 0);
		mTreasureScreen = nullptr;
		ShowMainMenu(false, false);
		if (!CallCurtainCallback())
			return;
		mWidgetManager->BringToFront(mCurtain);
		mCurtain->StartOpening();
		ClearUpdateBacklog(true);
		return;

	case TRANSITION_GAME_TO_MENU:
		if (!aClosed)
			break;
		KillDialog(1);
		ReturnToChapterSelect(false, true, true, true);
		mWidgetManager->RemoveWidget(mBookManager);
		if (mTreasureScreen != nullptr)
			mWidgetManager->RemoveWidget(mTreasureScreen);
		SafeDeleteWidget(mTreasureScreen);
		if (mBookScreen != nullptr)
			RemoveStatsBook();
		RemoveBookManager();
		mTreasureScreen = nullptr;
		RemoveBoard();
		if (mTimedMode) {
			mTimedMode = false;
			lua_pushboolean(mLua->L, 0);
			mLua->Call("main.SetTimedMode", 1, 0);
		}
		ShowMainMenu(false, false);
		if (!CallCurtainCallback())
			return;
		mCurtain->StartOpening();
		mWidgetManager->BringToFront(mCurtain);
		ClearUpdateBacklog(true);
		return;

	default:
		break;
	}

	// open (or nothing to do): the scripts' function may keep the curtain; otherwise it goes. A function that returns
	// nothing (the mini-games' ends) keeps nothing: 0x44d360 asks for a boolean only when there is a result
	bool aKeep = false;
	if (!mCurtainLuaCallback.empty()) {
		LuaLib::LuaClassRet aRet = LuaLib::CallLua(mCurtainLuaCallback, 0);
		aKeep = aRet.count > 0 && aRet.Bool();
		if (mShutdown)
			return;
	}
	if (!aKeep) {
		mTransitionState = TRANSITION_NONE;
		mWidgetManager->RemoveWidget(mCurtain);
		SafeDeleteWidget(mCurtain);
		mCurtain = nullptr;
	}
	if (mChapterInitQueued) {
		mChapterInitQueued = false;
		LuaLib::CallLua("gBattleEngine.mPlayerPtr:ChapterInit", 0);
	}
}

void GameApp::DoCurtainTransition(bool theClose, const std::string& theCallback, int thePriority)
{
	mCurtainLuaCallback = theCallback;
	if (thePriority < 0)
		thePriority = mCurtain != nullptr ? mCurtain->mPriority : 0;
	CurtainTransition* aCurtain = GetCurtain();
	if (theClose)
		aCurtain->StartClosing();
	else
		aCurtain->StartOpening();
	aCurtain->mPriority = thePriority;
}

void GameApp::StartNewGameTransition()
{
	mNukeProfile = false;
	mTransitionState = TRANSITION_NEW_GAME;
	if (mMainMenu != nullptr)
		mMainMenu->mFromTitle = false;
	DoCurtainTransition(true, "", -1);
}

void GameApp::StartSavedGameTransition()
{
	mTransitionState = TRANSITION_SAVED_GAME;
	DoCurtainTransition(true, "gBookManager:StartGame", -1);
}

void GameApp::StartToMenuTransition()
{
	if (mArrowHintManager != nullptr)
		mArrowHintManager->DeleteAllArrowHints();
	mTransitionState = TRANSITION_TO_MENU;
	DoCurtainTransition(true, "", -1);
}

void GameApp::StartGameToMenuTransition()
{
	mTransitionState = TRANSITION_GAME_TO_MENU;
	DoCurtainTransition(true, "", -1);
	mCurtain->mPriority = 99999;
}

void GameApp::DoFullCurtainTransition(const std::string& theCallback)
{
	mCurtainLuaCallback = theCallback;
	int aPriority = mCurtain != nullptr ? mCurtain->mPriority : 0;
	mCurtain = GetCurtain();
	mCurtain->StartClosing();
	mTransitionState = TRANSITION_CUTSCENE;
	mCurtain->mPriority = aPriority;
	mFullCurtain = true;
}

void GameApp::StartTreasureTransition()
{
	mTransitionState = TRANSITION_TREASURE;
	DoCurtainTransition(true, "gTreasureScreen:ContinueButtonPressed", -1);
}

// gName = gLinkedObjectTable[cobj]: the scripts' global of a screen's Lua object
static void SetLinkedGlobal(LuaLib::LuaLinkedClass* theObject, const char* theName)
{
	lua_State* L = LuaLib::gLuaSupport->L;
	lua_getglobal(L, "gLinkedObjectTable");
	theObject->PushLinkedObject();
	lua_gettable(L, -2);
	lua_setglobal(L, theName);
	lua_pop(L, 1);
}

void GameApp::InitGameScreen(bool theHidden)
{
	RemoveBookManager();
	if (mTreasureScreen != nullptr) {
		mWidgetManager->RemoveWidget(mTreasureScreen);
		SafeDeleteWidget(mTreasureScreen);
		mTreasureScreen = nullptr;
	}
	mBookManager = new BookManager(this);
	mBookManager->CreateLinkedObject(mBookManager);
	SetLinkedGlobal(mBookManager, "gBookManager");
	if (theHidden)
		mWidgetManager->AddWidget(mBookManager);
	else
		mBookManager->CallLinkedMethod("LoadBookScripts", 0);
	mTreasureScreen = new TreasureScreen(this);
	mTreasureScreen->CreateLinkedObject(mTreasureScreen);
	SetLinkedGlobal(mTreasureScreen, "gTreasureScreen");
	mTreasureScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTreasureScreen);
	if (!theHidden) {
		mTreasureScreen->SetVisible(false);
		mTreasureScreen->SetDisabled(true);
	}
	mBookManager->Resize(0, 0, mWidth, mHeight);
	if (!theHidden)
		mWidgetManager->AddWidget(mBookManager);
	Widget* aFront = theHidden ? (Widget*)mTreasureScreen : (Widget*)mBookManager;
	if (mMainMenu != nullptr)
		mWidgetManager->PutInfront(mMainMenu, aFront);
	mWidgetManager->SetFocus(aFront);
	ClearUpdateBacklog(true);
	if (!theHidden)
		mBookManager->CallLinkedMethod("DoArrowHints", 0);
	else
		HideBookManager();
}

void GameApp::RemoveBookManager()
{
	if (mBookManager == nullptr)
		return;
	mWidgetManager->RemoveWidget(mBookManager);
	mBookManager->CallLinkedMethod("Destructor", 0);
	SafeDeleteWidget(mBookManager);
	mBookManager = nullptr;
}

void GameApp::HideBookManager()
{
	if (mBookManager == nullptr)
		return;
	mBookManager->SetVisible(false);
	mBookManager->SetDisabled(true);
	for (auto& aButton : mBookManager->mButtons) {
		aButton.second->SetVisible(false);
		aButton.second->SetDisabled(true);
	}
}

void GameApp::RemoveMainMenu()
{
	if (mMainMenu == nullptr)
		return;
	mWidgetManager->RemoveWidget(mMainMenu);
	SafeDeleteWidget(mMainMenu);
	mMainMenu = nullptr;
}

void GameApp::ReturnToChapterSelect(bool theLexWon, bool theFromOptions, bool theNoMusic, bool theFromMainMenu)
{
	mProfile->Save();
	if (mArrowHintManager != nullptr)
		mArrowHintManager->DeleteAllArrowHints();  // 0x40beb0
	LuaLib::CallLua("PAMAnimator.CleanUp", 0);
	FreeChapterResources();
	mPopAnimManager->DeleteClones();
	CleanSharedImages();
	RemoveAllTooltips();  // 0x44c1a0
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		Unmute(true);
		Unmute(true);
		SafeDeleteWidget(mBoard);
		mBoard->SetDisabled(true);
		mBoard->SetVisible(false);
		mBoard->mLevelupOverlay = false;
		mBoard->RestoreButtons();
	}
	mBoard = nullptr;
	gBoard = nullptr;
	if (!ShowingConversationPanel())
		mWidgetManager->SetFocus(mBookManager);
	mBookManager->SetVisible(true);
	mBookManager->SetDisabled(false);
	for (int i = BookManager::BUTTON_BACK; i <= BookManager::BUTTON_OPTIONS; i++) {
		FadeButton* aButton = mBookManager->mButtons[i];
		aButton->SetVisible(true);
		aButton->SetDisabled(false);
	}
	lua_State* L = mLua->L;
	lua_pushboolean(L, theLexWon);
	lua_pushboolean(L, theFromOptions);
	lua_pushboolean(L, theNoMusic);
	lua_pushboolean(L, theFromMainMenu);
	bool anEndOfBook = mBookManager->CallLinkedMethod("ReturnToChapterSelect", 4).Int() == 0;
	if (!theFromMainMenu)
		mBookManager->CallLinkedMethod(anEndOfBook ? "DoEndOfBookSequence" : "DoArrowHints", 0);
	if (mCurtain != nullptr)
		mWidgetManager->BringToFront(mCurtain);
}

int GameApp::StartLoadChapterResources(lua_State* L)
{
	if (mLoadingChapter)
		luaL_error(L, "ERROR: StartLoadChapterResources called twice");
	HideBookManager();
	if (mTimedMode)
		HideTreasureScreen();
	bool aContinuing = lua_toboolean(L, 1) != 0;
	mLoadingChapter = true;
	mChapterResources.clear();
	lua_pushnil(L);
	while (lua_next(L, 2) != 0) {
		ChapterResource aResource;
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			lua_pushvalue(L, -2);
			const char* aKeyStr = lua_tostring(L, -1);
			std::string aKey = StringToUpper(aKeyStr != nullptr ? aKeyStr : "");
			lua_pop(L, 1);
			const char* aValue = lua_tostring(L, -1);
			if (aKey == "MTYPE")
				aResource.mType = (int)lua_tonumber(L, -1);
			else if (aKey == "MSTRING")
				aResource.mString = aValue != nullptr ? aValue : "";
			else if (aKey == "MSTRPARAMETER")
				aResource.mStrParameter = aValue != nullptr ? aValue : "";
			else if (aKey == "MINTPARAMETER")
				aResource.mIntParameter = (int)lua_tonumber(L, -1);
			else
				luaL_error(L, "Invalid key passed to StartLoadChapterResources: %s", aKey.c_str());
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		mChapterResources.push_back(aResource);
	}

	// The loading screen over the closed curtain, with a bar of the resources loaded (a group's resources, and
	// one for an animation), while the app updates until they are all loaded
	mChapterLoadTotal = 0;
	for (const ChapterResource& aResource : mChapterResources) {
		if (aResource.mType == CHAPTER_RES_GROUP)
			mChapterLoadTotal += mResourceManager->GetNumResources(aResource.mString);
		else
			mChapterLoadTotal++;
	}
	mChapterLoadCount = 0;
	mChapterLoadIndex = 0;
	mChapterLoadGroupStarted = false;
	mChapterLoadDone = false;
	mChapterLoadFailed = false;
	mChapterLoadError.clear();
	mLoadingDone = false;
#ifdef __EMSCRIPTEN__
	// port: the browser shows nothing until the game returns to it, and the scripts need the resources when
	// this returns, so the web build loads them at once, behind the closed curtain. The draw is the first hint the
	// loading screen picks (natively), so that a seed plays the same in both.
	Rand();
	LoadChapterResources(0);
#else
	mChapterLoadScreen = new ChapterLoadScreen(this);
	mChapterLoadWanted = mChapterLoadNum < mChapterLoadUpdates.size() ? mChapterLoadUpdates[mChapterLoadNum] : -1;
	mChapterLoadNum++;
	mChapterLoadStart = mUpdateCount;
	mWidgetManager->AddWidget(mChapterLoadScreen);
	mWidgetManager->BringToFront(mChapterLoadScreen);
	if (mCurtain != nullptr) {
		mCurtain->mPriority = 99999;
		mChapterLoadScreen->mPriority = mCurtain->mPriority + 1;
	}
	// (with -fast all of them in the first step, so that a test run takes the same updates on any machine)
	while (!mChapterLoadDone && !mShutdown)
		UpdateApp();
	mWidgetManager->RemoveWidget(mChapterLoadScreen);
	SafeDeleteWidget(mChapterLoadScreen);
	mChapterLoadScreen = nullptr;
#endif
	((LuaResourceManager*)mResourceManager)->ExportToLua();
	if (mShutdown) {
		mLoadingChapter = false;
		lua_pushboolean(L, 0);
		return 1;
	}
	if (mChapterLoadFailed)
		luaL_error(L, "There was an error loading the chapter's resources:\n%s", mChapterLoadError.c_str());
	if (!aContinuing)
		StartBattle();
	mLoadingChapter = false;
	lua_pushboolean(L, 1);
	return 1;
}

bool GameApp::LoadChapterResources(unsigned int theDeadline, int theCountLimit)
{
	while (mChapterLoadIndex < mChapterResources.size()) {
		if (mShutdown)
			return true;
		if (theCountLimit >= 0 && mChapterLoadCount >= theCountLimit)
			return false;
		const ChapterResource& aResource = mChapterResources[mChapterLoadIndex];
		if (aResource.mType == CHAPTER_RES_GROUP) {
			if (!mChapterLoadGroupStarted) {
				if (mResourceManager->IsGroupLoaded(aResource.mString)) {
					mChapterLoadIndex++;
					continue;
				}
				mResourceManager->StartLoadResources(aResource.mString);
				mChapterLoadGroupStarted = true;
			}
			while (mResourceManager->LoadNextResource()) {
				mChapterLoadCount++;
				if (mShutdown)
					return true;
				// port: the original gave up after twice the resources it had counted ("taking an unnaturally
				// long time")
				if (mChapterLoadScreen != nullptr)
					mChapterLoadScreen->MarkDirtyFull();
				if (theDeadline != 0 && SDL_TICKS_PASSED(SDL_GetTicks(), theDeadline))
					return false;
				if (theCountLimit >= 0 && mChapterLoadCount >= theCountLimit)
					return false;
			}
			mChapterLoadGroupStarted = false;
			if (mResourceManager->HadError()) {
				mChapterLoadFailed = true;
				mChapterLoadError = mResourceManager->GetErrorText();
				return true;
			}
		} else {
			PopAnim* anAnim = new PopAnim(aResource.mIntParameter, mPopAnimManager);
			if (!anAnim->LoadAnimation(aResource.mString)) {
				mChapterLoadFailed = true;
				mChapterLoadError = StrFormat("Error loading PAM file: \"%s\". Reason given: %s", aResource.mString.c_str(),
				                              anAnim->GetError().c_str());
				delete anAnim;
				return true;
			}
			mPopAnimManager->Add(aResource.mIntParameter, anAnim, aResource.mStrParameter);
			mChapterLoadCount++;
			if (mChapterLoadScreen != nullptr)
				mChapterLoadScreen->MarkDirtyFull();
		}
		mChapterLoadIndex++;
		if (theDeadline != 0 && SDL_TICKS_PASSED(SDL_GetTicks(), theDeadline))
			return false;
	}
	return true;
}

void GameApp::FreeChapterResources()
{
	for (const ChapterResource& aResource : mChapterResources) {
		if (aResource.mType == CHAPTER_RES_GROUP)
			mResourceManager->DeleteResources(aResource.mString);
		else
			mPopAnimManager->Delete(aResource.mIntParameter);
	}
}

void GameApp::HideTreasureScreen()
{
	mTreasureScreen->SetVisible(false);
	mTreasureScreen->SetDisabled(true);
	for (int i = TreasureScreen::BUTTON_BACK; i <= TreasureScreen::BUTTON_OPTIONS; i++) {
		FadeButton* aButton = mTreasureScreen->mButtons[i];
		bool aDisabled = aButton->mDisabled;
		aButton->SetVisible(false);
		if (!aDisabled) {
			aButton->SetDisabled(true);
			if (i == TreasureScreen::BUTTON_CONTINUE)
				aButton->mEffectOff = true;
		}
	}
}

void GameApp::RemoveBoard()
{
	if (mBoard == nullptr)
		return;
	mWidgetManager->RemoveWidget(mBoard);
	SafeDeleteWidget(mBoard);
	mBoard = nullptr;
}

void GameApp::PlayCutScene(const std::string& theFile, const std::string& theCallback, const std::string& theCaption,
                           bool theKillWhenDone, bool theHide)
{
	mKillCutSceneWhenDone = theKillWhenDone;
	mCutSceneFile = theFile;
	mCutSceneCallback = theCallback;
	mCutSceneCaption = theCaption;
	mCutScene = new CutScene(this);
	mWidgetManager->AddWidget(mCutScene);
	mCutScene->Load(theFile, "", theCaption);  // (the callback is the curtain's, once it has closed over the end)
	if (!mCutScene->mEnded)
		mCutScene->mFlash->Pause();
	mCutScene->mWaitingForCurtain = true;
	if (theHide)
		mCutScene->SetVisible(false);
	mTransitionState = TRANSITION_CUTSCENE;
	if (mTitleScreen != nullptr)
		mWidgetManager->BringToFront(mTitleScreen);
	if (mCurtain != nullptr && mCurtain->mDone && mCurtain->mDrawn) {
		// already closed
		mCurtainLuaCallback = theCallback;
		CurtainDone();
	} else
		DoCurtainTransition(true, theCallback, -1);
}

void GameApp::CloseCutScene()
{
	if (mCutScene == nullptr)
		return;
	mWidgetManager->RemoveWidget(mCutScene);
	SafeDeleteWidget(mCutScene);
	mCutScene = nullptr;
}

void GameApp::PlayMainMenuCutScene()
{
	InitGameScreen(false);
	std::string aCaption = mProfile->GetString("IntroCutsceneTitle");
	std::string aFile = mProfile->GetString("NeedsIntroCutscene");
	PlayCutScene(aFile, "gBookManager:MMCutsceneDone", aCaption, false, true);
	if (mHallOfFame != nullptr)
		mWidgetManager->PutBehind(mHallOfFame, mCurtain);
	mProfile->Delete("NeedsIntroCutscene");
	mProfile->Delete("IntroCutsceneTitle");
}

void GameApp::StartBattle()
{
	mBoard = new Board(this, false);
	mBoard->CreateLinkedObject(mBoard);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	if (mCurtain != nullptr) {
		mWidgetManager->BringToFront(mCurtain);
		mCurtain->MarkDirtyFull();
	}
	mBoard->Init();
	mWidgetManager->SetFocus(mBoard);
	// port: the original then freed the images' extra buffers (0x5492a0, and the resource manager's 0x575cd0 for
	// every group); the port's images keep none
}
