#include "FishFood.h"
#include "Host.h"
#include "ImageNode.h"
#include "Random.h"
#include "Sound.h"
#include "StageMgr.h"
#include "TextItem.h"

#include <algorithm>

using namespace feedingfrenzy;

FishFood* FishFood::sInstance = nullptr;

namespace
{
const char* const WINDOW_TITLE = "Feeding Frenzy";

// The game's archive (0x430b80) and the files init reads from it
const char* const ARCHIVE_FILE = "FFarchive.saf";
const char* const STRINGS_FILE = "config\\gameStrings.xml";
const char* const STAGE_MANIFEST_FILE = "config\\stages\\stageManifest.xml";

// The logo that slides down over the menus, and the "TM" beside it (0x430180)
const char* const LOGO_IMAGE = "resources\\menus\\logo.png";
const char* const LOGO_FONT = "resources\\menus\\infoText14.xml";
const float LOGO_HIDDEN_Y = -80.0f;
const float LOGO_SHOWN_Y = 50.0f;
const float LOGO_X = 320.0f;
const float LOGO_SLIDE_SECONDS = 0.3f;
const float TM_X = 105.0f;
const float TM_Y = 24.0f;
const float TM_OPACITY = 0.6f;

// The splash screen (0x42cc70): its picture is a loose file, its font the archive's
const char* const SPLASH_IMAGE = "resources\\splash\\gameSplash.jpg";
const char* const SPLASH_FONT = "resources\\hud\\FrenzyFontWhite.xml";
const float LOADING_X = 320.0f;
const float LOADING_Y = 150.0f;
const float LOADING_OPACITY = 0.8f;
const float SPLASH_SECONDS = 2.05f;      // how long it stays at least (0x430180)
const float SPLASH_FADE_SECONDS = 1.0f;  // how long it takes to fade away (0x42ec30)

// The fade over the whole screen (0x430180)
const float COVER_FADE_SECONDS = 1.0f;

// A picture the loading state fetches every frame (0x42ec30): the image cache keeps it
const char* const PRELOADED_IMAGE = "resources\\backdrops\\SunkenTreasure\\mg_plant2.jpg";

// The menu's stage (0x42ec30): the folder of its files and its name
const char* const MENU_STAGE_PATH = "config\\Stages\\MenuStage";
const char* const MENU_STAGE_NAME = "Menu Stage";

// The files of the screens (0x42e420) and of the sounds
const char* const MAIN_MENU_FILE = "config\\mainMenu.xml";
const char* const OPTIONS_FILE = "config\\optionScreen.xml";
const char* const HIGH_SCORE_FILE = "config\\HighScore.xml";
const char* const HIGH_SCORE_ENTRY_FILE = "config\\highScoreEntry.xml";
const char* const GAME_WON_FILE = "config\\gameWon.xml";
const char* const PAUSE_FILE = "config\\pauseScreen.xml";
const char* const FUN_FACTS_FILE = "config\\toolTips\\funFacts.xml";
const char* const CREDITS_FILE = "config\\credits.xml";
const char* const LOAD_MAP_NAME = "LoadMap";
const char* const GLOBAL_SOUNDS_FILE = "config\\globalSounds.xml";
const char* const MENU_MUSIC_NAME = "menuMusic";
const char* const MENU_MUSIC_FILE = "resources\\music\\menuMusic.wav";
const char* const SPAWN_SOUND_NAME = "playerSpawn";
const char* const SPAWN_SOUND_FILE = "resources\\sounds\\playerSpawn.wav";
const char* const GAME_WON_SOUND = "mermaid";

// The keys of the scores and the foodbank below the game's own (0x42e420)
const char* const NORMAL_SCORES_KEY = "scores\\normal";
const char* const TIMED_SCORES_KEY = "scores\\timed";
const char* const FOOD_BANK_KEY = "foodBank";

// The names that fill a list that has fewer than ten scores (0x42d0c0), each with no points
const char* const DEFAULT_NORMAL_NAME = "Andy";
const char* const DEFAULT_TIMED_NAME = "Eddie";
const int SCORES_PER_LIST = 10;

// The volume categories of a sound: the sounds, then the music
const int SOUND_CATEGORY_SOUNDS = 0;
const int SOUND_CATEGORY_MUSIC = 1;
const float MENU_MUSIC_FADE_IN_SECONDS = 1.0f;
const float MENU_MUSIC_FADE_OUT_SECONDS = 0.5f;

// The seconds screens take to fade in and out
const float SCREEN_FADE_IN_SECONDS = 0.2f;
const float SCREEN_FADE_OUT_SECONDS = 0.1f;
const float CREDITS_FADE_OUT_SECONDS = 0.3f;
const float LOAD_MAP_FADE_IN_SECONDS = 0.3f;
// How long the loading map shows before the stage's files load
const float LOAD_MAP_WAIT_SECONDS = 0.5f;

// The steps of the loading state (+0x60)
enum
{
	LOADING_STEP_WAIT = 0,    // the splash's minimum time
	LOADING_STEP_LOAD = 2,    // the menu's stage and screens are made
	LOADING_STEP_STAGE = 3,   // the menu's stage loads
	LOADING_STEP_FADE = 4     // the splash fades out
};

// The logo's states (+0xb8)
enum
{
	LOGO_HIDDEN = 0,
	LOGO_SLIDING_IN = 1,
	LOGO_SHOWN = 2,
	LOGO_SLIDING_OUT = 3
};

// A node of a screen is shown or hidden
void setNodeVisible(engine::ScreenLayout* theScreen, const char* theName, bool theVisible)
{
	theScreen->getNode(theName)->setVisible(theVisible);
}

}

// 0x430ee0
FishFood::FishFood() :
	mState(FOOD_STATE_LOADING), mStateSeconds(0.0f), mStep(0), mMapWait(0.0f), mPopup(nullptr), mShowFadeSeconds(0.0f),
	mLogoState(LOGO_HIDDEN), mLogoSecondsLeft(-1.0f), mLogoSlideSeconds(LOGO_SLIDE_SECONDS),
	mLogoHiddenPosition(LOGO_X, LOGO_HIDDEN_Y), mLogoShownPosition(LOGO_X, LOGO_SHOWN_Y), mMenuMusic(nullptr),
	mStageIndex(0), mCursorLocked(false), mGameMode(GAME_MODE_NORMAL), mScoresLoaded(false)
{
	sInstance = this;
	engine::gSignalBuild = true;
	engine::gEarlyBuild = true;
}

// 0x431310
FishFood::~FishFood()
{
	if (sInstance == this)
		sInstance = nullptr;
}

// 0x42b8e0: the window's title, icon, size and frame rate
void FishFood::configure(engine::AppConfig& theConfig)
{
	theConfig.mTitle = WINDOW_TITLE;
	theConfig.mIcon = WINDOW_ICON;
	theConfig.mWidth = SCREEN_WIDTH;
	theConfig.mHeight = SCREEN_HEIGHT;
	theConfig.mFrameRate = FRAME_RATE;
}

// 0x430b80: the settings are read and the display made in the full screen they say (the host has made the window by
// now), then the archive, the game's strings and the stage manifest, and the scene and the splash (startup)
void FishFood::init()
{
	mSettings.load();
	engine::Host* aHost = engine::getHost();
	aHost->setFullScreen(mSettings.isFullScreen());
	engine::App* anApp = engine::App::instance();
	anApp->setUseFiles(false);
	anApp->addArchive(ARCHIVE_FILE);
	mStrings.load(STRINGS_FILE);
	loadStageManifest(STAGE_MANIFEST_FILE, mManifest);
	startup();
}

// 0x430180: the scene and its two groups (the screens and the popups), the fade over everything, the logo with its
// "TM", and the splash; the window's signals go to the controller
void FishFood::startup()
{
	engine::App* anApp = engine::App::instance();
	unsigned int aSeed = engine::getHost()->timeMs();
	engine::getHost()->testSeed(aSeed);
	engine::crtSrand(aSeed);
	anApp->mUpdateSignal.connect(this, &FishFood::onAppUpdate);
	anApp->mActivateSignal.connect(this, &FishFood::onAppActivate);
	anApp->mKeyDownSignal.connect(this, &FishFood::onKeyDown);
	anApp->mKeyUpSignal.connect(this, &FishFood::onKeyUp);
	// 0x45ffd0, then the application's 0x88: the sound system
	engine::SoundSystem* aSounds = engine::getHost()->createSoundSystem();
	anApp->setSoundSystem(aSounds);

	mScene = new engine::Scene();
	mScene->setName("Scene");
	mScene->connect();
	mScene->layout();
	mRoot = new engine::Container();
	mRoot->setName("RootGroup");
	mRoot->setPosition(0.0f, 0.0f);
	mScene->addChild(mRoot);
	mPopups = new engine::Container();
	mPopups->setName("PopupGroup");
	mPopups->setPosition(0.0f, 0.0f);
	mScene->addChild(mPopups);

	mFader = new ScreenFader();
	mFader->setCoverColor(0.0f, 0.0f, 0.0f);
	mFader->setOpacity(1.0f);
	mFader->setDurations(COVER_FADE_SECONDS, -1.0f);
	mFader->setRect(engine::Rect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT));

	engine::Image* aLogo = anApp->getImage(LOGO_IMAGE);
	mLogoGroup = new engine::Container();
	mLogoGroup->addChild(new engine::ImageNode(aLogo));
	mScene->addChild(mLogoGroup);
	engine::TextItem* aTrademark = new engine::TextItem();
	mLogoGroup->addChild(aTrademark);
	aTrademark->setFont(anApp->getFont(LOGO_FONT));
	aTrademark->setText("TM");
	aTrademark->setPosition(TM_X, TM_Y);
	aTrademark->setOpacity(TM_OPACITY);
	mLogoGroup->setPosition(mLogoHiddenPosition);

	showSplash();
	mScene->addChild(mSplash);
	mScene->addChild(mFader);
	mSplash->setOpacity(1.0f);
	mStep = LOADING_STEP_WAIT;
	mFader->fadeFromColor();
	mStateSeconds = SPLASH_SECONDS;
	mState = FOOD_STATE_LOADING;
}

// 0x42cc70: the splash screen's picture with the word "LOADING" under it
void FishFood::showSplash()
{
	engine::App* anApp = engine::App::instance();
	mSplash = new engine::FadeContainer();
	mSplash->addChild(new engine::ImageNode(anApp->getFileImage(SPLASH_IMAGE)));
	engine::TextItem* aLoading = new engine::TextItem();
	mSplash->addChild(aLoading);
	aLoading->setFont(SPLASH_FONT);
	aLoading->setHAlign(1);
	aLoading->setPosition(LOADING_X, LOADING_Y);
	aLoading->setOpacity(LOADING_OPACITY);
	aLoading->setText(mStrings.get("loading"));
}

// 0x42fa80: the sound system's update, the popup screens, the logo, and the state's own
void FishFood::onAppUpdate(engine::UpdateInfo& theInfo)
{
	engine::SoundSystem* aSounds = engine::App::getSoundSystem();
	if (aSounds != nullptr)
		aSounds->update((int)(theInfo.mSeconds * 1000.0f));
	updateCursorLock();
	updateScreens();
	updateLogo(theInfo);
	switch (mState)
	{
	case FOOD_STATE_LOADING:
		updateLoading(theInfo);
		break;
	case FOOD_STATE_LOAD_MAP:
		updateLoadMap(theInfo);
		break;
	case FOOD_STATE_STAGE_ENDED:
		updateStageEnded();
		break;
	case FOOD_STATE_NEXT_STAGE:
		updateNextStage();
		break;
	}
}

// 0x42c060 (with 0x42bb50): the pointer is held and hidden while the game's stage is played, and let go while it waits
void FishFood::updateCursorLock()
{
	if (mGameStage != nullptr && !mGameStage->isWaiting())
	{
		if (!mCursorLocked)
		{
			engine::getHost()->setCursorLocked(true);
			mCursorLocked = true;
		}
		return;
	}
	if (mCursorLocked)
	{
		mCursorLocked = false;
		engine::getHost()->setCursorLocked(false);
	}
}

// port (test commands): the controller's state as a word
const char* FishFood::getStateName() const
{
	switch (mState)
	{
	case FOOD_STATE_LOADING:
		return "loading";
	case FOOD_STATE_MENU:
		return "menu";
	case FOOD_STATE_LOAD_MAP:
		return "loadmap";
	case FOOD_STATE_PLAYING:
		return "stage";
	case FOOD_STATE_STAGE_ENDED:
		return "stageended";
	case FOOD_STATE_GAME_WON:
		return "gamewon";
	case FOOD_STATE_ENTERING_SCORE:
		return "entername";
	case FOOD_STATE_NEXT_STAGE:
		return "nextstage";
	}
	return "unknown";
}

// port (test commands): the popup screen that shows, by the screen it is
const char* FishFood::getPopupName() const
{
	if (mPopup == nullptr)
		return "";
	if (mPopup == mOptionsScreen)
		return "options";
	if (mPopup == mHighScoreScreen)
		return "highscores";
	if (mPopup == mHighScoreEntryScreen)
		return "entername";
	if (mPopup == mGameWonScreen)
		return "gamewon";
	if (mPopup == mPauseScreen)
		return "pause";
	if (mPopup == mCreditsScreen)
		return "credits";
	if (mPopup == mMapScreen)
		return "map";
	if (mPopup == mLoadMapScreen)
		return "loadmap";
	return "popup";
}

// port (test commands)
engine::ScreenLayout* FishFood::getClickScreen()
{
	if (mPopup != nullptr)
		return mPopup;
	if (mState == FOOD_STATE_MENU)
		return mMainMenu;
	return nullptr;
}

// port (test commands)
void FishFood::unlockAllStages()
{
	mNormalScores.reachStage(getStageCount());
	mTimedScores.reachStage(getStageCount());
}

// port (test commands)
std::string FishFood::getEnteredName()
{
	return mHighScoreEntryScreen->getEnteredName();
}

// 0x42c230: the window's focus lost pauses a game that is running
void FishFood::onAppActivate(bool theActive)
{
	if (!theActive && mGameStage != nullptr && !mGameStage->isWaiting())
		pauseGame();
}

// 0x444c00: a handler joins the application's, unless it is there
void FishFood::addKeyHandler(engine::KeyHandler* theHandler)
{
	if (std::find(mKeyHandlers.begin(), mKeyHandlers.end(), theHandler) != mKeyHandlers.end())
		return;
	mKeyHandlers.push_back(theHandler);
	theHandler->addRef();
}

// 0x443640
void FishFood::removeKeyHandler(engine::KeyHandler* theHandler)
{
	std::vector<engine::KeyHandler*>::iterator anIt = std::find(mKeyHandlers.begin(), mKeyHandlers.end(), theHandler);
	if (anIt != mKeyHandlers.end())
	{
		mKeyHandlers.erase(anIt);
		theHandler->release();
	}
}

// A key that went down: the handlers added last are asked first, until one takes it (a handler may remove itself)
void FishFood::onKeyDown(int theKey)
{
	std::vector<engine::KeyHandler*> aHandlers = mKeyHandlers;
	for (size_t i = aHandlers.size(); i > 0; i--)
	{
		if (aHandlers[i - 1]->keyDown(theKey))
			return;
	}
}

// A key that went up
void FishFood::onKeyUp(int theKey)
{
	std::vector<engine::KeyHandler*> aHandlers = mKeyHandlers;
	for (size_t i = aHandlers.size(); i > 0; i--)
	{
		if (aHandlers[i - 1]->keyUp(theKey))
			return;
	}
}

// 0x42ec30: the splash stays its minimum time, then the menu's stage and screens are made and loaded, and the menu
// shows as the splash fades out
void FishFood::updateLoading(engine::UpdateInfo& theInfo)
{
	mStateSeconds -= theInfo.mSeconds;
	engine::App::instance()->getImage(PRELOADED_IMAGE);
	if (mStateSeconds < 0.0f)
	{
		switch (mStep)
		{
		case LOADING_STEP_WAIT:
			mStep = LOADING_STEP_LOAD;
			break;
		case LOADING_STEP_LOAD:
			mMenuStage = new StageMgr();
			createScreens();
			loadGlobalSounds();
			mMenuStageInfo.setPath(MENU_STAGE_PATH);
			mMenuStageInfo.setName(MENU_STAGE_NAME);
			mMenuStageInfo.setIndex(0);
			mMenuStage->startLoad(&mMenuStageInfo);
			mStateSeconds = 0.0f;
			mStep = LOADING_STEP_STAGE;
			break;
		case LOADING_STEP_STAGE:
			if (mMenuStage->isLoaded())
			{
				mMenuStage->init();
				mMenuStage->beginPlay();
				rebuildRoot();
				// port: the splash leaves the scene when it has faded out (the original leaves it there, clear)
				mSplash->fade(false, SPLASH_FADE_SECONDS, 1.0f, true);
				mStep = LOADING_STEP_FADE;
			}
			else
				mMenuStage->update(theInfo);
			break;
		case LOADING_STEP_FADE:
			mSplash = nullptr;
			enterMenu();
			slideLogoIn();
			break;
		}
	}
}

// 0x42c760: the loading map shows for a while, the game's stage is made and loads (a step each frame), and when it
// has loaded the game goes on
void FishFood::updateLoadMap(engine::UpdateInfo& theInfo)
{
	mMapWait -= theInfo.mSeconds;
	if (mGameStage == nullptr && mMapWait < 0.0f)
	{
		mGameStage = new StageMgr();
		mMenuStage->setVisible(false);
		mMenuStage->pause();
		mGameStage->startLoad(mManifest.mStages[mStageIndex].get());
	}
	if (mGameStage != nullptr)
	{
		mGameStage->update(theInfo);
		if (mGameStage->isLoaded())
		{
			mGameStage->init();
			startStage();
		}
	}
}

// 0x42fa80, state 6: the stage that was played goes; the game is won when the stage was the last, else a score that
// has made the list asks for a name, and else the menu comes back
void FishFood::updateStageEnded()
{
	if (mGameStage != nullptr)
	{
		removeGameStage();
		if (getStageCount() <= mStageIndex)
		{
			bool aTimed = mSession.getMode() == GAME_MODE_TIMED;
			setNodeVisible(mGameWonScreen, "timedMode", aTimed);
			setNodeVisible(mGameWonScreen, "timedHeader", aTimed);
			setNodeVisible(mGameWonScreen, "timedText", aTimed);
			setNodeVisible(mGameWonScreen, "helpText", !aTimed);
			setNodeVisible(mGameWonScreen, "helpHeader", !aTimed);
			showScreen(mGameWonScreen, SCREEN_FADE_IN_SECONDS);
			playSound(GAME_WON_SOUND, 1.0f, 1.0f);
			mState = FOOD_STATE_GAME_WON;
			return;
		}
		if (showHighScoreEntry(SCORES_PER_LIST))
			return;
	}
	enterMenu();
}

// 0x42fa80, state 9: the stage that was played goes, and the next one loads behind the loading map
void FishFood::updateNextStage()
{
	if (mGameStage != nullptr)
		removeGameStage();
	showLoadMap();
	mLoadMapScreen->setLoaded(false);
	mMapWait = LOAD_MAP_WAIT_SECONDS;
	mState = FOOD_STATE_LOAD_MAP;
}

// 0x42fa80 (states 6 and 9): the game's stage leaves the screens, lets go of what it holds, and is released
void FishFood::removeGameStage()
{
	mRoot->removeChild(mGameStage);
	mGameStage->cleanup();
	mGameStage = nullptr;
}

// 0x42fa80 (state 6) and 0x42f190 (gameWonDone): the name is asked for when the game's score is among the best of the
// normal game (the original asks the normal list in the timed game too)
bool FishFood::showHighScoreEntry(int theRankLimit)
{
	int aScore = mSession.getScore();
	int aRank = mNormalScores.getRank((unsigned int)aScore);
	if (aRank < theRankLimit && aScore > 0)
	{
		mHighScoreEntryScreen->setUp(mSettings.getDefaultScoreName());
		showScreen(mHighScoreEntryScreen, SCREEN_FADE_IN_SECONDS);
		addKeyHandler(mHighScoreEntryScreen);
		mState = FOOD_STATE_ENTERING_SCORE;
		return true;
	}
	return false;
}

// 0x42c3a0 (case 1): the menu's stage and screen are the root, and the stage plays on
void FishFood::enterMenu()
{
	rebuildRoot();
	mMenuStage->setVisible(true);
	mMenuStage->resume();
	mState = FOOD_STATE_MENU;
}

// 0x42c3a0 (case 5): the stage has loaded: the loading map is done, and the stage is played
void FishFood::startStage()
{
	mLoadMapScreen->setLoaded(true);
	mRoot->removeAllChildren();
	mState = FOOD_STATE_PLAYING;
	mGameStage->beginPlay();
}

// 0x42b980: the root has the menu's stage and the main menu, and the menu's music plays
void FishFood::rebuildRoot()
{
	mRoot->removeAllChildren();
	mRoot->addChild(mMenuStage);
	mRoot->addChild(mMainMenu);
	if (mMenuMusic != nullptr)
	{
		mMenuMusic->setFadeIn(MENU_MUSIC_FADE_IN_SECONDS);
		mMenuMusic->setFadeOut(MENU_MUSIC_FADE_OUT_SECONDS);
		mMenuMusic->setLoop(true);
		mMenuMusic->play();
	}
}

// 0x42ba00: the root has the loading map only, fading in
void FishFood::showLoadMap()
{
	mRoot->removeAllChildren();
	mRoot->addChild(mLoadMapScreen);
	mLoadMapScreen->prepare();
	mLoadMapScreen->fade(true, LOAD_MAP_FADE_IN_SECONDS, 1.0f, false);
}

// 0x42e420: the screens, the lists of scores, the facts and the music are made, once
void FishFood::createScreens()
{
	createMainMenu();

	engine::SoundSystem* aSounds = engine::App::getSoundSystem();
	if (aSounds != nullptr)
	{
		aSounds->load(MENU_MUSIC_NAME, MENU_MUSIC_FILE);
		mMenuMusic = aSounds->createSound(MENU_MUSIC_NAME, false, SOUND_CATEGORY_MUSIC);
	}

	mMapScreen = new GameMapLayout();
	mMapScreen->setIsMap(true);

	mOptionsScreen = new engine::ScreenLayout();
	mOptionsScreen->loadLayout(OPTIONS_FILE);
	mOptionsScreen->setListener(this);
	applySettings();

	mHighScoreScreen = new HighScoreLayout();
	mHighScoreScreen->loadLayout(HIGH_SCORE_FILE);
	mHighScoreScreen->setListener(this);
	mNormalScores.load(NORMAL_SCORES_KEY);
	mTimedScores.load(TIMED_SCORES_KEY);
	mFoodBank.load(FOOD_BANK_KEY);
	mScoresLoaded = true;
	mHighScoreScreen->setLists(&mNormalScores, &mTimedScores);
	if (mNormalScores.getCount() < SCORES_PER_LIST || mTimedScores.getCount() < SCORES_PER_LIST)
		fillScoreLists();
	mHighScoreScreen->refresh();

	mHighScoreEntryScreen = new HighScoreEntryLayout();
	mHighScoreEntryScreen->setListener(this);
	mHighScoreEntryScreen->loadLayout(HIGH_SCORE_ENTRY_FILE);

	mGameWonScreen = new engine::ScreenLayout();
	mGameWonScreen->setListener(this);
	mGameWonScreen->loadLayout(GAME_WON_FILE);

	mPauseScreen = new engine::ScreenLayout();
	mPauseScreen->loadLayout(PAUSE_FILE);
	mPauseScreen->setListener(this);

	mFunFacts.reset(new FunFacts());
	mFunFacts->load(FUN_FACTS_FILE);

	mLoadMapScreen = new GameMapLayout();
	mLoadMapScreen->setIsMap(false);
	mLoadMapScreen->setName(LOAD_MAP_NAME);
	mLoadMapScreen->setListener(this);

	mCreditsScreen = new engine::ScreenLayout();
	mCreditsScreen->setListener(this);
	mCreditsScreen->loadLayout(CREDITS_FILE);

	if (aSounds != nullptr)
	{
		aSounds->load(SPAWN_SOUND_NAME, SPAWN_SOUND_FILE);
		engine::Sound* aSpawnSound = aSounds->createSound(SPAWN_SOUND_NAME, false, SOUND_CATEGORY_SOUNDS);
		if (aSpawnSound != nullptr)
			aSpawnSound->close();
	}
}

// 0x42d980: the main menu
void FishFood::createMainMenu()
{
	mMainMenu = new MainMenuScreen();
	mMainMenu->loadLayout(MAIN_MENU_FILE);
	mMainMenu->setListener(this);
	mMainMenu->hideHelpers();
}

// 0x42d190: the sounds every stage uses are read into the sound bank
void FishFood::loadGlobalSounds()
{
	mSoundBank.load(GLOBAL_SOUNDS_FILE);
}

// 0x42d0c0: a list of fewer than ten scores gets scores of no points, named for the normal game and for the timed
void FishFood::fillScoreLists()
{
	while (mNormalScores.getCount() < SCORES_PER_LIST)
		mNormalScores.add(DEFAULT_NORMAL_NAME, 0);
	while (mTimedScores.getCount() < SCORES_PER_LIST)
		mTimedScores.add(DEFAULT_TIMED_NAME, 0);
}

// 0x42c0c0
StageMgr* FishFood::getStageMgr()
{
	return mState != FOOD_STATE_PLAYING ? mMenuStage.get() : mGameStage.get();
}

// 0x42be20
float FishFood::getLoadProgress()
{
	return mGameStage != nullptr ? mGameStage->getLoadProgress() : 0.0f;
}

// 0x42d230
void FishFood::playSound(const std::string& theName, float theVolume, float thePitch)
{
	mSoundBank.play(theName, theVolume, thePitch);
}

// 0x42ba60: a screen to show at the next update, with the time it fades in
void FishFood::showScreen(engine::ScreenLayout* theScreen, float theFadeSeconds)
{
	mShowingScreen = theScreen;
	theScreen->setEnabled(true);
	mShowFadeSeconds = theFadeSeconds;
}

// 0x42baf0: a screen that takes no more of the mouse goes, at once or fading out over the time
void FishFood::hideScreen(engine::ScreenLayout* theScreen, float theFadeSeconds)
{
	theScreen->setEnabled(false);
	if (mPopup == theScreen)
		mPopup = nullptr;
	if (theFadeSeconds == 0.0f)
		theScreen->addOwnFlags(engine::NODE_REMOVE);
	else
		theScreen->fade(false, theFadeSeconds, 1.0f, true);
}

// 0x42ba80: the screen to show joins the popups and fades in
void FishFood::updateScreens()
{
	if (mShowingScreen != nullptr)
	{
		mPopups->addChild(mShowingScreen);
		mPopup = mShowingScreen;
		mShowingScreen->fade(true, mShowFadeSeconds, 1.0f, false);
		mShowingScreen = nullptr;
	}
}

// The stage joins the screens
void FishFood::addStage(StageMgr* theStage)
{
	mRoot->addChild(theStage);
}

// 0x42bc40
void FishFood::slideLogoIn()
{
	if (mLogoState == LOGO_HIDDEN)
	{
		mLogoSecondsLeft = mLogoSlideSeconds;
		mLogoState = LOGO_SLIDING_IN;
	}
	else if (mLogoState == LOGO_SLIDING_OUT)
	{
		mLogoState = LOGO_SLIDING_IN;
		mLogoSecondsLeft = mLogoSlideSeconds - mLogoSecondsLeft;
	}
}

// 0x42bc90
void FishFood::slideLogoOut()
{
	if (mLogoState == LOGO_SLIDING_IN)
	{
		mLogoState = LOGO_SLIDING_OUT;
		mLogoSecondsLeft = mLogoSlideSeconds - mLogoSecondsLeft;
	}
	else if (mLogoState == LOGO_SHOWN)
	{
		mLogoSecondsLeft = mLogoSlideSeconds;
		mLogoState = LOGO_SLIDING_OUT;
	}
}

// 0x42bcd0: the logo's place between its two positions follows the slide's time
void FishFood::updateLogo(engine::UpdateInfo& theInfo)
{
	mLogoSecondsLeft -= theInfo.mSeconds;
	if (mLogoSecondsLeft < 0.0f)
		mLogoSecondsLeft = 0.0f;
	float aFraction = 0.0f;
	switch (mLogoState)
	{
	case LOGO_HIDDEN:
		aFraction = 0.0f;
		break;
	case LOGO_SLIDING_IN:
		aFraction = 1.0f - mLogoSecondsLeft / mLogoSlideSeconds;
		if (mLogoSecondsLeft <= 0.0f)
			mLogoState = LOGO_SHOWN;
		break;
	case LOGO_SHOWN:
		aFraction = 1.0f;
		break;
	case LOGO_SLIDING_OUT:
		aFraction = mLogoSecondsLeft / mLogoSlideSeconds;
		if (mLogoSecondsLeft <= 0.0f)
			mLogoState = LOGO_HIDDEN;
		break;
	}
	mLogoGroup->setPosition((mLogoShownPosition.x - mLogoHiddenPosition.x) * aFraction + mLogoHiddenPosition.x,
		(mLogoShownPosition.y - mLogoHiddenPosition.y) * aFraction + mLogoHiddenPosition.y);
}

// 0x42bec0: the sound and music categories are on or off, and the window is in the full screen the settings say
void FishFood::applySettings()
{
	engine::SoundSystem* aSounds = engine::App::getSoundSystem();
	if (aSounds != nullptr)
	{
		aSounds->setCategoryVolume(SOUND_CATEGORY_SOUNDS, mSettings.isSoundEnabled() ? 1.0f : 0.0f);
		aSounds->setCategoryVolume(SOUND_CATEGORY_MUSIC, mSettings.isMusicEnabled() ? 1.0f : 0.0f);
	}
	engine::Host* aHost = engine::getHost();
	if (aHost->isFullScreen() != mSettings.isFullScreen())
		aHost->setFullScreen(mSettings.isFullScreen());
}

// 0x42bf60: the game's stage stops, the logo slides in and the pause screen shows
void FishFood::pauseGame()
{
	if (mGameStage == nullptr)
		return;
	mGameStage->pause();
	slideLogoIn();
	mPauseScreen->clearCallback();
	showScreen(mPauseScreen, SCREEN_FADE_IN_SECONDS);
}

// 0x42bfe0: the game's stage plays on, the logo slides out and the pause screen goes
void FishFood::pauseResume()
{
	if (mGameStage == nullptr)
		return;
	mGameStage->resume();
	slideLogoOut();
	hideScreen(mPauseScreen, SCREEN_FADE_OUT_SECONDS);
}

// 0x42cf90: the foodbank is kept, and the game's stage is over (the pause screen goes with it)
void FishFood::finishStage()
{
	if (mScoresLoaded)
		mFoodBank.save(FOOD_BANK_KEY);
	mState = FOOD_STATE_STAGE_ENDED;
	hideScreen(mPauseScreen, SCREEN_FADE_OUT_SECONDS);
}

// 0x42dc90: the game goes to a stage of the manifest: the end of the game when there is none, else the furthest stage
// of the game's list is made further and the stage loads behind the loading map (a stage that is played is replaced)
void FishFood::goToStage(int theIndex)
{
	if (mScoresLoaded)
		mFoodBank.save(FOOD_BANK_KEY);
	mStageIndex = theIndex;
	if (getStageCount() <= theIndex)
	{
		finishStage();
		return;
	}
	int aMode = mSession.getMode();
	if (aMode == GAME_MODE_NORMAL)
		mNormalScores.reachStage(theIndex);
	else if (aMode == GAME_MODE_TIMED)
		mTimedScores.reachStage(theIndex);
	switch (mState)
	{
	case FOOD_STATE_MENU:
	case FOOD_STATE_LOAD_MAP:
		showLoadMap();
		mLoadMapScreen->setLoaded(false);
		mMapWait = LOAD_MAP_WAIT_SECONDS;
		mState = FOOD_STATE_LOAD_MAP;
		break;
	case FOOD_STATE_PLAYING:
		mState = FOOD_STATE_NEXT_STAGE;
		break;
	}
}

// 0x42ee90: a new game: the menu's music stops, the session starts again in the mode, and the game goes to the stage
void FishFood::startGame(int theMode, int theStage)
{
	if (mMenuMusic != nullptr)
		mMenuMusic->stop();
	mSession.reset();
	mSession.setMode(theMode);
	mGameMode = theMode;
	mStageIndex = theStage;
	goToStage(theStage);
}

// 0x42c9d0
void FishFood::shutdown()
{
	mSettings.save();
	if (mScoresLoaded)
	{
		mNormalScores.save(NORMAL_SCORES_KEY);
		mTimedScores.save(TIMED_SCORES_KEY);
		mFoodBank.save(FOOD_BANK_KEY);
	}
}

// 0x42f190: what a screen's button says is done. The screens' own actions the controller does not handle (the stages
// of the map, the stage screens') are done by the screens
void FishFood::screenAction(std::string theAction)
{
	if (theAction == "newGame")
	{
		if (mNormalScores.getFurthestStage() < 1)
			startGame(GAME_MODE_NORMAL, 0);
		else
		{
			mSession.reset();
			mSession.setMode(GAME_MODE_NORMAL);
			mGameMode = GAME_MODE_NORMAL;
			mMapScreen->prepare();
			showScreen(mMapScreen, SCREEN_FADE_IN_SECONDS);
		}
	}
	else if (theAction == "timeGame")
		startGame(GAME_MODE_TIMED, 0);
	else if (theAction == "quit")
		engine::App::instance()->quit(0);
	else if (theAction == "options")
	{
		if (mOptionsScreen != nullptr)
		{
			mSettings.updateScreen(mOptionsScreen, mManifest.mVersion);
			showScreen(mOptionsScreen, SCREEN_FADE_IN_SECONDS);
		}
	}
	else if (theAction == "showCredits")
		showScreen(mCreditsScreen, SCREEN_FADE_IN_SECONDS);
	else if (theAction == "creditsDone")
		hideScreen(mCreditsScreen, CREDITS_FADE_OUT_SECONDS);
	else if (theAction == "doOptionsDone")
	{
		hideScreen(mOptionsScreen, SCREEN_FADE_OUT_SECONDS);
		if (mGameStage != nullptr)
		{
			mGameStage->acceptScreen();
			if (mGameStage->isWaiting() && mGameStage->getState() != feedingfrenzy::STATE_END_SCREEN &&
			    mGameStage->getState() != feedingfrenzy::STATE_START_SCREEN)
				showScreen(mPauseScreen, SCREEN_FADE_IN_SECONDS);
		}
	}
	else if (theAction == "optionToggleSound")
	{
		mSettings.setSoundEnabled(!mSettings.isSoundEnabled());
		applySettings();
	}
	else if (theAction == "optionToggleMusic")
	{
		mSettings.setMusicEnabled(!mSettings.isMusicEnabled());
		applySettings();
	}
	else if (theAction == "optionToggleFullScreen")
	{
		mSettings.setFullScreen(!mSettings.isFullScreen());
		applySettings();
	}
	else if (theAction == "optionToggleToolTips")
	{
		mSettings.setTipsEnabled(!mSettings.areTipsEnabled());
		applySettings();
	}
	else if (theAction == "optionMouse1" || theAction == "optionMouse2" || theAction == "optionMouse3" ||
	         theAction == "optionMouse4")
	{
		mSettings.setMouseSensitivity(theAction[theAction.size() - 1] - '1');
		applySettings();
	}
	else if (theAction == "pauseResume")
		pauseResume();
	else if (theAction == "pauseOptions")
	{
		hideScreen(mPauseScreen, SCREEN_FADE_OUT_SECONDS);
		showScreen(mOptionsScreen, SCREEN_FADE_IN_SECONDS);
	}
	else if (theAction == "pauseQuit")
	{
		mSession.clearContinues();
		mGameStage->showGameOver();
	}
	else if (theAction == "highScores")
	{
		mHighScoreScreen->setMode(mGameMode);
		mMainMenu->setVisible(false);
		showScreen(mHighScoreScreen, SCREEN_FADE_IN_SECONDS);
	}
	else if (theAction == "highScoreDone")
	{
		mMainMenu->setVisible(true);
		hideScreen(mHighScoreScreen, SCREEN_FADE_OUT_SECONDS);
	}
	else if (theAction == "highScoreNextPage")
	{
		mHighScoreScreen->nextPage();
		mHighScoreScreen->refresh();
	}
	else if (theAction == "highScorePrevPage")
	{
		mHighScoreScreen->prevPage();
		mHighScoreScreen->refresh();
	}
	else if (theAction == "highScoreEntryDone")
	{
		std::string aName = mHighScoreEntryScreen->getEnteredName();
		if (mGameMode == GAME_MODE_NORMAL || mGameMode == GAME_MODE_TIMED)
		{
			ScoreList& aList = getScoreList(mGameMode);
			aList.add(aName, (unsigned int)mSession.getScore());
			if (mScoresLoaded)
				aList.save(mGameMode == GAME_MODE_NORMAL ? NORMAL_SCORES_KEY : TIMED_SCORES_KEY);
		}
		mSettings.setDefaultScoreName(aName);
		mSettings.save();
		removeKeyHandler(mHighScoreEntryScreen);
		hideScreen(mHighScoreEntryScreen, SCREEN_FADE_OUT_SECONDS);
		enterMenu();
	}
	else if (theAction == "gameWonDone")
	{
		hideScreen(mGameWonScreen, SCREEN_FADE_OUT_SECONDS);
		if (!showHighScoreEntry(SCORES_PER_LIST + 1))
			enterMenu();
	}
}
