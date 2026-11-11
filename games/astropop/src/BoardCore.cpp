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
	mEmergencyPause = false;
	mShownWhileRunning = false;
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
		mShipFlameFrame++;
		if (mShipFlameFrame > 7)
			mShipFlameFrame = 0;
	}

	if (mShipTilt != 2 && aPhase == 0)
	{
		mShipFrame++;
		if (mShipFrame > 3)
		{
			mShipFrame = 2;
			mShipTilt = 2;
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
		mOverlay.mShownWhileRunning = false;
		mOverlay.SetFill(mOverlay.mFill.mRed, mOverlay.mFill.mGreen, mOverlay.mFill.mBlue, 0);
		return;
	}

	mOverlay.SetVisible(true);
	mOverlay.SetDisabled(false);
	if (mPauseCount < 1)
		mOverlay.mShownWhileRunning = true;
}

// 0x434570: the replay recording. The input state (mDemoRecordAction, with its second byte (mDemoRecordArg >> 8)) is written when it changes, with
// the number of updates it lasted (a run of 255s and a remainder).
void Board::RecordStep(bool theForce)
{
	if (theForce && mDemoRecordAction == 0)
	{
		mDemoRecordLastAction = 0;
		mDemoRecordLastArg = mDemoRecordArg;
		mDemoRecordAction = 0xff;
	}

	if (mDemoRecordLastAction == mDemoRecordAction)
		mDemoRecordLastArg = mDemoRecordArg;
	else if (mDemoRecordLastAction == 0)
	{
		mDemoRecordBuffer.WriteByte(0);
		unsigned int aRuns = mDemoRecordLastArg / 0xff;
		char aCount = (char) aRuns;
		uchar aLast;
		if (aCount == 0 || mDemoRecordLastArg < 0x100)
		{
			mDemoRecordBuffer.WriteByte(1);
			aLast = (uchar) mDemoRecordLastArg;
		}
		else
		{
			mDemoRecordBuffer.WriteByte((uchar) (aCount + 1));
			for (aRuns = aRuns & 0xff; aRuns != 0; aRuns--)
				mDemoRecordBuffer.WriteByte(0xff);
			aLast = (uchar) (mDemoRecordLastArg + aCount);
		}
		mDemoRecordBuffer.WriteByte(aLast);
	}

	uchar aState = mDemoRecordAction;
	if (aState == 2 || aState == 1)
	{
		mDemoRecordBuffer.WriteByte(aState);
		aState = (uchar) (mDemoRecordArg >> 8);
		mDemoRecordBuffer.WriteByte((uchar) (mDemoRecordArg & 7));
	}
	else if (aState == 0)
	{
		mDemoRecordLastAction = mDemoRecordAction;
		mDemoRecordLastArg = mDemoRecordArg;
		return;
	}
	mDemoRecordBuffer.WriteByte(aState);

	mDemoRecordLastAction = mDemoRecordAction;
	mDemoRecordLastArg = mDemoRecordArg;
}

// 0x432a40: the number of updates until the next brick comes (-1: no delay). mPhase is the game's phase.
int Board::PickSpawnDelay()
{
	switch (mPhase)
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
	if (!mHeldBricks.empty())
	{
		Brick* aBrick = mHeldBricks.front();
		aRed = aBrick->mColor2.mRed;
		aGreen = aBrick->mColor2.mGreen;
		aBlue = aBrick->mColor2.mBlue;
	}

	StepToward(mBackdropTint.mRed, aRed);
	StepToward(mBackdropTint.mGreen, aGreen);
	StepToward(mBackdropTint.mBlue, aBlue);

	if (mBackdropTint.mRed == aRed && mBackdropTint.mGreen == aGreen && mBackdropTint.mBlue == aBlue)
		mBackdropTintChanging = false;
}

// 0x437a50: the speed table. The time played (mRampTime) picks the row it lies in; the values of the game's speed are
// mixed from the row before and the row it lies in. Every 6000 updates the stage goes up by one.
void Board::UpdateRamp()
{
	size_t aIndex = 0;
	while (aIndex < mSpeedRamp.size())
	{
		if (mRampTime < mSpeedRamp[aIndex].mTime)
			break;
		aIndex++;
	}

	if (aIndex < mSpeedRamp.size())
	{
		if (mRampTime >= 18000)
			mExtraColors = 1;

		const RampRecord& aNext = mSpeedRamp[aIndex];
		const RampRecord& aPrev = mSpeedRamp[aIndex > 0 ? aIndex - 1 : 0];
		float aFrac = (float) mRampTime / (float) aNext.mTime;

		m1be8 = aPrev.mF5 - (aPrev.mF5 - aNext.mF5) * aFrac;
		m1bec = (aNext.mF4 - aPrev.mF4) * aFrac + aPrev.mF4;
		mRowInterval = (int) ((float) aPrev.mV1 - (float) (aPrev.mV1 - aNext.mV1) * aFrac);
		mDropTicks = (int) ((float) aPrev.mV3 - (float) (aPrev.mV3 - aNext.mV3) * aFrac);
		mMaxMutators = (int) ((float) (aPrev.mV2 - aNext.mV2) * aFrac + (float) aPrev.mV2);
	}

	int aStage = mRampTime / 6000;
	mStageNumber = aStage + 1;
	mScoreMultiplier = (float) ((double) (aStage * 3) + 1.0);
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
		Brick* aBrick = mFieldBricks[aRow][theColumn];
		if (aBrick != NULL && aBrick->IsIdle())
			break;
		aRow--;
		if (aRow < 0)
			return -1;
	}
	return aRow;
}

// 0x438b60: true when the field has no room for the next piece. (The column of the piece at mShipX is left out of the
// count of full columns.)
bool Board::NoRoomLeft()
{
	int aLimit = 0x1a7 - (int) mHeldBricks.size() * 0x19;
	int aThreshold;
	if (FindBottomRow(mShipX, true) + 2 > 0)
		aThreshold = FindBottomRow(mShipX, true) * 0x19 + 0x3b;
	else
		aThreshold = 4;

	if (aLimit > aThreshold)
	{
		if (FindBottomRow(mShipX, true) < 0x13)
		{
			int aPieceColumn = (mShipX - 0xa6) / 0x2c;
			for (int aCol = 0; aCol < 10; aCol++)
			{
				if (aPieceColumn == aCol)
					continue;
				int aFilled = 0;
				for (size_t aRow = 0; aRow < mFieldBricks.size() && mFieldBricks[aRow][aCol] != NULL; aRow++)
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
	for (std::list<std::list<Brick*> >::iterator aGroup = mFiredGroups.begin(); aGroup != mFiredGroups.end(); ++aGroup)
	{
		for (std::list<Brick*>::iterator aBrickIt = aGroup->begin(); aBrickIt != aGroup->end(); ++aBrickIt)
		{
			Brick* aBrick = *aBrickIt;
			if (aBrick->mY != aBrick->mTargetY || aBrick->mState != 1)
			{
				if (!theStrict || !aBrick->mSupported || aBrick->mY > aBrick->mTargetY)
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
// period, faster the more the stage has speed (mWarpShipY of the boost, m1c00 of the other mode). The picture it shows
// counts down in mCur[1]; the picture that follows slides in on mNext until it fills the screen. The arithmetic is the
// x87's at 24 bits: every sum and product is a float, and the compares read the rounded values.
void Board::StepScrollLayer(ScrollLayer* theLayer)
{
	if (mUpdateCnt % theLayer->mPeriod != 0)
		return;

	float aSpeed = 0.0f;
	if (mWarpActive && mWarpShipY > 0.0f)
	{
		aSpeed = mWarpShipY * 0.02f;
		if (aSpeed > 9.0f)
			aSpeed = 9.0f;
	}
	else if (mBonusWarpIn)
	{
		aSpeed = m1c00;
		if (aSpeed > 9.0f)
			aSpeed = 9.0f;
	}

	float aStep;
	if (!mWarpActive && !mBonusWarpIn)
		aStep = theLayer->mSpeed;
	else
		aStep = ((mWarpActive ? 10.0f : 13.0f) - aSpeed) + theLayer->mSpeed;

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
// 100 streaks in the others (mWarpFlight, mAtWarpGate, mWarpGatePicked); the streaks always. The lightning draws from the game's random numbers.
void Board::UpdateScenery()
{
	if (!mWarpFlight && !mAtWarpGate && !mWarpGatePicked)
	{
		StepScrollLayer(&mSceneryLayer);

		for (size_t i = 0; i < mBackdropStars.size(); i++)
		{
			bool aSpecial = mBonusWarpIn && (m1bc0 == 1 || m1bc0 == 2);
			StarRec& aStar = mBackdropStars[i];
			if (mWarpActive || aSpecial)
			{
				aStar.mSpeed = (float) ((double) aStar.mSpeed + (double) 0.05f);
				if (aSpecial && !(aStar.mSpeed < 4.0f))
					aStar.mSpeed = 4.0f;
			}
			aStar.mY = (float) ((double) aStar.mSpeed + (double) aStar.mY);
			if (aStar.mY > 640.0)
			{
				int aLeast = 0;
				for (size_t j = 0; j < mBackdropStars.size(); j++)
				{
					if ((double) aLeast > (double) mBackdropStars[j].mY)
						aLeast = (int) mBackdropStars[j].mY;
				}
				aStar.mY = (float) (aLeast - 900);
			}
		}
	}
	else
	{
		for (int i = 0; i < 5; i++)
			StepScrollLayer(&mPlanets[i]);

		if (mWarpLightningAlpha == 0)
		{
			if (mUpdateCnt % mWarpLightningPeriod == 0)
			{
				mWarpLightningAlpha = 0xff;
				mWarpLightningPeriod = ((int) AstroRand() % 500000000) % 200 + 100;
				switch (((int) AstroRand() % 500000000) % 3)
				{
				case 0:		mWarpLightningImage = IMAGE_WARP_FX_LIGHTENING; break;
				case 1:		mWarpLightningImage = mApp->mLightningImage; break;
				default:	mWarpLightningImage = mApp->mLightningImage2; break;
				}
				mWarpLightningY = ((int) AstroRand() % 500000000) % 0x15e + 0x32;
				unsigned int aSoundRand = AstroRand();
				unsigned int aColumnRand = AstroRand();
				int aColumn = ((int) aColumnRand % 500000000) % 10;
				int aSound = ((int) aSoundRand % 500000000) % 2;
				mApp->PlaySampleAt(SOUND_THUNDER1 + aSound, aColumn);
			}
		}
		else if (mWarpLightningAlpha > 0)
		{
			mWarpLightningAlpha = mWarpLightningAlpha - 3;
			if (mWarpLightningAlpha < 0)
				mWarpLightningAlpha = 0;
			mOverlay.MarkDirty();
		}
	}

	for (size_t i = 0; i < 100 && i < mStreaks.size(); i++)
	{
		StreakRec& aStreak = mStreaks[i];
		aStreak.mY = (float) ((double) aStreak.mSpeed + (double) aStreak.mY);
		double aHeight = (double) mHeight;
		if (aHeight < (double) aStreak.mY)
		{
			bool aRespawn;
			if (!mWarpActive && !mWarpCharging && !mWarpFlight && !mBonusWarpIn)
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
	aUser->SetInt(0x66, mShipX);
	aUser->SetInt(0x4f, mStageNumber);
	aUser->SetInt(0x53, mMode);
	aUser->SetFloat(0x52, mScoreMultiplier);
	aUser->SetFloat(0x50, m1bd8);
	aUser->SetInt(0x51, mScore);
	aUser->SetInt(0x54, mPlayTime);
	aUser->SetInt(0x55, m1b1c);
	aUser->SetInt(0x63, m1b2c);
}

// 0x43b9c0: the klaxon plays while the field has no room left for the next piece
void Board::UpdateKlaxon()
{
	if (!mBonusRoundOn && !mBonusWarpIn && !mWarpCharging && !mWarpActive && !mWarpFlight && !mAtWarpGate && !mWarpGatePicked && !mScreenWiping)
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
	bool aIdle = theAssumeIdle ? true : (mPowerupActive == 0);
	if (!mWarpActive && !mWarpFlight && !mAtWarpGate && !mWarpGatePicked && !mWarpCharging && !mLevelUpPhase && !mGameOver && mPauseCount == 0 && mGameType != 2 && aIdle &&
		!mBonusRoundOn && mBrickometerFill == 1.0f && mHeldBricksNotLanded == 0 && mFallingGroups.empty() && mFiredGroups.empty())
	{
		if (CountMovingBricks(false) == 0 && mAtomicMarkedBricks.empty() && mTakenBricks.empty() && mKillAllTypes.empty())
			return true;
	}
	return false;
}

// 0x43d8b0: delete the bricks in the set and empty it
void Board::DeleteDoomedBricks()
{
	std::vector<Brick*> aBricks(mDoomedBricks.begin(), mDoomedBricks.end());
	for (size_t i = 0; i < aBricks.size(); i++)
	{
		aBricks[i]->mGone = true;
		delete aBricks[i];
	}
	mDoomedBricks.clear();
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
		mSpecialCooldowns[1] = 2000;
	else if (theType == 0xb)
		mSpecialCooldowns[2] = 2000;
	return aBrick;
}

// 0x439480: freeze (10), column nuke (0xd) or booster (0xe)
Brick* Board::CreatePowerBrick(int theType)
{
	Brick* aBrick = new PowerBrick(this, mBus, 0x4b);
	aBrick->SetType(theType);
	if (theType != 0xe)
		mSpecialCooldowns[3 + (theType != 10 ? 1 : 0)] = 2000;
	return aBrick;
}

// 0x43a680: the picture of the stage (or of a warp that is running) fades in and out, two values swinging like a
// PulsePair, a third one fading the picture away
void Board::UpdateStageBackground()
{
	if (!((unsigned int) mStageNumber < mStageHintPanels.size() || mTutorialStageForced || (!mHintSpots.empty() && mHintSpots[0]->mActive != 0)))
		return;

	StageBg* aBg;
	if (mHintSpots.empty() || mHintSpots[0]->mActive == 0)
		aBg = mStageHintPanels[mTutorialStageForced ? 1 : mStageNumber];
	else
		aBg = mHintSpots[0]->mBg;

	if (aBg->mFadedOut != 0)
		return;

	aBg->mPicAnim.Update();
	aBg->mOverPicAnim.Update();

	if (aBg->mPicAlphaStep != 0)
	{
		aBg->mPicAlpha += aBg->mPicAlphaStep;
		if (aBg->mPicAlpha < 0)
		{
			aBg->mPicAlphaStep = 0;
			aBg->mPicAlpha = 0;
		}
		if (aBg->mPicAlpha > 0xff)
		{
			aBg->mPicAlphaStep = 0;
			aBg->mPicAlpha = 0xff;
		}
	}

	if (aBg->mPanelAlphaStep != 0)
	{
		aBg->mPanelAlpha += aBg->mPanelAlphaStep;
		if (aBg->mPanelAlpha < 0)
		{
			aBg->mPanelAlphaStep = 0;
			aBg->mPanelAlpha = 0;
			aBg->mFadedOut = 1;
		}
		if (aBg->mPanelAlpha > 0xff)
		{
			aBg->mPanelAlphaStep = 0;
			aBg->mPanelAlpha = 0xff;
		}
	}

	if (aBg->mOverPic != 0)
	{
		if (aBg->mOverPicAlphaStep != 0)
		{
			aBg->mOverPicAlpha += aBg->mOverPicAlphaStep;
			if (aBg->mOverPicAlpha < 0)
			{
				aBg->mOverPicAlphaStep = 0;
				aBg->mOverPicAlpha = 0;
			}
			if (aBg->mOverPicAlpha > 0xff)
			{
				aBg->mOverPicAlphaStep = 0;
				aBg->mOverPicAlpha = 0xff;
			}
		}

		int aWait = aBg->mCrossfadeTimer;
		if (aWait < 1)
		{
			if ((aBg->mPicAlpha == 0 || aBg->mPicAlpha == 0xff) && (aBg->mOverPicAlpha == 0 || aBg->mOverPicAlpha == 0xff) &&
				aBg->mPicAlphaStep == 0 && aBg->mOverPicAlphaStep == 0 && aWait == 0)
				aBg->mCrossfadeTimer = 200;
		}
		else
		{
			aBg->mCrossfadeTimer = aWait - 1;
			if (aWait - 1 < 1)
			{
				aBg->mCrossfadeTimer = 0;
				aBg->mPicAlphaStep = ((((aBg->mPicAlpha != 0xff) ? 1 : 0) - 1) & -16) + 8;
				aBg->mOverPicAlphaStep = ((((aBg->mOverPicAlpha != 0xff) ? 1 : 0) - 1) & -16) + 8;
			}
		}
	}

	MarkDirtyFull();
}
