// port: test hook, the autoplay bot (-autoplay=<seed>). It plays whole games by calling the field's mouse handlers, so
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
#include "Fixes.h"
#include "Board.h"
#include "Grid.h"
#include "Dialogs.h"
#include "HintBox.h"
#include "BonusMolecules.h"
#include "Menu.h"
#include "PathFinder.h"
#include "Screens.h"
#include "Graphics.h"
#include "DDInterface.h"
#include <stb_image_write.h>
#include <vector>
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

// (HandleScriptCommand, with "shot <file>" and the standard test commands, is in Commands.cpp)

// "shot <file>": the next frame that is drawn goes to a PNG as it is (drawing it again would take more random numbers and step
// the animations that move in the draw). The script line stands at the update the frame follows.
bool AtomicaApp::DrawDirtyStuff()
{
	// fixes mod, "drawRandom": the pictures draw their random numbers from a generator of their own (Support.h, GameRand)
	bool aSideRand = FixEnabled("drawRandom");
	if (aSideRand)
		mRand.BeginDrawing();
	bool aDrawn = SexyApp::DrawDirtyStuff();
	if (aSideRand)
		mRand.EndDrawing();
	if (aDrawn && !mPendingShot.empty())
	{
		std::vector<ulong> aBits(mWidth * mHeight);
		if (mDDInterface->ReadScreen(&aBits[0]))
		{
			std::vector<uchar> aRGBA(mWidth * mHeight * 4);
			for (int i = 0; i < mWidth * mHeight; i++)
			{
				aRGBA[i * 4 + 0] = (uchar) (aBits[i] >> 16);
				aRGBA[i * 4 + 1] = (uchar) (aBits[i] >> 8);
				aRGBA[i * 4 + 2] = (uchar) aBits[i];
				aRGBA[i * 4 + 3] = 255;
			}
			stbi_write_png(mPendingShot.c_str(), mWidth, mHeight, 4, &aRGBA[0], mWidth * 4);
		}
		mPendingShot.clear();
	}
	return aDrawn;
}

namespace Sexy
{

namespace
{

// one click of the bot in the scenario file (-autoplayscript); a click is a move, a press and a release in one update
void RecordLine(AtomicaApp* theApp, const char* theLine, int theX, int theY)
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
		if (theLine != NULL)
			fprintf(sScript, "%d %s\n", aUpdate, theLine);
		else
			fprintf(sScript, "%d click %d %d\n", aUpdate, theX, theY);
		fflush(sScript);
	}
}

void Record(AtomicaApp* theApp, int theX, int theY)
{
	RecordLine(theApp, NULL, theX, theY);
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

void Log(AutoplayState* theState, const char* theFormat, ...)
{
	if (theState->mLog == NULL)
		return;
	va_list args;
	va_start(args, theFormat);
	vfprintf(theState->mLog, theFormat, args);
	va_end(args);
	fputc('\n', theState->mLog);
	fflush(theState->mLog);
}

// the atom of a cell if it can be moved: placed, and not a pre-atom
Atom* Movable(Grid* theGrid, int theCol, int theRow)
{
	if ((theCol < 0) || (theCol >= Grid::COLS) || (theRow < 0) || (theRow >= Grid::ROWS))
		return NULL;
	Atom* anAtom = theGrid->mCells[theCol][theRow];
	if ((anAtom == NULL) || anAtom->mPre || !anAtom->mPlaced || anAtom->mFollowing || anAtom->mDead || (theGrid->mCellState[theCol][theRow] < 0x96))
		return NULL;
	return anAtom;
}

bool Empty(Grid* theGrid, int theCol, int theRow)
{
	if ((theCol < 0) || (theCol >= Grid::COLS) || (theRow < 0) || (theRow >= Grid::ROWS))
		return false;
	// a free cell, or one a pre-atom waits in (an atom may be sent there)
	Atom* anAtom = theGrid->mCells[theCol][theRow];
	return (anAtom == NULL) || (anAtom->mPre && !anAtom->mPreStarted);
}

// all the atoms are still: nothing flies, follows a path or waits to be revealed
bool Quiet(Grid* theGrid)
{
	theGrid->mAtoms.Reset();
	Atom* anAtom = theGrid->mAtoms.Next();
	while (anAtom != NULL)
	{
		if (!anAtom->mPlaced || anAtom->mFollowing || anAtom->mDead)
			return false;
		anAtom = theGrid->mAtoms.Next();
	}
	return theGrid->mGroups.Size() == 0;
}

// the click of a player: move, press, release (the grid's coordinates)
void Press(Grid* theGrid, int theCol, int theRow)
{
	int aX = theCol * Grid::CELL + Grid::CELL / 2;
	int aY = theRow * Grid::CELL + Grid::CELL / 2;
	Click(theGrid->mApp, theGrid->mX + aX, theGrid->mY + aY);
}

// a move: select the atom, then the empty cell (the path is shown by the mouse move)
bool Move(Grid* theGrid, Atom* theAtom, int theToCol, int theToRow)
{
	if (!theGrid->mPathFinder->Find(theAtom->mCol, theAtom->mRow, theToCol, theToRow))
		return false;
	theGrid->mPathFinder->mPath.Clear();
	Press(theGrid, theAtom->mCol, theAtom->mRow);
	if (theGrid->mSelected != theAtom)
		return false;
	// the cell is clicked three updates later, as a player's would be (and as a scenario file replays it)
	AutoplayState* s = theGrid->mApp->mAutoplayState;
	s->mPending = true;
	s->mPendX = theGrid->mX + theToCol * Grid::CELL + Grid::CELL / 2;
	s->mPendY = theGrid->mY + theToRow * Grid::CELL + Grid::CELL / 2;
	s->mWait = 3;
	return true;
}

}

void AtomicaApp::AutoplayUpdate()
{
	AutoplayState* aState = mAutoplayState;
	aState->mTick++;
	if ((aState->mLog == NULL) && !mAutoplayLog.empty())
	{
		aState->mLog = fopen(mAutoplayLog.c_str(), "w");
		Log(aState, "autoplay seed %u mode %s games %d", aState->mSeed, mAutoplayMode == 1 ? "action" : "strategy", mAutoplayGames);
	}
	if ((aState->mTick % 2000 == 0) && (mBoard != NULL) && (mBoard->mGrid != NULL))
		Log(aState, "update %d: score %d level %d moves %d atoms %d (f8c %g i16c %d b33c %d quiet %d sel %d pause %d)", aState->mTick,
			mBoard->mScore, mBoard->mMeter.mLevel, aState->mMoves, mBoard->mGrid->CountAtoms(false), mBoard->mStartCountdown, mBoard->mGetReadyAlpha,
			(int) mBoard->mGrid->mFieldRevealed, (int) Quiet(mBoard->mGrid), mBoard->mGrid->mSelected != NULL, mPauseCount);
	if (aState->mTick > mAutoplayCap)
	{
		Log(aState, "cap reached at update %d: games %d", aState->mTick, aState->mGames);
		if (!mAutoplayShots.empty())
			SaveScreenshot(StrFormat("%s-cap.png", mAutoplayShots.c_str()));
		if (aState->mLog != NULL)
			fclose(aState->mLog);
		Shutdown();
		return;
	}
	if (mTitleScreen != NULL)
	{
		// recording a scenario: the title screen and the menu are left at the updates the original's script clicks them
		if (mLoaded && (aState->mTick > (mAutoplayScript.empty() ? 60 : 1510)))
			TitleScreenDone();
		return;
	}
	if (aState->mWait > 0)
	{
		aState->mWait--;
		return;
	}
	if (aState->mPending)
	{
		aState->mPending = false;
		Click(this, aState->mPendX, aState->mPendY);
		aState->mWait = 2;
		return;
	}

	if (mScoreDialog != NULL)
	{
		// the high score entry: wait until the box is in place, give a name, press the button
		HighScoreDialog* aDialog = (HighScoreDialog*) mScoreDialog;
		if (aDialog->mBoxY >= 0)
		{
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-entry%d.png", mAutoplayShots.c_str(), aState->mGames));
			// the name is typed, and the button pressed three updates later, as a player would
			RecordLine(this, "text BOT", 0, 0);
			mWidgetManager->KeyChar('B');
			mWidgetManager->KeyChar('O');
			mWidgetManager->KeyChar('T');
			aState->mPending = true;
			aState->mPendX = aDialog->mX + aDialog->mOk->mX + aDialog->mOk->mWidth / 2;
			aState->mPendY = aDialog->mY + aDialog->mOk->mY + aDialog->mOk->mHeight / 2;
			aState->mWait = 3;
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
		if (aMenu->mGameOver && !aState->mMenuSeen)
		{
			aState->mMenuSeen = true;
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-menuover%d.png", mAutoplayShots.c_str(), aState->mGames));
			aState->mWait = 3;
			return;
		}
		if (aState->mGames >= mAutoplayGames)
		{
			Log(aState, "done after %d games at update %d", aState->mGames, aState->mTick);
			if (aState->mLog != NULL)
				fclose(aState->mLog);
			Shutdown();
			return;
		}
		aState->mGames++;
		aState->mMenuSeen = false;
		aState->mOverLogged = false;
		aState->mLevelLogged = false;
		aState->mMoves = 0;
		aState->mLastLevel = 0;
		if (aMenu->mGameOver)
			Click(this, 0x14a + 0x67, 0x178 + 0x12);
		else
			Click(this, 0x5a + 0xe2, (mAutoplayMode == 1 ? 0xb1 : 0x4b) + 0x1d);
		Log(aState, "game %d starts at update %d", aState->mGames, aState->mTick);
		aState->mWait = 20;
		return;
	}
	if ((mBoard == NULL) || (mBoard->mGrid == NULL))
		return;

	Board* aBoard = mBoard;
	Grid* aGrid = mBoard->mGrid;
	if (aGrid->mGameOver)
	{
		if (!aState->mOverLogged)
		{
			aState->mOverLogged = true;
			Log(aState, "game %d over at update %d: score %d level %d moves %d", aState->mGames, aState->mTick, aBoard->mScore,
				aBoard->mMeter.mLevel, aState->mMoves);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-over%d.png", mAutoplayShots.c_str(), aState->mGames));
		}
		return;
	}
	if (!mHintBoxes.empty())
	{
		// a tutorial box: wait until it has faded in, look at it, tick the check box of every other one, press Continue
		HintBox* aBox = mHintBoxes[0];
		if (aBox->mAlpha >= 0xff)
		{
			if (!mAutoplayShots.empty() && (aState->mShots < 40))
			{
				aState->mShots++;
				SaveScreenshot(StrFormat("%s-hint%d.png", mAutoplayShots.c_str(), aBox->mHint));
			}
			Log(aState, "hint %d at update %d", aBox->mHint, aState->mTick);
			// the box's check mark, then (three updates later, as a player's would be) its button
			if ((aBox->mHint & 1) && aBox->mHasCheck)
			{
				ClickButton(this, aBox, aBox->mCheck);
				aState->mPending = true;
				aState->mPendX = aBox->mX + aBox->mContinue->mX + aBox->mContinue->mWidth / 2;
				aState->mPendY = aBox->mY + aBox->mContinue->mY + aBox->mContinue->mHeight / 2;
				aState->mWait = 3;
				return;
			}
			ClickButton(this, aBox, aBox->mContinue);
			aState->mWait = 5;
		}
		return;
	}
	if (mLevelUpDialog != NULL)
	{
		// the level-up box: wait until it has slid down, look at it, press its button
		LevelUpDialog* aDialog = (LevelUpDialog*) mLevelUpDialog;
		if (aDialog->mBoxY >= 0)
		{
			if (!mAutoplayShots.empty() && (aState->mShots < 3))
			{
				aState->mShots++;
				SaveScreenshot(StrFormat("%s-levelup%d.png", mAutoplayShots.c_str(), aState->mShots));
			}
			ClickButton(this, aDialog, aDialog->mContinue);
			aState->mWait = 5;
		}
		return;
	}
	if (aBoard->mMeter.mFull)
	{
		if (!aState->mLevelLogged)
		{
			aState->mLevelLogged = true;
			Log(aState, "level %d done at update %d: score %d moves %d", aBoard->mMeter.mLevel, aState->mTick, aBoard->mScore, aState->mMoves);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-level%d-%d.png", mAutoplayShots.c_str(), aState->mGames, aBoard->mMeter.mLevel));
		}
		return;
	}
	aState->mLevelLogged = false;
	if ((aBoard->mStartCountdown > 0.0f) || (aBoard->mGetReadyAlpha > 4) || !aGrid->mFieldRevealed || !Quiet(aGrid) || (aGrid->mSelected != NULL) ||
		(aBoard->mBombOn) || (aGrid->mCombo > 0 && aGrid->mComboEnded == false && aState->mTick % 4 != 0))
		return;
	if (mPauseCount > 0)
		return;

	if (mAutoplayBonus)
	{
		BonusMolecules* aBonus = aBoard->mBonus;
		if ((aState->mBonusStage == 0) && (aBonus->mTarget < 100) && (aBonus->mRound < aBonus->mTarget) && (aState->mMoves > 2))
			aBonus->mRound = aBonus->mTarget;
		if ((aState->mBonusStage == 0) && (aBonus->mPattern != NULL) && (aBonus->mX >= 0))
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
			Log(aState, "bonus shape made at update %d (element %d)", aState->mTick, aBonus->mElement);
			aState->mBonusStage = 1;
			aState->mWait = 400;
			return;
		}
		if (aState->mBonusStage == 1)
		{
			for (int aCol = 0; aCol < Grid::COLS; aCol++)
				for (int aRow = 0; aRow < Grid::ROWS; aRow++)
				{
					Atom* anAtom = aGrid->mCells[aCol][aRow];
					if ((anAtom != NULL) && anAtom->IsElement() && !anAtom->mPre)
					{
						Log(aState, "element atom type %d at %d,%d at update %d", anAtom->mType, aCol, aRow, aState->mTick);
						if (!mAutoplayShots.empty())
							SaveScreenshot(StrFormat("%s-element.png", mAutoplayShots.c_str()));
						// a click selects it, a double click sets it off
						Press(aGrid, aCol, aRow);
						int aX = aCol * Grid::CELL + Grid::CELL / 2;
						int aY = aRow * Grid::CELL + Grid::CELL / 2;
						aGrid->mIsOver = true;
						aGrid->MouseDown(aX, aY, 0, 1);
						aGrid->MouseDown(aX, aY, 0, 2);
						Log(aState, "element set off at update %d", aState->mTick);
						aState->mBonusStage = 3;
						aState->mWait = 100;
						return;
					}
				}
		}
		if ((aState->mBonusStage == 3) && !mAutoplayShots.empty() && (aState->mShots < 100))
		{
			aState->mShots = 100;
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
								aState->mMoves++;
								if ((mAutoplayFinish > 0) && (aState->mMoves % mAutoplayFinish == 0))
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
		Atom* anAtom = aAtoms[aState->Rand((int) aAtoms.size())];
		int aToCol = aState->Rand(Grid::COLS);
		int aToRow = aState->Rand(Grid::ROWS);
		if (!Empty(aGrid, aToCol, aToRow))
			continue;
		if (Move(aGrid, anAtom, aToCol, aToRow))
		{
			aState->mMoves++;
			if ((mAutoplayFinish > 0) && (aState->mMoves % mAutoplayFinish == 0))
				aBoard->mMeter.Add(1000, 0);
			return;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The `move` command (docs/TEST_COMMANDS.md): the bot's next move as the real input that makes it. It is the bot above as
// a function of the state: the same search (a block of four that lacks one atom of its colour, else a random atom to a
// random reachable cell), but it clicks nothing and changes nothing in the game; the driver sends the clicks as real
// input. Its dice are its own, seeded from the position, so the same position gives the same move and the game's
// generator is never drawn from. A move that takes two clicks (select an atom, then its cell) is two answers: the
// first returns the atom's click and remembers the cell, the next, with the atom selected, returns the cell's.
//
// The payload is {"kind":..., "input":[{"click":[x,y]}, {"key":"B"}, ...], "updates":n} (docs/TEST_COMMANDS.md, a move's
// input steps), or {"kind":"wait","updates":n} when the game is busy (a screen loading, atoms moving, a box fading in).

namespace
{

struct BotMemory
{
	int		mCol, mRow;			// the atom that was selected
	int		mToCol, mToRow;		// and the cell it goes to
	bool	mValid;
};
BotMemory gBotMemory = {0, 0, 0, 0, false};

std::string N(int theValue)
{
	char aBuf[32];
	snprintf(aBuf, sizeof(aBuf), "%d", theValue);
	return aBuf;
}

std::string ClickStep(int theX, int theY)
{
	return "{\"click\":[" + N(theX) + "," + N(theY) + "]}";
}

std::string WaitJson(const char* theWhy, int theUpdates)
{
	return std::string("{\"kind\":\"wait\",\"why\":\"") + theWhy + "\",\"updates\":" + N(theUpdates) + "}";
}

std::string ClickMove(const char* theKind, int theX, int theY, int theUpdates)
{
	return std::string("{\"kind\":\"") + theKind + "\",\"input\":[" + ClickStep(theX, theY) + "],\"updates\":" + N(theUpdates) + "}";
}

int CentreX(Widget* theWidget)
{
	return theWidget->mX + theWidget->mWidth / 2;
}

int CentreY(Widget* theWidget)
{
	return theWidget->mY + theWidget->mHeight / 2;
}

// a button of a box: the middle of it in game coordinates. A box's buttons are widgets of their own that the box places in
// game coordinates (GameDialog::Resize puts them at mX + 0x5f and so on), so the box's own position is not added.
std::string ButtonClick(Widget* theButton)
{
	return ClickStep(CentreX(theButton), CentreY(theButton));
}

unsigned int PositionSeed(AtomicaApp* theApp, Grid* theGrid)
{
	unsigned int aSeed = (unsigned int) theApp->mBoard->mScore * 2654435761u + (unsigned int) theApp->mBoard->mMeter.mLevel * 40503u;
	for (int aCol = 0; aCol < Grid::COLS; aCol++)
		for (int aRow = 0; aRow < Grid::ROWS; aRow++)
		{
			Atom* anAtom = theGrid->mCells[aCol][aRow];
			aSeed = aSeed * 31u + (anAtom == NULL ? 0u : (unsigned int) anAtom->mType + 1u);
		}
	return aSeed;
}

// a path to the cell exists (the path finder is the bot's own check, as Move uses it)
bool CanReach(Grid* theGrid, Atom* theAtom, int theToCol, int theToRow)
{
	bool aFound = theGrid->mPathFinder->Find(theAtom->mCol, theAtom->mRow, theToCol, theToRow);
	theGrid->mPathFinder->mPath.Clear();
	return aFound;
}

// the bot's choice, as the autoplay makes it: a block of four that lacks one atom, then a random move
bool ChooseMove(Grid* theGrid, unsigned int theSeed, Atom** theSource, int* theToCol, int* theToRow)
{
	for (int aCol = 0; aCol < Grid::COLS - 1; aCol++)
		for (int aRow = 0; aRow < Grid::ROWS - 1; aRow++)
			for (int aType = 0; aType < theGrid->mNumColors; aType++)
			{
				int aHave = 0;
				int aGapCol = -1;
				int aGapRow = -1;
				int aGaps = 0;
				for (int aDC = 0; aDC < 2; aDC++)
					for (int aDR = 0; aDR < 2; aDR++)
					{
						Atom* anAtom = Movable(theGrid, aCol + aDC, aRow + aDR);
						if ((anAtom != NULL) && ((anAtom->mType == aType) || (anAtom->mKind == KIND_CHROMIUM)))
							aHave++;
						else if (Empty(theGrid, aCol + aDC, aRow + aDR))
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
						Atom* aSource = Movable(theGrid, aC, aR);
						if ((aSource != NULL) && (aSource->mType == aType) && (aSource->mKind == KIND_PLAIN) &&
							CanReach(theGrid, aSource, aGapCol, aGapRow))
						{
							*theSource = aSource;
							*theToCol = aGapCol;
							*theToRow = aGapRow;
							return true;
						}
					}
			}
	std::vector<Atom*> aAtoms;
	for (int aCol = 0; aCol < Grid::COLS; aCol++)
		for (int aRow = 0; aRow < Grid::ROWS; aRow++)
		{
			Atom* anAtom = Movable(theGrid, aCol, aRow);
			if (anAtom != NULL)
				aAtoms.push_back(anAtom);
		}
	unsigned int aSeed = theSeed;
	for (int aTry = 0; (aTry < 60) && !aAtoms.empty(); aTry++)
	{
		aSeed = aSeed * 1103515245u + 12345u;
		Atom* anAtom = aAtoms[(aSeed >> 16) % (unsigned int) aAtoms.size()];
		aSeed = aSeed * 1103515245u + 12345u;
		int aToCol = (int) ((aSeed >> 16) % (unsigned int) Grid::COLS);
		aSeed = aSeed * 1103515245u + 12345u;
		int aToRow = (int) ((aSeed >> 16) % (unsigned int) Grid::ROWS);
		if (!Empty(theGrid, aToCol, aToRow))
			continue;
		if (CanReach(theGrid, anAtom, aToCol, aToRow))
		{
			*theSource = anAtom;
			*theToCol = aToCol;
			*theToRow = aToRow;
			return true;
		}
	}
	// the random tries found nothing (a crowded field): every atom against every empty cell, from a place that the position picks,
	// so a field that has a move is never taken for one that has none
	if (!aAtoms.empty())
	{
		unsigned int aFrom = (aSeed >> 16) % (unsigned int) aAtoms.size();
		for (size_t i = 0; i < aAtoms.size(); i++)
		{
			Atom* anAtom = aAtoms[(aFrom + i) % aAtoms.size()];
			for (int aToCol = 0; aToCol < Grid::COLS; aToCol++)
				for (int aToRow = 0; aToRow < Grid::ROWS; aToRow++)
					if (Empty(theGrid, aToCol, aToRow) && CanReach(theGrid, anAtom, aToCol, aToRow))
					{
						*theSource = anAtom;
						*theToCol = aToCol;
						*theToRow = aToRow;
						return true;
					}
		}
	}
	return false;
}

}

std::string AtomicaApp::BotMoveJson()
{
	// the dialogs the bot does not open: no answer
	if ((mOptionsDialog != NULL) || (mQuitDialog != NULL) || (mHighScoresBox != NULL))
		return "null";

	// the high score entry: the name is typed and the button pressed
	if (mScoreDialog != NULL)
	{
		HighScoreDialog* aDialog = (HighScoreDialog*) mScoreDialog;
		if (aDialog->mBoxY < 0)
			return WaitJson("entry box sliding in", 5);
		return std::string("{\"kind\":\"name\",\"input\":[{\"key\":\"B\"},{\"key\":\"O\"},{\"key\":\"T\"},{\"wait\":3}," ) +
			ButtonClick(aDialog->mOk) + "],\"updates\":5}";
	}

	// the title screen: its link, once it is there
	if (mTitleScreen != NULL)
	{
		if (mMenu != NULL)
			return WaitJson("menu doors", 20);
		HyperlinkWidget* aLink = mTitleScreen->mPlayLink;
		if (!mLoaded || !aLink->mVisible || (aLink->mWidth <= 0))
			return WaitJson("loading", 30);
		return ClickMove("play", CentreX(aLink), CentreY(aLink), 20);
	}

	// the menu: the start button of the mode that is set, or "main menu" after a game
	if (mMenu != NULL)
	{
		MainMenu* aMenu = (MainMenu*) mMenu;
		if (!aMenu->PanelShown())
			return WaitJson("menu panel", 20);
		if (aMenu->mGameOver)
		{
			if (!aMenu->mEntryShown)
				return WaitJson("scores", 20);
			return ClickMove("main menu", CentreX(aMenu->mBtn[6]), CentreY(aMenu->mBtn[6]), 20);
		}
		int anId = mMode * 2 + (mExpertMode != 0 ? 1 : 0);
		if (aMenu->mBtn[anId] == NULL)
			return "null";
		return ClickMove(mMode == 1 ? "start action" : "start strategy", CentreX(aMenu->mBtn[anId]), CentreY(aMenu->mBtn[anId]), 20);
	}

	if ((mBoard == NULL) || (mBoard->mGrid == NULL))
		return WaitJson("no board", 10);
	Board* aBoard = mBoard;
	Grid* aGrid = aBoard->mGrid;
	if (aGrid->mGameOver)
		return WaitJson("game over", 20);

	// a tutorial box: the check mark of every other one, then Continue
	if (!mHintBoxes.empty())
	{
		HintBox* aBox = mHintBoxes[0];
		if (aBox->mAlpha < 0xff)
			return WaitJson("hint box fading in", 3);
		if ((aBox->mHint & 1) && aBox->mHasCheck)
			return "{\"kind\":\"hint\",\"input\":[" + ButtonClick(aBox->mCheck) + ",{\"wait\":3}," +
				ButtonClick(aBox->mContinue) + "],\"updates\":5}";
		return ClickMove("hint", CentreX(aBox->mContinue), CentreY(aBox->mContinue), 5);
	}
	if (mLevelUpDialog != NULL)
	{
		LevelUpDialog* aDialog = (LevelUpDialog*) mLevelUpDialog;
		if (aDialog->mBoxY < 0)
			return WaitJson("level box sliding in", 5);
		return ClickMove("level up", CentreX(aDialog->mContinue), CentreY(aDialog->mContinue), 5);
	}
	if (aBoard->mMeter.mFull)
		return WaitJson("level made", 20);
	if ((aBoard->mStartCountdown > 0.0f) || (aBoard->mGetReadyAlpha > 4) || !aGrid->mFieldRevealed || !Quiet(aGrid) || aBoard->mBombOn || (mPauseCount > 0))
		return WaitJson("atoms moving", 4);

	// an atom is held: its cell is next
	if (aGrid->mSelected != NULL)
	{
		Atom* aHeld = aGrid->mSelected;
		if (gBotMemory.mValid && (gBotMemory.mCol == aHeld->mCol) && (gBotMemory.mRow == aHeld->mRow))
		{
			gBotMemory.mValid = false;
			return ClickMove("place", aGrid->mX + gBotMemory.mToCol * Grid::CELL + Grid::CELL / 2,
				aGrid->mY + gBotMemory.mToRow * Grid::CELL + Grid::CELL / 2, 3);
		}
		// not the bot's own selection: click the atom again to put it down
		return ClickMove("put down", aGrid->mX + aHeld->mCol * Grid::CELL + Grid::CELL / 2,
			aGrid->mY + aHeld->mRow * Grid::CELL + Grid::CELL / 2, 3);
	}

	Atom* aSource = NULL;
	int aToCol = 0;
	int aToRow = 0;
	if (!ChooseMove(aGrid, PositionSeed(this, aGrid), &aSource, &aToCol, &aToRow))
		return "null";
	gBotMemory.mCol = aSource->mCol;
	gBotMemory.mRow = aSource->mRow;
	gBotMemory.mToCol = aToCol;
	gBotMemory.mToRow = aToRow;
	gBotMemory.mValid = true;
	return ClickMove("select", aGrid->mX + aSource->mCol * Grid::CELL + Grid::CELL / 2,
		aGrid->mY + aSource->mRow * Grid::CELL + Grid::CELL / 2, 3);
}
