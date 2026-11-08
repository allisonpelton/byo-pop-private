// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command channel
// through WinDMApp::HandleScriptCommand. A command that is never given changes nothing, draws no random number and
// moves no state. Each answers on stdout with one line: STATE {...}, MOVE {...}, OK <command> or
// ERROR <command>: <reason>. They act through the game's own code (the title screen's play link, the menu's own start,
// DoMainMenu, the board's own GameOver). The game's notes list them (GAME_NOTES.md, "Test commands").

#include "WinDMApp.h"
#include "Board.h"
#include "Piece.h"
#include "MainMenu.h"
#include "TitleScreen.h"
#include "Profile.h"
#include "HighScoreWidget.h"
#include "EffectOverlay.h"
#include "Hyperspace.h"
#include "Dialog.h"
#include "DialogButton.h"
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

std::string Words(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + Words(theCommand));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + Words(theCommand) + ": " + theReason);
}

const char* const kModeNames[9] = {"classic", "action", "puzzle", "endless", "twilight", "hyper", "cognito", "finity",
	"hidden"};

int ModeFromName(const std::string& theName)
{
	for (int i = 0; i < 9; i++)
	{
		if (theName == kModeNames[i])
			return i;
	}
	return -1;
}

const char* PhaseName(int theState)
{
	static const char* const kNames[] = {"new", "idle", "swapping", "swap back", "commit swap", "clearing",
		"game over fall", "7", "level complete wait", "9", "whirlpool", "level transition", "board dropping",
		"post warp", "hyper swap done", "pod change", "puzzle solved", "puzzle stuck", "galaxy", "fade",
		"secret finished", "galaxy slide 1", "galaxy slide 2", "galaxy slide 3"};
	return (theState >= 0 && theState < (int)(sizeof(kNames) / sizeof(kNames[0]))) ? kNames[theState] : "?";
}

// a gem's type as one character: 0 to 6 the colours, the special gems by their number (7, 8, 9 as letters a, b, c)
char TypeChar(int theType)
{
	if (theType < 0)
		return '.';
	if (theType < 7)
		return (char)('0' + theType);
	return (char)('a' + (theType - 7));
}

}  // namespace

// The screen a driver sees: the loading screen and the title, the menu, a dialog over either, the game (with the
// warp, the galaxy map and the end screen as parts of it) and the high-score screen after a game
std::string WinDMApp::TestScreen()
{
	if (mTitleScreen != NULL)
		return mTitleScreen->mLoadingCompleted ? "loading" : "title";
	if (!mDialogMap.empty())
		return "dialog";
	if (mHighScoreWidget != NULL)
		return "highscores";
	if (mBoard != NULL)
	{
		if (mMainMenu != NULL && mBoard->mScreenState != Board::SCREEN_SHOWN)
			return "menu";
		return "game";
	}
	if (mMainMenu != NULL)
		return "menu";
	return "loading";
}

bool WinDMApp::HandleTestCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	if (aCommand != "state" && aCommand != "goto" && aCommand != "level" && aCommand != "unlock" && aCommand != "set" &&
		aCommand != "win" && aCommand != "lose" && aCommand != "seed" && aCommand != "move")
		return false;

	std::string aScreen = TestScreen();
	Board* aBoard = (mBoard != NULL && mTitleScreen == NULL) ? mBoard : NULL;
	bool aPlaying = aBoard != NULL && mHighScoreWidget == NULL && aScreen == "game";

	if (aCommand == "state")
	{
		std::string aLine = "STATE {\"screen\":\"" + aScreen + "\"";
		if (mMainMenu != NULL)
			aLine += StrFormat(",\"menu\":%d", (int)mMainMenu->mState);
		std::string aDialogs;
		for (DialogMap::iterator anItr = mDialogMap.begin(); anItr != mDialogMap.end(); ++anItr)
			aDialogs += StrFormat(aDialogs.empty() ? "%d" : ",%d", anItr->first);
		aLine += ",\"dialogs\":[" + aDialogs + "]";
		// the buttons of the dialog that is on top (the highest id), the middle of each, in game pixels
		if (!mDialogMap.empty())
		{
			Dialog* aDialog = mDialogMap.rbegin()->second;
			std::string aButtons;
			Widget* aList[2] = {aDialog->mYesButton, aDialog->mNoButton};
			const char* aNames[2] = {"yes", "no"};
			for (int i = 0; i < 2; i++)
			{
				if (aList[i] == NULL || !aList[i]->mVisible)
					continue;
				aButtons += StrFormat("%s\"%s\":[%d,%d]", aButtons.empty() ? "" : ",", aNames[i],
					aList[i]->mX + aList[i]->mWidth / 2, aList[i]->mY + aList[i]->mHeight / 2);
			}
			aLine += ",\"buttons\":{" + aButtons + "}";
		}
		aLine += mIsWindowed ? ",\"windowed\":true" : ",\"windowed\":false";
		aLine += mHasFocus ? ",\"focus\":true" : ",\"focus\":false";
		if (aBoard != NULL)
		{
			int aMode = mGameMode;
			bool aBusy = aBoard->mState != Board::STATE_IDLE || aBoard->mHyperspace != NULL || aBoard->mPauseCount > 0 ||
				aBoard->mInterfacePauseCount > 0 || aBoard->mUnpauseDelay > 0 || !mDialogMap.empty() ||
				aBoard->mScreenState != Board::SCREEN_SHOWN || mHighScoreWidget != NULL;
			aLine += StrFormat(",\"mode\":\"%s\",\"modeid\":%d,\"level\":%d,\"levelindex\":%d,\"score\":%d,\"phase\":\"%s\","
				"\"phaseid\":%d,\"tick\":%d,\"busy\":%s,\"paused\":%s",
				(aMode >= 0 && aMode < 9) ? kModeNames[aMode] : "?", aMode, aBoard->mNextLevel, aBoard->mLevel,
				aBoard->mScore, PhaseName(aBoard->mState), aBoard->mState, aBoard->mUpdateCnt, aBusy ? "true" : "false",
				(aBoard->mPauseCount > 0 || aBoard->mInterfacePauseCount > 0) ? "true" : "false");
			aLine += StrFormat(",\"hyperspace\":%s,\"tutorial\":%s,\"over\":%s", aBoard->mHyperspace != NULL ? "true" : "false",
				aBoard->mIsPlanetIntro ? "true" : "false",
				(aBoard->mState == Board::STATE_GAME_OVER_FALL || aBoard->mState == Board::STATE_7 || mHighScoreWidget != NULL) ? "true" : "false");
			if (aBoard->mSelectedPiece != NULL)
				aLine += StrFormat(",\"selected\":[%d,%d]", aBoard->mSelectedPiece->mCol, aBoard->mSelectedPiece->mRow);
			// the board: a row a string, a gem by its type's character, . for none
			std::string aRows;
			for (int aRow = 0; aRow < 8; aRow++)
			{
				aRows += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < 8; aCol++)
				{
					Piece* aPiece = aBoard->mBoard[aRow][aCol];
					aRows += aPiece != NULL ? TypeChar(aPiece->mType) : '.';
				}
				aRows += "\"";
			}
			aLine += ",\"board\":[" + aRows + "]";
		}
		else
			aLine += StrFormat(",\"modeid\":%d", mGameMode);
		Reply(aLine + "}");
		return true;
	}

	if (aCommand == "seed")
	{
		if (anArgs.empty())
		{
			ReplyError(theCommand, "seed <n>");
			return true;
		}
		SRand((ulong)strtoul(anArgs.c_str(), NULL, 0));  // the same SRand as -seed=<n>
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
		if (mProfile == NULL)
		{
			ReplyError(theCommand, "no profile yet");
			return true;
		}
		// the counters that open the secret modes (Profile::IsModeUnlocked)
		if (mProfile->mModeLevel[0] < 17)
			mProfile->mModeLevel[0] = 17;
		if (mProfile->mModeLevel[1] < 8)
			mProfile->mModeLevel[1] = 8;
		if (mProfile->mTotal7C < 80)
			mProfile->mTotal7C = 80;
		if (mProfile->mModeLevel[3] < 280)
			mProfile->mModeLevel[3] = 280;
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "goto")
	{
		if (anArgs == "credits" || anArgs == "options" || anArgs == "help" || anArgs == "highscores")
		{
			ReplyError(theCommand, "not yet: this screen is reached by input");
			return true;
		}
		int aMode = ModeFromName(anArgs);
		if (anArgs != "title" && anArgs != "menu" && aMode < 0)
		{
			ReplyError(theCommand, "unknown screen (title is not reachable; menu, classic, action, puzzle, endless, "
				"twilight, hyper, cognito, finity, hidden)");
			return true;
		}
		if (anArgs == "title")
		{
			ReplyError(theCommand, "the game has no title screen to return to");
			return true;
		}
		if (aScreen == "loading")
		{
			ReplyError(theCommand, "the game is still loading");
			return true;
		}
		if (aScreen == "title")
		{
			// the play link's own click (the title screen slides away; the menu follows)
			if (!mTitleScreen->mLoadingCompleted)
				LoadingCompleted();
			ReplyOK(theCommand);
			return true;
		}
		// close what is open over the menu or the game, then go
		while (!mDialogMap.empty())
		{
			int anId = mDialogMap.begin()->first;
			if (!KillDialog(anId))
				break;
		}
		if (!mDialogMap.empty())
		{
			ReplyError(theCommand, "a dialog does not close");
			return true;
		}
		if (anArgs == "menu")
		{
			if (mMainMenu == NULL || mBoard != NULL)
				DoMainMenu();
			ReplyOK(theCommand);
			return true;
		}
		// a mode: the menu's own start
		if (mMainMenu == NULL || mBoard != NULL || mMainMenu->mState != MainMenu::STATE_READY)
		{
			ReplyError(theCommand, "the menu is not ready (goto menu first, then wait for state menu 1)");
			return true;
		}
		mMainMenu->SelectMode(aMode);
		mMainMenu->StartGame();
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "move")
	{
		// The first move that FindMove finds, the swap of two gems as two clicks, with the first at a place chosen by a
		// hash of the board and the score so that a game does not repeat one move forever. FindMove only looks (it puts
		// the gems back), so asking changes nothing, draws no random number of the game's and the same question gets the
		// same answer. While the board is busy it says to wait.
		if (!aPlaying)
		{
			Reply("MOVE null");
			return true;
		}
		if (mGameMode == Board::MODE_PUZZLE || mGameMode == Board::MODE_COGNITO)
		{
			// the puzzle modes: the bot follows the puzzle's own solution file (the moves the hint shows)
			if (aBoard->mState != Board::STATE_IDLE || aBoard->mSelectedPiece != NULL || aBoard->mHyperspace != NULL ||
				aBoard->mPauseCount > 0 || aBoard->mInterfacePauseCount > 0 || aBoard->mUnpauseDelay > 0 ||
				aBoard->mPendingPod != -1)
			{
				Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
				return true;
			}
			unsigned int aWords[64];
			int aWordCount = aBoard->FindPuzzleValidMoves(aWords, 64);
			if (aWordCount == 0)
			{
				Reply("MOVE null");
				return true;
			}
			// the first valid move; the byte is column, row and direction (left, up, right, down)
			static const int aStep[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
			unsigned char aByte = aBoard->TranslateMoveByte((unsigned char)aWords[0]);
			int aCol1 = aByte & 7, aRow1 = aByte >> 3 & 7;
			int aCol2 = aCol1 + aStep[aByte >> 6][0], aRow2 = aRow1 + aStep[aByte >> 6][1];
			Piece* aFirst = aBoard->mBoard[aRow1][aCol1];
			if (aFirst == NULL || aCol2 < 0 || aCol2 > 7 || aRow2 < 0 || aRow2 > 7)
			{
				Reply("MOVE null");
				return true;
			}
			int aX1 = ScaleNum((int)(aFirst->mX + 42.0f), 0), aY1 = ScaleNum((int)(aFirst->mY + 42.0f), 0);
			int aX2, aY2;
			Piece* aSecond = aBoard->mBoard[aRow2][aCol2];
			if (aSecond != NULL)
			{
				aX2 = ScaleNum((int)(aSecond->mX + 42.0f), 0);
				aY2 = ScaleNum((int)(aSecond->mY + 42.0f), 0);
			}
			else
			{
				// an empty square: the neighbour's place, one square (84 pixels) from the first gem (the second click's
				// vertical direction is reversed on a flipped board)
				aX2 = aX1 + (aCol2 - aCol1) * ScaleNum(84, 0);
				aY2 = aY1 + (aRow2 - aRow1) * ScaleNum(84, 0) * (aBoard->mFlipped ? -1 : 1);
			}
			Reply(StrFormat("MOVE {\"kind\":\"swap\",\"from\":[%d,%d],\"to\":[%d,%d],\"input\":[{\"move\":[%d,%d]},"
				"{\"click\":[%d,%d]},{\"move\":[%d,%d]},{\"click\":[%d,%d]},{\"wait\":4}]}",
				aCol1, aRow1, aCol2, aRow2, aX1, aY1, aX1, aY1, aX2, aY2, aX2, aY2));
			return true;
		}
		if (aBoard->mState == Board::STATE_GAME_OVER_FALL)
		{
			Reply("MOVE null");
			return true;
		}
		if (aBoard->mState != Board::STATE_IDLE || aBoard->mSelectedPiece != NULL || aBoard->mHyperspace != NULL ||
			aBoard->mPauseCount > 0 || aBoard->mInterfacePauseCount > 0 || aBoard->mUnpauseDelay > 0)
		{
			Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
			return true;
		}
		int aCount = 0;
		while (aBoard->FindMove(NULL, aCount, true, true))
			aCount++;
		if (aCount == 0)
		{
			Reply("MOVE null");
			return true;
		}
		unsigned aHash = 2166136261u;  // FNV-1a over the board's gems and the score
		for (int aRow = 0; aRow < 8; aRow++)
		{
			for (int aCol = 0; aCol < 8; aCol++)
			{
				Piece* aPiece = aBoard->mBoard[aRow][aCol];
				aHash = (aHash ^ (unsigned)(aPiece != NULL ? aPiece->mType + 1 : 0)) * 16777619u;
			}
		}
		aHash = (aHash ^ (unsigned)aBoard->mScore) * 16777619u;
		int aMove[4];
		aBoard->FindMove(aMove, (int)(aHash % (unsigned)aCount), true, true);
		Piece* aPiece1 = aBoard->mBoard[aMove[1]][aMove[0]];
		Piece* aPiece2 = aBoard->mBoard[aMove[3]][aMove[2]];
		int aX1 = ScaleNum((int)(aPiece1->mX + 42.0f), 0), aY1 = ScaleNum((int)(aPiece1->mY + 42.0f), 0);
		int aX2 = ScaleNum((int)(aPiece2->mX + 42.0f), 0), aY2 = ScaleNum((int)(aPiece2->mY + 42.0f), 0);
		// the input as a hand makes it: the pointer to the first gem and a click, the pointer to the second and a click
		// (TEST_COMMANDS.md's steps), then a few updates for the swap to begin
		Reply(StrFormat("MOVE {\"kind\":\"swap\",\"from\":[%d,%d],\"to\":[%d,%d],\"input\":[{\"move\":[%d,%d]},"
			"{\"click\":[%d,%d]},{\"move\":[%d,%d]},{\"click\":[%d,%d]},{\"wait\":4}]}",
			aMove[0], aMove[1], aMove[2], aMove[3], aX1, aY1, aX1, aY1, aX2, aY2, aX2, aY2));
		return true;
	}

	// The rest act on a game in progress
	if (!aPlaying)
	{
		ReplyError(theCommand, "no game is on (screen " + aScreen + ")");
		return true;
	}

	if (aCommand == "set")
	{
		size_t aSpace = anArgs.find(' ');
		if (aSpace == std::string::npos)
		{
			ReplyError(theCommand, "set <name> <value>");
			return true;
		}
		std::string aName = anArgs.substr(0, aSpace);
		int aValue = atoi(anArgs.c_str() + aSpace + 1);
		if (aName == "score")
			aBoard->mScore = aValue;
		else
		{
			ReplyError(theCommand, "can set score");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "lose")
	{
		// the game's own ending, as when no move is left
		aBoard->mEffectOverlay->DoStrechyText("NO MOVES!", 0x50);
		aBoard->GameOver(200);
		ReplyOK(theCommand);
		return true;
	}

	// win and level: the level's goal is met, and the board's own update starts the whirlpool (Board::DoUpdate: the bar is
	// full, state 8, then 9, then DoWhirlpool); in the puzzle modes win is the puzzle's own ending
	if (aCommand == "win" || aCommand == "level")
	{
		if (aBoard->mState != Board::STATE_IDLE)
		{
			ReplyError(theCommand, std::string("the board is busy (") + PhaseName(aBoard->mState) + ")");
			return true;
		}
		if (mGameMode == Board::MODE_PUZZLE || mGameMode == Board::MODE_COGNITO)
		{
			if (aCommand == "level")
				ReplyError(theCommand, "the puzzle modes have no levels");
			else
			{
				aBoard->PuzzleCompleted();
				ReplyOK(theCommand);
			}
			return true;
		}
		if (aCommand == "level")
		{
			int aLevel = atoi(anArgs.c_str());
			if (aLevel < 2)
			{
				ReplyError(theCommand, "level <n>, n from 2: the game goes up to it from the level before");
				return true;
			}
			// the whirlpool's end makes mLevel and mNextLevel one more and sets the new level's values
			aBoard->mLevel = aLevel - 2;
			aBoard->mNextLevel = aLevel - 1;
		}
		int aGoal = aBoard->m15eec + (int)aBoard->m15ef8;  // the level's target score plus the drained time
		if (aBoard->mScore < aGoal)
			aBoard->mScore = aGoal;
		if (aBoard->m15e40 < aGoal)
			aBoard->m15e40 = aGoal;  // the shown score that the bar is made from
		ReplyOK(theCommand);
		return true;
	}

	ReplyError(theCommand, aCommand + " is not implemented yet");
	return true;
}

// port: the framework's commands came first; the standard test commands next
bool WinDMApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (HandleTestCommand(theCommand))
		return true;
	return SexyApp::HandleScriptCommand(theCommand);
}
