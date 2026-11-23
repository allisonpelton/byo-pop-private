#include "HighScoreEntryLayout.h"
#include "FishFood.h"
#include "GameSession.h"
#include "TextItem.h"

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The nodes of the layout the screen keeps, by name
const char* const SCORE_TEXT_NAME = "scoreText";
const char* const NAME_TEXT_NAME = "nameText";
// The action that tells the name is entered
const char* const DONE_ACTION = "highScoreEntryDone";
// The keys that make a letter, from the space up to the last that takes one; and the other two the box answers to
const int FIRST_LETTER_KEY = 0x20;
const int LAST_LETTER_KEY = 0x87;
const int BACKSPACE_KEY = 8;
const int ENTER_KEY = 0xd;
// The longest name
const size_t MAX_NAME_LENGTH = 15;

bool isLetterKey(int theKey)
{
	return theKey >= FIRST_LETTER_KEY && theKey <= LAST_LETTER_KEY;
}
}

// 0x4342b0
HighScoreEntryLayout::HighScoreEntryLayout() : mScore(0), mNameText(nullptr)
{
	setEnabled(true);
}

// 0x433ff0
void HighScoreEntryLayout::setUp(const std::string& theDefaultName)
{
	setUp(theDefaultName, FishFood::instance()->getSession()->getScore());
}

// 0x433ff0
void HighScoreEntryLayout::setUp(const std::string& theDefaultName, int theScore)
{
	mName = theDefaultName;
	mScore = theScore;
	static_cast<TextItem*>(getNode(SCORE_TEXT_NAME))->setNumber(mScore);
	mNameText = static_cast<TextItem*>(getNode(NAME_TEXT_NAME));
	mNameText->setText(mName);
}

// 0x42d3c0
std::string HighScoreEntryLayout::getEnteredName()
{
	return mName;
}

// 0x4340d0
bool HighScoreEntryLayout::keyDown(int theKey)
{
	if (isLetterKey(theKey))
	{
		if (mName.size() < MAX_NAME_LENGTH)
		{
			mName.append(1, (char)theKey);
			mNameText->setText(mName);
		}
		return true;
	}
	if (theKey == BACKSPACE_KEY)
	{
		if (!mName.empty())
		{
			mName = mName.substr(0, mName.size() - 1);
			mNameText->setText(mName);
		}
		return true;
	}
	if (theKey == ENTER_KEY)
	{
		sendAction(DONE_ACTION);
		return true;
	}
	return false;
}

// 0x433fd0
bool HighScoreEntryLayout::keyUp(int theKey)
{
	return isLetterKey(theKey);
}
