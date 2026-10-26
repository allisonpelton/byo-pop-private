// Test hook: the script command "autoplay <mode> <difficulty> <updates>" starts a game of that mode and plays it with
// random clicks and drags for that many updates, checking the board's grid on the way. It logs "autoplay: ..." lines
// (the status every 500 updates and a "done" line); tools/check.sh rocketmania reads them. Only scripts reach it.

#include "RocketApp.h"
#include "Board.h"
#include "Dialogs.h"
#include "WidgetManager.h"

#include <SDL.h>
#include <cstdio>

using namespace Sexy;

static unsigned int sSeed = 12345;

static int Next(int theRange)
{
	sSeed = sSeed * 1103515245u + 12345u;
	return (int) ((sSeed >> 16) % (unsigned int) theRange);
}

// the cells whose piece does not know its place, and the pieces that stand in two cells
static int BadCells(Board* theBoard)
{
	int aBad = 0;
	for (int aRow = 0; aRow < theBoard->mRows; aRow++)
	{
		for (int aColumn = 0; aColumn < theBoard->mCols; aColumn++)
		{
			Piece* aPiece = theBoard->mGrid[aRow][aColumn];
			if (aPiece != NULL && (aPiece->mRow != aRow || aPiece->mColumn != aColumn))
				aBad++;
		}
	}
	return aBad;
}

static int Tiles(Board* theBoard)
{
	int aCount = 0;
	for (int aRow = 0; aRow < theBoard->mRows; aRow++)
		for (int aColumn = 0; aColumn < theBoard->mCols; aColumn++)
			if (theBoard->mGrid[aRow][aColumn] != NULL)
				aCount++;
	return aCount;
}

bool RocketAutoplay(RocketApp* theApp, const ScriptCommand& theCommand)
{
	int aMode = 0, aDifficulty = 1, aLeft = 0, aTotal = -1;
	if (sscanf(theCommand.mArgs.c_str(), "%d %d %d %d", &aMode, &aDifficulty, &aLeft, &aTotal) < 3)
	{
		SDL_Log("autoplay: bad arguments '%s'", theCommand.mArgs.c_str());
		return true;
	}
	if (aTotal < 0)
	{
		aTotal = aLeft;
		sSeed = 12345;
		theApp->mMode = aMode;
		theApp->mDifficulty = aDifficulty;
		theApp->StartBoard(false);
	}
	Board* aBoard = theApp->mBoard;
	if (aBoard == NULL)
	{
		SDL_Log("autoplay: no board");
		return true;
	}
	int aDone = aTotal - aLeft;
	if (aLeft > 0 && aBoard->mState == 0 && aDone % 400 == 100 && !aBoard->m2e8)
	{
		// every now and then the lowest full row of tiles is turned into a straight pipe from edge to edge, so that the
		// rules of a launch run (random clicks hardly ever make one)
		for (int aRow = aBoard->mRows - 1; aRow >= 0; aRow--)
		{
			bool aFull = true;
			for (int aColumn = 0; aColumn < aBoard->mCols; aColumn++)
			{
				Piece* aPiece = aBoard->mGrid[aRow][aColumn];
				if (aPiece == NULL || aPiece->mType != 1)
					aFull = false;
			}
			if (aFull)
			{
				for (int aColumn = 0; aColumn < aBoard->mCols; aColumn++)
				{
					aBoard->mGrid[aRow][aColumn]->mKind = 0;
					aBoard->mGrid[aRow][aColumn]->mRotation = 1;
				}
				SDL_Log("autoplay: row %d made a straight pipe at update %d", aRow, aDone);
				break;
			}
		}
	}
	// (a click on a burning tile puts the fire out, so nothing is clicked while a connection burns)
	if (aLeft > 0 && aBoard->mState == 0 && !aBoard->m2e8)
	{
		int anAction = Next(10);
		int aRow = Next(aBoard->mRows);
		int aColumn = Next(aBoard->mCols);
		int aX = Board::CellX(aColumn) + 21;
		int aY = Board::CellY(aRow) + 21;
		int aButton = Next(3) == 0 ? -1 : 1;
		WidgetManager* aManager = theApp->mWidgetManager;
		if (anAction < 7)
		{
			aManager->MouseMove(aX, aY);
			aManager->MouseDown(aX, aY, aButton);
			aManager->MouseUp(aX, aY, aButton);
		}
		else if (anAction == 7)
		{
			aManager->MouseMove(aX, aY);
			aManager->MouseDown(aX, aY, 1);
			aManager->MouseDrag(aX + (Next(2) == 0 ? -30 : 30), aY);
			aManager->MouseUp(aX, aY, 1);
		}
		else if (anAction == 8)
		{
			aManager->MouseMove(300, 20);
			aManager->MouseDown(300, 20, 1);
			aManager->MouseUp(300, 20, 1);
		}
	}
	if (aDone % 500 == 0 || aLeft == 0)
	{
		SDL_Log("autoplay: %s update %d state %d score %d level %d tiles %d bad cells %d burning %d", aLeft == 0 ? "done" : "status",
			aDone, aBoard->mState, aBoard->mScore, aBoard->mLevel, Tiles(aBoard), BadCells(aBoard), aBoard->m2e8 ? 1 : 0);
	}
	if (aLeft > 0)
	{
		char anArgs[64];
		snprintf(anArgs, sizeof(anArgs), "%d %d %d %d", aMode, aDifficulty, aLeft - 1, aTotal);
		theApp->AddScriptCommand(theApp->mUpdateCount + 1, "autoplay", anArgs);
	}
	return true;
}

// "hint <n>": opens that hint (a bonus item's hint needs a board: an arcade game starts first)
bool RocketHint(RocketApp* theApp, const ScriptCommand& theCommand)
{
	int aHint = 0;
	if ((sscanf(theCommand.mArgs.c_str(), "%d", &aHint) < 1) || (aHint < 0) || (aHint > 7))
	{
		SDL_Log("hint: bad arguments '%s'", theCommand.mArgs.c_str());
		return true;
	}
	if ((aHint > 2) && (theApp->mBoard == NULL))
	{
		theApp->mMode = 0;
		theApp->mDifficulty = 1;
		theApp->StartBoard(false);
	}
	theApp->mNoHints = false;
	theApp->mHintFlags[aHint] = false;
	theApp->ShowModeHint(aHint, false);
	return true;
}

// "gameover <mode> <score>": a game of that mode ends with that score: the high scores open the way they do after a game
// (so that the dialog for a name comes up for a score that makes the lists)
bool RocketGameOver(RocketApp* theApp, const ScriptCommand& theCommand)
{
	int aMode = 0, aScore = 0;
	if (sscanf(theCommand.mArgs.c_str(), "%d %d", &aMode, &aScore) < 2)
	{
		SDL_Log("gameover: bad arguments '%s'", theCommand.mArgs.c_str());
		return true;
	}
	theApp->mMode = aMode;
	theApp->mDifficulty = 1;
	theApp->StartBoard(false);
	if (theApp->mBoard != NULL)
		theApp->mBoard->mScore = aScore;
	theApp->ShowHighScores(0);
	return true;
}

// "levelup <kind> <mode> <level> <score> [<page>]": a game of that mode, at that level and score, opens the level-up
// screen (kind 4 after a level, 5 after a game) with some figures; page sets the page it starts at
bool RocketLevelUp(RocketApp* theApp, const ScriptCommand& theCommand)
{
	int aKind = 4, aMode = 0, aLevel = 0, aScore = 0, aPage = 0;
	if ((sscanf(theCommand.mArgs.c_str(), "%d %d %d %d %d", &aKind, &aMode, &aLevel, &aScore, &aPage) < 4) ||
		((aKind != 4) && (aKind != 5)))
	{
		SDL_Log("levelup: bad arguments '%s'", theCommand.mArgs.c_str());
		return true;
	}
	theApp->mMode = aMode;
	theApp->mDifficulty = 1;
	theApp->mNoHints = true;
	theApp->StartBoard(false);
	theApp->mNoHints = false;
	Board* aBoard = theApp->mBoard;
	if (aBoard == NULL)
		return true;
	aBoard->mLevel = aLevel;
	aBoard->mScore = aScore;
	aBoard->mC4 = 7;
	aBoard->mC8 = 4;
	aBoard->mCc = 3;
	aBoard->mD0 = 1;
	aBoard->mD4 = 2;
	aBoard->mD8 = 1;
	aBoard->mDc = 5;
	aBoard->ShowLevelUp(aKind);
	LevelUpDialog* aDialog = (LevelUpDialog*) theApp->GetDialog(DIALOG_LEVEL_UP);
	if (aDialog != NULL)
		aDialog->mPhase = aPage;
	return true;
}

// "buyrockets <over> <level> <coins> <meter>": a game of the arcade mode at that level, with that many coins and every
// launcher's meter at that value, puts up the buy-rockets screen (over 1: the game is over, so that the coins turn into points)
bool RocketBuyRockets(RocketApp* theApp, const ScriptCommand& theCommand)
{
	int anOver = 0, aLevel = 0, aCoins = 0, aMeter = 0;
	if (sscanf(theCommand.mArgs.c_str(), "%d %d %d %d", &anOver, &aLevel, &aCoins, &aMeter) < 4)
	{
		SDL_Log("buyrockets: bad arguments '%s'", theCommand.mArgs.c_str());
		return true;
	}
	theApp->mMode = 0;
	theApp->mDifficulty = 1;
	theApp->StartBoard(false);
	theApp->mNoHints = false;
	Board* aBoard = theApp->mBoard;
	if (aBoard == NULL)
		return true;
	aBoard->mLevel = aLevel;
	aBoard->m8c[7] = aCoins;
	for (int i = 0; i < 9; i++)
		aBoard->mGen->mMeter[i] = aMeter;
	if (anOver)
		aBoard->F421de0(1);
	else
	{
		aBoard->mState = 6;
		aBoard->ShowBuyRockets();
	}
	return true;
}

