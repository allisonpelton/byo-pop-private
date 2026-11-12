#include "Hud.h"
#include "Game.h"
#include "GameText.h"
#include "Level.h"
#include "Particles.h"
#include "SavedGame.h"
#include "TextItem.h"
#include "WordBallSystem.h"
#include <cstdio>

using namespace engine;

// 0x424040: the original sets only the definition line, the next ball and the spawn button here, load() the rest;
// the port clears them all
Hud::Hud()
	: mMode(0), mDefinition(nullptr), mBallsRemaining(nullptr), mSpawnBallText(nullptr), mQuitBtn(nullptr),
	  mRetryBtn(nullptr), mHelpBtn(nullptr), mPauseBtn(nullptr), mMenuGroup(nullptr), mSubmitBtn(nullptr),
	  mHintBtn(nullptr), mWildcardBtn(nullptr), mLevelText(nullptr), mWildcardText(nullptr), mHintCounter(nullptr),
	  mHintText(nullptr), mSubmitShown(false), mHintReady(false), mWildcardReady(false), mNewBall(nullptr),
	  mSpawnText(nullptr), mSpawnBtn(nullptr), mNextBallTime(0), mFoundFade(0), mFoundIndex(0)
{
	for (int i = 0; i < 14; i++)
	{
		mWordTexts[i] = nullptr;
		mBrushes[i] = nullptr;
		mRecycles[i] = nullptr;
	}
	setName("Game HUD");
}

// 0x4221c0
void Hud::enableWords()
{
	TextItem** aText = mWordTexts;
	int aLeft = 14;
	do
	{
		(*aText)->setEnabled(true);
		aText++;
		aLeft--;
	} while (aLeft != 0);
}

// 0x4221f0
void Hud::disableWords()
{
	TextItem** aText = mWordTexts;
	int aLeft = 14;
	do
	{
		(*aText)->setEnabled(false);
		aText++;
		aLeft--;
	} while (aLeft != 0);
}

// 0x422220: the hint, submit and wildcard buttons only while they work
void Hud::setControlsEnabled(bool theEnabled, bool theFade, bool theAtOnce)
{
	if (theFade)
		mMenuGroup->fade(theEnabled, theAtOnce ? 0.0f : 0.25f, 1.0f);
	mQuitBtn->setEnabled(theEnabled);
	mHelpBtn->setEnabled(theEnabled);
	mPauseBtn->setEnabled(theEnabled);
	mMenuGroup->setEnabled(theEnabled);
	if (mRetryBtn != nullptr)
		mRetryBtn->setEnabled(theEnabled);
	if (mSpawnBtn != nullptr)
		mSpawnBtn->setEnabled(theEnabled);
	if ((mHintBtn != nullptr) && mHintReady)
		mHintBtn->setEnabled(theEnabled);
	if ((mSubmitBtn != nullptr) && mSubmitShown)
		mSubmitBtn->setEnabled(theEnabled);
	if ((mWildcardBtn != nullptr) && mWildcardReady)
		mWildcardBtn->setEnabled(theEnabled);
}

// 0x422340
void Hud::restartNextBall()
{
	if (mMode == MODE_FREEMATCH)
	{
		mSpawnBtn->setEnabled(false);
		mNewBall->setVisible(false);
		mNextBallTime = 0.5f;
	}
}

// 0x422380: the brush index is the word's in the list, which matches its line while there are 14 words or fewer
void Hud::markFound(int theIndex)
{
	mWords[theIndex]->mFound = true;
	if (0 < mFoundFade)
		mBrushes[mFoundIndex]->setOpacity(1.0f);
	mBrushes[theIndex]->setOpacity(0);
	mFoundIndex = theIndex;
	mFoundFade = 0.5f;
	mBrushes[theIndex]->setVisible(true);
}

// 0x422450
void Hud::setDefinition(std::string theText)
{
	if (mDefinition != nullptr)
		mDefinition->setText(theText);
}

// 0x4224b0: a recycle button goes to the right of its word, a brush is stretched to the word's width
void Hud::showWords()
{
	unsigned int aCount = (unsigned int)mWords.size();
	unsigned int aFirst = 0;
	if (14 < aCount)
		aFirst = aCount - 14;
	unsigned int i = 0;
	do
	{
		if (i < aCount)
		{
			FoundWord* aWord = mWords[aFirst + i];
			std::string aText = aWord->mWord;
			if (1 < aWord->mMultiplier)
			{
				char aBuf[16];
				std::sprintf(aBuf, " x%d", aWord->mMultiplier);
				aText += aBuf;
			}
			mWordTexts[i]->setText(aText);
			if (mRecycles[i] != nullptr)
			{
				mRecycles[i]->setVisible(true);
				Point aPos = *mWordTexts[i]->getPosition();
				aPos.x = (float)(mWordTexts[i]->getBounds()->width() * 0.5 + 10.0 + aPos.x);
				mRecycles[i]->setPosition(aPos);
			}
			if (mBrushes[i] != nullptr)
			{
				mBrushes[i]->setScale((float)((mWordTexts[i]->getBounds()->width() + 30.0) * 0.005988023952095809),
				                      1.0f);
				if (!aWord->mFound)
					mBrushes[i]->setVisible(false);
				else
					mBrushes[i]->setVisible(true);
			}
		}
		else
		{
			mWordTexts[i]->setText("");
			if (mRecycles[i] != nullptr)
				mRecycles[i]->setVisible(false);
			if (mBrushes[i] != nullptr)
				mBrushes[i]->setVisible(false);
		}
		i++;
	} while (i < 14);
}

// 0x422740: in freematch the timer for the next ball is left at 0 (restartNextBall starts it)
void Hud::setMode(int theMode)
{
	mMode = theMode;
	if (theMode == MODE_PUZZLE)
	{
		disableWords();
	}
	else if (theMode == MODE_SOLITAIRE)
	{
		enableWords();
	}
	else if (theMode == MODE_FREEMATCH)
	{
		disableWords();
		mNewBall = getNode("newBall");
		mNewBall->setEnabled(false);
		mSpawnText = static_cast<TextItem*>(getNode("spawnText"));
		mSpawnBtn = getNode("spawnBtn");
		if (mMode == MODE_FREEMATCH)
		{
			mSpawnBtn->setEnabled(false);
			mNewBall->setVisible(false);
			mNextBallTime = 0.5f;
		}
		mNextBallTime = 0;
	}
}

// 0x422850: the menu's and the playing buttons' actions (the editor's, never reached in this build, are the ball count
// buttons and recycle buttons that take the level's word away: left out)
void Hud::buttonPress(std::string theName)
{
	Game* aGame = Game::instance();
	if (theName == "retry")
	{
		aGame->retry();
	}
	else if (theName == "quit")
	{
		aGame->quit();
	}
	else if (theName == "pause")
	{
		if (aGame->isPaused())
			aGame->resume();
		else
			aGame->pause(true);
	}
	else if (theName == "help")
	{
		aGame->helpKey(1, mMode);
	}
	else if (theName == "submit")
	{
		aGame->mBallSystem->submit();
	}
	else if (theName == "nextLevel")
	{
		aGame->mBallSystem->levelDone();
	}
	else if (theName == "wildcard")
	{
		// port: a press of the faded-out, disabled button still arrives here, and the original then made a wildcard from
		// none and left the count at -1, which the next level's start turned into a loop of 2^32 letters (std::bad_alloc
		// in the port, "Exception Code Honeysuckle" in the original): no wildcard held, no press
		if (aGame->mWildcards > 0)
		{
			aGame->mBallSystem->addWildcard();
			aGame->mWildcards--;
		}
	}
	else if (theName == "hint")
	{
		aGame->mBallSystem->showHint();
	}
	else if (theName == "spawnBall")
	{
		aGame->mBallSystem->spawnNext();
		if (mSpawnBallText != nullptr)
		{
			mSpawnBallText->fade(false, 0.5f, 1.0f);
			mSpawnBallText = nullptr;
		}
		restartNextBall();
	}
	else if (theName.compare(0, 6, "submit") == 0)
	{
		// "submit1"...: a found word's letters back into the queue, its points and letters taken off. The number is
		// the word's place in the whole list, not its line's (they differ past 14 words); the word is not freed
		int anIndex;
		std::sscanf(theName.c_str(), "submit%d", &anIndex);
		anIndex--;
		FoundWord* aWord = mWords[anIndex];
		aGame->mBallSystem->queueLetters(aWord->mLetters, aWord->mChainWords);
		Game::instance()->addScore(-aWord->mPoints);
		aGame->mLettersUsed -= (int)aWord->mLetters.size();
		aGame->mLettersLeft = aGame->mCurrentLevel->mBallCriteria - aGame->mLettersUsed;
		mWords.erase(mWords.begin() + anIndex);
		showWords();
	}
}

// 0x422bb0
void Hud::hideBallButtons()
{
	getNode("increaseBalls")->setVisible(false);
	getNode("decreaseBalls")->setVisible(false);
	Game::instance()->mLettersLeft = Game::instance()->mCurrentLevel->mBallCriteria;
}

// 0x422c40: the button only while the controls are enabled
void Hud::showSubmit(bool theShow)
{
	if (!theShow)
	{
		if (mSubmitShown)
		{
			mSubmitShown = false;
			FadeContainer* aGroup = static_cast<FadeContainer*>(getNode("submit"));
			if (aGroup != nullptr)
			{
				aGroup->fade(false, 0.25f, 1.0f);
				mSubmitBtn->setEnabled(false);
			}
		}
	}
	else if (!mSubmitShown)
	{
		mSubmitShown = true;
		FadeContainer* aGroup = static_cast<FadeContainer*>(getNode("submit"));
		if (aGroup != nullptr)
		{
			aGroup->fade(true, 0.25f, 1.0f);
			if ((mQuitBtn != nullptr) && mQuitBtn->isEnabled())
				mSubmitBtn->setEnabled(true);
		}
	}
}

// 0x422d30
void Hud::setHintReady(bool theReady)
{
	if (!theReady)
	{
		if (mHintReady)
		{
			mHintReady = false;
			FadeContainer* aGroup = static_cast<FadeContainer*>(getNode("hint"));
			if (aGroup != nullptr)
			{
				mHintText->setText(GT(0x482e70));
				// half way out: the box stays, greyed, with "no hint available" and the count
				aGroup->fade(false, 0.25f, 0.5f);
				mHintBtn->setEnabled(false);
			}
		}
	}
	else if (!mHintReady)
	{
		mHintReady = true;
		FadeContainer* aGroup = static_cast<FadeContainer*>(getNode("hint"));
		if (aGroup != nullptr)
		{
			mHintText->setText(GT(0x482e84));
			aGroup->fade(true, 0.25f, 1.0f);
			if ((mQuitBtn != nullptr) && mQuitBtn->isEnabled())
				mHintBtn->setEnabled(true);
		}
	}
}

// 0x422f00
void Hud::setWildcardReady(bool theReady)
{
	if (!theReady)
	{
		if (mWildcardReady)
		{
			mWildcardReady = false;
			FadeContainer* aGroup = static_cast<FadeContainer*>(getNode("wildcard"));
			if (aGroup != nullptr)
			{
				aGroup->fade(false, 0.25f, 1.0f);
				mWildcardBtn->setEnabled(false);
			}
		}
	}
	else if (!mWildcardReady)
	{
		mWildcardReady = true;
		FadeContainer* aGroup = static_cast<FadeContainer*>(getNode("wildcard"));
		if (aGroup != nullptr)
		{
			aGroup->fade(true, 0.25f, 1.0f);
			if ((mQuitBtn != nullptr) && mQuitBtn->isEnabled())
				mWildcardBtn->setEnabled(true);
		}
	}
}

// 0x422ff0: the next piece's letters on it, a blossom where the balls come in
void Hud::showNextBall()
{
	if (mMode == MODE_FREEMATCH)
	{
		WordBallSystem* aSystem = Game::instance()->mBallSystem;
		mNewBall->setScale(0.2f, 0.2f);
		mNewBall->setVisible(true);
		mNextBallTime = 0.25f;
		std::string aPiece;
		aSystem->nextPiece(aPiece, true);
		mSpawnText->setText(aPiece);
		ParticleSystem* aBlossom = Game::instance()->mBallSystem->mBlossoms->getSystem();
		aBlossom->setPosition(470.0f, -26.0f);
		aBlossom->startEmitting();
		Game::instance()->playSound("BALL_RUSTLE");
		if (mSpawnBallText != nullptr)
			mSpawnBallText->fade(true, 0.5f, 1.0f);
	}
}

// 0x423130
std::string Hud::longestWord()
{
	std::string aLongest;
	unsigned int aLength = 0;
	for (unsigned int i = 0; i < mWords.size(); i++)
	{
		FoundWord* aWord = mWords[i];
		unsigned int aSize = (unsigned int)aWord->mWord.size();
		if (aLength < aSize)
		{
			aLongest = aWord->mWord;
			aLength = aSize;
		}
	}
	return aLongest;
}

// 0x423210
void Hud::setPlaying(bool thePlaying)
{
	if (mWildcardBtn != nullptr)
		mWildcardBtn->setEnabled(thePlaying);
	if (mSpawnBtn != nullptr)
		mSpawnBtn->setEnabled(thePlaying);
	if (!thePlaying)
	{
		showSubmit(false);
		setHintReady(false);
	}
}

// 0x423260
void Hud::showBonusWord()
{
	std::string aText = GT(0x482e9c);
	aText += Game::instance()->mBallSystem->mWord;
	setDefinition(aText);
}

// 0x423330: the HUD starts disabled, with the submit and hint buttons hidden and the wildcard button off
void Hud::load(std::string theFile)
{
	loadLayout(theFile);
	setEnabled(false);
	char aName[32];
	int aNumber = 1;
	int aLeft = 14;
	do
	{
		std::sprintf(aName, "word%d", aNumber);
		mWordTexts[aNumber - 1] = static_cast<TextItem*>(getNode(aName));
		std::sprintf(aName, "word%drecycle", aNumber);
		mRecycles[aNumber - 1] = getNode(aName);
		std::sprintf(aName, "brush%d", aNumber);
		mBrushes[aNumber - 1] = getNode(aName);
		aNumber++;
		aLeft--;
	} while (aLeft != 0);
	mDefinition = static_cast<TextItem*>(getNode("definition"));
	mBallsRemaining = static_cast<TextItem*>(getNode("ballsRemaining"));
	mQuitBtn = getNode("quitBtn");
	mRetryBtn = getNode("retryBtn");
	mHelpBtn = getNode("helpBtn");
	mPauseBtn = getNode("pauseBtn");
	mMenuGroup = static_cast<FadeContainer*>(getNode("menuButtonGroup"));
	mLevelText = static_cast<TextItem*>(getNode("level"));
	mWildcardText = static_cast<TextItem*>(getNode("wildcardText"));
	mHintCounter = static_cast<TextItem*>(getNode("hintCounter"));
	mHintText = static_cast<TextItem*>(getNode("hintText"));
	mSpawnBallText = static_cast<FadeContainer*>(getNode("spawnBallText"));
	if (mSpawnBallText != nullptr)
		mSpawnBallText->setVisible(false);
	mSubmitBtn = getNode("submitBtn");
	mHintBtn = getNode("hintBtn");
	mWildcardBtn = getNode("wildcardBtn");
	mSubmitShown = true;
	mHintReady = true;
	mWildcardReady = true;
	Node* aNode = getNode("hint");
	if (aNode != nullptr)
		aNode->setEnabled(false);
	aNode = getNode("submit");
	if (aNode != nullptr)
		aNode->setEnabled(false);
	aNode = getNode("wildcard");
	if (aNode != nullptr)
		aNode->setEnabled(false);
	mFoundFade = 0;
	if (mWildcardBtn != nullptr)
		mWildcardBtn->setEnabled(false);
	if (mSpawnBtn != nullptr)
		mSpawnBtn->setEnabled(false);
	showSubmit(false);  // inlined in the original
	setHintReady(false);
	hideBallButtons();
}

// 0x4238b0: the counters, freematch's next ball growing in, a found word's brush fading in. The original goes on to
// Container's update, not FadeContainer's
void Hud::update(UpdateInfo& theInfo)
{
	Game* aGame = Game::instance();
	char aBuf[16];
	mBallsRemaining->setNumber(aGame->mLettersLeft);
	if (mLevelText != nullptr)
		mLevelText->setNumber(aGame->mLevel + 1);
	if ((aGame->mWildcards != 0) && (mWildcardText != nullptr))
	{
		std::sprintf(aBuf, "x %d", aGame->mWildcards);
		mWildcardText->setText(aBuf);
	}
	if (mHintCounter != nullptr)
	{
		std::sprintf(aBuf, "x %d", aGame->mHints);
		mHintCounter->setText(aBuf);
	}
	if ((mMode == MODE_FREEMATCH) && (0 < mNextBallTime) && aGame->mBallSystem->mStarted && !aGame->isPaused())
	{
		mNextBallTime -= theInfo.mSeconds;
		if (mNextBallTime > 0)
		{
			if (mNewBall->isVisible())
			{
				double aScale = 0.5 - mNextBallTime;
				aScale = aScale + aScale;
				if (aScale < 1.0)
				{
					if (aScale <= 0.0)
						aScale = 0.0;
				}
				else
				{
					aScale = 1.0;
				}
				aScale = aScale * 0.8 + 0.2;
				mNewBall->setScale((float)aScale, (float)aScale);
			}
		}
		else
		{
			mNextBallTime = 0;
			if (!mNewBall->isVisible())
			{
				showNextBall();
			}
			else
			{
				mSpawnBtn->setEnabled(true);
				mNewBall->setScale(1.0f, 1.0f);
			}
		}
	}
	if (0 < mFoundFade)
	{
		mFoundFade -= theInfo.mSeconds;
		if (mFoundFade < 0)
			mFoundFade = 0;
		mBrushes[mFoundIndex]->setOpacity(1.0f - (mFoundFade + mFoundFade));
	}
	Container::update(theInfo);
}

// 0x423ba0
void Hud::addWord(std::string theWord)
{
	FoundWord* aWord = new FoundWord();
	aWord->mLetters = theWord;
	aWord->mWord = theWord;
	aWord->mPoints = 0;
	aWord->mMultiplier = 0;
	aWord->mFound = false;
	mWords.push_back(aWord);
	showWords();
}

// 0x423ca0
void Hud::addFound(const std::string& theLetters, const std::string& theWord, std::vector<std::string>& theChainWords,
	int thePoints, int theMultiplier)
{
	FoundWord* aWord = new FoundWord();
	aWord->mLetters = theLetters;
	aWord->mWord = theWord;
	aWord->mPoints = thePoints;
	aWord->mMultiplier = theMultiplier;
	aWord->mFound = false;
	aWord->mChainWords = theChainWords;
	mWords.push_back(aWord);
	showWords();
}

// 0x423db0
void Hud::reset()
{
	for (unsigned int i = 0; i < mWords.size(); i++)
		delete mWords[i];
	mWords.clear();
	showWords();
}

// 0x422400
void Hud::saveWords(SavedGame* theGame)
{
	for (unsigned int i = 0; i < mWords.size(); i++)
		theGame->addWord(mWords[i]);
}

// 0x423e40
void Hud::setWords(const std::vector<FoundWord*>& theWords)
{
	reset();
	unsigned int i = 0;
	while (i < theWords.size())
	{
		FoundWord* aWord = new FoundWord(*theWords[i]);
		mWords.push_back(aWord);
		i++;
	}
	showWords();
}
