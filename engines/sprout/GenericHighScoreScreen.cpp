#include "GenericHighScoreScreen.h"

using namespace engine;

namespace
{
// The seconds a held button waits before it scrolls again the first time (0x3ecccccd) and after that (0.1)
const float FIRST_REPEAT_TIME = 0.4f;
const float REPEAT_TIME = 0.1f;
}

// 0x46b880 (pizzafrenzy)
GenericHighScoreScreen::GenericHighScoreScreen() : mScrollDirection(0), mRepeatTimer(0)
{
}

// 0x46bbd0 (pizzafrenzy)
void GenericHighScoreScreen::setScoreboard(Scoreboard* theBoard)
{
	mScoreboard = theBoard;
	mTableModel = new ScoreboardTableModel(theBoard);
	mTable = static_cast<Table*>(getNode("scoreTable"));
	if (mTable != nullptr)
		mTable->setModel(mTableModel);
}

// 0x46b9a0 (pizzafrenzy)
void GenericHighScoreScreen::scroll()
{
	int aRows = mTable->getRows();
	int aRowCount = mTable->rowCount();
	int aTop = mTable->getTopRow() + mScrollDirection;
	if (-1 < aTop && aTop < (1 - aRows) + aRowCount)
		mTable->setTopRow(aTop);
}

// 0x46b9f0 (pizzafrenzy)
void GenericHighScoreScreen::update(UpdateInfo& theInfo)
{
	if (mScrollDirection != 0)
	{
		mRepeatTimer = mRepeatTimer - theInfo.mSeconds;
		if (mRepeatTimer <= 0.0f)
		{
			scroll();
			mRepeatTimer = mRepeatTimer + REPEAT_TIME;
		}
	}
	GenericScreen::update(theInfo);
}

// 0x46ba50 (pizzafrenzy)
void GenericHighScoreScreen::buttonDown(std::string theName)
{
	if (theName.compare("scoreScrollUp") == 0)
		mScrollDirection = -1;
	else if (theName.compare("scoreScrollDown") == 0)
		mScrollDirection = 1;
	else
		return;
	scroll();
	mRepeatTimer = FIRST_REPEAT_TIME;
}

// 0x46bb20 (pizzafrenzy)
void GenericHighScoreScreen::buttonUp(std::string theName)
{
	if (theName.compare("scoreScrollUp") == 0 || theName.compare("scoreScrollDown") == 0)
		mScrollDirection = 0;
}
