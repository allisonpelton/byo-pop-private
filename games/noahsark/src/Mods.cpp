// port(mod): the code behind Noah's Ark's mods (ModFeatures.h; each is a folder in games/noahsark/mods with a
// mod.json that names the feature). Undo (Ctrl+Z or U), the move helper (H), the tide (the flood's pace) and the
// daily board (every level starts from the same animals for everyone on a day). The rules of the original are
// untouched while no mod is on: every hook starts with its feature's test.

#include "Board.h"
#include "NoahApp.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ModFeatures.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cstdio>
#include <ctime>

using namespace Sexy;

namespace
{

// days since 1970-01-01 of a calendar date (proleptic Gregorian), and the date of a day number
long DaysFromCivil(int y, int m, int d)
{
	y -= (m <= 2);
	long era = (y >= 0 ? y : y - 399) / 400;
	unsigned yoe = (unsigned) (y - era * 400);
	unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + (long) doe - 719468;
}

void CivilFromDays(long z, int& y, int& m, int& d)
{
	z += 719468;
	long era = (z >= 0 ? z : z - 146096) / 146097;
	unsigned doe = (unsigned) (z - era * 146097);
	unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	y = (int) yoe + (int) era * 400;
	unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	unsigned mp = (5 * doy + 2) / 153;
	d = (int) (doy - (153 * mp + 2) / 5 + 1);
	m = (int) mp + (mp < 10 ? 3 : -9);
	y += (m <= 2);
}

}

// A level is about to be made. Feature "undo": nothing to take back yet. "tide": the pace of the water for this level.
// "daily": the animals come from the C generator, so it is seeded from the day, the mode, the difficulty and the
// level (a game that is continued has its board already).
void Board::ModsNewLevel()
{
	mUndoStack.clear();
	mModNote.clear();
	mTidePercent = ModFeatureEnabled("tide") ? ModFeatureInt("tide", "speed", 100, 0, 400) : 100;

	if (mLoadedGame || !ModFeatureEnabled("daily"))
		return;
	long aDay = (long) (time(NULL) / 86400);
	std::string aDate = ModFeatureText("daily", "date", "");
	int y, m, d;
	if (!aDate.empty() && (sscanf(aDate.c_str(), "%d-%d-%d", &y, &m, &d) == 3) && (m >= 1) && (m <= 12) && (d >= 1) && (d <= 31))
		aDay = DaysFromCivil(y, m, d);
	CivilFromDays(aDay, y, m, d);
	unsigned int aSeed = (unsigned int) aDay * 2654435761u;
	aSeed ^= (unsigned int) (mApp->mGameMode * 1000003 + mApp->mDifficulty * 7919 + mApp->mLevel * 104729);
	CrtSRand(aSeed);
	mModNote = StrFormat("Daily board %04d-%02d-%02d", y, m, d);
}

// Nothing is walking, flying, fading or falling, and no pair is half linked: the board holds still
bool Board::ModBoardSettled()
{
	if (!mLink.empty())
		return false;
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* p = mGrid[x][y];
			if (p == NULL)
				continue;
			if (p->mMoving || p->mRemoved || p->mFading)
				return false;
			if ((p->mType < 0xd) && ((p->mOffsetX != 0) || (p->mOffsetY != 0)))
				return false;
		}
	}
	return true;
}

// Before a click: with the undo mod on and the board still, the board as it is
bool Board::ModUndoBegin(std::string& theSnapshot)
{
	if (!ModFeatureEnabled("undo") || (mState != BOARD_PLAYING) || (mApp->mNextLevelWidget != NULL) ||
		(mApp->mHighScoreWidget != NULL) || !ModBoardSettled())
		return false;
	theSnapshot = SaveState();
	return true;
}

// After the click: when it started a move (animals walk, or the state moved on), the snapshot is what undo goes back to
void Board::ModUndoEnd(const std::string& theSnapshot, int theStateBefore)
{
	if ((mState == theStateBefore) && ModBoardSettled())
		return;
	mUndoStack.push_back(theSnapshot);
	size_t aDepth = (size_t) ModFeatureInt("undo", "depth", 10, 1, 100);
	while (mUndoStack.size() > aDepth)
		mUndoStack.erase(mUndoStack.begin());
	SDL_Log("mod undo: move kept (%d to undo)", (int) mUndoStack.size());
}

// Takes the board back to before the last move. The clock does not go back with it (the flood stays where it is),
// unless the mod's setting says so.
bool Board::ModUndo()
{
	if (!ModFeatureEnabled("undo") || mUndoStack.empty() || (mState != BOARD_PLAYING) ||
		(mApp->mNextLevelWidget != NULL) || (mApp->mHighScoreWidget != NULL))
		return false;
	std::string aSnapshot = mUndoStack.back();
	mUndoStack.pop_back();

	int aWaterY = mWaterY, aWaterTargetY = mWaterTargetY, aUpdateCnt = mUpdateCnt, aStartTick = mStartTick;
	ClearLinkList();
	ClearLink();
	mSelected = NULL;
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* p = mGrid[x][y];
			for (size_t i = 0; i < p->mPath.size(); i++)
				delete p->mPath[i];
			p->mPath.clear();
			p->mPartner = NULL;
			p->mMoving = p->mRemoved = p->mFading = p->mFlash = p->mSelected = false;
		}
	}
	if (!LoadState(aSnapshot))
		return false;
	if (!ModFeatureBool("undo", "restoreWater", false))
	{
		mWaterY = aWaterY;
		mWaterTargetY = aWaterTargetY;
	}
	mUpdateCnt = aUpdateCnt;
	mStartTick = aStartTick;
	mLastInputTick = mLastMoveTick = mUpdateCnt;
	mApp->PlaySample(SOUND_DESELECT);
	SDL_Log("mod undo: back to %d points (%d to undo)", mApp->mGameScore, (int) mUndoStack.size());
	return true;
}

// The move helper: H flashes a pair that can be joined (or, in the puzzle, a swap that makes three); H again plays
// it, as two clicks
void Board::ModShowMove()
{
	if (!ModFeatureEnabled("helper") || (mState != BOARD_PLAYING) || (mApp->mNextLevelWidget != NULL) ||
		(mApp->mHighScoreWidget != NULL) || (mApp->mGameMode == GAMEMODE_STAMPEDE) || !ModBoardSettled())
		return;
	Piece* aFirst = NULL;
	Piece* aSecond = NULL;
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			if (mGrid[x][y]->mFlash)
			{
				if (aFirst == NULL)
					aFirst = mGrid[x][y];
				else if (aSecond == NULL)
					aSecond = mGrid[x][y];
			}
		}
	}
	if ((aFirst != NULL) && (aSecond != NULL) && ModFeatureBool("helper", "playSecond", true))
	{
		int aFirstX = aFirst->mX, aFirstY = aFirst->mY, aSecondX = aSecond->mX, aSecondY = aSecond->mY;
		SDL_Log("mod helper: plays the move");
		mSelected = NULL;
		MouseDown(CellLeft(aFirstX) + 25, CellTop(aFirstY) + 25, 1);
		MouseDown(CellLeft(aSecondX) + 25, CellTop(aSecondY) + 25, 1);
		return;
	}
	SDL_Log("mod helper: shows a move");
	bool aShowHints = mApp->mShowHints;
	mApp->mShowHints = true;
	ShowHint();
	mApp->mShowHints = aShowHints;
}

void Board::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_CONTROL)
		mCtrlDown = true;
	if (((theKey == (KeyCode) 'Z') && mCtrlDown) || (theKey == (KeyCode) 'U'))
		ModUndo();
	else if (theKey == (KeyCode) 'H')
		ModShowMove();
}

void Board::KeyUp(KeyCode theKey)
{
	if (theKey == KEYCODE_CONTROL)
		mCtrlDown = false;
}

// The daily board's date, small and white, under the ark
void Board::DrawModNote(Graphics* g)
{
	if (mModNote.empty())
		return;
	ImageFont* aFont = mApp->mFonts[FONT_GARAMOND_TINY];
	g->SetFont(aFont);
	int aX = (CellLeft(0) - 10 - aFont->StringWidth(mModNote)) / 2;
	g->SetColor(Color(0, 0, 0, 0xc0));
	g->DrawString(mModNote, aX + 1, 301);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(mModNote, aX, 300);
}
