// Board: the small helpers the rest of the Board is built on (the overlay's colour, the counters, the speed table, the
// lookups in the field).

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "Res.h"

using namespace Sexy;

//////////////////////////////////////////////////////////////////////////
// BoardOverlay

BoardOverlay::BoardOverlay()        // 0x4327c0
{
	mBoard = NULL;
	mFill = Color(0xff, 0xff, 0xff, 0);
	mColor58 = mFill;
	mHasAlpha = true;
	mHasTransparencies = true;
	mFlag78 = false;
	mFlag80 = false;
	mMouseVisible = false;
}

void BoardOverlay::SetFill(int theRed, int theGreen, int theBlue, int theAlpha)     // 0x432990
{
	mFill.mRed = theRed;
	mFill.mGreen = theGreen;
	mFill.mBlue = theBlue;
	mFill.mAlpha = theAlpha;
}

//////////////////////////////////////////////////////////////////////////
// PulsePair

// 0x433850: two values that swing between 0 and 255; when both rest and the wait is over they start the next swing
// (each one the other way from where it stands)
void PulsePair::Update(int theSpeed)
{
	if (mDeltaA != 0)
	{
		mA += mDeltaA;
		if (mA < 0)
		{
			mA = 0;
			mDeltaA = 0;
		}
		else if (mA > 0xff)
		{
			mA = 0xff;
			mDeltaA = 0;
		}
	}

	if (mDeltaB != 0)
	{
		mB += mDeltaB;
		if (mB < 0)
		{
			mDeltaB = 0;
			mB = 0;
		}
		if (mB > 0xff)
		{
			mDeltaB = 0;
			mB = 0xff;
		}
	}

	int aWait = mWait;
	if (aWait < 1)
	{
		if ((mA == 0 || mA == 0xff) && (mB == 0 || mB == 0xff) && mDeltaA == 0 && mDeltaB == 0 && aWait == 0)
			mWait = mWaitReset;
	}
	else
	{
		mWait = aWait - 1;
		if (aWait - 1 < 1)
		{
			mWait = 0;
			mDeltaA = (mA == 0xff) ? -theSpeed : theSpeed;
			if (mB == 0xff)
				theSpeed = -theSpeed;
			mDeltaB = theSpeed;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Board

// 0x433500: the counters that step with the update count (a frame number of eight, the intro of four, the swing of a value)
void Board::UpdateAnimCounters()
{
	int aPhase = mUpdateCnt % 10;
	if (aPhase == 0)
	{
		m1950++;
		if (m1950 > 7)
			m1950 = 0;
	}

	if (m194c != 2 && aPhase == 0)
	{
		m1954++;
		if (m1954 > 3)
		{
			m1954 = 2;
			m194c = 2;
		}
	}

	if (0.0f <= m1bf8)
	{
		double aSum = (double) m1bfc + (double) m1bf8;
		m1bf8 = (float) aSum;
		if (5.0 <= aSum)
		{
			m1bf8 = 5.0f;
			m1bfc = -m1bfc;
			return;
		}
		if (aSum < 0.0)
		{
			m1bf8 = -1.0f;
			m1bfc = -m1bfc;
		}
	}
}

// 0x433c30: hide the overlay (fully clear, not clickable), or bring it back
void Board::SetOverlayHidden(bool theHide)
{
	if (theHide)
	{
		mOverlay.SetVisible(false);
		mOverlay.SetDisabled(true);
		m6e0 = false;
		mOverlay.SetFill(m6c8.mRed, m6c8.mGreen, m6c8.mBlue, 0);
		return;
	}

	mOverlay.SetVisible(true);
	mOverlay.SetDisabled(false);
	if (m1924 < 1)
		m6e0 = true;
}

// 0x434570: the replay recording. The input state (m948, with its second byte m94d) is written when it changes, with
// the number of updates it lasted (a run of 255s and a remainder).
void Board::RecordStep(bool theForce)
{
	if (theForce && m948 == 0)
	{
		m949 = 0;
		m950 = m94c;
		m948 = 0xff;
	}

	if (m949 == m948)
		m950 = m94c;
	else if (m949 == 0)
	{
		m758.WriteByte(0);
		unsigned int aRuns = m950 / 0xff;
		char aCount = (char) aRuns;
		uchar aLast;
		if (aCount == 0 || m950 < 0x100)
		{
			m758.WriteByte(1);
			aLast = (uchar) m950;
		}
		else
		{
			m758.WriteByte((uchar) (aCount + 1));
			for (aRuns = aRuns & 0xff; aRuns != 0; aRuns--)
				m758.WriteByte(0xff);
			aLast = (uchar) (m950 + aCount);
		}
		m758.WriteByte(aLast);
	}

	uchar aState = m948;
	if (aState == 2 || aState == 1)
	{
		m758.WriteByte(aState);
		aState = m94d;
		m758.WriteByte((uchar) (m94c & 7));
	}
	else if (aState == 0)
	{
		m949 = m948;
		m950 = m94c;
		return;
	}
	m758.WriteByte(aState);

	m949 = m948;
	m950 = m94c;
}

// 0x432a40: the number of updates until the next brick comes (-1: no delay). m1ab8 is the game's phase.
int Board::PickSpawnDelay()
{
	switch (m1ab8)
	{
	case 2:
		return ((int) AstroRand() % 500000000) % 0x19 + 0x19;
	case 3:
	case 5:
		return ((int) AstroRand() % 500000000) % 100 + 100;
	case 4:
		return ((int) AstroRand() % 500000000) % 400 + 100;
	case 6:
	case 7:
		break;
	case 8:
	case 9:
		return 0x19;
	case 10:
	case 0xb:
	case 0xc:
	case 0xd:
		return 10;
	default:
		return -1;
	}
	if (m1a38 != 0 && m1a38 != 3)
		return 0xf;
	return 0x3f;
}

// one channel of FadeBackdrop: 8 steps towards the target
static void StepToward(int& theValue, int theTarget)
{
	if (theTarget < theValue)
	{
		theValue -= 8;
		if (theValue < theTarget)
			theValue = theTarget;
	}
	else if (theValue < theTarget)
	{
		theValue += 8;
		if (theTarget < theValue)
			theValue = theTarget;
	}
}

// 0x436b80: the tint of the backdrop moves to the colour of the first falling brick (grey when none falls)
void Board::FadeBackdrop()
{
	int aRed = 100, aGreen = 100, aBlue = 100;
	if (!m70.empty())
	{
		Brick* aBrick = m70.front();
		aRed = aBrick->mColor2.mRed;
		aGreen = aBrick->mColor2.mGreen;
		aBlue = aBrick->mColor2.mBlue;
	}

	StepToward(m3c0.mRed, aRed);
	StepToward(m3c0.mGreen, aGreen);
	StepToward(m3c0.mBlue, aBlue);

	if (m3c0.mRed == aRed && m3c0.mGreen == aGreen && m3c0.mBlue == aBlue)
		m8e6 = false;
}

// 0x437a50: the speed table. The time played (m19b4) picks the row it lies in; the values of the game's speed are
// mixed from the row before and the row it lies in. Every 6000 updates the stage goes up by one.
void Board::UpdateRamp()
{
	size_t aIndex = 0;
	while (aIndex < m280.size())
	{
		if (m19b4 < m280[aIndex].mTime)
			break;
		aIndex++;
	}

	if (aIndex < m280.size())
	{
		if (m19b4 >= 18000)
			m1a28 = 1;

		const RampRecord& aNext = m280[aIndex];
		const RampRecord& aPrev = m280[aIndex > 0 ? aIndex - 1 : 0];
		double aFrac = (double) m19b4 / (double) aNext.mTime;

		m1be8 = (float) ((double) aPrev.mF5 - ((double) aPrev.mF5 - (double) aNext.mF5) * aFrac);
		m1bec = (float) (((double) aNext.mF4 - (double) aPrev.mF4) * aFrac + (double) aPrev.mF4);
		m19cc = (int) ((double) aPrev.mV1 - (double) (aPrev.mV1 - aNext.mV1) * aFrac);
		mDropTicks = (int) ((double) aPrev.mV3 - (double) (aPrev.mV3 - aNext.mV3) * aFrac);
		m1ac4 = (int) ((double) (aPrev.mV2 - aNext.mV2) * aFrac + (double) aPrev.mV2);
	}

	if (m19b4 >= 6000)
		mApp->mFlag65b = true;
	int aStage = m19b4 / 6000;
	mField193C = aStage + 1;
	m940 = (float) ((double) (aStage * 3) + 1.0);
}

// 0x437590: the row of the lowest idle brick in a column (18 is the bottom row), -1 when there is none. The column is
// a number 0 .. 9, or an x position of the screen when theIsX (166 + 44 per column).
int Board::FindBottomRow(int theColumn, bool theIsX)
{
	if (!theIsX)
	{
		if (theColumn < 0 || theColumn > 9)
			return -1;
	}
	else
	{
		if (theColumn < 0xa6 || theColumn > 0x25d)
			return -1;
		theColumn = (theColumn - 0xa6) / 0x2c;
	}

	int aRow = 0x12;
	for (;;)
	{
		Brick* aBrick = m314[aRow][theColumn];
		if (aBrick != NULL && aBrick->IsIdle())
			break;
		aRow--;
		if (aRow < 0)
			return -1;
	}
	return aRow;
}

// 0x438b60: true when the field has no room for the next piece. (The column of the piece at m198c is left out of the
// count of full columns.)
bool Board::NoRoomLeft()
{
	int aLimit = 0x1a7 - (int) m70.size() * 0x19;
	int aThreshold;
	if (FindBottomRow(m198c, true) + 2 > 0)
		aThreshold = FindBottomRow(m198c, true) * 0x19 + 0x3b;
	else
		aThreshold = 4;

	if (aLimit > aThreshold)
	{
		if (FindBottomRow(m198c, true) < 0x13)
		{
			int aPieceColumn = (m198c - 0xa6) / 0x2c;
			for (int aCol = 0; aCol < 10; aCol++)
			{
				if (aPieceColumn == aCol)
					continue;
				int aFilled = 0;
				for (size_t aRow = 0; aRow < m314.size() && m314[aRow][aCol] != NULL; aRow++)
					aFilled++;
				if (aFilled + 1 >= 0x13)
					return true;
			}
			return false;
		}
	}
	return true;
}

// 0x439b70: how many bricks of the falling groups are on their way (not at their target in state 1). With theStrict
// a brick with the flag at +0x188 counts only when it is below its target.
int Board::CountMovingBricks(bool theStrict)
{
	int aCount = 0;
	for (std::list<std::list<Brick*> >::iterator aGroup = m7c.begin(); aGroup != m7c.end(); ++aGroup)
	{
		for (std::list<Brick*>::iterator aBrickIt = aGroup->begin(); aBrickIt != aGroup->end(); ++aBrickIt)
		{
			Brick* aBrick = *aBrickIt;
			if (aBrick->mY != aBrick->mTargetY || aBrick->mState != 1)
			{
				if (!theStrict || !aBrick->mFlag188 || aBrick->mY > aBrick->mTargetY)
					aCount++;
			}
		}
	}
	return aCount;
}

// 0x438d60
bool Board::HasCell(int theCol, int theRow, const std::vector<CellRef>& theCells)
{
	for (size_t i = 0; i < theCells.size(); i++)
	{
		if (theCells[i].mCol == theCol && theCells[i].mRow == theRow)
			return true;
	}
	return false;
}

// 0x438db0
bool Board::HasCell3(int theCol, int theRow, const std::vector<CellRef3>& theCells)
{
	for (size_t i = 0; i < theCells.size(); i++)
	{
		if (theCells[i].mCol == theCol && theCells[i].mRow == theRow)
			return true;
	}
	return false;
}
