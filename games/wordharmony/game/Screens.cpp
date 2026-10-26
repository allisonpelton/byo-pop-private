#include "Screens.h"
#include "App.h"
#include "Game.h"
#include "HighScores.h"
#include "TextItem.h"
#include <cstdio>

using namespace engine;

// 0x409c20
CongratsScreen::CongratsScreen()
{
	setName("CongratsScreen");
}

// 0x409710
void CongratsScreen::showResults()
{
	Game* aGame = Game::instance();
	static_cast<TextItem*>(getNode("score"))->setNumber(aGame->mScore);
	static_cast<TextItem*>(getNode("time"))->setTime(aGame->mTime);
}

// 0x40b0d0
GameOverScreen::GameOverScreen()
{
	setName("GameOverScreen");
}

// 0x40aec0
void GameOverScreen::showResults()
{
	Game* aGame = Game::instance();
	static_cast<TextItem*>(getNode("score"))->setNumber(aGame->mScore);
	static_cast<TextItem*>(getNode("hints"))->setNumber(aGame->mHintsUsed);
	unsigned int i = 0;
	do
	{
		char aName[16];
		std::sprintf(aName, "word%d", i + 1);
		TextItem* aText = static_cast<TextItem*>(getNode(aName));
		if (i < aGame->mLongestWords.size())
			aText->setText(aGame->mLongestWords[i]);
		else
			aText->setText("");
		i++;
	} while (i < 5);
	Node* anExtra = getNode("extraScores");
	if (aGame->mMode != 0)
		anExtra->setVisible(true);
	else
		anExtra->setVisible(false);
}

// 0x40b510
HighScoreEntryLayout::HighScoreEntryLayout() : mScore(0), mNameText(nullptr)
{
	setEnabled(true);
	setName("HighScoreEntryLayout");
}

// 0x40b410
HighScoreEntryLayout::~HighScoreEntryLayout()
{
	App::instance()->getRootWindow()->removeKeyHandler(this);
}

// 0x40b1f0
void HighScoreEntryLayout::startEntry(std::string theName, int theScore)
{
	mName = theName;
	mScore = theScore;
	static_cast<TextItem*>(getNode("scoreText"))->setNumber(mScore);
	mNameText = static_cast<TextItem*>(getNode("nameText"));
	mNameText->setText(mName);
	App::instance()->getRootWindow()->addKeyHandler(this);
}

// 0x40b2e0
bool HighScoreEntryLayout::keyDown(int theKey)
{
	if ((0x1f < theKey) && (theKey < 0x88))
	{
		if (mName.size() < 16)
		{
			mName.append(1, (char)theKey);
			mNameText->setText(mName);
		}
		return true;
	}
	if (theKey == 8)
	{
		if (mName.size() != 0)
		{
			mName = mName.substr(0, mName.size() - 1);
			mNameText->setText(mName);
		}
		return true;
	}
	if (theKey == 0xd)
	{
		mListener->screenAction("highScoreEntryDone");
		return true;
	}
	return false;
}

// 0x40b1d0
bool HighScoreEntryLayout::keyUp(int theKey)
{
	return (0x1f < theKey) && (theKey < 0x88);
}

// 0x40c190
HighScoreScreen::HighScoreScreen() : mPuzzle(nullptr), mSolitaire(nullptr), mTsunami(nullptr), mFirst(0)
{
	setName("HighScoreScreen");
}

// 0x40b660
void HighScoreScreen::setTables(HighScores* thePuzzle, HighScores* theSolitaire, HighScores* theTsunami)
{
	mPuzzle = thePuzzle;
	mSolitaire = theSolitaire;
	mTsunami = theTsunami;
	setEnabled(true);
	mFirst = 0;
}

// 0x40b6a0
void HighScoreScreen::reset()
{
	mFirst = 0;
	showScores();
}

namespace
{

// One table's lines (inlined three times in 0x40b6c0); a line past the table's end is hidden
void showTable(HighScoreScreen* theScreen, HighScores* theTable, const char* thePrefix)
{
	char aName[64];
	int i = theScreen->mFirst;
	if (i < i + 10)
	{
		do
		{
			if (i < (int)theTable->getCount())
			{
				std::string aPlayer = theTable->getName(i);
				std::sprintf(aName, "%s-scoreRank%d", thePrefix, i - theScreen->mFirst);
				TextItem* aRank = static_cast<TextItem*>(theScreen->getNode(aName));
				char aNumber[16];
				std::sprintf(aNumber, "%d.", i + 1);
				aRank->setText(aNumber);
				aRank->setVisible(true);
				std::sprintf(aName, "%s-scoreName%d", thePrefix, i - theScreen->mFirst);
				TextItem* aText = static_cast<TextItem*>(theScreen->getNode(aName));
				aText->setText(aPlayer);
				aText->setVisible(true);
				unsigned int aScore = theTable->getScore(i);
				std::sprintf(aName, "%s-score%d", thePrefix, i - theScreen->mFirst);
				aText = static_cast<TextItem*>(theScreen->getNode(aName));
				aText->setNumber((int)aScore);
				aText->setVisible(true);
			}
			else
			{
				std::sprintf(aName, "%s-scoreRank%d", thePrefix, i - theScreen->mFirst);
				theScreen->getNode(aName)->setVisible(false);
				std::sprintf(aName, "%s-scoreName%d", thePrefix, i - theScreen->mFirst);
				theScreen->getNode(aName)->setVisible(false);
				std::sprintf(aName, "%s-score%d", thePrefix, i - theScreen->mFirst);
				theScreen->getNode(aName)->setVisible(false);
			}
			i++;
		} while (i < theScreen->mFirst + 10);
	}
}

}

// 0x40b6c0
void HighScoreScreen::showScores()
{
	showTable(this, mPuzzle, "puzzle");
	showTable(this, mTsunami, "tsunami");
	showTable(this, mSolitaire, "solitaire");
}

// 0x40c480
LevelsSelectionScreen::LevelsSelectionScreen(int theCount, int theStep) : mHighest(0)
{
	setName("LevelsSelectionScreen");
	mCount = theCount;
	mStep = theStep;
}

// 0x40c2b0
void LevelsSelectionScreen::showLevels(int theHighest)
{
	setEnabled(true);
	mHighest = theHighest + 1;
	int i = 0;
	if (0 < mCount)
	{
		do
		{
			int aLevel = mStep * i + 1;
			char aName[24];
			std::sprintf(aName, "level%d", aLevel);
			Node* aButton = getNode(aName);
			std::sprintf(aName, "level%dImg", aLevel);
			Node* anImage = getNode(aName);
			std::sprintf(aName, "level%dTxt", aLevel);
			Node* aText = getNode(aName);
			if (mHighest < aLevel)
			{
				aButton->setEnabled(false);
				aButton->setOpacity(0.5f);
				anImage->setOpacity(0.5f);
				aText->setOpacity(0.5f);
			}
			else
			{
				aButton->setEnabled(true);
				aButton->setOpacity(1.0f);
				anImage->setOpacity(1.0f);
				aText->setOpacity(1.0f);
			}
			i++;
		} while (i < mCount);
	}
}

// 0x40c770
MainMenuScreen::MainMenuScreen()
{
	setName("MainMenuScreen");
}

// 0x40c5a0
void MainMenuScreen::buttonEnter(std::string theName)
{
	Game* aGame = Game::instance();
	int aMode;
	if (theName.compare("newPuzzleGame") == 0)
		aMode = 0;
	else if (theName.compare("newSolitaireGame") == 0)
		aMode = 1;
	else if (theName.compare("newActionGame") == 0)
		aMode = 2;
	else
		return;
	aGame->showModeInfo(aMode);
}

// 0x40c700
void MainMenuScreen::buttonPress(std::string theName)
{
	ScreenLayout::buttonPress(theName);
}

// 0x40c650
void MainMenuScreen::buttonExit(std::string theName)
{
	Game* aGame = Game::instance();
	int aMode;
	if (theName.compare("newPuzzleGame") == 0)
		aMode = 0;
	else if (theName.compare("newSolitaireGame") == 0)
		aMode = 1;
	else if (theName.compare("newActionGame") == 0)
		aMode = 2;
	else
		return;
	aGame->hideModeInfo(aMode);
}

// 0x40db70
ScreenSaver::ScreenSaver()
{
	setName("ScreenSaver");
	addOwnFlags(NODE_MOUSE);
	App::instance()->getRootWindow()->addKeyHandler(this);
}

// 0x40db50
void ScreenSaver::mouseMove(Point& p)
{
	Game::instance()->endProgram();
}

// 0x40db60
bool ScreenSaver::keyDown(int theKey)
{
	Game::instance()->endProgram();
	return true;
}

// 0x40db60
bool ScreenSaver::keyUp(int theKey)
{
	Game::instance()->endProgram();
	return true;
}
