#include "SplashState.h"
#include "GameMusicPlayer.h"
#include "PizzaFrenzy.h"
#include "TileManifest.h"
#include "UserGameData.h"
#include "TextItem.h"
#include "Transition.h"

using namespace pizzafrenzy;

namespace
{
const float SPLASH_SECONDS = 1.0f;

// The dialogs' slide (0x475160 and 0x475070 in each stage)
const float DIALOG_SLIDE_SECONDS = 0.2f;
const float DIALOG_SLIDE_FROM_Y = -600.0f;

// The restore confirmation's fade
const float RESTORE_FADE_SECONDS = 0.1f;

// The tile manifest's file
const char* const TILE_MANIFEST_FILE = "res\\manifests\\tileManifest.xml";

// The load bar's share after each stage
const float LOADED_TILE_MANIFEST = 0.05f;
const float LOADED_PAUSE_MENU = 0.2f;
const float LOADED_CONFIRMATIONS = 0.25f;
const float LOADED_RESTORE_CONFIRMATION = 0.3f;
const float LOADED_MUSIC_PLAYER = 0.9f;
}

// 0x402400
void SplashState::enter()
{
	mDelay = SPLASH_SECONDS;
	mApp->mScreens->showScreen(mApp->mSplashScreen, 0);
}

void SplashState::setUpScreen(engine::GenericScreen* theScreen, const char* theLayout, engine::Transition* theIn,
                              engine::Transition* theOut)
{
	theScreen->loadLayout(theLayout);
	theScreen->setTransitions(theIn, theOut);
	theScreen->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
}

engine::Transition* SplashState::makeDialogSlide()
{
	engine::SlideTransition* aSlide = new engine::SlideTransition(DIALOG_SLIDE_SECONDS);
	aSlide->setPoints(0, DIALOG_SLIDE_FROM_Y, 0, 0);
	return aSlide;
}

// 0x40a330: the screens a stage makes take their transitions before anything else is made; the load bar shows the
// share the stage leaves (and the one before's on a frame that makes nothing)
void SplashState::update(engine::UpdateInfo& theInfo)
{
	mDelay = mDelay - theInfo.mSeconds;
	if (mApp->mScreens->isBusy() || !(mDelay < 0.0f))
		return;
	float aLoaded = 0;
	if (mApp->mTileManifest == nullptr)
	{
		// the manifest, read from its file, and the player's data, which reads the current player (the application's
		// +0x124 and +0x128)
		mApp->mTileManifest = new TileManifest();
		mApp->mTileManifest->load(TILE_MANIFEST_FILE);
		mApp->mUserGameData = new UserGameData();
		mApp->mUserGameData->init();
		aLoaded = LOADED_TILE_MANIFEST;
	}
	else if (mApp->mPauseMenu == nullptr)
	{
		engine::Ref<engine::Transition> aSlide = makeDialogSlide();
		mApp->mPauseMenu = new engine::GenericScreen();
		setUpScreen(mApp->mPauseMenu, "res/screenLayouts/pauseMenu.xml", aSlide, aSlide);
		aLoaded = LOADED_PAUSE_MENU;
	}
	else if (mApp->mAppQuitConfirm == nullptr)
	{
		engine::Ref<engine::Transition> aSlide = makeDialogSlide();
		mApp->mAppQuitConfirm = new engine::GenericScreen();
		setUpScreen(mApp->mAppQuitConfirm, "res/screenLayouts/appQuitConfirm.xml", aSlide, aSlide);
		aSlide = makeDialogSlide();
		mApp->mGameExitConfirm = new engine::GenericScreen();
		setUpScreen(mApp->mGameExitConfirm, "res/screenLayouts/gameExitConfirm.xml", aSlide, aSlide);
		aSlide = makeDialogSlide();
		mApp->mDeleteUserConfirm = new engine::GenericScreen();
		setUpScreen(mApp->mDeleteUserConfirm, "res/screenLayouts/deleteUserConfirm.xml", aSlide, aSlide);
		mApp->mDeleteUserText = static_cast<engine::TextItem*>(mApp->mDeleteUserConfirm->getNode("confirmString"));
		mApp->mDeleteUserTemplate = mApp->mDeleteUserText->mText;
		aLoaded = LOADED_CONFIRMATIONS;
	}
	else if (mApp->mRestoreGameConfirm == nullptr)
	{
		engine::Ref<engine::Transition> aFade = new engine::FadeTransition(RESTORE_FADE_SECONDS);
		mApp->mRestoreGameConfirm = new engine::GenericScreen();
		setUpScreen(mApp->mRestoreGameConfirm, "res/screenLayouts/restoreGameConfirm.xml", aFade, aFade);
		aLoaded = LOADED_RESTORE_CONFIRMATION;
	}
	else if (mApp->mMusicPlayer == nullptr)
	{
		mApp->mMusicPlayer = new GameMusicPlayer();
		aLoaded = LOADED_MUSIC_PLAYER;
	}
	else
	{
		// the stages not translated yet, and the next state
		return;
	}
	engine::Rect aClip;
	aClip.set(0, 0, (float)(int)((float)mApp->mLoadBarWidth * aLoaded), (float)mApp->mLoadBarHeight);
	mApp->mLoadBar->setClipRect(aClip);
}
