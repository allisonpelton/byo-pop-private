// Poly.cpp: see Poly.h. Drawing is not translated yet (Draw, DrawShadow). The arithmetic follows the original's x87 code as Line.cpp
// explains: a statement with several operations keeps the doubles of its steps and is rounded to a float once.

#include "Poly.h"

#include "DataSync.h"
#include "NotTranslated.h"
#include "Res.h"
#include "ThunderMath.h"

#include <cmath>

using namespace Sexy;

static const float BOX_EMPTY_LIMIT = 1e12f;	  // 0x5368d4a5: a box that has no lines starts as -this to this
static const int GROW_TIME = 20;			  // Poly.cpp:95,791: updates a piece takes to shrink away or to grow back
static const int MOVE_TOGGLE_FLIP_TIME = 40; // Poly.cpp:101,952: the update of a "movetoggle" piece's run at which it turns over
static const int COMEBACK_COUNTDOWN = 100;	  // Poly.cpp:102,973: updates a piece that is growing back waits
static const int GROW_SHRINKING = 0x100;	  // in mGrowType: the piece is shrinking away, and stays visible until it has
static const int GROW_COMES_BACK = 0x1000;	  // in mGrowType: the piece comes back
static const int VERSION_GROW_STATE = 0x23;	  // the files from this version on keep the grow state
static const int MOVE_TOGGLE_WINDOW = 10;	  // the first updates of a mover's cycle in which a "movetoggle" piece counts as at its first end
static const int MAX_POINTS = 100000;		  // a list of points longer than this is not a level

// 0x481670
Poly::Poly()
{
	mX = 0;
	mY = 0;
	ResetState();
	mConvex = true;
	mClosed = true;
	mKind = KIND_POLY;
	mIsRectangle = false;
}

// 0x481770
Poly::~Poly()
{
}

// 0x476b90
void Poly::ResetState()
{
	mRotation = 0;
	mAngularVelocity = 0;
	mNormalDir = 1;
	mPreviousRotation = 0;
	mGrowTimer = 0;
	mGrowType = 0;
	mScale = 1.0f;
	mCountdown = 0;
	mUnknown12F = false;
	mMirrorImage = false;
	mUnknown144 = true;
	mRotationDirty = false;
	mPositionDirty = false;
}

// 0x476d70
void Poly::SetPos(float theX, float theY)
{
	mPositionDirty = true;
	float aDeltaX = theX - mX;
	float aDeltaY = theY - mY;
	mX = theX;
	mY = theY;
	mLeft = aDeltaX + mLeft;
	mRight = aDeltaX + mRight;
	mTop = aDeltaY + mTop;
	mBottom = aDeltaY + mBottom;
}

// 0x476d30
void Poly::Translate(float theDeltaX, float theDeltaY)
{
	SetPos(mX + theDeltaX, mY + theDeltaY);
}

// 0x47bb90
void Poly::ActualSetPos(float theX, float theY)
{
	float aDeltaX = theX - mX;
	mPositionDirty = false;
	float aDeltaY = theY - mY;
	mX = theX;
	mY = theY;
	mLeft = aDeltaX + mLeft;
	mRight = aDeltaX + mRight;
	mTop = aDeltaY + mTop;
	mBottom = aDeltaY + mBottom;
	for (size_t i = 0; i < mLines.size(); i++)
		mLines[i]->SetPos(mX + mLineOffsets[i].x, mLineOffsets[i].y + mY);
}

// 0x47bcd0
void Poly::SetVelocity(float theDeltaX, float theDeltaY)
{
	for (size_t i = 0; i < mLines.size(); i++)
		mLines[i]->SetVelocity(theDeltaX, theDeltaY);
}

// 0x481750
void Poly::GetCenter(float* theCenter) const
{
	theCenter[0] = mX;
	theCenter[1] = mY;
}

// 0x47bab0
void Poly::CalcBoundingBox()
{
	mTop = BOX_EMPTY_LIMIT;
	mLeft = BOX_EMPTY_LIMIT;
	mBottom = -BOX_EMPTY_LIMIT;
	mRight = -BOX_EMPTY_LIMIT;
	for (size_t i = 0; i < mLines.size(); i++)
	{
		const Line* aLine = mLines[i];
		if (aLine->mLeft < mLeft)
			mLeft = aLine->mLeft;
		if (mRight < aLine->mRight)
			mRight = aLine->mRight;
		if (aLine->mTop < mTop)
			mTop = aLine->mTop;
		if (mBottom < aLine->mBottom)
			mBottom = aLine->mBottom;
	}
}

// 0x47a010
void Poly::CalcRotatedBoundingBox(float theAngle)
{
	mTop = BOX_EMPTY_LIMIT;
	mLeft = BOX_EMPTY_LIMIT;
	mBottom = -BOX_EMPTY_LIMIT;
	mRight = -BOX_EMPTY_LIMIT;
	float aCos = (float)std::cos((double)theAngle);
	float aSin = (float)std::sin((double)theAngle);
	for (size_t i = 0; i < mPoints.size(); i++)
	{
		float aPointX = mPoints[i].x;
		float aPointY = mPoints[i].y;
		float aX = (float)(((double)aPointX * aCos + (double)aPointY * aSin) + mX);
		float aY = (float)(((double)aCos * aPointY - (double)aPointX * aSin) + mY);
		if (aX < mLeft)
			mLeft = aX;
		if (mRight < aX)
			mRight = aX;
		if (aY < mTop)
			mTop = aY;
		if (mBottom < aY)
			mBottom = aY;
	}
	mRotationDirty = true;
}

// 0x47d880
void Poly::EnsureLines()
{
	if (mRotationDirty)
		InitRotatedFromPoints(mRotation);
	else if (mPositionDirty)
		ActualSetPos(mX, mY);
}

// 0x484730
void Poly::InitFromPoints()
{
	if (mLines.size() != mPoints.size() - 1)
	{
		mLines.clear();
		for (int i = 0; i < (int)mPoints.size() - 1; i++)
		{
			SharedPtr<Line> aLine(new Line(0, 0, 0, 0));
			mLines.push_back(aLine);
			SexyVector2 anOffset(0, 0);
			mLines.back()->mOneSided = mNormalDir != 0;
			mLineOffsets.push_back(anOffset);
		}
	}

	if (mRotation == 0)
	{
		for (int i = 0; i < (int)mPoints.size() - 1; i++)
		{
			const SexyVector2& aStart = mPoints[i];
			const SexyVector2& anEnd = mPoints[i + 1];
			Line* aLine = mLines[i];
			aLine->Init(aStart.x + mX, aStart.y + mY, mX + anEnd.x, anEnd.y + mY);
			mLineOffsets[i] = SexyVector2(aLine->mX1 - mX, aLine->mY1 - mY);
		}
		CalcBoundingBox();
	}
	else
		InitRotatedFromPoints(mRotation);

	CheckShape();
}

// 0x47d5b0
void Poly::InitRotatedFromPoints(float theAngle)
{
	if (mPoints.size() < 2)
		return;

	mRotationDirty = false;
	mPositionDirty = false;
	float aCos = (float)std::cos((double)theAngle);
	float aSin = (float)std::sin((double)theAngle);

	// each point is turned round the position: x cos + y sin, y cos - x sin
	float aFirstX = mPoints[0].x;
	float aFirstY = mPoints[0].y;
	float aPreviousX = (float)((double)aFirstX * aCos + (double)aFirstY * aSin);
	float aPreviousY = (float)((double)aCos * aFirstY - (double)aFirstX * aSin);
	for (int i = 0; i < (int)mPoints.size() - 1; i++)
	{
		float aNextX = mPoints[i + 1].x;
		float aNextY = mPoints[i + 1].y;
		float aTurnedX = (float)((double)aNextX * aCos + (double)aNextY * aSin);
		float aTurnedY = (float)((double)aCos * aNextY - (double)aNextX * aSin);
		Line* aLine = mLines[i];
		aLine->Init(aPreviousX + mX, mY + aPreviousY, aTurnedX + mX, mY + aTurnedY);
		mLineOffsets[i] = SexyVector2(aLine->mX1 - mX, aLine->mY1 - mY);
		aPreviousX = aTurnedX;
		aPreviousY = aTurnedY;
	}
	CalcBoundingBox();
}

// 0x47d3a0
void Poly::CheckShape()
{
	if (mPoints.size() <= 2)
		return;

	const SexyVector2& aFirst = mPoints.front();
	const SexyVector2& aLast = mPoints.back();
	mClosed = aFirst.x == aLast.x && aFirst.y == aLast.y;
	if (!mClosed)
	{
		mConvex = false;
		return;
	}

	mConvex = true;
	float aTurnSign = 0;
	float aPointX = aFirst.x;
	float aPointY = aFirst.y;
	// the edge into the first point, from the one before the last (the last is the first again)
	float aEdgeX = aFirst.x - mPoints[mPoints.size() - 2].x;
	float aEdgeY = aFirst.y - mPoints[mPoints.size() - 2].y;
	for (size_t i = 1; i < mPoints.size(); i++)
	{
		float aNextX = mPoints[i].x;
		float aNextY = mPoints[i].y;
		float aDeltaX = aNextX - aPointX;
		float aDeltaY = aNextY - aPointY;
		float aCross = (float)((double)aDeltaY * aEdgeX - (double)aDeltaX * aEdgeY);
		if ((aTurnSign < 0 && aCross > 0) || (aTurnSign > 0 && aCross < 0))
		{
			mConvex = false;
			return;
		}
		if (aTurnSign == 0 && aCross != 0)
			aTurnSign = aCross;
		aPointX = aNextX;
		aPointY = aNextY;
		aEdgeX = aDeltaX;
		aEdgeY = aDeltaY;
	}
}

// 0x47d810
void Poly::SetRotation(float theAngle)
{
	if (theAngle == mRotation)
		return;
	float aAngle = GetCanonicalAngleRad(theAngle);
	mPreviousRotation = aAngle;
	mRotation = aAngle;
	if (mMover != nullptr)
		mMover->mRotation = aAngle;
	InitRotatedFromPoints(mRotation);
}

// 0x484a80
void Poly::SetScale(float theScale)
{
	if (theScale > 0)
	{
		float aRatio = theScale / mScale;
		for (size_t i = 0; i < mPoints.size(); i++)
		{
			mPoints[i].x = mPoints[i].x * aRatio;
			mPoints[i].y = mPoints[i].y * aRatio;
		}
		mScale = theScale;
		InitFromPoints();
	}
}

// 0x47d930: whether a ray from the point to the right crosses an odd number of the lines
bool Poly::PointInside(float theX, float theY)
{
	EnsureLines();
	int aCrossings = 0;
	for (size_t i = 0; i < mLines.size(); i++)
	{
		const Line* aLine = mLines[i];
		float aX1 = aLine->mX1;
		float aY1 = aLine->mY1;
		float aX2 = aLine->mX2;
		float aY2 = aLine->mY2;
		bool aSpansY = aY1 != aY2 && ((aY1 <= theY && theY < aY2) || (aY2 <= theY && theY < aY1));
		if (aSpansY)
		{
			float aCrossingX = (float)(aX1 + (((double)aX2 - aX1) * ((double)theY - aY1)) / ((double)aY2 - aY1));
			if (aCrossingX > theX)
				aCrossings++;
		}
	}
	return (aCrossings & 1) != 0;
}

// 0x47e390
bool Poly::GetStandardRect(Rect& theRect)
{
	if (!mIsRectangle || mLines.size() != 4 || mRotation != 0)
		return false;
	EnsureLines();
	const Line* aFirst = mLines[0];
	const Line* aThird = mLines[2];
	theRect.mX = (int)aFirst->mX1;
	theRect.mY = (int)aFirst->mY1;
	theRect.mWidth = (int)((double)aThird->mX1 - aFirst->mX1);
	theRect.mHeight = (int)((double)aThird->mY1 - aFirst->mY1);
	return true;
}

// 0x47dfe0
bool Poly::CheckCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, float& theTime,
						  SexyVector2& theContact, SexyVector2& theNormal, SexyVector2& theBounce,
						  SexyVector2& theSurfaceVelocity)
{
	// the ball must be in the box, widened by 20 (as Line::CheckCollision does)
	if (!((float)(mLeft - 20.0) <= (double)thePos.x + theRadius && (double)thePos.x - theRadius <= (float)(mRight + 20.0) &&
		  (float)(mTop - 20.0) <= (double)thePos.y + theRadius && (double)thePos.y - theRadius <= (float)(20.0 + mBottom)))
		return false;

	EnsureLines();
	bool aConvex = mConvex;
	bool aTestInside = mNormalDir == 1 && mClosed; // a ball that is inside a solid shape is pushed out of it
	float aClosestDistance = 10000000.0f;
	Line* aClosestLine = nullptr;
	Line* aHitLine = nullptr;
	int aCrossings = 0;
	for (size_t i = 0; i < mLines.size(); i++)
	{
		Line* aLine = mLines[i];
		if (aLine->CheckCollision(thePos, theVel, theRadius, theTime, theContact, theNormal, theBounce, false))
			aHitLine = aLine;

		if (aTestInside)
		{
			float aToX = thePos.x - aLine->mX1;
			float aToY = thePos.y - aLine->mY1;
			float aDistance = -(float)((double)aLine->mNormalY * aToY + (double)aLine->mNormalX * aToX);
			bool aCandidate = true;
			if (!aConvex)
			{
				float aY1 = aLine->mY1;
				float aY2 = aLine->mY2;
				if (aY1 != aY2 && ((aY1 <= thePos.y && thePos.y < aY2) || (aY2 <= thePos.y && thePos.y < aY1)))
				{
					float aCrossingX = (float)((((double)aLine->mX2 - aLine->mX1) * ((double)thePos.y - aY1)) / ((double)aY2 - aY1) + aLine->mX1);
					if (aCrossingX > thePos.x)
						aCrossings++;
				}
				aCandidate = aDistance > 0.0f;
			}
			if (aCandidate && aDistance < aClosestDistance)
			{
				aClosestDistance = aDistance;
				aClosestLine = aLine;
			}
		}
	}

	if (aConvex && 0.0f < aClosestDistance)
		aCrossings = 1;

	if ((aCrossings & 1) != 0 && aClosestLine != nullptr && theTime != 0.0f)
	{
		// the ball is inside: push it out through the closest line
		theTime = 0.0f;
		float aDepth = (float)((double)aClosestDistance + theRadius);
		float aPushX = (float)((double)aDepth * aClosestLine->mNormalX);
		float aPushY = (float)((double)aDepth * aClosestLine->mNormalY);
		theContact.x = aPushX + thePos.x;
		theContact.y = thePos.y + aPushY;
		SexyVector2 aLineNormal(aClosestLine->mNormalX, aClosestLine->mNormalY);
		aClosestLine->GetWallBounce(aLineNormal, theBounce);
		theNormal = aLineNormal;
		aHitLine = aClosestLine;
	}

	if (aHitLine != nullptr)
	{
		if (mAngularVelocity != 0.0f)
		{
			// the turning of the shape carries the ball round with it
			float aFromCentreX = theContact.x - mX;
			float aFromCentreY = theContact.y - mY;
			theSurfaceVelocity.x = (float)((double)aFromCentreY * mAngularVelocity + theSurfaceVelocity.x);
			theSurfaceVelocity.y = (float)(theSurfaceVelocity.y - (double)mAngularVelocity * aFromCentreX);
		}
		return true;
	}
	return false;
}

// 0x476de0
void Poly::SetActiveWithGrowAnim(bool theActive)
{
	bool aWasShown = mVisible && (mGrowTimer == 0 || (mGrowType & GROW_SHRINKING) == 0);
	SetActive(theActive);
	if (aWasShown != mVisible)
	{
		if (theActive)
			mGrowType &= ~GROW_SHRINKING;
		else
		{
			mGrowType |= GROW_SHRINKING;
			mVisible = true; // it stays until it has shrunk away
		}
		mGrowTimer = GROW_TIME;
	}
}

// 0x47bd50
void Poly::Update()
{
	PhysObj::Update();

	if (mMover != nullptr)
	{
		float aMoverAngle = mMover->mAngle;
		if (mRotation == aMoverAngle)
			mAngularVelocity = 0;
		else
		{
			mAngularVelocity = aMoverAngle - mRotation;
			while ((double)mAngularVelocity > PI_AS_FLOAT)
				mAngularVelocity = (float)((double)mAngularVelocity - TWO_PI_AS_FLOAT);
			while ((double)mAngularVelocity < -(double)PI_AS_FLOAT)
				mAngularVelocity = (float)((double)mAngularVelocity + TWO_PI_AS_FLOAT);
			mPreviousRotation = aMoverAngle;
			mRotation = aMoverAngle;
			CalcRotatedBoundingBox(aMoverAngle);
		}
	}

	if (mLogic == "movetoggle" && mMovingTimer == MOVE_TOGGLE_FLIP_TIME)
	{
		mImageDX = -mImageDX;
		mMirrorImage = !mMirrorImage;
		PlayHitEffects(SOUND_FLIP);
	}

	if (mGrowTimer > 0)
	{
		mGrowTimer--;
		if ((mGrowType & GROW_SHRINKING) != 0)
			mVisible = mGrowTimer != 0;
		else if ((mGrowType & GROW_COMES_BACK) != 0 && mUnknown144)
			mCountdown = COMEBACK_COUNTDOWN;
	}
	if (mCountdown > 0)
		mCountdown--;
}

// 0x47d8c0
void Poly::UpdateSlowMo(float theSubFrame)
{
	if (mMover != nullptr)
	{
		float aSavedAngle = mMover->mAngle;
		PhysObj::UpdateSlowMo(theSubFrame);
		float aNewAngle = mMover->mAngle;
		mMover->mAngle = aSavedAngle;
		mPreviousRotation = aNewAngle;
		if (mRotation != aNewAngle)
			InitRotatedFromPoints(aNewAngle);
	}
}

// 0x47b9d0
void Poly::NotifyCollision()
{
	if (mLogic == "movetoggle" && !mMoving && mMover != nullptr && mMover->mPeriod > 0)
	{
		int aPeriod = mMover->mPeriod;
		if (mMoverTime % aPeriod < MOVE_TOGGLE_WINDOW)
		{
			if (mMirrorImage)
			{
				mMirrorImage = false;
				mImageDX = mImageDX * -1.0f;
			}
			MoveToPhase(0.5f, false);
			PlayHitEffects(SOUND_BUBBLES);
			return;
		}
		if (!mMirrorImage)
		{
			mMirrorImage = true;
			mImageDX = mImageDX * -1.0f;
		}
		MoveToPhase(0.0f, false);
		PlayHitEffects(SOUND_BUBBLES);
	}
}

// 0x487d00
void Poly::SyncCommon(DataSync& theSync, bool theSyncPoints)
{
	PhysObj::SyncState(theSync);
	bool aReading = theSync.IsReading();
	if (aReading)
		ResetState();

	bool aHasMover = mMover != nullptr;
	bool aHasNormalDir = mNormalDir != 1;
	bool aHasRotation = mRotation != 0 && aHasMover; // a piece without a mover keeps its turn in its points, so only a mover's is saved (and read back from the mover)
	bool aHasAngularVelocity = mAngularVelocity != 0;
	bool aHasPosition = !aHasMover;
	bool aHasScale = mScale != 1.0f && gSyncMode == SYNC_LEVEL_FLAGGED;
	bool aHasGrowTimer = mGrowTimer != 0;
	bool aHasGrowType = mGrowType != 0;
	bool aHasCountdown = mCountdown != 0;

	theSync.SyncBoolBit(mIsRectangle);
	theSync.SyncBoolBit(aHasNormalDir);
	theSync.SyncBoolBit(aHasRotation);
	theSync.SyncBoolBit(aHasAngularVelocity);
	theSync.SyncBoolBit(aHasPosition);
	theSync.SyncBoolBit(aHasScale);
	theSync.SyncBoolBit(mUnknown12F);
	theSync.SyncBoolBit(mMirrorImage);
	theSync.EndBit();
	if (theSync.mVersion >= VERSION_GROW_STATE)
	{
		theSync.SyncBoolBit(aHasGrowTimer);
		theSync.SyncBoolBit(aHasGrowType);
		theSync.SyncBoolBit(aHasCountdown);
		theSync.SyncBoolBit(mUnknown144);
		theSync.EndBit();
	}

	if (aHasRotation)
		theSync.SyncFloat(mRotation);
	if (aHasAngularVelocity)
		theSync.SyncFloat(mAngularVelocity);
	if (aHasScale)
		theSync.SyncFloat(mScale);
	if (aHasNormalDir)
		theSync.SyncSignedByte(mNormalDir);
	if (aHasPosition)
	{
		theSync.SyncFloat(mX);
		theSync.SyncFloat(mY);
	}

	if (aReading)
	{
		if (aHasMover)
		{
			if (!gSyncKeepSavedRotation)
				mRotation = mMover->mAngle;
			mX = mMover->mX;
			mY = mMover->mY;
		}
		mPreviousRotation = mRotation;
	}

	if (theSyncPoints)
	{
		// the list of points: how many, then each one's x and y
		if (!aReading)
		{
			int aCount = (int)mPoints.size();
			theSync.SyncLong(aCount);
			for (size_t i = 0; i < mPoints.size(); i++)
			{
				theSync.SyncFloat(mPoints[i].x);
				theSync.SyncFloat(mPoints[i].y);
			}
		}
		else
		{
			mPoints.clear();
			int aCount = 0;
			theSync.SyncLong(aCount);
			if (aCount > MAX_POINTS)
				throw DataSyncException();
			mPoints.reserve(aCount);
			for (int i = 0; i < aCount; i++)
			{
				mPoints.push_back(SexyVector2(0, 0));
				theSync.SyncFloat(mPoints.back().x);
				theSync.SyncFloat(mPoints.back().y);
			}
			InitFromPoints();
		}
	}

	if (aHasGrowTimer)
		theSync.SyncByte(mGrowTimer);
	if (aHasGrowType)
		theSync.SyncLong(mGrowType);
	if (aHasCountdown)
		theSync.SyncShort(mCountdown);
}

// 0x488010
void Poly::SyncState(DataSync& theSync)
{
	SyncCommon(theSync, true);
}

// 0x47a160, 0x47e7e0
void Poly::DrawShadow(Graphics* g)
{
	NOT_TRANSLATED("Poly::DrawShadow");
}

void Poly::Draw(Graphics* g)
{
	NOT_TRANSLATED("Poly::Draw");
}

// 0x486d00
void Poly::MakeRect(float theX, float theY, float theWidth, float theHeight)
{
	mPoints.clear();
	float aHalfWidth = (theWidth - 1.0f) * 0.5f;
	float aHalfHeight = (theHeight - 1.0f) * 0.5f;
	mPoints.push_back(SexyVector2(-aHalfWidth, -aHalfHeight));
	mPoints.push_back(SexyVector2(-aHalfWidth, aHalfHeight));
	mPoints.push_back(SexyVector2(aHalfWidth, aHalfHeight));
	mPoints.push_back(SexyVector2(aHalfWidth, -aHalfHeight));
	mPoints.push_back(SexyVector2(-aHalfWidth, -aHalfHeight));
	mIsRectangle = true;
	mX = aHalfWidth + theX;
	mY = aHalfHeight + theY;
	InitFromPoints();
}
