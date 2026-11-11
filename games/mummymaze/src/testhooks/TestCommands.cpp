// Test hooks: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel (-commands, byopop_test_command) through HandleScriptCommand. A command that is never given changes nothing:
// no random number is drawn and no state moves. Each answers on stdout with one line: STATE {...}, MOVE {...} or
// MOVE null, OK <command> or ERROR <command>: <reason>.
//
//   state              the game's state as one JSON line (the keys are listed in games/mummymaze/docs/GAME_NOTES.md)
//   goto <screen>      title (the play link), menu, classic, adventure, tutorial, options, highscores, credits: the
//                      menu's own links and dialogs
//   level <n>          a classic game from chamber n (1 to 225: pyramid (n-1)/15, chamber (n-1)%15) from the main menu, or
//                      the board of that chamber in a classic game that is up
//   unlock all         opens every pyramid of the adventure's map
//   set <name> <v>     score <n>; botmode classic|adventure|tutorial (what `move` starts from the main menu); stepinput
//                      keys|clicks (the real input of a step: the arrow key, or a click on the cell it goes to)
//   win, lose          the chamber ends as the game ends it when the explorer leaves, or is caught
//   seed <n>           seeds the C library generator all the game's draws come from
//   move               the move the bot would make next: the solver's way out (Board::FindWay, from where the
//                      explorer stands) as the arrow key that takes the first step, or the click or keys that get a
//                      screen out of the way (title, menu, dialogs, map, level complete, name entry). It reads the game
//                      and changes nothing; it draws no random number (the bot of Autoplay.cpp has a generator of its
//                      own, this one has no dice: it always plays the solver's way).

#include "MummyApp.h"
#include "Board.h"
#include "Screens.h"
#include "Dialogs.h"
#include "Common.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Dialog.h"
#include "WidgetManager.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "GameLevels.h"
#include "Tables.inc"

using namespace Sexy;

namespace
{

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs) + ": " + theReason);
}

std::string BotMode = "classic";
std::string StepInput = "keys";		// how the bot moves the explorer: "keys" (the arrow keys and Space) or "clicks" (on the cell)

// the middle of a widget in the window (the game's own coordinates: 640 x 480)
void Centre(Widget* theWidget, int& theX, int& theY)
{
	Point aPos = theWidget->GetAbsPos();
	theX = aPos.mX + theWidget->mWidth / 2;
	theY = aPos.mY + theWidget->mHeight / 2;
}

std::string ClickStep(Widget* theWidget)
{
	int aX, aY;
	Centre(theWidget, aX, aY);
	return StrFormat("{\"click\":[%d,%d]}", aX, aY);
}

// A move: kind, what the real input is as an ordered list, and the updates to wait afterwards
std::string Move(const char* theKind, const std::string& theSteps, int theUpdates, const std::string& theExtra = "")
{
	return StrFormat("MOVE {\"kind\":\"%s\"%s,\"input\":[%s],\"updates\":%d}", theKind, theExtra.c_str(), theSteps.c_str(), theUpdates);
}

// the real input for a step of the explorer: the key, or a click on the cell it goes to (the board is at x 0x96, its
// cells 10 pixels to the right of its own origin, Board::MouseDown)
std::string StepInputFor(Board* b, int theKey, int theCellX, int theCellY);

const char* KeyName(int theKey)
{
	switch (theKey)
	{
	case KEYCODE_UP: return "ArrowUp";
	case KEYCODE_DOWN: return "ArrowDown";
	case KEYCODE_LEFT: return "ArrowLeft";
	case KEYCODE_RIGHT: return "ArrowRight";
	default: return " ";
	}
}

std::string StepInputFor(Board* b, int theKey, int theCellX, int theCellY)
{
	if (StepInput == "clicks")
		return StrFormat("{\"click\":[%d,%d]}", b->mX + 10 + b->GetXAt(theCellX) + b->mCellSize / 2,
			b->mY + b->GetYAt(theCellY) + b->mCellSize / 2);
	return StrFormat("{\"key\":\"%s\"}", KeyName(theKey));
}

// the dialog that is up, the newest (highest id) first, or -1
int TopDialog(MMApp* theApp)
{
	for (int anId = 31; anId >= 0; anId--)
		if (theApp->GetDialog(anId) != NULL)
			return anId;
	return -1;
}

// The bot's plan for the chamber it is in (see BotMove)
struct Plan
{
	Board*					mBoard;
	int						mMaze, mFile, mMode;
	ChaseState				mStart;			// the state the plan was made from
	std::vector<ChaseState>	mPath;			// the states after each step of the way out
	Plan() : mBoard(NULL), mMaze(0), mFile(0), mMode(0) {}
} gPlan;

// where a dead mummy or scorpion lies does not matter
ChaseState NormalizeChase(ChaseState s)
{
	for (int i = 0; i < 2; i++)
		if (!s.mMummyAlive[i])
			s.mMummyX[i] = s.mMummyY[i] = 0;
	if (!s.mScorpionAlive)
		s.mScorpionX = s.mScorpionY = 0;
	return s;
}

bool BoardReady(Board* b)
{
	if (b->mState != STATE_READY || b->mStungTimer >= 0 || b->mRiseFade < 1.0 || b->mShowingSolution)
		return false;
	for (int i = 0; i < 2; i++)
		if (b->mMummies[i] != NULL && b->mMummies[i]->mFighting)
			return false;
	return true;
}

const char* ScreenName(MMApp* theApp)
{
	if (theApp->GetDialogCount() > 0)
		return "dialog";
	if (theApp->mTitleScreen != NULL)
		return theApp->mTitleScreen->mPlayShown ? "title" : "loading";
	if (theApp->mWorldMap != NULL)
		return "map";
	if (theApp->mMainMenu != NULL)
		return "menu";
	if (theApp->mHighScores != NULL)
		return "highscores";
	if (theApp->mLevelComplete != NULL)
		return "complete";
	if (theApp->mGamePanel != NULL)
		return "lost";
	if (theApp->mVictory != NULL)
		return "victory";
	if (theApp->mBoard != NULL)
		return "board";
	return "none";
}

const char* ModeName(int theMode)
{
	switch (theMode)
	{
	case MODE_CLASSIC: return "classic";
	case MODE_ADVENTURE: return "adventure";
	case MODE_ADVENTURE_DONE: return "adventure-treasure";
	case MODE_CLASSIC_DONE: return "classic-treasure";
	default: return "tutorial";
	}
}

// A game that has ended: the victory screen or the high score table after a lost game, with its name asked and the
// screen settled; or the tutorial handed over to the classic game that follows it
bool GameOver(MMApp* a)
{
	if (a->mVictory != NULL && a->mVictory->mFade >= 255.0 && a->mVictory->mNameAsked)
		return true;
	if (a->mHighScores != NULL && a->mHighScores->mFade >= 255.0 && a->mHighScoreBack == NULL && a->mHighScores->mNameAsked)
		return true;
	return BotMode == "tutorial" && a->mGameMode == MODE_CLASSIC && a->mBoard != NULL;
}

std::string StateJson(MMApp* theApp)
{
	Board* aBoard = theApp->mBoard;
	int aPyramid = theApp->mGameMode == MODE_ADVENTURE ? theApp->mAdvPyramid : theApp->mClassicPyramid;
	int aChamber = theApp->mGameMode == MODE_ADVENTURE ? theApp->mAdvChamber : theApp->mClassicChamber;
	std::string aJson = StrFormat("{\"screen\":\"%s\",\"mode\":\"%s\",\"pyramid\":%d,\"chamber\":%d,\"level\":%d,\"score\":%d,"
		"\"update\":%d,\"over\":%s,\"botmode\":\"%s\",\"stepinput\":\"%s\"", ScreenName(theApp), ModeName(theApp->mGameMode), aPyramid,
		aChamber, aPyramid * 15 + aChamber + 1, theApp->mScore, theApp->mUpdateCount, GameOver(theApp) ? "true" : "false", BotMode.c_str(),
		StepInput.c_str());
	if (theApp->GetDialogCount() > 0)
		aJson += StrFormat(",\"dialog\":%d", TopDialog(theApp));
	if (theApp->mGameMode == MODE_ADVENTURE || theApp->mGameMode == MODE_ADVENTURE_DONE)
		aJson += StrFormat(",\"deaths\":%d,\"seconds\":%.0f", theApp->mDeaths, theApp->mTotalTime);
	if (theApp->mLevelComplete != NULL)
		aJson += StrFormat(",\"levelSeconds\":%d", theApp->mLevelComplete->mSeconds);
	if (aBoard != NULL)
	{
		aJson += StrFormat(",\"moves\":%d,\"timer\":%d", (int) aBoard->mUndo.size(), ((int) (theApp->mElapsedBase - aBoard->mTimeBase) + aBoard->mUpdateCnt) / 100);
		aJson += StrFormat(",\"board\":{\"size\":%d,\"state\":%d,\"ready\":%s,\"maze\":%d,\"file\":%d,\"dark\":%s,\"red\":%s,\"gate\":%s,"
			"\"key\":[%d,%d],\"unsolvable\":%s", aBoard->mSize, (int) aBoard->mState, BoardReady(aBoard) ? "true" : "false", aBoard->mMaze, aBoard->mFile,
			aBoard->mDark ? "true" : "false", aBoard->mRed ? "true" : "false", aBoard->mGateClosed ? "true" : "false", aBoard->mKeyX, aBoard->mKeyY,
			aBoard->mUnsolvable ? "true" : "false");
		if (aBoard->mExplorer != NULL)
			aJson += StrFormat(",\"explorer\":[%d,%d]", aBoard->mExplorer->mX, aBoard->mExplorer->mY);
		for (int i = 0; i < 2; i++)
			if (aBoard->mMummies[i] != NULL)
				aJson += StrFormat(",\"mummy%d\":[%d,%d,%d]", i, aBoard->mMummies[i]->mX, aBoard->mMummies[i]->mY, aBoard->mMummies[i]->mAlive ? 1 : 0);
		if (aBoard->mScorpion != NULL)
			aJson += StrFormat(",\"scorpion\":[%d,%d,%d]", aBoard->mScorpion->mX, aBoard->mScorpion->mY, aBoard->mScorpion->mAlive ? 1 : 0);
		// the skull traps (99 = none), the death on one (its timer and which of the two it is: 0 the block, 1 the pit), and the
		// cells' wall bits, a hex digit a cell, row by row (1 north, 2 south, 4 east, 8 west)
		aJson += StrFormat(",\"traps\":[[%d,%d],[%d,%d]],\"death\":[%d,%d],\"walls\":\"", aBoard->mTrapX[0], aBoard->mTrapY[0], aBoard->mTrapX[1],
			aBoard->mTrapY[1], aBoard->mTrapDeathTimer, aBoard->mExitStyle);
		for (int y = 0; y < aBoard->mSize; y++)
			for (int x = 0; x < aBoard->mSize; x++)
				aJson += StrFormat("%x", aBoard->mCells[x][y] & 0xf);
		aJson += "\"}";
	}
	return aJson + "}";
}

// What the bot does on a dialog: the first button, a name typed into the boxes that ask for one
std::string DialogMove(MMApp* a, int theId)
{
	Dialog* aDialog = a->GetDialog(theId);
	if (theId == DIALOG_RECORD_SCORE)
	{
		RecordScoreDialog* aRecord = (RecordScoreDialog*) aDialog;
		return Move("name", ClickStep(aRecord->mNameEdit) + ",{\"key\":\"b\"},{\"key\":\"o\"},{\"key\":\"t\"}," + ClickStep(aDialog->mYesButton), 5);
	}
	if (theId == DIALOG_NEW_ADVENTURE_NAME)
	{
		NameDialog* aName = (NameDialog*) aDialog;
		return Move("name", ClickStep(aName->mNameEdit) + ",{\"key\":\"b\"},{\"key\":\"o\"},{\"key\":\"t\"}," + ClickStep(aDialog->mYesButton), 5);
	}
	if (theId == DIALOG_ADVENTURE_NEW)
	{
		// the saved adventures: the first slot (it goes on with that game, or starts a new one when it is empty)
		AdventureDialog* aSlots = (AdventureDialog*) aDialog;
		return Move("dialog", ClickStep(aSlots->mSlots[0]), 5);
	}
	if (theId == DIALOG_CONTINUE && aDialog->mNoButton != NULL)
		return Move("dialog", ClickStep(aDialog->mNoButton), 5);	// a new game, not the saved one
	if (aDialog->mYesButton != NULL)
		return Move("dialog", ClickStep(aDialog->mYesButton), 5);
	return "";
}

// The move for the screen the game is on
std::string BotMove(MMApp* theApp)
{
	// the dialogs
	if (theApp->GetDialogCount() > 0)
	{
		int anId = TopDialog(theApp);
		std::string aMove = anId >= 0 ? DialogMove(theApp, anId) : "";
		return aMove.empty() ? std::string("MOVE null") : aMove;
	}
	// the title: the play link, once it has appeared
	if (theApp->mTitleScreen != NULL)
	{
		if (theApp->mTitleScreen->mPlayShown)
			return Move("title", ClickStep(theApp->mTitleScreen->mPlayLink), 20);
		return Move("wait", "", 60);
	}
	// the world map: the first pyramid that is open, once the marker stands still
	if (theApp->mWorldMap != NULL)
	{
		MapWidget* m = theApp->mWorldMap;
		if (m->mStep >= 0 || m->mCurrent != m->mTarget)
			return Move("wait", "", 20);
		for (int i = 0; i < 15; i++)
			if (m->mProgress[i] >= 0 && m->mProgress[i] < 15)
				return Move("map", ClickStep(m->mPyramids[i]), 10, StrFormat(",\"pyramid\":%d", i));
		return "MOVE null";
	}
	if (theApp->mMainMenu != NULL)
	{
		if (theApp->mMainMenu->mScrollState != 1)
			return Move("wait", "", 20);
		int aLink = BotMode == "tutorial" ? 3 : (BotMode == "adventure" ? 1 : 0);
		return Move("menu", ClickStep(theApp->mMainMenu->mLinks[aLink]), 10, StrFormat(",\"link\":%d", aLink));
	}
	// the table after a game, and the victory screen: look at them, then back to the menu
	if (theApp->mHighScores != NULL)
	{
		if (theApp->mHighScores->mFade >= 255.0 && theApp->mHighScoreBack == NULL && theApp->mHighScores->mNameAsked)
			return Move("menu-back", ClickStep(theApp->mHighScores->mMenuLink), 20);
		return Move("wait", "", 20);
	}
	if (BotMode == "tutorial" && theApp->mGameMode == MODE_CLASSIC)
		return "MOVE null";
	// a chamber is done: on to the next
	if (theApp->mLevelComplete != NULL)
		return Move("next", ClickStep(theApp->mLevelComplete->mNextLink), 5);
	// the panel of a lost explorer: try again
	if (theApp->mGamePanel != NULL)
	{
		if (theApp->mGamePanel->mScrollState != 1)
			return Move("wait", "", 20);
		return Move("again", ClickStep(theApp->mGamePanel->mLinks[1]), 5);
	}
	if (theApp->mVictory != NULL)
	{
		if (theApp->mVictory->mFade >= 255.0 && theApp->mVictory->mNameAsked)
			return Move("menu-back", ClickStep(theApp->mVictory->mMenuLink), 20);
		return Move("wait", "", 20);
	}
	Board* aBoard = theApp->mBoard;
	if (aBoard == NULL)
		return Move("wait", "", 20);

	// the tutorial: the button that is showing, or the move the table allows
	if (theApp->mGameMode >= MODE_TUTORIAL && theApp->mGameMode <= 8)
	{
		if (aBoard->mState != STATE_READY || aBoard->mStungTimer >= 0 || aBoard->mRiseFade < 1.0)
			return Move("wait", "", 5);
		for (int i = 0; i < 2; i++)
			if (aBoard->mMummies[i] != NULL && aBoard->mMummies[i]->mFighting)
				return Move("wait", "", 5);
		if (aBoard->mTutorialStepButton->mVisible || aBoard->mPlayNowButton->mVisible)
			return Move("tutorial-button", ClickStep(aBoard->mTutorialStepButton->mVisible ? (Widget*) aBoard->mTutorialStepButton : (Widget*) aBoard->mPlayNowButton), 10);
		static const int kKeys[5] = {KEYCODE_UP, KEYCODE_LEFT, KEYCODE_RIGHT, KEYCODE_DOWN, KEYCODE_SPACE};
		int aMove = kTutorialMove[aBoard->mTutorialStep];
		if (aMove < 0 || aMove > 4)
			return Move("wait", "", 5);
		int aDX = aMove == 1 ? -1 : (aMove == 2 ? 1 : 0), aDY = aMove == 0 ? -1 : (aMove == 3 ? 1 : 0);
		return Move("step", StepInputFor(aBoard, kKeys[aMove], aBoard->mExplorer->mX + aDX, aBoard->mExplorer->mY + aDY), 1);
	}
	// the treasure scene at the end of a pyramid: the link on
	if (theApp->mGameMode == MODE_CLASSIC_DONE || theApp->mGameMode == MODE_ADVENTURE_DONE)
	{
		if (aBoard->mContinueLink != NULL && aBoard->mContinueLink->mVisible)
			return Move("treasure", ClickStep(aBoard->mContinueLink), 20);
		return Move("wait", "", 20);
	}
	if (!BoardReady(aBoard) || (theApp->mGameMode != MODE_CLASSIC && theApp->mGameMode != MODE_ADVENTURE))
		return Move("wait", "", 5);

	// the maze: the solver's way out. The plan is made once from where the explorer stands and followed while the board
	// keeps to it (the search is a depth first one that keeps the shortest way it found, so a new search at every step
	// could choose another way each time and walk in circles); it is made again when the board is off the plan, as the
	// autoplay bot does (Autoplay.cpp). The search is the one of Abandon Hope, with the fixes mod's key-stone fight when
	// that mod is on.
	if (aBoard->mExplorer == NULL || theApp->mSidePanel == NULL)
		return Move("wait", "", 5);
	ChaseState aHave = NormalizeChase(aBoard->GetChaseState());
	int aIndex = -1;
	if (gPlan.mBoard == aBoard && gPlan.mMaze == aBoard->mMaze && gPlan.mFile == aBoard->mFile && gPlan.mMode == theApp->mGameMode)
	{
		if (aHave == gPlan.mStart)
			aIndex = 0;
		else
			for (size_t i = 0; i < gPlan.mPath.size() && aIndex < 0; i++)
				if (NormalizeChase(gPlan.mPath[i]) == aHave)
					aIndex = (int) i + 1;
	}
	if (aIndex < 0 || aIndex >= (int) gPlan.mPath.size())
	{
		std::vector<ChaseState> aFound;
		if (!aBoard->FindWay(aFound) || aFound.empty())
		{
			// no way out from here: take the moves back, which the side panel's RESET MAZE does
			gPlan.mBoard = NULL;
			if (aBoard->mUndo.empty())
				return "MOVE null";
			return Move("reset", ClickStep(theApp->mSidePanel->mButtons[1]), 10);
		}
		gPlan.mBoard = aBoard;
		gPlan.mMaze = aBoard->mMaze;
		gPlan.mFile = aBoard->mFile;
		gPlan.mMode = theApp->mGameMode;
		gPlan.mStart = aHave;
		gPlan.mPath = aFound;
		aIndex = 0;
	}
	std::vector<ChaseState> aPath(gPlan.mPath.begin() + aIndex, gPlan.mPath.end());
	const ChaseState& aNext = aPath[0];
	int dx = aNext.mExplorerX - aBoard->mExplorer->mX;
	int dy = aNext.mExplorerY - aBoard->mExplorer->mY;
	int aKey = dx < 0 ? KEYCODE_LEFT : (dx > 0 ? KEYCODE_RIGHT : (dy < 0 ? KEYCODE_UP : (dy > 0 ? KEYCODE_DOWN : KEYCODE_SPACE)));
	return Move("step", StepInputFor(aBoard, aKey, aNext.mExplorerX, aNext.mExplorerY), 1, StrFormat(",\"to\":[%d,%d],\"remaining\":%d", aNext.mExplorerX, aNext.mExplorerY, (int) aPath.size()));
}

}	// namespace

bool MMApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	std::string anArgs = theCommand.mArgs;
	MMApp* anApp = this;

	if (aCommand == "state")
	{
		Reply("STATE " + StateJson(anApp));
		return true;
	}

	if (aCommand == "move")
	{
		Reply(BotMove(anApp));
		return true;
	}

	if (aCommand == "seed")
	{
		if (anArgs.empty())
		{
			ReplyError(theCommand, "seed needs a number");
			return true;
		}
		CrtSRand((unsigned int) strtoul(anArgs.c_str(), NULL, 10));
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "set")
	{
		size_t aSpace = anArgs.find(' ');
		std::string aName = anArgs.substr(0, aSpace);
		std::string aValue = aSpace == std::string::npos ? "" : anArgs.substr(aSpace + 1);
		if (aName == "score" && !aValue.empty())
			mScore = atoi(aValue.c_str());
		else if (aName == "stepinput" && (aValue == "keys" || aValue == "clicks"))
			StepInput = aValue;
		else if (aName == "botmode" && (aValue == "classic" || aValue == "adventure" || aValue == "tutorial"))
			BotMode = aValue;
		else
		{
			ReplyError(theCommand, "set score <n>, set botmode classic|adventure|tutorial or set stepinput keys|clicks");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "goto")
	{
		if (anArgs == "title")
		{
			if (mTitleScreen == NULL)
				ReplyError(theCommand, "the title is only up at the start");
			else
				ReplyOK(theCommand);
			return true;
		}
		if (mTitleScreen != NULL)
		{
			if (!mTitleScreen->mPlayShown)
			{
				ReplyError(theCommand, "the game is still loading");
				return true;
			}
			mTitleScreen->ButtonDepress(1);
			if (anArgs == "menu")
			{
				ReplyOK(theCommand);
				return true;
			}
		}
		if (anArgs == "menu")
		{
			if (mMainMenu == NULL)
				ReplyError(theCommand, "the menu comes from the title; leave a game through its Quit to Main button");
			else
				ReplyOK(theCommand);
			return true;
		}
		if (mMainMenu == NULL || mMainMenu->mScrollState != 1)
		{
			ReplyError(theCommand, std::string("only from the main menu (now ") + ScreenName(anApp) + ")");
			return true;
		}
		if (anArgs == "classic")
			mMainMenu->ButtonDepress(0);
		else if (anArgs == "adventure")
			mMainMenu->ButtonDepress(1);
		else if (anArgs == "tutorial")
			mMainMenu->ButtonDepress(3);
		else if (anArgs == "options")
			DoOptions();
		else if (anArgs == "highscores")
			DoHighScoreDialog();
		else if (anArgs == "credits")
			DoMessage(DIALOG_CREDITS, GT(0x00495158), GT(0x00495160), 3, 0xa2, 0x52, 0x13c);
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
		if (aLevel < 1 || aLevel > 225)
		{
			ReplyError(theCommand, "level 1 to 225");
			return true;
		}
		int aPyramid = (aLevel - 1) / 15, aChamber = (aLevel - 1) % 15;
		if (mMainMenu != NULL && mMainMenu->mScrollState == 1)
		{
			// as the bot's -autoplaystart does it: the classic link, then the place
			mMainMenu->ButtonDepress(0);
			mClassicPyramid = aPyramid;
			mClassicChamber = aChamber;
			ReplyOK(theCommand);
			return true;
		}
		if (mBoard != NULL && mGameMode == MODE_CLASSIC && GetDialogCount() == 0 && mLevelComplete == NULL && mGamePanel == NULL)
		{
			mClassicPyramid = aPyramid;
			mClassicChamber = aChamber;
			NewGame();
			ReplyOK(theCommand);
			return true;
		}
		ReplyError(theCommand, std::string("only from the main menu or a classic board (now ") + ScreenName(anApp) + ")");
		return true;
	}

	if (aCommand == "unlock")
	{
		if (anArgs != "all")
		{
			ReplyError(theCommand, "only \"unlock all\"");
			return true;
		}
		if (mWorldMap == NULL)
		{
			ReplyError(theCommand, "only on the adventure's world map (nothing else in the game is locked)");
			return true;
		}
		for (int i = 0; i < 15; i++)
			if (mWorldMap->mProgress[i] < 0)
				mWorldMap->mProgress[i] = 0;
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "win" || aCommand == "lose")
	{
		Board* aBoard = mBoard;
		if (aBoard == NULL || mLevelComplete != NULL || mGamePanel != NULL || (mGameMode != MODE_CLASSIC && mGameMode != MODE_ADVENTURE))
		{
			ReplyError(theCommand, "only on a classic or adventure board in play");
			return true;
		}
		if (aCommand == "win")
			DoNextLevel();
		else if (mGameMode == MODE_ADVENTURE)
			AdventureCaught();
		else
			aBoard->DoGameOver();
		ReplyOK(theCommand);
		return true;
	}

	return SexyApp::HandleScriptCommand(theCommand);
}
