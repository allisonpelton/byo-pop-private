// Test hooks: the standard script commands (docs/TEST_COMMANDS.md), reachable only from a test (-script, -commands, and the
// web test page's command call). A command that is never given changes nothing: no draw, no state. Every command answers
// with one line on stdout: OK <command>, ERROR <command>: <reason>, STATE {...} or MOVE {...} | MOVE null.
//
//   state                 STATE {json}: screen, mode, expert, level, score, molecules, atoms, meter, combo, windowed, over, update
//   goto <screen>         menu (from the title screen, or from the page after a game), options, highscores, or a game to start:
//                         strategy, strategy-expert, action, action-expert (the menu's four buttons)
//   level <n>             starts level n (1 to 41) of the game in progress, as the level box's Continue would
//   set <name> <value>    score (the board's), mode (strategy or action) and expert (0 or 1): what the menu's `move` starts
//   win                   the level's meter is filled now (the level box follows, as in play)
//   lose                  the field is full now (the game over follows, as in play)
//   seed <n>              the game's random numbers (its own generator and the C library's) are seeded with n
//   move                  MOVE {json}: the bot's next move (testhooks/Autoplay.cpp, BotMoveJson) and the real input that
//                         makes it; changes nothing in the game
//   unlock all            ERROR: nothing is locked, every mode is open from the start
//   shot <file>           the next frame drawn is written to a PNG (the identical-play pass's scenarios)

#include "AtomicaApp.h"
#include "Board.h"
#include "Grid.h"
#include "Menu.h"
#include "Dialogs.h"
#include "HintBox.h"
#include "Screens.h"
#include "Compat2004.h"
#include "WidgetManager.h"

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
	snprintf(aBuf, sizeof(aBuf), "%d", theValue);
	return aBuf;
}

// the screen the player is looking at: the box on top, else the page under it
const char* ScreenName(AtomicaApp* theApp)
{
	if (theApp->mScoreDialog != NULL)
		return "highscore-entry";
	if (theApp->mOptionsDialog != NULL)
		return "options";
	if (theApp->mQuitDialog != NULL)
		return "quit";
	if (theApp->mHighScoresBox != NULL)
		return "highscores";
	if (theApp->mLevelUpDialog != NULL)
		return "levelup";
	if (!theApp->mHintBoxes.empty())
		return "hint";
	if (theApp->mMenu != NULL)
	{
		MainMenu* aMenu = (MainMenu*) theApp->mMenu;
		if (aMenu->mClosing)
			return "starting";
		return aMenu->mGameOver ? "gameover" : "menu";
	}
	if (theApp->mBoard != NULL)
		return "board";
	if (theApp->mTitleScreen != NULL)
		return "title";
	return "none";
}

// the game that ended: its page is up and complete, the entry box (if the score earned one) is done
bool GameIsOver(AtomicaApp* theApp)
{
	if ((theApp->mMenu == NULL) || (theApp->mScoreDialog != NULL))
		return false;
	MainMenu* aMenu = (MainMenu*) theApp->mMenu;
	return aMenu->mGameOver && aMenu->mEntryShown && aMenu->PanelShown();
}

std::string StateJson(AtomicaApp* theApp)
{
	std::string s = std::string("{\"screen\":\"") + ScreenName(theApp) + "\"";
	s += std::string(",\"mode\":\"") + (theApp->mMode == 1 ? "action" : "strategy") + "\",\"expert\":" + Num(theApp->mExpertMode != 0 ? 1 : 0);
	Board* b = theApp->mBoard;
	if ((b != NULL) && (b->mGrid != NULL))
	{
		Grid* g = b->mGrid;
		s += ",\"level\":" + Num(b->mMeter.mLevel) + ",\"score\":" + Num(b->mScore) + ",\"molecules\":" + Num(b->mStatMolecules) +
			",\"atoms\":" + Num(g->CountAtoms(false)) + ",\"combo\":" + Num(g->mCombo) + ",\"meter\":" + Num((int) b->mMeter.mValue) +
			",\"meterMax\":" + Num((int) b->mMeter.mMax) + ",\"levelOver\":" + (g->mGameOver ? "true" : "false") +
			",\"held\":" + (g->mSelected != NULL ? "true" : "false") + ",\"hints\":" + Num((int) theApp->mHintBoxes.size());
	}
	else if (theApp->mMenu != NULL)
	{
		// the page after a game shows what the game came to
		MainMenu* aMenu = (MainMenu*) theApp->mMenu;
		if (aMenu->mGameOver)
			s += ",\"level\":" + Num(aMenu->mLevelReached) + ",\"score\":" + Num(aMenu->mScore) + ",\"molecules\":" + Num(aMenu->mMolecules);
	}
	s += ",\"paused\":" + Num(theApp->mPauseCount) + ",\"windowed\":" + (theApp->mIsWindowed ? "true" : "false") +
		",\"update\":" + Num(theApp->mUpdateCount);
	s += std::string(",\"over\":") + (GameIsOver(theApp) ? "true" : "false") + "}";
	return s;
}

// the menu's start button ids: the strategy mode 0, expert 1, the action mode 2, expert 3
bool StartButton(const std::string& theName, int* theId)
{
	if (theName == "strategy")
		*theId = 0;
	else if (theName == "strategy-expert")
		*theId = 1;
	else if (theName == "action")
		*theId = 2;
	else if (theName == "action-expert")
		*theId = 3;
	else
		return false;
	return true;
}

}

bool AtomicaApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	// "shot <file>": the next frame that is drawn goes to a PNG as it is (drawing it again would take more random numbers and
	// step the animations that move in the draw). The script line stands at the update the frame follows.
	if (aName == "shot")
	{
		mPendingShot = theCommand.mArgs;
		return true;
	}

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
		MainMenu* aMenu = (MainMenu*) mMenu;
		int anId = 0;
		if (anArg == "menu")
		{
			if (mTitleScreen != NULL)
			{
				if (!mLoaded)
					Error(aLine, "the game is still loading");
				else if (mMenu != NULL)
					Error(aLine, "the menu is already coming");
				else
				{
					TitleScreenDone();
					Ok(aLine);
				}
			}
			else if (GameIsOver(this))
			{
				aMenu->ButtonPress(6);
				Ok(aLine);
			}
			else
				Error(aLine, "the title screen or the page after a game is needed");
		}
		else if (anArg == "options")
		{
			if ((mBoard == NULL) && (mMenu == NULL))
				Error(aLine, "no menu or game is up");
			else
			{
				ShowOptions();
				Ok(aLine);
			}
		}
		else if (anArg == "highscores")
		{
			if (mMenu == NULL)
				Error(aLine, "the high scores are shown from the menu");
			else
			{
				ShowHighScores();
				Ok(aLine);
			}
		}
		else if (StartButton(anArg, &anId))
		{
			if ((aMenu == NULL) || !aMenu->PanelShown() || aMenu->mGameOver)
				Error(aLine, "the main page of the menu must be up (goto menu first)");
			else
			{
				mMode = anId / 2;
				mExpertMode = anId % 2;
				aMenu->ButtonPress(anId);
				Ok(aLine);
			}
		}
		else if (anArg == "title")
			Error(aLine, "the title screen comes once, at the start");
		else if ((anArg == "help") || (anArg == "credits"))
			Error(aLine, "the game has no such screen of its own: the credits are in the options box, its help is the hint boxes");
		else
			Error(aLine, "goto <menu|options|highscores|strategy|strategy-expert|action|action-expert>");
		return true;
	}
	if (aName == "level")
	{
		int aLevel = atoi(anArg.c_str());
		if ((anArg.empty()) || (aLevel < 1) || (aLevel > 41))
			Error(aLine, "level <1 to 41>");
		else if ((mBoard == NULL) || (mBoard->mGrid == NULL) || mBoard->mGrid->mGameOver)
			Error(aLine, "a game must be in progress");
		else
		{
			// as the board does once the level box is closed: the meter and the field start over at the new level
			Board* b = mBoard;
			b->mMeter.mLevel = aLevel;
			b->mMeter.mMoleculesMade = aLevel;
			b->mNumColorsStart = (aLevel + 2 > 11) ? 11 : aLevel + 2;
			b->mGetReadyAlpha = 400;
			b->mMeter.Reset();
			b->mMeter.mOverTick = 0;
			b->mGrid->Reset(b->mNumColorsStart);
			Ok(aLine);
		}
		return true;
	}
	if (aName == "unlock")
	{
		Error(aLine, "nothing is locked: every mode is open from the start");
		return true;
	}
	if (aName == "set")
	{
		std::string aValue;
		anArgs >> aValue;
		if (anArg == "score")
		{
			if ((mBoard == NULL) || aValue.empty())
				Error(aLine, "set score <n>, in a game");
			else
			{
				mBoard->mScore = atoi(aValue.c_str());
				Ok(aLine);
			}
		}
		else if (anArg == "mode")
		{
			if ((aValue != "strategy") && (aValue != "action") && (aValue != "0") && (aValue != "1"))
				Error(aLine, "set mode <strategy|action>");
			else if (mBoard != NULL)
				Error(aLine, "the mode of a game in progress cannot change");
			else
			{
				mMode = (aValue == "action" || aValue == "1") ? 1 : 0;
				Ok(aLine);
			}
		}
		else if (anArg == "expert")
		{
			if ((aValue != "0") && (aValue != "1"))
				Error(aLine, "set expert <0|1>");
			else if (mBoard != NULL)
				Error(aLine, "the mode of a game in progress cannot change");
			else
			{
				mExpertMode = atoi(aValue.c_str());
				Ok(aLine);
			}
		}
		else
			Error(aLine, "set <score|mode|expert> <value>");
		return true;
	}
	if (aName == "win")
	{
		if ((mBoard == NULL) || (mBoard->mGrid == NULL) || mBoard->mGrid->mGameOver || mBoard->mMeter.mFull)
			Error(aLine, "a level must be in progress");
		else
		{
			// the meter fills the way molecules fill it: the level is made, the level box follows
			mBoard->mMeter.mFrozen = false;
			mBoard->mMeter.Add((int) mBoard->mMeter.mMax + 1, 0);
			Ok(aLine);
		}
		return true;
	}
	if (aName == "lose")
	{
		if ((mBoard == NULL) || (mBoard->mGrid == NULL) || mBoard->mGrid->mGameOver)
			Error(aLine, "a game must be in progress");
		else
		{
			// the field is full (Grid::AfterMove's ending, 0x4127c0)
			mBoard->mGrid->mGameOver = true;
			mBoard->mGrid->mGameOverTick = 0;
			mBoard->mGrid->FlushStats();
			Ok(aLine);
		}
		return true;
	}
	if (aName == "seed")
	{
		if (anArg.empty())
			Error(aLine, "seed <n>");
		else
		{
			mRand.Seed((unsigned int) atoi(anArg.c_str()));
			CrtSRand((unsigned int) atoi(anArg.c_str()));
			Ok(aLine);
		}
		return true;
	}
	if ((aName == "move") || (aName == "botmove"))
	{
		Reply("MOVE " + BotMoveJson());
		return true;
	}
	return SexyApp::HandleScriptCommand(theCommand);
}
