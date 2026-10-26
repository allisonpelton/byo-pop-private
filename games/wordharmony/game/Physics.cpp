#include "Physics.h"
#include "App.h"
#include "Graphics.h"
#include "Selector.h"
#include <cctype>
#include <cmath>

using namespace engine;

// 0x40f200
Ball::Ball()
{
	mSpecial = 0;
	mB0 = 0;
	mRadius = 22.0f;
	mInverseMass = 1.0f;
	mDamping = 1.0f;
	mFixed = false;
	mState = BALL_FREE;
	mGravity = nullptr;
	mChain = nullptr;
	mE4 = false;
	mRow = -1;
	mColumn = -1;
	mSelector = nullptr;
	mLetter = ' ';
	mLinked = false;
	addOwnFlags(NODE_DRAWABLE);
	setFont(App::instance()->getFont("res\\fonts\\bembo-bold-18-red.xml"));
}

// 0x40efe0
Ball::~Ball()
{
	delete mGravity;
}

// 0x40ef30
void Ball::draw(Graphics* g)
{
	if (mState == BALL_GONE || !mVisible)
		return;
	g->setSnap(3);
	Container::draw(g);
	g->pushState();
	g->setHAlign(1);
	g->setVAlign(1);
	g->drawChar(mLetter);
	g->popState();
	g->setSnap(0);
}

// 0x40efa0
void Ball::setOn(bool theOn)
{
	if (mSelector == nullptr)
		return;
	static_cast<Selector*>(mSelector)->select(theOn ? 1 : 0, 0.0f, true);
}

// 0x4111b0
void Ball::setLetter(char theLetter)
{
	mLetter = (char)toupper(theLetter);
}

// 0x40ec20
BallArray::BallArray()
{
	mCount = 0;
	mBallsLeft = 0;
	mC = false;
	mGravity = Point(0, 0);
	mFriction = 0.2f;
	mBalls = nullptr;
}

// 0x40ec50
BallArray::~BallArray()
{
	mCount = 0;
	mBallsLeft = 0;
	delete[] mBalls;
	mBalls = nullptr;
}

// 0x40ed10
void BallArray::accumulate(float theTime)
{
	unsigned int i = 0;
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = &mBalls[i];
			if (!aBall->mFixed)
			{
				float x;
				float y;
				if (aBall->mInverseMass <= 0.0f)
				{
					x = mGravity.x;
					y = -mGravity.y;
				}
				else if (aBall->mGravity == nullptr)
				{
					y = mGravity.y;
					x = mGravity.x;
				}
				else
				{
					y = aBall->mGravity->y;
					x = aBall->mGravity->x;
				}
				aBall->mAcceleration.set(x, y);
				aBall->mAcceleration.x = theTime * aBall->mAcceleration.x * theTime;
				aBall->mAcceleration.y = theTime * aBall->mAcceleration.y * theTime;
			}
			else
			{
				aBall->mAcceleration.set(0, 0);
			}
			i++;
		} while (i < mCount);
	}
}

// 0x40ede0
void BallArray::integrate(float theTime, float theLastTime)
{
	unsigned int i = 0;
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = &mBalls[i];
			if (aBall->mState != BALL_GONE)
			{
				if (!aBall->mFixed)
				{
					Point aPos = *aBall->getPosition();
					float vx = aPos.x - aBall->mOld.x;
					float aDampingX = aBall->mDamping;
					float vy = aPos.y - aBall->mOld.y;
					float aDampingY = aBall->mDamping;
					float k = 1.0f;
					if (theLastTime != 0.0f)
						k = theTime / theLastTime;
					float ax = aBall->mAcceleration.x;
					float ay = aBall->mAcceleration.y;
					aBall->mOld = *aBall->getPosition();
					Point* aNow = aBall->getPosition();
					aPos.x = aNow->x + vx * aDampingX * k + ax;
					aPos.y = aNow->y + k * vy * aDampingY + ay;
					aBall->setPosition(aPos);
					aBall->mDamping = 1.0f;
				}
				else
				{
					aBall->mOld = *aBall->getPosition();
				}
			}
			i++;
		} while (i < mCount);
	}
}

// 0x4111d0
unsigned int BallArray::findBall(const Point& thePoint)
{
	unsigned int aFound = 0xffffffff;
	unsigned int i = 0;
	float aBest = 529.0f;
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = getBall(i);
			Point aPos = *aBall->getPosition();
			if (aBall->mState == BALL_FREE)
			{
				float d = (thePoint.y - aPos.y) * (thePoint.y - aPos.y) + (thePoint.x - aPos.x) * (thePoint.x - aPos.x);
				if (d < aBest)
				{
					aFound = i;
					aBest = d;
				}
			}
			i++;
		} while (i < mCount);
	}
	return aFound;
}

// 0x411300
float BallArray::getGap(int theFirst, int theSecond)
{
	Ball* a = getBall(theFirst);
	Ball* b = getBall(theSecond);
	float dx = (fabsf(a->getPosition()->x - b->getPosition()->x) - a->mRadius) - b->mRadius;
	float dy = (fabsf(a->getPosition()->y - b->getPosition()->y) - a->mRadius) - b->mRadius;
	if (dy < dx)
		dy = dx;
	return dy;
}

Follow::Follow(Node* theNode, float theC, float theRate, float theBlend, float theGrid)
{
	mNode = theNode;
	mC = theC;
	mRate = theRate;
	mBlend = theBlend;
	mLength = 0;
	mGrid = theGrid;
	mSnap = false;
}

// 0x40e170: the grid snap rounds towards zero (the x87 conversion) and adds 22, half a cell
void Follow::apply()
{
	if (!mActive)
		return;
	Point p = *mNode->getPosition();
	p.x = (mTarget.x - p.x) * mRate + p.x;
	p.y = (mTarget.y - p.y) * mRate + p.y;
	if (mSnap)
	{
		double k = 1.0 / mGrid;
		p.x = (float)((float)(int)(k * mTarget.x) * mGrid + 22.0);
		p.y = (float)((float)(int)(k * mTarget.y) * mGrid + 22.0);
	}
	mNode->setPosition(p);
}

Rope::Rope(Ball* theBall, const Point& theOffset, float theMaxStep, float theRate, float theBlend)
{
	mBall = theBall;
	mMaxStep = theMaxStep;
	mRate = theRate;
	mBlend = theBlend;
	mLength = 0;
	mOffset = theOffset;
}

// 0x40e2a0
void Rope::apply()
{
	if (!mActive)
		return;
	Point p = *mBall->getPosition();
	p.add(mOffset);
	float dx = mTarget.x - p.x;
	float dy = mTarget.y - p.y;
	float aLength2 = dx * dx + dy * dy;
	float aRest2 = mLength * mLength;
	if (aLength2 > aRest2)
	{
		float anExcess = sqrtf(aLength2) - mLength;
		float k = 1.0f / sqrtf(aLength2);
		dx = k * anExcess * dx;
		dy = k * anExcess * dy;
		float aStep2 = dx * dx + dy * dy;
		if (mMaxStep * mMaxStep < aStep2)
		{
			k = 1.0f / sqrtf(aStep2);
			dx = k * dx * mMaxStep;
			dy = k * dy * mMaxStep;
		}
		p.x = dx * mRate + p.x;
		p.y = dy * mRate + p.y;
		p.subtract(mOffset);
		mBall->setPosition(p);
		float aKeep = 1.0f - mBlend;
		mBall->mOld.x = aKeep * p.x + mBlend * mBall->mOld.x;
		mBall->mOld.y = aKeep * p.y + mBlend * mBall->mOld.y;
	}
}

// 0x40e950
void Distance::apply()
{
	if (!mActive)
		return;
	Point b = *mSecond->getPosition();
	Point a = *mFirst->getPosition();
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float d2 = dx * dx + dy * dy;
	if ((mMode != KEEP_AT_LEAST || d2 <= mDistance2) && (mMode != KEEP_AT_MOST || mDistance2 <= d2) &&
		(mMode != KEEP_EXACTLY || d2 != mDistance2))
	{
		float f = mDistance2 / (d2 + mDistance2) - 0.5f;
		dx = dx * f;
		dy = f * dy;
		a.x = a.x - fabsf(mFirst->mInverseMass) * dx;
		a.y = a.y - fabsf(mFirst->mInverseMass) * dy;
		b.x = fabsf(mSecond->mInverseMass) * dx + b.x;
		b.y = fabsf(mSecond->mInverseMass) * dy + b.y;
		mFirst->setPosition(a);
		mSecond->setPosition(b);
	}
}

Stick::Stick(Ball* theFirst, Ball* theSecond, float theMin2, float theMax2, int theMode)
{
	mFirst = theFirst;
	mSecond = theSecond;
	mMin2 = theMin2;
	mActive = true;
	mMax2 = theMax2;
	mMode = theMode;
}

// 0x40ead0
void Stick::apply()
{
	if (!mActive)
		return;
	Point b = *mSecond->getPosition();
	Point a = *mFirst->getPosition();
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float d2 = dx * dx + dy * dy;
	if ((mMode != 1 || (d2 <= mMin2 && mMax2 <= d2)) && (mMode != 0 || (mMin2 <= d2 && d2 <= mMax2)))
	{
		float f = mMin2 / (d2 + mMin2) - 0.5f;
		dx = dx * f;
		f = f * dy;
		a.x = a.x - fabsf(mFirst->mInverseMass) * dx;
		a.y = a.y - fabsf(mFirst->mInverseMass) * f;
		b.x = fabsf(mSecond->mInverseMass) * dx + b.x;
		b.y = fabsf(mSecond->mInverseMass) * f + b.y;
		mFirst->setPosition(a);
		mSecond->setPosition(b);
	}
}

// 0x40e5d0
Chain::Chain(BallArray* theBalls, int theMode)
{
	mAlong = true;
	mBalls = theBalls;
	mMode = theMode;
}

// 0x40e670
Chain::~Chain()
{
}

// 0x40e3f0
void Chain::apply()
{
	if (!mActive || mIndices.size() <= 1)
		return;
	if (!mAlong)
	{
		std::list<int>::iterator anIt = mIndices.begin();
		if (anIt != mIndices.end())
		{
			Ball* aPrev = mBalls->getBall(*anIt);
			++anIt;
			if (anIt != mIndices.end())
			{
				do
				{
					Ball* aBall = mBalls->getBall(*anIt);
					float d = aBall->mRadius + aPrev->mRadius;
					Distance(aPrev, aBall, d * d, mMode).apply();
					++anIt;
					aPrev = aBall;
				} while (anIt != mIndices.end());
			}
		}
		Ball* aFirst = mBalls->getBall(mIndices.front());
		Ball* aLast = mBalls->getBall(mIndices.back());
		float d = (float)(unsigned int)(mIndices.size() - 1) * (aLast->mRadius + aFirst->mRadius);
		Distance(aFirst, aLast, d * d, mMode).apply();
	}
	else
	{
		unsigned int i = 0;
		if (mBalls->mCount != 1)
		{
			do
			{
				Ball* a = mBalls->getBall(i);
				i++;
				Ball* b = mBalls->getBall(i);
				float d = (b->mRadius + a->mRadius) - 2.0f;
				Distance(a, b, d * d, mMode).apply();
			} while (i < mBalls->mCount - 1);
		}
	}
}

// 0x40e6e0
void Collisions::apply()
{
	if (!mActive)
		return;
	unsigned int i = 0;
	if (mBalls->mCount != 0)
	{
		do
		{
			Ball* a = mBalls->getBall(i);
			if (a->mState != BALL_GONE && !a->mFixed && a->mState != BALL_STATE_4)
			{
				unsigned int j = i + 1;
				if (j < mBalls->mCount)
				{
					do
					{
						Ball* b = mBalls->getBall(j);
						if (b->mState != BALL_GONE && !b->mFixed && b->mState != BALL_STATE_4)
						{
							Point pb = *b->getPosition();
							Point pa = *a->getPosition();
							float dx = pb.x - pa.x;
							float dy = pb.y - pa.y;
							float d2 = dx * dx + dy * dy;
							if (d2 <= 1936.0f)
							{
								float f = 1936.0f / (d2 + 1936.0f) - 0.5f;
								dx = dx * f;
								f = f * dy;
								pa.x = pa.x - fabsf(a->mInverseMass) * dx;
								pa.y = pa.y - fabsf(a->mInverseMass) * f;
								pb.x = fabsf(b->mInverseMass) * dx + pb.x;
								pb.y = fabsf(b->mInverseMass) * f + pb.y;
								a->setPosition(pa);
								b->setPosition(pb);
							}
							if (a->mLinked && b->mLinked)
							{
								float d = b->mRadius + a->mRadius + 3.0f;
								Stick(a, b, 1936.0f, d * d, 0).apply();
							}
						}
						j++;
					} while (j < mBalls->mCount);
				}
			}
			i++;
		} while (i < mBalls->mCount);
	}
}

// 0x410ef0
Bounds::Bounds(BallArray* theBalls, float theSize)
{
	mBalls = theBalls;
	mC = Point(0, 0);
	mBottom = theSize;
	mRight = theSize;
}

// 0x410f60
void Bounds::apply()
{
	if (!mActive)
		return;
	unsigned int i = 0;
	if (mBalls->mCount != 0)
	{
		do
		{
			applyTo(i);
			i++;
		} while (i < mBalls->mCount);
	}
}

// 0x410f90
void Bounds::applyTo(unsigned int theIndex)
{
	Ball* aBall = mBalls->getBall(theIndex);
	if (aBall->mState == BALL_GONE)
		return;
	if (aBall->mState == BALL_STATE_4)
		return;
	Point p = *aBall->getPosition();
	float r = aBall->mRadius;
	float aLimit = r;
	float aHits;
	if (r <= p.x)
	{
		aLimit = mRight - r;
		aHits = 0.0f;
		if (p.x <= aLimit)
			goto checkY;
	}
	p.x = aLimit;
	aHits = 1.0f;
checkY:
	if (p.y > r)
	{
		aLimit = mBottom - r;
		if (aLimit <= p.y)
		{
			aHits = aHits + 1.0f;
			p.y = aLimit;
		}
	}
	else
	{
		aHits = aHits + 1.0f;
		p.y = r;
	}
	if (0.0f < aHits)
		aBall->mDamping = aBall->mDamping - aHits * mBalls->mFriction;
	float aLow = r - 0.05f;
	float x = aLow;
	if (aLow <= aBall->mOld.x)
		x = aBall->mOld.x;
	aBall->mOld.x = x;
	float aHigh = (mRight - r) + 0.05f;
	if (aHigh < x)
		x = aHigh;
	aBall->mOld.x = x;
	float y = aLow;
	if (aLow <= aBall->mOld.y)
		y = aBall->mOld.y;
	aBall->mOld.y = y;
	aHigh = (mBottom - r) + 0.05f;
	if (aHigh < y)
		y = aHigh;
	aBall->mOld.y = y;
	aBall->setPosition(p);
}
