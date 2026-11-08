// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command channel
// through WinDMApp::HandleScriptCommand. A command that is never given changes nothing, draws no random number and moves
// no state. Each answers on stdout with one line: STATE {...}, MOVE {...}, OK <command> or ERROR <command>: <reason>.
// They act through the game's own code: the title screen's play link, the side bar's mode buttons, the dialogs, the
// board's own level end (StartBlackHole) and its own game over (GameOver). The game's notes list them
// (GAME_NOTES.md, "Test commands").

#include "WinDMApp.h"
#include "Board.h"
#include "TitleScreen.h"
#include "HighScoreWidget.h"
#include "Autoplay.h"
#include "Dialog.h"
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

// A gem colour as one character: 0..9, then a, b, ... (the original has 0..6; a mod can have more)
char ColorChar(int theColor)
{
	if (theColor < 0)
		return '.';
	return (char) ((theColor < 10) ? '0' + theColor : 'a' + (theColor - 10));
}

const char* PhaseName(int theState)
{
	static const char* const kNames[] = {"falling", "playing", "swapping", "swapping back", "clearing", "game over",
		"game over done", "vortex", "level wipe", "fall out", "twisting", "twisting back", "sliding", "sliding back",
		"arrow stepping"};
	return (theState >= 0 && theState < (int) (sizeof(kNames) / sizeof(kNames[0]))) ? kNames[theState] : "?";
}

// Rules a driver of the original game cannot play by swapping two gems
bool SwapsOnly(Board* theBoard)
{
	return !(theBoard->mVariant.mMoves && theBoard->mRules.mMoves.mTypes != GameRules::MOVE_SWAP);
}

}  // namespace

// The screen a driver sees: the loading screen and the title, the game with its side bar, a dialog over it, or the
// high-score screen after a game
std::string WinDMApp::TestScreen()
{
	if (mTitleScreen != NULL)
		return mTitleScreen->mLoadingComplete ? "title" : "loading";
	if (mBoard == NULL)
		return "loading";
	if (GetDialog(DIALOG_RECORD_SCORE) != NULL)
		return "recordscore";
	if (GetDialog(DIALOG_OPTIONS) != NULL)
		return "options";
	if (GetDialog(DIALOG_HIGH_SCORES) != NULL)
		return "highscores";
	if (GetDialog(DIALOG_FULL_INSTRUCTIONS) != NULL)
		return "help";
	if (GetDialog(DIALOG_QUIT) != NULL)
		return "quit";
	for (DialogMap::iterator anItr = mDialogMap.begin(); anItr != mDialogMap.end(); ++anItr)
	{
		if (anItr->first != DIALOG_INSTRUCTIONS)  // the first game's hint is no screen of its own
			return "dialog";
	}
	return (mHighScoreWidget != NULL) ? "gameover" : "game";
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
	bool aPlaying = aBoard != NULL && mHighScoreWidget == NULL;

	if (aCommand == "state")
	{
		std::string aLine = "STATE {\"screen\":\"" + aScreen + "\"";
		if (aBoard != NULL)
		{
			bool anOver = aBoard->mState == STATE_GAME_OVER || aBoard->mState == STATE_GAME_OVER_DONE;
			bool aBlocked = aBoard->mPauseCount > 0 || aBoard->mUserPauseCount > 0 || aBoard->mInputDelay > 0 ||
				aScreen != "game";
			aLine += StrFormat(",\"mode\":\"%s\",\"level\":%d,\"score\":%d,\"shown\":%d,\"start\":%d,\"target\":%d,"
				"\"bar\":%d,\"drain\":%d,\"phase\":\"%s\",\"tick\":%d,\"moves\":%d,\"over\":%s,\"busy\":%s,"
				"\"paused\":%s,\"newgame\":%s",
				aBoard->mTimedMode ? "timed" : "normal", aBoard->mDisplayLevel, aBoard->mScore, aBoard->mDisplayScore,
				aBoard->mLevelStartScore, aBoard->mLevelTargetScore, aBoard->mBarWidth, (int) aBoard->mTimeDrain,
				PhaseName(aBoard->mState), aBoard->mUpdateCnt, aBoard->mMoveCount, anOver ? "true" : "false",
				(aBoard->mState != STATE_PLAYING || aBlocked) ? "true" : "false",
				(aBoard->mPauseCount > 0 || aBoard->mUserPauseCount > 0) ? "true" : "false",
				aBoard->mIsNewGame ? "true" : "false");
			if (aBoard->mSelectedPiece != NULL)
				aLine += StrFormat(",\"selected\":[%d,%d]", aBoard->mSelectedPiece->mCol, aBoard->mSelectedPiece->mRow);
			// the board: a row a string, a gem by its colour's character, . for none
			std::string aRows;
			for (int aRow = 0; aRow < aBoard->mRows; aRow++)
			{
				aRows += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < aBoard->mCols; aCol++)
				{
					Piece* aPiece = aBoard->mBoard[aRow][aCol];
					aRows += aPiece != NULL ? ColorChar(aPiece->mColor) : '.';
				}
				aRows += "\"";
			}
			aLine += ",\"board\":[" + aRows + "]";
		}
		else
			aLine += StrFormat(",\"mode\":\"%s\"", mTimedMode ? "timed" : "normal");
		Reply(aLine + "}");
		return true;
	}

	if (aCommand == "goto")
	{
		bool aTimed = anArgs == "timed";
		if (anArgs == "title" || anArgs == "credits")
		{
			ReplyError(theCommand, "the game has no " + anArgs + " screen to return to");
			return true;
		}
		if (anArgs != "normal" && anArgs != "timed" && anArgs != "game" && anArgs != "menu" && anArgs != "options" &&
			anArgs != "highscores" && anArgs != "help")
		{
			ReplyError(theCommand, "unknown screen (normal, timed, game, menu, options, highscores, help)");
			return true;
		}
		if (aScreen == "loading")
		{
			ReplyError(theCommand, "the game is still loading");
			return true;
		}
		if (aScreen == "recordscore")
		{
			ReplyError(theCommand, "the name entry is open (answer it first)");
			return true;
		}
		if (aScreen == "title")
		{
			// the play link's own click: the board, the side bar and the saved game's question if there is a save
			TitleScreenIsFinished();
			if (anArgs == "timed")
				SetTimedMode(true, true);
			ReplyOK(theCommand);
			return true;
		}
		// close what is open over the game (Options, High Scores, Help, a question), then go
		while (!mDialogMap.empty())
		{
			int anId = mDialogMap.begin()->first;
			if (anId == DIALOG_INSTRUCTIONS)
			{
				KillDialog(anId);
				continue;
			}
			if (!KillDialog(anId))
				break;
		}
		if (anArgs == "menu")
			;  // nothing else: Bejeweled's side bar is always there
		else if (anArgs == "options")
			ShowOptionsDialog();
		else if (anArgs == "highscores")
			ShowHighScoreDialog();
		else if (anArgs == "help")
			ShowFullInstructionsDialog();
		else
			SetTimedMode(anArgs == "game" ? mTimedMode : aTimed, true);  // a new game, as the mode button starts one
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
		ReplyOK(theCommand);  // nothing is locked: both modes are open from the first run
		return true;
	}

	if (aCommand == "seed")
	{
		if (anArgs.empty())
		{
			ReplyError(theCommand, "seed <n>");
			return true;
		}
		SRand((ulong) strtoul(anArgs.c_str(), NULL, 0));  // the same SRand as -seed=<n>
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "move")
	{
		// The move -autoplay's valid moves choose from (AutoplayBot::ValidClickableMoves), the swap of two gems as two
		// clicks. The choice is a function of the board and the score, so asking changes nothing, draws no random number
		// of the game's and a repeated question gets the same move. While the board is busy it says to wait.
		if (!aPlaying || aScreen != "game" || aBoard->mPlaybackOnly)
		{
			Reply("MOVE null");
			return true;
		}
		if (!SwapsOnly(aBoard))
		{
			ReplyError(theCommand, "the rules' twists and slides have no bot move here");
			return true;
		}
		bool aBlocked = aBoard->mPauseCount > 0 || aBoard->mUserPauseCount > 0 || aBoard->mInputDelay > 0;
		if (aBoard->mState == STATE_GAME_OVER || aBoard->mState == STATE_GAME_OVER_DONE)
		{
			Reply("MOVE null");
			return true;
		}
		if (aBlocked || aBoard->mState != STATE_PLAYING || aBoard->mSelectedPiece != NULL)
		{
			// falling, swapping, clearing, the vortex, a pause or a gem still selected: nothing to do yet
			Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
			return true;
		}
		std::vector<int> aMoves;
		AutoplayBot::ValidClickableMoves(this, &aMoves);
		if (aMoves.empty())
		{
			Reply("MOVE null");
			return true;
		}
		unsigned aHash = 2166136261u;  // FNV-1a over the board's gems and the score
		for (int aRow = 0; aRow < aBoard->mRows; aRow++)
			for (int aCol = 0; aCol < aBoard->mCols; aCol++)
			{
				Piece* aPiece = aBoard->mBoard[aRow][aCol];
				aHash = (aHash ^ (unsigned) (aPiece != NULL ? aPiece->mColor + 1 : 0)) * 16777619u;
			}
		aHash = (aHash ^ (unsigned) aBoard->mScore) * 16777619u;
		int aPacked = aMoves[aHash % aMoves.size()];
		int aCol1 = aPacked >> 12, aRow1 = (aPacked >> 8) & 15, aCol2 = (aPacked >> 4) & 15, aRow2 = aPacked & 15;
		Piece* aPiece1 = aBoard->mBoard[aRow1][aCol1];
		Piece* aPiece2 = aBoard->mBoard[aRow2][aCol2];
		int aX1 = AutoplayBot::ToScreenX(this, (int) aPiece1->mX + 24), aY1 = AutoplayBot::ToScreenY(this, (int) aPiece1->mY + 24);
		int aX2 = AutoplayBot::ToScreenX(this, (int) aPiece2->mX + 24), aY2 = AutoplayBot::ToScreenY(this, (int) aPiece2->mY + 24);
		// the input as a hand makes it: the pointer to the first gem and a click, the pointer to the second and a click
		// (TEST_COMMANDS.md's steps), then a few updates for the swap to begin
		Reply(StrFormat("MOVE {\"kind\":\"swap\",\"from\":[%d,%d],\"to\":[%d,%d],\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]},"
			"{\"move\":[%d,%d]},{\"click\":[%d,%d]},{\"wait\":4}]}", aCol1, aRow1, aCol2, aRow2, aX1, aY1, aX1, aY1, aX2, aY2,
			aX2, aY2));
		return true;
	}

	// The rest act on a game in progress
	if (!aPlaying || aScreen != "game")
	{
		ReplyError(theCommand, "no game is on (screen " + aScreen + ")");
		return true;
	}
	if (aBoard->mVariant.mGold)
	{
		ReplyError(theCommand, "not on a gold board (its level ends with the gold)");
		return true;
	}

	if (aCommand == "set")
	{
		size_t aSpace = anArgs.find(' ');
		std::string aName = anArgs.substr(0, aSpace);
		if (aSpace == std::string::npos)
		{
			ReplyError(theCommand, "set <name> <value>");
			return true;
		}
		int aValue = atoi(anArgs.c_str() + aSpace + 1);
		if (aName == "score")
		{
			// both the real and the shown score, so that the level end or time up the new score brings comes at once
			aBoard->mScore = aValue;
			aBoard->mDisplayScore = aValue;
		}
		else if (aName == "drain" && aBoard->mTimedMode)
		{
			// the timed mode's clock is the points drained from its bar (there are no seconds): a big one ends the game
			aBoard->mTimeDrain = aValue;
		}
		else
		{
			ReplyError(theCommand, "can set score, and drain (the timed mode's points drained from the bar)");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aBoard->mState != STATE_PLAYING)
	{
		ReplyError(theCommand, std::string("the board is busy (") + PhaseName(aBoard->mState) + ")");
		return true;
	}

	if (aCommand == "level" || aCommand == "win")
	{
		if (aCommand == "level")
		{
			int aLevel = atoi(anArgs.c_str());
			if (aLevel < 2)
			{
				ReplyError(theCommand, "level <n>, n from 2: the game goes up to it from the level before");
				return true;
			}
			aBoard->mLevel = aLevel - 2;  // the vortex's end makes it level - 1, SetupLevel sets the new level's values
		}
		// the level's goal is met: the board's own update starts the vortex (Board::Update, StartBlackHole)
		int aGoal = aBoard->mLevelTargetScore + (int) aBoard->mTimeDrain;
		if (aBoard->mScore < aGoal)
			aBoard->mScore = aGoal;
		if (aBoard->mDisplayScore < aGoal)
			aBoard->mDisplayScore = aGoal;
		ReplyOK(theCommand);
		return true;
	}

	// lose: the game's own ending, as when no move is left (normal) or the time is up (timed)
	aBoard->ShowText(GetString(aBoard->mTimedMode ? "TIME_UP" : "NO_MOVES"), 400);
	aBoard->GameOver(200);
	ReplyOK(theCommand);
	return true;
}
