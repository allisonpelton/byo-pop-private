// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel through WinBMApp::HandleScriptCommand. A command that is never given changes nothing, draws no random
// number and moves no state. Each answers on stdout with one line: STATE {...}, MOVE {...}|null, OK <command> or
// ERROR <command>: <reason>. They act through the game's own code: the screens' own Show functions, StartGame,
// the board's own LevelComplete / GameOver / StartNextLevel.
//
// The set: state, goto <screen>, level <n>, unlock all, set <name> <value>, win, lose, seed <n>, move.
// GAME_NOTES.md, "Test commands" lists the keys they print and the names they take.

#include "WinBMApp.h"
#include "Board.h"
#include "Piece.h"
#include "PuzzleSolver.h"
#include "TitleScreen.h"
#include "GameSelector.h"
#include "HelpWidget.h"
#include "RecordScoreDialog.h"
#include "Compat12.h"
#include "Res.h"

#include "Dialog.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "HyperlinkWidget.h"

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

std::string CommandText(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + CommandText(theCommand));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + CommandText(theCommand) + ": " + theReason);
}

const char* ModeName(int theMode)
{
	switch (theMode)
	{
	case GAMETYPE_ACTION: return "action";
	case GAMETYPE_STRATEGY: return "strategy";
	case GAMETYPE_PUZZLE: return "puzzle";
	}
	return "?";
}

int ModeByName(const std::string& theName)
{
	if (theName == "action")
		return GAMETYPE_ACTION;
	if (theName == "strategy")
		return GAMETYPE_STRATEGY;
	if (theName == "puzzle")
		return GAMETYPE_PUZZLE;
	return -1;
}

const char* BoardStateName(int theState)
{
	switch (theState)
	{
	case STATE_PLAYING: return "playing";
	case STATE_LEVEL_DONE: return "level done";
	case STATE_LEVEL_COMPLETE: return "level complete";
	case STATE_PUZZLE_SWEEP: return "sweep";
	case STATE_LEVEL_CLEAR_FALL: return "clear fall";
	case STATE_GAME_OVER: return "game over";
	case STATE_GAME_OVER_SCORES: return "game over scores";
	case STATE_BOMB: return "bomb";
	case STATE_FROZEN: return "frozen";
	}
	return "?";
}

// What the bot (the move command) is to play: -1 for the app's own settings. set mode / set skill.
int gTargetMode = -1;
int gTargetSkill = -1;

// The dialog that is open, or -1: the ids that can be up in a test
const int kDialogIds[] = { DIALOG_NEW_GAME, DIALOG_OPTIONS, DIALOG_RECORD_SCORE, DIALOG_CONTINUE_GAME, DIALOG_QUIT,
	DIALOG_HIGH_SCORES, DIALOG_CREDITS, DIALOG_INFO, DIALOG_URL_FAILED, DIALOG_OPENING_BROWSER };

int OpenDialog(WinBMApp* theApp)
{
	for (int anId : kDialogIds)
	{
		if (theApp->GetDialog(anId) != NULL)
			return anId;
	}
	return -1;
}

// The board is over: the game-over table has slid in, the name (if any) is entered, "play again" is shown
bool IsOver(WinBMApp* theApp)
{
	Board* aBoard = theApp->mBoard;
	return (aBoard != NULL) && (aBoard->mState == STATE_GAME_OVER_SCORES) && (aBoard->mHighScoreSlide <= -146) &&
		(theApp->GetDialog(DIALOG_RECORD_SCORE) == NULL) && (aBoard->mContinueButton != NULL) &&
		(aBoard->mContinueButton->mWidth > 0);
}

// Every piece rests in its cell and none falls
bool IsSettled(Board* theBoard)
{
	if (theBoard->mPiecesMoving)
		return false;
	for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
		{
			Piece* aPiece = theBoard->mGrid[aRow][aCol];
			if ((aPiece != NULL) && ((aPiece->mX != (double) Board::ColX(aCol)) || (aPiece->mY != (double) Board::RowY(aRow))))
				return false;
		}
	}
	return true;
}

std::string Screen(WinBMApp* theApp)
{
	if (theApp->mTitleScreen != NULL)
		return "title";
	if (theApp->mHelpWidget != NULL)
		return "help";
	if (theApp->mGameSelector != NULL)
		return "selector";
	if (theApp->mBoard != NULL)
		return "board";
	return "none";
}

std::string StateJson(WinBMApp* theApp)
{
	Board* aBoard = theApp->mBoard;
	std::string aJson = "{";
	char aBuf[160];
	snprintf(aBuf, sizeof(aBuf), "\"screen\":\"%s\",\"dialog\":%d,\"mode\":\"%s\",\"skill\":%d,\"updates\":%d",
		Screen(theApp).c_str(), OpenDialog(theApp), ModeName(theApp->mGameType), theApp->mSkillLevel, theApp->mUpdateCount);
	aJson += aBuf;
	if (theApp->mTitleScreen != NULL)
		aJson += std::string(",\"loaded\":") + (theApp->mTitleScreen->mLoadDone ? "true" : "false");
	if (aBoard != NULL)
	{
		snprintf(aBuf, sizeof(aBuf), ",\"level\":%d,\"score\":%d,\"boardState\":\"%s\",\"bags\":%d,\"bagsNeeded\":%d,"
			"\"paused\":%s,\"settled\":%s,\"go\":%d",
			aBoard->mLevel + 1, aBoard->mScore, BoardStateName(aBoard->mState), aBoard->mBagsBanked, aBoard->mBagsNeeded,
			aBoard->mPauseCount > 0 ? "true" : "false", IsSettled(aBoard) ? "true" : "false", aBoard->mGoTimer);
		aJson += aBuf;
		if (theApp->mGameType == GAMETYPE_STRATEGY)
		{
			snprintf(aBuf, sizeof(aBuf), ",\"moves\":%d", aBoard->mMovesLeft);
			aJson += aBuf;
		}
		aJson += ",\"board\":[";
		for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
		{
			std::string aLine;
			for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
			{
				Piece* aPiece = aBoard->mGrid[aRow][aCol];
				if (aPiece == NULL)
					aLine += '.';
				else if (aPiece->mIsSpecial)
					aLine += (aPiece->mSpecialType == PIECE_MONEYBAG) ? 'B' : 'X';
				else
					aLine += "0123456789abcdef"[aPiece->mColor & 15];
			}
			aJson += (aRow > 0 ? ",\"" : "\"") + aLine + "\"";
		}
		aJson += "]";
	}
	aJson += std::string(",\"over\":") + (IsOver(theApp) ? "true" : "false") + "}";
	return aJson;
}

// The largest clickable group (the first in row order on a tie), and a coin of it as a screen point
bool BestGroup(Board* theBoard, int* theX, int* theY, int* theSize)
{
	int aMark[Board::BOARD_ROWS][Board::BOARD_COLS] = {};
	int aBestSize = 0;
	Piece* aBest = NULL;
	for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
		{
			Piece* aPiece = theBoard->mGrid[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mIsSpecial) || (aMark[aRow][aCol] != 0))
				continue;
			int aSize = 0;
			std::vector<std::pair<int, int>> aStack(1, std::make_pair(aRow, aCol));
			aMark[aRow][aCol] = 1;
			while (!aStack.empty())
			{
				std::pair<int, int> aCell = aStack.back();
				aStack.pop_back();
				aSize++;
				for (int d = 0; d < theBoard->GroupDirs(); d++)
				{
					int r = aCell.first + gGroupDirs[d][1];
					int c = aCell.second + gGroupDirs[d][0];
					if ((r < 0) || (r >= Board::BOARD_ROWS) || (c < 0) || (c >= Board::BOARD_COLS) || (aMark[r][c] != 0))
						continue;
					Piece* aNeighbour = theBoard->mGrid[r][c];
					if ((aNeighbour == NULL) || (aNeighbour->mIsSpecial) || (aNeighbour->mColor != aPiece->mColor))
						continue;
					aMark[r][c] = 1;
					aStack.push_back(std::make_pair(r, c));
				}
			}
			if ((aSize >= theBoard->mMinGroup) && (aSize > aBestSize))
			{
				aBestSize = aSize;
				aBest = aPiece;
			}
		}
	}
	if (aBest == NULL)
		return false;
	*theX = theBoard->mX + (int) aBest->mX + 14;
	*theY = theBoard->mY + (int) aBest->mY + 14;
	*theSize = aBestSize;
	return true;
}

// The bot's own Puzzle solver (the solver of mods/puzzle-solver, but the bot's, so the mod need not be on): it
// searches the settled board, as the mod does per tick, but here all at once, and keeps its plan from click to click
PuzzleSolver gSolver;

// The Puzzle solver's next click, as a cell of the board (true), when it has a plan that wins the level
bool SolverClick(Board* theBoard, int* theCol, int* theRow)
{
	PuzzleModel aModel;
	theBoard->GetSolverModel(&aModel);
	if ((gSolver.mStatus == PuzzleSolver::STATUS_NONE) || (gSolver.GetBoardHash() != aModel.Hash()))
		gSolver.SetBoard(aModel);
	int aLooked = 0;
	while ((gSolver.mStatus == PuzzleSolver::STATUS_THINKING) && (aLooked < PuzzleSolver::MAX_POSITIONS + 20000))
	{
		gSolver.Step(20000);
		aLooked += 20000;
	}
	if ((gSolver.mStatus != PuzzleSolver::STATUS_WINNABLE) || (gSolver.mPlan.empty()))
		return false;
	*theCol = gSolver.mPlan[0].mCol;
	*theRow = gSolver.mPlan[0].mRow - theBoard->SolverRowOffset();
	return true;
}

// MOVE {"kind":"<what>","clicks":[{"x":..,"y":..}],"keys":[..],"updates":n}: the real input that makes the move, in
// game coordinates, and the updates to wait after it
void ReplyMove(const char* theKind, const std::string& theClicks, const std::string& theKeys, int theUpdates)
{
	std::string aJson = std::string("MOVE {\"kind\":\"") + theKind + "\"";
	if (!theClicks.empty())
		aJson += ",\"clicks\":[" + theClicks + "]";
	if (!theKeys.empty())
		aJson += ",\"keys\":[" + theKeys + "]";
	char aBuf[32];
	snprintf(aBuf, sizeof(aBuf), ",\"updates\":%d}", theUpdates);
	Reply(aJson + aBuf);
}

std::string ClickAtPoint(int theX, int theY)
{
	char aBuf[48];
	snprintf(aBuf, sizeof(aBuf), "{\"x\":%d,\"y\":%d}", theX, theY);
	return aBuf;
}

std::string ClickOnWidget(Widget* theParent, Widget* theWidget)
{
	int aX = (theParent != NULL ? theParent->mX : 0) + theWidget->mX + theWidget->mWidth / 2;
	int aY = (theParent != NULL ? theParent->mY : 0) + theWidget->mY + theWidget->mHeight / 2;
	return ClickAtPoint(aX, aY);
}

void MoveCommand(WinBMApp* theApp)
{
	// The title screen: the play link once loading is done
	if (theApp->mTitleScreen != NULL)
	{
		HyperlinkWidget* aLink = theApp->mTitleScreen->mPlayLink;
		if ((theApp->mTitleScreen->mLoadDone) && (aLink != NULL) && (aLink->mVisible) && (aLink->mWidth > 0))
			ReplyMove("title: play", ClickOnWidget(NULL, aLink), "", 60);
		else
			ReplyMove("wait: the title is loading", "", "", 30);
		return;
	}

	// Dialogs: the record-score name, the continue prompt's No, a yes-no question's No, a notice's button
	int aDialogId = OpenDialog(theApp);
	if (aDialogId >= 0)
	{
		Dialog* aDialog = theApp->GetDialog(aDialogId);
		if (aDialogId == DIALOG_RECORD_SCORE)
		{
			RecordScoreDialog* aRecord = (RecordScoreDialog*) aDialog;
			std::string aKeys;
			size_t aLength = (aRecord->mNameWidget != NULL) ? aRecord->mNameWidget->mString.length() : 0;
			for (size_t i = 0; i < aLength; i++)
				aKeys += "\"Backspace\",";
			aKeys += "\"B\",\"o\",\"t\",\"Enter\"";
			ReplyMove("high score: enter the name Bot", "", aKeys, 60);
			return;
		}
		DialogButton* aButton = (aDialog->mNoButton != NULL) ? aDialog->mNoButton : aDialog->mYesButton;
		if ((aButton != NULL) && (aButton->mWidth > 0))
			ReplyMove("dialog: its button", ClickOnWidget(aDialog, aButton), "", 40);
		else
			ReplyMove("wait: a dialog without a button", "", "", 30);
		return;
	}

	// The game selector: the type, then the skill, then start (only while it is in place)
	if ((theApp->mGameSelector != NULL) && (theApp->mHelpWidget == NULL))
	{
		GameSelector* aSelector = theApp->mGameSelector;
		if ((aSelector->mSlidingIn) || (aSelector->mSlidingOut))
		{
			ReplyMove("wait: the selector slides", "", "", 20);
			return;
		}
		int aMode = (gTargetMode >= 0) ? gTargetMode : theApp->mGameType;
		int aSkill = (gTargetSkill >= 1) ? gTargetSkill : theApp->mSkillLevel;
		static const int kSkillX[3] = { 29, 186, 339 };
		if (aSelector->mGameType != aMode)
			ReplyMove("selector: the type", ClickAtPoint(178 + 154 * aMode + 77, 190), "", 40);
		else if (aSelector->mSkill != aSkill)
			ReplyMove("selector: the skill", ClickAtPoint(GameSelector::PANEL_X + kSkillX[aSkill - 1] + 56, 385), "", 40);
		else
			ReplyMove("selector: start", ClickAtPoint(400, 448), "", 60);
		return;
	}

	// The help screen: "click here to start game"
	if (theApp->mHelpWidget != NULL)
	{
		HyperlinkWidget* aLink = theApp->mHelpWidget->mStartLink;
		if ((aLink != NULL) && (aLink->mWidth > 0))
			ReplyMove("help: start game", ClickOnWidget(theApp->mHelpWidget, aLink), "", 60);
		else
			ReplyMove("wait: the help screen", "", "", 20);
		return;
	}

	Board* aBoard = theApp->mBoard;
	if (aBoard == NULL)
	{
		Reply("MOVE null");
		return;
	}
	if (aBoard->mPauseCount > 0)
	{
		// a click anywhere resumes (Board::MouseDown)
		ReplyMove("unpause", ClickAtPoint(400, 240), "", 30);
		return;
	}
	DialogButton* aContinue = aBoard->mContinueButton;
	if (((aBoard->mState == STATE_LEVEL_DONE) || (aBoard->mState == STATE_GAME_OVER_SCORES)) && (aContinue != NULL) &&
		(aContinue->mWidth > 0))
	{
		if (aBoard->mState == STATE_GAME_OVER_SCORES && !IsOver(theApp))
		{
			ReplyMove("wait: the game-over table slides in", "", "", 20);
			return;
		}
		ReplyMove(aBoard->mState == STATE_LEVEL_DONE ? "next level" : "play again", ClickOnWidget(aBoard, aContinue), "", 100);
		return;
	}
	if (aBoard->mState != STATE_PLAYING)
	{
		// a level ending, a game ending, a bomb, the selector over a frozen board: nothing to click yet
		ReplyMove("wait: the board is busy", "", "", 20);
		return;
	}
	if (aBoard->mGoTimer > 0)
	{
		ReplyMove("wait: get ready", "", "", 20);
		return;
	}

	// Puzzle: the solver's plan, once the board has settled
	if (theApp->mGameType == GAMETYPE_PUZZLE)
	{
		if (!IsSettled(aBoard))
		{
			ReplyMove("wait: the board settles", "", "", 10);
			return;
		}
		int aCol, aRow;
		if (SolverClick(aBoard, &aCol, &aRow))
		{
			ReplyMove("puzzle: the solver's click", ClickAtPoint(Board::ColX(aCol) + 14, Board::RowY(aRow) + 14), "", 20);
			return;
		}
	}
	int aX, aY, aSize;
	if (BestGroup(aBoard, &aX, &aY, &aSize))
	{
		char aKind[32];
		snprintf(aKind, sizeof(aKind), "group of %d", aSize);
		ReplyMove(aKind, ClickAtPoint(aX, aY), "", 12);
		return;
	}
	// no group to click: the board goes on by itself (rising coins, the game over)
	ReplyMove("wait: no group", "", "", 20);
}

} // namespace

bool WinBMApp::HandleTestCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	std::string aArg = theCommand.mArgs;
	while (!aArg.empty() && (aArg[0] == ' ' || aArg[0] == '\t'))
		aArg.erase(0, 1);
	while (!aArg.empty() && (aArg.back() == ' ' || aArg.back() == '\t' || aArg.back() == '\r'))
		aArg.pop_back();

	if (aCommand == "state")
	{
		Reply("STATE " + StateJson(this));
		return true;
	}
	if (aCommand == "move")
	{
		MoveCommand(this);
		return true;
	}
	if (aCommand == "seed")
	{
		char* anEnd = NULL;
		unsigned long aSeed = strtoul(aArg.c_str(), &anEnd, 10);
		if (aArg.empty() || *anEnd != 0)
		{
			ReplyError(theCommand, "a number is needed");
			return true;
		}
		// Every board takes its seed from the CRT's rand() when it is made (Board::Board), so this fixes the next board
		CrtSRand((unsigned int) aSeed);
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "unlock")
	{
		if (aArg != "all")
		{
			ReplyError(theCommand, "only \"unlock all\"");
			return true;
		}
		// The Steam build runs registered: every game type and skill is open
		Reply("OK unlock all (nothing in Big Money is locked)");
		return true;
	}
	if (aCommand != "goto" && aCommand != "level" && aCommand != "set" && aCommand != "win" && aCommand != "lose")
		return false;

	// set mode / set skill only choose what move is to play, so they work from the title on
	bool isBotChoice = (aCommand == "set") && ((aArg.compare(0, 5, "mode ") == 0) || (aArg.compare(0, 6, "skill ") == 0));
	if ((mTitleScreen != NULL) && (!isBotChoice))
	{
		ReplyError(theCommand, "the title screen is still up (click its play link first)");
		return true;
	}

	if (aCommand == "goto")
	{
		if (aArg == "menu" || aArg == "selector" || aArg == "title")
		{
			if (aArg == "title")
			{
				ReplyError(theCommand, "the title screen is shown once, at launch");
				return true;
			}
			ShowGameSelector();
		}
		else if (aArg == "options")
			ShowOptionsDialog();
		else if (aArg == "help")
			ShowHelp();
		else if (aArg == "highscores" || aArg == "scores")
			ShowHighScoreDialog();
		else if (aArg == "credits")
			ShowCreditsDialog();
		else
		{
			// a mode's name, with an optional skill: starts it as the selector's start button does
			std::string aName = aArg;
			int aSkill = mSkillLevel;
			size_t aSpace = aArg.find(' ');
			if (aSpace != std::string::npos)
			{
				aName = aArg.substr(0, aSpace);
				aSkill = atoi(aArg.c_str() + aSpace + 1);
			}
			int aMode = ModeByName(aName);
			if (aMode < 0 || aSkill < 1 || aSkill > 3)
			{
				ReplyError(theCommand, "unknown screen (title, menu, options, help, highscores, credits, or action|strategy|puzzle [1-3])");
				return true;
			}
			mGameType = aMode;
			mSkillLevel = aSkill;
			StartGame();
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "set")
	{
		size_t aSpace = aArg.find(' ');
		if (aSpace == std::string::npos)
		{
			ReplyError(theCommand, "set <name> <value> (score, moves, mode, skill)");
			return true;
		}
		std::string aName = aArg.substr(0, aSpace);
		std::string aValue = aArg.substr(aSpace + 1);
		int aNumber = atoi(aValue.c_str());
		if (aName == "mode")
		{
			if (ModeByName(aValue) < 0)
			{
				ReplyError(theCommand, "mode is action, strategy or puzzle");
				return true;
			}
			gTargetMode = ModeByName(aValue);
		}
		else if (aName == "skill")
		{
			if (aNumber < 1 || aNumber > 3)
			{
				ReplyError(theCommand, "skill is 1 to 3");
				return true;
			}
			gTargetSkill = aNumber;
		}
		else if (aName == "score" || aName == "moves")
		{
			if (mBoard == NULL)
			{
				ReplyError(theCommand, "no board");
				return true;
			}
			if (aName == "score")
			{
				mBoard->mScore = aNumber;
				mBoard->mDisplayScore = aNumber;
			}
			else
				mBoard->mMovesLeft = aNumber;
		}
		else
		{
			ReplyError(theCommand, "unknown name (score, moves, mode, skill)");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	// level, win, lose act on the running board
	if (mBoard == NULL)
	{
		ReplyError(theCommand, "no game is running");
		return true;
	}
	if (aCommand == "level")
	{
		int aLevel = atoi(aArg.c_str());
		if (aLevel < 1)
		{
			ReplyError(theCommand, "level <n>, n from 1");
			return true;
		}
		// Board::NextLevel counts the level up and sets it up: one less, then the level change the continue button makes
		mBoard->mLevel = aLevel - 2;
		mBoard->StartNextLevel();
		ReplyOK(theCommand);
		return true;
	}
	if (mBoard->mState != STATE_PLAYING)
	{
		ReplyError(theCommand, std::string("the board is not playing (") + BoardStateName(mBoard->mState) + ")");
		return true;
	}
	if (aCommand == "win")
		mBoard->LevelComplete();
	else
		mBoard->GameOver();
	ReplyOK(theCommand);
	return true;
}
