// Ball.cpp: see Ball.h. The physics and the drawing are not translated yet.

#include "Ball.h"

#include "DataSync.h"
#include "NotTranslated.h"
#include "PhysObjFactory.h"

using namespace Sexy;

static const float DEFAULT_RADIUS = 6.0f;		// the original's global at 0x649d78 (nothing writes to it)
static const int NO_TIMER = 1000000;			// mUnknown148 until set
static const int NO_VALUE_158 = -1000;			// mUnknown158 until set
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
	mUnknown10C = 0;
	mUnknown110 = 0;
	for (int i = 0; i < 8; i++)
		mUnknown114[i] = 0;
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
		theSync.SyncFloat(mUnknown134);
		theSync.SyncFloat(mUnknown138);
		theSync.SyncLong(mUnknownE4);
		theSync.SyncFloat(mUnknown10C);
		theSync.SyncFloat(mUnknown110);
		if (theSync.mVersion >= VERSION_PEG_ROW)
		{
			for (int i = 0; i < 8; i++)
				theSync.SyncFloat(mUnknown114[i]);
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
	mUnknown134 = mX;
	mUnknown138 = mY;
	mUnknown104 = mX;
	mUnknown108 = mY;
}

// 0x477070: the ball has been put in the game, so it has been here all along
void Ball::AddedToGame()
{
	mPreviousX = mX;
	mPreviousY = mY;
	mUnknown134 = mX;
	mUnknown138 = mY;
}

// 0x480f50
void Ball::SetHat(bool theHasHat, bool theMakeBall)
{
	NOT_TRANSLATED("Ball::SetHat");
}

// 0x47a450
void Ball::Update()
{
	NOT_TRANSLATED("Ball::Update");
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

// 0x477a00
bool Ball::IsCollidableWith(const PhysObj* theOther) const
{
	NOT_TRANSLATED("Ball::IsCollidableWith");
}
