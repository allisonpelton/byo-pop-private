// Board: the small helpers the rest of the Board is built on (the overlay's colour, the counters, the speed table, the
// lookups in the field).

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "SoundInstance.h"
#include "Image.h"
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
		float aSum = m1bfc + m1bf8;
		m1bf8 = aSum;
		if (5.0f <= aSum)
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
		mOverlay.mFlag80 = false;
		mOverlay.SetFill(mOverlay.mFill.mRed, mOverlay.mFill.mGreen, mOverlay.mFill.mBlue, 0);
		return;
	}

	mOverlay.SetVisible(true);
	mOverlay.SetDisabled(false);
	if (m1924 < 1)
		mOverlay.mFlag80 = true;
}

// 0x434570: the replay recording. The input state (m948, with its second byte (m94c >> 8)) is written when it changes, with
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
		aState = (uchar) (m94c >> 8);
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
	if (mMode != 0 && mMode != 3)
		return 0xf;
	return 0x3f;
}

// one channel of FadeBackdrop: 8 steps towards the target
// port: one channel of Board::FadeBackdrop (0x436b80)
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
		float aFrac = (float) m19b4 / (float) aNext.mTime;

		m1be8 = aPrev.mF5 - (aPrev.mF5 - aNext.mF5) * aFrac;
		m1bec = (aNext.mF4 - aPrev.mF4) * aFrac + aPrev.mF4;
		m19cc = (int) ((float) aPrev.mV1 - (float) (aPrev.mV1 - aNext.mV1) * aFrac);
		mDropTicks = (int) ((float) aPrev.mV3 - (float) (aPrev.mV3 - aNext.mV3) * aFrac);
		m1ac4 = (int) ((float) (aPrev.mV2 - aNext.mV2) * aFrac + (float) aPrev.mV2);
	}

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

// 0x4333c0: one step of a scrolling layer of the scenery. The layer moves when the update count is a multiple of its
// period, faster the more the stage has speed (m938 of the boost, m1c00 of the other mode). The picture it shows
// counts down in mCur[1]; the picture that follows slides in on mNext until it fills the screen. The arithmetic is the
// x87's at 24 bits: every sum and product is a float, and the compares read the rounded values.
void Board::StepScrollLayer(ScrollLayer* theLayer)
{
	if (mUpdateCnt % theLayer->mPeriod != 0)
		return;

	float aSpeed = 0.0f;
	if (m8eb && m938 > 0.0f)
	{
		aSpeed = m938 * 0.02f;
		if (aSpeed > 9.0f)
			aSpeed = 9.0f;
	}
	else if (m917)
	{
		aSpeed = m1c00;
		if (aSpeed > 9.0f)
			aSpeed = 9.0f;
	}

	float aStep;
	if (!m8eb && !m917)
		aStep = theLayer->mSpeed;
	else
		aStep = ((m8eb ? 10.0f : 13.0f) - aSpeed) + theLayer->mSpeed;

	if (theLayer->mCur[1] > 0.0f)
	{
		float aRest = theLayer->mCur[1] - aStep;
		theLayer->mCur[1] = aRest;
		if (aRest <= 0.0f)
		{
			theLayer->mCur[1] = 0.0f;
			theLayer->mF28 = 0.0f;
			theLayer->mNext[3] = 0.0f;
			theLayer->mNext[1] = (float) theLayer->mImage->mHeight;
		}
		return;
	}

	float aHeight = (float) mHeight;
	if (aHeight > theLayer->mF24)
	{
		theLayer->mNext[1] = theLayer->mNext[1] - aStep;
		theLayer->mNext[3] = aStep + theLayer->mNext[3];
		float aPlace = aStep + theLayer->mF24;
		theLayer->mF24 = aPlace;
		if (aPlace >= aHeight)
		{
			memcpy(theLayer->mCur, theLayer->mNext, sizeof(theLayer->mCur));
			theLayer->mF24 = 0.0f;
			theLayer->mNext[3] = 0.0f;
		}
	}
}

// 0x43a1f0: the scenery. The layer at +0x77c and the stars move in the plain game; the planets, the lightning and the
// 100 streaks in the others (m8ed, m8ef, m8f4); the streaks always. The lightning draws from the game's random numbers.
void Board::UpdateScenery()
{
	if (!m8ed && !m8ef && !m8f4)
	{
		StepScrollLayer(&m77c);

		for (size_t i = 0; i < m190.size(); i++)
		{
			bool aSpecial = m917 && (m1bc0 == 1 || m1bc0 == 2);
			StarRec& aStar = m190[i];
			if (m8eb || aSpecial)
			{
				aStar.mSpeed = (float) ((double) aStar.mSpeed + (double) 0.05f);
				if (aSpecial && !(aStar.mSpeed < 4.0f))
					aStar.mSpeed = 4.0f;
			}
			aStar.mY = (float) ((double) aStar.mSpeed + (double) aStar.mY);
			if (aStar.mY > 640.0)
			{
				int aLeast = 0;
				for (size_t j = 0; j < m190.size(); j++)
				{
					if ((double) aLeast > (double) m190[j].mY)
						aLeast = (int) m190[j].mY;
				}
				aStar.mY = (float) (aLeast - 900);
			}
		}
	}
	else
	{
		for (int i = 0; i < 5; i++)
			StepScrollLayer(&m160[i]);

		if (m19f0 == 0)
		{
			if (mUpdateCnt % m19f4 == 0)
			{
				m19f0 = 0xff;
				m19f4 = ((int) AstroRand() % 500000000) % 200 + 100;
				switch (((int) AstroRand() % 500000000) % 3)
				{
				case 0:		m2f8 = IMAGE_WARP_FX_LIGHTENING; break;
				case 1:		m2f8 = mApp->mLightningImage; break;
				default:	m2f8 = mApp->mLightningImage2; break;
				}
				m19f8 = ((int) AstroRand() % 500000000) % 0x15e + 0x32;
				unsigned int aSoundRand = AstroRand();
				unsigned int aColumnRand = AstroRand();
				int aColumn = ((int) aColumnRand % 500000000) % 10;
				int aSound = ((int) aSoundRand % 500000000) % 2;
				mApp->PlaySampleAt(SOUND_THUNDER1 + aSound, aColumn);
			}
		}
		else if (m19f0 > 0)
		{
			m19f0 = m19f0 - 3;
			if (m19f0 < 0)
				m19f0 = 0;
			mOverlay.MarkDirty();
		}
	}

	for (size_t i = 0; i < 100 && i < m170.size(); i++)
	{
		StreakRec& aStreak = m170[i];
		aStreak.mY = (float) ((double) aStreak.mSpeed + (double) aStreak.mY);
		double aHeight = (double) mHeight;
		if (aHeight < (double) aStreak.mY)
		{
			bool aRespawn;
			if (!m8eb && !m8ec && !m8ed && !m917)
				aRespawn = true;
			else
				aRespawn = (double) (aStreak.mMargin + mHeight) <= (double) aStreak.mY;
			if (aRespawn)
				aStreak.mY = (float) (((int) AstroRand() % 500000000) % 0x65 - 100);
		}
	}
}

// 0x432f40: the state of the game goes into the player's profile (so a game that is left can be continued)
void Board::StoreProgress()
{
	UserProfile* aUser = mApp->mUser;
	aUser->SetInt(0x66, m198c);
	aUser->SetInt(0x4f, mField193C);
	aUser->SetInt(0x53, mMode);
	aUser->SetFloat(0x52, m940);
	aUser->SetFloat(0x50, m1bd8);
	aUser->SetInt(0x51, mScore);
	aUser->SetInt(0x54, m19b8);
	aUser->SetInt(0x55, m1b1c);
	aUser->SetInt(0x63, m1b2c);
}

// 0x43b9c0: the klaxon plays while the field has no room left for the next piece
void Board::UpdateKlaxon()
{
	if (!m910 && !m917 && !m8ec && !m8eb && !m8ed && !m8ef && !m8f4 && !m901)
	{
		SoundInstance* aKlaxon = mApp->mLoopSounds[7];
		if (!NoRoomLeft())
		{
			if (aKlaxon->IsPlaying())
				aKlaxon->Stop();
		}
		else
		{
			if (!aKlaxon->IsPlaying())
				aKlaxon->Play(true, false);
		}
	}
}

// 0x43d2f0: nothing moves or waits: no falling bricks, no groups, no cells marked, no mode that runs on its own
bool Board::IsSettled(bool theAssumeIdle)
{
	bool aIdle = theAssumeIdle ? true : (m1a34 == 0);
	if (!m8eb && !m8ed && !m8ef && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0 && m1a60 != 2 && aIdle &&
		!m910 && m928 == 1.0f && m1990 == 0 && m94.empty() && m7c.empty())
	{
		if (CountMovingBricks(false) == 0 && m1a0.empty() && ma0.empty() && mf8.empty())
			return true;
	}
	return false;
}

// 0x43d8b0: delete the bricks in the set and empty it
void Board::DeleteDoomedBricks()
{
	std::vector<Brick*> aBricks(m128.begin(), m128.end());
	for (size_t i = 0; i < aBricks.size(); i++)
	{
		aBricks[i]->mGone = true;
		delete aBricks[i];
	}
	m128.clear();
}

// 0x4332b0: a new plain brick of a type (the column 0x19 puts its sounds in the middle)
Brick* Board::CreateBrick(int theType)
{
	Brick* aBrick = new Brick(this, mBus, 0x19);
	aBrick->SetType(theType);
	return aBrick;
}

// 0x433320: a brick of type 0 that starts to mutate
Brick* Board::CreateMutator()
{
	Brick* aBrick = CreateBrick(0);
	aBrick->StartMutation();
	return aBrick;
}

// 0x433340
Brick* Board::CreateFuelCell(int theColumn, int theRow)
{
	FuelCell* aCell = new FuelCell(this, mBus);
	aCell->SetPlace(theColumn, theRow);
	aCell->SetType(8);
	return aCell;
}

// 0x4393d0: a radial bomb (6), stone (7), steel (9) or row nuke (0xb); the bomb and the nuke start their counters
SpecialBrick* Board::CreateSpecialBrick(int theType)
{
	SpecialBrick* aBrick = new SpecialBrick(this, mBus, ((theType != 6) ? 0 : -0x4b) + 0x7d);
	aBrick->SetType(theType);
	if (theType == 6)
		m1f0[1] = 2000;
	else if (theType == 0xb)
		m1f0[2] = 2000;
	return aBrick;
}

// 0x439480: freeze (10), column nuke (0xd) or booster (0xe)
Brick* Board::CreatePowerBrick(int theType)
{
	Brick* aBrick = new PowerBrick(this, mBus, 0x4b);
	aBrick->SetType(theType);
	if (theType != 0xe)
		m1f0[3 + (theType != 10 ? 1 : 0)] = 2000;
	return aBrick;
}

// 0x43a680: the picture of the stage (or of a warp that is running) fades in and out, two values swinging like a
// PulsePair, a third one fading the picture away
void Board::UpdateStageBackground()
{
	if (!((unsigned int) mField193C < m220.size() || m90a || (!m230.empty() && m230[0]->m1c != 0)))
		return;

	StageBg* aBg;
	if (m230.empty() || m230[0]->m1c == 0)
		aBg = m220[m90a ? 1 : mField193C];
	else
		aBg = m230[0]->mBg;

	if (aBg->mf0 != 0)
		return;

	aBg->m70.Update();
	aBg->mb0.Update();

	if (aBg->m10c != 0)
	{
		aBg->m100 += aBg->m10c;
		if (aBg->m100 < 0)
		{
			aBg->m10c = 0;
			aBg->m100 = 0;
		}
		if (aBg->m100 > 0xff)
		{
			aBg->m10c = 0;
			aBg->m100 = 0xff;
		}
	}

	if (aBg->m118 != 0)
	{
		aBg->m114 += aBg->m118;
		if (aBg->m114 < 0)
		{
			aBg->m118 = 0;
			aBg->m114 = 0;
			aBg->mf0 = 1;
		}
		if (aBg->m114 > 0xff)
		{
			aBg->m118 = 0;
			aBg->m114 = 0xff;
		}
	}

	if (aBg->m5c != 0)
	{
		if (aBg->m110 != 0)
		{
			aBg->m104 += aBg->m110;
			if (aBg->m104 < 0)
			{
				aBg->m110 = 0;
				aBg->m104 = 0;
			}
			if (aBg->m104 > 0xff)
			{
				aBg->m110 = 0;
				aBg->m104 = 0xff;
			}
		}

		int aWait = aBg->m108;
		if (aWait < 1)
		{
			if ((aBg->m100 == 0 || aBg->m100 == 0xff) && (aBg->m104 == 0 || aBg->m104 == 0xff) &&
				aBg->m10c == 0 && aBg->m110 == 0 && aWait == 0)
				aBg->m108 = 200;
		}
		else
		{
			aBg->m108 = aWait - 1;
			if (aWait - 1 < 1)
			{
				aBg->m108 = 0;
				aBg->m10c = ((((aBg->m100 != 0xff) ? 1 : 0) - 1) & -16) + 8;
				aBg->m110 = ((((aBg->m104 != 0xff) ? 1 : 0) - 1) & -16) + 8;
			}
		}
	}

	MarkDirtyFull();
}
