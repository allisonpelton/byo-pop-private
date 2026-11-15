#include "HighScoreScreen.h"
#include "App.h"
#include "GameText.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"
#include "Scoreboards.h"
#include "TextItem.h"

using namespace pizzafrenzy;

namespace
{
// The modes' boards by the mode (the application's +0x58), and the string table's text of each one's name
const int SPEED_MODE = 0;
const int MEMORY_MODE = 1;
const int CONCENTRATION_MODE = 2;
const int STRING_SPEED_MODE = 0xce;
const int STRING_MEMORY_MODE = 0xcd;
const int STRING_CONCENTRATION_MODE = 0xcf;

// The most scores a board keeps
const int BOARD_SIZE = 10;

// The first scores of a board with none: the points and the level of each (the names are the exe's)
struct FirstScore
{
	int mPoints;
	int mLevel;
};
const FirstScore FIRST_SCORES[] = {
	{40000, 9}, {35000, 8}, {30000, 7}, {25000, 6}, {20000, 5}, {15000, 4}, {10000, 3}, {5000, 2}};

// The scrolling arrows' actions
const char* const ACTION_MODE_LEFT = "toggleScoreModeLeft";
const char* const ACTION_MODE_RIGHT = "toggleScoreModeRight";

// The buttons' places after the button that led out of the game is gone: the same 200 apart, centred
const float DONE_BUTTON_X = -100.0f;
const float CLEAR_BUTTON_X = 100.0f;
}

// 0x4371b0
void HighScoreScreen::setScoreboard(engine::Scoreboard* theBoard)
{
	GenericHighScoreScreen::setScoreboard(theBoard);
	static_cast<engine::TextItem*>(getNode("highScoreModeText"))->setText(theBoard->getTitle());
}

// 0x437a10
void HighScoreScreen::init()
{
	// The original's layout has a button that switches to the online scores, which leads out of the game: it, the
	// online scores' title and the text on the other title go, and the two buttons left in the row close the gap, the
	// same 200 apart and centred. A button's text is the text with no name at the button's place and goes with it.
	removeScreenNode(this, "ToggleGlobal");
	removeScreenNode(this, "globalText");
	removeScreenNode(this, "toggleGlobalText");
	moveButtonWithLabels(this, "Done", DONE_BUTTON_X);
	moveButtonWithLabels(this, "ClearScores", CLEAR_BUTTON_X);

	engine::App* anApp = engine::App::instance();
	mBoards[MEMORY_MODE] = new HighScoreBoard("memoryMode", BOARD_SIZE);
	mBoards[MEMORY_MODE]->setTitle(anApp->loadString(STRING_MEMORY_MODE));
	mBoards[SPEED_MODE] = new HighScoreBoard("speedMode", BOARD_SIZE);
	mBoards[SPEED_MODE]->setTitle(anApp->loadString(STRING_SPEED_MODE));
	mBoards[CONCENTRATION_MODE] = new HighScoreBoard("concentrationMode", BOARD_SIZE);
	mBoards[CONCENTRATION_MODE]->setTitle(anApp->loadString(STRING_CONCENTRATION_MODE));
	loadBoards();
	mActionSignal.connect(this, &HighScoreScreen::onAction);
	mMode = 0;
	showLocalScores();
}

// 0x437050
void HighScoreScreen::action(std::string theAction)
{
	if (theAction.compare(ACTION_MODE_LEFT) == 0)
	{
		mMode--;
		if (mMode < 0)
			mMode = MODE_COUNT - 1;
		refresh();
	}
	else if (theAction.compare(ACTION_MODE_RIGHT) == 0)
	{
		mMode++;
		if (mMode == MODE_COUNT)
			mMode = 0;
		refresh();
	}
	else
		PizzaFrenzy::instance()->screenAction(theAction);
}

// 0x436f70
void HighScoreScreen::refresh()
{
	setScoreboard(mBoards[mMode]);
}

// 0x437750
void HighScoreScreen::showLocalScores()
{
	getNode("personalText")->setVisible(true);
	engine::Node* aClear = getNode("ClearScores");
	aClear->setColorMode(0);
	aClear->setEnabled(true);
	refresh();
}

// 0x436db0: the application's game session (+0x120, whose +0xc is the score) and its level (+0x54) and mode (+0x58) are
// translated next with the game
bool HighScoreScreen::addCurrentScore()
{
	return false;
}

// 0x436fd0
void HighScoreScreen::clear()
{
	mBoards[mMode]->clear();
	engine::Scoreboards::instance()->saveBoard(mBoards[mMode]);
	refresh();
}

// 0x437930
void HighScoreScreen::loadBoards()
{
	for (int i = 0; i < MODE_COUNT; i++)
	{
		engine::Scoreboards::instance()->loadBoard(mBoards[i]);
		fillDefaultScores(mBoards[i]);
	}
}

// 0x437260
void HighScoreScreen::fillDefaultScores(HighScoreBoard* theBoard)
{
	if (theBoard->isEmpty())
	{
		const std::string* aNames[] = {&GT(0x4fd804), &GT(0x4fd7fc), &GT(0x4fd400), &GT(0x4fd7f4), &GT(0x4fd7ec),
			&GT(0x4fd7e4), &GT(0x4fd7e0), &GT(0x4fd7dc)};
		for (size_t i = 0; i < sizeof(FIRST_SCORES) / sizeof(FIRST_SCORES[0]); i++)
			theBoard->add(new HighScore(*aNames[i], FIRST_SCORES[i].mPoints, FIRST_SCORES[i].mLevel));
	}
	engine::Scoreboards::instance()->saveBoard(theBoard);
}
