// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command channel
// through HandleScriptCommand. A command that is never given changes nothing. Each answers on stdout with one line:
// STATE {...}, MOVE {...}, OK <command> or ERROR <command>: <reason>. They act through the game's own code: the screens'
// own functions, the profile's records, the board's own throw and pull. The moves come from the best known solutions
// (levels/maxedscores.txt, the same moves -autoplay plays), so a bot mod can reuse them.

#include "Checkbox.h"
#include "PixelusApp.h"
#include "Board.h"
#include "Grid.h"
#include "TileMaster.h"
#include "TestHooks.h"
#include "IslandMap.h"
#include "TempleScreen.h"
#include "PuzzleCompleted.h"
#include "StoryScreen.h"
#include "CompletionScreen.h"
#include "HelpScreen.h"
#include "MainMenu.h"
#include "LoadingScreen.h"
#include "Temples.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "UserDirectory.h"
#include "WidgetManager.h"
#include "ButtonWidget.h"
#include "Common.h"
#include "Compat2004.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Sexy;

namespace {

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand.mCommand + ": " + theReason);
}

// the next move of the best known solution of the board's puzzle, after the moves the board has made
bool NextSolutionMove(PixelusApp* theApp, Board* theBoard, Grid::Move* theMove)
{
	const ProgressManager::Best* aBest = theApp->mProgress->FindBest(theBoard->mUuid);
	if (aBest == NULL)
		return false;
	const std::vector<Grid::Move>& aHistory = theBoard->mGrid->mHistory;
	size_t aSkip = 0;
	for (size_t i = 0; i < aHistory.size(); i++, aSkip += 6)
		if (aBest->mSolution.compare(aSkip, 6, Grid::MoveToString(aHistory[i])) != 0)
			return false;	// the player is off the solution
	if (aSkip + 6 > aBest->mSolution.size())
		return false;
	return Grid::ParseMove(aBest->mSolution.substr(aSkip, 6), theMove);
}

// "win": the moves still to make go in one by one as the board settles
bool gWinning = false;

}  // namespace

std::string PixelusApp::TestScreen()
{
	if (mLoadingScreen != NULL)
		return "loading";
	if (GetDialogCount() > 0)
		return "dialog";
	if (mHelpScreen != NULL)
		return "help";
	if (mCompletion != NULL)
		return "completion";
	if (mStoryScreen != NULL)
		return "story";
	if (mPuzzleCompleted != NULL)
		return "completed";
	if (mBoard != NULL)
		return "board";
	if (mTempleScreen != NULL)
		return "temple";
	if (mIslandMap != NULL)
		return "island";
	if (mMainMenu != NULL)
		return "menu";
	return "none";
}

// every screen goes, so the next one comes onto an empty window
void PixelusApp::TestClearScreens()
{
	gWinning = false;
	for (int anId = 0; anId < 64; anId++)
		if (GetDialog(anId) != NULL)
			KillDialog(anId);
	Widget** aScreens[] = { (Widget**) &mHelpScreen, (Widget**) &mCompletion, (Widget**) &mStoryScreen,
		(Widget**) &mPuzzleCompleted, (Widget**) &mBoard, (Widget**) &mTempleScreen, (Widget**) &mIslandMap,
		(Widget**) &mMainMenu };
	for (size_t i = 0; i < sizeof(aScreens) / sizeof(aScreens[0]); i++)
	{
		Widget* aWidget = *aScreens[i];
		if (aWidget == NULL)
			continue;
		*aScreens[i] = NULL;
		mWidgetManager->RemoveWidget(aWidget);
		SafeDeleteWidget(aWidget);
	}
	Pause(false);
}

// called from every update: "win" plays the rest of the solution
void PixelusApp::TestUpdate()
{
	if (!gWinning)
		return;
	if (mBoard == NULL || mBoard->mGrid->mDone)
	{
		gWinning = false;
		return;
	}
	if (mBoard->mGrid->mTileMoving || (mUpdateCount % 4) != 0)
		return;
	Grid::Move aMove;
	if (!NextSolutionMove(this, mBoard, &aMove))
	{
		gWinning = false;
		return;
	}
	mBoard->Aim(aMove.mSide, aMove.mCursor);
	if (aMove.mRemove)
		mBoard->DoPull();
	else
		mBoard->DoThrow();
}

// the click targets of the screen, for a driver that plays with real input: every enabled button in the tree, and the
// puzzles in view on the temple screen (centres in game pixels)
static void CollectTargets(WidgetContainer* theContainer, int theX, int theY, std::string& theOut)
{
	for (WidgetList::iterator anItr = theContainer->mWidgets.begin(); anItr != theContainer->mWidgets.end(); ++anItr)
	{
		Widget* aWidget = *anItr;
		if (!aWidget->mVisible)
			continue;
		int aX = theX + aWidget->mX, aY = theY + aWidget->mY;
		ButtonWidget* aButton = dynamic_cast<ButtonWidget*>(aWidget);
		if (aButton != NULL && !aButton->mDisabled && aWidget->mWidth > 0)
			theOut += StrFormat("%s{\"id\":%d,\"label\":\"%s\",\"x\":%d,\"y\":%d}", theOut.empty() ? "" : ",", aButton->mId,
				aButton->mLabel.c_str(), aX + aWidget->mWidth / 2, aY + aWidget->mHeight / 2);
		CollectTargets(aWidget, aX, aY, theOut);
	}
}

// the check boxes in view (the new player's expert box, the help screen's), for a driver that clicks them
static void CollectChecks(WidgetContainer* theContainer, int theX, int theY, std::string& theOut)
{
	for (WidgetList::iterator anItr = theContainer->mWidgets.begin(); anItr != theContainer->mWidgets.end(); ++anItr)
	{
		Widget* aWidget = *anItr;
		if (!aWidget->mVisible)
			continue;
		int aX = theX + aWidget->mX, aY = theY + aWidget->mY;
		Checkbox* aBox = dynamic_cast<Checkbox*>(aWidget);
		if (aBox != NULL)
			theOut += StrFormat("%s{\"id\":%d,\"checked\":%s,\"x\":%d,\"y\":%d}", theOut.empty() ? "" : ",", aBox->mId,
				aBox->mChecked ? "true" : "false", aX + aWidget->mWidth / 2, aY + aWidget->mHeight / 2);
		CollectChecks(aWidget, aX, aY, theOut);
	}
}

bool PixelusApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	std::string anArgs = theCommand.mArgs;
	size_t aSpace = anArgs.find(' ');
	std::string aFirst = anArgs.substr(0, aSpace);
	std::string aRest = aSpace == std::string::npos ? "" : anArgs.substr(aSpace + 1);
	UserProfile* aUser = mUserDirectory != NULL ? mUserDirectory->GetCurrent() : NULL;

	if (aCommand == "state")
	{
		std::string aLine = StrFormat("STATE {\"screen\":\"%s\"", TestScreen().c_str());
		if (mLoadingScreen != NULL && mLoadingScreen->mLoadDone)
			aLine += ",\"ready\":true";
		if (aUser != NULL)
			aLine += StrFormat(",\"player\":\"%s\",\"solved\":%d,\"expert\":%s", aUser->mName.c_str(), aUser->CountSolved(),
				aUser->mFlag38 ? "true" : "false");
		if (mLevelData != NULL && !mLevelData->mCurrentLevel.empty())
			aLine += StrFormat(",\"temple\":\"%s\",\"level\":%d,\"puzzles\":%d", mLevelData->mCurrentLevel.c_str(),
				mLevelData->mCurrentPuzzle, mLevelData->GetPuzzleCount(mLevelData->mCurrentLevel));
		if (mBoard != NULL)
		{
			const Grid* g = mBoard->mGrid;
			int aTiles = 0;
			for (int r = 0; r < g->mRows; r++)
				for (int c = 0; c < g->mCols; c++)
					if (g->HasObject(r, c))
						aTiles++;
			aLine += StrFormat(",\"moves\":%d,\"timer\":%d,\"board\":{\"uuid\":\"%s\",\"rows\":%d,\"cols\":%d,\"tiles\":%d,"
				"\"done\":%s,\"busy\":%s,\"tutorial\":%s}", g->mMoveCount, g->mTime, mBoard->mUuid.c_str(), g->mRows, g->mCols,
				aTiles, g->mDone ? "true" : "false", g->mTileMoving ? "true" : "false", mBoard->mTutorial ? "true" : "false");
		}
		Reply(aLine + "}");
		return true;
	}

	if (aCommand == "targets")
	{
		std::string aButtons, aThumbs, aChecks;
		CollectTargets(mWidgetManager, 0, 0, aButtons);
		CollectChecks(mWidgetManager, 0, 0, aChecks);
		if (mTempleScreen != NULL)
			for (int aRow = 0; aRow < TempleScreen::ROWS; aRow++)
				for (int aCol = 0; aCol < TempleScreen::COLUMNS; aCol++)
				{
					int anIndex = mTempleScreen->mFirst + aRow * TempleScreen::COLUMNS + aCol;
					if (anIndex >= (int) mTempleScreen->mThumbs.size())
						continue;
					const TempleScreen::Thumb& aThumb = mTempleScreen->mThumbs[anIndex];
					aThumbs += StrFormat("%s{\"index\":%d,\"solved\":%s,\"mystery\":%s,\"x\":%d,\"y\":%d}", aThumbs.empty() ? "" : ",",
						anIndex, aThumb.mSolved ? "true" : "false", aThumb.mMystery ? "true" : "false",
						TempleScreen::LEFT + TempleScreen::PITCH * aCol + TempleScreen::CELL / 2,
						TempleScreen::TOP + TempleScreen::PITCH * aRow + TempleScreen::CELL / 2);
				}
		Reply(StrFormat("TARGETS {\"screen\":\"%s\",\"buttons\":[%s],\"puzzles\":[%s],\"checks\":[%s]}", TestScreen().c_str(), aButtons.c_str(),
			aThumbs.c_str(), aChecks.c_str()));
		return true;
	}

	if (aCommand == "move")
	{
		Grid::Move aMove;
		int aHoverX, aHoverY, aX, aY;
		if (mBoard == NULL || mBoard->mGrid->mDone || mBoard->mGrid->mTileMoving || !NextSolutionMove(this, mBoard, &aMove) ||
			!TestFindCell(mBoard, aMove, &aHoverX, &aHoverY, &aX, &aY))
		{
			Reply("MOVE null");
			return true;
		}
		std::string anInput;
		if (aHoverX >= 0)
			anInput = StrFormat("{\"move\":[%d,%d]},", aHoverX, aHoverY);
		anInput += StrFormat("{\"click\":[%d,%d]}", aX, aY);
		Reply(StrFormat("MOVE {\"kind\":\"%s\",\"side\":%d,\"cursor\":%d,\"input\":[%s]}", aMove.mRemove ? "pull" : "throw",
			aMove.mSide, aMove.mCursor, anInput.c_str()));
		return true;
	}

	if (aCommand == "goto")
	{
		if (aFirst == "temple" || aFirst == "island" || aFirst == "menu" || aFirst == "credits" || aFirst == "options" || aFirst == "help")
		{
			if (mLoadingScreen != NULL)
			{
				// the loading screen's play link, once the bar is full
				if (!mLoadingScreen->mLoadDone)
				{
					ReplyError(theCommand, "the game is still loading");
					return true;
				}
				PlayClicked();
			}
			if (aFirst != "menu" && aUser == NULL)
			{
				ReplyError(theCommand, "no player yet");
				return true;
			}
			TestClearScreens();
			if (aFirst == "menu")
				ShowMainMenu();
			else if (aFirst == "island")
				ShowIslandMap();
			else if (aFirst == "temple")
			{
				std::string aName = aRest;
				int aTemple = aName.empty() ? 0 : -1;
				for (int i = 0; i < NUM_TEMPLES && !aName.empty(); i++)
					if (aName == gTemples[i].mName || aName == StrFormat("%d", i))
						aTemple = i;
				if (aTemple < 0)
				{
					ShowMainMenu();
					ReplyError(theCommand, "no such temple: " + aName);
					return true;
				}
				ShowIslandMap();
				mLevelData->mCurrentLevel = gTemples[aTemple].mName;
				ShowTemple();
			}
			else
			{
				ShowMainMenu();
				if (aFirst == "credits")
					ShowCredits();
				else if (aFirst == "options")
					ShowOptions();
				else
					ShowHelp();
			}
			Reply("OK goto " + anArgs);
			return true;
		}
		ReplyError(theCommand, "goto menu | island | temple [name or number] | credits | options | help");
		return true;
	}

	if (aCommand == "level")
	{
		// level [<temple>] <n>: the puzzle n of the temple (the current one), started as the temple screen starts it
		std::string aTemple = mLevelData->mCurrentLevel;
		std::string aNumber = anArgs;
		if (!aRest.empty())
		{
			aTemple = aFirst;
			aNumber = aRest;
		}
		for (int i = 0; i < NUM_TEMPLES; i++)
			if (aTemple == StrFormat("%d", i))
				aTemple = gTemples[i].mName;
		const LevelData::Level* aLevel = mLevelData->GetLevel(aTemple);
		int aPuzzle = atoi(aNumber.c_str());
		if (aUser == NULL || aLevel == NULL || aNumber.empty() || aPuzzle < 0 || aPuzzle >= (int) aLevel->mPuzzles.size())
		{
			ReplyError(theCommand, "level [<temple>] <puzzle number>, with a player and a temple");
			return true;
		}
		TestClearScreens();
		mLevelData->mCurrentLevel = aTemple;
		mLevelData->mCurrentPuzzle = aPuzzle;
		StartPuzzle(true);
		Reply("OK level " + anArgs);
		return true;
	}

	if (aCommand == "unlock")
	{
		if (aFirst != "all" || aUser == NULL)
		{
			ReplyError(theCommand, "unlock all, with a player");
			return true;
		}
		for (int i = 0; i < NUM_TEMPLES; i++)
		{
			const LevelData::Level* aLevel = mLevelData->GetLevel(gTemples[i].mName);
			if (aLevel == NULL)
				continue;
			for (size_t p = 0; p < aLevel->mPuzzles.size(); p++)
			{
				const std::string& aUuid = aLevel->mPuzzles[p].mUuid;
				if (aUser->GetRecord(aUuid) != NULL)
					continue;
				int aMoves = mProgress->GetBestMoves(aUuid);
				aUser->SetRecord(aUuid, 1, aMoves > 0 ? aMoves : 1, 0);
			}
		}
		mUserDirectory->Save();
		Reply("OK unlock all");
		return true;
	}

	if (aCommand == "set")
	{
		if (mBoard == NULL)
		{
			ReplyError(theCommand, "not in a puzzle");
			return true;
		}
		int aValue = atoi(aRest.c_str());
		if (aFirst == "timer")
			mBoard->mGrid->mTime = aValue * 1000;	// seconds, kept in milliseconds
		else
		{
			ReplyError(theCommand, "can set timer (seconds on the puzzle)");
			return true;
		}
		Reply("OK set " + anArgs);
		return true;
	}

	if (aCommand == "seed")
	{
		SeedMTRand2004(mAppRand, (unsigned int) atoi(anArgs.c_str()));
		Reply("OK seed " + anArgs);
		return true;
	}

	if (aCommand == "win")
	{
		if (mBoard == NULL || mBoard->mGrid->mDone)
		{
			ReplyError(theCommand, "not in an unfinished puzzle");
			return true;
		}
		Grid::Move aMove;
		if (!NextSolutionMove(this, mBoard, &aMove))
		{
			ReplyError(theCommand, "no best known solution from the board's moves");
			return true;
		}
		gWinning = true;
		Reply("OK win");
		return true;
	}

	if (aCommand == "lose")
	{
		ReplyError(theCommand, "a puzzle cannot be lost");
		return true;
	}

	return false;
}
