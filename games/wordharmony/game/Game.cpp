#include "Game.h"
#include "Button.h"
#include "FadeContainer.h"
#include "GameText.h"
#include "Host.h"
#include "ImageNode.h"
#include "Hud.h"
#include "Music.h"
#include "Random.h"
#include "SavedGame.h"
#include "Screens.h"
#include "Selector.h"
#include "Sound.h"
#include "SoundBank.h"
#include "Stream.h"
#include "TextEffects.h"
#include "TextItem.h"
#include "WordBallSystem.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace engine;

Game* Game::sInstance = nullptr;

namespace
{

// 0x401200: a screen faded out goes from the root window at the next update (a fade's callback)
void removeWhenFaded(void* theNode)
{
	Game::instance()->mRemoveNext = static_cast<Node*>(theNode);
}

// 0x401220: a screen faded in becomes the root window's only child at the next update (a fade's callback)
void makeOnlyScreen(void* theNode)
{
	Game::instance()->mNextRoot = static_cast<Node*>(theNode);
}

// 0x421b50: a number with commas between the thousands (a minus sign is not counted as a digit, unlike 0x4279f0's)
std::string formatNumber(int theNumber)
{
	char aBuffer[20];
	std::sprintf(aBuffer, "%d", theNumber);
	int aLast = (int)std::strlen(aBuffer) - 1;
	int aCommas;
	if (theNumber < 0)
		aCommas = (aLast - 1) / 3;
	else
		aCommas = aLast / 3;
	int aNewLast = aLast + aCommas;
	int aDigits = 0;
	int aDest = aNewLast;
	if (-1 < aDest)
	{
		const char* aSource = aBuffer + aLast;
		do
		{
			if (aDigits == 3 && 0 < aDest)
			{
				aBuffer[aDest] = ',';
				aDest--;
				aDigits = 0;
			}
			aDest--;
			aBuffer[aDest + 1] = *aSource;
			aSource--;
			aDigits++;
		} while (-1 < aDest);
	}
	aBuffer[aNewLast + 1] = '\0';
	return std::string(aBuffer);
}

// The fade the screens use (0.3 seconds, to full opacity)
const float SCREEN_FADE = 0.3f;

}

// 0x406980 (the port starts the pointers at null; the original leaves most of them to init)
Game::Game()
	: mScore(0), mTime(0), mLevelPoints(0), mLettersLeft(0), mLettersUsed(0), mBonus(0), mWildcards(0), mHints(0),
	  mHintsUsed(0), mLevelHintsUsed(0), mCountUp(0), mContinue(false), mTextEffects(nullptr), mEditorHud(nullptr),
	  mGridSnap(false), mLevelTime(0), mNextRoot(nullptr), mPuzzleMusic(nullptr), mActionMusic(nullptr),
	  mGameOverMusic(nullptr), mMenuMusic(nullptr), mMusic(nullptr), mState(0), mNextState(9), mTimer(0),
	  mSplashTime(0), mSplash(nullptr), mMainMenu(nullptr), mOptions(nullptr), mHighScoreEntry(nullptr),
	  mSaverMode(0), mScreenSaver(nullptr), mShowNext(nullptr), mRemoveNext(nullptr), mHighScoreScreen(nullptr),
	  mLevelEnd(nullptr), mPausedScreen(nullptr), mGameOverScreen(nullptr), mCongratsScreen(nullptr),
	  mLevelsSelection(nullptr), mDifficultySelection(nullptr), mSolitaireSelection(nullptr), mPuzzleHelp(nullptr),
	  mActionHelp(nullptr), mSolitaireHelp(nullptr), mPuzzleBrief(nullptr), mActionBrief(nullptr),
	  mSolitaireBrief(nullptr), mNewGameScreen(nullptr), mQuitGameScreen(nullptr), mLoadScreen(nullptr),
	  mContinueText(nullptr), mContinueButton(nullptr), mScoreText(nullptr), mQuitting(false), mLevel(0),
	  mCurrentLevel(nullptr), mBaseLevel(nullptr), mSavedGame(nullptr), mTemplate(0), mBallSystem(nullptr),
	  mGameScreen(nullptr), mMode(0), mRetrying(false), mSounds(nullptr)
{
	for (int i = 0; i < 5; i++)
		mLoadTexts[i] = nullptr;
}

// 0x4073f0: the members' destructors only
Game::~Game()
{
}

// 0x402eb0
void Game::configure(AppConfig& theConfig)
{
	mSettings.load();
	theConfig.mTitle = "Word Harmony";
	theConfig.mIcon = 101;
	theConfig.mFrameRate = 30;
	theConfig.mWidth = 800;
	theConfig.mHeight = 600;
	theConfig.mUnknown9C = false;
	sInstance = this;
	readCommandLine();
	// As the screen saver (/s or /c) the original makes the registry's SaverInstallPath (under HKEY_LOCAL_MACHINE)
	// the current directory, and throws when the key or the path is missing; the host finds the game's files itself
	theConfig.mUseFiles = false;
	App::instance()->setUseFiles(false);
	App::instance()->addArchive("WHArchive.saf");
}

// 0x406b30
void Game::init()
{
	mSplash = nullptr;
	mMainMenu = nullptr;
	mOptions = nullptr;
	mHighScoreScreen = nullptr;
	mLevelEnd = nullptr;
	mPausedScreen = nullptr;
	mGameOverScreen = nullptr;
	mCongratsScreen = nullptr;
	mHighScoreEntry = nullptr;
	mLevelsSelection = nullptr;
	mDifficultySelection = nullptr;
	mSolitaireSelection = nullptr;
	mPuzzleHelp = nullptr;
	mActionHelp = nullptr;
	mSolitaireHelp = nullptr;
	mPuzzleBrief = nullptr;
	mActionBrief = nullptr;
	mSolitaireBrief = nullptr;
	mNewGameScreen = nullptr;
	mQuitGameScreen = nullptr;
	mMusic = nullptr;
	mSounds = new SoundBank();
	mShowNext = nullptr;
	mRemoveNext = nullptr;
	mNextRoot = nullptr;
	mLevel = 0;
	crtSrand(App::instance()->getTime());  // GetTickCount()
	// The sound system of the window (DirectSound, 0x443ca0)
	App::instance()->setSoundSystem(getHost()->createSoundSystem());
	// (the beta's play time upload is given its URL here: left out)
	mScore = 0;
	mTime = 0;
	mWildcards = 0;
	mHints = 0;
	mHintsUsed = 0;
	mLevelHintsUsed = 0;
	mLongestWords.clear();
	if (mSaverMode != 0)
	{
		App::instance()->getRootWindow();
		mNextState = 9;
		startScreenSaver();
		mState = 8;
		mTimer = 3.0f;
		return;
	}
	setState(0);
}

// 0x401750
void Game::shutdown()
{
	if (mSaverMode == 0)
	{
		mSettings.save();
		delete mSounds;
	}
}

// 0x4070a0
void Game::activate(bool theActive)
{
	if (mState == 5 && !theActive)
		pause(true);
}

// 0x401090
void Game::makeSplash()
{
	if (mSplash == nullptr)
	{
		mSplash = new Selector();
		mSplash->addRef();
		Image* anImage = App::instance()->getFileImage("res\\splash\\sproutSplash.jpg");
		mSplash->addChild(new ImageNode(anImage));
	}
}

// 0x401170
void Game::switchScreens()
{
	Window* aRoot = App::instance()->getRootWindow();
	if (mShowNext != nullptr)
	{
		aRoot->addChild(mShowNext);
		mShowNext->setVisible(false);
		mShowNext->fade(true, SCREEN_FADE, 1.0f);
		mShowNext->clearCallback();
		mShowNext = nullptr;
	}
	if (mRemoveNext != nullptr)
	{
		aRoot->removeChild(mRemoveNext);
		mRemoveNext = nullptr;
	}
}

// 0x401240
void Game::endProgram()
{
	App::instance()->quit(0);
}

// 0x401260
void Game::addScore(int thePoints)
{
	int aScore = mScore + thePoints;
	mLevelPoints += thePoints;
	mScore = aScore;
	mScoreText->setNumber(aScore);
}

// 0x4012b0
void Game::pointsShown(int thePoints)
{
	Game* aGame = sInstance;
	aGame->mScore += thePoints;
	aGame->mLevelPoints += thePoints;
	aGame->mScoreText->setNumber(aGame->mScore);
}

// 0x4012e0
void Game::applyVolumes()
{
	SoundSystem* aSystem = App::getSoundSystem();
	if (aSystem != nullptr)
	{
		if (!mSettings.isSoundEnabled())
			aSystem->setCategoryVolume(0, 0.0f);
		else
			aSystem->setCategoryVolume(0, 1.0f);
		if (mSettings.isMusicEnabled())
		{
			aSystem->setCategoryVolume(1, 1.0f);
			return;
		}
		aSystem->setCategoryVolume(1, 0.0f);
	}
}

// 0x401340
HighScores* Game::getHighScores(int theMode)
{
	if (theMode == 0)
		return &mPuzzleScores;
	if (theMode != 1)
		return &mActionScores;
	return &mSolitaireScores;
}

// 0x401370
void Game::setActionLevel(int theLevel)
{
	float aDelay = 8.0f / (float)std::pow((double)(theLevel + 1), 0.5);
	int aSize;
	if (theLevel < 1)
		aSize = 1;
	else if (theLevel < 4)
		aSize = 2;
	else
		aSize = (7 < theLevel) + 3;
	mCurrentLevel->mBallCriteria = theLevel * 8 + 20;
	mCurrentLevel->mSpawnDelay = aDelay;
	mCurrentLevel->mMaxBalls = 30;
	mCurrentLevel->mLongestPiece = aSize;
	mCurrentLevel->mBalls = 5;
}

// 0x401410
void Game::setSolitaireLevel(int theLevel)
{
	int aBalls = theLevel + 4;
	if (49 < aBalls)
		aBalls = 50;
	mCurrentLevel->mBallCriteria = aBalls;
	mCurrentLevel->mMaxBalls = aBalls;
	mCurrentLevel->mLongestPiece = 3;
	mCurrentLevel->mSpawnDelay = 0.1f;
	mCurrentLevel->mBalls = aBalls;
	mCurrentLevel->mPieces = aBalls / 10 + 1;
}

// 0x401490
void Game::showModeInfo(int theMode)
{
	if (theMode == 0)
		mShowNext = mPuzzleBrief;
	else if (theMode == 1)
		mShowNext = mSolitaireBrief;
	else if (theMode == 2)
		mShowNext = mActionBrief;
}

// 0x4014d0
void Game::hideModeInfo(int theMode)
{
	if (theMode == 0)
	{
		mPuzzleBrief->fade(false, SCREEN_FADE, 1.0f);
		mPuzzleBrief->setCallback(&removeWhenFaded, mPuzzleBrief);
	}
	else if (theMode == 1)
	{
		mSolitaireBrief->fade(false, SCREEN_FADE, 1.0f);
		mSolitaireBrief->setCallback(&removeWhenFaded, mSolitaireBrief);
	}
	else if (theMode == 2)
	{
		mActionBrief->fade(false, SCREEN_FADE, 1.0f);
		mActionBrief->setCallback(&removeWhenFaded, mActionBrief);
	}
}

// 0x401580
bool Game::isPaused()
{
	return mState == 4;
}

// 0x401590: the screen saver's arguments: /s shows it, /c its settings, /p or /l the preview (the window handles
// after /c and /p are read and not used)
void Game::readCommandLine()
{
	mSaverMode = 0;
	std::string aLine = getHost()->commandLine();
	const char* aStr = aLine.c_str();
	char c = *aStr;
	if (c == '"')
	{
		c = aStr[1];
		while (aStr++, c != '\0')
		{
			if (c == '"')
				goto skipped;
			c = aStr[1];
		}
	}
	else
	{
		while (c != '\0')
		{
			if (c == ' ')
				goto skipped;
			aStr++;
			c = *aStr;
		}
	}
	goto arguments;
skipped:
	if (*aStr != '\0')
		aStr++;
arguments:
	c = *aStr;
	while (c == ' ')
	{
		aStr++;
		c = *aStr;
	}
	if (*aStr == '-' || *aStr == '/')
		aStr++;
	c = *aStr;
	if (c != 'p' && c != 'P' && c != 'l' && c != 'L')
	{
		if (c == 's' || c == 'S')
		{
			mSaverMode = 1;
			return;
		}
		if (c != 'c' && c != 'C')
			return;
		mSaverMode = 2;
		return;
	}
	mSaverMode = 3;
}

// 0x401790
void Game::showMainMenu()
{
	Window* aRoot = App::instance()->getRootWindow();
	aRoot->addChild(mMainMenu);
	mMainMenu->setOpacity(0.0f);
	mMainMenu->fade(true, SCREEN_FADE, 1.0f);
	mMainMenu->setCallback(&makeOnlyScreen, mMainMenu);
	mOptions->setCallback(&removeWhenFaded, mOptions);
	if (mMusic != nullptr)
		mMusic->stop();
	mMusic = mMenuMusic;
	mMenuMusic->start();
}

// 0x401820
void Game::showGameScreen()
{
	Window* aRoot = App::instance()->getRootWindow();
	if (mMusic != nullptr)
		mMusic->stop();
	if (-1 < mMode)
	{
		if (mMode < 2)
			mMusic = mPuzzleMusic;
		else if (mMode == 2)
			mMusic = mActionMusic;
	}
	aRoot->addChild(mGameScreen);
	mGameOverScreen->setVisible(false);
	mCongratsScreen->setVisible(false);
	mGameScreen->setVisible(false);
	mGameScreen->fade(true, 0.5f, 1.0f);
	mGameScreen->setCallback(&makeOnlyScreen, mGameScreen);
}

// 0x4018f0
void Game::showGameOver()
{
	if (mMode == 0 && mLevel == (int)mLevels.size())
	{
		mCongratsScreen->showResults();
		mCongratsScreen->fade(true, SCREEN_FADE, 1.0f);
		mSettings.mSaverEnabled = true;
		mSettings.save();
		mSettings.load();
		mMusic = mGameOverMusic;
		mGameOverMusic->start();
		return;
	}
	mGameOverScreen->showResults();
	mGameOverScreen->fade(true, SCREEN_FADE, 1.0f);
	mMusic = mGameOverMusic;
	mGameOverMusic->start();
}

// 0x401cb0
void Game::setLoadText(std::string theText, int theIndex)
{
	mLoadTexts[theIndex]->setText(theText);
	mLoadTexts[theIndex]->setVisible(true);
}

// 0x401d20 (randomInt(0, count) inlined)
void Game::makeActionLevel(int theLevel, int theTemplate)
{
	if (theTemplate < 0)
		theTemplate = randomInt(0, (int)mActionLevels.size());
	mTemplate = theTemplate;
	Level* aTemplate = mActionLevels[theTemplate];
	mCurrentLevel = new Level(*aTemplate);
	setActionLevel(theLevel);
}

// 0x401de0 (randomInt(0, count) inlined)
void Game::makeSolitaireLevel(int theLevel, int theTemplate)
{
	if (theTemplate < 0)
		theTemplate = randomInt(0, (int)mSolitaireLevels.size());
	mTemplate = theTemplate;
	Level* aTemplate = mSolitaireLevels[theTemplate];
	mCurrentLevel = new Level(*aTemplate);
	setSolitaireLevel(theLevel);
}

// 0x401f00: the editor's branches (the templates themselves, never in this build) are kept
void Game::chooseLevel(int theTemplate)
{
	if (mLevel < 0)
		mLevel = 0;
	mSettings.setHighestLevel(mMode, mLevel + 1);
	int aMode = mMode;
	if (aMode == 0)
	{
		if (mLevels.size() <= (unsigned int)mLevel)
			mLevel = (int)mLevels.size() - 1;
		mCurrentLevel = mLevels[mLevel];
	}
	else if (aMode == 1)
	{
		if (mBallSystem->mEditor)
		{
			if (mSolitaireLevels.size() <= (unsigned int)mLevel)
				mLevel = (int)mSolitaireLevels.size() - 1;
			mCurrentLevel = mSolitaireLevels[mLevel];
			return;
		}
		makeSolitaireLevel(mLevel, theTemplate);
	}
	else if (aMode == 2)
	{
		if (mBallSystem->mEditor)
		{
			if (mActionLevels.size() <= (unsigned int)mLevel)
				mLevel = (int)mActionLevels.size() - 1;
			mCurrentLevel = mActionLevels[mLevel];
			return;
		}
		makeActionLevel(mLevel, theTemplate);
	}
}

// 0x402980: a letter at a time taken from a random place of what is left (randomInt(0, size) inlined)
std::string Game::shuffle(std::string theLetters)
{
	std::string aResult;
	while (theLetters.size() != 0)
	{
		unsigned int anIndex = (unsigned int)randomInt(0, (int)theLetters.size());
		aResult.append(1, theLetters.at(anIndex));
		theLetters.erase(anIndex, 1);
	}
	return aResult;
}

// 0x402b10
void Game::playSound(std::string theName)
{
	mSounds->play(theName, 1.0f, 1.0f);
}

// 0x403290
void Game::makeOptions()
{
	if (mOptions == nullptr)
	{
		mOptions = new ScreenLayout();
		mOptions->loadLayout("config\\optionScreen.xml");
		mOptions->addRef();
		mOptions->setListener(this);
		mSettings.setScreen(mOptions);
		applyVolumes();
	}
}

// 0x403370: the original says why in a message box before it ends
void Game::startScreenSaver()
{
	const char* aText;
	if (!mSettings.mSaverEnabled)
	{
		aText = "To enable this screen saver, you must first complete all Relax mode puzzles.";
	}
	else
	{
		if (mSaverMode == 1)
		{
			mScreenSaver = new ScreenSaver();
			mScreenSaver->loadLayout("config\\screensaver.xml");
			App::instance()->getRootWindow()->addChild(mScreenSaver);
			mScreenSaver->setEnabled(false);
			getHost()->setFullScreen(true);
			mScreenSaver->fade(true, SCREEN_FADE, 1.0f);
			return;
		}
		if (mSaverMode != 2)
			goto end;
		aText = "This screen saver has no options that you can set.";
	}
	getHost()->showMessage("Word Harmony", aText);
end:
	App::instance()->quit(0);
}

// 0x4034c0
void Game::loadSounds()
{
	if (mSounds == nullptr)
		mSounds = new SoundBank();
	mSounds->load("config\\globalSounds.xml");
	mPuzzleMusic = new MusicSystem();
	mPuzzleMusic->load("config\\music.xml");
	mActionMusic = new MusicSystem();
	mActionMusic->load("config\\drums.xml");
	mGameOverMusic = new MusicSystem();
	mGameOverMusic->load("config\\game-over-music.xml");
	mMenuMusic = new MusicSystem();
	mMenuMusic->load("config\\menuMusic.xml");
}

// 0x4036c0
void Game::clearLoadTexts()
{
	TextItem** aText = mLoadTexts;
	int i = 5;
	do
	{
		(*aText)->setText("");
		aText++;
		i--;
	} while (i != 0);
}

// 0x403760: the names are the original's texts
void Game::addDefaultScores()
{
	HighScores* aTable = &mPuzzleScores;
	if ((int)aTable->getCount() < 5)
	{
		aTable->add(GT(0x480584), 1000);
		aTable->add(GT(0x48057c), 2000);
		aTable->add(GT(0x480574), 3000);
		aTable->add(GT(0x48056c), 4000);
		aTable->add(GT(0x480564), 5000);
	}
	aTable = &mSolitaireScores;
	if ((int)aTable->getCount() < 5)
	{
		aTable->add(GT(0x48055c), 1000);
		aTable->add(GT(0x480554), 2000);
		aTable->add(GT(0x48054c), 3000);
		aTable->add(GT(0x480544), 4000);
		aTable->add(GT(0x48053c), 5000);
	}
	aTable = &mActionScores;
	if ((int)aTable->getCount() < 5)
	{
		aTable->add(GT(0x480534), 1000);
		aTable->add(GT(0x48052c), 2000);
		aTable->add(GT(0x480524), 3000);
		aTable->add(GT(0x48051c), 4000);
		aTable->add(GT(0x480514), 5000);
	}
}

// 0x403a30: "complete" outside freematch, "up" in it
void Game::showLevelEnd()
{
	bool anUp = mMode == 2;
	mLevelEnd->getNode("complete")->setVisible(!anUp);
	mLevelEnd->getNode("up")->setVisible(anUp);
	mLevelEnd->setVisible(true);
	mLevelEnd->fade(true, SCREEN_FADE, 1.0f);
}

// 0x403b30
void Game::makeGameScreen()
{
	mGameScreen = new FadeContainer();
	mGameScreen->addRef();
	ScreenLayout* aBackground = new ScreenLayout();
	aBackground->loadLayout("config\\bgLayout.xml");
	mGameScreen->addChild(aBackground);
	mBallSystem = new WordBallSystem(mMode);
	mBallSystem->setPosition(211.0f, 108.0f);
	mGameScreen->addChild(mBallSystem);

	mLoadScreen = new ScreenLayout();
	mLoadScreen->loadLayout("config\\loadScreen.xml");
	mLoadScreen->addRef();
	mLoadScreen->setListener(this);
	TextItem** aText = mLoadTexts;
	unsigned int i = 0;
	do
	{
		i++;
		char aName[16];
		std::sprintf(aName, "text%d", i);
		*aText = static_cast<TextItem*>(mLoadScreen->getNode(aName));
		aText++;
	} while (i < 5);
	mContinueText = mLoadScreen->getNode("continue");
	mContinueButton = mLoadScreen->getNode("continueBtn");
	mScoreText = static_cast<TextItem*>(mLoadScreen->getNode("score"));
	mGameScreen->addChild(mLoadScreen);

	mLevelEnd = new ScreenLayout();
	mLevelEnd->addRef();
	mLevelEnd->loadLayout("config\\levelEndScreen.xml");
	mLevelEnd->setPosition(210.0f, 90.0f);
	mGameScreen->addChild(mLevelEnd);
	mLevelEnd->setVisible(false);

	mGameOverScreen = new GameOverScreen();
	mGameOverScreen->addRef();
	mGameOverScreen->loadLayout("config\\gameOverScreen.xml");
	mGameOverScreen->setPosition(210.0f, 90.0f);
	mGameScreen->addChild(mGameOverScreen);
	mGameOverScreen->setVisible(false);

	mCongratsScreen = new CongratsScreen();
	mCongratsScreen->addRef();
	mCongratsScreen->loadLayout("config\\congrats.xml");
	mCongratsScreen->setPosition(210.0f, 90.0f);
	mGameScreen->addChild(mCongratsScreen);
	mCongratsScreen->setVisible(false);

	mPausedScreen = new ScreenLayout();
	mPausedScreen->addRef();
	mPausedScreen->loadLayout("config\\gamePausedScreen.xml");
	mPausedScreen->setPosition(210.0f, 90.0f);
	mGameScreen->addChild(mPausedScreen);
	mPausedScreen->setVisible(false);

	mTextEffects = new TextEffects();
	mTextEffects->load("config\\splats.xml");
	mTextEffects->addRef();
	mGameScreen->addChild(mTextEffects);
}

// 0x404170
void Game::showLoadScreen(bool theShow, bool theAtOnce)
{
	mBallSystem->mHud->setDefinition("");
	mLoadScreen->setEnabled(theShow);
	mContinueButton->setEnabled(theShow);
	mBallSystem->mHud->setControlsEnabled(!theShow, true, false);
	if (theAtOnce)
	{
		mContinueText->setVisible(theShow);
		return;
	}
	// (a layout's group is a FadeContainer)
	static_cast<FadeContainer*>(mContinueText)->fade(theShow, 0.5f, 1.0f);
}

// 0x404220
void Game::resume()
{
	mState = 5;
	mBallSystem->mHud->setPlaying(true);
	WordBallSystem* aSystem = mBallSystem;
	if (aSystem->mHints.size() != 0 && (mMode == 2 || (mMode == 1 && mHints != 0)))
		aSystem->mHud->setHintReady(true);
	mContinue = false;
	applyVolumes();
	if (mPausedScreen->isVisible())
	{
		mPausedScreen->fade(false, SCREEN_FADE, 1.0f);
		showLoadScreen(false, false);
	}
	if (mMode == 2 && mBallSystem->mWord.size() != 0)
		mBallSystem->mHud->showBonusWord();
	mBallSystem->mLayer->fade(true, SCREEN_FADE, 1.0f);
}

// 0x404320: VisitURL opened when it is a web address
void Game::openLink()
{
	std::string aURL = mSettings.mVisitURL;
	if (aURL.size() < 7)
		return;
	if (aURL.substr(0, 7).compare("http://") != 0)
	{
		std::string aStart = aURL.substr(0, 8);
		if (aStart.compare("https://") != 0)
		{
			getHost()->showMessage("Bad URL", "Bad URL");
			return;
		}
	}
	getHost()->openURL(aURL);
}

// 0x4047d0
void Game::startLevel()
{
	mLevelPoints = 0;
	mLettersLeft = mCurrentLevel->mBallCriteria;
	mLettersUsed = 0;
	mLevelHintsUsed = 0;
	mHints = mHints + 1;
	mLoadScreen->setEnabled(false);
	mBallSystem->mHud->setDefinition("");
	mLoadScreen->setEnabled(false);
	mContinueButton->setEnabled(false);
	mBallSystem->mHud->setControlsEnabled(true, true, false);
	mContinueText->setVisible(false);
	mBallSystem->mHud->setControlsEnabled(false, true, true);
	clearLoadTexts();
	mLevelEnd->fade(false, SCREEN_FADE, 1.0f);
	char aText[64];
	std::sprintf(aText, GT(0x4806a0).c_str(), mLevel + 1);
	setLoadText(aText, 0);
	int i = 0;
	do
	{
		std::string& anIntro = mCurrentLevel->mIntro[i];
		if (anIntro.size() != 0)
			setLoadText(anIntro, i + 1);
		i++;
	} while (i < 3);
	mBallSystem->addHud();
	mMusic->stop();
	if (mMode == 2)
		mMusic->select(0, false);
	mMusic->start();
	playSound("LEVEL_INTRO");
}

// 0x4049b0
void Game::endLevel()
{
	mMusic->stop();
	mBallSystem->reset();
	playSound(mMode == 2 ? "LEVEL_UP" : "LEVEL_COMPLETE");
	mLoadScreen->setEnabled(true);
	mBallSystem->mHud->setControlsEnabled(false, true, false);
	clearLoadTexts();
	showLevelEnd();
	if (0 < mMode && mMode < 3)
	{
		char aText[128];
		std::string aWord = mBallSystem->mHud->longestWord();
		std::sprintf(aText, GT(0x480704).c_str(), aWord.c_str());
		setLoadText(aText, 1);
		mLoadTexts[1]->setVisible(false);
		int aWordBonus = (int)aWord.size() * 300;
		std::sprintf(aText, GT(0x4806e0).c_str(), (int)aWord.size(), formatNumber(aWordBonus).c_str());
		setLoadText(aText, 2);
		mLoadTexts[2]->setVisible(false);
		int aHintBonus = mLevelHintsUsed * -500;
		std::sprintf(aText, GT(0x4806c4).c_str(), mLevelHintsUsed, formatNumber(aHintBonus).c_str());
		setLoadText(aText, 3);
		mLoadTexts[3]->setVisible(false);
		int aBonus = aWordBonus + aHintBonus;
		if (aBonus < 1)
			aBonus = 0;
		mBonus = aBonus;
		std::sprintf(aText, GT(0x4806ac).c_str(), formatNumber(aBonus).c_str());
		setLoadText(aText, 4);
		mLoadTexts[4]->setVisible(false);
	}
	mBallSystem->mLayer->fade(false, 2.0f, 1.0f);
}

// 0x404db0
void Game::endGame()
{
	mMusic->stop();
	mBallSystem->reset();
	clearLoadTexts();
	showGameOver();
	mBallSystem->mLayer->fade(false, 3.0f, 1.0f);
	showLoadScreen(true, false);
}

// 0x404e10
void Game::gameOver(bool theQuit, bool theSave)
{
	mQuitting = theQuit;
	mContinue = false;
	if (!theQuit)
	{
		mSettings.setSavedGame(mMode, nullptr);
		playSound("GAME_OVER");
		if (mMode == 0 && mLevel == (int)mLevels.size() - 1)
			mLevel = mLevel + 1;
		mNextState = 6;
		return;
	}
	if (theSave)
	{
		SavedGame* aGame = mSettings.newSavedGame(mMode);
		mBallSystem->saveState(aGame);
		aGame->mWildcards = mWildcards;
		aGame->mHints = mHints;
		aGame->mHintsUsed = mHintsUsed;
		aGame->mLevelHintsUsed = mLevelHintsUsed;
		aGame->mLettersUsed = mLettersUsed;
		aGame->mLevelPoints = mLevelPoints;
		aGame->mTemplate = mTemplate;
		aGame->mTime = (int)((double)mTime + mLevelTime);
		endGame();
		mSettings.save();
		mSettings.load();
		mNextState = 1;
		return;
	}
	mSettings.setSavedGame(mMode, nullptr);
	mNextState = 6;
}

// 0x404f60: hint words drawn until one has theMin to theMax letters (randomInt(0, count) inlined; at() throws for
// an empty list, as the original's check does)
std::string Game::makeLetters(unsigned int theMin, unsigned int theMax)
{
	std::string aWord;
	while (!(theMin <= aWord.size() && aWord.size() <= theMax))
	{
		aWord.erase();
		unsigned int anIndex = (unsigned int)randomInt(0, (int)mHintWords.size());
		aWord = mHintWords.at(anIndex);
	}
	return aWord;
}

// 0x4050f0: under 4, the start of a longer word's shuffle; else whole words until they make exactly theCount
std::string Game::randomLetters(int theCount)
{
	std::string aLetters;
	if (theCount < 4)
	{
		std::string aWord = makeLetters(4, 10);
		aLetters = shuffle(aWord).substr(0, theCount);
	}
	else
	{
		int aLeft;
		do
		{
			aLetters.erase();
			aLeft = theCount;
			while (0 < aLeft)
			{
				std::string aWord = makeLetters(4, 7);
				aLeft -= (int)aWord.size();
				aLetters.append(aWord);
			}
		} while (aLeft < 0);
	}
	return shuffle(aLetters);
}

// 0x405360
void Game::showHighScores()
{
	if ((int)mPuzzleScores.getCount() < 10 || (int)mSolitaireScores.getCount() < 10
		|| (int)mActionScores.getCount() < 10)
		addDefaultScores();
	mHighScoreScreen->showScores();
}

// 0x4055f0
void Game::setState(int theState)
{
	Window* aRoot = App::instance()->getRootWindow();
	mNextState = 9;
	switch (theState)
	{
	case 0:
		makeOptions();
		makeSplash();
		aRoot->removeAllChildren();
		aRoot->addChild(mSplash);
		mSplash->select(0, 0.5f, true);
		mSplashTime = 2.0f;
		mState = 0;
		return;
	case 1:
		applyVolumes();
		showMainMenu();
		mState = 1;
		return;
	case 2:
		mBallSystem->clear();
		chooseLevel(mRetrying ? mTemplate : -1);
		if (mMode == 2 || mMode == 1)
		{
			mTimer = 1.0f;
			mContinue = true;
			showLoadScreen(false, true);
		}
		else
		{
			mTimer = 1.0f;
			mContinue = false;
		}
		startLevel();
		mState = 2;
		return;
	case 4:
		mState = 4;
		return;
	case 5:
		mBallSystem->startLevel();
		// (on into 3)
	case 3:
		mTimer = 0.0f;
		mState = 5;
		return;
	case 6:
		mTime = (int)((double)mTime + mLevelTime);
		endGame();
		mState = 6;
		return;
	case 7:
		endLevel();
		mTime = (int)((double)mTime + mLevelTime);
		mState = 7;
		if (mMode == 0)
		{
			mTimer = 1.5f;
			return;
		}
		break;
	case 8:
		startScreenSaver();
		mState = 8;
		break;
	default:
		mState = theState;
		return;
	}
	mTimer = 3.0f;
}

// 0x405800: the game link button (the distributor's images ILink + "Normal.png" and "High.png" from loose files, and
// "doGameLink") is left out: nothing in the port sets ILink
void Game::makeMenus()
{
	if (mMainMenu == nullptr)
	{
		mMainMenu = new MainMenuScreen();
		mMainMenu->loadLayout("config\\mainMenu.xml");
		mMainMenu->addRef();
		mMainMenu->setListener(this);
		if (mHighScoreScreen == nullptr)
		{
			mHighScoreScreen = new HighScoreScreen();
			mHighScoreScreen->addRef();
			mHighScoreScreen->loadLayout("config\\HighScore.xml");
			mHighScoreScreen->setListener(this);
			mHighScoreScreen->setVisible(false);
			mPuzzleScores.load("scores\\normal");
			mSolitaireScores.load("scores\\solitaire");
			mActionScores.load("scores\\action");
			mHighScoreScreen->setTables(&mPuzzleScores, &mSolitaireScores, &mActionScores);
			showHighScores();
		}
		if (mHighScoreEntry == nullptr)
		{
			mHighScoreEntry = new HighScoreEntryLayout();
			mHighScoreEntry->addRef();
			mHighScoreEntry->setListener(this);
			mHighScoreEntry->loadLayout("config\\highScoreEntry.xml");
		}
		if (mLevelsSelection == nullptr)
		{
			mLevelsSelection = new LevelsSelectionScreen(10, 10);
			mLevelsSelection->addRef();
			mLevelsSelection->loadLayout("config\\levelsSelection.xml");
			mLevelsSelection->setListener(this);
		}
		if (mDifficultySelection == nullptr)
		{
			mDifficultySelection = new LevelsSelectionScreen(3, 4);
			mDifficultySelection->addRef();
			mDifficultySelection->loadLayout("config\\difficultySelection.xml");
			mDifficultySelection->setListener(this);
		}
		if (mSolitaireSelection == nullptr)
		{
			mSolitaireSelection = new LevelsSelectionScreen(3, 7);
			mSolitaireSelection->addRef();
			mSolitaireSelection->loadLayout("config\\solitaireSelection.xml");
			mSolitaireSelection->setListener(this);
		}
		if (mPuzzleHelp == nullptr)
		{
			mPuzzleHelp = new ScreenLayout();
			mPuzzleHelp->addRef();
			mPuzzleHelp->loadLayout("config\\help\\puzzleHelp.xml");
			mPuzzleHelp->setListener(this);
			CheckBox* aBox = static_cast<CheckBox*>(mPuzzleHelp->getNode("againBox"));
			aBox->setChecked(!mSettings.getShowPuzzleHelp());
		}
		if (mActionHelp == nullptr)
		{
			mActionHelp = new ScreenLayout();
			mActionHelp->addRef();
			mActionHelp->loadLayout("config\\help\\actionHelp.xml");
			mActionHelp->setListener(this);
			CheckBox* aBox = static_cast<CheckBox*>(mActionHelp->getNode("againBox"));
			aBox->setChecked(!mSettings.getShowActionHelp());
		}
		if (mSolitaireHelp == nullptr)
		{
			mSolitaireHelp = new ScreenLayout();
			mSolitaireHelp->addRef();
			mSolitaireHelp->loadLayout("config\\help\\solitaireHelp.xml");
			mSolitaireHelp->setListener(this);
			CheckBox* aBox = static_cast<CheckBox*>(mSolitaireHelp->getNode("againBox"));
			aBox->setChecked(!mSettings.getShowSolitaireHelp());
		}
		if (mPuzzleBrief == nullptr)
		{
			mPuzzleBrief = new ScreenLayout();
			mPuzzleBrief->addRef();
			mPuzzleBrief->loadLayout("config\\help\\puzzleBrief.xml");
		}
		if (mActionBrief == nullptr)
		{
			mActionBrief = new ScreenLayout();
			mActionBrief->addRef();
			mActionBrief->loadLayout("config\\help\\actionBrief.xml");
		}
		if (mSolitaireBrief == nullptr)
		{
			mSolitaireBrief = new ScreenLayout();
			mSolitaireBrief->addRef();
			mSolitaireBrief->loadLayout("config\\help\\solitaireBrief.xml");
		}
		if (mNewGameScreen == nullptr)
		{
			mNewGameScreen = new ScreenLayout();
			mNewGameScreen->addRef();
			mNewGameScreen->loadLayout("config\\newGameScreen.xml");
			mNewGameScreen->setListener(this);
		}
		if (mQuitGameScreen == nullptr)
		{
			mQuitGameScreen = new ScreenLayout();
			mQuitGameScreen->addRef();
			mQuitGameScreen->loadLayout("config\\quitGameScreen.xml");
			mQuitGameScreen->setListener(this);
		}
	}
}

// 0x406430: setState(2) inlined at the end
void Game::retry()
{
	if (mState == 4)
		resume();
	int aScore = mScore - mLevelPoints;
	mLevelPoints = 0;
	mScore = aScore;
	mScoreText->setNumber(aScore);
	mLevelPoints = 0;
	if (mLevelHintsUsed != 0)
		mHints = mHints + mLevelHintsUsed;
	mHints = mHints - 1;
	mRetrying = true;
	setState(2);
}

// 0x406520
void Game::pause(bool theShowMenu)
{
	if (theShowMenu)
	{
		mPausedScreen->fade(true, SCREEN_FADE, 1.0f);
		showLoadScreen(true, false);
		if (mSettings.isMusicEnabled())
		{
			SoundSystem* aSystem = App::getSoundSystem();
			if (aSystem != nullptr)
				aSystem->setCategoryVolume(1, 0.0f);
		}
	}
	mContinue = false;
	mBallSystem->mHud->setPlaying(false);
	mBallSystem->mLayer->fade(false, 1.0f, 1.0f);
	App::instance()->getRootWindow();
	mNextState = 9;
	mState = 4;
}

// 0x406da0: setState(2) inlined at the end
void Game::newGame(int theMode, int theLevel)
{
	mLevel = theLevel;
	mScore = 0;
	mTime = 0;
	mWildcards = 0;
	mMode = theMode;
	mCurrentLevel = nullptr;
	mBallSystem->setMode(theMode);
	mLevelHintsUsed = 0;
	mHints = 0;
	mHintsUsed = 0;
	mTemplate = 0;
	mLongestWords.clear();
	showGameScreen();
	mSavedGame = nullptr;
	mSettings.setSavedGame(theMode, nullptr);
	mRetrying = false;
	// A later start gives the points the earlier levels would have (and solitaire's hints)
	if (theMode == 2)
	{
		if (theLevel == 4)
			mScore = mScore + 10000;
		else if (theLevel == 8)
			mScore = mScore + 25000;
	}
	else if (theMode == 1)
	{
		if (theLevel == 7)
		{
			mScore = mScore + 6000;
			mHints = 3;
		}
		else if (theLevel == 14)
		{
			mScore = mScore + 16000;
			mHints = 3;
		}
	}
	mScoreText->setNumber(mScore);
	setState(2);
}

// 0x406f30: the letters used stand for the level's points too (from the saved game's +0x24, not its +0x20)
void Game::continueGame(SavedGame* theGame)
{
	mLevel = theGame->mLevel;
	int aScore = theGame->mScore;
	mScore = aScore;
	mTime = theGame->mTime;
	mScoreText->setNumber(aScore);
	mWildcards = theGame->mWildcards;
	int aMode = theGame->mMode;
	mMode = aMode;
	mCurrentLevel = nullptr;
	mBallSystem->setMode(aMode);
	mHints = theGame->mHints;
	mHintsUsed = theGame->mHintsUsed;
	mLevelHintsUsed = theGame->mLevelHintsUsed;
	mLongestWords.clear();
	showGameScreen();
	mSavedGame = theGame;
	mBallSystem->clear();
	int aTemplate = theGame->mTemplate;
	mTemplate = aTemplate;
	chooseLevel(aTemplate);
	mBallSystem->startLevel();
	mBallSystem->loadState(mSavedGame);
	mLevelPoints = theGame->mLettersUsed;
	int aUsed = theGame->mLettersUsed;
	mLettersUsed = aUsed;
	mLettersLeft = mCurrentLevel->mBallCriteria - aUsed;
	clearLoadTexts();
	showLoadScreen(false, true);
	mMusic->stop();
	if (mMode == 2)
		mMusic->select(0, false);
	mMusic->start();
	App::instance()->getRootWindow();
	mTimer = 0.0f;
	mNextState = 9;
	mState = 5;
}

// 0x4070d0: a pause without its menu first (from a pause, the game resumed and paused again)
void Game::helpKey(bool theInGame, int theMode)
{
	if (mState == 4)
	{
		resume();
	}
	else if (mState != 5)
	{
		goto show;
	}
	mContinue = false;
	mBallSystem->mHud->setPlaying(false);
	mBallSystem->mLayer->fade(false, 1.0f, 1.0f);
	App::instance()->getRootWindow();
	mNextState = 9;
	mState = 4;
show:
	ScreenLayout* aHelp;
	if (theMode == 0)
		aHelp = mPuzzleHelp;
	else if (theMode == 1)
		aHelp = mSolitaireHelp;
	else if (theMode == 2)
		aHelp = mActionHelp;
	else
		aHelp = nullptr;  // (the original uses whatever its register holds)
	aHelp->getNode("menuHelp")->setVisible(!theInGame);
	aHelp->getNode("gameHelp")->setVisible(theInGame);
	aHelp->getNode("OK")->setEnabled(!theInGame);
	aHelp->getNode("Cancel")->setEnabled(!theInGame);
	aHelp->getNode("ReturnToGame")->setEnabled(theInGame);
	mShowNext = aHelp;
}

// 0x407280: a pause without its menu, under the quit screen
void Game::quit()
{
	mShowNext = mQuitGameScreen;
	if (mState == 4)
		resume();
	mContinue = false;
	mBallSystem->mHud->setPlaying(false);
	mBallSystem->mLayer->fade(false, 1.0f, 1.0f);
	App::instance()->getRootWindow();
	mNextState = 9;
	mState = 4;
}

// 0x407580
void Game::screenAction(std::string theAction)
{
	FadeContainer* aScreen;
	if (theAction.compare("options") == 0)
	{
		mSettings.isFullScreen();
		mShowNext = mOptions;
		return;
	}
	if (theAction.compare("doOptionsDone") == 0)
	{
		mOptions->fade(false, SCREEN_FADE, 1.0f);
		mOptions->setCallback(&removeWhenFaded, mOptions);
		return;
	}
	if (theAction.compare("optionToggleSound") == 0)
	{
		applyVolumes();
		return;
	}
	if (theAction.compare("optionToggleMusic") == 0)
	{
		applyVolumes();
		return;
	}
	if (theAction.compare("optionToggleFullScreen") == 0)
	{
		getHost()->setFullScreen(!getHost()->isFullScreen());
		applyVolumes();
		return;
	}
	if (theAction.compare("optionTogglePuzzleHelp") == 0)
	{
		CheckBox* aBox = static_cast<CheckBox*>(mPuzzleHelp->getNode("againBox"));
		mSettings.setShowPuzzleHelp(!aBox->isChecked());
		return;
	}
	if (theAction.compare("optionToggleActionHelp") == 0)
	{
		CheckBox* aBox = static_cast<CheckBox*>(mActionHelp->getNode("againBox"));
		mSettings.setShowActionHelp(!aBox->isChecked());
		return;
	}
	if (theAction.compare("optionToggleSolitaireHelp") == 0)
	{
		CheckBox* aBox = static_cast<CheckBox*>(mSolitaireHelp->getNode("againBox"));
		mSettings.setShowSolitaireHelp(!aBox->isChecked());
		return;
	}
	if (theAction.compare("clickToContinue") == 0)
	{
		mContinue = true;
		return;
	}
	if (theAction.compare("newPuzzleGame") == 0)
	{
		if (mSettings.getSavedGame(0) != nullptr)
		{
			mMode = 0;
			mShowNext = mNewGameScreen;
			return;
		}
		if (10 < mSettings.mHighestLevels[0])
		{
			mLevelsSelection->showLevels(mSettings.mHighestLevels[0]);
			mShowNext = mLevelsSelection;
			return;
		}
		if (mSettings.getShowPuzzleHelp())
		{
			mLevel = 0;
			helpKey(false, 0);
			return;
		}
		newGame(0, 0);
		return;
	}
	if (theAction.compare("levelsSelectionDone") == 0)
	{
		mLevelsSelection->fade(false, SCREEN_FADE, 1.0f);
		aScreen = mLevelsSelection;
		goto removeScreen;
	}
	if (theAction.compare(0, 6, "levels") == 0)
	{
		int aLevel;
		std::sscanf(theAction.c_str(), "levels%d", &aLevel);
		if (mSettings.getShowPuzzleHelp())
		{
			mLevel = aLevel - 1;
			helpKey(false, 0);
			return;
		}
		newGame(0, aLevel - 1);
		return;
	}
	if (theAction.compare("doPuzzleHelpDone") == 0)
	{
		SavedGame* aGame = mSettings.getSavedGame(0);
		if (aGame == nullptr)
			newGame(0, mLevel);
		else
			continueGame(aGame);
		return;
	}
	if (theAction.compare("doPuzzleHelpCancel") == 0)
	{
		mContinue = true;
		mPuzzleHelp->fade(false, SCREEN_FADE, 1.0f);
		aScreen = mPuzzleHelp;
		goto removeScreen;
	}
	if (theAction.compare("difficultySelectionDone") == 0)
	{
		mDifficultySelection->fade(false, SCREEN_FADE, 1.0f);
		aScreen = mDifficultySelection;
		goto removeScreen;
	}
	if (theAction.compare("doActionHelpDone") == 0)
	{
		SavedGame* aGame = mSettings.getSavedGame(2);
		if (aGame == nullptr)
			newGame(2, mLevel);
		else
			continueGame(aGame);
		return;
	}
	if (theAction.compare("doActionHelpCancel") == 0)
	{
		mContinue = true;
		mActionHelp->fade(false, SCREEN_FADE, 1.0f);
		aScreen = mActionHelp;
		goto removeScreen;
	}
	if (theAction.compare("newActionGame") == 0)
	{
		mLevel = 0;
		if (mSettings.getSavedGame(2) == nullptr)
		{
			if (mSettings.mHighestLevels[2] < 5)
			{
				if (!mSettings.getShowActionHelp())
				{
					newGame(2, 0);
				}
				else
				{
					mLevel = 0;
					helpKey(false, 2);
				}
			}
			else
			{
				mDifficultySelection->showLevels(mSettings.mHighestLevels[2]);
				mShowNext = mDifficultySelection;
			}
		}
		else
		{
			mMode = 2;
			mShowNext = mNewGameScreen;
		}
		return;
	}
	if (theAction.compare(0, 6, "action") == 0)
	{
		int aLevel;
		std::sscanf(theAction.c_str(), "action%d", &aLevel);
		if (!mSettings.getShowActionHelp())
		{
			newGame(2, aLevel - 1);
		}
		else
		{
			mLevel = aLevel - 1;
			helpKey(false, 2);
		}
		return;
	}
	if (theAction.compare("solitaireSelectionDone") == 0)
	{
		mSolitaireSelection->fade(false, SCREEN_FADE, 1.0f);
		aScreen = mSolitaireSelection;
		goto removeScreen;
	}
	if (theAction.compare("doSolitaireHelpDone") == 0)
	{
		SavedGame* aGame = mSettings.getSavedGame(1);
		if (aGame == nullptr)
			newGame(1, mLevel);
		else
			continueGame(aGame);
		return;
	}
	if (theAction.compare("doSolitaireHelpCancel") == 0)
	{
		mContinue = true;
		mSolitaireHelp->fade(false, SCREEN_FADE, 1.0f);
		aScreen = mSolitaireHelp;
		goto removeScreen;
	}
	if (theAction.compare("newSolitaireGame") == 0)
	{
		if (mSettings.getSavedGame(1) == nullptr)
		{
			if (mSettings.mHighestLevels[1] < 6)
			{
				if (!mSettings.getShowSolitaireHelp())
				{
					newGame(1, 0);
				}
				else
				{
					mLevel = 0;
					helpKey(false, 1);
				}
			}
			else
			{
				mSolitaireSelection->showLevels(mSettings.mHighestLevels[1]);
				mShowNext = mSolitaireSelection;
			}
		}
		else
		{
			mMode = 1;
			mShowNext = mNewGameScreen;
		}
		return;
	}
	if (theAction.compare(0, 9, "solitaire") == 0)
	{
		int aLevel;
		std::sscanf(theAction.c_str(), "solitaire%d", &aLevel);
		if (!mSettings.getShowSolitaireHelp())
		{
			newGame(1, aLevel - 1);
		}
		else
		{
			mLevel = aLevel - 1;
			helpKey(false, 1);
		}
		return;
	}
	if (theAction.compare("highScoreDone") == 0)
	{
		mHighScoreScreen->fade(false, SCREEN_FADE, 1.0f);
		aScreen = mHighScoreScreen;
		goto removeScreen;
	}
	if (theAction.compare("highScoreEntryDone") == 0)
	{
		HighScores* aTable = getHighScores(mMode);
		aTable->add(mHighScoreEntry->mName, mScore);
		aTable->save();
		mSettings.setDefaultScoreName(mHighScoreEntry->mName);
		mSettings.save();
		mSettings.load();
		mHighScoreScreen->showScores();
		App::instance()->getRootWindow()->removeKeyHandler(mHighScoreEntry);
		Window* aRoot = App::instance()->getRootWindow();
		aRoot->removeAllChildren();
		setState(1);  // (inlined)
		aRoot->addChild(mHighScoreEntry);
		mShowNext = mHighScoreScreen;
		return;
	}
	if (theAction.compare("highscore") == 0)
	{
		mShowNext = mHighScoreScreen;
		return;
	}
	if (theAction.compare("doContinueGame") == 0)
	{
		mNewGameScreen->fade(false, SCREEN_FADE, 1.0f);
		mNewGameScreen->setCallback(&removeWhenFaded, mNewGameScreen);
		int aMode = mMode;
		if (!mSettings.getShowHelp(aMode))
			continueGame(mSettings.getSavedGame(aMode));
		else
			helpKey(false, aMode);
		return;
	}
	if (theAction.compare("doStartNewGame") == 0)
	{
		mSettings.setSavedGame(mMode, nullptr);
		mNewGameScreen->fade(false, SCREEN_FADE, 1.0f);
		mNewGameScreen->setCallback(&removeWhenFaded, mNewGameScreen);
		int aMode = mMode;
		if (aMode == 0)
			screenAction("newPuzzleGame");
		else if (aMode == 1)
			screenAction("newSolitaireGame");
		else if (aMode == 2)
			screenAction("newActionGame");
		return;
	}
	if (theAction.compare("doQuitNoSave") == 0)
	{
		mQuitGameScreen->fade(false, SCREEN_FADE, 1.0f);
		mQuitGameScreen->setCallback(&removeWhenFaded, mQuitGameScreen);
		mQuitting = true;
		mContinue = false;
		mSettings.setSavedGame(mMode, nullptr);
		mNextState = 6;
		return;
	}
	if (theAction.compare("doQuitGame") == 0)
	{
		mQuitGameScreen->fade(false, SCREEN_FADE, 1.0f);
		mQuitGameScreen->setCallback(&removeWhenFaded, mQuitGameScreen);
		gameOver(true, true);
		return;
	}
	if (theAction.compare("doCancelQuit") == 0)
	{
		mQuitGameScreen->fade(false, SCREEN_FADE, 1.0f);
		mQuitGameScreen->setCallback(&removeWhenFaded, mQuitGameScreen);
		resume();
		return;
	}
	if (theAction.compare("quit") == 0)
	{
		App::instance()->quit(0);
		return;
	}
	if (theAction.compare("doGameLink") == 0)
		openLink();
	return;
removeScreen:
	aScreen->setCallback(&removeWhenFaded, aScreen);
}

// 0x408190
void Game::loadData()
{
	LevelHandler aLevels;
	aLevels.load("config\\levels.xml", &mLevels);
	aLevels.load("config\\freematch-levels.xml", &mActionLevels);
	aLevels.load("config\\solitaire-levels.xml", &mSolitaireLevels);
	mCurrentLevel = mLevels[0];
	{
		Ref<InputStream> aStream = App::instance()->openStream("config\\matchList.txt");
		LineReader aReader(aStream);
		std::string aLine;
		int aLength = aReader.readLine(aLine);
		while (aLength != 0)
		{
			if (2 < aLine.size())
				mWords.add(aLine.c_str());
			aLength = aReader.readLine(aLine);
		}
	}
	{
		Ref<InputStream> aStream = App::instance()->openStream("config\\hintList.txt");
		LineReader aReader(aStream);
		std::string aLine;
		int aLength = aReader.readLine(aLine);
		while (aLength != 0)
		{
			if (3 < aLine.size())
			{
				mDictionary.add(aLine.c_str());
				mHintWords.push_back(aLine);
			}
			aLength = aReader.readLine(aLine);
		}
	}
	loadSounds();
	// (the frame rate display, 0x42a240, is made here and never shown: left out)
	mEditorHud = new ScreenLayout();
	mEditorHud->loadLayout("config\\EditorHUD.xml");
	mEditorHud->addRef();
	CheckBox* aSnap = static_cast<CheckBox*>(mEditorHud->getNode("gridSnap"));
	mGridSnap = true;
	aSnap->setChecked(true);
}

// 0x408640: under 4, the start of a longer word; else whole words until they make exactly theCount (the words joined
// by spaces are logged)
void Game::makeWords(int theCount, std::vector<std::string>& theWords)
{
	std::string aLog;
	if (theCount < 4)
	{
		std::string aWord = makeLetters(4, 10).substr(0, theCount);
		theWords.push_back(aWord);
		aLog.append(aWord);
		aLog.append(" ");
	}
	else
	{
		int aLeft;
		do
		{
			theWords.clear();
			aLeft = theCount;
			while (0 < aLeft)
			{
				std::string aWord = makeLetters(4, 7);
				aLeft -= (int)aWord.size();
				theWords.push_back(aWord);
			}
		} while (aLeft < 0);
		for (unsigned int i = 0; i < theWords.size(); i++)
		{
			aLog.append(theWords[i]);
			aLog.append(" ");
		}
	}
	char aText[256];
	std::snprintf(aText, sizeof(aText), "random words: %s\n", aLog.c_str());
	App::instance()->log(aText);
}

// 0x4088f0: a longer word takes a place and moves the one there down; five words at most
void Game::noteWord(std::string theWord)
{
	std::string aWord = theWord;
	for (unsigned int i = 0; i < mLongestWords.size(); i++)
	{
		std::string anOther = mLongestWords[i];
		if (anOther.size() < aWord.size())
		{
			mLongestWords[i] = aWord;
			aWord = anOther;
		}
	}
	if (mLongestWords.size() < 5)
		mLongestWords.push_back(aWord);
}

// 0x408fd0
void Game::updateSplash(UpdateInfo& theInfo)
{
	if (mMainMenu == nullptr && mSplashTime < 1.0f)
	{
		loadData();
		makeMenus();
		makeGameScreen();
	}
	float aTime = mSplashTime - theInfo.mSeconds;
	mSplashTime = aTime;
	if (aTime < 0.0f)
		setState(1);  // (inlined)
}

// 0x409060
void Game::update(UpdateInfo& theInfo)
{
	Window* aRoot = App::instance()->getRootWindow();
	if (App::getSoundSystem() != nullptr)
		App::getSoundSystem()->update((int)(theInfo.mSeconds * 1000.0f));
	if (mMusic != nullptr)
		mMusic->update(theInfo);
	switchScreens();
	if (mNextRoot != nullptr)
	{
		aRoot->removeAllChildren();
		aRoot->addChild(mNextRoot);
		mNextRoot = nullptr;
		if (mHighScoreScreen->isVisible())
			aRoot->addChild(mHighScoreScreen);
	}
	switch (mState)
	{
	case 0:
		updateSplash(theInfo);
		break;
	case 2:
	{
		float aTime = mTimer - theInfo.mSeconds;
		mTimer = aTime;
		if (aTime < 0.0f)
		{
			if (!mContinue)
			{
				showLoadScreen(true, false);
			}
			else
			{
				showLoadScreen(false, false);
				clearLoadTexts();
				mLoadScreen->setEnabled(false);
				setState(5);  // (inlined)
			}
		}
		break;
	}
	case 4:
		if (mContinue)
			resume();
		break;
	case 5:
		if (!mBallSystem->mEditor)
		{
			if (mMode == 2 && 0.0f < mTimer)
			{
				float aTime = mTimer - theInfo.mSeconds;
				mTimer = aTime;
				if (aTime <= 0.0f)
				{
					mTimer = 0.0f;
					mLevelEnd->fade(false, SCREEN_FADE, 1.0f);
					mMusic->start();
				}
			}
			mLevelTime = theInfo.mSeconds + mLevelTime;
			if (mCurrentLevel->mBallCriteria <= mLettersUsed)
				mBallSystem->levelDone();
		}
		break;
	case 6:
		if (mContinue)
		{
			mContinue = false;
			HighScores* aTable = getHighScores(mMode);
			int aRank = (int)aTable->findRank((unsigned int)mScore);
			if (aRank < 10 && 0 < mScore)
			{
				mHighScoreEntry->startEntry(mSettings.mDefaultScoreName, mScore);
				mShowNext = mHighScoreEntry;
			}
			else
			{
				setState(1);  // (inlined)
			}
		}
		break;
	case 7:
		if (!mBallSystem->isBusy())
		{
			float aTime = mTimer - theInfo.mSeconds;
			mTimer = aTime;
			if (aTime < 3.0f && !mLoadTexts[1]->isVisible() && mMode != 0)
			{
				mContinue = false;
				mCountUp = 0.0f;
				mLoadTexts[1]->setVisible(true);
				playSound("MESSAGE_ADD");
			}
			if (mTimer < 2.5f && !mLoadTexts[2]->isVisible() && mMode != 0)
			{
				mLoadTexts[2]->setVisible(true);
				playSound("MESSAGE_ADD");
			}
			if (mTimer < 2.0f && !mLoadTexts[3]->isVisible() && mMode != 0)
			{
				mLoadTexts[3]->setVisible(true);
				playSound("MESSAGE_ADD");
			}
			else if (mTimer < 1.5f && mMode != 0)
			{
				char aText[128];
				std::sprintf(aText, GT(0x4806ac).c_str(), formatNumber(mBonus).c_str());
				setLoadText(aText, 4);
			}
			if (mTimer < 1.0f && 0 < mBonus && mMode != 0)
			{
				// A hundred points at a time, faster as it goes
				float aStep = 2.0f - mCountUp;
				if (aStep <= 0.0f)
					aStep = 0.0f;
				mBonus = mBonus - 100;
				aStep = aStep * 0.15f + 0.05f;
				mCountUp = aStep + mCountUp;
				mTimer = aStep + mTimer;
				playSound("STATS_COUNTUP");
				TextEffect* aPoints = mTextEffects->create("bonusPoints", 570.0f, 394.0f, &Game::pointsShown, 100);
				aPoints->setText("100");
				showLoadScreen(true, false);
				if (!mContinue)
					break;
				mTextEffects->clear();
				addScore(mBonus);
			}
			else
			{
				if (0.0f <= mTimer || !mTextEffects->allDone())
					break;
				showLoadScreen(true, false);
				if (!mContinue)
					break;
			}
			mLevel = mLevel + 1;
			mRetrying = false;
			setState(2);
		}
		break;
	case 8:
	{
		float aTime = mTimer - theInfo.mSeconds;
		mTimer = aTime;
		if (aTime < 0.0f)
			mScreenSaver->setEnabled(true);
		break;
	}
	}
	if (mNextState != 9)
		setState(mNextState);
}

// 0x401290
void Game::scoreShown(int theLetters)
{
	Game* aGame = sInstance;
	int aLeft = aGame->mLettersLeft - theLetters;
	if (aLeft < 0)
		aLeft = 0;
	aGame->mLettersLeft = aLeft;
}
