#include "Obstacles.h"
#include "App.h"
#include "Graphics.h"
#include "Physics.h"
#include <cmath>
#include <cstdio>

using namespace engine;

// 0x42b610
std::string pointText(const Point& p)
{
	char aText[256];
	snprintf(aText, sizeof(aText), "%f,%f", (double)p.x, (double)p.y);
	return aText;
}

// 0x410800
bool Obstacle::contains(Point& p)
{
	return mPosition.x - mExtents.x <= p.x && mPosition.x + mExtents.x >= p.x && mPosition.y - mExtents.y <= p.y &&
		mExtents.y + mPosition.y >= p.y;
}

// The shared part of 0x40f440 and 0x410040: a ball in a corner region is pushed out from the corner nearest it,
// to its radius. Returns whether it was. The x87 code in double: the corner's offsets are stored as floats, the rest
// kept until the position is stored (theDy is the unrounded one the callers keep).
static bool pushFromCorner(const Point& theOrigin, const Point& theExtents, Ball* theBall, BallArray* theBalls,
	float theDx, double theDy, Point& p)
{
	float cx = fabsf(theExtents.x);
	float cy = fabsf(theExtents.y);
	if (theDx < 0.0f)
		cx = -cx;
	if (theDy < 0.0)
		cy = -cy;
	double vx = p.x - ((double)cx + theOrigin.x);
	double vy = p.y - ((double)cy + theOrigin.y);
	double d2 = vy * vy + vx * vx;
	double r = theBall->mRadius;
	if (d2 < r * r)
	{
		double d = sqrt(d2);
		if (d != 0.0)
		{
			d = 1.0 / d;
			vx = d * vx;
			vy = d * vy;
		}
		p.set((float)(vx * r + theOrigin.x + cx), (float)(vy * r + theOrigin.y + cy));
		theBall->mDamping = theBall->mDamping - theBalls->mFriction;
		return true;
	}
	return false;
}

// 0x40fb00
Box::Box(const Point& theOrigin, int theType)
{
	mExtents = Point(BALL_RADIUS, BALL_RADIUS);
	mType = theType;
	mActive = true;
	mImage = nullptr;
	mLetter = 0;
	if (theType == 0)
	{
		mImage = App::instance()->getImage("res\\cube01.png");
		mImage->setPivot(1);
		mImage->setAlphaMode(1, 0x80);
		mImage->addRef();
		addOwnFlags(NODE_DRAWABLE);
	}
	else if (theType == 3)
	{
		setVisible(false);
	}
	setPosition(theOrigin);
	setFont(App::instance()->getFont("res\\fonts\\bembo-bold-18-red.xml"));
}

// 0x40f360
Box::~Box()
{
	if (mImage != nullptr)
		mImage->release();
}

// 0x40f3c0
void Box::draw(Graphics* g)
{
	if (!mActive || !mVisible)
		return;
	g->pushState();
	g->drawImage(mImage);
	if (mLetter != 0)
	{
		g->setHAlign(1);
		g->setVAlign(1);
		g->setColor(1.0f, 1.0f, 0.0f);
		g->drawChar(mLetter);
	}
	g->popState();
}

// 0x40f880: the image's rectangle from the position (the pivot is not taken off)
void Box::layout()
{
	if (mImage == nullptr)
	{
		mBounds = Rect();
	}
	else
	{
		float aBottom = (float)mImage->getHeight() + mPosition.y;
		mBounds.set(mPosition.x, mPosition.y, (float)mImage->getWidth() + mPosition.x, aBottom);
	}
	clearFlags(NODE_LAYOUT);
}

// 0x40f440: a ball overlapping the block is pushed out along the axis it is least deep in, or from a corner
void Box::collide(BallArray* theBalls)
{
	if (!mActive)
		return;
	unsigned int i = 0;
	if (theBalls->mCount == 0)
		return;
	do
	{
		Ball* aBall = theBalls->getBall(i);
		if (aBall->mState != BALL_GONE && !aBall->mFixed && aBall->mState != BALL_SPAWNING)
		{
			// The x87 code in double: dy is kept unrounded to the end; |dy| is compared before it is stored
			float hx = mExtents.x + aBall->mRadius;
			float hy = aBall->mRadius + mExtents.y;
			Point p = *aBall->getPosition();
			float dx = p.x - mPosition.x;
			double dy = (double)p.y - mPosition.y;
			float ax = fabsf(dx);
			float ay;
			if (ax < hx && (ay = (float)fabs(dy), fabs(dy) < hy))
			{
				if (ax <= mExtents.x || ay <= mExtents.y)
				{
					if ((double)hy - ay <= (double)hx - ax)
					{
						if (dy <= 0.0)
						{
							if (-hy <= dy)
								dy = -hy;
						}
						else if (dy <= hy)
						{
							dy = hy;
						}
					}
					else if (dx <= 0.0f)
					{
						if (-hx <= dx)
							dx = -hx;
					}
					else if (dx <= hx)
					{
						dx = hx;
					}
					p.set(dx + mPosition.x, (float)(dy + mPosition.y));
					aBall->mDamping = aBall->mDamping - theBalls->mFriction;
				}
				else
				{
					pushFromCorner(mPosition, mExtents, aBall, theBalls, dx, dy, p);
				}
			}
			aBall->setPosition(p);
		}
		i++;
	} while (i < theBalls->mCount);
}

// 0x40fc70
Obstacle* Box::clone()
{
	Box* aBox = new Box(mPosition, mType);
	aBox->setExtents(mExtents);
	aBox->setLetter(mLetter);
	aBox->setName(mName);
	return aBox;
}

// 0x4109f0
Ramp::Ramp(const Point& theOrigin, int theRotation)
{
	mExtents = Point(BALL_RADIUS, BALL_RADIUS);
	mRotation = theRotation;
	mActive = true;
	mImage = nullptr;
	mLetter = 0;
	const char* aFile;
	if (theRotation == 0)
		aFile = "res\\cube03-0.png";
	else if (theRotation == 90)
		aFile = "res\\cube03-90.png";
	else if (theRotation == 180)
		aFile = "res\\cube03-180.png";
	else
		aFile = "res\\cube03-270.png";
	mImage = App::instance()->getImage(aFile);
	mImage->addRef();
	mImage->setAlphaMode(1, 0x80);
	mImage->setPivot(1);
	addOwnFlags(NODE_DRAWABLE);
	setPosition(theOrigin);
	setFont(App::instance()->getFont("res\\fonts\\bembo-bold-18-red.xml"));
}

// 0x40ff60
Ramp::~Ramp()
{
	if (mImage != nullptr)
		mImage->release();
}

// 0x40ffc0
void Ramp::draw(Graphics* g)
{
	if (!mActive || !mVisible)
		return;
	g->pushState();
	g->drawImage(mImage);
	if (mLetter != 0)
	{
		g->setHAlign(1);
		g->setVAlign(1);
		g->setColor(1.0f, 1.0f, 0.0f);
		g->drawChar(mLetter);
	}
	g->popState();
}

// 0x410910 (the same as the block's)
void Ramp::layout()
{
	if (mImage == nullptr)
	{
		mBounds = Rect();
	}
	else
	{
		float aBottom = (float)mImage->getHeight() + mPosition.y;
		mBounds.set(mPosition.x, mPosition.y, (float)mImage->getWidth() + mPosition.x, aBottom);
	}
	clearFlags(NODE_LAYOUT);
}

// 0x410040: the block's test with the open half: a ball on the slope's side is put on the diagonal (q is the
// radius over the square root of 2), one on the solid side is pushed out like the block's, and the corner regions
// next to the slope are tested per rotation. Unlike the block, it also moves balls in state 4.
void Ramp::collide(BallArray* theBalls)
{
	if (!mActive)
		return;
	unsigned int i = 0;
	if (theBalls->mCount == 0)
		return;
	do
	{
		Ball* aBall = theBalls->getBall(i);
		if (aBall->mState != BALL_GONE && !aBall->mFixed)
		{
			// The x87 code in double: dy is kept unrounded to the end and the region tests are made in double; |dx|,
			// |dy|, q and the slope's first sum are stored as floats
			float hx = mExtents.x + aBall->mRadius;
			float hy = mExtents.y + aBall->mRadius;
			Point p = *aBall->getPosition();
			float dx = p.x - mPosition.x;
			double dy = (double)p.y - mPosition.y;
			double x = dx;
			double ex = mExtents.x;
			double ey = mExtents.y;
			int r = mRotation;
			bool aCorner = false;
			float ax = fabsf(dx);
			float ay = (float)fabs(dy);
			if (hx <= ax || hy <= fabs(dy))
				goto done;
			if (r < 181)
			{
				if (r == 180)
				{
					if ((x <= ex - (dy - ey) || dy <= ey) && (-ex <= x || (-x - ex) - ey <= dy) &&
						(x <= ex || -ey <= dy))
						goto slope;
					aCorner = true;
				}
				else
				{
					if (r == 0)
					{
						if (((-dy - ey) - ex <= x || -ey <= dy) && (x <= ex || dy <= ey - (x - ex)))
						{
							if (-ex <= x || dy <= ey)
								goto slope;
						}
					}
					else
					{
						if (r != 90)
							goto other;
						if (((dy - ey) - ex <= x || dy <= ey) && (x <= ex || (x - ex) - ey <= dy) &&
							(-ex <= x || -ey <= dy))
							goto slope;
					}
					aCorner = true;
				}
			}
			else
			{
				if (r == 270)
				{
					if ((x <= ex - (-dy - ey) || -ey <= dy) && (-ex <= x || dy <= ey - (-x - ex)))
					{
						if (x <= ex || dy <= ey)
							goto slope;
					}
					aCorner = true;
				}
				else
				{
				other:
					if (!aCorner)
						goto slope;
				}
			}
			pushFromCorner(mPosition, mExtents, aBall, theBalls, dx, dy, p);
			goto done;
		slope:
			{
				float q = aBall->mRadius * 0.707f;
				if (r != 0 || dx <= dy)
				{
					if (r != 90 || dx <= -dy)
					{
						if (r != 180 || dy <= dx)
						{
							if (r != 270 || -dy <= dx)
							{
								if ((double)hy - ay <= (double)hx - ax)
								{
									if (dy <= 0.0)
									{
										if (-hy <= dy)
											dy = -hy;
									}
									else if (dy <= hy)
									{
										dy = hy;
									}
								}
								else if (dx <= 0.0f)
								{
									if (-hx <= dx)
										dx = -hx;
								}
								else if (dx <= hx)
								{
									dx = hx;
								}
								p.set(dx + mPosition.x, (float)(dy + mPosition.y));
								aBall->mDamping = aBall->mDamping - theBalls->mFriction;
							}
							else
							{
								float b = q + dx;
								double c = dy + q;
								if (-c < b)
								{
									double t = (b - c) * 0.5f;
									p.set((float)(t + mPosition.x - q), (float)(mPosition.y - t - q));
								}
							}
						}
						else
						{
							float b = q + dx;
							double c = dy - q;
							if (c < b)
							{
								double t = (c + b) * 0.5f;
								p.set((float)(t + mPosition.x - q), (float)(t + mPosition.y + q));
							}
						}
					}
					else
					{
						float b = dx - q;
						double c = dy - q;
						if (b < -c)
						{
							double t = (b - c) * 0.5f;
							p.set((float)(t + mPosition.x + q), (float)(mPosition.y - t + q));
						}
					}
				}
				else
				{
					float b = dx - q;
					double c = dy + q;
					if (b < c)
					{
						double t = (b + c) * 0.5f;
						p.set((float)(t + mPosition.x + q), (float)(t + mPosition.y - q));
					}
				}
			}
		done:
			aBall->setPosition(p);
		}
		i++;
	} while (i < theBalls->mCount);
}

// 0x410b90
Obstacle* Ramp::clone()
{
	Ramp* aRamp = new Ramp(mPosition, mRotation);
	aRamp->setExtents(mExtents);
	aRamp->setLetter(mLetter);
	aRamp->setName(mName);
	return aRamp;
}
