// The effects (RocketManiaGame 0x4172f0-0x41abb0, 0x4194d0): particle emitters for the fireworks, the sparkles and flames
// of the rockets, the leaves on the menu. See Effects.h and docs/GAME_NOTES.md.

#include "Effects.h"
#include "Board.h"
#include "RocketApp.h"
#include "Common.h"
#include "Graphics.h"
#include "Image.h"
#include <math.h>

using namespace Sexy;

// What the constructor (0x4172f0) stores for a kind of effect (generated from it, see EffectKinds.inc)
struct EffectKind
{
	int			mXRandMod, mXRandBase;      // x = GlobalRand() % mod + base (mod 0: the fixed x below)
	bool		mNormalize;                 // the direction is made a unit vector
	bool		mNotifyBoard, mSounds;      // +8, +9
	double		mX, mY, mZ;                 // +0x48
	double		mSpreadX, mSpreadY, mSpreadZ;   // +0x60
	double		mMoveX, mMoveY, mMoveZ;              // +0x78
	double		mAccelX, mAccelY, mAccelZ;              // +0xa0
	double		mDirX, mDirY, mDirZ;        // +0xc8
	double		mDirSpreadX, mDirSpreadY, mDirSpreadZ;  // +0xe0
	double		mSpeed, mSpeedSpread, mDrag;    // +0xf8, +0x100, +0x108
	int			mParticleLife, mParticleLifeSpread;     // +0x110
	int			mCount, mCountSpread, mInterval;        // +0x118
};

#define EFFECT_KINDS 0x18
static const EffectKind sEffectKinds[EFFECT_KINDS] =
{
#include "EffectKinds.inc"
};

// the pool of a manager and the number of particles an effect may have
#define POOL_SIZE 1000
#define MAX_PARTICLES 200
#define EFFECT_LIFE 500

// 1 / 2^31: the generator of the original (0x45b9ae) gives 31 bits
#define RAND_UNIT 4.656612875245797e-10

// port: base plus or minus up to theSpread, evenly
static double SpreadRand(double theBase, double theSpread)
{
	return (((theSpread + theSpread) * (double) GlobalRand() * RAND_UNIT) - theSpread) + theBase;
}

// ---------------------------------------------------------------------------------------------------------
// ParticlePool

// 0x41ac40: the free ones are chained by their next pointers
ParticlePool::ParticlePool(int theCount) :
	mParticles(theCount), mFree(NULL)
{
	for (int anIndex = 0; anIndex < theCount; anIndex++)
		mParticles[anIndex].mNext = (anIndex + 1 < theCount) ? &mParticles[anIndex + 1] : NULL;
	mFree = theCount > 0 ? &mParticles[0] : NULL;
}

// 0x41ad50
Particle* ParticlePool::Take()
{
	if (mFree == NULL)
		return NULL;
	Particle* aParticle = mFree;
	mFree = aParticle->mNext;
	return aParticle;
}

// 0x41ad70
void ParticlePool::Give(Particle* theParticle)
{
	theParticle->mNext = mFree;
	mFree = theParticle;
}

// ---------------------------------------------------------------------------------------------------------
// Effect

// 0x4172f0 (0x416f50, the normalising of a vector, inline in it)
Effect::Effect(RocketApp* theApp, EffectManager* theManager, int theKind, bool theFlag) :
	mApp(theApp), mManager(theManager), mKind(theKind), mNotifyBoard(false), mSounds(false), mColorChoice(0), mSound(-1),
	mFlag(theFlag), mX(0), mY(0), mZ(0), mSpreadX(0), mSpreadY(0), mSpreadZ(0), mMoveX(0), mMoveY(0), mMoveZ(0), mLayer(false),
	mParticles(NULL), mPool(theManager->mPool), mAccelX(0), mAccelY(0), mAccelZ(0), mMaxParticles(MAX_PARTICLES), mParticleCount(0),
	mDelay(0), mLife(EFFECT_LIFE), mDirX(0), mDirY(0), mDirZ(0), mDirSpreadX(0), mDirSpreadY(0), mDirSpreadZ(0), mSpeed(0),
	mSpeedSpread(0), mDrag(0), mParticleLife(0), mParticleLifeSpread(0), mCount(0), mCountSpread(0), mInterval(0)
{
	mColorChoice = GlobalRand() % 3 + 3;

	// six colours the particles start with and six they end with
	mColors[0] = Color(0, 0xff, 0);
	mColors[1] = Color(0xff, 0, 0);
	mColors[2] = Color(0xff, 0xff, 0);
	mColors[3] = Color(0xff, 0x80, 0x80);
	mColors[4] = Color(0xff, 0xff, 0x80);
	mColors[5] = Color(0, 0, 0xff);
	mColors[6] = Color(200, 0xff, 200);
	mColors[7] = Color(0xff, 200, 0xff);
	mColors[8] = Color(0xff, 0xff, 0xff);
	mColors[9] = Color(0xff, 0, 0x80);
	mColors[10] = Color(0xff, 0xff, 0);
	mColors[11] = Color(0, 0x80, 0xff);

	// (a kind that the original does not store, past 0x17, is left with the zeros)
	if (mKind < 0 || mKind >= EFFECT_KINDS)
		return;

	const EffectKind& aKind = sEffectKinds[mKind];
	mNotifyBoard = aKind.mNotifyBoard;
	mSounds = aKind.mSounds;
	mX = aKind.mX;
	if (aKind.mXRandMod != 0)
		mX = (double) (GlobalRand() % aKind.mXRandMod + aKind.mXRandBase);
	mY = aKind.mY;
	mZ = aKind.mZ;
	mSpreadX = aKind.mSpreadX;
	mSpreadY = aKind.mSpreadY;
	mSpreadZ = aKind.mSpreadZ;
	mMoveX = aKind.mMoveX;
	mMoveY = aKind.mMoveY;
	mMoveZ = aKind.mMoveZ;
	mAccelX = aKind.mAccelX;
	mAccelY = aKind.mAccelY;
	mAccelZ = aKind.mAccelZ;
	mDirX = aKind.mDirX;
	mDirY = aKind.mDirY;
	mDirZ = aKind.mDirZ;
	if (aKind.mNormalize)
	{
		// 0x416f50: a vector without length points up
		double aLengthSquared = mDirZ * mDirZ + mDirY * mDirY + mDirX * mDirX;
		if (aLengthSquared > 1e-18)
		{
			double aScale = 1.0 / sqrt(aLengthSquared);
			mDirX = aScale * mDirX;
			mDirY = aScale * mDirY;
			mDirZ = aScale * mDirZ;
		}
		else
		{
			mDirX = 0;
			mDirY = 1.0;
			mDirZ = 0;
		}
	}
	mDirSpreadX = aKind.mDirSpreadX;
	mDirSpreadY = aKind.mDirSpreadY;
	mDirSpreadZ = aKind.mDirSpreadZ;
	mSpeed = aKind.mSpeed;
	mSpeedSpread = aKind.mSpeedSpread;
	mDrag = aKind.mDrag;
	mParticleLife = aKind.mParticleLife;
	mParticleLifeSpread = aKind.mParticleLifeSpread;
	mCount = aKind.mCount;
	mCountSpread = aKind.mCountSpread;
	mInterval = aKind.mInterval;
}

// 0x418fc0
Effect::~Effect()
{
	Particle* aParticle = mParticles;
	while (aParticle != NULL)
	{
		Particle* aNext = aParticle->mNext;
		RemoveParticle(aParticle);
		aParticle = aNext;
	}
	if (mNotifyBoard && !mManager->mClosing && mApp->mBoard != NULL)
		mApp->mBoard->AttachLaunchBonuses();
}

// 0x419020 (0x416f50 inline in it): new particles at the front of the list
void Effect::Spawn(int theCount)
{
	int aCount = 0;
	if (0 < theCount)
	{
		do
		{
			if (!mPool->HasFree())
				return;
			if (mMaxParticles <= mParticleCount)
				return;

			Particle* aParticle = mPool->Take();
			if (mParticles != NULL)
				mParticles->mPrev = aParticle;
			aParticle->mPrev = NULL;
			aParticle->mNext = mParticles;
			mParticles = aParticle;
			mParticleCount++;
			aParticle->mSerial = mParticleCount;

			aParticle->mX = SpreadRand(mX, mSpreadX);
			aParticle->mY = SpreadRand(mY, mSpreadY);
			aParticle->mZ = SpreadRand(mZ, mSpreadZ);
			aParticle->mVX = SpreadRand(mDirX, mDirSpreadX);
			aParticle->mVY = SpreadRand(mDirY, mDirSpreadY);
			aParticle->mVZ = SpreadRand(mDirZ, mDirSpreadZ);
			if (aParticle->mVX != 0.0 || aParticle->mVY != 0.0 || aParticle->mVZ != 0.0)
			{
				// 0x416f50
				double aLengthSquared = aParticle->mVZ * aParticle->mVZ + aParticle->mVY * aParticle->mVY +
					aParticle->mVX * aParticle->mVX;
				if (aLengthSquared > 1e-18)
				{
					double aScale = 1.0 / sqrt(aLengthSquared);
					aParticle->mVX = aScale * aParticle->mVX;
					aParticle->mVY = aScale * aParticle->mVY;
					aParticle->mVZ = aScale * aParticle->mVZ;
				}
				else
				{
					aParticle->mVX = 0;
					aParticle->mVY = 1.0;
					aParticle->mVZ = 0;
				}
			}
			double aSpeed = SpreadRand(mSpeed, mSpeedSpread);
			aParticle->mVX = aSpeed * aParticle->mVX;
			aParticle->mVY = aSpeed * aParticle->mVY;
			aParticle->mVZ = aSpeed * aParticle->mVZ;

			int aLife = (GlobalRand() % (mParticleLifeSpread * 2 + 1) + mParticleLife) - mParticleLifeSpread;
			aParticle->mAge = aLife;
			aParticle->mLife = aLife;
			if (mKind == 0xf || mKind == 5)
				aParticle->mColor = GlobalRand() % 3;
			else
				aParticle->mColor = mColorChoice;
			aCount++;
		} while (aCount < theCount);
	}
}

// 0x4192d0: a particle moves, then its speed is scaled and the acceleration added; at the end of its life it goes
void Effect::UpdateParticle(Particle* theParticle)
{
	theParticle->mAge--;
	if (-1 < theParticle->mAge)
	{
		theParticle->mX = theParticle->mX + theParticle->mVX;
		theParticle->mY = theParticle->mY + theParticle->mVY;
		theParticle->mZ = theParticle->mVZ + theParticle->mZ;
		theParticle->mVX = mDrag * theParticle->mVX + mAccelX;
		theParticle->mVY = mDrag * theParticle->mVY + mAccelY;
		theParticle->mVZ = theParticle->mVZ * mDrag + mAccelZ;
		return;
	}
	RemoveParticle(theParticle);
}

// 0x419280
void Effect::RemoveParticle(Particle* theParticle)
{
	if (theParticle->mPrev == NULL)
		mParticles = theParticle->mNext;
	else
		theParticle->mPrev->mNext = theParticle->mNext;
	if (theParticle->mNext != NULL)
		theParticle->mNext->mPrev = theParticle->mPrev;
	mPool->Give(theParticle);
	mParticleCount--;
}

// 0x419340: waits out the delay, then moves, moves its particles and makes new ones while it has life; false when it
// has no life and no particles left
bool Effect::Update()
{
	if (mDelay < 1)
	{
		mX = mMoveX + mX;
		mY = mMoveY + mY;
		mZ = mMoveZ + mZ;
		Particle* aParticle = mParticles;
		while (aParticle != NULL)
		{
			Particle* aNext = aParticle->mNext;
			UpdateParticle(aParticle);
			aParticle = aNext;
		}
		if (mLife == 0)
		{
			if (mParticles == NULL)
				return false;
		}
		else if ((mInterval == 0) || (GlobalRand() % (mInterval + 1) == 0))
		{
			int aNumber = (GlobalRand() % (mCountSpread * 2 + 1) + mCount) - mCountSpread;
			if (0 < aNumber)
				Spawn(aNumber);
		}
		if (0 < mLife)
			mLife--;
	}
	else
	{
		mDelay--;
		if ((mDelay == 0) && mSounds)
		{
			if (mSound != -1)
				mApp->PlaySample(mSound);
			mApp->PlaySample(GlobalRand() % 6 + 0x12);
		}
	}
	return true;
}

// 0x419450: the colour of a particle goes from the second colour of its pair (fresh) to the first (finished)
void Effect::SetBlendColor(Graphics* g, Particle* theParticle, double theFraction)
{
	const Color& aFrom = mColors[theParticle->mColor];
	const Color& aTo = mColors[theParticle->mColor + 6];
	double aKeep = 1.0 - theFraction;
	int aBlue = (int) ((double) aFrom.mBlue * aKeep + (double) aTo.mBlue * theFraction);
	int aGreen = (int) ((double) aFrom.mGreen * aKeep + (double) aTo.mGreen * theFraction);
	int aRed = (int) ((double) aFrom.mRed * aKeep + (double) aTo.mRed * theFraction);
	g->SetColor(Color(aRed, aGreen, aBlue));
}

// port: The kinds that go through a strip of pictures with the particle's age: kinds 0 and 3 and the flames (0x14 to 0x16)
// take the whole strip without a colour; the rest blend the colours.
void Effect::DrawFrames(Graphics* g, Image* theImage, int theFrames, bool theBlend)
{
	int aWidth = theImage->GetWidth() / theFrames;
	int aHeight = theImage->GetHeight();
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	if (theBlend)
		g->SetColorizeImages(true);

	for (Particle* aParticle = mParticles; aParticle != NULL; aParticle = aParticle->mNext)
	{
		double aFraction = (double) aParticle->mAge / (double) aParticle->mLife;
		if (theBlend)
			SetBlendColor(g, aParticle, aFraction);
		int aStep = (int) (aFraction * (double) (theFrames - 1));
		int aFrame = (theFrames - 1) - aStep;
		g->DrawImage(theImage, (int) aParticle->mX, (int) aParticle->mY, Rect(aFrame * aWidth, 0, aWidth, aHeight));
	}

	if (theBlend)
		g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x4194d0: draws the particles of one kind and one layer
void Effect::Draw(Graphics* g, bool theLayer)
{
	if (mLayer != theLayer)
		return;

	Image** anImages = mApp->mImages;
	Particle* aParticle = mParticles;
	switch (mKind)
	{
	case 0:
	case 3:
	case 0x14:
	case 0x15:
		DrawFrames(g, anImages[IMAGE_EXPLOSION], 15, false);
		break;

	case 0x16:
		g->SetColorizeImages(true);
		g->SetColor(Color(200, 0x40, 0x80));
		DrawFrames(g, anImages[IMAGE_EXPLOSION], 15, false);
		g->SetColorizeImages(false);
		break;

	case 1:
	case 0x11:
	{
		// nine pictures, the colour blended: the strip of 8 steps
		Image* anImage = anImages[IMAGE_EXPLOSION_STRIP];
		int aWidth = anImage->GetWidth() / 9;
		int aHeight = anImage->GetHeight();
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		for (; aParticle != NULL; aParticle = aParticle->mNext)
		{
			double aFraction = (double) aParticle->mAge / (double) aParticle->mLife;
			SetBlendColor(g, aParticle, aFraction);
			int aFrame = 8 - (int) (aFraction * 8.0);
			g->DrawImage(anImage, (int) aParticle->mX, (int) aParticle->mY, Rect(aFrame * aWidth, 0, aWidth, aHeight));
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;
	}

	case 2:
	{
		// the leaves of the menu: five pictures by the age; the first 60 updates of a leaf's life the colour fades in
		Image* anImage = anImages[IMAGE_LEAF];
		int aWidth = anImage->GetWidth() / 5;
		int aHeight = anImage->GetHeight();
		for (; aParticle != NULL; aParticle = aParticle->mNext)
		{
			int anAge = aParticle->mAge;
			int aLife = aParticle->mLife;
			int aFrame = 4 - (int) ((double) anAge / (double) aLife * 4.0);
			int aFade = (aLife - anAge) / 2;
			if (aFade < 0x3c)
			{
				g->SetColorizeImages(true);
				int aValue = aFade + 0xc3;
				g->SetColor(Color(aValue, aValue, aValue, aValue));
				g->DrawImage(anImage, (int) aParticle->mX, (int) aParticle->mY, Rect(aFrame * aWidth, 0, aWidth, aHeight));
				g->SetColorizeImages(false);
			}
			else
				g->DrawImage(anImage, (int) aParticle->mX, (int) aParticle->mY, Rect(aFrame * aWidth, 0, aWidth, aHeight));
		}
		break;
	}

	case 4:
	case 5:
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		for (; aParticle != NULL; aParticle = aParticle->mNext)
		{
			SetBlendColor(g, aParticle, (double) aParticle->mAge / (double) aParticle->mLife);
			g->DrawImage(anImages[IMAGE_SMALLPINKSTAR], (int) aParticle->mX, (int) aParticle->mY);
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;

	case 6:
		// the stars of the fuse: every other one turns (a turn in 15 steps over its life)
		for (; aParticle != NULL; aParticle = aParticle->mNext)
		{
			if ((aParticle->mSerial & 1) == 0)
				g->DrawImage(anImages[IMAGE_SMALLPINKSTAR], (int) aParticle->mX, (int) aParticle->mY);
			else
			{
				int aStep = 0xe - (int) ((double) aParticle->mAge / (double) aParticle->mLife * 14.0);
				double anAngle = (double) aStep * 0.06666666666666667 * 6.283185307;
				g->DrawImageRotated(anImages[IMAGE_BIGPINKSTAR], (int) aParticle->mX, (int) aParticle->mY, anAngle, NULL);
			}
		}
		break;

	case 7:
	case 8:
	case 9:
	case 10:
	{
		// ten pictures, the colour blended
		Image* anImage = anImages[(mKind <= 8) ? IMAGE_FIREWORKSROTATE1 : IMAGE_FIREWORKSNOVA1];
		int aWidth = anImage->GetWidth() / 10;
		int aHeight = anImage->GetHeight();
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		for (; aParticle != NULL; aParticle = aParticle->mNext)
		{
			double aFraction = (double) aParticle->mAge / (double) aParticle->mLife;
			SetBlendColor(g, aParticle, aFraction);
			int aFrame = (9 - (int) (aFraction * 9.0)) % 10;
			g->DrawImage(anImage, (int) aParticle->mX, (int) aParticle->mY, Rect(aFrame * aWidth, 0, aWidth, aHeight));
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;
	}

	case 0xb:
	case 0xc:
	case 0xd:
	case 0xe:
	case 0x17:
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		for (; aParticle != NULL; aParticle = aParticle->mNext)
			g->DrawImage(anImages[IMAGE_SMALLPINKSTAR], (int) aParticle->mX, (int) aParticle->mY);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;

	case 0xf:
	case 0x10:
	case 0x12:
	{
		// fourteen sparkles, the colour blended
		Image* anImage = anImages[IMAGE_SPARKLE];
		int aWidth = anImage->GetWidth() / 14;
		int aHeight = anImage->GetHeight();
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		for (; aParticle != NULL; aParticle = aParticle->mNext)
		{
			double aFraction = (double) aParticle->mAge / (double) aParticle->mLife;
			SetBlendColor(g, aParticle, aFraction);
			int aFrame = (0xd - (int) (aFraction * 13.0)) % 0xe;
			g->DrawImage(anImage, (int) aParticle->mX, (int) aParticle->mY, Rect(aFrame * aWidth, 0, aWidth, aHeight));
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;
	}

	case 0x13:
	{
		// the same sparkles in an orange
		Image* anImage = anImages[IMAGE_SPARKLE];
		int aWidth = anImage->GetWidth() / 14;
		int aHeight = anImage->GetHeight();
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		for (; aParticle != NULL; aParticle = aParticle->mNext)
		{
			double aFraction = (double) aParticle->mAge / (double) aParticle->mLife;
			int aFrame = (0xd - (int) (aFraction * 13.0)) % 0xe;
			g->SetColor(Color(0xff, 0x78, 0x32));
			g->DrawImage(anImage, (int) aParticle->mX, (int) aParticle->mY, Rect(aFrame * aWidth, 0, aWidth, aHeight));
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;
	}
	}
}

// ---------------------------------------------------------------------------------------------------------
// EffectManager

// 0x41a830
EffectManager::EffectManager(RocketApp* theApp) :
	mApp(theApp), mPool(new ParticlePool(POOL_SIZE)), mClosing(false)
{
}

// 0x41a8c0 (with the pool's destructor 0x41ad00)
EffectManager::~EffectManager()
{
	mClosing = true;
	for (std::list<Effect*>::iterator anItr = mEffects.begin(); anItr != mEffects.end(); ++anItr)
		delete *anItr;
	mEffects.clear();
	delete mPool;
}

// 0x41a9c0
Effect* EffectManager::Add(int theKind, bool theFlag)
{
	Effect* anEffect = new Effect(mApp, this, theKind, theFlag);
	mEffects.push_back(anEffect);
	return anEffect;
}

// 0x41aae0
void EffectManager::Clear()
{
	std::list<Effect*>::iterator anItr = mEffects.begin();
	while (anItr != mEffects.end())
	{
		Effect* anEffect = *anItr;
		anItr = mEffects.erase(anItr);
		delete anEffect;
	}
}

// 0x41aa80
void EffectManager::Remove(Effect* theEffect)
{
	for (std::list<Effect*>::iterator anItr = mEffects.begin(); anItr != mEffects.end(); ++anItr)
	{
		if (*anItr == theEffect)
		{
			mEffects.erase(anItr);
			delete theEffect;
			return;
		}
	}
}

// 0x41ab40
bool EffectManager::Has(Effect* theEffect)
{
	for (std::list<Effect*>::iterator anItr = mEffects.begin(); anItr != mEffects.end(); ++anItr)
	{
		if (*anItr == theEffect)
			return true;
	}
	return false;
}

// 0x41ab70: every effect is updated; the finished ones that the manager owns (the flag) go
void EffectManager::Update()
{
	std::list<Effect*>::iterator anItr = mEffects.begin();
	while (anItr != mEffects.end())
	{
		Effect* anEffect = *anItr;
		++anItr;
		bool aGoesOn = anEffect->Update();
		if (!aGoesOn && anEffect->mFlag)
			Remove(anEffect);
	}
}

// 0x41abb0
void EffectManager::Draw(Graphics* g, bool theLayer)
{
	for (std::list<Effect*>::iterator anItr = mEffects.begin(); anItr != mEffects.end(); ++anItr)
		(*anItr)->Draw(g, theLayer);
}
