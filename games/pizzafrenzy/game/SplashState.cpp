#include "SplashState.h"
#include "EditBox.h"
#include "GameMusicPlayer.h"
#include "GameScreen.h"
#include "GameStats.h"
#include "LevelManifest.h"
#include "MenuState.h"
#include "PizzaFrenzy.h"
#include "Scene.h"
#include "Players.h"
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
// The three confirmations (quit, leave the game, delete a player) slide up from below instead
const float CONFIRMATION_SLIDE_FROM_Y = 600.0f;

// The restore confirmation's fade
const float RESTORE_FADE_SECONDS = 0.1f;

// The tile manifest's file
const char* const TILE_MANIFEST_FILE = "res\\manifests\\tileManifest.xml";

// The main menu's fade, and the high scores'
const float MENU_FADE_SECONDS = 0.5f;
const float HIGH_SCORES_FADE_SECONDS = 0.3f;

// The unlock notice slides in from the right
const float UNLOCK_NOTICE_SLIDE_FROM_X = 800.0f;

// The level manifest's file, and the city select's fade
const char* const LEVEL_MANIFEST_FILE = "res\\manifests\\levelManifestSpeed.xml";
const float CITY_SELECT_FADE_SECONDS = 0.3f;

// The game screen's fade, and the splat factory's file
const float GAME_SCREEN_FADE_SECONDS = 0.5f;
const char* const SPLATS_FILE = "res\\manifests\\splats.xml";

// The story screen's fade
const float STORY_SCREEN_FADE_SECONDS = 0.4f;

// The topping screens' fade
const float TOPPING_SCREEN_FADE_SECONDS = 0.5f;

// The load bar's share after each stage
const float LOADED_TILE_MANIFEST = 0.05f;
const float LOADED_MAIN_MENU = 0.1f;
const float LOADED_OPTIONS = 0.15f;
const float LOADED_PAUSE_MENU = 0.2f;
const float LOADED_CONFIRMATIONS = 0.25f;
const float LOADED_RESTORE_CONFIRMATION = 0.3f;
const float LOADED_GAME_STATS = 0.4f;
const float LOADED_STORY_SCREEN = 0.55f;
const float LOADED_UNLOCK_NOTICE = 0.6f;
const float LOADED_USER_SELECT = 0.65f;
const float LOADED_NEW_USER_ENTRY = 0.7f;
const float LOADED_HIGH_SCORES = 0.75f;
const float LOADED_MUSIC_PLAYER = 0.9f;
const float LOADED_TOPPING_SCREENS = 0.85f;
const float LOADED_LEVEL_MANIFEST = 0.95f;
const float LOADED_CITY_SELECT = 1.0f;
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
	return makeSlide(0, DIALOG_SLIDE_FROM_Y);
}

engine::Transition* SplashState::makeConfirmationSlide()
{
	return makeSlide(0, CONFIRMATION_SLIDE_FROM_Y);
}

engine::Transition* SplashState::makeSlide(float theFromX, float theFromY)
{
	engine::SlideTransition* aSlide = new engine::SlideTransition(DIALOG_SLIDE_SECONDS);
	aSlide->setPoints(theFromX, theFromY, 0, 0);
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
	else if (mApp->mMainMenu == nullptr)
	{
		engine::Ref<engine::Transition> aFadeIn = new engine::FadeTransition(MENU_FADE_SECONDS);
		engine::Ref<engine::Transition> aFadeOut = new engine::FadeTransition(MENU_FADE_SECONDS);
		mApp->mMainMenu = new MainMenu();
		mApp->mMainMenu->loadLayout("res/screenLayouts/mainMenu.xml");
		mApp->mMainMenu->setup();
		mApp->mMainMenu->setTransitions(aFadeIn, aFadeOut);
		mApp->mMainMenu->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		aLoaded = LOADED_MAIN_MENU;
	}
	else if (mApp->mOptions == nullptr)
	{
		engine::Ref<engine::Transition> aSlide = makeDialogSlide();
		mApp->mOptions = new OptionScreen();
		mApp->mOptions->loadLayout("res/screenLayouts/optionMenu.xml");
		mApp->mOptions->setupOptions(mApp, mApp->mSettings);
		mApp->mOptions->setTransitions(aSlide, aSlide);
		mApp->mOptions->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		aLoaded = LOADED_OPTIONS;
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
		engine::Ref<engine::Transition> aSlide = makeConfirmationSlide();
		mApp->mAppQuitConfirm = new engine::GenericScreen();
		setUpScreen(mApp->mAppQuitConfirm, "res/screenLayouts/appQuitConfirm.xml", aSlide, aSlide);
		aSlide = makeConfirmationSlide();
		mApp->mGameExitConfirm = new engine::GenericScreen();
		setUpScreen(mApp->mGameExitConfirm, "res/screenLayouts/gameExitConfirm.xml", aSlide, aSlide);
		aSlide = makeConfirmationSlide();
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
	else if (mApp->mGameScreen == nullptr)
	{
		// the game screen with its fade, the stats, the city's map and the hud (given the stats), the link between
		// them, and the splat factory, which the scene holds
		engine::Ref<engine::Transition> aFade = new engine::FadeTransition(GAME_SCREEN_FADE_SECONDS);
		mApp->mGameScreen = new GameScreen();
		mApp->mGameScreen->setTransitions(aFade, aFade);
		mApp->mGameScreen->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		mApp->mGameStats = new GameStats();
		mApp->mCityMap = new CityMap();
		mApp->mHud = new Hud();
		mApp->mHud->setup(mApp->mGameStats);
		mApp->mHud->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		mApp->mGameScreen->link(mApp->mHud, mApp->mCityMap);
		mApp->mSplatFactory = new engine::SplatFactory();
		mApp->mSplatFactory->load(SPLATS_FILE);
		mApp->mScene->addChild(mApp->mSplatFactory);
		aLoaded = LOADED_GAME_STATS;
	}
	// The original makes these stages here, which the port does not have yet: the screens of the pizza designer (the
	// decorating screen at +0xf4 and the designer at +0xe8, each with a fade of 0.5; the load bar at 0.43), the pizza
	// manifest and the player's pizzas (0.44, which show no load bar), and the pizza editor and name screens (+0x100
	// with a fade of 0.3, +0x104 sliding as the options do, and the editor's object at +0xfc; 0.45)
	else if (mApp->mStoryScreen == nullptr)
	{
		// the story screen (which is also the level's end) with its layout, its nodes, the game's stats and its
		// transitions, which come last in this stage
		engine::Ref<engine::Transition> aFade = new engine::FadeTransition(STORY_SCREEN_FADE_SECONDS);
		mApp->mStoryScreen = new StoryScreen();
		mApp->mStoryScreen->loadLayout("res/screenLayouts/storyScreen.xml");
		mApp->mStoryScreen->setup();
		mApp->mStoryScreen->setGameStats(mApp->mGameStats);
		mApp->mStoryScreen->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		mApp->mStoryScreen->setTransitions(aFade, aFade);
		aLoaded = LOADED_STORY_SCREEN;
	}
	else if (mApp->mUnlockNotice == nullptr)
	{
		engine::Ref<engine::Transition> aSlide = makeSlide(UNLOCK_NOTICE_SLIDE_FROM_X, 0);
		mApp->mUnlockNotice = new engine::GenericScreen();
		mApp->mUnlockNotice->loadLayout("res/screenLayouts/unlockPizzaDesignerNotify.xml");
		mApp->mUnlockNotice->setTransitions(aSlide, aSlide);
		mApp->mUnlockNotice->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		engine::TextItem* aNotice = static_cast<engine::TextItem*>(mApp->mUnlockNotice->getNode("notifyText"));
		if (aNotice != nullptr)
			mApp->mUnlockNoticeTemplate = aNotice->mText;
		aLoaded = LOADED_UNLOCK_NOTICE;
	}
	else if (mApp->mUserSelect == nullptr)
	{
		engine::Ref<engine::Transition> aSlide = makeDialogSlide();
		mApp->mUserSelect = new engine::GenericUserSelectScreen();
		mApp->mUserSelect->loadLayout("res/screenLayouts/userSelect.xml");
		mApp->mUserSelect->setup();
		mApp->mUserSelect->setTransitions(aSlide, aSlide);
		mApp->mUserSelect->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		aLoaded = LOADED_USER_SELECT;
	}
	else if (mApp->mNewUserEntry == nullptr)
	{
		engine::Ref<engine::Transition> aSlide = makeDialogSlide();
		mApp->mNewUserEntry = new engine::GenericScreen();
		setUpScreen(mApp->mNewUserEntry, "res/screenLayouts/newUserEntry.xml", aSlide, aSlide);
		static_cast<engine::EditBox*>(mApp->mNewUserEntry->getNode("nameEntry"))
			->setCharFilter(engine::filterPlayerNameChar);
		aLoaded = LOADED_NEW_USER_ENTRY;
	}
	else if (mApp->mHighScores == nullptr)
	{
		// the high scores screen takes its own actions (no connection to the application: it passes on the ones that
		// are not its own); its reset confirmation is made with it
		engine::Ref<engine::Transition> aFade = new engine::FadeTransition(HIGH_SCORES_FADE_SECONDS);
		mApp->mHighScores = new HighScoreScreen();
		mApp->mHighScores->loadLayout("res/screenLayouts/highScores.xml");
		mApp->mHighScores->setTransitions(aFade, aFade);
		mApp->mHighScores->init();
		engine::Ref<engine::Transition> aSlide = makeDialogSlide();
		mApp->mHighScoreResetConfirm = new engine::GenericScreen();
		mApp->mHighScoreResetConfirm->loadLayout("res/screenLayouts/highScoreResetConfirm.xml");
		mApp->mHighScoreResetConfirm->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		mApp->mHighScoreResetConfirm->setTransitions(aSlide, aSlide);
		aLoaded = LOADED_HIGH_SCORES;
	}
	// The original makes the global score post's screen here (+0xd8, sliding in from the right; the load bar at 0.8),
	// which the port does not have: it has no online scores
	else if (mApp->mNewToppingScreen == nullptr)
	{
		// the topping screens: the original makes the topping book (+0xe0, from 0x445b20, layout toppingBook.xml,
		// sliding in from the left) and the level start screen (+0xe4, from 0x448860, layout
		// toppingSelectionScreen.xml, a fade of 0.5) before the new topping screen in this one stage, each with its
		// slot 0x17c; they are translated with the level start, and the new topping screen is the one made so far
		engine::Ref<engine::Transition> aFade = new engine::FadeTransition(TOPPING_SCREEN_FADE_SECONDS);
		mApp->mNewToppingScreen = new NewToppingScreen();
		mApp->mNewToppingScreen->loadLayout("res/screenLayouts/newToppingScreen.xml");
		mApp->mNewToppingScreen->setTransitions(aFade, aFade);
		mApp->mNewToppingScreen->setup();
		aLoaded = LOADED_TOPPING_SCREENS;
	}
	else if (mApp->mMusicPlayer == nullptr)
	{
		mApp->mMusicPlayer = new GameMusicPlayer();
		mApp->setMusicEnabled(mApp->mSettings->isMusicEnabled());
		aLoaded = LOADED_MUSIC_PLAYER;
	}
	else if (mApp->mCities.empty())
	{
		loadLevelManifest(LEVEL_MANIFEST_FILE);
		aLoaded = LOADED_LEVEL_MANIFEST;
	}
	else if (mApp->mCitySelect == nullptr)
	{
		engine::Ref<engine::Transition> aFade = new engine::FadeTransition(CITY_SELECT_FADE_SECONDS);
		mApp->mCitySelect = new CitySelectScreen();
		mApp->mCitySelect->loadLayout("res/screenLayouts/citySelectScreen.xml");
		mApp->mCitySelect->setTransitions(aFade, aFade);
		mApp->mCitySelect->setup(mApp->mCities);
		mApp->mCitySelect->mActionSignal.connect(mApp, &PizzaFrenzy::onScreenAction);
		aLoaded = LOADED_CITY_SELECT;
	}
	else
	{
		// The stages not translated yet come between these
		mApp->mStates.changeState(new MenuState(mApp));
		return;
	}
	engine::Rect aClip;
	aClip.set(0, 0, (float)(int)((float)mApp->mLoadBarWidth * aLoaded), (float)mApp->mLoadBarHeight);
	mApp->mLoadBar->setClipRect(aClip);
}
