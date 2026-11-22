#include "Shape.h"

using namespace engine;

namespace
{

// 0x43c1a0 (feedingfrenzy2): a point times a number, each coordinate rounded to a float
Vector2f scaled(const Vector2f& thePoint, float theScale)
{
	return Vector2f((float)((double)thePoint.x * (double)theScale), (float)((double)theScale * (double)thePoint.y));
}

// 0x415330 (feedingfrenzy2): a point moved by an offset
Vector2f moved(const Vector2f& thePoint, const Vector2f& theOffset)
{
	return Vector2f((float)((double)thePoint.x + (double)theOffset.x),
	                (float)((double)theOffset.y + (double)thePoint.y));
}

} // namespace

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
	mCenter = moved(mCenter, theOffset);
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
	mCenter = moved(mCenter, theOffset);
}

} // namespace shape
