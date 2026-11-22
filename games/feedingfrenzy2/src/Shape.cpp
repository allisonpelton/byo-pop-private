#include "Shape.h"

#include "VectorMath.h"

#include <cmath>

using namespace engine;

namespace
{

// 0x566850 (feedingfrenzy2)
const float MINUS_ONE = -1.0f;

// 0x4677c0's sign tests: -1 below 0, else 1 (0 included)
float signOf(float theValue)
{
	return theValue < 0.0f ? MINUS_ONE : 1.0f;
}

// 0x43ce10 (feedingfrenzy2): a box against a circle. With the circle's centre inside the box, the gap to the box's
// side the centre is nearest by the corner's diagonals, the normal that side's; else the gap from the box's nearest
// point less the radius, the normal from that point to the centre
void collideBoxCircle(const shape::Box& theBox, const shape::Circle& theCircle, ShapeContact& theContact)
{
	Vector2f aDelta = difference(theCircle.mCenter, theBox.mCenter);
	float anAbsX = fabsf(aDelta.x);
	if (anAbsX < theBox.mDiagonal.x && fabsf(aDelta.y) < theBox.mDiagonal.y)
	{
		// The diagonals through the centre: the one to the upper right corner and the one to the lower right
		Vector2f anUpperCorner(theBox.mDiagonal.x, -theBox.mDiagonal.y);
		float anUpper = (float)dot(anUpperCorner, aDelta);
		double aLower = dot(theBox.mDiagonal, aDelta);
		if (0.0 < aLower)
		{
			if (0.0f < anUpper)
			{
				theContact.mSeparation = (float)((double)aDelta.y - theBox.mDiagonal.y);
				theContact.mNormal = Vector2f(0.0f, 1.0f);
			}
			else
			{
				theContact.mSeparation = (float)(-(double)aDelta.x - theBox.mDiagonal.x);
				theContact.mNormal = Vector2f(MINUS_ONE, 0.0f);
			}
		}
		else if (0.0f < anUpper)
		{
			theContact.mSeparation = (float)((double)aDelta.x - theBox.mDiagonal.x);
			theContact.mNormal = Vector2f(1.0f, 0.0f);
		}
		else
		{
			theContact.mSeparation = (float)(-(double)aDelta.y - theBox.mDiagonal.y);
			theContact.mNormal = Vector2f(0.0f, MINUS_ONE);
		}
		return;
	}
	float aLeft = (float)((double)theBox.mCenter.x - theBox.mDiagonal.x);
	float aRight = (float)((double)theBox.mDiagonal.x + theBox.mCenter.x);
	float aTop = (float)((double)theBox.mCenter.y - theBox.mDiagonal.y);
	float aBottom = (float)((double)theBox.mDiagonal.y + theBox.mCenter.y);
	Vector2f aNearest;
	if (theCircle.mCenter.x < aLeft)
		aNearest.x = aLeft;
	else if (theCircle.mCenter.x <= aRight)
		aNearest.x = theCircle.mCenter.x;
	else
		aNearest.x = aRight;
	if (theCircle.mCenter.y < aTop)
		aNearest.y = aTop;
	else if (theCircle.mCenter.y <= aBottom)
		aNearest.y = theCircle.mCenter.y;
	else
		aNearest.y = aBottom;
	Vector2f aWay = difference(theCircle.mCenter, aNearest);
	float aLength = length(aWay);
	theContact.mSeparation = (float)((double)aLength - theCircle.mRadius);
	theContact.mNormal = scaled(aWay, (float)(1.0 / aLength));
}

// A box's distance from an edge along its normal (0x43b8b0 and 0x5171e9 (feedingfrenzy2)): the centre's distance
// less the box's extent across the edge, each half size's share rounded to a float
float boxEdgeSeparation(const shape::Box& theBox, const shape::Edge& theEdge)
{
	double aCentre = dot(theEdge.mNormal, theBox.mCenter) + theEdge.mDistance;
	float aDown = (float)fabs(dot(theEdge.mNormal, Vector2f(0.0f, theBox.mDiagonal.y)));
	float anAcross = (float)fabs(dot(theEdge.mNormal, Vector2f(theBox.mDiagonal.x, 0.0f)));
	float anExtent = (float)((double)anAcross + aDown);
	return (float)(aCentre - anExtent);
}

// A circle's (0x50ad6a and 0x50ad36 (feedingfrenzy2)): the centre's distance less the radius
float circleEdgeSeparation(const shape::Circle& theCircle, const shape::Edge& theEdge)
{
	return (float)(dot(theEdge.mNormal, theCircle.mCenter) + theEdge.mDistance - theCircle.mRadius);
}

} // namespace

// The centre's part of 0x43dcc0 and 0x43d820 (feedingfrenzy2): turned (0x5085b3), scaled (0x43c1a0), scaled on each
// axis (0x43dc80) and moved (0x43dca0)
Vector2f transformed(const Vector2f& theCenter, const ShapeTransform& theTransform)
{
	Vector2f aCenter = rotated(theCenter, theTransform.mRotation);
	aCenter = scaled(aCenter, theTransform.mScale);
	aCenter = scaled(aCenter, theTransform.mAxisScale.x, theTransform.mAxisScale.y);
	return moved(aCenter, theTransform.mTranslation.x, theTransform.mTranslation.y);
}

bool Shape::overlaps(const Shape& theOther) const
{
	ShapeContact aContact;
	aContact.mSeparation = 0.0f;
	collide(theOther, aContact);
	return aContact.mSeparation <= 0.0f;
}

namespace shape
{

Shape* Box::clone() const
{
	// 0x476b20 (feedingfrenzy2): the copy's centre and diagonal
	Box* aCopy = new Box();
	aCopy->mCenter = mCenter;
	aCopy->mDiagonal = mDiagonal;
	return aCopy;
}

Vector2f Box::getLeft() const
{
	return Vector2f(mCenter.x - mDiagonal.x, mCenter.y);
}

Vector2f Box::getRight() const
{
	return Vector2f(mDiagonal.x + mCenter.x, mCenter.y);
}

void Box::scale(float theScale)
{
	mCenter = scaled(mCenter, theScale);
	mDiagonal = scaled(mDiagonal, theScale);
}

void Box::translate(const Vector2f& theOffset)
{
	mCenter = moved(mCenter, theOffset.x, theOffset.y);
}

Shape* Box::copyTo(Shape* theTarget) const
{
	if (theTarget == nullptr)
		return clone();
	return theTarget->assignFromBox(*this);
}

void Box::transform(const ShapeTransform& theTransform)
{
	mCenter = transformed(mCenter, theTransform);
	mDiagonal.x = (float)((double)mDiagonal.x * theTransform.mScale);
	mDiagonal.y = (float)((double)theTransform.mScale * mDiagonal.y);
}

void Box::collideWithBox(const Box& theFirst, ShapeContact& theContact) const
{
	// The centres' distance on each axis less both half sizes
	float aDeltaX = (float)((double)mCenter.x - theFirst.mCenter.x);
	float aSignX = signOf(aDeltaX);
	float aGapX = (float)((double)(float)((double)aDeltaX * aSignX) - mDiagonal.x - theFirst.mDiagonal.x);
	float aDeltaY = (float)((double)mCenter.y - theFirst.mCenter.y);
	float aSignY = signOf(aDeltaY);
	float aGapY = (float)((double)(float)((double)aDeltaY * aSignY) - mDiagonal.y - theFirst.mDiagonal.y);
	if (aGapY < aGapX)
	{
		theContact.mSeparation = aGapX;
		theContact.mNormal = Vector2f(aSignX, 0.0f);
	}
	else
	{
		theContact.mSeparation = aGapY;
		theContact.mNormal = Vector2f(0.0f, aSignY);
	}
}

void Box::collideWithCircle(const Circle& theFirst, ShapeContact& theContact) const
{
	collideBoxCircle(*this, theFirst, theContact);
	theContact.mNormal = scaled(theContact.mNormal, MINUS_ONE);
}

Shape* Box::assignFromBox(const Box& theSource)
{
	mCenter = theSource.mCenter;
	mDiagonal = theSource.mDiagonal;
	return this;
}

Shape* Box::assignFromCircle(const Circle& theSource)
{
	return theSource.clone();
}

Shape* Box::assignFromEdge(const Edge& theSource)
{
	return theSource.clone();
}

void Box::collideWithEdge(const Edge& theFirst, ShapeContact& theContact) const
{
	theContact.mSeparation = boxEdgeSeparation(*this, theFirst);
	theContact.mNormal = theFirst.mNormal;
}

Shape* Circle::clone() const
{
	Circle* aCopy = new Circle();
	aCopy->mCenter = mCenter;
	aCopy->mRadius = mRadius;
	return aCopy;
}

Vector2f Circle::getLeft() const
{
	return Vector2f(mCenter.x - mRadius, mCenter.y);
}

Vector2f Circle::getRight() const
{
	return Vector2f(mRadius + mCenter.x, mCenter.y);
}

void Circle::scale(float theScale)
{
	mCenter = scaled(mCenter, theScale);
	mRadius = (float)((double)mRadius * (double)theScale);
}

void Circle::translate(const Vector2f& theOffset)
{
	mCenter = moved(mCenter, theOffset.x, theOffset.y);
}

Shape* Circle::copyTo(Shape* theTarget) const
{
	if (theTarget == nullptr)
		return clone();
	return theTarget->assignFromCircle(*this);
}

void Circle::transform(const ShapeTransform& theTransform)
{
	mCenter = transformed(mCenter, theTransform);
	mRadius = (float)((double)mRadius * theTransform.mScale);
}

void Circle::collideWithBox(const Box& theFirst, ShapeContact& theContact) const
{
	collideBoxCircle(theFirst, *this, theContact);
}

void Circle::collideWithCircle(const Circle& theFirst, ShapeContact& theContact) const
{
	Vector2f aWay = difference(mCenter, theFirst.mCenter);
	float aLength = length(aWay);
	theContact.mNormal = aWay;
	theContact.mSeparation = (float)((double)aLength - theFirst.mRadius - mRadius);
	theContact.mNormal = scaled(theContact.mNormal, (float)(1.0 / aLength));
}

Shape* Circle::assignFromBox(const Box& theSource)
{
	return theSource.clone();
}

Shape* Circle::assignFromCircle(const Circle& theSource)
{
	mCenter = theSource.mCenter;
	mRadius = theSource.mRadius;
	return this;
}

Shape* Circle::assignFromEdge(const Edge& theSource)
{
	return theSource.clone();
}

void Circle::collideWithEdge(const Edge& theFirst, ShapeContact& theContact) const
{
	theContact.mSeparation = circleEdgeSeparation(*this, theFirst);
	theContact.mNormal = theFirst.mNormal;
}

Shape* Edge::clone() const
{
	// 0x51713f (feedingfrenzy2): the copy's normal and distance
	Edge* aCopy = new Edge();
	aCopy->mNormal = mNormal;
	aCopy->mDistance = mDistance;
	return aCopy;
}

Shape* Edge::copyTo(Shape* theTarget) const
{
	if (theTarget == nullptr)
		return clone();
	return theTarget->assignFromEdge(*this);
}

Vector2f Edge::getCenter() const
{
	float aBack = -mDistance;
	return Vector2f((float)((double)aBack * mNormal.x), (float)((double)aBack * mNormal.y));
}

void Edge::collideWithBox(const Box& theFirst, ShapeContact& theContact) const
{
	theContact.mSeparation = boxEdgeSeparation(theFirst, *this);
	theContact.mNormal = scaled(mNormal, MINUS_ONE);
}

void Edge::collideWithCircle(const Circle& theFirst, ShapeContact& theContact) const
{
	theContact.mSeparation = circleEdgeSeparation(theFirst, *this);
	theContact.mNormal = scaled(mNormal, MINUS_ONE);
}

Shape* Edge::assignFromBox(const Box& theSource)
{
	return theSource.clone();
}

Shape* Edge::assignFromCircle(const Circle& theSource)
{
	return theSource.clone();
}

Shape* Edge::assignFromEdge(const Edge& theSource)
{
	mNormal = theSource.mNormal;
	mDistance = theSource.mDistance;
	return this;
}

} // namespace shape
