#include "Particles.h"
#include "Sprite.h"
#include "Graphics.h"
#include "Color.h"
#include "SexyAppBase.h"
#include "X87.h"

#include <math.h>

using namespace Sexy;

// 0x42b8c0
Particles::Particles()
{
}

// 0x42b8f0
Particles::~Particles()
{
}

// 0x42b900
void Particles::Emit(Sprite* theSprite, double x, double y, double theVelX, double theVelY, int theCount,
					 double theSpread, double theGravity, double theVelYLimit, int theFrameUpdates, bool fades,
					 int theFrame)
{
	if (theCount <= 0)
		return;

	do
	{
		Particle* aParticle = new Particle;
		mParticles.Append(aParticle);
		aParticle->mSprite = theSprite;
		aParticle->mY = y;
		aParticle->mX = x;

		double aSpeed = 0.0;
		if (theSpread > 0.0)
		{
			int aRand = Rand();
			aSpeed = (double)(aRand % (int)(theSpread * 100.0)) * 0.01;
		}

		// Two draws: the x and y speeds each take their own direction
		int anAngle = Rand() % 3600;
		aParticle->mVelX = X87Cos((double)anAngle * 0.0017453292520882224) * aSpeed + theVelX;
		anAngle = Rand() % 3600;
		aParticle->mGravity = theGravity;
		aParticle->mFades = fades;
		aParticle->mVelYLimit = theVelYLimit;
		aParticle->mVelY = X87Sin((double)anAngle * 0.0017453292520882224) * aSpeed + theVelY;

		if (theFrame < 0)
		{
			aParticle->mFrame = 0.0;
			aParticle->mFrameSpeed = (double)theSprite->mColumns / (double)theFrameUpdates;
		}
		else
		{
			aParticle->mFrameSpeed = 0.0;
			aParticle->mFrame = (double)theFrame;
		}

		theCount--;
	}
	while (theCount != 0);
}

// 0x42ba60
void Particles::Update()
{
	for (mParticles.mCurrent = mParticles.mHead; mParticles.mCurrent->mNext != NULL; )
	{
		mParticles.mCurrent = mParticles.mCurrent->mNext;
		Particle* aParticle = mParticles.Current();

		aParticle->mX = aParticle->mVelX + aParticle->mX;
		aParticle->mY = aParticle->mY + aParticle->mVelY;
		if (aParticle->mGravity <= 0.0)
		{
			if (aParticle->mVelY > aParticle->mVelYLimit)
				aParticle->mVelY = aParticle->mVelY + aParticle->mGravity;
		}
		else if (aParticle->mVelY < aParticle->mVelYLimit)
		{
			aParticle->mVelY = aParticle->mVelY + aParticle->mGravity;
		}

		aParticle->mFrame = aParticle->mFrameSpeed + aParticle->mFrame;
		if ((int)aParticle->mFrame >= aParticle->mSprite->mColumns)
			mParticles.RemoveCurrent();
	}
}

// 0x42bb50
void Particles::Draw(Graphics* g)
{
	for (mParticles.mCurrent = mParticles.mHead; mParticles.mCurrent->mNext != NULL; )
	{
		mParticles.mCurrent = mParticles.mCurrent->mNext;
		Particle* aParticle = mParticles.Current();

		if (aParticle->mFades)
		{
			g->SetColorizeImages(true);
			double aColumns = (double)aParticle->mSprite->mColumns;
			int anAlpha = (int)(255.0 / (aColumns / (aColumns - aParticle->mFrame)));
			g->SetColor(Color(255, 255, 255, anAlpha));
		}
		aParticle->mSprite->Draw(g, (int)aParticle->mX, (int)aParticle->mY, true, (int)aParticle->mFrame, 0, false);
		g->SetColorizeImages(false);
	}
}

// 0x42bc40
void Particles::Clear()
{
	mParticles.Clear();
}
