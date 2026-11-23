#include "SplashState.h"

#include "DrawContext.h"
#include "EditBox.h"
#include "Engine.h"
#include "FishFactory.h"
#include "MenuState.h"
#include "ScreenMgr.h"
#include "Theme.h"
#include "Transition.h"

using namespace engine;

namespace
{

// The splash screen's fade, in and out (the float at 0x5668b0 (feedingfrenzy2))
const float SPLASH_FADE_SECONDS = 0.25f;
// Where the "LOADING" is centred, and the pixels between its bubbles (the floats at 0x566b4c, 0x566c10 and 0x54dd6c
// (feedingfrenzy2))
const float BUBBLE_TEXT_X = 400.0f;
const float BUBBLE_TEXT_Y = 280.0f;
const float BUBBLE_TEXT_SPACING = 8.0f;
// The shell's screens' fades, in and out, and the restore screen's out (the floats at 0x54ddf8 and 0x566880
// (feedingfrenzy2))
const float SHELL_FADE_SECONDS = 0.2f;
const float RESTORE_FADE_OUT_SECONDS = 0.05f;
// The frames per second while loading and after
const int LOADING_FRAME_RATE = 30;
const int GAME_FRAME_RATE = 60;
// The most the bar moves a frame towards the loader's progress (the double at 0x566d58 (feedingfrenzy2), the float's
// value)
const float LOAD_BAR_STEP = 0.01f;

// 0x4a14e0 (feedingfrenzy2): the name entry's filter, refusing the slash, the backslash, the backquote and the tilde
char nameEntryFilter(char theChar)
{
	if (theChar == '/' || theChar == 0x5c || theChar == '`' || theChar == '~')
	{
		// The shell theme's sound textInvalidInput played at full volume and normal pitch (0x4747a0
		// (feedingfrenzy2)): not yet translated
		getEngine()->log("SplashState: the sound textInvalidInput not yet translated\n");
		return 0;
	}
	return theChar;
}

} // namespace

SplashState::SplashState(FeedingFrenzyTwo* theApp, bool theSaverMode)
	: AppState(theApp), mTimer(0.0f), mLoadBarWidth(0), mLoadBarHeight(0), mPhase(SPLASH_WAITING),
	  mAbortLoading(false), mLoadProgress(0.0f), mShownProgress(0.0f), mSaverMode(false), mNextStateLogged(false)
{
	if (mApp->mSplashScreen == nullptr)
	{
		Engine* anEngine = getEngine();
		anEngine->setUseLooseFiles(true);
		FadeTransition* aFade = new FadeTransition(SPLASH_FADE_SECONDS);
		mApp->mSplashScreen = new GenericScreen();
		mApp->mSplashScreen->load("splash/splashScreen.xml", mApp->mTheme.get());
		mApp->mSplashScreen->setTransitions(aFade, aFade);
		anEngine->setUseLooseFiles(false);
	}
	mSaverMode = theSaverMode;
}

void SplashState::enter()
{
	mTimer = 0.0f;
	mLoadBar = dynamic_cast<Image*>(mApp->mSplashScreen->findComponent("loadBar"));
	mLoadBar->computeBounds();
	mLoadBarHeight = mLoadBar->getHeight();
	mLoadBarWidth = mLoadBar->getWidth();
	mLoadBar->setSource(RectI(0, 0, 0, mLoadBarHeight));

	mApp->mTheme = new Theme();
	mApp->mTheme->load("resource/themes/minimalTheme.xml");
	mApp->mScreenMgr->showScreen(mApp->mSplashScreen.get(), 0.0f);

	mBubbleText = new BubbleText();
	mBubbleText->setPosition(BUBBLE_TEXT_X, BUBBLE_TEXT_Y);
	mBubbleText->setFont(mApp->mTheme->getFont("bubbleTextFont"));
	mBubbleText->setBubble(mApp->mTheme->getImage("bubbleTextBubbleSplash"));
	mBubbleText->setEffect(mApp->mTheme->getParticleDef("bubbleTextFx"));
	mBubbleText->setLetterColor(mApp->mTheme->getColor("splashTextColor"));
	mBubbleText->setSpacing(BUBBLE_TEXT_SPACING);
	mBubbleText->setFilterMode(FILTER_MODE_SMOOTH);
	mBubbleText->setText("LOADING");
	mApp->mSplashScreen->addChild(mBubbleText);
	mBubbleText->appear(0.0f, 1.0f);
}

void SplashState::exit()
{
	mApp->mSplashScreen = nullptr;
}

void SplashState::update(UpdateContext* theContext)
{
	mTimer = (float)((double)mTimer - theContext->mElapsed);
	switch (mPhase)
	{
	case SPLASH_WAITING:
		if (mApp->mScreenMgr->isBusy())
			return;
		if (!(mTimer < 0.0f))
			return;
		getEngine()->setFrameRate(LOADING_FRAME_RATE);
		startLoading();
		return;
	case SPLASH_LOADING:
		if (mLoadBar == nullptr)
			return;
		if (1.0f <= mLoadProgress)
			mShownProgress = 1.0f;
		else if (mShownProgress < mLoadProgress)
			mShownProgress = (float)(mShownProgress + (double)LOAD_BAR_STEP);
		// _ftol2 truncates
		mLoadBar->setSource(RectI(0, 0, (int)((double)mLoadBarWidth * mShownProgress), mLoadBarHeight));
		return;
	case SPLASH_FAILED:
		getEngine()->quit();
		return;
	case SPLASH_LOADED:
		mLoadBar->setSource(RectI(0, 0, mLoadBarWidth, mLoadBarHeight));
		getEngine()->setFrameRate(GAME_FRAME_RATE);
		mBubbleText->burst();
		// 0x494820 (feedingfrenzy2): the stage shown under the screens
		if (mApp->mStageContainer != nullptr)
			mApp->mContainerSlot->addChild(mApp->mStageContainer);
		// The burst's sound, "bubbleTextBurst" at full volume and normal pitch (the game's sounds, 0x4747a0
		// (feedingfrenzy2)), and the sound system's update (0x434c80 (feedingfrenzy2)): not yet translated
		getEngine()->log("SplashState: the sound bubbleTextBurst not yet translated\n");
		mPhase = SPLASH_BURSTING;
		// (on into the bursting phase in the same update)
	case SPLASH_BURSTING:
		// The sound system's update by the frame's milliseconds (0x434c80 (feedingfrenzy2)): not yet translated
		mApp->mStage->update(theContext);
		if (!mBubbleText->isDone())
			return;
		// The next state: the menu. The screensaver's (SaverState, 0x18 bytes) in saver mode, and the game's
		// (GamePlayState, 0x1c bytes) when the configuration names a stage to start in (the application's +0x338),
		// are not yet translated, so in saver mode the splash stays
		if (!mSaverMode)
		{
			mApp->mStateMgr.changeState(new MenuState(mApp));
			return;
		}
		if (!mNextStateLogged)
			getEngine()->log("SplashState: the screensaver's state not yet translated\n");
		mNextStateLogged = true;
		return;
	}
}

void SplashState::startLoading()
{
	mShownProgress = 0.0f;
	// The original makes a thread (the engine's slot 0xc4, at a priority of 2) to run the loader, which sets the
	// phase to loading as it begins; the phase is set to failed when the thread does not start. The splash goes on
	// drawing while the loader runs. The port loads at once, at the machine's speed, so the bar fills in a frame
	mPhase = SPLASH_LOADING;
	load();
}

void SplashState::load()
{
	mLoadProgress = 0.0f;
	mApp->mGameData = new GameData();
	// The command line's stages= and menuStages= (the engine's table of arguments, its slot 0x38, read by 0x402970
	// (feedingfrenzy2)) would name other manifests: not yet translated
	mApp->mGameData->getStages().load("resource/Stages/stageManifest.xml");
	mApp->mGameData->getMenuStages().load(
		mSaverMode ? "resource/Stages/saverManifest.xml" : "resource/Stages/menuManifest.xml");
	// (The original checks after each part whether the state has left, then sets the phase to failed and stops; the
	// port loads at once, so it cannot have)
	mApp->mStageScreenData = new StageScreenData();
	mApp->mStageScreenData->load("resource/Stages/stageScreenData.xml");
	// 0x4946a0 (feedingfrenzy2): the shell's theme, falling back on the minimal one
	mApp->mShellTheme = new Theme();
	mApp->mShellTheme->load("resource/themes/shellTheme.xml");
	mApp->mShellTheme->setFallback(mApp->mTheme);
	mApp->mFunFacts = new FunFacts();
	mApp->mFunFacts->load("resource/funFacts.xml");
	mApp->mPlayerRanks = new PlayerRanks();
	mApp->mPlayerRanks->load("resource/playerRanks.xml");
	FishFactory::get()->load("resource/actors/fishes.xml");
	mApp->mMainMenu = new MainMenu();
	mApp->mMainMenu->init();
	mApp->mMainMenu->mActionSignal.connect(mApp, &FeedingFrenzyTwo::actionSlot);
	// The debug level select (the application's +0x2c, DebugSelectMenu over the game's stage manifest, its setup
	// 0x491b80 (feedingfrenzy2)), its actions connected likewise: not yet translated
	mLoadProgress = 0.1f;
	// The application's sound manager (+0x34, SoundMgr, 0x2c bytes, its constructor 0x48cee0 (feedingfrenzy2): two
	// empty lists) is made here, and nothing the export shows reads it: not translated
	mLoadProgress = 0.2f;

	FadeTransition* aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mPauseScreen = loadShellScreen("resource/shell/pauseScreen.xml", mApp->mShellTheme.get(), aFade, aFade);
	mLoadProgress = 0.3f;
	mApp->mStage = new Stage(mApp->mGameData.get(), mApp->mStageContainer.get());
	mLoadProgress = 0.4f;

	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mUserEntry = loadShellScreen("resource/shell/userEntry.xml", mApp->mShellTheme.get(), aFade, aFade);
	static_cast<EditBox*>(mApp->mUserEntry->findComponent("nameEntry"))->setFilter(nameEntryFilter);
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mUserDuplicateConfirm =
		loadShellScreen("resource/shell/userDuplicateConfirm.xml", mApp->mShellTheme.get(), aFade, aFade);
	mLoadProgress = 0.45f;
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mUserListFullConfirm =
		loadShellScreen("resource/shell/userlistfullconfirm.xml", mApp->mShellTheme.get(), aFade, aFade);
	mLoadProgress = 0.45f;
	// (Read without a theme)
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mAppQuitConfirm = loadShellScreen("resource/shell/appQuitConfirm.xml", nullptr, aFade, aFade);
	mLoadProgress = 0.475f;

	mApp->mUserData = new UserData();
	mApp->mUserData->init();
	// The players' list, read like the other shell screens but of its own kind (its list is filled by its init,
	// called each time the menu shows it)
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	GenericUserSelectScreen* aUserSelect = new GenericUserSelectScreen();
	mApp->mUserSelect = aUserSelect;
	aUserSelect->load("resource/shell/userSelect.xml", mApp->mShellTheme.get());
	aUserSelect->setTransitions(aFade, aFade);
	aUserSelect->mActionSignal.connect(mApp, &FeedingFrenzyTwo::actionSlot);
	mLoadProgress = 0.5f;
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mUserDeleteConfirm =
		loadShellScreen("resource/shell/userDeleteConfirm.xml", mApp->mShellTheme.get(), aFade, aFade);
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mGameExitConfirm =
		loadShellScreen("resource/shell/gameExitConfirm.xml", mApp->mShellTheme.get(), aFade, aFade);
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mResetHighScoresConfirm =
		loadShellScreen("resource/shell/resetHighScoresConfirm.xml", mApp->mShellTheme.get(), aFade, aFade);
	mLoadProgress = 0.6f;
	// (Its own, quicker, out transition, made after the in)
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	FadeTransition* aFadeOut = new FadeTransition(RESTORE_FADE_OUT_SECONDS);
	mApp->mRestoreGameScreen =
		loadShellScreen("resource/shell/restoreGameScreen.xml", mApp->mShellTheme.get(), aFade, aFadeOut);
	mLoadProgress = 0.65f;
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mGenericLoadScreen =
		loadShellScreen("resource/shell/genericLoadScreen.xml", mApp->mShellTheme.get(), aFade, aFade);
	mLoadProgress = 0.7f;

	// The options (+0xa4, FeedingFrenzyTwoOptionScreen): not yet translated
	mLoadProgress = 0.75f;
	// The map: read with the shell's theme, given one fade for both ways, set up from the game's stages
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mGameMap = new GameMapScreen();
	mApp->mGameMap->load("resource/shell/gameMapBase.xml", mApp->mShellTheme.get());
	mApp->mGameMap->setTransitions(aFade, aFade);
	mApp->mGameMap->setup(&mApp->mGameData->getStages());
	mApp->mGameMap->mActionSignal.connect(mApp, &FeedingFrenzyTwo::actionSlot);
	mLoadProgress = 0.75f;
	// The stage's intro: read with the shell's theme, given one fade for both ways
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mStageIntro = new StageIntroScreen();
	mApp->mStageIntro->load("resource/shell/stageStartBase.xml", mApp->mShellTheme.get());
	mApp->mStageIntro->setTransitions(aFade, aFade);
	mApp->mStageIntro->mActionSignal.connect(mApp, &FeedingFrenzyTwo::actionSlot);
	mLoadProgress = 0.75f;
	// The normal stage's end: read by its loader, given one fade for both ways
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mStageCompleteNormal = new StageCompleteScreenNormal();
	mApp->mStageCompleteNormal->loadScreen("resource/shell/stageCompleteBase.xml");
	mApp->mStageCompleteNormal->setTransitions(aFade, aFade);
	mApp->mStageCompleteNormal->mActionSignal.connect(mApp, &FeedingFrenzyTwo::actionSlot);
	mLoadProgress = 0.8f;
	// The bonus stage's end, read the same way
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mStageCompleteBonus = new StageCompleteScreenBonus();
	mApp->mStageCompleteBonus->loadScreen("resource/shell/stageCompleteBonus.xml");
	mApp->mStageCompleteBonus->setTransitions(aFade, aFade);
	mApp->mStageCompleteBonus->mActionSignal.connect(mApp, &FeedingFrenzyTwo::actionSlot);
	mLoadProgress = 0.8f;
	// The continue offer and the game over, each given one fade for both ways
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mGameOverContinue =
		loadShellScreen("resource/shell/gameOverContinuesLeft.xml", mApp->mShellTheme.get(), aFade, aFade);
	mLoadProgress = 0.8f;
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mGameOver =
		loadShellScreen("resource/shell/gameOverNoContinuesLeft.xml", mApp->mShellTheme.get(), aFade, aFade);
	// The high scores (+0x8c), their own kind of screen: not yet translated
	getEngine()->log("SplashState: the options and high score screens not yet translated\n");
	mLoadProgress = 0.82f;
	// (Read without a theme)
	aFade = new FadeTransition(SHELL_FADE_SECONDS);
	mApp->mCredits = loadShellScreen("resource/shell/credits.xml", nullptr, aFade, aFade);
	mLoadProgress = 0.84f;

	// The stage's loader (Stage::StageLoader, 0x34 bytes, its constructor 0x495080 and its run 0x48a810
	// (feedingfrenzy2)) loading the first menu stage on a thread the loader waits on, its progress the bar's last
	// part. port: loaded at once, as the rest. The run's other steps: the stage emptied first (its slot 0x08), the
	// fish factory's file read again (read above), and once loaded the entry's time and canSuck kept on the
	// application (+0x190 and +0x1d0, 0x48a910 and 0x4944f0 (feedingfrenzy2)): not yet translated. (The stage the
	// configuration names, the application's +0x338, is not yet translated either)
	const std::vector<StageManifest::Stage>& aMenuStages = mApp->mGameData->getMenuStages().getStages();
	if (!aMenuStages.empty())
		mApp->mStage->load(aMenuStages[0]);
	mLoadProgress = 1.0f;
	mPhase = SPLASH_LOADED;
}

GenericScreen* SplashState::loadShellScreen(const char* thePath, Theme* theTheme, Transition* theIn,
                                            Transition* theOut)
{
	GenericScreen* aScreen = new GenericScreen();
	aScreen->load(thePath, theTheme);
	aScreen->setTransitions(theIn, theOut);
	aScreen->mActionSignal.connect(mApp, &FeedingFrenzyTwo::actionSlot);
	return aScreen;
}
