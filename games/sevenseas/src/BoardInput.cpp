// Board.cpp, part 5: the mouse (WinSS.exe 0x40a4a0 MouseDown, 0x40ad20 MouseMove, 0x40ae70 MouseUp, 0x40ae90 the
// button of the tutorial).

#include "Board.h"
#include "SevenSeasApp.h"
#include "Res.h"
#include "Compat2004.h"
#include "DialogButton.h"
#include "GameText.h"
#include "ImageFont.h"
#include "WidgetManager.h"

using namespace Sexy;

// the heading (0 up, 4 right, 8 down, 0xc left) of a step of (dx, dy)
static int StepHeadingOf(int theDX, int theDY)
{
	unsigned int aHeading = 0;
	if (theDY < 0)
	{
		if (theDX < 0)
			aHeading = 0xe;
		else
			aHeading = (theDX < 1) - 1 & 2;
	}
	else if (theDY < 1)
	{
		if (theDX < 0)
			aHeading = 0xc;
		else if (0 < theDX)
			aHeading = 4;
	}
	else if (theDX < 0)
		aHeading = 10;
	else
		aHeading = ((theDX < 1) - 1 & 0xfffffffe) + 8;
	return (int) aHeading;
}

// 0x40a4a0
void Board::MouseDown(int x, int y, int theClickCount)
{
	(void) theClickCount;
	mMouseX = x;
	mMouseY = y;
	if (mApp->mTutorial)
	{
		switch (mTutorialStep)
		{
		case 0:
		case 3:
		case 0xb:
		case 0xc:
		case 0xd:
			if (x < CellToPixel(8) + 4)
				return;
			if (CellToPixel(8) + 0x72 < x)
				return;
			if (y < CellToPixel(2) - 4)
				return;
			if (CellToPixel(2) + 0x27 < y)
				return;
			mTutorialStep = mTutorialStep + 1;
			if (mTutorialStep != 0xe)
				return;
			mStartLink = new DialogButton(mApp->mImages[IMAGE_DBUTTONWOOD], 1, this);
			mStartLink->Resize(mX + 0x37, mHeight - 0x53 + mY, mWidth - 0x7a, 0x22);
			mStartLink->SetFont(mApp->mFonts[FONT_MT11]);
			mStartLink->mLabel = GT(0x004a3084);
			mStartLink->SetColor(DialogButton::COLOR_LABEL, Color(0xff, 0xe4, 0x37));
			mWidgetManager->AddWidget(mStartLink);
			mWidgetManager->PutInfront(mStartLink, this);
			return;
		}
	}
	if (!mBonus)
	{
		if (((mState == STATE_IDLE) || (mState == STATE_BLINK)) && !mLevelOver)
		{
			int aDX = 0;
			int aDY = 0;
			int aCellX = PixelToCellX(x);
			int aCellY = PixelToCellY(y);
			if ((aCellX == mPlayerX - 1) && ((aCellY == mPlayerY - 1) || (aCellY == mPlayerY) || (aCellY == mPlayerY + 1)))
				aDX = -1;
			if ((aCellX == mPlayerX + 1) && ((aCellY == mPlayerY - 1) || (aCellY == mPlayerY) || (aCellY == mPlayerY + 1)))
				aDX = aDX + 1;
			if ((aCellY == mPlayerY - 1) && ((aCellX == mPlayerX - 1) || (aCellX == mPlayerX) || (aCellX == mPlayerX + 1)))
				aDY = -1;
			if ((aCellY == mPlayerY + 1) && ((aCellX == mPlayerX - 1) || (aCellX == mPlayerX) || (aCellX == mPlayerX + 1)))
				aDY = aDY + 1;
			if (mApp->mTutorial)
			{
				switch (mTutorialStep)
				{
				case 1:
				case 9:
					if (aDX != 0)
						return;
					if (aDY != 1)
						return;
					ClearCellFlash();
					mCellFlash[mPlayerX][mPlayerY + 1] = 1;
					break;
				case 2:
				case 10:
					if (PixelToCellX(x) != mPlayerX)
						return;
					// the original converts the y with the column function here
					if (PixelToCellX(y) != mPlayerY)
						return;
					break;
				default:
					return;
				case 4:
					if (aDX != 1)
						return;
					if (aDY != 0)
						return;
					break;
				case 5:
				case 6:
				case 7:
				case 8:
					if (aDX != 1)
						return;
					if (aDY != 1)
						return;
				}
				mTutorialStep = mTutorialStep + 1;
			}
			int aNewX = mPlayerX + aDX;
			int aNewY = aDY + mPlayerY;
			if (((aDX == 0) && (aDY == 0)) || (aNewX < 0) || (10 < aNewX) || (aNewY < 0) || (10 < aNewY) ||
				!CellFree(aNewX, aNewY))
			{
				if ((PixelToCellX(x) == mPlayerX) && (PixelToCellY(y) == mPlayerY))
				{
					SaveUndo();
					mTutorialShown = false;
					mBlink = false;
					FirePlayerShots();
				}
			}
			else
			{
				int aHeading = StepHeadingOf(aDX, aDY);
				SaveUndo();
				mWantHeading = aHeading;
				mBlink = false;
				mDestX = mPlayerX + aDX;
				mDestY = aDY + mPlayerY;
				mState = STATE_TURN_MOVE;
				mTutorialShown = false;
				mDirty = true;
				mApp->PlaySample(SOUND_MOVEGOOD, Pan(mPlayerX));
			}
		}
	}
	else
	{
		// the bonus screen: dig for the treasure
		int aCol = PixelToCellX(x - 5);
		int aRow = PixelToCellX(y - 0x12);
		int aColIdx = aCol - 1;
		int aRowIdx = aRow - 1;
		if (mBonusState == 0)
		{
			if (((-1 < aColIdx) && (aColIdx < 9)) && (-1 < aRowIdx) && (aRowIdx < 9) && (mBonusGrid[aColIdx][aRowIdx] < 0))
			{
				int* aCell = &mBonusGrid[aColIdx][aRowIdx];
				if (!mBonusReady)
					SetupBonus(aColIdx, aRowIdx);
				if (*aCell == -2)
				{
					mBonusTick = mUpdateCnt;
					*aCell = 99;
					mBonusState = 1;
					mApp->SetMusicOrder(0x10, false);
					mApp->SetMusicOrder(0x10, false);
				}
				else if (*aCell == -3)
				{
					*aCell = 100;
					SpawnGhost(aCol, aRow);
					mBonusState = 2;
					mBonusTick2 = mUpdateCnt;
					mApp->PlaySample(0x16);
				}
				else
				{
					mApp->PlaySample(SOUND_BONUSDIG, Pan(aColIdx));
					*aCell = 0;
					bool aNear = false;
					int aFirstCol = aCol - 2;
					if (aFirstCol <= aCol)
					{
						int aColsLeft = (aCol - aFirstCol) + 1;
						int aBase = aFirstCol * 9 + 0x265;
						do
						{
							for (int aR = aRow - 2; aR <= aRow; aR = aR + 1)
							{
								if (((0x264 < aBase) && (aBase < 0x2ae)) && ((-1 < aR) && (aR < 9)))
								{
									int aValue = (&mBonusGrid[0][0])[aBase - 0x265 + aR];
									if ((aValue == -3) || (aValue == 100))
									{
										aNear = true;
										*aCell = *aCell + 1;
									}
								}
							}
							aBase = aBase + 9;
							aColsLeft = aColsLeft - 1;
						}
						while (aColsLeft != 0);
						if (aNear)
							mApp->PlaySample(SOUND_BONUSGRAVES, Pan(aColIdx));
					}
					mShovelX = aColIdx;
					mShovelY = aRowIdx;
				}
			}
		}
		else
		{
			if (mBonusState == 1)
			{
				if (mUpdateCnt - mBonusTick < 0x32)
					return;
			}
			else if (mBonusState != 2)
				return;
			mBonus = false;
			ClearAnims();
			NewLevel();
		}
	}
}

// 0x40ad20
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mMouseX = x;
	mMouseY = y;
	mApp->FollowMouse(mX + x, mY + y);
	if (!mApp->mTutorial)
	{
		int aCellX = PixelToCellX(x);
		int aCellY = PixelToCellY(y);
		// port: the original indexes the 11 x 11 table without a check; the board is 467 pixels wide, so its
		// rightmost strip reached the next field
		if ((aCellX >= 0) && (aCellX < GRID) && (aCellY >= 0) && (aCellY < GRID))
		{
			if (mCellFlash[aCellX][aCellY] == 0)
			{
				ClearCellFlash();
				mCellFlash[aCellX][aCellY] = 1;
			}
		}
	}
	else
	{
		switch (mTutorialStep)
		{
		case 0:
		case 3:
		case 0xb:
		case 0xc:
		case 0xd:
			if ((CellToPixel(8) + 4 <= x) && (x <= CellToPixel(8) + 0x72) && (CellToPixel(2) <= y) &&
				(y <= CellToPixel(2) + 0x27))
			{
				mApp->SetCursor(1);
				return;
			}
			mApp->SetCursor(0);
			return;
		}
	}
	if (mBonus)
	{
		mApp->SetCursor(4);
		return;
	}
	mApp->SetCursor(0);
}

// 0x40ae70
void Board::MouseUp(int x, int y)
{
	Widget::MouseUp(x, y);
}

// 0x40ae90: "Click here to start playing!" ends the tutorial
void Board::ButtonDepress(int theId)
{
	if (theId == 1)
	{
		mApp->mTutorial = false;
		mApp->mDifficulty = 1;
		mApp->EndTutorial();
	}
}
