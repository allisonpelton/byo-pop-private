#include "PizzaFrenzy.h"
#include "App.h"
#include "Build.h"
#include "Host.h"
#include "Players.h"
#include "Random.h"
#include "Scene.h"
#include "Scoreboards.h"
#include "GameText.h"
#include "BonusStates.h"
#include "DialogState.h"
#include "GameState.h"
#include "ScreenNodes.h"
#include "TextItem.h"
#include "LevelStartState.h"
#include "NewToppingState.h"
#include "PizzaEditorState.h"
#include "StoryState.h"
#include "Sound.h"
#include "SpeedGame.h"
#include "MemoryGame.h"
#include "ConcentrationGame.h"
#include "SplashState.h"
#include "Transition.h"

#include <cctype>
#include <ctime>
#include <stdexcept>

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

// The pause menu's captions (the string table's texts): a level's day and city, a bonus round's day, and the tips
const int STRING_PAUSE_CITY_DAY = 0xe3;
const int STRING_PAUSE_BONUS_DAY = 0xe4;
const int STRING_PAUSE_TIPS = 0xe5;

// A pizza of the player's is a topping of this name with the pizza's number, whose picture (and coupon) are these. It
// has an upgrade for each step from -2 on, while the step plus the combo base is under the end sum: the multiplier is
// the step plus its base, and the combo is the step plus the combo base (the step itself from 0 on) in fives.
const char* const DESIGNED_TOPPING_NAME = "pizza%d";
const char* const DESIGNED_COUPON_IMAGE = "res\\pizza\\couponBlue.jpg";
const int STRING_DESIGNED_UPGRADE = 0xdb;
const int DESIGNED_UPGRADE_FIRST_STEP = -2;
const int DESIGNED_UPGRADE_END_SUM = 4;
const int DESIGNED_MULTIPLIER_BASE = 3;
const int DESIGNED_COMBO_BASE = 2;
const int DESIGNED_COMBO_UNIT = 5;

// The stories of a level's end
const char* const STORY_LEVEL_SUMMARY = "res\\story\\levelSummary.xml";
const char* const STORY_BONUS_SUMMARY = "res\\story\\bonusSummary.xml";
const char* const STORY_GAME_WON = "res\\story\\gameWon.xml";
}

// 0x40c170: the exe has the engine's signal build (engines/sprout/Build.h), which the engine is told before anything
// of it is made
PizzaFrenzy::PizzaFrenzy() :
	mCurrentLevel(0), mCurrentCity(0), mDay(0), mGameMode(0), mLoadBarWidth(0), mLoadBarHeight(0),
	mBonusRound(false), mShowHighScores(false), mShowCitySelect(false), mActive(false)
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

// 0x402370: the mode's saved game gets the game's place (day, level, city), and the game's score, time and toppings
void PizzaFrenzy::saveGame()
{
	ModeRecord& aMode = mUserGameData->mModes[mGameMode];
	aMode.mLastCity = mCurrentCity;
	aMode.mLastLevel = mCurrentLevel;
	aMode.mLastDay = mDay;
	mGameStats->saveTo(mUserGameData, mGameMode);
	mUserGameData->save();
}

// 0x40c700
void PizzaFrenzy::startGame(int theMode, int theCity)
{
	mGameMode = theMode;
	mBonusRound = false;
	mGameStats->reset();
	if (mUserGameData->hasSavedGame(theMode))
	{
		mCurrentCity = mUserGameData->getLastCity(theMode);
		mCurrentLevel = mUserGameData->getLastLevel(theMode);
		mDay = mUserGameData->getLastDay(theMode);
		mGameStats->resume(mUserGameData, theMode);
		startLevel();
	}
	else
	{
		beginCity(theCity);
		startLevel();
	}
}

// 0x403b60
void PizzaFrenzy::beginCity(int theCity)
{
	mCurrentLevel = 0;
	mCurrentCity = theCity;
	mDay = 1;
	for (int i = 0; i < theCity; i++)
		mDay = mDay + (int)mCities[i]->mLevels.size();
}

// 0x4023d0
void PizzaFrenzy::showCitySelect()
{
	mCitySelect->refresh();
	mScreens->showScreen(mCitySelect, 0);
}

// 0x403bc0: the city's levels are counted before the city changes, and the new city is not looked at when the last
// one is done
void PizzaFrenzy::advanceLevel()
{
	mCurrentLevel = mCurrentLevel + 1;
	City* aCity = mCities[mCurrentCity];
	if ((int)aCity->mLevels.size() <= mCurrentLevel)
	{
		mCurrentLevel = 0;
		mCurrentCity = mCurrentCity + 1;
		mUserGameData->setMaxCity(mCurrentCity);
		mUserGameData->save();
		if ((int)mCities.size() <= mCurrentCity)
			return;
	}
	mDay = mDay + 1;
}

// 0x404660, from the level lists' subscript: the exe's checked subscript throws an exception of the standard library
Level* PizzaFrenzy::getCurrentLevel()
{
	City* aCity = mCities[mCurrentCity];
	if ((unsigned int)aCity->mLevels.size() <= (unsigned int)mCurrentLevel)
		throw std::out_of_range("invalid vector<T> subscript");
	return aCity->mLevels[mCurrentLevel];
}

// 0x409f30
void PizzaFrenzy::startLevel()
{
	Level* aLevel = getCurrentLevel();
	std::string aStory = aLevel->getStory(mGameMode);
	bool aShowStory = false;
	if (!aStory.empty())
	{
		if (mSettings->isReplayStory() || mUserGameData->getHighestDay(mGameMode) < mDay)
			aShowStory = true;
	}
	if (!aShowStory)
	{
		if (aLevel->mBonusRound)
			setUpLevel();
		else
			giveToppings();
		return;
	}
	enterStoryState(aLevel, aStory);
}

// 0x408090
void PizzaFrenzy::giveToppings()
{
	Level* aLevel = getCurrentLevel();
	std::vector<engine::Ref<Topping>>& aToppings = aLevel->getToppings();
	for (size_t i = 0; i < aToppings.size(); i++)
		mUserGameData->addNewTopping(aToppings[i]);
	mUserGameData->save();
	if (mUserGameData->getUnlockTopping() && !mUserGameData->getNewToppings().empty())
	{
		enterNewToppingState();
		return;
	}
	showLevelStart();
}

// 0x407fc0
void PizzaFrenzy::showLevelStart()
{
	Level* aLevel = getCurrentLevel();
	if (aLevel->mBonusRound)
	{
		setUpLevel();
		return;
	}
	enterLevelStartState(aLevel);
}

// 0x406dc0
void PizzaFrenzy::setUpLevel()
{
	Level* aLevel = getCurrentLevel();
	mBonusRound = aLevel->mBonusRound;
	mUserGameData->setHighestDay(mGameMode, mDay);
	mUserGameData->save();
	if (mBonusRound)
	{
		enterBonusRound(aLevel);
		return;
	}
	loadCityFile(aLevel->getCityFile(), mGameScreen);
	enterModeGame(aLevel);
}

// 0x409f30 makes the state (0x402ba0) after the story screen's slot 0x17c reads the file
void PizzaFrenzy::enterStoryState(Level* theLevel, const std::string& theStory)
{
	mStoryScreen->setStory(theStory);
	mStates.changeState(new StoryState(this, theLevel));
}

// 0x402ce0: the buttons of the new toppings are made (the screen's slot 0x184), and the state shows the screen
void PizzaFrenzy::enterNewToppingState()
{
	mNewToppingScreen->fillButtons();
	mStates.changeState(new NewToppingState(this));
}

// 0x402c40: the level start screen shows the level (the screen's slot 0x180), and its state starts
void PizzaFrenzy::enterLevelStartState(Level* theLevel)
{
	mLevelStartScreen->setLevel(theLevel);
	mStates.changeState(new LevelStartState(this, theLevel));
}

// 0x4022f0
void PizzaFrenzy::stopGame()
{
	if (mMode != nullptr)
		mMode->clearLevel();
	mGameScreen->clearGroups();
	mCityMap->clear();
	mMode = nullptr;
}

// 0x407080: the pause menu's captions say the day and the city (the bonus round has no city), and the tips the
// player has; the menu is shown, and the state that paused comes back when it closes
void PizzaFrenzy::pauseGame(engine::State* theNextState)
{
	engine::TextItem* aCaption = static_cast<engine::TextItem*>(mPauseMenu->getNode("pauseCaption1"));
	engine::TextItem* aTips = static_cast<engine::TextItem*>(mPauseMenu->getNode("pauseCaption2"));
	engine::App* anApp = engine::App::instance();
	if (!getCurrentLevel()->mBonusRound)
		aCaption->setText(formatMessage(anApp->loadString(STRING_PAUSE_CITY_DAY), std::to_string(mDay),
			mCities[mCurrentCity]->getName()));
	else
		aCaption->setText(formatMessage(anApp->loadString(STRING_PAUSE_BONUS_DAY), std::to_string(mDay)));
	aTips->setNumber(mGameStats->mTipMoney + mGameStats->mScore, anApp->loadString(STRING_PAUSE_TIPS));
	mStates.changeState(new DialogState(this, mPauseMenu, theNextState));
}

// 0x407370: the level is over: the game is let go, the summary is set up from the stats, and the game goes on to the
// next level; the summary's state follows, or the game's end after the last level of the last city
void PizzaFrenzy::endLevel()
{
	Level* aLevel = getCurrentLevel();
	mStoryScreen->setLevel(aLevel);
	stopGame();
	if (!mBonusRound)
	{
		mStoryScreen->setStory(STORY_LEVEL_SUMMARY);
		mStoryScreen->setUpSummary();
	}
	else
	{
		mStoryScreen->setStory(STORY_BONUS_SUMMARY);
		mStoryScreen->setUpBonusSummary();
	}
	advanceLevel();
	if (mCurrentCity < (int)mCities.size())
	{
		mStates.changeState(new LevelSummaryState(this, aLevel));
		return;
	}
	mUserGameData->clearSavedGame(mGameMode);
	mStoryScreen->setStory(STORY_GAME_WON);
	mStates.changeState(new StoryState(this, nullptr));
}

// 0x406dc0's second half: the mode's game is made (the mode number picks it) and plays the level
void PizzaFrenzy::enterModeGame(Level* theLevel)
{
	if (mGameMode == MODE_SPEED)
		mMode = new SpeedGame();
	else if (mGameMode == MODE_MEMORY)
		mMode = new MemoryGame();
	else
		mMode = new ConcentrationGame();
	mMode->setup(mGameScreen, mCityMap, mHud, mGameStats);
	mMode->setLevel(theLevel);
	mStates.changeState(new GameState(this));
}

// 0x45f830, 0x402a40: the bonus round's game takes the decorating screen and the stats, then the level; the state
// shows the screen
void PizzaFrenzy::enterBonusRound(Level* theLevel)
{
	mMode = new BonusGame();
	static_cast<BonusGame*>(mMode.get())->setStats(mDecorateScreen, mGameStats);
	mMode->setLevel(theLevel);
	mStates.changeState(new BonusGameState(this));
}

// 0x4099b0
void PizzaFrenzy::addPizza(Pizza* thePizza, bool theDesigned)
{
	if (theDesigned)
	{
		addDesignedPizza(thePizza);
		return;
	}
	// the pizza goes in the list when it is not there, and is kept by its number
	bool aKept = false;
	for (size_t i = 0; i < mPizzas.size(); i++)
		if (mPizzas[i] == thePizza)
			aKept = true;
	if (!aKept)
		mPizzas.push_back(thePizza);
	mPizzaById[thePizza->mId] = thePizza;
}

// 0x40dab0, 0x402d70: the city editor (editorTiles, editorExit) of the original's development; the button that opens
// it is hidden and disabled by the main menu, and nothing else starts it
void PizzaFrenzy::startPizzaDesigner()
{
}

// 0x40c7a0, 0x4084b0
void PizzaFrenzy::startPizzaEditor(bool theFromNewTopping)
{
	mPizzaEditor->show();
	mStates.changeState(new PizzaEditorState(this, theFromNewTopping));
}

// 0x4099b0, the player's pizzas: the pizza is in their list once. Its topping, made the first time with the name
// "pizza" and the pizza's number, has the pizza's picture and the pizza's name in capitals, and four upgrades, each
// with the same text, the multipliers 1 to 4 and the combos 0, 5, 10 and 20; the rest of the manifest's pizzas'
// bookkeeping does not apply to it.
void PizzaFrenzy::addDesignedPizza(Pizza* thePizza)
{
	bool aKept = false;
	for (size_t i = 0; i < mDesignedPizzas.size(); i++)
		if (mDesignedPizzas[i] == thePizza)
			aKept = true;
	if (!aKept)
		mDesignedPizzas.push_back(thePizza);
	engine::Ref<engine::Image> aPicture = thePizza->makeImage();
	engine::Image* aCouponImage = engine::App::instance()->getImage(DESIGNED_COUPON_IMAGE);
	std::string aDisplayName = thePizza->mName;
	for (size_t i = 0; i < aDisplayName.size(); i++)
		aDisplayName[i] = (char)std::toupper((unsigned char)aDisplayName[i]);
	engine::Ref<Topping> aTopping;
	if (mPizzaToppings.find(thePizza->mId) != mPizzaToppings.end())
	{
		aTopping = mPizzaToppings[thePizza->mId];
		aTopping->setImage(aPicture);
		aTopping->mDisplayName = aDisplayName;
	}
	else
	{
		aTopping = new Topping(formatText(DESIGNED_TOPPING_NAME, thePizza->mId), aDisplayName, aPicture,
		                       aCouponImage, 1);
		mPizzaToppings[thePizza->mId] = aTopping;
	}
	aTopping->mUpgrades.clear();
	int aStep = DESIGNED_UPGRADE_FIRST_STEP;
	do
	{
		int aCombo = (aStep + DESIGNED_COMBO_BASE + (aStep < 0 ? 0 : aStep)) * DESIGNED_COMBO_UNIT;
		aTopping->mUpgrades.push_back(new Upgrade(thePizza->mName,
			engine::App::instance()->loadString(STRING_DESIGNED_UPGRADE), aStep + DESIGNED_MULTIPLIER_BASE, aCombo));
		aStep++;
	} while (aStep + DESIGNED_COMBO_BASE < DESIGNED_UPGRADE_END_SUM);
}
