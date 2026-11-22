// Line.cpp: see Line.h. The original's x87 code evaluates one expression in double precision and rounds it to a float where it is
// stored, so a statement with more than one operation keeps the doubles of its steps here and rounds once, and a statement with
// one operation on floats is just that (the two come out the same). A comparison of a sum is made on the unrounded sum.

#include "Line.h"

#include "Color.h"
#include "DataSync.h"
#include "Graphics.h"
#include "ThunderMath.h"

#include <cmath>

using namespace Sexy;

bool Sexy::gAllowParallelContact = false; // 0x650a54

static const float PENETRATION_MARGIN = 0.01f;		// how far inside a radius a ball counts as touching (0x5d5be8, as a double)
static const float PARALLEL_SPEED_LIMIT = 0.2f;		// a ball closing in on the wall faster than this (the unit normal times the speed) is not tested against the side
static const double BOX_MARGIN = 20.0;				// 0x5d4f20: the box test of CheckCollision is made this much wider than the line

// 0x481480
Line::Line(float theX1, float theY1, float theX2, float theY2)
{
	mVelocityX = 0;
	mVelocityY = 0;
	mNormalX = 0;
	mNormalY = 0;
	mKind = KIND_LINE;
	mOneSided = false;
	Init(theX1, theY1, theX2, theY2);
}

// 0x4782f0
void Line::Init(float theX1, float theY1, float theX2, float theY2)
{
	mX1 = theX1;
	mX2 = theX2;
	mY1 = theY1;
	mY2 = theY2;
	mDeltaX = theX2 - theX1;
	mDeltaY = theY2 - theY1;
	mLeft = theX1;
	mRight = theX2;
	mTop = theY1;
	mBottom = theY2;
	if (theX2 < theX1)
		std::swap(mLeft, mRight);
	if (mTop > mBottom)
		std::swap(mTop, mBottom);

	float aAbsDeltaY = std::fabs(mY1 - mY2);
	float aAbsDeltaX = std::fabs(mX1 - mX2);
	mVertical = aAbsDeltaX < aAbsDeltaY;

	SexyVector2 aNormal = NormalizeVector(SexyVector2(mY1 - mY2, mX2 - mX1));
	mNormalX = aNormal.x;
	mNormalY = aNormal.y;
	mOffset = -(float)((double)mNormalY * mY1 + (double)mNormalX * mX1);
}

// 0x478440
void Line::SetPos(float theX, float theY)
{
	mX1 = theX;
	mY1 = theY;
	mX2 = mDeltaX + theX;
	mY2 = mDeltaY + theY;
	mOffset = -(float)((double)mNormalY * theY + (double)mNormalX * theX);
	mLeft = mX1;
	mRight = mX2;
	mTop = mY1;
	mBottom = mY2;
	if (mX2 < mX1)
		std::swap(mLeft, mRight);
	if (mTop > mBottom)
		std::swap(mTop, mBottom);
}

// 0x475620
void Line::Translate(float theDeltaX, float theDeltaY)
{
	SetPos(mX1 + theDeltaX, mY1 + theDeltaY);
}

// 0x475600
void Line::SetVelocity(float theDeltaX, float theDeltaY)
{
	mVelocityX = theDeltaX;
	mVelocityY = theDeltaY;
}

// 0x476390
void Line::Draw(Graphics* g)
{
	g->SetColor(Color(mOutlineColor));
	g->DrawLine((int)mX1, (int)mY1, (int)mX2, (int)mY2);
}

// 0x4814f0
void Line::SyncState(DataSync& theSync)
{
	PhysObj::SyncState(theSync);

	bool aHasVelocityX = mVelocityX != 0;
	bool aHasVelocityY = mVelocityY != 0;
	theSync.SyncBoolBit(aHasVelocityX);
	theSync.SyncBoolBit(aHasVelocityY);
	theSync.SyncBoolBit(mOneSided);
	theSync.EndBit();

	theSync.SyncFloat(mX1);
	theSync.SyncFloat(mY1);
	theSync.SyncFloat(mX2);
	theSync.SyncFloat(mY2);
	if (aHasVelocityX)
		theSync.SyncFloat(mVelocityX);
	if (aHasVelocityY)
		theSync.SyncFloat(mVelocityY);

	// (the editor's sync mode also writes the worked-out numbers; the editor is not part of the game)
	if (theSync.IsReading())
		Init(mX1, mY1, mX2, mY2);
}

// 0x475660: the part of the wall's own movement that goes along the normal, which the ball is given on top of its bounce
void Line::GetWallBounce(const SexyVector2& theNormal, SexyVector2& theBounce) const
{
	float aVelocityX = mVelocityX;
	float aVelocityY = mVelocityY;
	float aAlongNormal = (float)((double)theNormal.y * aVelocityY + (double)theNormal.x * aVelocityX);
	theBounce.x = theNormal.x * aAlongNormal;
	theBounce.y = aAlongNormal * theNormal.y;
}

// 0x475bd0: where the line's segment crosses the circle of radius theRadius around (theX, theY). Returns how many of its two
// crossings are on the segment; the first one's place goes to thePoint1 and the second one's to thePoint2 (or to thePoint1 when
// it is the only one), when they are given.
int Line::IntersectCircle(float theX, float theY, float theRadius, float* thePoint1, float* thePoint2)
{
	float aFromCircleX = mX1 - theX;
	float aFromCircleY = mY1 - theY;
	float aDirectionX = mX2 - mX1;
	float aDirectionY = mY2 - mY1;
	float aA = (float)((double)aDirectionY * aDirectionY + (double)aDirectionX * aDirectionX);
	float aB = (float)((double)aDirectionX * aFromCircleX + (double)aDirectionY * aFromCircleY);
	aB = aB + aB;
	float aDistanceSquared = (float)((double)aFromCircleY * aFromCircleY + (double)aFromCircleX * aFromCircleX);
	float aC = (float)((double)aDistanceSquared - (double)theRadius * theRadius);
	float aDiscriminant = (float)((double)aB * aB - (double)aC * ((double)aA * 4.0));
	if (!(aDiscriminant >= 0.0))
		return 0;

	float aRoot = (float)std::sqrt((double)aDiscriminant);
	float aT1 = (float)(((double)aRoot - aB) / ((double)aA + aA));
	float aT2 = (float)((-(double)aB - aRoot) / ((double)aA + aA));
	float aStartX = aFromCircleX + theX;
	float aStartY = aFromCircleY + theY;

	int aCount = 0;
	if (aT1 >= 0.0 && aT1 <= 1.0)
	{
		if (thePoint1 != nullptr)
		{
			float aStepX = aDirectionX * aT1;
			float aStepY = aT1 * aDirectionY;
			thePoint1[0] = aStepX + aStartX;
			thePoint1[1] = aStepY + aStartY;
		}
		aCount = 1;
	}

	if (aT2 < 0.0 || aT2 > 1.0)
		return aCount;
	if (aCount == 0)
	{
		if (thePoint1 != nullptr)
		{
			float aStepX = aDirectionX * aT2;
			float aStepY = aT2 * aDirectionY;
			thePoint1[0] = aStartX + aStepX;
			thePoint1[1] = aStartY + aStepY;
			return 1;
		}
	}
	else if (thePoint2 != nullptr)
	{
		float aStepX = aDirectionX * aT2;
		float aStepY = aT2 * aDirectionY;
		thePoint2[0] = aStartX + aStepX;
		thePoint2[1] = aStartY + aStepY;
		return aCount + 1;
	}
	return aCount + 1;
}

// 0x478500
bool Line::CheckPenetration(const SexyVector2& thePos, float theRadius, SexyVector2& theContact, SexyVector2& theNormal,
							SexyVector2& theBounce)
{
	float aInnerRadius = theRadius - PENETRATION_MARGIN;
	float aDotProduct = (float)((double)mNormalY * thePos.y + (double)thePos.x * mNormalX);
	float aSignedDistance = aDotProduct + mOffset;
	float aDistance = std::fabs(aSignedDistance);
	if (aInnerRadius < aDistance)
		return false;

	SexyVector2 aPoint(0, 0);
	if (!mVertical)
	{
		if ((double)mLeft > (double)thePos.x + theRadius)
			return false;
		if ((double)mRight < (double)thePos.x - theRadius)
			return false;
	}
	else
	{
		if ((double)mTop > (double)theRadius + thePos.y)
			return false;
		if ((double)mBottom < (double)thePos.y - theRadius)
			return false;
	}

	int aCrossings = IntersectCircle(thePos.x, thePos.y, aInnerRadius, &aPoint.x, nullptr);
	if (aCrossings == 0)
		return false;

	if (aCrossings == 1)
	{
		// the circle is over one end of the segment: bounce off the end that is nearer
		float aToStartX = mX1 - thePos.x;
		float aToStartY = mY1 - thePos.y;
		float aToEndX = mX2 - thePos.x;
		float aToEndY = mY2 - thePos.y;
		float aStartSquaredX = aToStartX * aToStartX;
		float aStartSquaredY = aToStartY * aToStartY;
		float aStartDistance = aStartSquaredX + aStartSquaredY;
		float aEndSquaredX = aToEndX * aToEndX;
		float aEndSquaredY = aToEndY * aToEndY;
		float aEndDistance = aEndSquaredX + aEndSquaredY;

		float aEndX;
		float aEndY;
		float aOffsetX;
		float aOffsetY;
		if (aEndDistance <= aStartDistance)
		{
			aEndX = mX2;
			aEndY = mY2;
			aOffsetX = aEndX - aPoint.x;
			aOffsetY = aEndY - aPoint.y;
			if (aOffsetX == 0.0f && aOffsetY == 0.0f)
			{
				SexyVector2 aAlong = NormalizeVector(SexyVector2(mX2 - mX1, mY2 - mY1));
				aOffsetX = (float)((double)aAlong.x * PENETRATION_MARGIN);
				aOffsetY = (float)((double)PENETRATION_MARGIN * aAlong.y);
			}
		}
		else
		{
			aEndX = mX1;
			aEndY = mY1;
			aOffsetX = aEndX - aPoint.x;
			aOffsetY = aEndY - aPoint.y;
			if (aOffsetX == 0.0f && aOffsetY == 0.0f)
			{
				SexyVector2 aAlong = NormalizeVector(SexyVector2(mX1 - mX2, mY1 - mY2));
				aOffsetX = (float)((double)aAlong.x * PENETRATION_MARGIN);
				aOffsetY = (float)((double)PENETRATION_MARGIN * aAlong.y);
			}
		}
		theContact.x = thePos.x + aOffsetX;
		theContact.y = aOffsetY + thePos.y;

		theNormal = NormalizeVector(SexyVector2(theContact.x - aEndX, theContact.y - aEndY));
		float aPushX = (float)((double)theNormal.x * PENETRATION_MARGIN);
		float aPushY = (float)((double)PENETRATION_MARGIN * theNormal.y);
		theContact.x = theContact.x + aPushX;
		theContact.y = aPushY + theContact.y;
		GetWallBounce(theNormal, theBounce);
		return true;
	}

	// the circle is over the middle of the segment: push it out along the normal
	theNormal.x = mNormalX;
	theNormal.y = mNormalY;
	if (!mOneSided && aSignedDistance < 0.0f)
	{
		theNormal.x = theNormal.x * -1.0f;
		theNormal.y = theNormal.y * -1.0f;
		aSignedDistance = aSignedDistance * -1.0f;
	}
	float aPenetration = theRadius - aSignedDistance;
	float aPushX = theNormal.x * aPenetration;
	float aPushY = aPenetration * theNormal.y;
	theContact.x = thePos.x + aPushX;
	theContact.y = aPushY + thePos.y;
	GetWallBounce(theNormal, theBounce);
	return true;
}

// 0x475890
bool Line::CheckEdgeTimeCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, SexyVector2& theContact,
								  SexyVector2& theNormal, SexyVector2& theBounce, float& theTime)
{
	float aOffset = mOffset;
	float aNormalX = mNormalX;
	float aNormalY = mNormalY;
	if (!mOneSided)
	{
		// a line that is not one-sided is tested from the side the ball is on
		float aToBallX = thePos.x - mX1;
		float aToBallY = thePos.y - mY1;
		float aSide = (float)((double)aToBallY * aNormalY + (double)aToBallX * aNormalX);
		if (aSide < 0.0f)
		{
			aNormalX = aNormalX * -1.0f;
			aNormalY = aNormalY * -1.0f;
			aOffset = -1.0f * aOffset;
		}
	}

	float aClosing = (float)((double)theVel.x * aNormalX + (double)aNormalY * theVel.y);
	if (aClosing > PARALLEL_SPEED_LIMIT)
		return false;

	if (aClosing == 0.0f)
	{
		// the ball runs along the line: it is taken by it only when it is already against it, and only the first time
		if (!gAllowParallelContact)
			return false;
		if (aNormalY != 0.0f)
			return false;

		float aBackX = (float)((double)aNormalX * theRadius);
		float aBackY = (float)((double)aNormalY * theRadius);
		float aEdgeX = thePos.x - aBackX;
		float aEdgeY = thePos.y - aBackY;
		bool anInside;
		if (!mVertical)
			anInside = mLeft <= aEdgeX && mRight >= aEdgeX;
		else
			anInside = mTop <= aEdgeY && mBottom >= aEdgeY;
		if (anInside)
		{
			float aDot = (float)((double)aNormalX * thePos.x + (double)aNormalY * thePos.y);
			float aDistance = std::fabs(aDot + aOffset);
			if (aDistance > theRadius)
				return false;
			theTime = 0.0f;
			theContact = thePos;
			theNormal.x = aNormalX;
			theNormal.y = aNormalY;
			theBounce.x = 0.0f;
			theBounce.y = 0.0f;
			gAllowParallelContact = false;
			return true;
		}
		// (a ball that is not against the segment goes on to the division below, by zero)
	}

	float aPositionDot = (float)((double)thePos.y * aNormalY + (double)aNormalX * thePos.x);
	float aTime = (float)((((double)theRadius - PENETRATION_MARGIN) - aOffset - aPositionDot) / aClosing);
	if (!(aTime >= 0.0f))
		return false;
	if (!(aTime < theTime))
		return false;

	float aMoveX = theVel.x * aTime;
	float aMoveY = aTime * theVel.y;
	float aHitX = aMoveX + thePos.x;
	float aHitY = thePos.y + aMoveY;
	float aBackX = aNormalX * theRadius;
	float aBackY = theRadius * aNormalY;
	float aEdgeX = aHitX - aBackX;
	float aEdgeY = aHitY - aBackY;
	if (!mVertical)
	{
		if (!(mLeft <= aEdgeX && mRight >= aEdgeX))
			return false;
	}
	else
	{
		if (!(mTop <= aEdgeY && mBottom >= aEdgeY))
			return false;
	}

	theTime = aTime;
	float aPushX = (float)((double)aNormalX * PENETRATION_MARGIN);
	float aPushY = (float)((double)PENETRATION_MARGIN * aNormalY);
	theContact.x = aHitX + aPushX;
	theContact.y = aHitY + aPushY;
	theNormal.x = aNormalX;
	theNormal.y = aNormalY;
	GetWallBounce(theNormal, theBounce);
	return true;
}

// 0x4756c0
bool Line::CheckVertexTimeCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, float& theTime,
									SexyVector2& theContact, float theVertexX, float theVertexY, SexyVector2& theNormal,
									SexyVector2& theBounce)
{
	float aToVertexX = thePos.x - theVertexX;
	float aToVertexY = thePos.y - theVertexY;
	float aInnerRadius = theRadius - PENETRATION_MARGIN;

	// the ball's centre is on the circle of radius aInnerRadius around the vertex where a*t*t + b*t + c = 0
	double aSpeedXSquared = (double)theVel.x * theVel.x; // (kept unrounded)
	float aA = (float)((double)theVel.y * theVel.y + aSpeedXSquared);
	float aB = (float)((double)theVel.x * aToVertexX + (double)aToVertexY * theVel.y);
	aB = aB + aB;
	float aDistanceSquared = (float)((double)aToVertexY * aToVertexY + (double)aToVertexX * aToVertexX);
	float aC = (float)((double)aDistanceSquared - (double)aInnerRadius * aInnerRadius);
	float aDiscriminant = (float)((double)aB * aB - (double)aC * ((double)aA * 4.0));
	if (!(aDiscriminant >= 0.0))
		return false;

	float aMinusB = -aB;
	float aRoot = (float)std::sqrt((double)aDiscriminant);
	float aTime = (float)(((double)aMinusB - aRoot) / ((double)aA + aA));
	if (!(aTime >= 0.0f))
		return false;
	if (!(aTime < theTime))
		return false;

	float aSpeed = (float)std::sqrt((double)(float)((double)theVel.y * theVel.y + aSpeedXSquared));
	float aBackedTime = (float)((double)aTime - (double)PENETRATION_MARGIN / aSpeed);
	float aMoveX = theVel.x * aBackedTime;
	float aMoveY = aBackedTime * theVel.y;
	theContact.x = thePos.x + aMoveX;
	theContact.y = thePos.y + aMoveY;
	theTime = aBackedTime > 0.0f ? aBackedTime : 0.0f;

	theNormal = NormalizeVector(SexyVector2(theContact.x - theVertexX, theContact.y - theVertexY));
	GetWallBounce(theNormal, theBounce);
	return true;
}

// 0x478940
bool Line::CheckCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, float& theTime,
						  SexyVector2& theContact, SexyVector2& theNormal, SexyVector2& theBounce, bool theCheckSecondEnd)
{
	// the ball must be in the line's box, widened
	if (!((float)(mLeft - BOX_MARGIN) <= (double)thePos.x + theRadius &&
		  (double)thePos.x - theRadius <= (float)(mRight + BOX_MARGIN) &&
		  (float)(mTop - BOX_MARGIN) <= (double)thePos.y + theRadius &&
		  (double)thePos.y - theRadius <= (float)(mBottom + BOX_MARGIN)))
		return false;

	if (CheckPenetration(thePos, theRadius, theContact, theNormal, theBounce))
	{
		theTime = 0;
		return true;
	}

	bool anEdgeHit = CheckEdgeTimeCollision(thePos, theVel, theRadius, theContact, theNormal, theBounce, theTime);
	bool aVertexHit = CheckVertexTimeCollision(thePos, theVel, theRadius, theTime, theContact, mX1, mY1, theNormal, theBounce);
	bool aHit = aVertexHit || anEdgeHit;
	if (theCheckSecondEnd &&
		CheckVertexTimeCollision(thePos, theVel, theRadius, theTime, theContact, mX2, mY2, theNormal, theBounce))
		aHit = true;
	return aHit;
}
