#include "GullFlock.h"
#include "Graphics.h"
#include "Image.h"
#include "Compat2004.h"
#include <math.h>

using namespace Sexy;

static const double EPSILON = 1e-6;			// 0x510eb0

// port: a helper of the port (Length)
static double Length(const Vec3& v)
{
	return sqrt(v.z * v.z + v.y * v.y + v.x * v.x);
}

// 0x42fb00
static void Normalize(Vec3& v)
{
	double aLength = Length(v);
	if (aLength > EPSILON)
	{
		aLength = 1.0 / aLength;
		v.x *= aLength;
		v.y *= aLength;
		v.z *= aLength;
	}
}

// 0x42fb50
static Vec3 Normalized(const Vec3& v)
{
	Vec3 aResult = v;
	Normalize(aResult);
	return aResult;
}

// 0x430ab0: groups of four to eleven gulls round a place picked in the middle of the map, until there are theCount
GullFlock::GullFlock(Image* theImage, int theCount)
{
	mImage = theImage;
	mScaredGull = NULL;
	mScareTime = 0;
	mTargetGull = theCount;
	mTargetTime = 0;
	mPointerDown = false;
	mPointerX = 0;
	mPointerY = 0;
	while (theCount > 0)
	{
		int aGroupX, aGroupY, aGroupSize;
		do
		{
			aGroupX = CrtRand() % 0x140 + 0xa0;
			aGroupY = CrtRand() % 0xf0 + 0x78;
			aGroupSize = CrtRand() % 8 + 4;
		}
		while (aGroupSize < 1);
		for (int i = 0; i < aGroupSize; i++)
		{
			double anOffsetX = 0xa0 - CrtRand() % 0x140;
			double anOffsetY = 0x78 - CrtRand() % 0xf0;
			Gull* aGull = new Gull();
			aGull->mVel.x = CrtRand() % 0x14 - 10;
			aGull->mVel.y = CrtRand() % 0x14 - 10;
			aGull->mPos.x = anOffsetX * 0.1 + aGroupX;
			aGull->mPos.y = anOffsetY * 0.1 + aGroupY;
			mGulls.push_back(aGull);
			theCount--;
		}
	}
}

GullFlock::~GullFlock()
{
	for (size_t i = 0; i < mGulls.size(); i++)
		delete mGulls[i];
}

void GullFlock::MouseMove(int x, int y)
{
	mPointerX = x;
	mPointerY = y;
}

void GullFlock::MouseDown(int x, int y)
{
	mPointerDown = true;
	mPointerX = x;
	mPointerY = y;
}

void GullFlock::MouseUp(int x, int y)
{
	mPointerDown = false;
	mPointerX = x;
	mPointerY = y;
}

// 0x430620: the gulls within 20 of this one that are ahead of it
void GullFlock::FindNeighbours(Gull* theGull, std::vector<Gull*>& theList)
{
	Vec3 aHeading = Normalized(theGull->mVel);
	for (size_t i = 0; i < mGulls.size(); i++)
	{
		Gull* anOther = mGulls[i];
		if (anOther == theGull)
			continue;
		Vec3 aDelta(anOther->mPos.x - theGull->mPos.x, anOther->mPos.y - theGull->mPos.y, anOther->mPos.z - theGull->mPos.z);
		double aDistance = Length(aDelta);
		if (aDistance >= 20.0)
			continue;
		if (aHeading.x * aDelta.x + aHeading.y * aDelta.y + aHeading.z * aDelta.z > -(0.0 * aDistance))	// 0x542d90 is 0
			theList.push_back(anOther);
	}
}

// 0x4306f0
int GullFlock::StepGull(Gull* g, double theStep)
{
	std::vector<Gull*> aNeighbours;
	FindNeighbours(g, aNeighbours);
	if (!aNeighbours.empty())
	{
		// 0x42fd00 keeps away from each, 60 over the distance
		for (size_t i = 0; i < aNeighbours.size(); i++)
		{
			Gull* anOther = aNeighbours[i];
			Vec3 aDelta(anOther->mPos.x - g->mPos.x, anOther->mPos.y - g->mPos.y, anOther->mPos.z - g->mPos.z);
			double aDistance = Length(aDelta);
			if (aDistance > EPSILON)
			{
				double aScale = 1.0 / aDistance;
				aDelta.x *= aScale;
				aDelta.y *= aScale;
				aDelta.z *= aScale;
			}
			aDistance = 1.0 / aDistance;
			g->mAccel.x -= aDelta.x * 60.0 * aDistance;
			g->mAccel.y -= aDelta.y * 60.0 * aDistance;
			g->mAccel.z -= aDistance * aDelta.z * 60.0;
		}

		// 0x42fe20 turns towards the way the others fly
		Vec3 aSum;
		for (size_t i = 0; i < aNeighbours.size(); i++)
		{
			Vec3 aVel = aNeighbours[i]->mVel;
			double aSpeed = Length(aVel);
			if (aSpeed > EPSILON)
			{
				aSpeed = 1.0 / aSpeed;
				aVel.x *= aSpeed;
				aVel.y *= aSpeed;
				aVel.z *= aSpeed;
			}
			aSum.x += aVel.x;
			aSum.y += aVel.y;
			aSum.z += aVel.z;
		}
		double anInverse = 1.0 / (double) aNeighbours.size();
		Vec3 aHeading = Normalized(g->mVel);
		Vec3 aCross;
		aCross.x = aHeading.y * aSum.z * anInverse - aHeading.z * anInverse * aSum.y;
		aCross.y = anInverse * aSum.x * aHeading.z - aSum.z * anInverse * aHeading.x;
		aCross.z = aHeading.x * anInverse * aSum.y - aHeading.y * anInverse * aSum.x;
		g->mAccel.x += (aHeading.z * aCross.y - aHeading.y * aCross.z) * 15.0;
		g->mAccel.y += (aCross.z * aHeading.x - aHeading.z * aCross.x) * 15.0;
		g->mAccel.z += (aHeading.y * aCross.x - aCross.y * aHeading.x) * 15.0;

		// 0x42fff0 moves towards their middle
		Vec3 aMiddle;
		for (size_t i = 0; i < aNeighbours.size(); i++)
		{
			aMiddle.x += aNeighbours[i]->mPos.x;
			aMiddle.y += aNeighbours[i]->mPos.y;
			aMiddle.z += aNeighbours[i]->mPos.z;
		}
		double aCount = 1.0 / (double) aNeighbours.size();
		Vec3 aToMiddle(aCount * aMiddle.x - g->mPos.x, aCount * aMiddle.y - g->mPos.y, aMiddle.z * aCount - g->mPos.z);
		Normalize(aToMiddle);
		g->mAccel.x += aToMiddle.x * 20.0;
		g->mAccel.y += aToMiddle.y * 20.0;
		g->mAccel.z += aToMiddle.z * 20.0;
	}

	// the box the gulls fly in turns them back (x from 60 to 580, y from 40 to 440)
	if (g->mPos.x < 60.0 && g->mVel.x < 5.0)
		g->mAccel = Vec3(10.0, 0.0, 0.0);
	if (g->mPos.x > 580.0 && g->mVel.x > -5.0)
		g->mAccel = Vec3(-10.0, 0.0, 0.0);
	if (g->mPos.y < 40.0 && g->mVel.y < 5.0)
		g->mAccel = Vec3(0.0, 10.0, 0.0);
	if (g->mPos.y > 440.0 && g->mVel.y > -5.0)
		g->mAccel = Vec3(0.0, -10.0, 0.0);

	g->mPos.z = 0.0;
	g->mVel.z = 0.0;
	g->mAccel.z = 0.0;
	g->mVel.x += theStep * g->mAccel.x;
	g->mVel.y += theStep * g->mAccel.y;
	g->mVel.z += theStep * g->mAccel.z;
	double aDeltaZ = theStep * g->mVel.z;
	g->mPos.x += theStep * g->mVel.x;
	g->mPos.y += theStep * g->mVel.y;
	g->mPos.z += aDeltaZ;

	// the speed is kept between 7 and 20
	double aSpeed = Length(g->mVel);
	if (aSpeed >= EPSILON)
	{
		if (aSpeed < 7.0)
		{
			Normalize(g->mVel);
			g->mVel.x *= 7.0;
			g->mVel.y *= 7.0;
			g->mVel.z *= 7.0;
		}
		else if (aSpeed > 20.0)
		{
			Normalize(g->mVel);
			g->mVel.x *= 20.0;
			g->mVel.y *= 20.0;
			g->mVel.z *= 20.0;
		}
	}
	else
		g->mVel.x = 0.01;

	// the wings beat once for a stretch of flight; slow gulls often glide for a while
	g->mWingTimer -= aSpeed * theStep;
	if (g->mWingTimer < 0.0)
	{
		g->mFrame = (g->mFrame + 1) % FRAMES;		// 0x436dc0, repeating
		if (aSpeed > 12.0 || (CrtRand() & 1) == 0)
			g->mWingTimer = 4.0;
		else
			g->mWingTimer = 50.0;
	}
	g->mAccel = Vec3();
	return (int) aNeighbours.size();
}

// 0x430140: the gulls near the scared one are pushed away from it; the scare ends after three seconds
bool GullFlock::StepScare()
{
	for (size_t i = 0; i < mGulls.size(); i++)
	{
		Gull* anOther = mGulls[i];
		Vec3 aDelta(anOther->mPos.x - mScaredGull->mPos.x, anOther->mPos.y - mScaredGull->mPos.y,
			anOther->mPos.z - mScaredGull->mPos.z);
		double aDistance = Length(aDelta);
		Normalize(aDelta);
		if (aDistance < 20.0)
		{
			anOther->mAccel.x += aDelta.x * 10.0;
			anOther->mAccel.y += aDelta.y * 10.0;
			anOther->mAccel.z += aDelta.z * 10.0;
		}
	}
	mScareTime -= 0.01;
	return mScareTime < 0.0;
}

// 0x430cf0
void GullFlock::Update()
{
	for (size_t i = 0; i < mGulls.size(); i++)
	{
		int aNeighbours = StepGull(mGulls[i], 0.01);
		if (aNeighbours >= 10 && mScaredGull == NULL)
		{
			mScaredGull = mGulls[i];
			mScareTime = 3.0;
		}
	}
	if (mScaredGull != NULL && StepScare())
		mScaredGull = NULL;

	// one gull at a time is pulled towards a place picked at random, for five seconds
	bool aFollowing = false;
	if (mTargetGull < (int) mGulls.size())
	{
		mTargetTime -= 0.01;
		if (mTargetTime >= 0.0)
		{
			Gull* aGull = mGulls[mTargetGull];
			Vec3 aDelta(mTarget.x - aGull->mPos.x, mTarget.y - aGull->mPos.y, mTarget.z - aGull->mPos.z);
			double aDistance = Length(aDelta);
			Normalize(aDelta);
			if (aDistance >= 10.0)
			{
				aGull->mAccel.x += aDelta.x * 6.0;
				aGull->mAccel.y += aDelta.y * 6.0;
				aGull->mAccel.z += aDelta.z * 6.0;
			}
			aFollowing = true;
		}
	}
	if (!aFollowing)
	{
		mTargetGull = CrtRand() % (int) mGulls.size();
		mTargetTime = 5.0;
		mTarget.x = CrtRand() % 0x140 + 0xa0;
		mTarget.y = CrtRand() % 0xf0 + 0x78;
		mTarget.z = 0;
	}

	// the pointer scares the gulls near it, or draws them while a button is held
	int aPointerX = mPointerX < 0x3c ? 0x3c : (mPointerX > 0x243 ? 0x244 : mPointerX);
	int aPointerY = mPointerY < 0x28 ? 0x28 : (mPointerY > 0x1b7 ? 0x1b8 : mPointerY);
	for (size_t i = 0; i < mGulls.size(); i++)
	{
		Gull* aGull = mGulls[i];
		Vec3 aDelta(aPointerX - aGull->mPos.x, aPointerY - aGull->mPos.y, -aGull->mPos.z);
		double aDistance = Length(aDelta);
		Normalize(aDelta);
		if (aDistance < 20.0)
		{
			double aPush = mPointerDown ? 20.0 : -20.0;
			aGull->mAccel.x += aDelta.x * aPush;
			aGull->mAccel.y += aDelta.y * aPush;
			aGull->mAccel.z += aDelta.z * aPush;
		}
	}
}

// 0x430270: black, a picture of the wing at each gull's place (the strip's frame, centred on pivot (4, 3))
void GullFlock::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, 255));
	int aWidth = mImage->GetWidth() / FRAMES;
	for (size_t i = 0; i < mGulls.size(); i++)
	{
		Gull* aGull = mGulls[i];
		g->DrawImage(mImage, (int) aGull->mPos.x - 4, (int) aGull->mPos.y - 3,
			Rect(aGull->mFrame * aWidth, 0, aWidth, mImage->GetHeight()));
	}
}
