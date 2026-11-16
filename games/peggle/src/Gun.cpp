// Gun.cpp: see Gun.h. The aim, the shot, the guide and the drawing are not translated yet.

#include "Gun.h"

#include "DataSync.h"
#include "Line.h"
#include "NotTranslated.h"
#include "ThunderMath.h"

using namespace Sexy;

static const float DEFAULT_BALL_RADIUS = 6.0f;	// the original's global at 0x649d78, the radius of a ball when the cannon holds none
static const int MUZZLE_DROP = 1;				// the muzzle is this far below the cannon's place (the original's ModValInt at Gun.cpp line 17, 151)
static const int NUM_CORNERS = 3;				// the triangle's lines
static const int STILL_TIME = 30;				// updates the aim must stay for UpdateCommon to note it
static const int GUIDE_LENGTH = 40;				// the guide's length, doubled when it shows the bounces

// 0x486a90
Gun::Gun(Board* theBoard)
{
	mUnknown174 = 0;
	mUnknown178 = 0;
	mUnknown1A4 = 0;
	mUnknown1A8 = 0;
	mUnknown1AC = 0;
	mUnknown1C8 = 0;
	mUnknown1CC = 0;
	mUnknown1D0 = 0;
	mUnknown1D8 = 0;
	mUnknown1DC = 0;
	mX = 0;
	mY = 0;
	mBoard = theBoard;
	mAngle = 0;
	mLaunchSpeed = 4.0f;
	mNormalDir = 1;
	mUnknown180 = true;
	mStillAngle = 0;
	mStableAngle = 0;
	mUpdateCount = 0;
	mLastAngle = 0;
	mUnknown198 = -1;
	mLastBallX = 0;
	mUnknown19C = -1;
	mLastBallY = 0;
	mStableTime = 0;
	mLastSpeed = 0;
	mStillCount = 0;
	mBouncyGuide = false;
	mTipLength = 30.0f;
	mUnknown195 = false;
	mUnknown1A1 = false;
	mHalfWidth = 10.0f;
	mBaseShift = 60.0f;
	mBarrelLength = 26.0f;
	mUnknown17C = 1000.0f;
	mUnknown1A2 = false;
	CalcPoints();
}

// 0x4882c0
void Gun::SyncState(DataSync& theSync)
{
	SyncCommon(theSync, false);
	bool aReading = theSync.IsReading();
	if (aReading)
		mUpdateCount = 0;

	bool aHasLastShot = mLastSpeed != 0;
	bool aHasUpdateCount = mUpdateCount != 0;
	theSync.SyncBoolBit(aHasUpdateCount);
	theSync.SyncBoolBit(mBouncyGuide);
	theSync.SyncBoolBit(aHasLastShot);
	theSync.SyncBoolBit(mUnknown195);
	theSync.SyncBoolBit(mUnknown1A1);
	theSync.SyncBoolBit(mUnknown1A2);
	theSync.EndBit();

	if (aHasUpdateCount)
		theSync.SyncLong(mUpdateCount);
	if (aHasLastShot)
	{
		theSync.SyncFloat(mLastAngle);
		theSync.SyncFloat(mLastBallX);
		theSync.SyncFloat(mLastBallY);
		theSync.SyncFloat(mLastSpeed);
	}
	theSync.SyncFloat(mAngle);
	theSync.SyncSharedObject(mBall);
	if (aReading)
	{
		CalcPoints();
		mTargetAngle = mAngle;
	}
}

// 0x483da0. The triangle is the point, then the two ends of the base; its three lines are one-sided, so a ball hits it from
// outside. The ball's place is the fourth point.
void Gun::CalcPoints()
{
	mConvex = true;
	mClosed = true;
	if (mLines.size() != NUM_CORNERS)
	{
		mLines.clear();
		for (int i = 0; i < NUM_CORNERS; i++)
		{
			mLines.push_back(SharedPtr<Line>(new Line(0, 0, 0, 0)));
			mLines.back()->mOneSided = true;
			mLineOffsets.push_back(SexyVector2(0, 0));
		}
	}

	float aBallRadius = DEFAULT_BALL_RADIUS;
	if (mBall != nullptr)
		aBallRadius = mBall->mRadius;

	float aPoints[8];
	aPoints[0] = mTipLength + mX;
	aPoints[1] = mY;
	aPoints[2] = mX;
	aPoints[3] = mY - mHalfWidth;
	aPoints[4] = mX;
	aPoints[5] = mHalfWidth + mY;
	float aMuzzleOffset = mBarrelLength + aBallRadius;
	aPoints[6] = aMuzzleOffset + mX;
	aPoints[7] = (float)MUZZLE_DROP + mY;
	for (int i = 0; i < 4; i++)
	{
		aPoints[i * 2] = aPoints[i * 2] + mBaseShift;
		RotateXY(&aPoints[i * 2], &aPoints[i * 2 + 1], mX, mY, mAngle);
	}

	for (int i = 0; i < NUM_CORNERS; i++)
	{
		int aNext = (i + 1) % NUM_CORNERS;
		SharedPtr<Line>& aLine = mLines[i];
		aLine->Init(aPoints[i * 2], aPoints[i * 2 + 1], aPoints[aNext * 2], aPoints[aNext * 2 + 1]);
		mLineOffsets[i].x = aLine->mX1 - mX;
		mLineOffsets[i].y = aLine->mY1 - mY;
	}

	if (mBall != nullptr)
		mBall->SetAbsPos(aPoints[6], aPoints[7]);
	mBallX = aPoints[6];
	mBallY = aPoints[7];
	CalcBoundingBox();
}

// 0x4752b0
void Gun::UpdateCommon()
{
	if (mStillAngle == mAngle)
	{
		mStillCount++;
		if (mStillCount > STILL_TIME - 1)
		{
			mStableAngle = mAngle;
			mStableTime = mUpdateCount;
			return;
		}
	}
	else
	{
		mStillCount = 0;
		mStillAngle = mAngle;
	}
}

// 0x4754a0
int Gun::GetMaxGuideLength() const
{
	return mBouncyGuide ? GUIDE_LENGTH * 2 : GUIDE_LENGTH;
}

// 0x4754c0
bool Gun::NeedDrawMouseClick() const
{
	if (mBouncyGuide || mUnknown198 >= 0)
	{
		if (mUnknown198 <= GetMaxGuideLength())
			return false;
	}
	return true;
}

// 0x484120
void Gun::Update()
{
	NOT_TRANSLATED("Gun::Update");
}

// 0x47df50
void Gun::UpdateSlowMo(float theSubFrame)
{
	Poly::UpdateSlowMo(theSubFrame);
	UpdateCommon();
}

// 0x47f590
void Gun::Draw(Graphics* g)
{
	NOT_TRANSLATED("Gun::Draw");
}

// 0x4843a0
bool Gun::SetAngle(float theAngle, bool theAtOnce)
{
	NOT_TRANSLATED("Gun::SetAngle");
}

// 0x484530
SharedPtr<Ball> Gun::Fire(bool theKeepAim)
{
	NOT_TRANSLATED("Gun::Fire");
}

// 0x482910
void Gun::UpdateBouncyGuide()
{
	NOT_TRANSLATED("Gun::UpdateBouncyGuide");
}
