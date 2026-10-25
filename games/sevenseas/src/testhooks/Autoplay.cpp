// Test hooks: the autoplay bot (-autoplay=<seed>). It plays whole games through the board's mouse handler and the
// app's buttons, so everything the game does for a player runs: ships move and fire, levels and the bonus screens
// come and go, the game ends and the score goes to the high-score list. The bot has its own random numbers (the
// game's use the C library generator, seeded by -seed), so the same -seed and -autoplay always make the same
// game. It writes a log of what happened (-autoplaylog=<file>) that tests compare.
//
//   -autoplay=<seed>      play with the bot (its own seed)
//   -autoplaydiff=1|2|3   the difficulty (default: the bot's dice)
//   -autoplaylevel=N      the first game starts at level N (tests of the bonus screen, the map, the new ships)
//   -autoplaygames=N      play N games, then quit
//   -autoplaycap=N        quit after N updates in any case
//   -autoplaylog=<file>   the log
//   -autoplayshots=<prefix>  screenshots at each game over

#include "SevenSeasApp.h"
#include "Board.h"
#include "Screens.h"
#include "Dialogs.h"
#include "Graphics.h"
#include "WidgetManager.h"

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
	unsigned int	mSeed;
	int				mTick;
	int				mGames;             // games started
	int				mWait;              // updates to wait before the next action
	bool			mGameOverLogged;
	bool			mGameOverShot;      // the screenshot of the finished high-score screen was taken
	unsigned int	mDialogShots;       // bit per dialog id: its first screenshot was taken
	int				mOptionsStage;      // with screenshots on: 0 not yet, 1 the options dialog is up, 2 its high scores asked for
	int				mLastLevel;
	int				mMoves;             // moves made in this game
	FILE*			mLog;

	AutoplayState() : mSeed(1), mTick(0), mGames(0), mWait(0), mGameOverLogged(false), mGameOverShot(false), mDialogShots(0), mOptionsStage(0), mLastLevel(0), mMoves(0),
		mLog(NULL) {}

	int Rand(int theRange)
	{
		mSeed = mSeed * 1103515245u + 12345u;
		return (int) ((mSeed >> 16) % (unsigned int) theRange);
	}
};

}

void SevenSeasApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplayState = new AutoplayState();
		mAutoplayState->mSeed = (unsigned int) atoi(theParamValue.c_str());
		mScriptedInput = true;   // the bot's input only, as in a -script run
	}
	else if (theParamName == "-autoplaylevel")
		mAutoplayLevel = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaydiff")
		mAutoplayDiff = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaygames")
		mAutoplayGames = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLog = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShots = theParamValue;
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
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

// a click on a cell of the board (the cell's middle, in the board's own coordinates)
void ClickCell(Board* b, int x, int y)
{
	b->MouseDown(Board::CellToPixel(x) + 21, Board::CellToPixel(y) + 21 + 12, 1);
}

}

// the bot, once per update
void SevenSeasApp::AutoplayUpdate()
{
	AutoplayState* s = mAutoplayState;
	if (s == NULL)
		return;
	if (s->mLog == NULL && !mAutoplayLog.empty())
		s->mLog = fopen(mAutoplayLog.c_str(), "w");
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

	// the title: wait for the load, then click
	if (mTitleScreen != NULL)
	{
		if (mTitleScreen->mPlayShown)
		{
			mTitleScreen->ButtonDepress(1);
			s->mWait = 20;
		}
		return;
	}
	// the dialogs: each is answered at once (the first of each kind is photographed: their sizes are checked by eye)
	for (int anId = 0; anId < 32; anId++)
	{
		if ((GetDialog(anId) != NULL) && !(s->mDialogShots & (1u << anId)) && !mAutoplayShots.empty())
		{
			s->mDialogShots |= 1u << anId;
			SaveScreenshot(StrFormat("%s-dialog%d.png", mAutoplayShots.c_str(), anId));
		}
	}
	if (GetDialog(DIALOG_NEXT_LEVEL) != NULL)
	{
		Log(s, "level %d done, score %d", mBoard->mLevel, mBoard->mScore);
		ButtonDepress(2000 + DIALOG_NEXT_LEVEL);
		s->mWait = 5;
		return;
	}
	static const int kAnswers[] = {DIALOG_RECORD_SCORE, DIALOG_BONUS_NAG, DIALOG_WARNING, DIALOG_HIGH_SCORES};
	for (size_t i = 0; i < sizeof(kAnswers) / sizeof(kAnswers[0]); i++)
	{
		if (GetDialog(kAnswers[i]) != NULL)
		{
			ButtonDepress(2000 + kAnswers[i]);
			s->mWait = 5;
			return;
		}
	}
	if (GetDialog(DIALOG_OPTIONS) != NULL)
	{
		// the options dialog (photographed above), then its high scores (answered above), then it is closed
		if (s->mOptionsStage == 1)
		{
			s->mOptionsStage = 2;
			((OptionsDialog*) GetDialog(DIALOG_OPTIONS))->ButtonDepress(2);
		}
		else
			ButtonDepress(2000 + DIALOG_OPTIONS);
		s->mWait = 5;
		return;
	}
	if (GetDialog(DIALOG_CONTINUE) != NULL)
	{
		ButtonDepress(3000 + DIALOG_CONTINUE);
		s->mWait = 5;
		return;
	}
	if (GetDialogCount() > 0)
		return;
	if (mMapWidget != NULL)
	{
		((MapWidget*) mMapWidget)->ButtonDepress(100);
		s->mWait = 5;
		return;
	}

	// the menu: beginner or normal, by the bot's own dice, then play
	if (mIntroWidget != NULL)
	{
		if (!mIntroWidget->mStarting)
		{
			if (!mAutoplayShots.empty() && (s->mOptionsStage == 0))
			{
				s->mOptionsStage = 1;
				ShowOptions();
				s->mWait = 3;
				return;
			}
			mIntroWidget->SetDifficulty((mAutoplayDiff >= 1) && (mAutoplayDiff <= 3) ? mAutoplayDiff : 1 + s->Rand(3));
			mIntroWidget->ButtonDepress(0);
			s->mGames++;
			s->mGameOverLogged = false;
			s->mGameOverShot = false;
			s->mLastLevel = 0;
			s->mMoves = 0;
			Log(s, "game %d started, difficulty %d", s->mGames, mIntroWidget->mDifficulty);
		}
		return;
	}

	// the game is over: the list of scores
	if (mHighScoreWidget != NULL)
	{
		if (!s->mGameOverLogged)
		{
			s->mGameOverLogged = true;
			Log(s, "game %d over at update %d: score %d level %d moves %d", s->mGames, s->mTick, mBoard != NULL ? mBoard->mScore : 0,
				mBoard != NULL ? mBoard->mLevel : 0, s->mMoves);
			s->mWait = 10;
			return;
		}
		// the screen is shown from the top down; the name dialog (if the score earned a place) comes at the end
		if (mHighScoreWidget->mReveal < mHighScoreWidget->mHeight)
			return;
		if (!s->mGameOverShot)
		{
			s->mGameOverShot = true;
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-over%d.png", mAutoplayShots.c_str(), s->mGames));
			s->mWait = 10;
			return;
		}
		if (s->mGames >= mAutoplayGames)
		{
			Shutdown();
			return;
		}
		mHighScoreWidget->ButtonDepress(1);
		s->mWait = 10;
		return;
	}

	Board* b = mBoard;
	if (b == NULL)
		return;
	if ((mAutoplayLevel > 1) && (b->mLevel == 1) && (s->mLastLevel == 0))
	{
		// a test of the later levels: the game starts at the given one
		b->mLevel = mAutoplayLevel - 1;
		b->NewLevel();
	}
	if (b->mLevel != s->mLastLevel)
	{
		s->mLastLevel = b->mLevel;
		Log(s, "level %d at update %d, lives %d, score %d", b->mLevel, s->mTick, b->mLives, b->mScore);
	}
	if (b->mState == STATE_LEVEL_END)
		return;
	if (b->mBonus)
	{
		// the bonus screen: dig in a random square (a click after the result continues)
		if (b->mBonusState != 0)
		{
			b->MouseDown(Board::CellToPixel(5), Board::CellToPixel(5) + 12, 1);
			s->mWait = 10;
		}
		else
			b->MouseDown(Board::CellToPixel(1 + s->Rand(9)) + 5 + 21, Board::CellToPixel(1 + s->Rand(9)) + 0x12 + 21, 1);
		return;
	}
	if (((b->mState != STATE_IDLE) && (b->mState != STATE_BLINK)) || b->mLevelOver)
		return;
	// the hint boxes of the beginner's game do not hold the bot up: it plays anyway

	// the ships that could be shot from here: any enemy on the two broadside lines (within three cells)
	int aTargets = 0;
	for (std::list<Ship*>::iterator anItr = b->mShips.begin(); anItr != b->mShips.end(); ++anItr)
	{
		if (b->CanFireAt(b->mPlayerX, b->mPlayerY, b->mHeading, (*anItr)->mX, (*anItr)->mY, 3))
			aTargets++;
	}
	if ((aTargets > 0) && (s->Rand(4) != 0))
	{
		ClickCell(b, b->mPlayerX, b->mPlayerY);
		s->mMoves++;
		s->mWait = 4;
		return;
	}
	// a move to a free cell next to the ship: not within reach of an enemy (which sails one cell toward the ship),
	// and from where a broadside would hit one; else the cell farthest from them
	struct Cand { int mX, mY, mSafe, mTargets; };
	std::vector<Cand> aCands;
	for (int dx = -1; dx <= 1; dx++)
		for (int dy = -1; dy <= 1; dy++)
		{
			int nx = b->mPlayerX + dx;
			int ny = b->mPlayerY + dy;
			if (((dx == 0) && (dy == 0)) || (nx < 0) || (nx > 10) || (ny < 0) || (ny > 10) || !b->CellFree(nx, ny))
				continue;
			Cand c;
			c.mX = nx;
			c.mY = ny;
			c.mSafe = 1000;
			for (std::list<Ship*>::iterator anItr = b->mShips.begin(); anItr != b->mShips.end(); ++anItr)
				c.mSafe = std::min(c.mSafe, std::max(abs((*anItr)->mX - nx), abs((*anItr)->mY - ny)));
			// the heading after the move: 0 up, 4 right, 8 down, 0xc left, the diagonals between
			static const int kHeading[3][3] = {{0xe, 0xc, 0xa}, {0, 0, 8}, {2, 4, 6}};
			int aHeading = kHeading[dx + 1][dy + 1];
			c.mTargets = 0;
			for (std::list<Ship*>::iterator anItr = b->mShips.begin(); anItr != b->mShips.end(); ++anItr)
				if (b->CanFireAt(nx, ny, aHeading, (*anItr)->mX, (*anItr)->mY, 3))
					c.mTargets++;
			aCands.push_back(c);
		}
	if (aCands.empty())
	{
		ClickCell(b, b->mPlayerX, b->mPlayerY);
		s->mWait = 4;
		return;
	}
	int aBestScore = -1000000;
	std::vector<int> aBestIdx;
	for (size_t i = 0; i < aCands.size(); i++)
	{
		int aScore = (aCands[i].mSafe >= 3 ? 100 : aCands[i].mSafe * 20) + aCands[i].mTargets * 30;
		if (aScore > aBestScore)
		{
			aBestScore = aScore;
			aBestIdx.clear();
		}
		if (aScore == aBestScore)
			aBestIdx.push_back((int) i);
	}
	const Cand& aPick = aCands[aBestIdx[s->Rand((int) aBestIdx.size())]];
	ClickCell(b, aPick.mX, aPick.mY);
	s->mMoves++;
	s->mWait = 4;
}
