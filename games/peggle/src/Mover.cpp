// Mover.cpp: see Mover.h. The original computes with the x87 unit: its sines and cosines take a double, and each result is
// stored as a float where the code stores one; the same steps are taken here.

#include "Mover.h"

#include "DataSync.h"
#include "ThunderMath.h"

#include <cmath>

using namespace Sexy;

static const int DEFAULT_PERIOD = 200; // 0x47a980

// 0x47b430
Mover::Mover()
{
	Reset();
}

// 0x47a980
void Mover::Reset()
{
	mPhase = 0;
	mPathRotation = 0;
	mPostDelayPhase = 0;
	mType = 0;
	mMaxAngle = 0;
	mOffset = 0;
	mAngle = 0;
	mRadius = 0;
	mRotation = 0;
	mPeriod = DEFAULT_PERIOD;
	mY = 0;
	mRadius2 = 0;
	mX = 0;
	mPause1 = 0;
	mCenterY = 0;
	mPause2 = 0;
	mCenterX = 0;
	mPhase1 = 0;
	mPhase2 = 0;
	mFollowedX = 0;
	mFollowedY = 0;
	mFollowed = nullptr;
}

// 0x47a9f0
void Mover::SyncState(DataSync& theSync)
{
	if (theSync.IsReading())
		Reset();

	theSync.SyncSignedByte(mType);
	theSync.SyncFloat(mCenterX);
	theSync.SyncFloat(mCenterY);
	theSync.SyncShort(mPeriod);

	// what differs from the default is written; the rest is left out
	bool aHasOffset = mOffset != 0;
	bool aHasRadius = mRadius != 0;
	bool aHasPhase = mPhase != 0;
	bool aHasPathRotation = mPathRotation != 0;
	bool aHasRadius2 = mRadius2 != 0;
	bool aHasPause1 = mPause1 != 0;
	bool aHasPause2 = mPause2 != 0;
	bool aHasPhase1 = mPhase1 != 0;
	bool aHasPhase2 = mPhase2 != 0;
	bool aHasPostDelayPhase = mPostDelayPhase != 0;
	bool aHasMaxAngle = mMaxAngle != 0;
	bool aHasAngle = mAngle != 0;
	bool aHasFollowed = mFollowed != nullptr;
	bool aHasPosition = mX != mCenterX || mY != mCenterY;
	bool aHasRotation = mRotation != 0;

	theSync.SyncBoolBit(aHasOffset);
	theSync.SyncBoolBit(aHasRadius);
	theSync.SyncBoolBit(aHasPhase);
	theSync.SyncBoolBit(aHasPathRotation);
	theSync.SyncBoolBit(aHasRadius2);
	theSync.SyncBoolBit(aHasPause1);
	theSync.SyncBoolBit(aHasPause2);
	theSync.SyncBoolBit(aHasPhase1);
	theSync.EndBit();
	theSync.SyncBoolBit(aHasPhase2);
	theSync.SyncBoolBit(aHasPostDelayPhase);
	theSync.SyncBoolBit(aHasMaxAngle);
	theSync.SyncBoolBit(aHasAngle);
	theSync.SyncBoolBit(aHasFollowed);
	theSync.SyncBoolBit(aHasPosition);
	theSync.SyncBoolBit(aHasRotation);
	theSync.EndBit();

	if (aHasOffset)
		theSync.SyncSignedShort(mOffset);
	if (aHasRadius)
		theSync.SyncSignedShort(mRadius);
	if (aHasPhase)
		theSync.SyncFloat(mPhase);
	if (aHasPathRotation)
		theSync.SyncFloat(mPathRotation);
	if (aHasRadius2)
		theSync.SyncSignedShort(mRadius2);
	if (aHasPause1)
		theSync.SyncShort(mPause1);
	if (aHasPause2)
		theSync.SyncShort(mPause2);
	if (aHasPhase1)
		theSync.SyncSignedByte(mPhase1);
	if (aHasPhase2)
		theSync.SyncSignedByte(mPhase2);
	if (aHasPostDelayPhase)
		theSync.SyncFloat(mPostDelayPhase);
	if (aHasMaxAngle)
		theSync.SyncFloat(mMaxAngle);
	if (aHasAngle)
		theSync.SyncFloat(mAngle);
	if (aHasRotation)
		theSync.SyncFloat(mRotation);
	if (aHasFollowed)
	{
		theSync.SyncFloat(mFollowedX);
		theSync.SyncFloat(mFollowedY);
		theSync.SyncSharedObject(mFollowed);
	}
	if (aHasPosition)
	{
		theSync.SyncFloat(mX);
		theSync.SyncFloat(mY);
	}
	else
	{
		mX = mCenterX;
		mY = mCenterY;
	}
}

// 0x4764f0: how far through its cycle the mover is at an update, from 0 up to but not including 1. The pauses hold the
// position still while the updates go on.
float Mover::GetCyclePosition(int theTime) const
{
	int aPeriod = mPeriod;
	int aSteps; // the update within the cycle, counted without the pauses
	if (mPause1 > 0)
	{
		int aTime = mOffset + theTime;
		int aTotal = aPeriod + mPause2 + mPause1; // the cycle with its pauses
		aSteps = aTime;
		if (aTotal > 0)
		{
			int aFirstPauseAt = aPeriod * mPhase1 / 100;
			int aSecondPauseAt = mPause1 + aFirstPauseAt + (mPhase2 - mPhase1) * aPeriod / 100;
			int aInCycle = ((int)((double)mPostDelayPhase * aTotal) + aTime) % aTotal;
			if (aInCycle <= aFirstPauseAt)
				aSteps = aInCycle;
			else if (aInCycle <= mPause1 + aFirstPauseAt)
				aSteps = aFirstPauseAt;
			else if (aInCycle <= aSecondPauseAt)
				aSteps = aInCycle - mPause1;
			else if (aInCycle <= aSecondPauseAt + mPause2)
				aSteps = aSecondPauseAt - mPause1;
			else
				aSteps = aInCycle - mPause2 - mPause1;
		}
	}
	else
		aSteps = mOffset + theTime;
	int aWrapped = (int)((double)mPhase * aPeriod + aSteps) % aPeriod;
	return (float)((double)aWrapped / aPeriod);
}

// 0x476420
void Mover::Shift(float theDeltaX, float theDeltaY)
{
	mX = theDeltaX + mX;
	mY = theDeltaY + mY;
	mCenterX = mCenterX + theDeltaX;
	mCenterY = theDeltaY + mCenterY;
}

// 0x478ea0
void Mover::CalcPos(int theTime, float theSubFrame)
{
	// a mover that follows another one keeps up with how far that one has moved
	if (mFollowed != nullptr)
	{
		float aMovedX = (float)((double)mFollowed->mX - mFollowedX);
		float aMovedY = (float)((double)mFollowed->mY - mFollowedY);
		Shift(aMovedX, aMovedY);
		mFollowedX = mFollowed->mX;
		mFollowedY = mFollowed->mY;
	}

	if (mType == 0 || mPeriod < 1)
	{
		mX = mCenterX;
		mY = mCenterY;
		mAngle = mRotation;
		return;
	}

	int aShape = (int)std::fabs((float)mType);
	int aDirection = mType >= 0 ? 1 : -1;
	float aPhase = (float)((double)GetCyclePosition(theTime) + (double)theSubFrame / mPeriod);
	float aRadiusX = (float)mRadius;
	float aRadiusY = mRadius2 != 0 ? (float)mRadius2 : aRadiusX;

	switch (aShape)
	{
	case SHAPE_VERTICAL:
	{
		mX = mCenterX;
		float aTurn = (float)(aPhase * TWO_PI_AS_FLOAT * aDirection);
		float aSin = (float)std::sin((double)aTurn);
		mY = (float)((double)mCenterY - (double)aSin * aRadiusY);
		mAngle = mRotation;
		break;
	}
	case SHAPE_HORIZONTAL:
	{
		float aTurn = (float)(aPhase * TWO_PI_AS_FLOAT * aDirection);
		float aCos = (float)std::cos((double)aTurn);
		mX = (float)((double)aCos * aRadiusX + mCenterX);
		mY = mCenterY;
		mAngle = mRotation;
		break;
	}
	case SHAPE_CIRCLE:
	{
		float aTurn = (float)(aPhase * TWO_PI_AS_FLOAT * aDirection);
		float aCos = (float)std::cos((double)aTurn);
		mX = (float)((double)aCos * aRadiusX + mCenterX);
		float aSin = (float)std::sin((double)aTurn);
		mY = (float)((double)mCenterY - (double)aSin * aRadiusY);
		mAngle = mRotation;
		break;
	}
	case SHAPE_FIGURE_EIGHT:
	{
		float aTurn = (float)(aPhase * TWO_PI_AS_FLOAT * aDirection);
		float aCos = (float)std::cos((double)aTurn);
		mX = (float)((double)aCos * aRadiusX + mCenterX);
		float aDoubleTurn = (float)((double)aTurn + aTurn);
		float aSin = (float)std::sin((double)aDoubleTurn);
		mY = (float)((double)mCenterY - ((double)aSin * aRadiusY) * 0.5);
		mAngle = mRotation;
		break;
	}
	case SHAPE_FIGURE_EIGHT_UP:
	{
		float aTurn = (float)(aPhase * TWO_PI_AS_FLOAT * aDirection);
		float aDoubleTurn = (float)((double)aTurn + aTurn);
		float aSinDouble = (float)std::sin((double)aDoubleTurn);
		mX = (float)((double)aSinDouble * aRadiusX + mCenterX);
		float aSin = (float)std::sin((double)aTurn);
		mY = (float)((double)mCenterY - (double)aSin * aRadiusY);
		mAngle = mRotation;
		break;
	}
	case SHAPE_SWING_ARC:
	case SHAPE_SWING_ARC_FLIPPED:
	{
		float aMaxAngle = mMaxAngle == 0 ? PI_AS_FLOAT : mMaxAngle;
		float aWave = (float)((double)aPhase * TWO_PI_AS_FLOAT - HALF_PI_AS_FLOAT);
		float aSinWave = (float)std::sin((double)aWave);
		float aSwing = (float)(((double)aSinWave + 1.0) * aMaxAngle * 0.5);
		if (aShape == SHAPE_SWING_ARC)
		{
			float aCos = (float)std::cos((double)aSwing);
			mX = (float)((double)aCos * aRadiusX + mCenterX);
			float aSin = (float)std::sin((double)aSwing);
			mY = (float)((double)mCenterY - (double)aSin * aRadiusY);
		}
		else
		{
			float aSin = (float)std::sin((double)aSwing);
			mX = (float)((double)aSin * aRadiusX + mCenterX);
			float aCos = (float)std::cos((double)aSwing);
			mY = (float)((double)mCenterY - (double)aCos * aRadiusY);
		}
		mAngle = mRotation;
		break;
	}
	case SHAPE_SPIN:
	{
		double aTurn = mMaxAngle == 0 ? aPhase * TWO_PI_AS_FLOAT * aDirection : (double)aDirection * mMaxAngle;
		mAngle = (float)(aTurn + mRotation);
		mX = mCenterX;
		mY = mCenterY;
		break;
	}
	case SHAPE_ROCK:
	{
		float aMaxAngle = mMaxAngle == 0 ? PI_AS_FLOAT : mMaxAngle;
		float aWave = (float)((double)aPhase * TWO_PI_AS_FLOAT);
		float aSin = (float)std::sin((double)aWave);
		float aRock = (float)(((double)aSin * aMaxAngle + mPathRotation) * aDirection);
		mAngle = (float)((double)aRock + mRotation);
		mX = mCenterX;
		mY = mCenterY;
		break;
	}
	case SHAPE_UNUSED:
		break;
	case SHAPE_SAWTOOTH_UP:
	{
		mX = mCenterX;
		float aFraction = (float)((double)aPhase - (int)aPhase);
		mY = (float)((double)mCenterY - (double)aFraction * aRadiusY * aDirection);
		mAngle = mRotation;
		break;
	}
	case SHAPE_SAWTOOTH_ACROSS:
	{
		float aFraction = (float)((double)aPhase - (int)aPhase);
		mX = (float)((double)aRadiusX * aFraction * aDirection + mCenterX);
		mY = mCenterY;
		mAngle = mRotation;
		break;
	}
	case SHAPE_CIRCLE_TURNING:
	{
		float aTurn = (float)(aPhase * TWO_PI_AS_FLOAT * aDirection);
		float aCos = (float)std::cos((double)aTurn);
		mX = (float)((double)aCos * aRadiusX + mCenterX);
		float aSin = (float)std::sin((double)aTurn);
		mY = (float)((double)mCenterY - (double)aSin * aRadiusY);
		if (mRadius2 != 0 && mRadius != 0)
		{
			// the piece points along the path: the angle of the point a quarter turn on, seen from the middle
			float aAhead = (float)((double)aTurn + HALF_PI_AS_FLOAT);
			float aAheadSin = (float)std::sin((double)aAhead);
			float aAheadY = (float)((double)mCenterY - (double)aAheadSin * aRadiusY);
			float aDeltaY = (float)((double)mCenterY - (double)aAheadY);
			float aAheadCos = (float)std::cos((double)aAhead);
			float aAheadX = (float)((double)aAheadCos * aRadiusX + mCenterX);
			float aDeltaX = (float)((double)aAheadX - mCenterX);
			mAngle = (float)(std::atan2((double)aDeltaY, (double)aDeltaX) + mRotation);
		}
		else
			mAngle = (float)((double)mRotation + aTurn);
		break;
	}
	default:
		break;
	}

	if (mPathRotation != 0)
		RotateXY(&mX, &mY, mCenterX, mCenterY, mPathRotation);
}
