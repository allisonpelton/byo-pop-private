// Test hooks: the autoplay bot (-autoplay=<seed>). It plays whole games by calling the field's mouse handlers, so
// everything the game does for a player runs: atoms are selected and sent along a path, molecules are made and
// scored, the meter fills, levels come and go, the game ends. The bot looks for a block of four atoms of one colour
// that lacks one, and sends it an atom of that colour from elsewhere; if there is none, it makes a random move.
// It has its own random numbers (the game's use the C library generator, seeded by -seed), so the same -seed and
// -autoplay always make the same game. It writes a log of what happened (-autoplaylog=<file>) that tests compare.
//
//   -autoplay=<seed>      play with the bot (its own seed)
//   -autoplaymode=strategy|action   the game type (default strategy)
//   -autoplaygames=N      play N games, then quit
//   -autoplaycap=N        quit after N updates in any case
//   -autoplaybonus=1      make the bonus molecule the sign asks for (a test of the bonus molecules and the elements)
//   -autoplayfinish=N     every N moves the level's meter is filled (a test of the level changes)
//   -autoplaylog=<file>   the log
//   -autoplayshots=<prefix>  screenshots at each level done and game over
//   -autoplayscript=<file>   every click the bot makes, as a scenario for games/atomica/tools/identical/compare.py
//                            ("<update> click <x> <y>", window coordinates), to replay the same game in the original

#include "AtomicaApp.h"
#include "Board.h"
#include "Grid.h"
#include "Dialogs.h"
#include "HintBox.h"
#include "BonusMolecules.h"
#include "Menu.h"
#include "PathFinder.h"
#include "Screens.h"
#include "Graphics.h"
#include "WidgetManager.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace Sexy;

void AtomicaApp::ScriptClick(int theX, int theY)
{
	MouseMoveTo(theX, theY);
	mWidgetManager->MouseDown(theX, theY, 1);
	mWidgetManager->MouseUp(theX, theY, 1);
}

namespace Sexy
{

namespace
{

// one click of the bot in the scenario file (-autoplayscript); a click is a move, a press and a release in one update
void Record(AtomicaApp* theApp, int theX, int theY)
{
	static FILE* sScript = NULL;
	static int sLast = -100;
	if (theApp->mAutoplayScript.empty())
		return;
	if (sScript == NULL)
		sScript = fopen(theApp->mAutoplayScript.c_str(), "w");
	if (sScript != NULL)
	{
		// the clicks of one action of the bot (select an atom, then its target) come in one update, where a player's
		// would not: the later ones are put three updates apart, so that they are not a double click
		int aUpdate = theApp->mUpdateCount;
		if (aUpdate < sLast + 3)
			aUpdate = sLast + 3;
		sLast = aUpdate;
		fprintf(sScript, "%d click %d %d\n", aUpdate, theX, theY);
		fflush(sScript);
	}
}

// a click of the bot goes through the widget manager as a player's does (a box that is up takes it), and is recorded
void Click(AtomicaApp* theApp, int theX, int theY)
{
	Record(theApp, theX, theY);
	theApp->ScriptClick(theX, theY);
}

void ClickButton(AtomicaApp* theApp, Widget* theParent, Widget* theButton)
{
	Click(theApp, theParent->mX + theButton->mX + theButton->mWidth / 2, theParent->mY + theButton->mY + theButton->mHeight / 2);
}

}

struct AutoplayState
{
	unsigned int	mSeed;
	int				mTick;
	int				mGames;				// games started
	int				mWait;				// updates to wait before the next action
	int				mShots;
	int				mMoves;				// moves made in this game
	int				mLastLevel;
	bool			mOverLogged;
	bool			mLevelLogged;
	bool			mMenuSeen;
	int				mBonusStage;		// 0 waiting for the sign, 1 shape made, 2 element atom seen, 3 effect set off
	FILE*			mLog;
	bool			mPending;			// a second click waits for the wait to end (the cell an atom is sent to, a box's button)
	int				mPendX;
	int				mPendY;

	AutoplayState(unsigned int theSeed) : mSeed(theSeed), mTick(0), mGames(0), mWait(0), mShots(0), mMoves(0),
		mLastLevel(0), mOverLogged(false), mLevelLogged(false), mMenuSeen(false), mBonusStage(0), mLog(NULL), mPending(false), mPendX(0), mPendY(0) {}

	int Rand(int theRange)
	{
		mSeed = mSeed * 1103515245u + 12345u;
		return (int) ((mSeed >> 16) % (unsigned int) theRange);
	}
};

AutoplayState* NewAutoplayState(unsigned int theSeed)
{
	return new AutoplayState(theSeed);
}

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

// the atom of a cell if it can be moved: placed, and not a pre-atom
Atom* Movable(Grid* g, int theCol, int theRow)
{
	if ((theCol < 0) || (theCol >= Grid::COLS) || (theRow < 0) || (theRow >= Grid::ROWS))
		return NULL;
	Atom* anAtom = g->mCells[theCol][theRow];
	if ((anAtom == NULL) || anAtom->mPre || !anAtom->mPlaced || anAtom->mFollowing || anAtom->mDead || (g->mCellState[theCol][theRow] < 0x96))
		return NULL;
	return anAtom;
}

bool Empty(Grid* g, int theCol, int theRow)
{
	if ((theCol < 0) || (theCol >= Grid::COLS) || (theRow < 0) || (theRow >= Grid::ROWS))
		return false;
	// a free cell, or one a pre-atom waits in (an atom may be sent there)
	Atom* anAtom = g->mCells[theCol][theRow];
	return (anAtom == NULL) || (anAtom->mPre && !anAtom->mPreStarted);
}

// all the atoms are still: nothing flies, follows a path or waits to be revealed
bool Quiet(Grid* g)
{
	g->mAtoms.Reset();
	Atom* anAtom = g->mAtoms.Next();
	while (anAtom != NULL)
	{
		if (!anAtom->mPlaced || anAtom->mFollowing || anAtom->mDead)
			return false;
		anAtom = g->mAtoms.Next();
	}
	return g->mGroups.Size() == 0;
}

// the click of a player: move, press, release (the grid's coordinates)
void Press(Grid* g, int theCol, int theRow)
{
	int aX = theCol * Grid::CELL + Grid::CELL / 2;
	int aY = theRow * Grid::CELL + Grid::CELL / 2;
	Click(g->mApp, g->mX + aX, g->mY + aY);
}

// a move: select the atom, then the empty cell (the path is shown by the mouse move)
bool Move(Grid* g, Atom* theAtom, int theToCol, int theToRow)
{
	if (!g->mPathFinder->Find(theAtom->mCol, theAtom->mRow, theToCol, theToRow))
		return false;
	g->mPathFinder->mPath.Clear();
	Press(g, theAtom->mCol, theAtom->mRow);
	if (g->mSelected != theAtom)
		return false;
	// the cell is clicked three updates later, as a player's would be (and as a scenario file replays it)
	AutoplayState* s = g->mApp->mAutoplayState;
	s->mPending = true;
	s->mPendX = g->mX + theToCol * Grid::CELL + Grid::CELL / 2;
	s->mPendY = g->mY + theToRow * Grid::CELL + Grid::CELL / 2;
	s->mWait = 3;
	return true;
}

}

void AtomicaApp::AutoplayUpdate()
{
	AutoplayState* s = mAutoplayState;
	s->mTick++;
	if ((s->mLog == NULL) && !mAutoplayLog.empty())
	{
		s->mLog = fopen(mAutoplayLog.c_str(), "w");
		Log(s, "autoplay seed %u mode %s games %d", s->mSeed, mAutoplayMode == 1 ? "action" : "strategy", mAutoplayGames);
	}
	if ((s->mTick % 2000 == 0) && (mBoard != NULL) && (mBoard->mGrid != NULL))
		Log(s, "update %d: score %d level %d moves %d atoms %d (f8c %g i16c %d b33c %d quiet %d sel %d pause %d)", s->mTick,
			mBoard->mScore, mBoard->mMeter.mLevel, s->mMoves, mBoard->mGrid->CountAtoms(false), mBoard->mF8c, mBoard->mI16c,
			(int) mBoard->mGrid->mB33c, (int) Quiet(mBoard->mGrid), mBoard->mGrid->mSelected != NULL, mPauseCount);
	if (s->mTick > mAutoplayCap)
	{
		Log(s, "cap reached at update %d: games %d", s->mTick, s->mGames);
		if (!mAutoplayShots.empty())
			SaveScreenshot(StrFormat("%s-cap.png", mAutoplayShots.c_str()));
		if (s->mLog != NULL)
			fclose(s->mLog);
		Shutdown();
		return;
	}
	if (mTitleScreen != NULL)
	{
		// recording a scenario: the title screen and the menu are left at the updates the original's script clicks them
		if (mLoaded && (s->mTick > (mAutoplayScript.empty() ? 60 : 1510)))
			TitleScreenDone();
		return;
	}
	if (s->mWait > 0)
	{
		s->mWait--;
		return;
	}
	if (s->mPending)
	{
		s->mPending = false;
		Click(this, s->mPendX, s->mPendY);
		s->mWait = 2;
		return;
	}

	if (mScoreDialog != NULL)
	{
		// the high score entry: wait until the box is in place, give a name, press the button
		HighScoreDialog* aDialog = (HighScoreDialog*) mScoreDialog;
		if (aDialog->mBoxY >= 0)
		{
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-entry%d.png", mAutoplayShots.c_str(), s->mGames));
			aDialog->mEdit->SetText("BOT");
			aDialog->ButtonDepress(2);
			s->mWait = 5;
		}
		return;
	}
	if (mMenu != NULL)
	{
		MainMenu* aMenu = (MainMenu*) mMenu;
		if (!aMenu->PanelShown() || (!mAutoplayScript.empty() && (mUpdateCount < 2310)))
			return;
		if (aMenu->mGameOver && !aMenu->mEntryShown)
			return;
		if (aMenu->mGameOver && !s->mMenuSeen)
		{
			s->mMenuSeen = true;
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-menuover%d.png", mAutoplayShots.c_str(), s->mGames));
			s->mWait = 3;
			return;
		}
		if (s->mGames >= mAutoplayGames)
		{
			Log(s, "done after %d games at update %d", s->mGames, s->mTick);
			if (s->mLog != NULL)
				fclose(s->mLog);
			Shutdown();
			return;
		}
		s->mGames++;
		s->mMenuSeen = false;
		s->mOverLogged = false;
		s->mLevelLogged = false;
		s->mMoves = 0;
		s->mLastLevel = 0;
		if (aMenu->mGameOver)
			Click(this, 0x14a + 0x67, 0x178 + 0x12);
		else
			Click(this, 0x5a + 0xe2, (mAutoplayMode == 1 ? 0xb1 : 0x4b) + 0x1d);
		Log(s, "game %d starts at update %d", s->mGames, s->mTick);
		s->mWait = 20;
		return;
	}
	if ((mBoard == NULL) || (mBoard->mGrid == NULL))
		return;

	Board* aBoard = mBoard;
	Grid* aGrid = mBoard->mGrid;
	if (aGrid->mLevelOver)
	{
		if (!s->mOverLogged)
		{
			s->mOverLogged = true;
			Log(s, "game %d over at update %d: score %d level %d moves %d", s->mGames, s->mTick, aBoard->mScore,
				aBoard->mMeter.mLevel, s->mMoves);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-over%d.png", mAutoplayShots.c_str(), s->mGames));
		}
		return;
	}
	if (!mHintBoxes.empty())
	{
		// a tutorial box: wait until it has faded in, look at it, tick the check box of every other one, press Continue
		HintBox* aBox = mHintBoxes[0];
		if (aBox->mAlpha >= 0xff)
		{
			if (!mAutoplayShots.empty() && (s->mShots < 40))
			{
				s->mShots++;
				SaveScreenshot(StrFormat("%s-hint%d.png", mAutoplayShots.c_str(), aBox->mHint));
			}
			Log(s, "hint %d at update %d", aBox->mHint, s->mTick);
			// the box's check mark, then (three updates later, as a player's would be) its button
			if ((aBox->mHint & 1) && aBox->mHasCheck)
			{
				ClickButton(this, aBox, aBox->mCheck);
				s->mPending = true;
				s->mPendX = aBox->mX + aBox->mContinue->mX + aBox->mContinue->mWidth / 2;
				s->mPendY = aBox->mY + aBox->mContinue->mY + aBox->mContinue->mHeight / 2;
				s->mWait = 3;
				return;
			}
			ClickButton(this, aBox, aBox->mContinue);
			s->mWait = 5;
		}
		return;
	}
	if (mLevelUpDialog != NULL)
	{
		// the level-up box: wait until it has slid down, look at it, press its button
		LevelUpDialog* aDialog = (LevelUpDialog*) mLevelUpDialog;
		if (aDialog->mBoxY >= 0)
		{
			if (!mAutoplayShots.empty() && (s->mShots < 3))
			{
				s->mShots++;
				SaveScreenshot(StrFormat("%s-levelup%d.png", mAutoplayShots.c_str(), s->mShots));
			}
			ClickButton(this, aDialog, aDialog->mContinue);
			s->mWait = 5;
		}
		return;
	}
	if (aBoard->mMeter.mFull)
	{
		if (!s->mLevelLogged)
		{
			s->mLevelLogged = true;
			Log(s, "level %d done at update %d: score %d moves %d", aBoard->mMeter.mLevel, s->mTick, aBoard->mScore, s->mMoves);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-level%d-%d.png", mAutoplayShots.c_str(), s->mGames, aBoard->mMeter.mLevel));
		}
		return;
	}
	s->mLevelLogged = false;
	if ((aBoard->mF8c > 0.0f) || (aBoard->mI16c > 4) || !aGrid->mB33c || !Quiet(aGrid) || (aGrid->mSelected != NULL) ||
		(aBoard->mBombOn) || (aGrid->mCombo > 0 && aGrid->mB358 == false && s->mTick % 4 != 0))
		return;
	if (mPauseCount > 0)
		return;

	if (mAutoplayBonus)
	{
		BonusMolecules* aBonus = aBoard->mBonus;
		if ((s->mBonusStage == 0) && (aBonus->mTarget < 100) && (aBonus->mRound < aBonus->mTarget) && (s->mMoves > 2))
			aBonus->mRound = aBonus->mTarget;
		if ((s->mBonusStage == 0) && (aBonus->mPattern != NULL) && (aBonus->mX >= 0))
		{
			// the sign is out: the shape it asks for is built in the middle of the field and looked for
			for (int aCol = 2; aCol < 7; aCol++)
				for (int aRow = 1; aRow < 6; aRow++)
				{
					Atom* anOld = aGrid->mCells[aCol][aRow];
					if (anOld != NULL)
					{
						anOld->RemoveFromCell();
						aGrid->mAtoms.Remove(anOld);
						delete anOld;
					}
				}
			for (int aCol = 0; aCol < 5; aCol++)
				for (int aRow = 0; aRow < 5; aRow++)
				{
					int aWanted = aBonus->mCurrent[aCol * 5 + aRow];
					if (aWanted != 0)
					{
						Atom* aNew = (aWanted - 1 < 0xc) ? new Atom(aGrid, aWanted - 1, true) : new ElementAtom(aGrid, aWanted - 1, true);
						aNew->PlaceAt(2 + aCol, 1 + aRow, true);
						aNew->mPre = false;
					}
				}
			aBonus->Check(aGrid);
			Log(s, "bonus shape made at update %d (element %d)", s->mTick, aBonus->mElement);
			s->mBonusStage = 1;
			s->mWait = 400;
			return;
		}
		if (s->mBonusStage == 1)
		{
			for (int aCol = 0; aCol < Grid::COLS; aCol++)
				for (int aRow = 0; aRow < Grid::ROWS; aRow++)
				{
					Atom* anAtom = aGrid->mCells[aCol][aRow];
					if ((anAtom != NULL) && anAtom->IsElement() && !anAtom->mPre)
					{
						Log(s, "element atom type %d at %d,%d at update %d", anAtom->mType, aCol, aRow, s->mTick);
						if (!mAutoplayShots.empty())
							SaveScreenshot(StrFormat("%s-element.png", mAutoplayShots.c_str()));
						// a click selects it, a double click sets it off
						Press(aGrid, aCol, aRow);
						int aX = aCol * Grid::CELL + Grid::CELL / 2;
						int aY = aRow * Grid::CELL + Grid::CELL / 2;
						aGrid->mIsOver = true;
						aGrid->MouseDown(aX, aY, 0, 1);
						aGrid->MouseDown(aX, aY, 0, 2);
						Log(s, "element set off at update %d", s->mTick);
						s->mBonusStage = 3;
						s->mWait = 100;
						return;
					}
				}
		}
		if ((s->mBonusStage == 3) && !mAutoplayShots.empty() && (s->mShots < 100))
		{
			s->mShots = 100;
			SaveScreenshot(StrFormat("%s-effect.png", mAutoplayShots.c_str()));
		}
	}

	// the best move: a block of four that lacks one atom of its colour
	for (int aCol = 0; aCol < Grid::COLS - 1; aCol++)
	{
		for (int aRow = 0; aRow < Grid::ROWS - 1; aRow++)
		{
			for (int aType = 0; aType < aGrid->mNumColors; aType++)
			{
				int aHave = 0;
				int aGapCol = -1;
				int aGapRow = -1;
				int aGaps = 0;
				for (int aDC = 0; aDC < 2; aDC++)
					for (int aDR = 0; aDR < 2; aDR++)
					{
						Atom* anAtom = Movable(aGrid, aCol + aDC, aRow + aDR);
						if ((anAtom != NULL) && ((anAtom->mType == aType) || (anAtom->mKind == KIND_CHROMIUM)))
							aHave++;
						else if (Empty(aGrid, aCol + aDC, aRow + aDR))
						{
							aGaps++;
							aGapCol = aCol + aDC;
							aGapRow = aRow + aDR;
						}
					}
				if ((aHave != 3) || (aGaps != 1))
					continue;
				for (int aC = 0; aC < Grid::COLS; aC++)
					for (int aR = 0; aR < Grid::ROWS; aR++)
					{
						if ((aC >= aCol) && (aC <= aCol + 1) && (aR >= aRow) && (aR <= aRow + 1))
							continue;
						Atom* aSource = Movable(aGrid, aC, aR);
						if ((aSource != NULL) && (aSource->mType == aType) && (aSource->mKind == KIND_PLAIN))
						{
							if (Move(aGrid, aSource, aGapCol, aGapRow))
							{
								s->mMoves++;
								if ((mAutoplayFinish > 0) && (s->mMoves % mAutoplayFinish == 0))
									aBoard->mMeter.Add(1000, 0);
								return;
							}
						}
					}
			}
		}
	}

	// a random move: an atom to an empty cell it can reach
	std::vector<Atom*> aAtoms;
	for (int aCol = 0; aCol < Grid::COLS; aCol++)
		for (int aRow = 0; aRow < Grid::ROWS; aRow++)
		{
			Atom* anAtom = Movable(aGrid, aCol, aRow);
			if (anAtom != NULL)
				aAtoms.push_back(anAtom);
		}
	for (int aTry = 0; (aTry < 30) && !aAtoms.empty(); aTry++)
	{
		Atom* anAtom = aAtoms[s->Rand((int) aAtoms.size())];
		int aToCol = s->Rand(Grid::COLS);
		int aToRow = s->Rand(Grid::ROWS);
		if (!Empty(aGrid, aToCol, aToRow))
			continue;
		if (Move(aGrid, anAtom, aToCol, aToRow))
		{
			s->mMoves++;
			if ((mAutoplayFinish > 0) && (s->mMoves % mAutoplayFinish == 0))
				aBoard->mMeter.Add(1000, 0);
			return;
		}
	}
}
