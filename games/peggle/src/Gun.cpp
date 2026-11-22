// Gun.cpp: see Gun.h. The aim, the shot, the guide and the drawing are not translated yet.

#include "Gun.h"

#include "Ball.h"
#include "Board.h"
#include "CollisionMgr.h"
#include "LogicMgr.h"
#include "DataSync.h"
#include "Line.h"
#include "NotTranslated.h"
#include "ThunderMath.h"

#include <cmath>

using namespace Sexy;

static const float DEFAULT_BALL_RADIUS = 6.0f;	// the original's global at 0x649d78, the radius of a ball when the cannon holds none
static const int MUZZLE_DROP = 1;				// the muzzle is this far below the cannon's place (the original's ModValInt at Gun.cpp line 17, 151)
static const int TURN_MEMORY_UPDATES = 20;		// the turns older than this are forgotten by GetTurnSpeed
static const double GUN_TURN_LENGTH = 10.0;		// how far ahead the turn speed carries the held ball
static const float GUN_TURN_STEP = 0.2f;		// the radians the aim turns in an update (ModValFloat, Gun.cpp 24, 385)
static const int GUN_LAST_SPEED_KEPT = 150;		// the last shot's speed is forgotten after this many updates of the shot
static const int NUM_CORNERS = 3;				// the triangle's lines
static const int STILL_TIME = 30;				// updates the aim must stay for UpdateCommon to note it
static const float GUIDE_DEFAULT_RADIUS = 6.0f;		// the guide ball's radius when the cannon holds no ball
static const int GUIDE_STEPS_A = 300;				// ModValInt Gun.cpp 18,245: updates the guide ball is walked (when mUnknown1A2)
static const int GUIDE_STEPS_B = 200;				// (19,245)
static const float GUIDE_FIREBALL_RADIUS = 10.0f;	// the guide ball of a fireball
static const int GUIDE_STOP_BELOW = 20;       		// the guide stops this far below the board's line
static const int GUIDE_HITS_A = 1;					// ModValInt Gun.cpp 20,264: hits the guide shows (when mUnknown1A2)
static const int GUIDE_HITS_B = 1;					// (21,264)
static const float GUIDE_UNSTEADY = 1000.0f;		// mUnknown17C before the first steadiness is known
static const float GUIDE_STEADY_MIN = 0.0001f;
static const int GUIDE_MOVER_STEPS = 5;				// ModValInt Gun.cpp 22,311: updates the guide goes on after hitting a moving piece
static const int GUIDE_BOUNCY_STEPS = 100;			// (23,323): updates the bouncy guide goes on after its hit
static const int GUIDE_LENGTH = 40;				// the guide's length, doubled when it shows the bounces

// 0x486a90
Gun::Gun(Board* theBoard)
{
	mGuideDirX = 0;
	mGuideDirY = 0;
	mFirstHitX = 0;
	mFirstHitY = 0;
	mFirstHitAngle = 0;
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

// 0x47f5f0
void Gun::Reset()
{
	mBall = nullptr;
	mGuidePoints.clear();
	mTurns.clear();
	mUpdateCount = 0;
	mUnknown17C = 1000.0f;
	mBouncyGuide = false;
	mUnknown1A1 = false;
	mUnknown1A2 = false;
	mUnknown180 = true;
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

// 0x47bf30: how far the aim has turned in an update, over the turns of the last 20 updates (the older ones are forgotten)
float Gun::GetTurnSpeed()
{
	if (mTurns.empty())
		return 0;
	while (!mTurns.empty() && TURN_MEMORY_UPDATES < mUpdateCount - mTurns.front().mTime)
		mTurns.pop_front();
	float aSpeed = 0;
	if (!mTurns.empty())
	{
		float aSum = 0;
		for (std::list<GunTurn>::iterator anItr = mTurns.begin(); anItr != mTurns.end(); ++anItr)
			aSum = (float)((double)anItr->mDelta + aSum);
		int aTime = mUpdateCount - mTurns.front().mTime;
		if (aTime < 1)
			aTime = 1;
		aSpeed = (float)((double)aSum / (float)aTime);
	}
	return aSpeed;
}

// 0x484120: the cannon's polygon is updated; the ball it holds is given the speed of the cannon's turn (so that it goes on turning
// with it); the aim is moved towards its goal by 0.2 radians an update; the guide is worked out while the player aims
void Gun::Update()
{
	Poly::Update();
	mUpdateCount++;
	float aVelocityX = 0;
	float aVelocityY = 0;
	if (!mLines.empty())
	{
		aVelocityX = mLines[0]->mVelocityX;
		aVelocityY = mLines[0]->mVelocityY;
	}
	float aTurnSpeed = GetTurnSpeed();
	if (mBall.get() != nullptr)
	{
		float aLength = (float)((double)aTurnSpeed * GUN_TURN_LENGTH);
		float aDirection = (float)((double)mAngle + HALF_PI_AS_FLOAT);
		float aSin = (float)std::sin((double)aDirection);
		float aNewY = (float)((double)aVelocityY - (double)aSin * aLength);
		float aCos = (float)std::cos((double)aDirection);
		float aNewX = (float)((double)aCos * aLength + aVelocityX);
		mBall->SetVelocity(aNewX, aNewY);
	}
	mAngularVelocity = aTurnSpeed;
	if (!gSlowMoUpdate)
		UpdateCommon();
	if (mAngle != mTargetAngle)
	{
		float aTarget = mTargetAngle;
		float aCurrent = mAngle;
		if (aTarget < HALF_PI_AS_FLOAT)
			aTarget = (float)((double)aTarget + TWO_PI_AS_FLOAT);
		if (aCurrent < HALF_PI_AS_FLOAT)
			aCurrent = (float)(TWO_PI_AS_FLOAT + (double)aCurrent);
		float aStep = GUN_TURN_STEP;
		if (aTarget <= aCurrent)
		{
			mAngle = (float)((double)mAngle - aStep);
			float aNext = (float)((double)aCurrent - aStep);
			if (!(aNext > aTarget))
			{
				float aCanonical = GetCanonicalAngleRad(mTargetAngle);
				mTargetAngle = aCanonical;
				mAngle = aCanonical;
			}
		}
		else
		{
			mAngle = (float)((double)mAngle + aStep);
			float aNext = (float)((double)aCurrent + aStep);
			if (!(aNext < aTarget))
			{
				float aCanonical = GetCanonicalAngleRad(mTargetAngle);
				mTargetAngle = aCanonical;
				mAngle = aCanonical;
			}
		}
		CalcPoints();
	}
	// (the level editor can stop the guide with a flag at 0x650a57; the game never sets it)
	if (mBoard->mLogicMgr->mState == LOGIC_STATE_PRE_SHOT)
		UpdateBouncyGuide();
	if (mLastSpeed != 0 && (mBoard->mLogicMgr->mState != LOGIC_STATE_SHOT || GUN_LAST_SPEED_KEPT < mBoard->mLogicMgr->mStateTime))
		mLastSpeed = 0;
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
	if (mAngle == theAngle)
		return false;
	if (theAtOnce)
	{
		mTurns.clear();
		mAngle = theAngle;
		mTargetAngle = theAngle;
		mStableTime = mUpdateCount - 1000;
		CalcPoints();
		return true;
	}
	if (mTargetAngle != theAngle)
	{
		float aDelta = CalcAngleDiff(mAngle, theAngle);
		// a turn the other way than the one asked for last makes that one pointless: the list starts again
		if (!mTurns.empty() && Sign(aDelta) == Sign(mTurns.back().mDelta))
			mTurns.clear();
		GunTurn aTurn = {aDelta, mUpdateCount};
		mTurns.push_back(aTurn);
		mTargetAngle = theAngle;
	}
	return true;
}

static const int GUN_AIM_SETTLE_UPDATES = 10; // an aim younger than this is not yet the one the player settled on (ModValInt-less constant of Gun::Fire)

// 0x484530: lets the ball go along the aim; the ball it returns is the cannon's no longer
SharedPtr<Ball> Gun::Fire(bool theKeepAim)
{
	if (!mBall)
		return SharedPtr<Ball>();
	float aSavedAngle = mAngle;
	// an aim that has stayed for under 10 updates is taken as the one it was before the hand moved
	if (theKeepAim && mUpdateCount - mStableTime < GUN_AIM_SETTLE_UPDATES)
	{
		mAngle = mStableAngle;
		CalcPoints();
		if (mBouncyGuide)
			UpdateBouncyGuide();
	}
	SharedPtr<Ball> aBall = mBall;
	mBall = nullptr;
	mLastAngle = mAngle;
	mLastBallX = mBallX;
	mLastBallY = mBallY;
	mLastSpeed = mLaunchSpeed;
	aBall->mIsPeg = false;
	float aSin = (float)std::sin((double)mAngle);
	float aCos = (float)std::cos((double)mAngle);
	float aVelocityY = (float)(-(double)mLaunchSpeed * aSin);
	float aVelocityX = (float)((double)mLaunchSpeed * aCos);
	aBall->SetVelocity(aVelocityX, aVelocityY);
	if (mAngle != aSavedAngle)
	{
		mAngle = aSavedAngle;
		CalcPoints();
	}
	return aBall;
}

// 0x482910: the guide. A ball is made at the muzzle with the shot's speed and walked through the level until it has hit as many
// times as the guide shows (or has gone as many steps or as low as the board's bottom); the places it went through are the guide's
// points. The ball does not change the level: no contact is noted and no peg is hit.
void Gun::UpdateBouncyGuide()
{
	mGuidePoints.clear();
	mUnknown198 = -1;
	mUnknown19C = -1;

	float aRadius = mBall.get() != nullptr ? mBall->mRadius : GUIDE_DEFAULT_RADIUS;
	SharedPtr<Ball> aBall = new Ball(false);
	aBall->SetPos(mBallX, mBallY);
	aBall->mRadius = aRadius;
	aBall->mUnknown163 = false;
	float aSin = (float)std::sin((double)mAngle);
	float aCos = (float)std::cos((double)mAngle);
	aBall->SetVelocity((float)((double)mLaunchSpeed * aCos), (float)(-(double)mLaunchSpeed * aSin));

	int aMaxSteps = mUnknown1A2 ? GUIDE_STEPS_A : GUIDE_STEPS_B;
	int anEnd = 0;
	CollisionMgr* aMgr = mBoard->mCollisionMgr.get();
	aMgr->mUnknown00 = false;
	bool aFireball = mBall.get() != nullptr && mBall->mFireball;
	aMgr->mUnknown78 = aFireball;
	if (aFireball)
		aBall->mRadius = GUIDE_FIREBALL_RADIUS;
	gContactRecorder = nullptr;
	aMgr->BuildLocalPieces(aBall.get());
	mUnknown1A0 = false;
	float aStopY = (float)(mBoard->mUnknown1B4 + GUIDE_STOP_BELOW);
	int aMaxHits = mUnknown1A2 ? GUIDE_HITS_A : GUIDE_HITS_B;
	int aStep = 0;
	int aHits = 0;
	while (aStep < aMaxSteps)
	{
		aBall->Update();
		SexyVector2 aContact(0, 0);
		if (aMgr->UpdateOneBall(aBall.get(), &aContact))
		{
			if (aHits == 0)
			{
				mUnknown198 = aStep;
				mFirstHitX = aContact.x;
				mFirstHitY = aContact.y;
				bool aMoverHit = false;
				for (size_t i = 0; i < aMgr->mCandidates.size(); i++)
				{
					PhysObj* aPiece = aMgr->mCandidates[i];
					if (aPiece->mMover.get() != nullptr && aPiece->mMoving)
						aMoverHit = true;
				}
				if (mUnknown180)
				{
					SexyVector2 aDir = NormalizeVector(SexyVector2(aBall->mVelocityX, aBall->mVelocityY));
					float aSteadiness;
					if (mUnknown17C < GUIDE_UNSTEADY)
					{
						float aDot = (float)std::fabs((double)mGuideDirY * aDir.y + (double)mGuideDirX * aDir.x);
						mUnknown17C = aDot;
						aSteadiness = (float)(std::acos((double)aDot) / HALF_PI_AS_FLOAT);
						if (aSteadiness < GUIDE_STEADY_MIN)
							aSteadiness = GUIDE_STEADY_MIN;
						if (aSteadiness > 1.0f)
							aSteadiness = 1.0f;
					}
					else
						aSteadiness = 1.0f;
					mUnknown17C = aSteadiness;
					mGuideDirX = aDir.x;
					mGuideDirY = aDir.y;
					mUnknown180 = false;
				}
				mFirstHitAngle = (float)std::atan2((double)aBall->mVelocityY, (double)aBall->mVelocityX);
				if (aMoverHit)
				{
					mUnknown1A0 = true;
					if (anEnd == 0 || GUIDE_MOVER_STEPS < anEnd)
						anEnd = GUIDE_MOVER_STEPS;
				}
			}
			aHits++;
			if (aHits == aMaxHits && anEnd == 0)
				anEnd = mBouncyGuide ? GUIDE_BOUNCY_STEPS : 1;
		}
		if (mUnknown19C == -1 && aMgr->mUnknown00)
			mUnknown19C = aStep;
		if (aHits > aMaxHits || aStopY < aBall->mY)
			break;
		mGuidePoints.push_back(SexyVector2(aBall->mX, aBall->mY));
		if (anEnd != 0 && --anEnd == 0)
			break;
		if (++aStep >= aMaxSteps)
			break;
	}
	mUnknown195 = !aMgr->mUnknown00;
	aMgr->mUnknown78 = false;
}

// 0x4844c0
void Gun::SetBall(const SharedPtr<Ball>& theBall)
{
	mBall = theBall;
	mBall->mIsPeg = true; // (+0x140: the flag the cannon sets on the ball it holds)
	mUnknown1A1 = false;
	CalcPoints();
}

// 0x47f6e0: the guide's list of points (the three words at +0x1c8) is emptied
void Gun::SetBouncyGuide(bool theOn)
{
	mBouncyGuide = theOn;
	mGuidePoints.clear();
}
