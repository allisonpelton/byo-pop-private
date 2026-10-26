#include "Screens.h"
#include "App.h"
#include "Game.h"
#include "TextItem.h"
#include <cstdio>

using namespace engine;

// 0x409c20
CongratsScreen::CongratsScreen()
{
	setName("CongratsScreen");
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
