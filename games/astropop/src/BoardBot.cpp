// Board: what the test commands read of the game (the field) and the bot behind `move` (docs/TEST_COMMANDS.md). The bot
// takes a brick or more from the bottom of a column and shoots them up the column where they join the biggest group of their
// kind: a group is broken up when it reaches mSetSize bricks. It reads the field and the hand and changes nothing, and it draws
// no random number. port: nothing here is in the original; none of it changes the game.

#include "Board.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "Compat2004.h"
#include "FullScreenHelp.h"
#include "StoryScreen.h"

#include <cstdio>
#include <string>

using namespace Sexy;

// The field as a JSON array, one string for each of its rows (row 0 on top), one character for each column: a dot for no
// brick, a digit or letter for the kind of a brick that is still (the colours are 0 to 4, the rest the special kinds, a
// virus brick is c), a star for one that moves.
std::string Board::DescribeField()
{
	std::string aResult = "[";
	for (size_t aRow = 0; aRow < mFieldBricks.size(); aRow++)
	{
		if (aRow > 0)
			aResult += ",";
		aResult += "\"";
		for (size_t aCol = 0; aCol < mFieldBricks[aRow].size(); aCol++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick == NULL)
				aResult += '.';
			else if (!aBrick->IsIdle())
				aResult += '*';
			else
			{
				int aType = aBrick->GetEffectiveType();
				if (aType >= 0 && aType <= 9)
					aResult += (char) ('0' + aType);
				else if (aType >= 10 && aType < 36)
					aResult += (char) ('a' + (aType - 10));
				else
					aResult += '?';
			}
		}
		aResult += "\"";
	}
	aResult += "]";
	return aResult;
}

namespace
{

const int kRows = 19;
const int kCols = 10;
const int kNone = -1;
const int kMoving = 99;         // a brick that moves is not one that can be taken or joined

// The pointer goes to the middle of a column, at the height of the ship
int ColumnX(int theCol)
{
	return 0xa6 + theCol * 0x2c + 0x16;
}
const int kShipY = 0x1b8;

struct FieldGrid
{
	int mKind[kRows][kCols];
};

// The bricks that touch the cell and hold the same kind, the cell included (the top row never joins a group)
int GroupSize(const FieldGrid& theGrid, int theKind, int theCol, int theRow)
{
	bool aSeen[kRows][kCols];
	for (int r = 0; r < kRows; r++)
		for (int c = 0; c < kCols; c++)
			aSeen[r][c] = false;

	int aStack[kRows * kCols][2];
	int aTop = 0;
	int aCount = 0;
	aStack[aTop][0] = theCol;
	aStack[aTop][1] = theRow;
	aTop++;
	aSeen[theRow][theCol] = true;
	while (aTop > 0)
	{
		aTop--;
		int aCol = aStack[aTop][0];
		int aRow = aStack[aTop][1];
		aCount++;
		static const int aStep[4][2] = {{1, 0}, {-1, 0}, {0, -1}, {0, 1}};
		for (int i = 0; i < 4; i++)
		{
			int aNextCol = aCol + aStep[i][0];
			int aNextRow = aRow + aStep[i][1];
			if (aNextCol < 0 || aNextCol >= kCols || aNextRow < 1 || aNextRow >= kRows)
				continue;
			if (aSeen[aNextRow][aNextCol] || theGrid.mKind[aNextRow][aNextCol] != theKind)
				continue;
			aSeen[aNextRow][aNextCol] = true;
			aStack[aTop][0] = aNextCol;
			aStack[aTop][1] = aNextRow;
			aTop++;
		}
	}
	return aCount;
}

// The row of the lowest brick of a column, or -1
int BottomRow(const FieldGrid& theGrid, int theCol)
{
	for (int r = kRows - 1; r >= 0; r--)
	{
		if (theGrid.mKind[r][theCol] != kNone)
			return r;
	}
	return -1;
}

// The kinds the bot takes and shoots: the colours
bool IsColour(int theKind)
{
	return theKind >= 0 && theKind <= 5;
}

std::string Steps(const std::string& theSteps)
{
	return "\"input\":[" + theSteps + "]";
}

std::string MoveStep(int theCol)
{
	return StrFormat("{\"move\":[%d,%d]}", ColumnX(theCol), kShipY);
}

std::string ClickStep(int theCol, bool theRight)
{
	return StrFormat("{\"click\":[%d,%d]%s}", ColumnX(theCol), kShipY, theRight ? ",\"button\":\"right\"" : "");
}

std::string WaitMove()
{
	return "{\"kind\":\"wait\",\"updates\":10,\"input\":[]}";
}

}

// The game takes no input now: bricks are in the air, a row comes, a stage is counted, a box or the pause is up (the
// conditions that the pointer and the buttons of the field turn away themselves)
// port: the bot (a test driver's view of the game); the original has none
int Board::BotBusyReason()
{
	if (mDemoPlaying && !mDemoInputActive)
		return 1;
	if (mScreenWiping || mWarpCharging || mClearCountRunning || mWarpGatePicked || mStreaksFadingOut)
		return 2;
	if (mWarpActive && !mAtWarpGate)
		return 3;
	if (!mWarpActive && !mWarpFlight && !mAtWarpGate && mLevelUpPanelHeight + mLevelUpPanelY > 0)
		return 4;
	if (mPauseCount != 0 || mLevelUpPhase || mHeldBricksNotLanded > 0)
		return 5;
	if (CountMovingBricks(false) > 0)
		return 6;
	return (!mFiredGroups.empty() || !mFallingGroups.empty()) ? 7 : 0;
}

// port: the bot (a test driver's view of the game); the original has none
bool Board::BotBusy()
{
	return BotBusyReason() != 0;
}

// The move the bot makes next: a text (JSON) with what it is and `input`, the real input that makes it. "null" when there is
// no move to make (the game is over, or no brick of the field makes a group), and a wait when the game takes none now.
// port: the bot (a test driver's view of the game); the original has none
std::string Board::BotMove()
{
	if (mGameOver)
		return "null";

	// the picture of how to play that a game opens with is closed with its button
	if (mApp->mHelpScreen != NULL)
	{
		AstroButton* aButton = ((FullScreenHelp*) mApp->mHelpScreen)->mContinueButton;
		return "{\"kind\":\"help\",\"updates\":10," +
			   Steps(StrFormat("{\"click\":[%d,%d]}", aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2)) + "}";
	}

	// a hint that waits to be read is put away with its button
	if (mHintOkButton != NULL && mHintOkButton->mVisible && (mWeaponHintShown || (!mHintSpots.empty() && mHintSpots[0]->mActive)))
		return "{\"kind\":\"hint\"," +
			   Steps(StrFormat("{\"click\":[%d,%d]}", mHintOkButton->mX + mHintOkButton->mWidth / 2, mHintOkButton->mY + mHintOkButton->mHeight / 2)) + "}";

	// the tutorial's demo plays by itself; the link it shows (to start a game) ends it
	if (mDemoPlaying && mDemoStartLink != NULL && mDemoStartLink->mVisible)
		return "{\"kind\":\"demo\",\"updates\":10," +
			   Steps(StrFormat("{\"click\":[%d,%d]}", mDemoStartLink->mX + mDemoStartLink->mWidth / 2, mDemoStartLink->mY + mDemoStartLink->mHeight / 2)) + "}";

	// the story that comes before a level: a click brings all of it in, then its button takes it away
	if (mStoryScreen != NULL)
	{
		AstroButton* aStoryButton = mStoryScreen->mButton;
		if (aStoryButton == NULL)
			return WaitMove();
		if (!aStoryButton->mVisible)
			return "{\"kind\":\"story\",\"updates\":10," + Steps("{\"click\":[320,200]}") + "}";
		return "{\"kind\":\"story\",\"updates\":10," +
			   Steps(StrFormat("{\"click\":[%d,%d]}", aStoryButton->mX + aStoryButton->mWidth / 2, aStoryButton->mY + aStoryButton->mHeight / 2)) + "}";
	}

	// the box of the warp screen after every fourth level (the weapon has been upgraded): its button closes it
	Dialog* anUpgradeBox = mApp->GetDialog(0x76);
	if (anUpgradeBox != NULL)
	{
		DialogButton* aButton = anUpgradeBox->mYesButton;
		return "{\"kind\":\"upgrade\",\"updates\":10," +
			   Steps(StrFormat("{\"click\":[%d,%d]}", aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2)) + "}";
	}

	// the warp screen between two levels waits for a click on its first panel (the next level) once the screen is up
	if (mAtWarpGate && mGameType == 1 && mLevelUpPanelY >= 0 && !mWarpGatePicked)
		return "{\"kind\":\"gate\",\"updates\":10," +
			   Steps(StrFormat("{\"click\":[%d,%d]}", mSlowGateX + 0x3c, 0xe1 + 0x14)) + "}";

	// the box that explains the weapon, before it starts: its button closes it and the weapon goes on
	Dialog* aPowerBox = mApp->GetDialog(0x7b);
	if (aPowerBox != NULL)
	{
		DialogButton* aButton = aPowerBox->mYesButton;
		return "{\"kind\":\"power\",\"updates\":10," +
			   Steps(StrFormat("{\"click\":[%d,%d]}", aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2)) + "}";
	}

	if (BotBusy())
		return WaitMove();

	// the weapon of the pilot, charged: the Return key starts it (the middle button and the start button do the same)
	if (mPowerupActive == 0 && m1bd8 == m1bdc && !mNewRowBouncing && !mBonusRoundOn && !mDemoPlaying)
		return "{\"kind\":\"weapon\",\"updates\":10," + Steps("{\"key\":\"Return\"}") + "}";

	FieldGrid aGrid;
	for (int r = 0; r < kRows; r++)
	{
		for (int c = 0; c < kCols; c++)
		{
			Brick* aBrick = (r < (int) mFieldBricks.size() && c < (int) mFieldBricks[r].size()) ? mFieldBricks[r][c] : NULL;
			if (aBrick == NULL)
				aGrid.mKind[r][c] = kNone;
			else if (!aBrick->IsIdle())
				aGrid.mKind[r][c] = kMoving;
			else
				aGrid.mKind[r][c] = aBrick->GetEffectiveType();
		}
	}

	// the supa weapon's laser takes the bottom brick of a column away: the tallest column loses it
	if (mPowerupActive == 1)
	{
		if (mMode != 0 && mMode != 3)
			return WaitMove();
		int aBest = -1;
		int aBestRow = 0;
		for (int c = 0; c < kCols; c++)
		{
			int aRow = BottomRow(aGrid, c);
			if (aRow > aBestRow)
			{
				aBestRow = aRow;
				aBest = c;
			}
		}
		if (aBest < 0)
			return WaitMove();      // the weapon has emptied the field: the next row comes by itself
		// the psychic takes the lit cells under the pointer: the pointer goes to the bottom brick of the tallest column. A
		// move lights its cell only when the pointer's last position was in the field already (MouseMove tests mLastPointerX before
		// it stores the new one), so the pointer moves twice within the cell before the click
		if (mMode == 3)
		{
			int aCellY = 0x22 + (aBestRow - 1) * 0x19 + 12;
			return StrFormat("{\"kind\":\"psychic\",\"column\":%d,", aBest) +
				   Steps(StrFormat("{\"move\":[%d,%d]},{\"wait\":2},{\"move\":[%d,%d]},{\"wait\":2},{\"click\":[%d,%d]}",
								   ColumnX(aBest), aCellY, ColumnX(aBest), aCellY + 1, ColumnX(aBest), aCellY + 1)) + "}";
		}
		return StrFormat("{\"kind\":\"laser\",\"column\":%d,", aBest) + Steps(MoveStep(aBest) + "," + ClickStep(aBest, false)) + "}";
	}

	// the bricks the ship holds (it shoots them), or the ones it will take
	bool aHolding = !mHeldBricks.empty();
	int aHandKind = -1;
	int aHandCount = 0;
	if (aHolding)
	{
		aHandKind = (mHandKindBrick != NULL) ? (mHandKindBrick->mMutated == 1 ? 0xc : mHandKindBrick->mType) : mHeldBricks.front()->GetEffectiveType();
		aHandCount = (int) mHeldBricks.size();
		if (!IsColour(aHandKind))
			return "null";
	}

	int aBestScore = -100000;
	int aBestPick = -1;
	int aBestDrop = -1;
	int aBestCount = 0;
	int aBestKind = -1;
	int aBestGroup = 0;
	int aPickEnd = aHolding ? 1 : kCols;
	for (int aPick = 0; aPick < aPickEnd; aPick++)
	{
		FieldGrid aWork = aGrid;
		int aKind = aHandKind;
		int aCount = aHandCount;
		if (!aHolding)
		{
			// from the bottom of the stack up: the bricks of the kind, up to six, never the top row
			int aRow = BottomRow(aWork, aPick);
			if (aRow < 1)
				continue;
			aKind = aWork.mKind[aRow][aPick];
			if (!IsColour(aKind))
				continue;
			aCount = 0;
			while (aRow >= 1 && aCount < 6 && aWork.mKind[aRow][aPick] == aKind)
			{
				aWork.mKind[aRow][aPick] = kNone;
				aCount++;
				aRow--;
			}
		}

		for (int aDrop = 0; aDrop < kCols; aDrop++)
		{
			if (!aHolding && aDrop == aPick)
				continue;
			int aBottom = BottomRow(aWork, aDrop);
			// FirePiece turns a shot away that would reach the ship: the lowest row it lands in stays above row 16
			if (aBottom + aCount > 15)
				continue;
			FieldGrid aShot = aWork;
			for (int i = 1; i <= aCount; i++)
				aShot.mKind[aBottom + i][aDrop] = aKind;
			int aGroup = GroupSize(aShot, aKind, aDrop, aBottom + 1);

			int aScore;
			if (aGroup >= mSetSize)
				aScore = 1000 + aGroup * 10 - (aBottom + aCount);
			else if (aGroup > aCount)
				aScore = aGroup * 10 - (aBottom + aCount);
			else
				continue;
			if (aScore > aBestScore)
			{
				aBestScore = aScore;
				aBestPick = aPick;
				aBestDrop = aDrop;
				aBestCount = aCount;
				aBestKind = aKind;
				aBestGroup = aGroup;
			}
		}
	}

	if (aBestDrop < 0)
		return "null";

	std::string aSteps;
	if (!aHolding)
		aSteps = MoveStep(aBestPick) + "," + ClickStep(aBestPick, false) + ",{\"wait\":6},";
	aSteps += MoveStep(aBestDrop) + "," + ClickStep(aBestDrop, true);
	return StrFormat("{\"kind\":\"shoot\",\"pick\":%d,\"drop\":%d,\"bricks\":%d,\"colour\":%d,\"group\":%d,\"updates\":20,",
					 aHolding ? -1 : aBestPick, aBestDrop, aBestCount, aBestKind, aBestGroup) +
		   Steps(aSteps) + "}";
}
