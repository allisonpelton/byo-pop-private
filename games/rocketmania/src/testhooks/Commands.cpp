// Test hooks: the standard script commands (docs/TEST_COMMANDS.md), reachable only from -script and -commands. A command that
// is never given changes nothing: no draw, no state. Every command answers with one line on stdout: OK <command>,
// ERROR <command>: <reason>, STATE {...} or MOVE {...}.
//
//   state                 STATE {json}: screen, mode, difficulty, level, score, moves, coins, the rockets' counters and a board
//                         summary (one string a row, "<kind><rotation>" a cell, "--" an empty one)
//   goto <screen>         menu, options, highscores, credits, or a mode (arcade, classic, strategy) with its difficulty 1 to 3
//   level <n>             starts level n of the current game (a game of the last mode if there is none), as the game's own
//                         NextLevel does
//   set <name> <value>    score, level, moves (the strategy mode's), coins; difficulty (1 to 3, for the dialog a move answers)
//   win, lose             the level ends now (the head rides in on its rocket, the level-up screen follows); the game ends
//                         now, as the night running out, the top line filling or the moves running out do
//   seed <n>              the game's global generator is seeded with n (the board's generator comes from it at the next game)
//   move                  MOVE {json}: the bot's next move and the real input that makes it; changes nothing. On the board: the
//                         clicks that turn a path of tiles from the left edge to the right one (UiBot's planner); in the menu
//                         the next mode button, and the play link or the button of the dialog that is up elsewhere
//   unlock all            ERROR: the game has nothing locked

#include "RocketApp.h"
#include "Board.h"
#include "Dialogs.h"
#include "Screens.h"
#include "UiBot.h"
#include "WidgetManager.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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

const char* ModeName(int theMode)
{
	return theMode == 0 ? "arcade" : theMode == 1 ? "classic" : "strategy";
}

// the dialogs of the game, closed before a test command opens a screen of its own
void CloseDialogs(RocketApp* theApp)
{
	std::vector<int> anIds;
	for (DialogMap::iterator anItr = theApp->mDialogMap.begin(); anItr != theApp->mDialogMap.end(); ++anItr)
		anIds.push_back(anItr->first);
	for (size_t i = 0; i < anIds.size(); i++)
		theApp->KillDialog(anIds[i]);
}

// the way out of a game that has ended (state 2, the level-up screen of a game's end): QuitGame then shows the selector
void LeaveBoard(RocketApp* theApp)
{
	if (theApp->mBoard == NULL)
		return;
	theApp->mBoard->mState = 2;
	theApp->QuitGame();
}

void StartGame(RocketApp* theApp, int theMode, int theDifficulty)
{
	CloseDialogs(theApp);
	LeaveBoard(theApp);
	theApp->mMode = theMode;
	theApp->mDifficulty = theDifficulty;
	theApp->StartBoard(false);
}

void State(RocketApp* theApp)
{
	std::string aJson = "{";
	const char* aScreen = "menu";
	int aDialog = -1;
	if (theApp->mLoadingScreen != NULL)
		aScreen = "loading";
	else if (theApp->mBoard != NULL)
		aScreen = "board";
	if (!theApp->mDialogMap.empty())
		aDialog = theApp->mDialogMap.begin()->first;
	aJson += std::string("\"screen\":\"") + aScreen + "\"";
	if (aDialog >= 0)
		aJson += ",\"dialog\":" + Num(aDialog);
	Board* aBoard = theApp->mBoard;
	if (aBoard != NULL)
	{
		aJson += std::string(",\"mode\":\"") + ModeName(theApp->mMode) + "\"";
		aJson += ",\"difficulty\":" + Num(theApp->mDifficulty);
		aJson += ",\"level\":" + Num(aBoard->mLevel + 1);
		aJson += ",\"score\":" + Num(aBoard->mScore);
		aJson += ",\"moves\":" + Num(aBoard->m108);
		aJson += ",\"coins\":" + Num(aBoard->m8c[7]);
		aJson += ",\"boardState\":" + Num(aBoard->mState);
		aJson += ",\"paused\":" + Num(aBoard->m350);
		aJson += ",\"rockets\":[" + Num(aBoard->m74) + "," + Num(aBoard->m78) + "," + Num(aBoard->m7c) + "," + Num(aBoard->m80) + "]";
		aJson += ",\"board\":{\"rows\":" + Num(aBoard->mRows) + ",\"cols\":" + Num(aBoard->mCols) + ",\"grid\":[";
		for (int aRow = 0; aRow < aBoard->mRows; aRow++)
		{
			std::string aLine;
			for (int aColumn = 0; aColumn < aBoard->mCols; aColumn++)
			{
				Piece* aPiece = aBoard->mGrid[aRow][aColumn];
				if (aColumn > 0)
					aLine += " ";
				aLine += aPiece == NULL ? "--" : Num(aPiece->mKind) + Num(aPiece->mRotation);
			}
			aJson += std::string(aRow > 0 ? "," : "") + "\"" + aLine + "\"";
		}
		aJson += "]}";
	}
	aJson += "}";
	Reply("STATE " + aJson);
}

std::string ClickStep(Widget* theWidget)
{
	return "{\"click\":[" + Num(theWidget->mX + theWidget->mWidth / 2) + "," + Num(theWidget->mY + theWidget->mHeight / 2) + "]}";
}

int sDifficulty = 2;    // the difficulty a move picks in the difficulty dialog (set difficulty <n>)
int sNextMode = 0;      // the mode a move picks next in the menu

// what the player does next away from the board: the play link, the dialog that is up (the choices the bot makes), the
// next mode button of the menu. Empty when there is nothing to do yet.
bool ScreenMove(RocketApp* theApp, std::string& theWhat, std::string& theInput)
{
	static bool sNameTyped = false;
	if (theApp->mLoadingScreen != NULL)
	{
		if (!theApp->mLoadingScreen->mLinkShown)
			return false;
		theWhat = "the play link";
		theInput = "[" + ClickStep(theApp->mLoadingScreen->mPlayLink) + "]";
		return true;
	}
	if (theApp->GetDialogCount() > 0)
	{
		static const int sOrder[] = { DIALOG_RECORD_SCORE, DIALOG_LEVEL_UP, DIALOG_HIGH_SCORES, DIALOG_HINT, DIALOG_CONTINUE,
			DIALOG_DIFFICULTY, DIALOG_GAME_OVER, DIALOG_QUIT };
		int aFound = -1;
		for (size_t i = 0; i < sizeof(sOrder) / sizeof(sOrder[0]) && aFound < 0; i++)
			if (theApp->GetDialog(sOrder[i]) != NULL)
				aFound = sOrder[i];
		Widget* aTarget = NULL;
		Dialog* aDialog = aFound < 0 ? NULL : theApp->GetDialog(aFound);
		switch (aFound)
		{
		case DIALOG_RECORD_SCORE:
			if (!sNameTyped)
			{
				sNameTyped = true;
				theWhat = "the name for a high score";
				theInput = "[{\"key\":\"B\"},{\"key\":\"O\"},{\"key\":\"T\"},{\"key\":\"Return\"},{\"wait\":30}]";
				return true;
			}
			aTarget = aDialog->mYesButton;
			break;
		case DIALOG_LEVEL_UP:
		{
			HyperlinkWidget* aLink = ((LevelUpDialog*) aDialog)->mButton;
			if (aLink != NULL && aLink->mVisible && !aLink->mDisabled)
				aTarget = aLink;
			break;
		}
		case DIALOG_HIGH_SCORES:
		{
			HyperlinkWidget* aLink = ((HighScoreDisplayDialog*) aDialog)->mMenuButton;
			if (aLink != NULL && aLink->mVisible && !aLink->mDisabled)
				aTarget = aLink;
			break;
		}
		case DIALOG_HINT:
			aTarget = ((HintDialog*) aDialog)->mPlayButton;
			break;
		case DIALOG_CONTINUE:
			aTarget = ((ContinueDialog*) aDialog)->mNewButton;
			break;
		case DIALOG_DIFFICULTY:
			aTarget = ((DifficultyDialog*) aDialog)->mButtons[sDifficulty - 1];
			break;
		case DIALOG_GAME_OVER:
		case DIALOG_QUIT:
			aTarget = aDialog->mNoButton;
			break;
		default:
			// some other dialog: its yes button
			if (!theApp->mDialogMap.empty())
				aTarget = theApp->mDialogMap.begin()->second->mYesButton;
			break;
		}
		if (aTarget == NULL)
			return false;
		theWhat = "a dialog's button";
		theInput = "[" + ClickStep(aTarget) + "]";
		return true;
	}
	sNameTyped = false;
	if (theApp->mBoard == NULL && theApp->mGameSelector != NULL)
	{
		theWhat = "a game mode of the menu";
		theInput = "[" + ClickStep(theApp->mGameSelector->mModeButtons[sNextMode]) + "]";
		sNextMode = (sNextMode + 1) % 3;
		return true;
	}
	return false;
}

// the bot's next move: away from the board what ScreenMove says; on it the clicks that turn a path of tiles across it, each a
// few updates after the one before
void Move(RocketApp* theApp)
{
	std::string aWhat, anInput;
	Board* aBoard = theApp->mBoard;
	if (ScreenMove(theApp, aWhat, anInput))
	{
		Reply("MOVE {\"what\":\"" + aWhat + "\",\"input\":" + anInput + "}");
		return;
	}
	if (aBoard == NULL || theApp->GetDialogCount() > 0 || aBoard->mState != 0 || aBoard->m2e8 || aBoard->m350 != 0)
	{
		Reply("MOVE null");
		return;
	}
	static UiBot* sBot = NULL;
	if (sBot == NULL)
	{
		// the bot's constructor marks the input as scripted, which a driver's real input must not meet
		bool aScripted = theApp->mScriptedInput;
		sBot = new UiBot(theApp, UiBotParams());
		theApp->mScriptedInput = aScripted;
	}
	if (!sBot->PlanPath(aBoard))
	{
		Reply("MOVE null");
		return;
	}
	anInput = "[";
	for (size_t i = 0; i < sBot->mQueue.size(); i++)
	{
		const UiBot::Click& aClick = sBot->mQueue[i];
		if (i > 0)
			anInput += ",{\"wait\":3},";
		anInput += "{\"click\":[" + Num(aClick.mX) + "," + Num(aClick.mY) + "]";
		if (aClick.mButton < 0)
			anInput += ",\"button\":\"right\"";
		anInput += "}";
	}
	anInput += "]";
	sBot->mQueue.clear();
	Reply("MOVE {\"what\":\"turn a path of tiles from the left edge to the right\",\"input\":" + anInput + "}");
}

}

bool RocketStandardCommand(RocketApp* theApp, const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	const std::string& aArgs = theCommand.mArgs;
	if (aName == "state")
		State(theApp);
	else if (aName == "move")
		Move(theApp);
	else if (aName == "unlock")
		Error("unlock", "the game has nothing locked");
	else if (aName == "seed")
	{
		if (aArgs.empty())
			Error("seed", "seed needs a number");
		else
		{
			SeedGlobalRand((unsigned long) strtoul(aArgs.c_str(), NULL, 10));
			Ok("seed");
		}
	}
	else if (aName == "goto")
	{
		char aWhere[32] = "";
		int aDifficulty = 2;
		sscanf(aArgs.c_str(), "%31s %d", aWhere, &aDifficulty);
		std::string aTo = aWhere;
		if (theApp->mLoadingScreen != NULL)
			Error("goto", "the loading screen is still up (startgame clicks its play link)");
		else if (aDifficulty < 1 || aDifficulty > 3)
			Error("goto", "the difficulty is 1 to 3");
		else if (aTo == "menu")
		{
			CloseDialogs(theApp);
			LeaveBoard(theApp);
			Ok("goto");
		}
		else if (aTo == "options")
		{
			CloseDialogs(theApp);
			theApp->ShowOptions(theApp->mBoard == NULL);
			Ok("goto");
		}
		else if (aTo == "highscores")
		{
			CloseDialogs(theApp);
			theApp->ShowHighScores(1);
			Ok("goto");
		}
		else if (aTo == "credits")
		{
			CloseDialogs(theApp);
			theApp->ShowCredits();
			Ok("goto");
		}
		else if (aTo == "arcade" || aTo == "classic" || aTo == "strategy")
		{
			StartGame(theApp, aTo == "arcade" ? 0 : aTo == "classic" ? 1 : 2, aDifficulty);
			Ok("goto");
		}
		else
			Error("goto", "unknown screen '" + aTo + "' (menu, options, highscores, credits, arcade, classic, strategy)");
	}
	else if (aName == "level")
	{
		int aLevel = atoi(aArgs.c_str());
		if (aLevel < 1)
			Error("level", "level needs a number from 1");
		else
		{
			if (theApp->mBoard == NULL)
				StartGame(theApp, theApp->mMode, theApp->mDifficulty > 0 ? theApp->mDifficulty : 2);
			else
				CloseDialogs(theApp);
			Board* aBoard = theApp->mBoard;
			if (aBoard == NULL)
				Error("level", "no game could be started");
			else
			{
				aBoard->mLevel = aLevel - 2;
				aBoard->NextLevel();
				Ok("level");
			}
		}
	}
	else if (aName == "set")
	{
		char aKey[32] = "";
		int aValue = 0;
		Board* aBoard = theApp->mBoard;
		if (sscanf(aArgs.c_str(), "%31s %d", aKey, &aValue) < 2)
			Error("set", "set needs a name and a number (score, level, moves, coins)");
		else if (std::string(aKey) == "difficulty" && aValue >= 1 && aValue <= 3)
		{
			sDifficulty = aValue;
			Ok("set");
		}
		else if (aBoard == NULL)
			Error("set", "there is no game");
		else
		{
			std::string aName2 = aKey;
			if (aName2 == "score")
				aBoard->mScore = aValue;
			else if (aName2 == "level")
				aBoard->mLevel = aValue - 1;
			else if (aName2 == "moves")
				aBoard->m108 = aValue;
			else if (aName2 == "coins")
				aBoard->m8c[7] = aValue;
			else
				aName2 = "";
			if (aName2.empty())
				Error("set", std::string("unknown name '") + aKey + "' (score, level, moves, coins)");
			else
				Ok("set");
		}
	}
	else if (aName == "win")
	{
		Board* aBoard = theApp->mBoard;
		if (aBoard == NULL || aBoard->mState != 0)
			Error("win", "there is no level in play");
		else
		{
			aBoard->EndLevelA();
			Ok("win");
		}
	}
	else if (aName == "lose")
	{
		Board* aBoard = theApp->mBoard;
		if (aBoard == NULL || aBoard->mState != 0)
			Error("lose", "there is no level in play");
		else
		{
			aBoard->F421de0(theApp->mMode == 0 ? 1 : theApp->mMode == 1 ? 0 : 2);
			Ok("lose");
		}
	}
	else
		return false;
	return true;
}
