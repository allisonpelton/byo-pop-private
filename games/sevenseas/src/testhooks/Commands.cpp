// Test hooks: the standard script commands (docs/TEST_COMMANDS.md), reachable only from -script (and the later live
// channel). A command that is never given changes nothing: no draw, no state. Every command answers with one line on
// stdout: OK <command>, ERROR <command>: <reason>, STATE {...} or MOVE {...}.
//
//   state                 STATE {json}: screen, mode, level, score, lives, kills and a board summary
//   goto <screen>         menu, options, help, highscores (after a game), or a mode: easy, average, hard, tutorial
//   level <n>             starts level n of the current game, as -autoplaylevel does
//   set <name> <value>    score or lives
//   win, lose             the level is complete now; the last life is lost now (the game over follows as in play)
//   seed <n>              the game's random numbers (the C library generator) are seeded with n
//   move                    MOVE {json}: the bot's next move (Bot.h) and the real input that makes it; changes nothing
//   unlock all            ERROR: the game has nothing locked

#include "SevenSeasApp.h"
#include "Bot.h"
#include "Board.h"
#include "Screens.h"
#include "Demo.h"
#include "Dialogs.h"
#include "Compat2004.h"
#include "WidgetManager.h"
#include "Dialog.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

using namespace Sexy;

namespace
{

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void Ok(const std::string& theCommand)
{
	Reply("OK " + theCommand);
}

void Error(const std::string& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand + ": " + theReason);
}

std::string Num(int theValue)
{
	char aBuf[32];
	sprintf(aBuf, "%d", theValue);
	return aBuf;
}

const char* ScreenName(SevenSeasApp* theApp)
{
	if (theApp->mTitleScreen != NULL)
		return "title";
	if (theApp->GetDialog(DIALOG_OPTIONS) != NULL)
		return "options";
	if (theApp->mHelpWidget != NULL)
		return "help";
	if (theApp->mHighScoreWidget != NULL)
		return "highscores";
	if (theApp->mMapWidget != NULL)
		return "map";
	if (theApp->mDemoWidget != NULL)
		return "demo";
	if (theApp->mIntroWidget != NULL)
		return "menu";
	if (theApp->mBoard != NULL)
		return theApp->mBoard->mBonus ? "bonus" : "board";
	return "none";
}

bool CenterOf(Widget* w, int* x, int* y);
bool GameOver(Board* b);
bool BotCanMove(Board* b);
bool UiClick(SevenSeasApp* theApp, std::string* theWhat, int* x, int* y);

std::string StateJson(SevenSeasApp* theApp)
{
	Board* b = theApp->mBoard;
	std::string s = std::string("{\"screen\":\"") + ScreenName(theApp) + "\"";
	if (theApp->mTutorial)
		s += ",\"mode\":\"tutorial\"";
	else
	{
		static const char* const kModes[] = {"easy", "average", "hard"};
		int d = theApp->mDifficulty;
		s += std::string(",\"mode\":\"") + ((d >= 1 && d <= 3) ? kModes[d - 1] : "none") + "\"";
	}
	// the dialogs that are up: id, whether it sits in the widget manager, its box, and its button's middle
	s += ",\"dialogs\":[";
	bool aFirstDialog = true;
	for (int anId = 0; anId < 32; anId++)
	{
		Dialog* aDialog = theApp->GetDialog(anId);
		if (aDialog == NULL)
			continue;
		int bx = -1, by = -1;
		CenterOf(aDialog->mYesButton, &bx, &by);
		s += std::string(aFirstDialog ? "" : ",") + "{\"id\":" + Num(anId) + ",\"managed\":" + (aDialog->mWidgetManager != NULL ? "true" : "false") +
			",\"visible\":" + (aDialog->mVisible ? "true" : "false") + ",\"box\":[" + Num(aDialog->mX) + "," + Num(aDialog->mY) + "," +
			Num(aDialog->mWidth) + "," + Num(aDialog->mHeight) + "],\"button\":[" + Num(bx) + "," + Num(by) + "]}";
		aFirstDialog = false;
	}
	s += "]";
	if (theApp->mIntroWidget != NULL)
	{
		// the menu's buttons, in game coordinates: the play button, the tutorial button, the three difficulty wheels
		int x, y;
		s += ",\"menu\":{\"play\":";
		s += CenterOf(theApp->mIntroWidget->mPlayButton, &x, &y) ? "[" + Num(x) + "," + Num(y) + "]" : "null";
		s += ",\"tutorial\":";
		s += CenterOf(theApp->mIntroWidget->mTutorialButton, &x, &y) ? "[" + Num(x) + "," + Num(y) + "]" : "null";
		s += ",\"wheels\":[";
		for (int i = 0; i < 3; i++)
			if (CenterOf(theApp->mIntroWidget->mWheels[i], &x, &y))
				s += std::string(i ? "," : "") + "[" + Num(x) + "," + Num(y) + "]";
		s += "]}";
	}
	if (b != NULL)
	{
		s += std::string(",\"over\":") + (GameOver(b) ? "true" : "false");
		// busy: nothing to click yet (a ship sails, a level ends), the game goes on by itself
		std::string aWhat;
		int aUX, aUY;
		s += std::string(",\"busy\":") + ((!GameOver(b) && !BotCanMove(b) && !UiClick(theApp, &aWhat, &aUX, &aUY)) ? "true" : "false");
		s += ",\"level\":" + Num(b->mLevel) + ",\"score\":" + Num(b->mScore) + ",\"lives\":" + Num(b->mLives) +
			",\"kills\":" + Num(b->mKillsTotal);
		s += ",\"board\":{\"player\":[" + Num(b->mPlayerX) + "," + Num(b->mPlayerY) + "],\"heading\":" + Num(b->mHeading) +
			",\"state\":" + Num(b->mState) + ",\"levelOver\":" + (b->mLevelOver ? "true" : "false") + ",\"ships\":[";
		bool aFirst = true;
		for (std::list<Ship*>::iterator anItr = b->mShips.begin(); anItr != b->mShips.end(); ++anItr)
		{
			s += std::string(aFirst ? "" : ",") + "[" + Num((*anItr)->mType) + "," + Num((*anItr)->mX) + "," + Num((*anItr)->mY) + "]";
			aFirst = false;
		}
		s += "],\"islands\":[";
		aFirst = true;
		std::string aWrecks;
		for (int x = 0; x < GRID; x++)
			for (int y = 0; y < GRID; y++)
			{
				if (b->mTerrain[x][y] != 0)
				{
					s += std::string(aFirst ? "" : ",") + "[" + Num(x) + "," + Num(y) + "," + Num(b->mTerrain[x][y]) + "]";
					aFirst = false;
				}
				if (b->mVisited[x][y])
					aWrecks += std::string(aWrecks.empty() ? "" : ",") + "[" + Num(x) + "," + Num(y) + "]";
			}
		s += "],\"wrecks\":[" + aWrecks + "],\"whirlpools\":[";
		aFirst = true;
		for (std::list<Port*>::iterator anItr = b->mPorts.begin(); anItr != b->mPorts.end(); ++anItr)
		{
			s += std::string(aFirst ? "" : ",") + "[" + Num((*anItr)->mX) + "," + Num((*anItr)->mY) + "," + ((*anItr)->mActive ? "1" : "0") + "]";
			aFirst = false;
		}
		s += "]}";
	}
	return s + "}";
}

// the bot may move only when the ship waits for a click
bool BotCanMove(Board* b)
{
	return (b != NULL) && !b->mBonus && ((b->mState == STATE_IDLE) || (b->mState == STATE_BLINK)) && !b->mLevelOver &&
		(b->mPlayerX >= 0);
}

// the middle of a widget, in game coordinates (every widget of the game sits in the one widget manager)
bool CenterOf(Widget* w, int* x, int* y)
{
	if (w == NULL || !w->mVisible || w->mDisabled || w->mWidth <= 0)
		return false;
	*x = w->mX + w->mWidth / 2;
	*y = w->mY + w->mHeight / 2;
	return true;
}

// the click that carries the game on when no ship waits for a move: the title's Play link, a dialog's button, the
// menu's Play, the demo's and the map's buttons, and the bonus level's digging (what AutoplayUpdate does by calls)
bool UiClick(SevenSeasApp* theApp, std::string* theWhat, int* x, int* y)
{
	if (theApp->mTitleScreen != NULL)
	{
		*theWhat = "title";
		return theApp->mTitleScreen->mPlayShown && CenterOf(theApp->mTitleScreen->mPlayLink, x, y);
	}
	for (int anId = 0; anId < 32; anId++)
	{
		Dialog* aDialog = theApp->GetDialog(anId);
		if (aDialog != NULL)
		{
			*theWhat = "dialog " + Num(anId);
			return CenterOf(aDialog->mYesButton, x, y) || CenterOf(aDialog->mNoButton, x, y);
		}
	}
	if (theApp->mMapWidget != NULL)
	{
		*theWhat = "map";
		return CenterOf(((MapWidget*) theApp->mMapWidget)->mNextButton, x, y);
	}
	if (theApp->mDemoWidget != NULL)
	{
		*theWhat = "demo";
		return CenterOf(theApp->mDemoWidget->mButton, x, y);
	}
	if (theApp->mIntroWidget != NULL)
	{
		*theWhat = "menu";
		return !theApp->mIntroWidget->mStarting && CenterOf(theApp->mIntroWidget->mPlayButton, x, y);
	}
	if (theApp->mHighScoreWidget != NULL)
	{
		*theWhat = "highscores";
		return CenterOf(theApp->mHighScoreWidget->mNewGameButton, x, y);
	}
	Board* b = theApp->mBoard;
	if (b != NULL && b->mBonus && b->mState != STATE_LEVEL_END && !b->mLevelOver)
	{
		*theWhat = "bonus";
		if (b->mBonusState != 0)
		{
			*x = b->mX + Board::CellToPixel(5);
			*y = b->mY + Board::CellToPixel(5) + 12;
		}
		else
		{
			// a square of the bonus screen that changes with every ask, so that a dug square is not asked for again
			unsigned int aDice = (unsigned int) (theApp->mUpdateCount * 2654435761u + b->mScore);
			*x = b->mX + Board::CellToPixel(1 + (aDice >> 8) % 9) + 5 + 21;
			*y = b->mY + Board::CellToPixel(1 + (aDice >> 16) % 9) + 0x12 + 21;
		}
		return true;
	}
	return false;
}

// the game is over: the last life is gone
bool GameOver(Board* b)
{
	return b != NULL && b->mLives < 1;
}

}

bool SevenSeasApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	std::istringstream anArgs(theCommand.mArgs);
	std::string anArg;
	anArgs >> anArg;
	std::string aLine = aName + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);

	if (aName == "state")
	{
		Reply("STATE " + StateJson(this));
		return true;
	}
	if (aName == "goto")
	{
		if (mTitleScreen != NULL)
			Error(aLine, "the title screen is still up");
		else if (anArg == "title" || anArg == "credits")
			Error(aLine, "the game has no " + anArg + " screen to go to");
		else if (anArg == "menu")
		{
			ShowIntro();
			Ok(aLine);
		}
		else if (anArg == "options")
		{
			ShowOptions();
			Ok(aLine);
		}
		else if (anArg == "help")
		{
			ShowHelp();
			Ok(aLine);
		}
		else if (anArg == "highscores")
		{
			if (mBoard == NULL)
				Error(aLine, "no game has been played yet");
			else
			{
				ShowHighScoreWidget();
				Ok(aLine);
			}
		}
		else if (anArg == "easy" || anArg == "average" || anArg == "hard" || anArg == "tutorial")
		{
			if (mIntroWidget == NULL)
				ShowIntro();
			if (mIntroWidget == NULL || mIntroWidget->mStarting)
				Error(aLine, "the menu cannot start a game now");
			else
			{
				if (anArg == "tutorial")
					mIntroWidget->ButtonDepress(1);
				else
				{
					mIntroWidget->SetDifficulty(anArg == "easy" ? 1 : anArg == "average" ? 2 : 3);
					mIntroWidget->ButtonDepress(0);
				}
				Ok(aLine);
			}
		}
		else
			Error(aLine, "unknown screen '" + anArg + "'");
		return true;
	}
	if (aName == "level")
	{
		int aLevel = atoi(anArg.c_str());
		if (mBoard == NULL || mTutorial || mDemoWidget != NULL || mTitleScreen != NULL)
			Error(aLine, "no game is being played");
		else if (aLevel < 1)
			Error(aLine, "the level must be 1 or more");
		else
		{
			mBoard->mLevel = aLevel - 1;
			if (mBoard->mLives < 1)
				mBoard->mLives = 1;
			mBoard->mLevelOver = false;
			mBoard->NewLevel();
			Ok(aLine);
		}
		return true;
	}
	if (aName == "set")
	{
		std::string aValue;
		anArgs >> aValue;
		if (mBoard == NULL || mTutorial)
			Error(aLine, "no game is being played");
		else if (aValue.empty())
			Error(aLine, "set <name> <value>");
		else if (anArg == "score")
		{
			mBoard->mScore = atoi(aValue.c_str());
			mBoard->mDirty = true;
			Ok(aLine);
		}
		else if (anArg == "lives")
		{
			mBoard->mLives = atoi(aValue.c_str());
			mBoard->mDirty = true;
			Ok(aLine);
		}
		else
			Error(aLine, "the game's state keys are score and lives");
		return true;
	}
	if (aName == "win")
	{
		if (mBoard == NULL || mTutorial || mBoard->mBonus || mBoard->mState == STATE_LEVEL_END)
			Error(aLine, "no level is being played");
		else
		{
			mBoard->mLevelComplete = true;   // what the board sets when the last pirate is gone (0x405700)
			Ok(aLine);
		}
		return true;
	}
	if (aName == "lose")
	{
		if (mTutorial || !BotCanMove(mBoard))
			Error(aLine, "no ship is waiting for a move");
		else
		{
			mBoard->mLives = 1;
			mBoard->LoseLife();   // the last life, as a broadside or a collision takes it: the game over follows
			Ok(aLine);
		}
		return true;
	}
	if (aName == "unlock")
	{
		Error(aLine, "the game has nothing locked");
		return true;
	}
	if (aName == "seed")
	{
		if (anArg.empty())
			Error(aLine, "seed <n>");
		else
		{
			CrtSRand((unsigned int) atoi(anArg.c_str()));
			Ok(aLine);
		}
		return true;
	}
	if (aName == "move")
	{
		// a dialog, the demo, the map and the like come first: the ship waits behind them
		std::string aWhat;
		int aUX, aUY;
		if (!GameOver(mBoard) && UiClick(this, &aWhat, &aUX, &aUY))
		{
			Reply("MOVE {\"kind\":\"ui\",\"what\":\"" + aWhat + "\",\"input\":[{\"move\":[" + Num(aUX) + "," + Num(aUY) + "]},{\"click\":[" +
				Num(aUX) + "," + Num(aUY) + "]}]}");
			return true;
		}
		if (!BotCanMove(mBoard))
		{
			Reply("MOVE null");
			return true;
		}
		// the bot's dice start from the position, so the same position gives the same move and nothing is drawn
		unsigned int aSeed = (unsigned int) (mBoard->mLevel * 131 + mBoard->mScore * 17 + mBoard->mPlayerX * 7 + mBoard->mPlayerY);
		BotChoice aChoice = ChooseBotMove(mBoard, aSeed);
		int aGX, aGY;
		BotCellToGame(mBoard, aChoice.mX, aChoice.mY, &aGX, &aGY);
		static const char* const kKinds[] = {"fire", "wait", "sail"};
		Reply(std::string("MOVE {\"kind\":\"") + kKinds[aChoice.mKind] + "\",\"cell\":[" + Num(aChoice.mX) + "," + Num(aChoice.mY) +
			"],\"input\":[{\"move\":[" + Num(aGX) + "," + Num(aGY) + "]},{\"click\":[" + Num(aGX) + "," + Num(aGY) + "]}]}");
		return true;
	}
	return SexyApp::HandleScriptCommand(theCommand);
}
