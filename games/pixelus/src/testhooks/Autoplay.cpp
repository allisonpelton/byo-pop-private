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
		for (int r = 0; r < g->mRows; r++)
			for (int c = 0; c < g->mCols; c++)
			{
				if (g->HasObject(r, c) != theMove.mRemove)
					continue;
				int aCursor = 0;
				if (ClickSide(g, r, c, aPreferred, &aCursor) != theMove.mSide || aCursor != theMove.mCursor)
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
				*theX = Board::GRID_X + c * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
				*theY = Board::GRID_Y + r * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
				return true;
			}
	}
	return false;
}

}

StateLog::StateLog(PixelusApp* theApp)
{
	mApp = theApp;
	mFile = fopen(theApp->mStateLogFileName.c_str(), "w");
	for (int i = 0; i < NUM_FIELDS; i++)
		mLast[i] = -999999;
}

StateLog::~StateLog()
{
	if (mFile != NULL)
		fclose(mFile);
}

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
void PixelusApp::AutoplayUpdate()
{
	AutoplayState* s = mAutoplayState;
	if (s == NULL)
		return;
	if (s->mLog == NULL && !mAutoplayLog.empty())
		s->mLog = fopen(mAutoplayLog.c_str(), "w");
	if (s->mScript == NULL && !mAutoplayScript.empty())
		s->mScript = fopen(mAutoplayScript.c_str(), "w");
	s->mTick++;
	if (s->mTick >= mAutoplayCap)
	{
		Log(s, "cap reached at update %d", s->mTick);
		Shutdown();
		return;
	}
	if (s->mWait > 0)
	{
		s->mWait--;
		return;
	}
	// the loading screen: wait until the bar is full, then press its play link
	if (mLoadingScreen != NULL)
	{
		if (mLoadingScreen->mLoadDone && ++s->mScreenTicks > 150)
		{
			s->mScreenTicks = 0;
			if (mAutoplayUI)
				ClickWidget(this, s, (Widget*) mLoadingScreen->mPlayLink);
			else
				PlayClicked();
			s->mWait = 20;
		}
		return;
	}

	// the dialogs: the one for a new player gets a name, the others their first button
	if (GetDialog(0x13) != NULL)
	{
		if (s->mNewUserSeen == 0)
		{
			NewUserDialog* aDialog = (NewUserDialog*) GetDialog(0x13);
			Log(s, "new player dialog %d,%d %dx%d edit %d,%d %dx%d box %d,%d %dx%d yes %d,%d %dx%d no %d,%d %dx%d", aDialog->mX, aDialog->mY,
				aDialog->mWidth, aDialog->mHeight, aDialog->mEdit->mX, aDialog->mEdit->mY, aDialog->mEdit->mWidth, aDialog->mEdit->mHeight,
				aDialog->mExpertBox->mX, aDialog->mExpertBox->mY, aDialog->mExpertBox->mWidth, aDialog->mExpertBox->mHeight,
				aDialog->mYesButton->mX, aDialog->mYesButton->mY, aDialog->mYesButton->mWidth, aDialog->mYesButton->mHeight,
				aDialog->mNoButton->mX, aDialog->mNoButton->mY, aDialog->mNoButton->mWidth, aDialog->mNoButton->mHeight);
			s->mNewUserSeen = 1;
		}
		if (mAutoplayUI)
		{
			Script(this, s, "char", "B");
			Script(this, s, "char", "o");
			Script(this, s, "char", "t");
			ClickWidget(this, s, ((NewUserDialog*) GetDialog(0x13))->mYesButton);
		}
		else
		{
			((NewUserDialog*) GetDialog(0x13))->SetName("Bot");
			ButtonDepress(2019);
		}
		s->mWait = 10;
		return;
	}
	if (GetDialogCount() > 0)
	{
		for (int anId = 0; anId < 64; anId++)
			if (GetDialog(anId) != NULL)
			{
				if (mAutoplayUI)
					ClickWidget(this, s, GetDialog(anId)->mYesButton);
				else
					ButtonDepress(2000 + anId);
				break;
			}
		s->mWait = 10;
		return;
	}

	UserProfile* aUser = mUserDirectory != NULL ? mUserDirectory->GetCurrent() : NULL;
	if (mMainMenu != NULL)
	{
		if (aUser == NULL)
			return;
		if (++s->mScreenTicks < 150)
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
				Log(s, "-autoplayshow: no such puzzle");
				Shutdown();
				return;
			}
			if (aParts.size() >= 4)
				aUser->SetRecord(aLevel->mPuzzles[anIndex].mUuid, 1, atoi(aParts[2].c_str()), atoi(aParts[3].c_str()));
			Log(s, "show %s %d", aParts[0].c_str(), anIndex);
			s->mScreenTicks = 0;
			mWidgetManager->RemoveWidget(mMainMenu);
			SafeDeleteWidget(mMainMenu);
			mMainMenu = NULL;
			ShowBoard(aParts[0], anIndex);
			s->mWait = 20;
			return;
		}
		Shot(this, s, SHOT_MENU, "menu");
		Log(s, "play");
		s->mScreenTicks = 0;
		if (mAutoplayUI)
			ClickWidget(this, s, (Widget*) mMainMenu->mPlayButton);
		else
			PlayGame();
		s->mWait = 20;
		return;
	}
	if (mHelpScreen != NULL)
	{
		// the "how to play" screen a new player sees before every puzzle: leave it with its button
		if (++s->mScreenTicks < 100)
			return;
		s->mScreenTicks = 0;
		Log(s, "help screen");
		if (mAutoplayUI)
			ClickWidget(this, s, (Widget*) mHelpScreen->mOKButton);
		else
			mHelpScreen->ButtonDepress(HelpScreen::ID_OK);
		s->mWait = 20;
		return;
	}
	if (mCompletion != NULL)
	{
		if (++s->mScreenTicks < 100)
			return;
		s->mScreenTicks = 0;
		Log(s, "completion screen mode %d", mCompletion->mMode);
		if (mAutoplayUI)
			ClickWidget(this, s, (Widget*) mCompletion->mContinueButton);
		else
			mCompletion->ButtonDepress(CompletionScreen::ID_CONTINUE);
		s->mWait = 20;
		return;
	}
	if (mStoryScreen != NULL)
	{
		// every speech has come in by now: photograph, then press the button
		s->mScreenTicks++;
		if (s->mScreenTicks == 700 && !mAutoplayShots.empty())
		{
			char aName[32];
			snprintf(aName, sizeof(aName), "-story%d.png", s->mSolved);
			AutoplayShot(mAutoplayShots + aName);
		}
		if (s->mScreenTicks < 750)
			return;
		s->mScreenTicks = 0;
		Log(s, "story with %d speeches", (int) mStoryScreen->mSpeeches.size());
		if (mAutoplayUI)
			ClickWidget(this, s, (Widget*) mStoryScreen->mContinueButton);
		else
			mStoryScreen->ButtonDepress(StoryScreen::ID_CONTINUE);
		s->mWait = 20;
		return;
	}

	if (mPuzzleCompleted != NULL)
	{
		s->mScreenTicks++;
		if (s->mScreenTicks == 150 && !mAutoplayShots.empty() && (s->mSolved == 0 || s->mSolved == 8 || s->mSolved == 9 || s->mSolved == 10 || s->mSolved == 11))
		{
			char aName[32];
			snprintf(aName, sizeof(aName), "-completed%d.png", s->mSolved);
			AutoplayShot(mAutoplayShots + aName);
		}
		if (s->mScreenTicks < 220)
			return;
		s->mScreenTicks = 0;
		s->mSolved++;
		Log(s, "completed %s moves %d time %d record %d maxed %d opened %d temple %d tutorial %d", s->mUuid.c_str(),
			mPuzzleCompleted->mMoves, mPuzzleCompleted->mTime, mPuzzleCompleted->mWorldRecord, mPuzzleCompleted->mWasMaxed,
			mPuzzleCompleted->mOpenedTemple, mPuzzleCompleted->mTempleDone, mPuzzleCompleted->mTutorialDone);
		if (mAutoplayUI)
			ClickWidget(this, s, (Widget*) mPuzzleCompleted->mContinueButton);
		else
			mPuzzleCompleted->ButtonDepress(PuzzleCompletedScreen::ID_CONTINUE);
		s->mMoveIndex = -1;
		s->mWait = 20;
		return;
	}

	if (mBoard != NULL && !mAutoplayShow.empty())
	{
		if (++s->mScreenTicks == 250)
		{
			AutoplayShot(mAutoplayShots + "-show.png");
			Shutdown();
		}
		return;
	}

	if (mBoard != NULL)
	{
		Board* b = mBoard;
		if (s->mMoveIndex < 0)
		{
			// the board has just come: read the best known solution
			s->mUuid = b->mUuid;
			s->mSolution.clear();
			const ProgressManager::Best* aBest = mProgress->FindBest(b->mUuid);
			if (aBest != NULL)
				for (size_t i = 0; i + 6 <= aBest->mSolution.size(); i += 6)
				{
					Grid::Move aMove;
					if (Grid::ParseMove(aBest->mSolution.substr(i, 6), &aMove))
						s->mSolution.push_back(aMove);
				}
			Log(s, "board %s %s, solution of %d moves", b->mLevelName.c_str(), b->mUuid.c_str(), (int) s->mSolution.size());
			s->mMoveIndex = 0;
			s->mWait = 120;
			Shot(this, s, SHOT_BOARD, "board");
			return;
		}
		if (b->mGrid->mDone)
		{
			// three quarters through the sweep that shows the finished picture over the board
			const Grid* g = b->mGrid;
			if (!g->mTileMoving && g->mReveal > 0 && g->mReveal <= g->mRows * g->mCols / 4)
				Shot(this, s, SHOT_SWEEP, "sweep");
			return;
		}
		if (s->mMoveIndex >= (int) s->mSolution.size())
		{
			// out of moves and not done: the log says so and the run stops
			Log(s, "solution ended without a finished puzzle (%d moves)", b->mGrid->mMoveCount);
			Shutdown();
			return;
		}
		const Grid::Move& aMove = s->mSolution[s->mMoveIndex++];
		int aClickX, aClickY, aHoverX, aHoverY;
		if (mAutoplayUI && FindCell(b, aMove, &aHoverX, &aHoverY, &aClickX, &aClickY))
		{
			if (aHoverX >= 0)
			{
				char anArgs[32];
				snprintf(anArgs, sizeof(anArgs), "%d %d", aHoverX, aHoverY);
				Script(this, s, "move", anArgs);
			}
			ClickAt(this, s, aClickX, aClickY);
		}
		else
		{
			if (mAutoplayUI)
				Log(s, "no cell makes move %d (side %d cursor %d remove %d kind %d; the tile maker faces %d)", s->mMoveIndex - 1, aMove.mSide, aMove.mCursor, aMove.mRemove, aMove.mKind, b->mTileMaster->GetSide());
			b->Aim(aMove.mSide, aMove.mCursor);
			if (aMove.mRemove)
				b->DoPull();
			else
				b->DoThrow();
		}
		if (!b->mTutorial && s->mMoveIndex == 3)
			Shot(this, s, SHOT_PLAY, "play");
		if (s->mMoveIndex == (int) s->mSolution.size() - 1)
			Shot(this, s, SHOT_BOARD_END, "board-end");
		s->mWait = 4;
		return;
	}

	if (mTempleScreen != NULL)
	{
		if (++s->mScreenTicks < 120)
			return;
		Shot(this, s, SHOT_TEMPLE, "temple");
		s->mScreenTicks = 0;
		if (s->mSolved >= mAutoplayPuzzles)
		{
			Log(s, "done: %d puzzles", s->mSolved);
			Shutdown();
			return;
		}
		if (mAutoplaySkip && mTempleScreen->mTemple == 0 && mTempleScreen->mSkipButton->mVisible)
		{
			Log(s, "skip the tutorial");
			ClickWidget(this, s, (Widget*) mTempleScreen->mSkipButton);
			s->mWait = 60;
			return;
		}
		int aPuzzle = FindPuzzle(this, aUser, mLevelData->mCurrentLevel);
		if (aPuzzle < 0)
		{
			Log(s, "temple %s done", mLevelData->mCurrentLevel.c_str());
			if (mAutoplayUI)
				ClickWidget(this, s, (Widget*) mTempleScreen->mBackButton);
			else
				BackToIsland();
			s->mWait = 20;
			return;
		}
		s->mMoveIndex = -1;
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
				Log(s, "no thumbnail for puzzle %d", aPuzzle);
				Shutdown();
				return;
			}
			ClickAt(this, s, aX, aY);
		}
		else
		{
			mLevelData->mCurrentPuzzle = aPuzzle;
			StartPuzzle(true);
		}
		s->mWait = 20;
		return;
	}

	if (mIslandMap != NULL)
	{
		if (++s->mScreenTicks < 150)
			return;
		Shot(this, s, SHOT_ISLAND, "island");
		s->mScreenTicks = 0;
		if (s->mSolved >= mAutoplayPuzzles)
		{
			Log(s, "done: %d puzzles", s->mSolved);
			Shutdown();
			return;
		}
		int aTemple = (mAutoplaySkip && !aUser->mFlag79) ? 0 : FindTemple(this, aUser);
		if (aTemple < 0)
		{
			Log(s, "done: no temple left");
			Shutdown();
			return;
		}
		Log(s, "temple %s", gTemples[aTemple].mName);
		if (mAutoplayUI)
			ClickWidget(this, s, (Widget*) mIslandMap->mHotspots[aTemple]);
		else
			EnterTemple(aTemple);
		s->mWait = 20;
		return;
	}
}
