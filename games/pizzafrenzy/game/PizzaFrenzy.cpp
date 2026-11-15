#include "PizzaFrenzy.h"
#include "App.h"
#include "Build.h"
#include "Host.h"
#include "Players.h"
#include "Random.h"
#include "Scene.h"
#include "Scoreboards.h"
#include "GameText.h"
#include "Sound.h"
#include "SplashState.h"
#include "Transition.h"

#include <ctime>

using namespace pizzafrenzy;

PizzaFrenzy* PizzaFrenzy::sInstance = nullptr;
bool PizzaFrenzy::sDesignerUnlockOverride = false;  // 0x532908

namespace
{
// The string table's texts the configuration reads
const int STRING_PUBLISHER = 0xc9;  // the settings' registry key's publisher
const int STRING_GAME = 0xca;       // its game
const int STRING_TITLE = 0xcb;      // the window's title

// The window (configure)
const int WINDOW_ICON = 0x67;
const int FRAME_RATE = 30;

// The game's code the scoreboards are kept by (init): the text at 0x4f9ae8 of the exe
const unsigned int GAME_CODE_ADDRESS = 0x4f9ae8;

// The splash screen's fade in and out (init)
const float SPLASH_FADE_SECONDS = 0.5f;
}

// 0x40c170: the exe has the engine's signal build (engines/sprout/Build.h), which the engine is told before anything
// of it is made
PizzaFrenzy::PizzaFrenzy() :
	mGameMode(0), mLoadBarWidth(0), mLoadBarHeight(0), mShowHighScores(false), mShowCitySelect(false), mActive(false)
{
	engine::gSignalBuild = true;
}

// 0x40c6e0
PizzaFrenzy::~PizzaFrenzy()
{
	if (sInstance == this)
		sInstance = nullptr;
}

// 0x4050d0: the settings (the registry key Software\<publisher>\<game>) are made and loaded first, for the full
// screen; then the random numbers are seeded with the time
void PizzaFrenzy::configure(engine::AppConfig& theConfig)
{
	engine::App* anApp = engine::App::instance();
	std::string aPublisher = anApp->loadString(STRING_PUBLISHER);
	std::string aGame = anApp->loadString(STRING_GAME);
	mSettings = new Settings(aPublisher, aGame);
	mSettings->load();
	mTitle = anApp->loadString(STRING_TITLE);
	theConfig.mTitle = mTitle.c_str();
	theConfig.mHeight = engine::SCREEN_HEIGHT;
	theConfig.mWidth = engine::SCREEN_WIDTH;
	theConfig.mFullScreen = mSettings->isFullScreen();
	theConfig.mFrameRate = FRAME_RATE;
	theConfig.mIcon = WINDOW_ICON;
	unsigned int aSeed = (unsigned int)time(nullptr);
	engine::getHost()->testSeed(aSeed);
	engine::crtSrand(aSeed);
}

// 0x409220: the display (the application's slot 0x04, in the full screen the settings say: the host has made the
// window by now, in the configuration's mode), the archive, the scene, the sounds and the screen manager; the splash
// screen with its background, a loose file beside the exe, and its load bar, shown as far as the loading state has
// got; the window's signals; and the loading state
void PizzaFrenzy::init()
{
	sInstance = this;
	engine::App* anApp = engine::App::instance();
	anApp->setUseFiles(false);
	anApp->addArchive("PizzaFrenzy.saf");
	mScene = new engine::Scene();
	mScene->connect();
	initSound();
	setSoundEnabled(mSettings->isSoundEnabled());
	mSounds = new engine::SoundMgr();
	mSounds->load("res\\manifests\\soundManifest.xml");
	mScreens = new engine::ScreenMgr();
	mScene->addChild(mScreens);
	// The players (the registry key of the users) and the scoreboards (their key and the game's code). The game's
	// string comes first, then the publisher's. The original has the scoreboards post online too; the port has no
	// online scores and no posting.
	std::string aGame = anApp->loadString(STRING_GAME);
	std::string aPublisher = anApp->loadString(STRING_PUBLISHER);
	engine::Players::instance()->open(aPublisher, aGame);
	engine::Scoreboards::instance()->open(aPublisher, aGame);
	engine::Scoreboards::instance()->setGameCode(GT(GAME_CODE_ADDRESS));
	engine::Ref<engine::Transition> aFade = new engine::FadeTransition(SPLASH_FADE_SECONDS);
	mSplashScreen = new engine::GenericScreen();
	mSplashScreen->loadLayout("res/screenLayouts/splashScreen.xml");
	mSplashScreen->setTransitions(aFade, aFade);
	mSplashScreen->mActionSignal.connect(this, &PizzaFrenzy::onScreenAction);
	engine::Image* aBackground = anApp->getFileImage("splash/splashBg.jpg");
	engine::ImageNode* aBackgroundNode = new engine::ImageNode(aBackground);
	static_cast<engine::Container*>(mSplashScreen->getNode("bgImage"))->addChild(aBackgroundNode);
	mLoadBar = static_cast<engine::ImageNode*>(mSplashScreen->getNode("loadBar"));
	mLoadBar->layout();
	mLoadBarHeight = mLoadBar->getImageHeight();
	mLoadBarWidth = mLoadBar->getImageWidth();
	engine::Rect aClip;
	aClip.set(0, 0, 0, (float)mLoadBarHeight);
	mLoadBar->setClipped(true);
	mLoadBar->setClipRect(aClip);
	anApp->mMaximizeSignal.connect(this, &PizzaFrenzy::onWindowCommand);
	anApp->mMinimizeSignal.connect(this, &PizzaFrenzy::onWindowCommand);
	anApp->mActivateSignal.connect(this, &PizzaFrenzy::onActivate);
	anApp->mUpdateSignal.connect(this, &PizzaFrenzy::onAppUpdate);
	anApp->mTimerSignal.connect(this, &PizzaFrenzy::onTimer);
	mStates.pushState(new SplashState(this));
}

// 0x401df0
void PizzaFrenzy::shutdown()
{
}

// 0x402340: the music's volume, and the game's music player's
void PizzaFrenzy::setMusicEnabled(bool theEnabled)
{
	GenericApplication::setMusicEnabled(theEnabled);
	if (mMusicPlayer != nullptr)
		mMusicPlayer->setEnabled(theEnabled);
}

// 0x401d90: the current state's update, then the sound system's (the frame's milliseconds), then the music player's
void PizzaFrenzy::update(engine::UpdateInfo& theInfo)
{
	mStates.update(theInfo);
	engine::SoundSystem* aSounds = engine::App::getSoundSystem();
	if (aSounds != nullptr)
		aSounds->update((int)(theInfo.mSeconds * 1000.0f));
	if (mMusicPlayer != nullptr)
		mMusicPlayer->update();
}

// 0x401030
void PizzaFrenzy::screenAction(std::string theAction)
{
	mStates.action(theAction);
}

// 0x4059d0
void PizzaFrenzy::windowCommand()
{
	if (mSettings != nullptr && mOptions != nullptr)
	{
		mSettings->setFullScreen(engine::getHost()->isFullScreen());
		mOptions->setupOptions(this, mSettings);
	}
	mStates.action("blur");
}

// 0x405a90: losing the focus pauses a game ("blur")
void PizzaFrenzy::windowActivate(bool theActive)
{
	if (!theActive)
	{
		mActive = false;
		mStates.action("blur");
	}
	else
	{
		mActive = true;
	}
}

// 0x401090
void PizzaFrenzy::setFullScreen(bool theFullScreen)
{
	engine::getHost()->setFullScreen(theFullScreen);
	windowCommand();
}

// 0x4010b0: the sound system's 100 ms update
void PizzaFrenzy::timerTick(int theMs)
{
	engine::SoundSystem* aSounds = engine::App::getSoundSystem();
	if (aSounds != nullptr)
		aSounds->update(theMs);
}

// 0x401010
void PizzaFrenzy::quitApplication()
{
	engine::App::instance()->quit(0);
}

// 0x402370: translated next
void PizzaFrenzy::saveGame()
{
}

// 0x40c700: translated next
void PizzaFrenzy::startGame(int theMode, int theCity)
{
}

// 0x4023d0: translated next
void PizzaFrenzy::showCitySelect()
{
}

// 0x403bc0: translated next
void PizzaFrenzy::advanceLevel()
{
}

// 0x409f30: translated next
void PizzaFrenzy::startLevel()
{
}

// 0x40dab0, 0x402d70: translated next
void PizzaFrenzy::startPizzaDesigner()
{
}

// 0x402e00: translated next
void PizzaFrenzy::startPizzaEditor()
{
}
