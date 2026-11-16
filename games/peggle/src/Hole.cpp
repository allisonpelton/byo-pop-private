// Hole.cpp: see Hole.h. The drawing is not translated yet.

#include "Hole.h"

#include "DataSync.h"
#include "NotTranslated.h"
#include "PhysObjFactory.h"

using namespace Sexy;

static const int DEFAULT_WIDTH = 100;
static const int DEFAULT_HEIGHT = 20;

// 0x4810e0
Hole::Hole()
{
	mKind = KIND_HOLE;
	mOutDelay = 0;
	mFlashTimer = 0;
	mScore = 0;
	mX = 0;
	mY = 0;
	mVelocityX = 0;
	mVelocityY = 0;
	mCircular = false;
	SetSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

// 0x481210
void Hole::SyncState(DataSync& theSync)
{
	PhysObj::SyncState(theSync);
	bool aReading = theSync.IsReading();
	if (aReading)
	{
		mFlashTimer = 0;
		mScore = 0;
		mOutDelay = 0;
		mExitPiece = nullptr;
	}

	bool aHasMover = mMover != nullptr;
	bool aHasFlash = mFlashTimer != 0;
	bool aHasPosition = !aHasMover;
	bool aHasScore = mScore != 0;
	bool aHasExit = mExitPiece != nullptr;
	bool aHasOutDelay = mOutDelay != 0;
	bool aHasVelocity = mVelocityX != 0 || mVelocityY != 0;

	theSync.SyncBoolBit(mCircular);
	theSync.SyncBoolBit(aHasFlash);
	theSync.SyncBoolBit(aHasPosition);
	theSync.SyncBoolBit(aHasScore);
	theSync.SyncBoolBit(aHasExit);
	theSync.SyncBoolBit(aHasOutDelay);
	theSync.SyncBoolBit(aHasVelocity);
	theSync.EndBit();
	theSync.SyncLong(mWidth);
	theSync.SyncLong(mHeight);
	if (aHasFlash)
		theSync.SyncShort(mFlashTimer);
	if (aHasScore)
		theSync.SyncLong(mScore);
	if (aHasOutDelay)
		theSync.SyncLong(mOutDelay);
	if (aHasExit)
		theSync.SyncSharedPolymorphic(mExitPiece, MakePhysObj);

	if (aHasPosition)
	{
		theSync.SyncFloat(mX);
		theSync.SyncFloat(mY);
	}
	else if (aReading && aHasMover)
	{
		mX = mMover->mX;
		mY = mMover->mY;
	}

	if (aHasVelocity)
	{
		theSync.SyncFloat(mVelocityX);
		theSync.SyncFloat(mVelocityY);
	}
	else
	{
		mVelocityY = 0;
		mVelocityX = 0;
	}
	SetSize(mWidth, mHeight);
}

// 0x475540
void Hole::SetPos(float theX, float theY)
{
	Translate(theX - mX, theY - mY);
}

// 0x4754f0
void Hole::Translate(float theDeltaX, float theDeltaY)
{
	mX = theDeltaX + mX;
	mY = theDeltaY + mY;
	mLeft = mLeft + theDeltaX;
	mRight = mRight + theDeltaX;
	mTop = mTop + theDeltaY;
	mBottom = theDeltaY + mBottom;
}

// 0x475600
void Hole::SetVelocity(float theDeltaX, float theDeltaY)
{
	mVelocityX = theDeltaX;
	mVelocityY = theDeltaY;
}

// 0x475580
void Hole::SetSize(int theWidth, int theHeight)
{
	mWidth = theWidth;
	mHeight = theHeight;
	mLeft = mX - (float)theWidth * 0.5f;
	mRight = (float)theWidth * 0.5f + mX;
	mTop = mY - (float)theHeight * 0.5f;
	mBottom = (float)theHeight * 0.5f + mY;
}

// 0x47a930
void Hole::Update()
{
	PhysObj::Update();
	if (mFlashTimer != 0)
		mFlashTimer--;
}

// 0x47ce60
void Hole::Draw(Graphics* g)
{
	NOT_TRANSLATED("Hole::Draw");
}
