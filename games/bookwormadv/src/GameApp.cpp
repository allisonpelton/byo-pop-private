#include "GameApp.h"

#include <SDL.h>

#include "CurtainTransition.h"
#include "GameText.h"
#include "Lex.h"
#include "LuaInterface.h"
#include "LuaResourceManager.h"
#include "MainMenu.h"
#include "MusicInterface.h"
#include "PopAnimManager.h"
#include "Profile.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SongPlayer.h"
#include "SoundPlayer.h"
#include "TitleScreen.h"
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
	delete mSongPlayer;
	delete mSoundPlayer;
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

bool GameApp::UpdateAppStep(bool* updated)
{
	// The loading thread's next 20 ms of work, between the frames (StartLoading)
	if (mLoadingPhase != LOADING_IDLE && mLoadingPhase != LOADING_DONE && mDrawCount > 0) {
		if (LoadingStep(SDL_GetTicks() + 20)) {
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
	// port: every 100 updates the original checked the trial's limits of its marketing modes (an hour's play,
	// or the Hydra); the port is the full game
	if (mTitleScreenDone) {
		mAppTime++;
		mUserUpdates++;
	}
	mSoundPlayer->Update();
}

void GameApp::StartLoading()
{
	if (mLoadingThreadStarted)
		return;
	mLoadingThreadStarted = true;
	// the framework runs LoadingThreadProc in one go while this is false
	mLoadingThreadRunning = true;
	mLoadingPhase = LOADING_COMMON;
	mLoadingGroup = 0;
	mLoadingGroupStarted = false;
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

bool GameApp::LoadingStep(unsigned int theDeadline)
{
	auto aTimeLeft = [theDeadline]() { return theDeadline == 0 || !SDL_TICKS_PASSED(SDL_GetTicks(), theDeadline); };
	while (mLoadingPhase != LOADING_DONE) {
		if (mShutdown) {
			mLoadingPhase = LOADING_DONE;
			break;
		}
		if (!aTimeLeft())
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
	if (mSkipLoadingScripts) {
		mSkipLoadingScripts = false;
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
		// port: the original's dialog (0x443f90) is not ported yet
		Popup(StrFormat(GT(0x7acdc0).c_str(), mProfileErrors.c_str()));
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
	// port: not yet: the "GAME LAUNCHED" statistic, the ConversationPanel (0x42f740) and the ArrowHintManager
	// (0x40b2c0). The update check is left out.
	RegistryWriteInteger("TotalBooks", mTotalBooks);
}

void GameApp::TitleScreenDone()
{
	if (GetDialogCount() > 0) {
		mTitleScreenDoneQueued = true;
		return;
	}
	mTitleScreenDone = true;
	if (!RegistryReadInteger("AppTime", &mAppTime))
		mAppTime = 0;
	if (mAppTime < 0)
		mAppTime = 0;
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
		// port: the original's dialogs (0x443f90 with an error title, the app's vf 0x78) are not ported yet
		if (aReasons.empty())
			Popup(StrFormat(GT(0x7acec8).c_str(), ""));
		else
			Popup(StrFormat(GT(0x7acf08).c_str(), aReasons.c_str()));
		return false;
	}
	RegistryWriteString("LastUser", mProfile->mName);
	mUserUpdates = 0;  // port: the original also cleared +0x974 and +0x978 here (not ported yet)
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
	// port: a new menu then told the player of what was unlocked (0x449660: the profile's NeedsBestiaryDialog,
	// NeedsMiniGamesDialog, NeedsArenaDialog, ...), with dialogs not ported yet
	if (theToBack && mHallOfFame != nullptr)
		mWidgetManager->BringToFront(mHallOfFame);
	if (mCurtain != nullptr)
		mWidgetManager->BringToFront(mCurtain);
	if (!mNoMusic && mSongPlayer != nullptr)
		mSongPlayer->PlaySong(0x8c, 0.005f, true);  // the main menu's song
	mWidgetManager->SetFocus(mMainMenu);
	ClearUpdateBacklog(false);
}

void GameApp::StartIntro(int theMode)
{
	// port: not yet: a new profile (0x487ba0), the intro cut scene (images/cutscenes/intro.swf, a Flash
	// movie), then the scripts' gBookManager:StartGame
	SDL_Log("GameApp::StartIntro: the intro and the first book are not ported yet");
}

bool GameApp::IsCurtainMoving() const
{
	return mCurtain != nullptr && !(mCurtain->mDone && mCurtain->mDrawn);
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

// port: only the title screen's transitions so far (states 9 and 2)
void GameApp::CurtainDone()
{
	if (mTransitionState == TRANSITION_TITLE_CURTAIN) {
		void (*aCallback)(void*) = mCurtainCallback;
		void* anObject = mCurtainCallbackObject;
		mCurtainCallback = nullptr;
		mCurtainCallbackObject = nullptr;
		if (aCallback != nullptr)
			aCallback(anObject);
		return;
	}
	if (mTransitionState == TRANSITION_OPENING) {
		if (mCurtain->mClosing)
			return;  // (closing over a screen the scripts change: not ported yet)
		if (mTitleToMainMenu) {
			mTitleToMainMenu = false;
			mMainMenu->PlayReaction(MainMenu::REACTION_WELCOME);
		}
	}
	mTransitionState = TRANSITION_NONE;
	mWidgetManager->RemoveWidget(mCurtain);
	SafeDeleteWidget(mCurtain);
	mCurtain = nullptr;
}
