#include "HighScoreLayout.h"
#include "App.h"
#include "FishFood.h"
#include "FoodBank.h"
#include "FoodBankRank.h"
#include "Font.h"
#include "GameStrings.h"
#include "TextItem.h"

#include <stdio.h>

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The nodes of the layout the screen keeps, by name
const char* const MODE_TEXT_NAME = "modeText";
const char* const FOOD_BANK_GROUP_NAME = "foodBankGrp";
const char* const FOOD_BANK_TOTAL_NAME = "foodBankTotal";
const char* const FOOD_BANK_RANK_NAME = "foodBankRank";
// A score's three nodes are named for its place on the screen, from 0
const char* const RANK_NAME_FORMAT = "scoreRank%d";
const char* const NAME_NAME_FORMAT = "scoreName%d";
const char* const SCORE_NAME_FORMAT = "score%d";
// The place shows as its number and a point
const char* const RANK_TEXT_FORMAT = "%d.";
// The texts of the two modes
const char* const NORMAL_MODE_TEXT = "normalMode";
const char* const TIMED_MODE_TEXT = "timeAttack";
// The font the names are measured in, and the dots that lead from a name to its score
const char* const SCORE_FONT_FILE = "resources/hud/textFont.xml";
const char* const NAME_LEADER = " .";
const int NAME_WIDTH = 265;
// How many scores the screen has room for
const int SCORES_SHOWN = 10;
// The mode of the normal game
const int NORMAL_MODE = 0;

// A node of the screen for a place of the list
TextItem* getScoreNode(ScreenLayout* theScreen, const char* theFormat, int thePlace)
{
	char aName[32];
	snprintf(aName, sizeof(aName), theFormat, thePlace);
	return static_cast<TextItem*>(theScreen->getNode(aName));
}
}

// 0x434b90
HighScoreLayout::HighScoreLayout() : mNormalList(nullptr), mTimedList(nullptr), mShownList(nullptr), mFirstRank(0)
{
}

// 0x4343c0
void HighScoreLayout::setLists(ScoreList* theNormalList, ScoreList* theTimedList)
{
	mNormalList = theNormalList;
	mTimedList = theTimedList;
	mShownList = theNormalList;
	mFirstRank = 0;
}

// 0x434460
void HighScoreLayout::refresh()
{
	FishFood* aFood = FishFood::instance();
	TextItem* aModeText = static_cast<TextItem*>(getNode(MODE_TEXT_NAME));
	if (mShownList == mNormalList)
	{
		aModeText->setText(aFood->getStrings().get(NORMAL_MODE_TEXT));
		getNode(FOOD_BANK_GROUP_NAME)->setVisible(true);
		static_cast<TextItem*>(getNode(FOOD_BANK_TOTAL_NAME))->setNumber(aFood->getFoodBank().getCount());
		static_cast<TextItem*>(getNode(FOOD_BANK_RANK_NAME))->setText(getFoodBankRankName(aFood->getFoodBank()));
	}
	else
	{
		aModeText->setText(aFood->getStrings().get(TIMED_MODE_TEXT));
		getNode(FOOD_BANK_GROUP_NAME)->setVisible(false);
	}

	Font* aFont = App::instance()->getFont(SCORE_FONT_FILE);
	int aLeaderWidth = aFont->getStringWidth(NAME_LEADER);
	for (int aRank = mFirstRank; aRank < mFirstRank + SCORES_SHOWN; aRank++)
	{
		int aPlace = aRank - mFirstRank;
		TextItem* aRankText = getScoreNode(this, RANK_NAME_FORMAT, aPlace);
		TextItem* aNameText = getScoreNode(this, NAME_NAME_FORMAT, aPlace);
		TextItem* aScoreText = getScoreNode(this, SCORE_NAME_FORMAT, aPlace);
		if (aRank < mShownList->getCount())
		{
			const ScoreEntry& anEntry = mShownList->getEntry(aRank);
			char aRankLabel[16];
			snprintf(aRankLabel, sizeof(aRankLabel), RANK_TEXT_FORMAT, aRank + 1);
			aRankText->setText(aRankLabel);
			aRankText->setVisible(true);

			// dots fill the space between the name and the score
			std::string aName = anEntry.mName;
			int aLeaderCount = (NAME_WIDTH - aFont->getStringWidth(aName.c_str())) / aLeaderWidth;
			for (int aLeader = 0; aLeader < aLeaderCount; aLeader++)
				aName += NAME_LEADER;
			aNameText->setText(aName);
			aNameText->setVisible(true);

			aScoreText->setNumber((int)anEntry.mScore);
			aScoreText->setVisible(true);
		}
		else
		{
			aRankText->setVisible(false);
			aNameText->setVisible(false);
			aScoreText->setVisible(false);
		}
	}
}

// 0x434420
void HighScoreLayout::setMode(int theMode)
{
	mShownList = theMode == NORMAL_MODE ? mNormalList : mTimedList;
	mFirstRank = 0;
	refresh();
}

// 0x4343f0
void HighScoreLayout::nextPage()
{
	mShownList = mShownList == mNormalList ? mTimedList : mNormalList;
	mFirstRank = 0;
	refresh();
}

// 0x4343f0
void HighScoreLayout::prevPage()
{
	nextPage();
}
