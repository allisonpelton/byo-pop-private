// Ball.cpp: see Ball.h. The physics and the drawing are not translated yet.

#include "Ball.h"

#include "DataSync.h"
#include "CollisionMgr.h"
#include "Line.h"
#include "NotTranslated.h"
#include "PhysObjFactory.h"
#include "Poly.h"
#include "SexyVector.h"
#include "ThunderMath.h"

#include <cmath>

using namespace Sexy;

static const float DEFAULT_RADIUS = 6.0f;		// the original's global at 0x649d78 (nothing writes to it)
static const int NO_TIMER = 1000000;			// mUnknown148 until set
static const int NO_VALUE_158 = -1000;			// mUnknown158 until set
static const int BALL_TRAIL_STEP = 10;			// every this many updates the ball's place is noted
static const int BALL_TRAIL_PLACES = 6;			// the places kept
static const float BALL_MOVED_FAR = 4.0f;		// the places a ball has been at since this far (squared) from where it is: it is not still
static const float BALL_MOVED_A_BIT = 0.040000003f;	// this far (squared) counts as moving a little, which sets its rest count back
static const int BALL_MOVED_A_BIT_REWIND = 5;
static const float BALL_GRAVITY = 0.05f;
static const double BALL_HAT_LOWER = 2.0;		// the hat's goal is not below this under the ball
static const double BALL_HAT_FOLLOW = 0.25;		// how much of the way to the ball the hat goes each update
static const double BALL_HAT_SHIFT_X = 0;		// where the hat sits on its goal (ModValInt, Ball.cpp 4 and 5, 1048 and 1049: 0 and 21)
static const double BALL_HAT_SHIFT_Y = 21;
static const float BALL_MAX_SPEED = 15.0f;		// (ModValFloat, Ball.cpp 6, 1055)
static const int HAT_COLLIDE_AFTER = 10;		// a hat can be hit once its mover has run more than this many updates
static const float COLLIDE_MARGIN = 0.01f;		// how far inside a peg's radius a ball counts as touching it
static const float BALL_BALL_KEEP = 0.95f;		// the share of the speed along the line that two balls trade
static const float PEG_BOUNCINESS = 0.9f;		// a peg that has none of its own
static const float PEG_ROLLINESS = 1.0f;		// ModValFloat, Ball.cpp 1,533
static const double PEG_FORCE_DIVISOR = 999999995905.0;	// the original's two constants (0x5f0bc0 and 0x5f0bb8) that are nearly a million million
static const double PEG_FORCE_FACTOR = 999999995904.0;
static const float WALL_BOUNCINESS = 0.82f;		// a wall that has none of its own (ModValFloat, Ball.cpp 2,674)
static const float WALL_ROLLINESS = 1.4f;		// (3,675)
static const float UPRIGHT_WALL = 0.99f;		// a wall this near to upright (the x of its normal) makes the hit count five times
static const double UPRIGHT_FACTOR = 5.0;
static const int MAX_HITS_AN_UPDATE = 20;		// a ball that has hit this many times in one update is left where it is
static const int DEFAULT_180 = 10000;			// mUnknown180 until set
static const int VERSION_PEG_ROW = 12;			// the files from this version on keep four pairs of floats for a ball in play
static const int VERSION_PEG_FLAGS = 13;		// ... and a run of flags
static const int VERSION_BALL_ID = 24;			// ... and the number of the piece
static const int VERSION_PEG_FLAGS_2 = 30;		// ... and a second run of flags
static const int VERSION_UNKNOWN_180 = 63;		// ... and a third number of the group at 0x178

// 0x480de0
Ball::Ball(bool theIsPeg)
{
	mUnknownE4 = 0;
	for (int i = 0; i < 12; i++)
		mTrail[i / 2][i % 2] = 0;
	mUnknown168 = 0;
	mUnknown16C = 0;
	mBallId = mSerialNumber;
	mIsPeg = theIsPeg;
	mKind = theIsPeg ? KIND_PEG : KIND_SHOT_BALL;
	mRadius = DEFAULT_RADIUS;
	mUnknown14C = 0;
	mUnknown150 = 0;
	mPreviousY = 0;
	mUnknown154 = 0;
	mPreviousX = 0;
	mUnknown158 = NO_VALUE_158;
	mUnknown174 = 0;
	mUnknown148 = NO_TIMER;
	mUnknown170 = 0;
	mUnknown160 = false;
	mUnknown163 = true;
	mFireball = false;
	mUnknown164 = false;
	mUnknown165 = false;
	mUnknown142 = false;
	mUnknown143 = false;
	mUnknown144 = false;
	mUnknown145 = false;
	mUnknown146 = false;
	mIsHat = false;
	mHasHat = false;
	mUnknown162 = false;
	mUnknown178 = 0;
	mUnknown17C = 0;
	mUnknown180 = DEFAULT_180;
	mFireballSound = nullptr;
	SetPos(0, 0);
	SetVelocity(0, 0);
}

// 0x483410
void Ball::SyncState(DataSync& theSync)
{
	PhysObj::SyncState(theSync);
	bool aReading = theSync.IsReading();
	if (aReading)
	{
		mUnknown148 = NO_TIMER;
		mUnknown160 = false;
		mUnknown14C = 0;
		mUnknown150 = 0;
		mUnknown158 = NO_VALUE_158;
		mUnknown154 = 0;
		mUnknownPiece15C = nullptr;
		mUnknown142 = false;
		mUnknown170 = 0;
		mUnknown143 = false;
	}

	bool aHasPosition = mMover == nullptr;
	bool aHasTimer = mUnknown148 < NO_TIMER;
	bool aHas14C = mUnknown14C != 0;
	bool aHasPiece = mUnknownPiece15C != nullptr;
	bool aHas150 = mUnknown150 != 0;
	bool aHas158 = mUnknown158 != NO_VALUE_158;
	bool aHas154 = mUnknown154 != 0;

	theSync.SyncBoolBit(mIsPeg);
	theSync.SyncBoolBit(aHasPosition);
	theSync.SyncBoolBit(aHasTimer);
	theSync.SyncBoolBit(mUnknown160);
	theSync.SyncBoolBit(mUnknown163);
	theSync.SyncBoolBit(aHas14C);
	theSync.SyncBoolBit(aHasPiece);
	theSync.SyncBoolBit(mFireball);
	theSync.EndBit();

	if (aHasPosition)
	{
		theSync.SyncFloat(mX);
		theSync.SyncFloat(mY);
	}
	else
	{
		if (mMover == nullptr)
			throw DataSyncException();
		mX = mMover->mX;
		mY = mMover->mY;
	}
	theSync.SyncFloat(mRadius);

	if (mKind == KIND_SHOT_BALL)
	{
		theSync.SyncFloat(mPreviousX);
		theSync.SyncFloat(mPreviousY);
		theSync.SyncFloat(mVelocityX);
		theSync.SyncFloat(mVelocityY);
		theSync.SyncFloat(mTrail[5][0]);
		theSync.SyncFloat(mTrail[5][1]);
		theSync.SyncLong(mUnknownE4);
		theSync.SyncFloat(mTrail[0][0]);
		theSync.SyncFloat(mTrail[0][1]);
		if (theSync.mVersion >= VERSION_PEG_ROW)
		{
			for (int i = 0; i < 8; i++)
				theSync.SyncFloat(mTrail[1 + i / 2][i % 2]);
		}
		if (theSync.mVersion >= VERSION_PEG_FLAGS)
		{
			theSync.SyncBoolBit(mUnknown164);
			theSync.SyncBoolBit(mUnknown165);
			theSync.SyncBoolBit(aHas150);
			theSync.SyncBoolBit(aHas158);
			theSync.SyncBoolBit(aHas154);
			theSync.SyncBoolBit(mUnknown142);
			theSync.SyncBoolBit(mUnknown144);
			theSync.SyncBoolBit(mUnknown145);
			theSync.EndBit();
		}
		if (theSync.mVersion >= VERSION_BALL_ID)
			theSync.SyncLong(mBallId);
		bool aHas178 = mUnknown178 != 0;
		if (theSync.mVersion >= VERSION_PEG_FLAGS_2)
		{
			theSync.SyncBoolBit(mUnknown146);
			theSync.SyncBoolBit(mHasHat);
			theSync.SyncBoolBit(mIsHat);
			theSync.SyncBoolBit(aHas178);
			theSync.SyncBoolBit(mUnknown162);
			theSync.EndBit();
		}
		if (aHasTimer)
			theSync.SyncLong(mUnknown148);
		if (aHas14C)
			theSync.SyncLong(mUnknown14C);
		if (aHasPiece)
			theSync.SyncSharedPolymorphic(mUnknownPiece15C, MakePhysObj);
		if (aHas150)
			theSync.SyncLong(mUnknown150);
		if (aHas158)
			theSync.SyncLong(mUnknown158);
		if (aHas154)
			theSync.SyncLong(mUnknown154);
		if (aHas178)
		{
			theSync.SyncLong(mUnknown178);
			theSync.SyncLong(mUnknown17C);
			if (theSync.mVersion >= VERSION_UNKNOWN_180)
				theSync.SyncLong(mUnknown180);
		}
		if (mHasHat)
			theSync.SyncSharedObject(mHat);
	}

	if (aReading)
	{
		mLeft = mX - mRadius;
		mRight = mRadius + mX;
		mTop = mY - mRadius;
		mBottom = mY + mRadius;
		mUnknown104 = mX;
		mUnknown108 = mY;
		if (mFireball)
			NOT_TRANSLATED("Ball: starting the fireball sound of a ball that is read"); // 0x474f00
	}
}

// 0x474e60
void Ball::SetPos(float theX, float theY)
{
	mX = theX;
	mY = theY;
	mLeft = theX - mRadius;
	mRight = mRadius + theX;
	mTop = theY - mRadius;
	mBottom = theY + mRadius;
}

// 0x474eb0
void Ball::Translate(float theDeltaX, float theDeltaY)
{
	mX = theDeltaX + mX;
	mY = theDeltaY + mY;
	mLeft = mLeft + theDeltaX;
	mRight = mRight + theDeltaX;
	mTop = mTop + theDeltaY;
	mBottom = theDeltaY + mBottom;
}

// 0x475600
void Ball::SetVelocity(float theDeltaX, float theDeltaY)
{
	mVelocityX = theDeltaX;
	mVelocityY = theDeltaY;
}

// 0x474e40
void Ball::GetCenter(float* theCenter) const
{
	theCenter[0] = mX;
	theCenter[1] = mY;
}

// 0x474dd0
void Ball::SetAbsPos(float theX, float theY)
{
	SetPos(theX, theY);
	mPreviousX = mX;
	mPreviousY = mY;
	mTrail[5][0] = mX;
	mTrail[5][1] = mY;
	mUnknown104 = mX;
	mUnknown108 = mY;
}

// 0x477070: the ball has been put in the game, so it has been here all along
void Ball::AddedToGame()
{
	mPreviousX = mX;
	mPreviousY = mY;
	mTrail[5][0] = mX;
	mTrail[5][1] = mY;
}

// 0x480f50
void Ball::SetHat(bool theHasHat, bool theMakeBall)
{
	NOT_TRANSLATED("Ball::SetHat");
}

// 0x47a450: one step of a ball in play: its counters, where it has been lately (a ball that stays within a small place for a while
// is counted as still), gravity, the hat that follows it, the speed limit, and the box the collisions look in
void Ball::Update()
{
	if (mIsHat)
	{
		mMoverTime++;
		return;
	}
	if (mIsPeg)
	{
		mPreviousX = mX;
		mPreviousY = mY;
		PhysObj::Update();
		mUnknown141 = true;
		return;
	}
	PhysObj::Update();
	mUnknown148++;
	if (mUnknown150 != 0 && --mUnknown150 == 0)
		mUnknown154 = 0;
	mUnknownE4++;
	int aPlace = mUnknownE4 / BALL_TRAIL_STEP;
	if (mUnknownE4 % BALL_TRAIL_STEP == 0)
	{
		aPlace = aPlace < 1 ? 1 : (BALL_TRAIL_PLACES < aPlace ? BALL_TRAIL_PLACES : aPlace);
		aPlace--;
		mTrail[aPlace][0] = mX;
		mTrail[aPlace][1] = mY;
		bool aSlowed = mUnknown164;
		mUnknown164 = false;
		for (int i = 0; i < aPlace; i++)
		{
			float aDeltaX = mTrail[i][0] - mX;
			float aDeltaY = mTrail[i][1] - mY;
			float aDistanceSq = (float)((double)aDeltaX * aDeltaX + (double)aDeltaY * aDeltaY);
			if (BALL_MOVED_FAR <= aDistanceSq)
			{
				mUnknownE4 = 0;
				break;
			}
			if (BALL_MOVED_A_BIT <= aDistanceSq && !aSlowed)
			{
				mUnknownE4 -= BALL_MOVED_A_BIT_REWIND;
				aSlowed = true;
				mUnknown164 = true;
			}
		}
	}
	mPreviousX = mX;
	mPreviousY = mY;
	if (0 < mUnknown14C)
	{
		mUnknownE4 = 0;
		mUnknown14C--;
		if (mUnknown14C != 0)
		{
			mUnknown141 = true;
			return;
		}
		SetActive(true);
		if (mUnknownPiece15C != nullptr)
		{
			float aY = mUnknownPiece15C->GetYPos();
			float aX = mUnknownPiece15C->GetXPos();
			SetAbsPos(aX, aY);
			mUnknownPiece15C = nullptr;
		}
	}
	mVelocityY = (float)((double)mVelocityY + (double)BALL_GRAVITY);
	if (mHasHat)
	{
		if ((double)mY + BALL_HAT_LOWER < (double)mUnknown108)
			mUnknown108 = (float)((double)mY + BALL_HAT_LOWER);
		float aDeltaX = (float)((double)mX - mUnknown104);
		float aDeltaY = (float)((double)mY - mUnknown108);
		float aMoveX = (float)((double)aDeltaX * BALL_HAT_FOLLOW);
		float aMoveY = (float)(BALL_HAT_FOLLOW * (double)aDeltaY);
		mUnknown104 = (float)((double)aMoveX + mUnknown104);
		mUnknown108 = (float)((double)mUnknown108 + aMoveY);
		if (mHat != nullptr)
			mHat->SetPos((float)(BALL_HAT_SHIFT_X + (double)mUnknown104), (float)((double)mUnknown108 - BALL_HAT_SHIFT_Y));
	}
	float aMaxSpeed = BALL_MAX_SPEED;
	float aVelocityX = mVelocityX;
	float aVelocityY = mVelocityY;
	if ((float)((double)aMaxSpeed * aMaxSpeed) < (double)aVelocityY * aVelocityY + (double)aVelocityX * aVelocityX)
	{
		SexyVector2 aDirection = SexyVector2(aVelocityX, aVelocityY).Normalize();
		aVelocityX = (float)((double)aDirection.x * aMaxSpeed);
		aVelocityY = (float)((double)aMaxSpeed * aDirection.y);
		mVelocityX = aVelocityX;
		mVelocityY = aVelocityY;
	}
	mLeft = (float)(((double)mX - mRadius) + (0.0f <= aVelocityX ? 0.0f : aVelocityX));
	mRight = (float)(((double)mRadius + mX) + (aVelocityX <= 0.0f ? 0.0f : aVelocityX));
	mTop = (float)(((double)mY - mRadius) + (0.0f <= aVelocityY ? 0.0f : aVelocityY));
	mUnknown141 = false;
	mUnknown142 = false;
	mBottom = (float)(((double)mRadius + mY) + (aVelocityY <= 0.0f ? 0.0f : aVelocityY));
	mTrail[5][0] = mX;
	mTrail[5][1] = mY;
}

// 0x47a8d0
void Ball::UpdateSlowMo(float theSubFrame)
{
	NOT_TRANSLATED("Ball::UpdateSlowMo");
}

// 0x4838f0
void Ball::DrawShadow(Graphics* g)
{
	NOT_TRANSLATED("Ball::DrawShadow");
}

// 0x47b0c0
void Ball::Draw(Graphics* g)
{
	NOT_TRANSLATED("Ball::Draw");
}

// 0x477a00: a hat is hit by the pegs that are not yet hit (and only once it has been going for more than 10 updates); any other ball
// by the pieces that are collidable (a peg, by balls in play only)
bool Ball::IsCollidableWith(const PhysObj* theOther) const
{
	if (mIsHat)
	{
		if (mMoverTime <= HAT_COLLIDE_AFTER)
			return false;
		if (theOther->mPegInfo.get() == nullptr || theOther->mPegInfo->mHit)
			return false;
		return mCollidable && theOther->mCollidable;
	}
	if (mIsPeg && theOther->mKind != KIND_SHOT_BALL)
		return false;
	return mCollidable && theOther->mCollidable;
}

// 0x475000: the ball (this one, a peg) against a ball at thePos going by theVel, as a time in the update when the two touch
bool Ball::CheckCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, float& theTime,
						  SexyVector2& theContact) const
{
	float aRadius = (float)(((double)mRadius - COLLIDE_MARGIN) + theRadius);
	float aRadiusSq = (float)((double)aRadius * aRadius);
	float aX = mX;
	float aY = mY;
	float aDeltaX = thePos.x - aX;
	float aDeltaY = thePos.y - aY;
	float aDistSq = (float)((double)aDeltaX * aDeltaX + (double)aDeltaY * aDeltaY);
	if (aDistSq < aRadiusSq)
	{
		// already inside: it is at once
		theTime = 0;
		theContact = thePos;
		return true;
	}
	float aSpeedSq = (float)((double)theVel.x * theVel.x + (double)theVel.y * theVel.y);
	float aHalfB = (float)((double)aDeltaX * theVel.x + (double)aDeltaY * theVel.y);
	float aB = (float)((double)aHalfB + aHalfB);
	float aPosSq = (float)((double)thePos.x * thePos.x + (double)thePos.y * thePos.y);
	float aCenterSq = (float)((double)aX * aX + (double)aY * aY);
	float aCross = (float)((double)aY * thePos.y + (double)aX * thePos.x);
	float aC = (float)(((double)aPosSq + aCenterSq) - ((double)aCross + aCross) - aRadiusSq);
	float aDiscriminant = (float)((double)aB * aB - (double)aC * ((double)aSpeedSq * 4.0));
	if (aDiscriminant < 0)
		return false;
	float aRoot = (float)std::sqrt((double)aDiscriminant);
	float aT = (float)(((double)-aB - aRoot) / ((double)aSpeedSq + aSpeedSq));
	if (!(aT >= 0))
		aT = (float)(((double)aRoot - aB) / ((double)aSpeedSq + aSpeedSq));
	if (aT >= 0 && aT < theTime)
	{
		theTime = aT;
		float aMoveX = (float)((double)theVel.x * aT);
		float aMoveY = (float)((double)aT * theVel.y);
		theContact.x = (float)((double)thePos.x + aMoveX);
		theContact.y = (float)((double)thePos.y + aMoveY);
		return true;
	}
	return false;
}

// 0x4775b0: this ball has met another ball (or a peg): two balls bounce off each other, a ball off a peg
void Ball::CollideWithBall(Ball* theOther)
{
	if (!mIsPeg)
	{
		if (!theOther->mIsPeg)
			BounceBalls(theOther);
		else
			NOT_TRANSLATED("Ball::CollideWithBall: a ball and a peg (the pieces that meet this way are balls in play)");
	}
	else if (!theOther->mIsPeg)
		theOther->BounceOffPeg(this);
}

// 0x477110: two balls in play meet and trade the speed along the line between them (95 percent of it), and are pushed apart
void Ball::BounceBalls(Ball* theOther)
{
	SexyVector2 aNormal = NormalizeVector(SexyVector2((float)((double)theOther->mX - mX), (float)((double)theOther->mY - mY)));
	float aVelocityX = mVelocityX;
	float aVelocityY = mVelocityY;
	float aOtherVelocityX = theOther->mVelocityX;
	float aOtherVelocityY = theOther->mVelocityY;
	float aRelativeX = (float)((double)aVelocityX - aOtherVelocityX);
	float aRelativeY = (float)((double)aVelocityY - aOtherVelocityY);
	float aForce = (float)((double)aRelativeX * aNormal.x + (double)aNormal.y * aRelativeY);
	aForce = (float)((double)aForce * BALL_BALL_KEEP);
	float aTwice = (float)((double)aForce + aForce);
	float aImpact = (float)std::fabs((double)aTwice);
	theOther->mUnknown170 = aImpact;
	mUnknown170 = aImpact;
	float aHalf = (float)((double)aTwice * 0.5);
	float aGive = -aHalf;
	float aGiveX = (float)((double)aNormal.x * aGive);
	float aGiveY = (float)((double)aNormal.y * aGive);
	float aNewX = (float)((double)aVelocityX + aGiveX);
	float aNewY = (float)((double)aVelocityY + aGiveY);
	float aTakeX = (float)((double)aNormal.x * aHalf);
	float aTakeY = (float)((double)aHalf * aNormal.y);
	float aOtherNewX = (float)((double)aOtherVelocityX + aTakeX);
	float aOtherNewY = (float)((double)aTakeY + aOtherVelocityY);
	float aRadius = mRadius;
	float aOtherRadius = theOther->mRadius;
	theOther->mVelocityX = aOtherNewX;
	theOther->mVelocityY = aOtherNewY;
	mVelocityX = aNewX;
	mVelocityY = aNewY;
	float aSeparateX = (float)((double)mX - theOther->mX);
	float aSeparateY = (float)((double)mY - theOther->mY);
	double aRadii = (double)aOtherRadius + aRadius;
	float aSquareX = (float)((double)aSeparateX * aSeparateX);
	float aSquareY = (float)((double)aSeparateY * aSeparateY);
	float aDistance = (float)std::sqrt((double)(float)((double)aSquareX + aSquareY));
	float aOverlap = (float)(aRadii - aDistance);
	if (aOverlap > 0)
	{
		float aPushX = (float)((double)aNormal.x * aOverlap);
		float aPushY = (float)((double)aOverlap * aNormal.y);
		float aHalfPushX = (float)((double)aPushX * 0.5);
		float aHalfPushY = (float)(0.5 * (double)aPushY);
		theOther->Translate(aHalfPushX, aHalfPushY);
		Translate(-aHalfPushX, -aHalfPushY);
	}
	NoteContact(this, theOther);
}

// 0x477340: this ball hits a round peg, along the line between their middles: the speed along it is turned round (by the peg's
// bounciness, or more when it is fast) and the ball is pushed out of the peg
void Ball::BounceOffPeg(Ball* thePeg)
{
	float aDeltaX = (float)((double)mX - thePeg->mX);
	float aDeltaY = (float)((double)mY - thePeg->mY);
	float aSquareX = (float)((double)aDeltaX * aDeltaX);
	float aSquareY = (float)((double)aDeltaY * aDeltaY);
	float aDistSq = (float)((double)aSquareX + aSquareY);
	float aRadius = mRadius;
	float aPegRadius = thePeg->mRadius;
	SexyVector2 aNormal = NormalizeVector(SexyVector2((float)((double)thePeg->mX - mX), (float)((double)thePeg->mY - mY)));
	float aVelocityX = mVelocityX;
	float aVelocityY = mVelocityY;
	float aPegVelocityX = thePeg->mVelocityX;
	float aPegVelocityY = thePeg->mVelocityY;
	mUnknown168 = -aNormal.x;
	mUnknown16C = -aNormal.y;
	float aBounciness = thePeg->mBounciness;
	float aRolliness = thePeg->mRolliness;
	if (aBounciness == 0)
		aBounciness = PEG_BOUNCINESS;
	if (aRolliness == 0)
		aRolliness = PEG_ROLLINESS; // ModVal 1,533 of Ball.cpp
	float aRelativeX = (float)((double)aVelocityX - aPegVelocityX);
	float aRelativeY = (float)((double)aVelocityY - aPegVelocityY);
	float aForce = (float)((double)aRelativeY * aNormal.y + (double)aRelativeX * aNormal.x);
	aForce = (float)((double)aForce * aBounciness);
	float aAbsForce = (float)std::fabs((double)aForce);
	if (aRolliness >= aAbsForce)
		mUnknown148 = 0;
	else
		aForce = (float)((double)aForce + aForce);
	mUnknown170 = (float)std::fabs((double)aForce);
	if (!mFireball || thePeg->mPegInfo.get() == nullptr)
	{
		float aShare = (float)((double)aForce / PEG_FORCE_DIVISOR);
		aShare = (float)(-(double)aShare * PEG_FORCE_FACTOR);
		float aPushX = (float)((double)aNormal.x * aShare);
		float aPushY = (float)((double)aShare * aNormal.y);
		mVelocityX = (float)((double)aVelocityX + aPushX);
		mVelocityY = (float)((double)aVelocityY + aPushY);
		double aRadii = (double)aPegRadius + aRadius;
		float aDistance = (float)std::sqrt((double)aDistSq);
		float aOverlap = (float)(aRadii - aDistance);
		float aMoveX = (float)((double)aNormal.x * aOverlap);
		float aMoveY = (float)((double)aOverlap * aNormal.y);
		Translate(-aMoveX, -aMoveY);
		if (thePeg->mBallStopReset)
			mUnknownE4 = 0;
	}
	NoteContact(this, thePeg);
}

// 0x4775f0: this ball hits a wall (a line or a polygon) at theContact, where the wall's side is theNormal and it moves by
// theBounce (and its surface by theSurface): the ball goes to the contact and its speed is turned round by the wall's bounciness,
// or, when it is only running along the wall, it keeps the part of its speed along it
void Ball::BounceOffWall(SexyVector2& theVelocity, PhysObj* theWall, const SexyVector2& theNormal, const SexyVector2& theContact,
						 const SexyVector2& theBounce, SexyVector2& theSurface)
{
	mUnknown168 = theNormal.x;
	mUnknown16C = theNormal.y;
	if (mFireball && theWall->mPegInfo.get() != nullptr)
	{
		NoteContact(this, theWall);
		return;
	}
	SetPos(theContact.x, theContact.y);
	float aRelativeX = (float)((double)theVelocity.x - theBounce.x);
	float aRelativeY = (float)((double)theVelocity.y - theBounce.y);
	float aSurfaceRelativeX = (float)((double)aRelativeX - theSurface.x);
	float aSurfaceRelativeY = (float)((double)aRelativeY - theSurface.y);
	float aApproach = (float)((double)theNormal.y * aSurfaceRelativeY + (double)theNormal.x * aSurfaceRelativeX);
	aApproach = -aApproach;
	if (aApproach < 0)
	{
		mUnknown170 = 0;
		return;
	}
	mUnknown170 = aApproach;
	float aBounciness = theWall->mBounciness;
	float aRolliness = theWall->mRolliness;
	if (aBounciness == 0)
		aBounciness = WALL_BOUNCINESS; // Ball.cpp 2,674
	if (aRolliness == 0)
		aRolliness = WALL_ROLLINESS; // Ball.cpp 3,675
	float aAbsNormalX = (float)std::fabs((double)theNormal.x);
	if (aAbsNormalX > UPRIGHT_WALL)
		aApproach = (float)((double)aApproach * UPRIGHT_FACTOR);
	if (aRolliness < aApproach)
	{
		// it bounces
		float aAlong = (float)((double)theVelocity.y * theNormal.y + (double)theVelocity.x * theNormal.x);
		if (aAlong < 0)
		{
			float aAlongAgain = (float)((double)theVelocity.x * theNormal.x + (double)theVelocity.y * theNormal.y);
			float aProjectedX = (float)((double)theNormal.x * aAlongAgain);
			float aProjectedY = (float)((double)aAlongAgain * theNormal.y);
			float aTwiceX = (float)((double)aProjectedX * 2.0);
			float aTwiceY = (float)(2.0 * (double)aProjectedY);
			float aReflectedX = (float)((double)theVelocity.x - aTwiceX);
			float aReflectedY = (float)((double)theVelocity.y - aTwiceY);
			theVelocity.x = (float)((double)aReflectedX * aBounciness);
			theVelocity.y = (float)((double)aReflectedY * aBounciness);
		}
		float aWallX = (float)((double)theBounce.x + theSurface.x);
		float aWallY = (float)((double)theBounce.y + theSurface.y);
		float aFactor = (float)((double)aBounciness + 1.0);
		float aGiveX = (float)((double)aWallX * aFactor);
		float aGiveY = (float)((double)aFactor * aWallY);
		float aAway = (float)((double)theNormal.y * aGiveY + (double)theNormal.x * aGiveX);
		if (aAway > 0)
			Translate(aGiveX, aGiveY);
		theVelocity.x = (float)((double)aGiveX + theVelocity.x);
		theVelocity.y = (float)((double)aGiveY + theVelocity.y);
		mVelocityX = theVelocity.x;
		mVelocityY = theVelocity.y;
		float aMaxSpeed = theWall->mMaxBounceVelocity;
		if (aMaxSpeed > 0)
		{
			float aMaxSquare = (float)((double)aMaxSpeed * aMaxSpeed);
			if (aMaxSquare < (double)mVelocityY * mVelocityY + (double)mVelocityX * mVelocityX)
			{
				SexyVector2 aDirection = NormalizeVector(SexyVector2(mVelocityX, mVelocityY));
				mVelocityX = (float)((double)aDirection.x * aMaxSpeed);
				mVelocityY = (float)((double)aMaxSpeed * aDirection.y);
			}
		}
	}
	else
	{
		// it rolls along the wall
		mUnknown148 = 0;
		float aAlong = (float)((double)theVelocity.y * theNormal.y + (double)theVelocity.x * theNormal.x);
		float aAlongX = (float)((double)theNormal.x * aAlong);
		float aAlongY = (float)((double)aAlong * theNormal.y);
		float aRemainX = (float)((double)theVelocity.x - aAlongX);
		float aRemainY = (float)((double)theVelocity.y - aAlongY);
		float aSurfaceAlong = (float)((double)theNormal.y * theSurface.y + (double)theNormal.x * theSurface.x);
		float aSurfaceX = 0;
		float aSurfaceY = 0;
		if (aSurfaceAlong > 0)
		{
			float aSurfaceAlongAgain = (float)((double)theNormal.x * theSurface.x + (double)theNormal.y * theSurface.y);
			aSurfaceX = (float)((double)theNormal.x * aSurfaceAlongAgain);
			aSurfaceY = (float)((double)aSurfaceAlongAgain * theNormal.y);
		}
		theSurface.x = aSurfaceX;
		theSurface.y = aSurfaceY;
		mVelocityX = (float)(((double)theBounce.x + aRemainX) + theSurface.x);
		mVelocityY = (float)(((double)theBounce.y + aRemainY) + theSurface.y);
	}
	if (theWall->mBallStopReset)
		mUnknownE4 = 0;
	NoteContact(this, theWall);
}

// 0x475200: a ball that hit nothing in the update moves by its speed
void Ball::DoNonCollideUpdate()
{
	Translate(mVelocityX, mVelocityY);
}

// 0x47f160: this ball moves for the rest of the update, hitting what the collision manager has found it may meet (theCandidates):
// each hit takes the ball to the place of the hit, turns its speed and goes on with the time that is left, up to 20 hits. The place
// of the first hit is given back in theFirstContact. False when the ball is not one that moves, true when it hit.
bool Ball::DoCollideUpdate(std::vector<PhysObj*>& theCandidates, SexyVector2* theFirstContact)
{
	if (mIsPeg || (!mCollidable && !mIsHat))
		return false;
	float aTimeLeft = 1.0f;
	float aTime = 1.0f;
	mUnknown141 = true;
	bool aHit = false;
	gAllowParallelContact = true;
	int aHitCount = 0;
	while (true)
	{
		SexyVector2 aPos(mX, mY);
		SexyVector2 aVelocity(mVelocityX, mVelocityY);
		SexyVector2 aContact(0, 0);
		SexyVector2 aBounce(0, 0);
		SexyVector2 aSurface(0, 0);
		SexyVector2 aNormal(0, 0);
		PhysObj* aWall = nullptr;
		if (theCandidates.size() < 1)
			break;
		for (size_t i = 0; i < theCandidates.size(); i++)
		{
			PhysObj* anObj = theCandidates[i];
			if (anObj->mKind == KIND_LINE)
			{
				if (static_cast<Line*>(anObj)->CheckCollision(aPos, aVelocity, mRadius, aTime, aContact, aNormal, aBounce, true))
				{
					aSurface = SexyVector2(0, 0);
					aWall = anObj;
				}
			}
			else if (anObj->mKind == KIND_POLY)
			{
				if (static_cast<Poly*>(anObj)->CheckCollision(aPos, aVelocity, mRadius, aTime, aContact, aNormal, aBounce, aSurface))
					aWall = anObj;
			}
			else if (anObj->mKind == KIND_PEG)
			{
				if (static_cast<Ball*>(anObj)->CheckCollision(aPos, aVelocity, mRadius, aTime, aContact))
					aWall = anObj;
			}
		}
		if (aWall == nullptr)
			break;
		if (!mFireball || aWall->mPegInfo.get() == nullptr)
		{
			mUnknown142 = true;
			mUnknown165 = true;
			aHit = true;
			if (theFirstContact != nullptr && aHitCount == 0)
				*theFirstContact = aContact;
			if (aWall->mKind == KIND_PEG)
			{
				SetPos(aContact.x, aContact.y);
				BounceOffPeg(static_cast<Ball*>(aWall));
			}
			else
				BounceOffWall(aVelocity, aWall, aNormal, aContact, aBounce, aSurface);
			mTrail[5][0] = mX;
			mTrail[5][1] = mY;
			aTime = (float)((double)aTimeLeft - aTime);
			aTimeLeft = aTime;
		}
		else
		{
			// a fireball goes through the pegs: this one is not looked at again
			NoteContact(this, aWall);
			for (size_t i = 0; i < theCandidates.size(); i++)
			{
				if (theCandidates[i] == aWall)
				{
					theCandidates.erase(theCandidates.begin() + i);
					break;
				}
			}
		}
		aHitCount++;
		if (aHitCount >= MAX_HITS_AN_UPDATE)
			return aHit;
	}
	mX = (float)((double)mVelocityX * aTimeLeft + mX);
	mY = (float)((double)aTimeLeft * mVelocityY + mY);
	return aHit;
}

// 0x47a350
void Ball::SetHoleOut(PhysObj* thePiece, int theDelay)
{
	if (0 < theDelay)
	{
		SetActive(false);
		mUnknown14C = theDelay;
		mUnknownPiece15C = thePiece;
	}
	else
		SetAbsPos(thePiece->GetXPos(), thePiece->GetYPos());
}
