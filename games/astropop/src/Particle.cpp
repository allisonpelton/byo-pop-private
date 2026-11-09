#include "Particle.h"
#include <math.h>

using namespace Sexy;

// 0x402930 and 0x4034a0
Particle::Particle(float theSpeed, float theVX, float theVY, int theAngle, float theGravity, float theHeight,
	const float* theSin, const float* theCos)
{
	mSpeed = theSpeed;
	mGravity = theGravity;
	mEndY = theHeight;
	mVY = theVY;
	mSin = theSin;
	mCos = theCos;
	mVX = theVX;
	mField2C = 0;
	mAge = 0;
	mY = 0;
	mX = 0;
	mAngle = theAngle;
	mDone = false;
	mPaused = false;
	mDelay = 0;

	// the range of the shot: the angle's sine and cosine, the speed and the gravity
	float aSin = theSin[theAngle];
	float aHeight = (aSin * aSin * theSpeed * theSpeed) / (theGravity + theGravity) + theHeight;
	mEndX = (theSpeed * theSin[theAngle] * (1.0f / theGravity) + (float) sqrt((aHeight + aHeight) * (1.0f / theGravity))) *
		10.0f * theCos[theAngle] * theSpeed;

	mImage = NULL;
	mNumCels = 0;
	mFade = false;
	mCelDelay = 0;
	mCelWidth = 0;
	mCel = 0;
	mAdditive = false;
	mHidden = false;
	for (int i = 0; i < 4; i++)
		mStep[i] = 0;
	mLife = -1;
	for (int i = 0; i < 4; i++)
	{
		mColor[i] = 0;
		mTarget[i] = 0;
	}
}

void Particle::SetImage(Image* theImage, int theCelDelay)
{
	mImage = theImage;
	mNumCels = theImage->mNumCols;
	mCelDelay = theCelDelay;
	mCel = 0;
	mCelWidth = theImage->GetWidth() / mNumCels;
}

void Particle::SetColors(const Color& theColor, const Color& theTarget)
{
	mColor[0] = theColor.mRed;
	mColor[1] = theColor.mGreen;
	mColor[2] = theColor.mBlue;
	mColor[3] = theColor.mAlpha;
	mTarget[0] = theTarget.mRed;
	mTarget[1] = theTarget.mGreen;
	mTarget[2] = theTarget.mBlue;
	mTarget[3] = theTarget.mAlpha;
}

void Particle::SetFadeBlue()
{
	mFade = true;
	mStep[1] = -8;
}

void Particle::SetX(float theX)
{
	mX = theX;
	mEndX = theX + mEndX;
}

void Particle::SetY(float theY)
{
	mY = theY;
	mEndY = theY + mEndY;
}

void Particle::Update()
{
	if (mPaused)
		return;

	// the base part: after the delay the particle falls and drifts
	mAge++;
	if ((mX != mEndX || mY != mEndY) && mAge >= mDelay)
	{
		float aVY = (float) (mAge - mDelay) * 0.01f * mGravity + mVY;
		mVY = aVY;
		mX = mVX + mX;
		mY = aVY + mY;
	}

	if (mAge < mDelay)
		return;

	if (mFade)
	{
		// red, alpha and green move towards their targets; blue does too and turns the green's step around when it
		// has got there
		for (int i = 0; i < 4; i++)
		{
			int aStep = mStep[i];
			int aTarget = mTarget[i];
			if (aStep < 0)
			{
				if (mColor[i] > aTarget)
				{
					mColor[i] += aStep;
					if (mColor[i] < aTarget)
					{
						if (i == 2)
							mStep[1] = -8;
						mColor[i] = aTarget;
					}
				}
			}
			else if (aStep > 0)
			{
				if (mColor[i] < aTarget)
				{
					mColor[i] += aStep;
					if (mColor[i] > aTarget)
					{
						if (i == 2)
							mStep[1] = 8;
						mColor[i] = aTarget;
					}
				}
			}
		}
	}

	if (mImage != NULL && !mDone && (mAge - mDelay) % mCelDelay == 0)
	{
		mCel++;
		if (mCel >= mNumCels)
			mDone = true;
	}

	if (mLife > 0)
	{
		mLife--;
		if (mLife < 1)
			mDone = true;
	}
}

void Particle::Draw(Graphics* g)
{
	if (mDelay <= mAge && !mHidden && mImage != NULL && !mDone)
	{
		if (mFade)
		{
			g->SetColor(Color(mColor[0], mColor[1], mColor[2], mColor[3]));
			g->SetColorizeImages(true);
		}
		if (mAdditive)
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImageCel(mImage, (int) mX, (int) mY, mCel);
		if (mAdditive)
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		if (mFade)
			g->SetColorizeImages(false);
	}
}
