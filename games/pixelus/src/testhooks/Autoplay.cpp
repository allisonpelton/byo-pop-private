// Test hooks: the autoplay bot (-autoplay=<seed>). It plays puzzles the way a player would: it makes a player, presses
// play, walks from the island to a temple and into the first puzzle that is not solved, throws and pulls the tiles of the
// best known solution (levels/maxedscores.txt) through the board's own handlers, and presses the tablet's button when the
// puzzle is done. So everything the game does for a player runs: the screens come and go, the results go into the
// player's record and into the table of the best solutions, temples open. The bot has its own random numbers (the
// game's use the C library generator, seeded by -seed); the same -seed and -autoplay always make the same run. It writes
// a log of what happened (-autoplaylog=<file>) that tests compare.
//
//   -autoplay=<seed>         play with the bot (its own seed)
//   -autoplaypuzzles=N       solve N puzzles, then quit (default 3)
//   -autoplaycap=N           quit after N updates in any case
//   -autoplaylog=<file>      the log
//   -autoplayshots=<prefix>  screenshots of each screen the bot comes to
//   -autoplayskip            with -autoplayui: press the tutorial's skip button first
//   -autoplayui              the bot presses with mouse clicks and typed characters (script commands, as a -script run
//                            gives them) instead of calling the handlers
//   -autoplayscript=<file>   with -autoplayui: what it did as -script lines (origrun replays them in the original)
//   -statelog=<file>         (any run) each change of the screens' and the puzzle's values with its update count: StateLog

#include "PixelusApp.h"
#include "StateLog.h"
#include "Compat2004.h"
#include "HelpScreen.h"
#include "Board.h"
#include "IslandMap.h"
#include "TempleScreen.h"
#include "PuzzleCompleted.h"
#include "StoryScreen.h"
#include "CompletionScreen.h"
#include "MainMenu.h"
#include "LoadingScreen.h"
#include "Temples.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "UserDirectory.h"
#include "UserDialogs.h"
#include "Grid.h"
#include "Graphics.h"
#include "WidgetManager.h"
#include "HyperlinkWidget.h"
#include "EditWidget.h"
#include "Checkbox.h"
#include "DialogButton.h"
#include "TileMaster.h"
#include "TestHooks.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <vector>

using namespace Sexy;

namespace Sexy
{

struct AutoplayState
{
	FILE*			mScript;			// -autoplayscript
	int				mNewUserSeen;
	unsigned int	mSeed;
	int				mTick;
	int				mWait;				// updates to wait before the next action
	int				mSolved;			// puzzles finished by the bot
	int				mMoveIndex;			// the next move of the solution
	int				mScreenTicks;		// updates on the screen the bot is waiting on
	std::vector<Grid::Move>	mSolution;
	std::string		mUuid;
	unsigned int	mScreens;			// bit per kind of screen: its screenshot was taken
	FILE*			mLog;

	AutoplayState() : mScript(NULL), mNewUserSeen(0), mSeed(1), mTick(0), mWait(0), mSolved(0), mMoveIndex(-1), mScreenTicks(0), mScreens(0), mLog(NULL) {}
};

}

// port: a helper of the port (PixelusApp::HandleCmdLineParam)
void PixelusApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplayState = new AutoplayState();
		mAutoplayState->mSeed = (unsigned int) atoi(theParamValue.c_str());
		mScriptedInput = true;   // the bot's input only, as in a -script run
	}
	else if (theParamName == "-autoplaypuzzles")
		mAutoplayPuzzles = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLog = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShots = theParamValue;
	else if (theParamName == "-autoplayshow")
		mAutoplayShow = theParamValue;
	else if (theParamName == "-autoplayui")
		mAutoplayUI = true;
	else if (theParamName == "-autoplayskip")
		mAutoplaySkip = true;
	else if (theParamName == "-autoplayscript")
		mAutoplayScript = theParamValue;
	else if (theParamName == "-statelog")
		mStateLogFileName = theParamValue;
	else
		NcApp::HandleCmdLineParam(theParamName, theParamValue);
}

namespace
{

void Log(AutoplayState* s, const char* theFormat, ...)
{
	if (s->mLog == NULL)
		return;
	va_list args;
	va_start(args, theFormat);
	vfprintf(s->mLog, theFormat, args);
	va_end(args);
	fputc('\n', s->mLog);
	fflush(s->mLog);
}

// a screenshot, the first time a kind of screen comes up
void Shot(PixelusApp* theApp, AutoplayState* s, int theKind, const char* theName)
{
	if (theApp->mAutoplayShots.empty() || (s->mScreens & (1u << theKind)))
		return;
	s->mScreens |= 1u << theKind;
	theApp->AutoplayShot(theApp->mAutoplayShots + "-" + theName + ".png");
}

enum { SHOT_MENU, SHOT_ISLAND, SHOT_TEMPLE, SHOT_BOARD, SHOT_BOARD_END, SHOT_COMPLETED, SHOT_COMPLETED_2, SHOT_SWEEP, SHOT_PLAY };

// the first temple that is open and has a puzzle without a result; -1 when there is none
int FindTemple(PixelusApp* theApp, UserProfile* theUser)
{
	for (int i = theApp->mAutoplaySkip ? 1 : 0; i < NUM_TEMPLES; i++)
	{
		if (IsTempleLocked(theApp, i) || !IsTempleOpen(theApp, i))
			continue;
		if (theUser->CountSolved(gTemples[i].mName) < theApp->mLevelData->GetPuzzleCount(gTemples[i].mName))
			return i;
	}
	return -1;
}

// the first puzzle of a temple without a result (the tutorial's come in order, the mystery one is the last)
int FindPuzzle(PixelusApp* theApp, UserProfile* theUser, const std::string& theTemple)
{
	const LevelData::Level* aLevel = theApp->mLevelData->GetLevel(theTemple);
	if (aLevel == NULL)
		return -1;
	for (size_t i = 0; i < aLevel->mPuzzles.size(); i++)
		if (theUser->GetRecord(aLevel->mPuzzles[i].mUuid) == NULL)
			return (int) i;
	return -1;
}

// the bot's input as -script commands: run by the next update, and written to -autoplayscript for the original
void Script(PixelusApp* theApp, AutoplayState* s, const char* theCommand, const char* theArgs, const char* theNote = "")
{
	theApp->AddScriptCommand(theApp->mUpdateCount + 1, theCommand, theArgs);
	if (s->mScript != NULL)
	{
		fprintf(s->mScript, "%d %s %s%s\n", theApp->mUpdateCount + 1, theCommand, theArgs, theNote);
		fflush(s->mScript);
	}
}

void ClickAt(PixelusApp* theApp, AutoplayState* s, int x, int y, const char* theNote = "")
{
	char anArgs[32];
	snprintf(anArgs, sizeof(anArgs), "%d %d", x, y);
	Script(theApp, s, "click", anArgs, theNote);
}

void ClickWidget(PixelusApp* theApp, AutoplayState* s, Widget* theWidget)
{
	// the note tells tools/identical/toorig.py that a button is clicked, which the original wants hovered a little earlier
	ClickAt(theApp, s, theWidget->mX + theWidget->mWidth / 2, theWidget->mY + theWidget->mHeight / 2, " #button");
}

// the side a click on a cell would use, with the tile maker facing thePreferred (Board::MouseDown picks it from the cell)
int ClickSide(const Grid* g, int r, int c, int thePreferred, int* theCursor)
{
	return g->HasObject(r, c) ? g->FindPullSide(r, c, thePreferred, theCursor) : g->FindThrowSide(r, c, thePreferred, theCursor);
}

// the pixels of a cell where a click makes the move: a throw into an empty cell or a pull of the tile in one, from the side
// and the place along it that the solution says. A click uses the side the tile maker faces if the cell allows it, so the
// bot first moves the mouse over a cell that turns the tile maker to the side it needs (the way a player aims)
bool FindCell(Board* theBoard, const Grid::Move& theMove, int* theHoverX, int* theHoverY, int* theX, int* theY)
{
	const Grid* g = theBoard->mGrid;
	int aSide = theBoard->mTileMaster->GetSide();
	*theHoverX = *theHoverY = -1;
	for (int aTry = 0; aTry < 5; aTry++)
	{
		int aPreferred = aTry == 0 ? aSide : aTry - 1;
		if (aTry > 0 && aPreferred == aSide)
			continue;
		for (int aRow = 0; aRow < g->mRows; aRow++)
			for (int aCol = 0; aCol < g->mCols; aCol++)
			{
				if (g->HasObject(aRow, aCol) != theMove.mRemove)
					continue;
				int aCursor = 0;
				if (ClickSide(g, aRow, aCol, aPreferred, &aCursor) != theMove.mSide || aCursor != theMove.mCursor)
					continue;
				if (aPreferred != aSide)
				{
					// a cell the mouse can pass over to turn the tile maker to aPreferred
					bool aHover = false;
					for (int r2 = 0; r2 < g->mRows && !aHover; r2++)
						for (int c2 = 0; c2 < g->mCols && !aHover; c2++)
						{
							int aCursor2 = 0;
							if (ClickSide(g, r2, c2, aSide, &aCursor2) == aPreferred)
							{
								*theHoverX = Board::GRID_X + c2 * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
								*theHoverY = Board::GRID_Y + r2 * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
								aHover = true;
							}
						}
					if (!aHover)
						continue;
				}
				*theX = Board::GRID_X + aCol * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
				*theY = Board::GRID_Y + aRow * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
				return true;
			}
	}
	return false;
}

}

// port: the constructor (StateLog::StateLog)
StateLog::StateLog(PixelusApp* theApp)
{
	mApp = theApp;
	mFile = fopen(theApp->mStateLogFileName.c_str(), "w");
	for (int i = 0; i < NUM_FIELDS; i++)
		mLast[i] = -999999;
}

// port: the destructor (StateLog::~StateLog)
StateLog::~StateLog()
{
	if (mFile != NULL)
		fclose(mFile);
}

// port: a helper of the port (StateLog::Update)
void StateLog::Update()
{
	if (mFile == NULL)
		return;

	static const char* const aNames[NUM_FIELDS] = {"load", "menu", "story", "island", "temple", "board", "completed", "help",
		"moves", "time", "done", "supply", "cursor", "side", "empty", "stray", "reveal", "moving", "objects",
		"o0x", "o0y", "o0state", "o1x", "o1y", "o1state", "o2x", "o2y", "o2state", "o2variant", "wanted", "rand", "o0variant", "o1variant", "o3variant", "apprand"};
	int aValues[NUM_FIELDS];
	for (int i = 0; i < NUM_FIELDS; i++)
		aValues[i] = -1000000;  // "-": no board or no such tile
	aValues[0] = mApp->mLoadingScreen != NULL;
	aValues[1] = mApp->mMainMenu != NULL;
	aValues[2] = mApp->mStoryScreen != NULL;
	aValues[3] = mApp->mIslandMap != NULL;
	aValues[4] = mApp->mTempleScreen != NULL;
	aValues[5] = mApp->mBoard != NULL;
	aValues[6] = mApp->mPuzzleCompleted != NULL;
	aValues[7] = mApp->mHelpScreen != NULL;
	if (mApp->mBoard != NULL)
	{
		const Grid* g = mApp->mBoard->mGrid;
		aValues[8] = g->mMoveCount;
		aValues[9] = g->mTime;
		aValues[10] = g->mDone ? 1 : 0;
		aValues[11] = g->mSupply;
		aValues[12] = g->mCursor;
		aValues[13] = g->mSide;
		aValues[14] = g->mEmptyWanted;
		aValues[15] = g->mStrayTiles;
		aValues[16] = g->mReveal;
		aValues[17] = g->mTileMoving ? 1 : 0;
		aValues[18] = (int) g->mObjects.size();
		for (int i = 0; i < 3 && i < (int) g->mObjects.size(); i++)
		{
			aValues[19 + i * 3] = g->mObjects[i]->mX;
			aValues[20 + i * 3] = g->mObjects[i]->mY;
			aValues[21 + i * 3] = g->mObjects[i]->mState;
		}
		if (g->mObjects.size() > 2)
			aValues[28] = g->mObjects[2]->mVariant;
		for (int i = 0; i < 4 && i < (int) g->mObjects.size(); i++)
			if (i != 2)
				aValues[i == 3 ? 33 : 31 + i] = g->mObjects[i]->mVariant;
		aValues[29] = g->mWantedCells;
	}
	aValues[30] = (int) mApp->mRandDraws;
	aValues[34] = (int) mApp->mAppRandDraws;
	for (int i = 0; i < 35; i++)
	{
		if (aValues[i] == mLast[i])
			continue;
		mLast[i] = aValues[i];
		if (aValues[i] == -1000000)
			fprintf(mFile, "%d %s -\n", mApp->mUpdateCount, aNames[i]);
		else
			fprintf(mFile, "%d %s %d\n", mApp->mUpdateCount, aNames[i], aValues[i]);
	}
	fflush(mFile);
}

// the bot, once per update
// port: a helper of the port (PixelusApp::AutoplayUpdate)
void PixelusApp::AutoplayUpdate()
{
	AutoplayState* aState = mAutoplayState;
	if (aState == NULL)
		return;
	if (aState->mLog == NULL && !mAutoplayLog.empty())
		aState->mLog = fopen(mAutoplayLog.c_str(), "w");
	if (aState->mScript == NULL && !mAutoplayScript.empty())
		aState->mScript = fopen(mAutoplayScript.c_str(), "w");
	aState->mTick++;
	if (aState->mTick >= mAutoplayCap)
	{
		Log(aState, "cap reached at update %d", aState->mTick);
		Shutdown();
		return;
	}
	if (aState->mWait > 0)
	{
		aState->mWait--;
		return;
	}
	// the loading screen: wait until the bar is full, then press its play link
	if (mLoadingScreen != NULL)
	{
		if (mLoadingScreen->mLoadDone && ++aState->mScreenTicks > 150)
		{
			aState->mScreenTicks = 0;
			if (mAutoplayUI)
				ClickWidget(this, aState, (Widget*) mLoadingScreen->mPlayLink);
			else
				PlayClicked();
			aState->mWait = 20;
		}
		return;
	}

	// the dialogs: the one for a new player gets a name, the others their first button
	if (GetDialog(0x13) != NULL)
	{
		if (aState->mNewUserSeen == 0)
		{
			NewUserDialog* aDialog = (NewUserDialog*) GetDialog(0x13);
			Log(aState, "new player dialog %d,%d %dx%d edit %d,%d %dx%d box %d,%d %dx%d yes %d,%d %dx%d no %d,%d %dx%d", aDialog->mX, aDialog->mY,
				aDialog->mWidth, aDialog->mHeight, aDialog->mEdit->mX, aDialog->mEdit->mY, aDialog->mEdit->mWidth, aDialog->mEdit->mHeight,
				aDialog->mExpertBox->mX, aDialog->mExpertBox->mY, aDialog->mExpertBox->mWidth, aDialog->mExpertBox->mHeight,
				aDialog->mYesButton->mX, aDialog->mYesButton->mY, aDialog->mYesButton->mWidth, aDialog->mYesButton->mHeight,
				aDialog->mNoButton->mX, aDialog->mNoButton->mY, aDialog->mNoButton->mWidth, aDialog->mNoButton->mHeight);
			aState->mNewUserSeen = 1;
		}
		if (mAutoplayUI)
		{
			Script(this, aState, "char", "B");
			Script(this, aState, "char", "o");
			Script(this, aState, "char", "t");
			ClickWidget(this, aState, ((NewUserDialog*) GetDialog(0x13))->mYesButton);
		}
		else
		{
			((NewUserDialog*) GetDialog(0x13))->SetName("Bot");
			ButtonDepress(2019);
		}
		aState->mWait = 10;
		return;
	}
	if (GetDialogCount() > 0)
	{
		for (int anId = 0; anId < 64; anId++)
			if (GetDialog(anId) != NULL)
			{
				if (mAutoplayUI)
					ClickWidget(this, aState, GetDialog(anId)->mYesButton);
				else
					ButtonDepress(2000 + anId);
				break;
			}
		aState->mWait = 10;
		return;
	}

	UserProfile* aUser = mUserDirectory != NULL ? mUserDirectory->GetCurrent() : NULL;
	if (mMainMenu != NULL)
	{
		if (aUser == NULL)
			return;
		if (++aState->mScreenTicks < 150)
			return;
		if (!mAutoplayShow.empty())
		{
			// the board of one puzzle, with a record set first when the value has moves and a time
			std::vector<std::string> aParts;
			size_t aStart = 0;
			for (size_t aPos; (aPos = mAutoplayShow.find(':', aStart)) != std::string::npos; aStart = aPos + 1)
				aParts.push_back(mAutoplayShow.substr(aStart, aPos - aStart));
			aParts.push_back(mAutoplayShow.substr(aStart));
			const LevelData::Level* aLevel = aParts.size() >= 2 ? mLevelData->GetLevel(aParts[0]) : NULL;
			int anIndex = aParts.size() >= 2 ? atoi(aParts[1].c_str()) : -1;
			if (aLevel == NULL || anIndex < 0 || anIndex >= (int) aLevel->mPuzzles.size())
			{
				Log(aState, "-autoplayshow: no such puzzle");
				Shutdown();
				return;
			}
			if (aParts.size() >= 4)
				aUser->SetRecord(aLevel->mPuzzles[anIndex].mUuid, 1, atoi(aParts[2].c_str()), atoi(aParts[3].c_str()));
			Log(aState, "show %s %d", aParts[0].c_str(), anIndex);
			aState->mScreenTicks = 0;
			mWidgetManager->RemoveWidget(mMainMenu);
			SafeDeleteWidget(mMainMenu);
			mMainMenu = NULL;
			ShowBoard(aParts[0], anIndex);
			aState->mWait = 20;
			return;
		}
		Shot(this, aState, SHOT_MENU, "menu");
		Log(aState, "play");
		aState->mScreenTicks = 0;
		if (mAutoplayUI)
			ClickWidget(this, aState, (Widget*) mMainMenu->mPlayButton);
		else
			PlayGame();
		aState->mWait = 20;
		return;
	}
	if (mHelpScreen != NULL)
	{
		// the "how to play" screen a new player sees before every puzzle: leave it with its button
		if (++aState->mScreenTicks < 100)
			return;
		aState->mScreenTicks = 0;
		Log(aState, "help screen");
		if (mAutoplayUI)
			ClickWidget(this, aState, (Widget*) mHelpScreen->mOKButton);
		else
			mHelpScreen->ButtonDepress(HelpScreen::ID_OK);
		aState->mWait = 20;
		return;
	}
	if (mCompletion != NULL)
	{
		if (++aState->mScreenTicks < 100)
			return;
		aState->mScreenTicks = 0;
		Log(aState, "completion screen mode %d", mCompletion->mMode);
		if (mAutoplayUI)
			ClickWidget(this, aState, (Widget*) mCompletion->mContinueButton);
		else
			mCompletion->ButtonDepress(CompletionScreen::ID_CONTINUE);
		aState->mWait = 20;
		return;
	}
	if (mStoryScreen != NULL)
	{
		// every speech has come in by now: photograph, then press the button
		aState->mScreenTicks++;
		if (aState->mScreenTicks == 700 && !mAutoplayShots.empty())
		{
			char aName[32];
			snprintf(aName, sizeof(aName), "-story%d.png", aState->mSolved);
			AutoplayShot(mAutoplayShots + aName);
		}
		if (aState->mScreenTicks < 750)
			return;
		aState->mScreenTicks = 0;
		Log(aState, "story with %d speeches", (int) mStoryScreen->mSpeeches.size());
		if (mAutoplayUI)
			ClickWidget(this, aState, (Widget*) mStoryScreen->mContinueButton);
		else
			mStoryScreen->ButtonDepress(StoryScreen::ID_CONTINUE);
		aState->mWait = 20;
		return;
	}

	if (mPuzzleCompleted != NULL)
	{
		aState->mScreenTicks++;
		if (aState->mScreenTicks == 150 && !mAutoplayShots.empty() && (aState->mSolved == 0 || aState->mSolved == 8 || aState->mSolved == 9 || aState->mSolved == 10 || aState->mSolved == 11))
		{
			char aName[32];
			snprintf(aName, sizeof(aName), "-completed%d.png", aState->mSolved);
			AutoplayShot(mAutoplayShots + aName);
		}
		if (aState->mScreenTicks < 220)
			return;
		aState->mScreenTicks = 0;
		aState->mSolved++;
		Log(aState, "completed %s moves %d time %d record %d maxed %d opened %d temple %d tutorial %d", aState->mUuid.c_str(),
			mPuzzleCompleted->mMoves, mPuzzleCompleted->mTime, mPuzzleCompleted->mWorldRecord, mPuzzleCompleted->mWasMaxed,
			mPuzzleCompleted->mOpenedTemple, mPuzzleCompleted->mTempleDone, mPuzzleCompleted->mTutorialDone);
		if (mAutoplayUI)
			ClickWidget(this, aState, (Widget*) mPuzzleCompleted->mContinueButton);
		else
			mPuzzleCompleted->ButtonDepress(PuzzleCompletedScreen::ID_CONTINUE);
		aState->mMoveIndex = -1;
		aState->mWait = 20;
		return;
	}

	if (mBoard != NULL && !mAutoplayShow.empty())
	{
		if (++aState->mScreenTicks == 250)
		{
			AutoplayShot(mAutoplayShots + "-show.png");
			Shutdown();
		}
		return;
	}

	if (mBoard != NULL)
	{
		Board* aBoard = mBoard;
		if (aState->mMoveIndex < 0)
		{
			// the board has just come: read the best known solution
			aState->mUuid = aBoard->mUuid;
			aState->mSolution.clear();
			const ProgressManager::Best* aBest = mProgress->FindBest(aBoard->mUuid);
			if (aBest != NULL)
				for (size_t i = 0; i + 6 <= aBest->mSolution.size(); i += 6)
				{
					Grid::Move aMove;
					if (Grid::ParseMove(aBest->mSolution.substr(i, 6), &aMove))
						aState->mSolution.push_back(aMove);
				}
			Log(aState, "board %s %s, solution of %d moves", aBoard->mLevelName.c_str(), aBoard->mUuid.c_str(), (int) aState->mSolution.size());
			aState->mMoveIndex = 0;
			aState->mWait = 120;
			Shot(this, aState, SHOT_BOARD, "board");
			return;
		}
		if (aBoard->mGrid->mDone)
		{
			// three quarters through the sweep that shows the finished picture over the board
			const Grid* g = aBoard->mGrid;
			if (!g->mTileMoving && g->mReveal > 0 && g->mReveal <= g->mRows * g->mCols / 4)
				Shot(this, aState, SHOT_SWEEP, "sweep");
			return;
		}
		if (aState->mMoveIndex >= (int) aState->mSolution.size())
		{
			// out of moves and not done: the log says so and the run stops
			Log(aState, "solution ended without a finished puzzle (%d moves)", aBoard->mGrid->mMoveCount);
			Shutdown();
			return;
		}
		const Grid::Move& aMove = aState->mSolution[aState->mMoveIndex++];
		int aClickX, aClickY, aHoverX, aHoverY;
		if (mAutoplayUI && FindCell(aBoard, aMove, &aHoverX, &aHoverY, &aClickX, &aClickY))
		{
			if (aHoverX >= 0)
			{
				char anArgs[32];
				snprintf(anArgs, sizeof(anArgs), "%d %d", aHoverX, aHoverY);
				Script(this, aState, "move", anArgs);
			}
			ClickAt(this, aState, aClickX, aClickY);
		}
		else
		{
			if (mAutoplayUI)
				Log(aState, "no cell makes move %d (side %d cursor %d remove %d kind %d; the tile maker faces %d)", aState->mMoveIndex - 1, aMove.mSide, aMove.mCursor, aMove.mRemove, aMove.mKind, aBoard->mTileMaster->GetSide());
			aBoard->Aim(aMove.mSide, aMove.mCursor);
			if (aMove.mRemove)
				aBoard->DoPull();
			else
				aBoard->DoThrow();
		}
		if (!aBoard->mTutorial && aState->mMoveIndex == 3)
			Shot(this, aState, SHOT_PLAY, "play");
		if (aState->mMoveIndex == (int) aState->mSolution.size() - 1)
			Shot(this, aState, SHOT_BOARD_END, "board-end");
		aState->mWait = 4;
		return;
	}

	if (mTempleScreen != NULL)
	{
		if (++aState->mScreenTicks < 120)
			return;
		Shot(this, aState, SHOT_TEMPLE, "temple");
		aState->mScreenTicks = 0;
		if (aState->mSolved >= mAutoplayPuzzles)
		{
			Log(aState, "done: %d puzzles", aState->mSolved);
			Shutdown();
			return;
		}
		if (mAutoplaySkip && mTempleScreen->mTemple == 0 && mTempleScreen->mSkipButton->mVisible)
		{
			Log(aState, "skip the tutorial");
			ClickWidget(this, aState, (Widget*) mTempleScreen->mSkipButton);
			aState->mWait = 60;
			return;
		}
		int aPuzzle = FindPuzzle(this, aUser, mLevelData->mCurrentLevel);
		if (aPuzzle < 0)
		{
			Log(aState, "temple %s done", mLevelData->mCurrentLevel.c_str());
			if (mAutoplayUI)
				ClickWidget(this, aState, (Widget*) mTempleScreen->mBackButton);
			else
				BackToIsland();
			aState->mWait = 20;
			return;
		}
		aState->mMoveIndex = -1;
		if (mAutoplayUI)
		{
			// the thumbnail of the puzzle: the first pixel the screen's hit test gives that puzzle for
			int aX = -1, aY = -1;
			for (int y = 0; y < 480 && aX < 0; y += 4)
				for (int x = 0; x < 640; x += 4)
					if (mTempleScreen->HitTest(x, y) == aPuzzle)
					{
						aX = x;
						aY = y;
						break;
					}
			if (aX < 0)
			{
				Log(aState, "no thumbnail for puzzle %d", aPuzzle);
				Shutdown();
				return;
			}
			ClickAt(this, aState, aX, aY);
		}
		else
		{
			mLevelData->mCurrentPuzzle = aPuzzle;
			StartPuzzle(true);
		}
		aState->mWait = 20;
		return;
	}

	if (mIslandMap != NULL)
	{
		if (++aState->mScreenTicks < 150)
			return;
		Shot(this, aState, SHOT_ISLAND, "island");
		aState->mScreenTicks = 0;
		if (aState->mSolved >= mAutoplayPuzzles)
		{
			Log(aState, "done: %d puzzles", aState->mSolved);
			Shutdown();
			return;
		}
		int aTemple = (mAutoplaySkip && !aUser->mTutorialDone) ? 0 : FindTemple(this, aUser);
		if (aTemple < 0)
		{
			Log(aState, "done: no temple left");
			Shutdown();
			return;
		}
		Log(aState, "temple %s", gTemples[aTemple].mName);
		if (mAutoplayUI)
			ClickWidget(this, aState, (Widget*) mIslandMap->mHotspots[aTemple]);
		else
			EnterTemple(aTemple);
		aState->mWait = 20;
		return;
	}
}

// port: a helper of the port (Sexy::TestFindCell)
bool Sexy::TestFindCell(Board* theBoard, const Grid::Move& theMove, int* theHoverX, int* theHoverY, int* theX, int* theY)
{
	return FindCell(theBoard, theMove, theHoverX, theHoverY, theX, theY);
}
