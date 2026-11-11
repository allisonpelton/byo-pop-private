// Test hooks: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel through HandleScriptCommand. A command that is never given changes nothing. Each answers on stdout with one
// line: STATE {...}, MOVE {...}|null, OK <command> or ERROR <command>: <reason>.
//
//   state                         the screen, the mode, level and score, and the board
//   goto menu|title|options|highscores|<mode> [easy|medium|hard]   a screen; or a game of a mode from the menu, by the
//                                 menu's own button, the question about a saved game and the difficulty dialog's button
//   level <n>                     level n of the game in progress, as the level-up panel's link starts a level
//   unlock all                    nothing in this game is locked
//   set score <n> | set level <n> the score (and the shown score), the level counter (for the next level)
//   set mode <mode> | set difficulty <d>   the game the bot starts from the menu (move stays bare: with arguments it is
//                                 the framework's pointer move under -realinput)
//   win, lose                     ends the round of an action or strategy game: the board cleared, the water at the ark
//   seed <n>                      the C library's generator (the game's draws), and the bot's own
//   move                          the next input toward playing a game of the mode set, as the bot (Bot.cpp, the one
//                                 -autoplay clicks) would make it:
//                                 MOVE {"kind":...,"input":[{"move":[x,y]},{"click":[x,y]},{"key":"B"}],"updates":N}, "kind":"wait" when
//                                 the game needs time (animals walking, a level ending), MOVE null when there is nothing
//                                 to do (the menu with no mode asked for)

#include "NoahApp.h"
#include "Board.h"
#include "Screens.h"
#include "Dialogs.h"
#include "Bot.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "HyperlinkWidget.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace Sexy;

namespace
{

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

std::string FullCommand(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + FullCommand(theCommand));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + FullCommand(theCommand) + ": " + theReason);
}

const char* ModeName(int theMode)
{
	switch (theMode)
	{
	case GAMEMODE_ACTION: return "action";
	case GAMEMODE_STRATEGY: return "strategy";
	case GAMEMODE_PUZZLE: return "puzzle";
	case GAMEMODE_STAMPEDE: return "stampede";
	}
	return "none";
}

int ModeByName(const std::string& theName)
{
	if (theName == "action")
		return GAMEMODE_ACTION;
	if (theName == "strategy")
		return GAMEMODE_STRATEGY;
	if (theName == "puzzle")
		return GAMEMODE_PUZZLE;
	return 0;
}

const char* DifficultyName(int theDifficulty)
{
	return (theDifficulty == 0) ? "easy" : ((theDifficulty == 4) ? "medium" : "hard");
}

int DifficultyByName(const std::string& theName)
{
	if (theName == "easy")
		return 0;
	if (theName == "medium")
		return 4;
	if (theName == "hard")
		return 8;
	return -1;
}

const char* PhaseName(int theState)
{
	switch (theState)
	{
	case BOARD_GET_READY: return "getready";
	case BOARD_PLAYING: return "playing";
	case BOARD_WON: return "won";
	case BOARD_LOST: return "lost";
	case BOARD_LEVEL_DONE: return "leveldone";
	case BOARD_NO_MOVES: return "nomoves";
	case BOARD_ABANDON: return "abandon";
	case BOARD_SWAP: return "swap";
	case BOARD_SWAP_BACK: return "swapback";
	case BOARD_SETTLE: return "settle";
	}
	return "?";
}

// the input steps of a click (docs/TEST_COMMANDS.md): the pointer goes there first, as a hand's does
std::string Click(int theX, int theY)
{
	return StrFormat("{\"move\":[%d,%d]},{\"click\":[%d,%d]}", theX, theY, theX, theY);
}

// a click in the middle of a widget (the buttons and links of the screens are top-level widgets: their rectangles are the
// game's own)
std::string ClickOn(Widget* theWidget)
{
	return Click(theWidget->mX + theWidget->mWidth / 2, theWidget->mY + theWidget->mHeight / 2);
}

std::string MoveClicks(const char* theKind, const std::string& theClicks, int theUpdates)
{
	return StrFormat("MOVE {\"kind\":\"%s\",\"input\":[%s],\"updates\":%d}", theKind, theClicks.c_str(), theUpdates);
}

std::string MoveWait(int theUpdates)
{
	return StrFormat("MOVE {\"kind\":\"wait\",\"input\":[],\"updates\":%d}", theUpdates);
}

}  // namespace

bool NoahApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	return HandleTestCommand(theCommand);
}

// The screen a driver sees
std::string NoahApp::TestScreen()
{
	if (mTitleScreen != NULL)
		return (mLoaded && mTitleScreen->mLinkShown) ? "title" : "loading";
	if (GetDialog(DIALOG_RECORD_SCORE) != NULL)
		return "entername";
	if (mHighScoreWidget != NULL)
		return (mHighScoreWidget->mChecked && (mHighScoreWidget->mFade >= 255.0)) ? "highscores" : "gameover";
	if (GetDialog(DIALOG_QUIT) != NULL)
		return "quit";
	if (GetDialog(DIALOG_OPTIONS) != NULL)
		return "options";
	if (GetDialog(DIALOG_TUTORIAL) != NULL)
		return "tutorial";
	if (GetDialog(DIALOG_MODE_INTRO) != NULL)
		return "modeintro";
	if (GetDialog(DIALOG_DIFFICULTY) != NULL)
		return "difficulty";
	if (GetDialog(DIALOG_NEW_GAME) != NULL)
		return "newgame";
	if (GetDialog(DIALOG_HIGH_SCORES) != NULL)
		return "scores";
	if (GetDialogCount() > 0)
		return "dialog";
	if (mNextLevelWidget != NULL)
		return "nextlevel";
	if (mBoard != NULL)
		return "game";
	if (mIntroWidget != NULL)
		return "menu";
	return "loading";
}

// "goto <mode>" went to the menu's button; the dialogs that follow are answered once they are up, as a player's clicks
// would (the question about a saved game: Yes; the difficulty: the button asked for)
void NoahApp::UpdateTestGoto()
{
	if (mTestGotoMode == 0)
		return;
	std::string aScreen = TestScreen();
	if (aScreen == "newgame")
		ButtonDepress(2000 + DIALOG_NEW_GAME);
	else if (aScreen == "difficulty")
	{
		DifficultyDialog* aDialog = (DifficultyDialog*) GetDialog(DIALOG_DIFFICULTY);
		aDialog->ButtonDepress((mTestGotoDiff == 0) ? 0x3c : ((mTestGotoDiff == 4) ? 0x3d : 0x3e));
		mTestGotoMode = 0;
	}
	else if (aScreen != "menu")
		mTestGotoMode = 0;
}

bool NoahApp::HandleTestCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	Board* aBoard = (mBoard != NULL) ? mBoard : NULL;

	if (aCommand == "state")
	{
		std::string aScreen = TestScreen();
		std::string aLine = StrFormat("STATE {\"update\":%d,\"screen\":\"", mUpdateCount) + aScreen + "\"";
		bool anOver = (aScreen == "highscores");
		if (aBoard != NULL && (aScreen == "game"))
		{
			aLine += StrFormat(",\"mode\":\"%s\",\"difficulty\":\"%s\",\"level\":%d,\"score\":%d,\"phase\":\"%s\",\"busy\":%s",
				ModeName(mGameMode), DifficultyName(mDifficulty), mLevel, mGameScore, PhaseName(aBoard->mState),
				((aBoard->mState != BOARD_PLAYING) || !BotBoardSettled(aBoard)) ? "true" : "false");
			aLine += StrFormat(",\"water\":%d,\"waterTarget\":%d,\"ark\":%d,\"animals\":%d", aBoard->mWaterY, aBoard->mWaterTargetY, aBoard->mArkLevelY, aBoard->AnimalCount());
			aLine += StrFormat(",\"paused\":%s", aBoard->mPaused ? "true" : "false");           // a dialog has paused the board
			// the board: a row a string, a cell a letter (. empty, X block, Y crate, 0-9 and A-D the animals)
			std::string aRows;
			for (int y = 0; y < 9; y++)
			{
				aRows += (y == 0) ? "\"" : ",\"";
				for (int x = 0; x < 9; x++)
				{
					Piece* aPiece = aBoard->mGrid[x][y];
					int aType = (aPiece == NULL) ? PIECE_EMPTY : aPiece->mType;
					aRows += (aType == PIECE_EMPTY) ? '.' : (aType == PIECE_BLOCK) ? 'X' : (aType == PIECE_CRATE) ? 'Y' : (char) ((aType < 10) ? ('0' + aType) : ('A' + aType - 10));
				}
				aRows += "\"";
			}
			aLine += ",\"board\":[" + aRows + "]";
		}
		else if (mGameScore > 0 || (mHighScoreWidget != NULL))
			aLine += StrFormat(",\"mode\":\"%s\",\"level\":%d,\"score\":%d", ModeName(mGameMode), mLevel, mGameScore);
		// the window's focus (a lost focus draws PAUSED and stops the board) and the window mode, on every screen
		aLine += StrFormat(",\"focus\":%s,\"windowed\":%s,\"over\":%s}", mHasFocus ? "true" : "false", mIsWindowed ? "true" : "false", anOver ? "true" : "false");
		Reply(aLine);
		return true;
	}

	if (aCommand == "goto")
	{
		std::string aScreen = TestScreen();
		std::string aName = anArgs.substr(0, anArgs.find(' '));
		std::string aRest = (aName.size() < anArgs.size()) ? anArgs.substr(aName.size() + 1) : "";
		int aMode = ModeByName(aName);
		if ((aName == "menu") || (aName == "title"))
		{
			if (aScreen == "title")
				TitleScreenDone();
			else if (aScreen == "game")
				ButtonDepress(2000 + DIALOG_QUIT);          // the quit question's Yes: the game is saved and the menu comes back
			else if (aScreen == "highscores")
				mHighScoreWidget->ButtonDepress(1);
			else if (aScreen != "menu")
			{
				ReplyError(theCommand, "not from the screen " + aScreen);
				return true;
			}
		}
		else if (aName == "options")
		{
			if (aScreen != "options")
			{
				if (mBoard != NULL)
					mBoard->mPaused = true;
				ShowOptions();
			}
		}
		else if (aName == "highscores")
			ShowHighScores();
		else if (aMode != 0)
		{
			int aDiff = aRest.empty() ? 0 : DifficultyByName(aRest);
			if (aDiff < 0)
			{
				ReplyError(theCommand, "difficulty is easy, medium or hard");
				return true;
			}
			if (aScreen != "menu")
			{
				ReplyError(theCommand, "only from the menu (now " + aScreen + ")");
				return true;
			}
			mTestGotoMode = aMode;
			mTestGotoDiff = aDiff;
			ButtonDepress((aMode == GAMEMODE_ACTION) ? BUTTON_ACTION : ((aMode == GAMEMODE_STRATEGY) ? BUTTON_STRATEGY : BUTTON_PUZZLE));
		}
		else
		{
			ReplyError(theCommand, "unknown screen");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "level")
	{
		int aLevel = atoi(anArgs.c_str());
		if (aBoard == NULL || aLevel < 1 || (mHighScoreWidget != NULL))
		{
			ReplyError(theCommand, "needs a game in progress and a level of 1 or more");
			return true;
		}
		if (mNextLevelWidget != NULL)
			RemoveNextLevel();
		if (mBonusRound)
		{
			mBonusRound = false;
			mGameMode = mSavedGameMode;
		}
		mLevel = aLevel - 1;
		aBoard->NewLevel();
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "unlock")
	{
		if (anArgs != "all")
		{
			ReplyError(theCommand, "only \"unlock all\"");
			return true;
		}
		ReplyOK(theCommand);        // every mode is open from the start; the game has nothing locked
		return true;
	}

	if (aCommand == "set")
	{
		std::string aName = anArgs.substr(0, anArgs.find(' '));
		std::string aText = (aName.size() < anArgs.size()) ? anArgs.substr(aName.size() + 1) : "";
		int aValue = atoi(aText.c_str());
		if (aName == "mode")
		{
			// the game the bot starts from the menu (a bare "move" cannot say: under -realinput "move <x> <y>" is the pointer's)
			if (ModeByName(aText) == 0)
			{
				ReplyError(theCommand, "mode is action, strategy or puzzle");
				return true;
			}
			mTestWantMode = ModeByName(aText);
		}
		else if (aName == "difficulty")
		{
			if (DifficultyByName(aText) < 0)
			{
				ReplyError(theCommand, "difficulty is easy, medium or hard");
				return true;
			}
			mTestWantDiff = DifficultyByName(aText);
		}
		else if (aName == "score")
			mGameScore = mShownScore = aValue;
		else if (aName == "level")
			mLevel = aValue;
		else
		{
			ReplyError(theCommand, "score, level, mode or difficulty");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "seed")
	{
		unsigned int aSeed = (unsigned int) strtoul(anArgs.c_str(), NULL, 10);
		CrtSRand(aSeed);
		mTestBotSeed = aSeed;
		ReplyOK(theCommand);
		return true;
	}

	if ((aCommand == "win") || (aCommand == "lose"))
	{
		if (aBoard == NULL || TestScreen() != "game" || (aBoard->mState != BOARD_PLAYING) ||
			((mGameMode != GAMEMODE_ACTION) && (mGameMode != GAMEMODE_STRATEGY)))
		{
			ReplyError(theCommand, "needs a round of action or strategy that is being played");
			return true;
		}
		if (aCommand == "win")
		{
			// every animal saved: the board clear, the round ends as the game ends it
			for (int x = 0; x < 9; x++)
				for (int y = 0; y < 9; y++)
					if (aBoard->mGrid[x][y]->mType < 0xd)
						aBoard->mGrid[x][y]->mType = PIECE_EMPTY;
		}
		else
			aBoard->mWaterY = aBoard->mArkLevelY;       // the water reaches the ark
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "move")
	{
		std::string aScreen = TestScreen();
		if (aScreen == "loading")
			Reply(MoveWait(30));
		else if (aScreen == "title")
			Reply(MoveClicks("play", ClickOn(mTitleScreen->mPlayLink), 20));
		else if (aScreen == "menu")
		{
			if (mTestWantMode == 0)
				Reply("MOVE null");
			else
			{
				Widget* aButton = (mTestWantMode == GAMEMODE_ACTION) ? (Widget*) mIntroWidget->mActionButton
					: ((mTestWantMode == GAMEMODE_STRATEGY) ? (Widget*) mIntroWidget->mStrategyButton : (Widget*) mIntroWidget->mPuzzleButton);
				Reply(MoveClicks("mode", ClickOn(aButton), 10));
			}
		}
		else if (aScreen == "newgame")
			Reply(MoveClicks("yes", ClickOn(GetDialog(DIALOG_NEW_GAME)->mYesButton), 10));
		else if (aScreen == "difficulty")
		{
			DifficultyDialog* aDialog = (DifficultyDialog*) GetDialog(DIALOG_DIFFICULTY);
			Widget* aButton = (mTestWantDiff == 0) ? (Widget*) aDialog->mEasyButton
				: ((mTestWantDiff == 4) ? (Widget*) aDialog->mMediumButton : (Widget*) aDialog->mHardButton);
			Reply(MoveClicks("difficulty", ClickOn(aButton), 10));
		}
		else if (aScreen == "tutorial")
			Reply(MoveClicks("close", ClickOn(GetDialog(DIALOG_TUTORIAL)->mYesButton), 10));
		else if (aScreen == "modeintro")
			Reply(MoveClicks("close", ClickOn(GetDialog(DIALOG_MODE_INTRO)->mYesButton), 10));
		else if (aScreen == "nextlevel")
			Reply(MoveClicks("next", ClickOn(mNextLevelWidget->mNextLink), 10));
		else if (aScreen == "gameover")
			Reply(MoveWait(10));
		else if (aScreen == "entername")
		{
			RecordScoreDialog* aDialog = (RecordScoreDialog*) GetDialog(DIALOG_RECORD_SCORE);
			if (aDialog->mNameEdit->mString.empty())
				Reply("MOVE {\"kind\":\"name\",\"input\":[{\"key\":\"B\"},{\"key\":\"O\"},{\"key\":\"T\"}],\"updates\":5}");
			else
				Reply(MoveClicks("name ok", ClickOn(aDialog->mYesButton), 10));
		}
		else if (aScreen == "highscores")
			Reply(MoveClicks("menu", ClickOn(mHighScoreWidget->mMenuLink), 10));
		else if (aScreen == "game")
		{
			Board* aBoard = mBoard;
			BotMove aMove;
			if ((aBoard->mState != BOARD_PLAYING) || aBoard->mPaused ||
				((mGameMode != GAMEMODE_STAMPEDE) && !BotBoardSettled(aBoard)))
				Reply(MoveWait(10));
			else if (BotChooseMove(aBoard, mGameMode, mTestBotSeed, aMove))
			{
				std::string aClicks;
				for (int i = 0; i < aMove.mCount; i++)
					aClicks += (i > 0 ? "," : "") + Click(Board::CellLeft(aMove.mCellX[i]) + 25 + aBoard->mX, Board::CellTop(aMove.mCellY[i]) + 25 + aBoard->mY);
				Reply(MoveClicks((mGameMode == GAMEMODE_STAMPEDE) ? "group" : (mGameMode == GAMEMODE_PUZZLE) ? "swap" : "pair", aClicks, 4));
			}
			else
				Reply(MoveWait(20));            // no move now: the game's own check ends the round or the game
		}
		else
			Reply("MOVE null");                 // a dialog the bot does not open (quit, options): nothing to do
		return true;
	}

	return false;
}
