#include "Particles.h"

#include <cmath>
#include <cstdint>
#include <cstring>

#include "Graphics.h"
#include "Image.h"
#include "Rect.h"
#include "Res.h"
#include "SexyAppBase.h"

using namespace Sexy;

static const double UNIT_SCALE = 4.656612875245797e-10;		// 1 / 2^31
static const double FULL_TURN = 6.28318;						// the exe's own number for 2 pi

// ---- the generator ----

// 0x4b2a40: the 1.2 framework's MTRand with its default seed: the first word 4357, each next one the last times 69069,
// and the index at the end of the block, so that the first draw makes the first block
ParticleRand::ParticleRand()
{
	uint32_t aState[MTRAND_N];
	aState[0] = 0x1105;
	for (int i = 1; i < MTRAND_N; i++)
		aState[i] = aState[i - 1] * 0x10dcd;
	mRand.SetState(aState, MTRAND_N);
}

// 0x4b1b40
int ParticleRand::Next()
{
	return (int)mRand.NextNoAssert();
}

// 0x473930
double ParticleRand::NextUnit()
{
	return (double)Next() * UNIT_SCALE;
}

// 0x473960: the draws are made in the order first, second, third and used as second, first, third
ParticleVector ParticleRand::NextVector()
{
	double aFirst = (double)Next();
	double aSecond = (double)Next();
	double aThird = (double)Next();

	ParticleVector aVector;
	aVector.mX = aSecond * UNIT_SCALE - 0.5;
	aVector.mY = aFirst * UNIT_SCALE - 0.5;
	aVector.mZ = aThird * UNIT_SCALE - 0.5;
	return aVector;
}

// 0x4407d0
ParticleVector Sexy::NormalizeParticleVector(const ParticleVector& theVector)
{
	double aLength = sqrt((theVector.mZ * theVector.mZ + theVector.mY * theVector.mY) + theVector.mX * theVector.mX);
	if (1e-06 < aLength)
	{
		double aScale = 1.0 / aLength;
		ParticleVector aVector;
		aVector.mX = theVector.mX * aScale;
		aVector.mY = theVector.mY * aScale;
		aVector.mZ = theVector.mZ * aScale;
		return aVector;
	}
	return theVector;
}

// ---- the envelope ----

// 0x473890
double ParticleEnvelope::Eval(double theTime, double theDuration) const
{
	if (theTime <= mStart || theDuration <= theTime)
		return 0.0;

	double aLocal = theTime - mStart;
	if (aLocal <= mAttack)
		return aLocal / mAttack;

	aLocal = aLocal - mAttack;
	if (aLocal <= mDecay)
		return 1.0 - ((1.0 - mSustain) / mDecay) * aLocal;

	aLocal = aLocal - mDecay;
	double aSustainEnd = ((theDuration - mStart) - mAttack) - mDecay;
	if (aLocal <= aSustainEnd)
		return mSustain;
	return (1.0 - (aLocal - aSustainEnd) / mRelease) * mSustain;
}

// ---- the definitions ----

struct EmitterRow
{
	const char*		mName;
	int				mCount;
	double			mRate;
	double			mEnvelope[5];
	int				mDirMode;
	double			mScatter[3];
	double			mDirection[3];
	double			mJitter[3];
	double			mSpeed;
	double			mSpeedJitter;
	double			mGravity[3];
	double			mDrag;
	double			mDuration;
	double			mLifeJitter;
};

struct FragmentRow
{
	const char*		mName;
	Image**			mImage;
	int				mAnimMode;
	bool			mRandomStartFrame;
	double			mFrameRate;
	bool			mRandomAngle;
	double			mSpin;
	bool			mColorize;
	double			mFadeStart;
	int				mDrawMode;
	double			mLife;
	double			mAcceleration[3];
	double			mTangentialAccel;
	double			mRadialAccel;
	double			mDampingX;
	double			mDampingY;
	double			mDamping;
};

#include "ParticleDefs.inc"

static ParticleVector MakeVector(const double theValues[3])
{
	ParticleVector aVector;
	aVector.mX = theValues[0];
	aVector.mY = theValues[1];
	aVector.mZ = theValues[2];
	return aVector;
}

// 0x475500 (with the two constructors 0x475280 and 0x4745d0: a row sets every field the code reads)
ParticleDefs::ParticleDefs()
{
	for (size_t i = 0; i < sizeof(EMITTER_ROWS) / sizeof(EMITTER_ROWS[0]); i++)
	{
		const EmitterRow& aRow = EMITTER_ROWS[i];
		EmitterDef aDef;
		aDef.mName = aRow.mName;
		aDef.mCount = aRow.mCount;
		aDef.mRate = aRow.mRate;
		aDef.mEnvelope.mAttack = aRow.mEnvelope[0];
		aDef.mEnvelope.mDecay = aRow.mEnvelope[1];
		aDef.mEnvelope.mSustain = aRow.mEnvelope[2];
		aDef.mEnvelope.mRelease = aRow.mEnvelope[3];
		aDef.mEnvelope.mStart = aRow.mEnvelope[4];
		aDef.mDirMode = aRow.mDirMode;
		aDef.mScatter = MakeVector(aRow.mScatter);
		aDef.mDirection = MakeVector(aRow.mDirection);
		aDef.mJitter = MakeVector(aRow.mJitter);
		aDef.mSpeed = aRow.mSpeed;
		aDef.mSpeedJitter = aRow.mSpeedJitter;
		aDef.mGravity = MakeVector(aRow.mGravity);
		aDef.mDrag = aRow.mDrag;
		aDef.mDuration = aRow.mDuration;
		aDef.mLifeJitter = aRow.mLifeJitter;
		mEmitters.push_back(aDef);
	}

	for (size_t i = 0; i < sizeof(FRAGMENT_ROWS) / sizeof(FRAGMENT_ROWS[0]); i++)
	{
		const FragmentRow& aRow = FRAGMENT_ROWS[i];
		FragmentDef aDef;
		aDef.mName = aRow.mName;
		aDef.mImage = *aRow.mImage;
		aDef.mAnimMode = aRow.mAnimMode;
		aDef.mRandomStartFrame = aRow.mRandomStartFrame;
		aDef.mFrameRate = aRow.mFrameRate;
		aDef.mRandomAngle = aRow.mRandomAngle;
		aDef.mSpin = aRow.mSpin;
		aDef.mColorize = aRow.mColorize;
		aDef.mFadeStart = aRow.mFadeStart;
		aDef.mDrawMode = aRow.mDrawMode;
		aDef.mLife = aRow.mLife;
		aDef.mAcceleration = MakeVector(aRow.mAcceleration);
		aDef.mTangentialAccel = aRow.mTangentialAccel;
		aDef.mRadialAccel = aRow.mRadialAccel;
		aDef.mDampingX = aRow.mDampingX;
		aDef.mDampingY = aRow.mDampingY;
		aDef.mDamping = aRow.mDamping;

		// 0x4745d0: an animated fragment with no rate plays its frames over its life; a rate of -1 holds the first
		if (aDef.mAnimMode != 0)
		{
			if (aDef.mFrameRate == 0.0)
				aDef.mFrameRate = (double)aDef.mImage->mNumCols / aDef.mLife;
			else if (aDef.mFrameRate == -1.0)
				aDef.mFrameRate = 0.0;
		}
		mFragments.push_back(aDef);
	}
}

// 0x473f50
const EmitterDef* ParticleDefs::FindEmitter(const char* theName) const
{
	for (size_t i = 0; i < mEmitters.size(); i++)
		if (strcmp(mEmitters[i].mName.c_str(), theName) == 0)
			return &mEmitters[i];
	return nullptr;
}

// 0x473eb0
const FragmentDef* ParticleDefs::FindFragment(const char* theName) const
{
	for (size_t i = 0; i < mFragments.size(); i++)
		if (strcmp(mFragments[i].mName.c_str(), theName) == 0)
			return &mFragments[i];
	return nullptr;
}

// ---- a fragment ----

// 0x474090
ParticleFragment::ParticleFragment(ParticleRand& theRand, const FragmentDef& theDef) :
	mAge(0.0),
	mRateScale(1.0),
	mAngle(0.0),
	mFrameOffset(0),
	mDead(false)
{
	mPos.mX = mPos.mY = mPos.mZ = 0.0;
	mVel.mX = mVel.mY = mVel.mZ = 0.0;

	if (theDef.mRandomStartFrame)
		mFrameOffset = (int)((unsigned int)theRand.Next() % (unsigned int)theDef.mImage->mNumCols);
	if (theDef.mRandomAngle)
		mAngle = theRand.NextUnit() * FULL_TURN;
}

// 0x474140
void ParticleFragment::Init(const ParticleVector& thePos, const ParticleVector& theVel, double theRateScale)
{
	mPos = thePos;
	mVel = theVel;
	mDead = false;
	mAge = 0.0;
	mRateScale = theRateScale;
}

// 0x474180
void ParticleFragment::Update(const FragmentDef& theDef, double theDelta)
{
	mAge = theDelta * mRateScale + mAge;
	if (theDef.mLife <= mAge)
	{
		mDead = true;
		return;
	}

	double aAccelZ = theDelta * theDef.mAcceleration.mZ;
	mVel.mX = theDelta * theDef.mAcceleration.mX + mVel.mX;
	mVel.mY = theDelta * theDef.mAcceleration.mY + mVel.mY;
	mVel.mZ = aAccelZ + mVel.mZ;

	if ((0.0 < theDef.mRadialAccel || 0.0 < theDef.mTangentialAccel) && 0.0 < mVel.mY * mVel.mY + mVel.mX * mVel.mX)
	{
		ParticleVector aDir = NormalizeParticleVector(mVel);
		double aRadial = theDelta * theDef.mRadialAccel;
		mVel.mX = aDir.mX * aRadial + mVel.mX;
		mVel.mY = aRadial * aDir.mY + mVel.mY;
		mVel.mZ = aDir.mZ * aRadial + mVel.mZ;
		double aTangential = theDelta * theDef.mTangentialAccel;
		mVel.mX = -aDir.mY * aTangential + mVel.mX;
		mVel.mY = aTangential * aDir.mX + mVel.mY;
		mVel.mZ = aTangential * 0.0 + mVel.mZ;
	}

	double aFactor = 1.0 - theDelta * theDef.mDampingX;
	if (aFactor < 0.0)
		aFactor = 0.0;
	mVel.mX = aFactor * mVel.mX;

	aFactor = 1.0 - theDelta * theDef.mDampingY;
	if (aFactor < 0.0)
		aFactor = 0.0;
	mVel.mY = aFactor * mVel.mY;

	aFactor = 1.0 - theDelta * theDef.mDamping;
	if (aFactor < 0.0)
		aFactor = 0.0;
	mVel.mX = aFactor * mVel.mX;
	mVel.mY = aFactor * mVel.mY;

	double aMoveZ = theDelta * mVel.mZ;
	mPos.mX = theDelta * mVel.mX + mPos.mX;
	mPos.mY = theDelta * mVel.mY + mPos.mY;
	mPos.mZ = aMoveZ + mPos.mZ;

	if (theDef.mSpin != 0.0)
		mAngle = (theDelta * theDef.mSpin) * FULL_TURN + mAngle;
}

// 0x4743b0. The fragment is turned only when the renderer can turn a coloured picture (theRotateFaded), when it is
// not coloured, or when its life ends before its fade starts (never, in these definitions). The unturned draw takes the
// vertical half from the cel's width, as the original does.
void ParticleFragment::Draw(Graphics* g, bool theRotateFaded, const FragmentDef& theDef, const Color& theColor,
	int theOffsetX, int theOffsetY) const
{
	int aX = (int)mPos.mX;
	int aY = (int)mPos.mY;

	Color aColor = theColor;
	if (theDef.mColorize)
	{
		if (theDef.mFadeStart <= mAge)
		{
			int aFade = (int)(((mAge - theDef.mFadeStart) * 255.0) / (theDef.mLife - theDef.mFadeStart));
			if (aFade > 254)
				aFade = 255;
			aColor = Color(theColor.mRed, theColor.mGreen, theColor.mBlue, ((255 - aFade) * theColor.mAlpha) >> 8);
		}
		g->SetColor(aColor);
	}

	int aFrame = (int)(theDef.mFrameRate * mAge);
	int aCols = theDef.mImage->mNumCols;
	switch (theDef.mAnimMode)
	{
	case 0:
	case 2:
		aFrame = (mFrameOffset + aFrame) % aCols;
		break;
	case 1:
		if (aCols <= aFrame)
			aFrame = aCols - 1;
		break;
	case 3:
		{
			int aSpan = aCols * 2 - 2;
			aFrame = (mFrameOffset + aFrame) % aSpan;
			if (aSpan / 2 < aFrame)
				aFrame = aSpan - aFrame;
		}
		break;
	}

	int aCelWidth = theDef.mImage->GetCelWidth();
	int aCelHeight = theDef.mImage->GetCelHeight();

	if (mAngle != 0.0 && (theRotateFaded || !theDef.mColorize || theDef.mLife < theDef.mFadeStart))
	{
		Rect aSrc(aCelWidth * aFrame, 0, aCelWidth, aCelHeight);
		g->DrawImageRotated(theDef.mImage, (aX - aCelWidth / 2) + theOffsetX, (aY - aCelHeight / 2) + theOffsetY, mAngle, &aSrc);
		return;
	}

	Rect aSrc(aCelWidth * aFrame, 0, aCelWidth, theDef.mImage->mHeight);
	g->DrawImage(theDef.mImage, (aX - aCelWidth / 2) + theOffsetX, (aY - aCelWidth / 2) + theOffsetY, aSrc);
}

// ---- an effect ----

// 0x474bf0
ParticleEffect::ParticleEffect(ParticleRand& theRand, const EmitterDef* theEmitter, const FragmentDef* theFragment,
	const Color& theColor, const ParticleVector& thePos, const ParticleVector& theVel) :
	mEmitter(theEmitter),
	mFragment(theFragment),
	mPos(thePos),
	mVel(theVel),
	mAge(0.0),
	mSpawnAccumulator(0.0),
	mColor(theColor),
	mLifeScale(0.0),
	mStopped(false)
{
	mFragments.reserve(theEmitter->mCount > 0 ? theEmitter->mCount : 0);
	for (int i = 0; i < theEmitter->mCount; i++)
		mFragments.push_back(ParticleFragment(theRand, *theFragment));
	for (size_t i = 0; i < mFragments.size(); i++)
		mPool.push_back(&mFragments[i]);

	mLifeScale = (theRand.NextUnit() - 0.5) * theEmitter->mLifeJitter + 1.0;
}

// 0x474dc0
bool ParticleEffect::Update(ParticleRand& theRand, double theDelta)
{
	const EmitterDef& aDef = *mEmitter;

	mAge = theDelta + mAge;
	if (mAge < aDef.mDuration)
	{
		double aGravityZ = theDelta * aDef.mGravity.mZ;
		mVel.mX = theDelta * aDef.mGravity.mX + mVel.mX;
		mVel.mY = theDelta * aDef.mGravity.mY + mVel.mY;
		mVel.mZ = aGravityZ + mVel.mZ;

		double aDragY = (theDelta * aDef.mDrag) * mVel.mY;
		double aDragZ = (theDelta * aDef.mDrag) * mVel.mZ;
		mVel.mX = mVel.mX - (theDelta * aDef.mDrag) * mVel.mX;
		mVel.mY = mVel.mY - aDragY;
		mVel.mZ = mVel.mZ - aDragZ;

		double aMoveZ = theDelta * mVel.mZ;
		mPos.mX = theDelta * mVel.mX + mPos.mX;
		mPos.mY = theDelta * mVel.mY + mPos.mY;
		mPos.mZ = aMoveZ + mPos.mZ;

		mSpawnAccumulator = (aDef.mEnvelope.Eval(mAge, aDef.mDuration) * aDef.mRate) * theDelta + mSpawnAccumulator;

		if (!mStopped)
		{
			bool aFractional = false;
			while (0.0 < mSpawnAccumulator)
			{
				if (mPool.empty())
					break;

				// less than one fragment owed: it goes out with that chance, and no more than one at a time
				if (mSpawnAccumulator < 1.0)
				{
					aFractional = true;
					if (mSpawnAccumulator <= theRand.NextUnit())
						break;
				}

				mSpawnAccumulator = mSpawnAccumulator - 1.0;
				ParticleFragment* aFragment = mPool.back();
				mPool.pop_back();
				mLive.push_back(aFragment);

				double aSpeed = (theRand.NextUnit() - 0.5) * aDef.mSpeedJitter + aDef.mSpeed;

				ParticleVector aRandom = theRand.NextVector();
				ParticleVector aScatter;
				aScatter.mX = aDef.mScatter.mX * aRandom.mX;
				aScatter.mY = aRandom.mY * aDef.mScatter.mY;
				aScatter.mZ = aRandom.mZ * aDef.mScatter.mZ;

				ParticleVector aJitterRandom = theRand.NextVector();
				double aJitterX = aDef.mJitter.mX * aJitterRandom.mX;
				double aJitterY = aJitterRandom.mY * aDef.mJitter.mY;

				ParticleVector aDir = { 0.0, 0.0, 0.0 };
				switch (aDef.mDirMode)
				{
				case PARTICLE_DIR_EMITTER:
					aDir = mVel;
					break;
				case PARTICLE_DIR_OPPOSITE:
					aDir.mX = -mVel.mX;
					aDir.mY = -mVel.mY;
					aDir.mZ = -mVel.mZ;
					break;
				case PARTICLE_DIR_SCATTER:
					aDir = aScatter;
					break;
				case PARTICLE_DIR_FIXED:
					aDir = aDef.mDirection;
					break;
				}

				aDir.mX = aDir.mX + aJitterX;
				aDir.mY = aDir.mY + aJitterY;
				aDir.mZ = aDir.mZ + aJitterRandom.mZ * aDef.mJitter.mZ;

				double aLengthSquared = (aDir.mZ * aDir.mZ + aDir.mY * aDir.mY) + aDir.mX * aDir.mX;
				if (0.0 < aLengthSquared)
				{
					double aScale = aSpeed / sqrt(aLengthSquared);
					aDir.mX = aDir.mX * aScale;
					aDir.mY = aDir.mY * aScale;
					aDir.mZ = aDir.mZ * aScale;
				}

				ParticleVector aStart;
				aStart.mX = aScatter.mX + mPos.mX;
				aStart.mY = aScatter.mY + mPos.mY;
				aStart.mZ = aScatter.mZ + mPos.mZ;
				aFragment->Init(aStart, aDir, mLifeScale);

				if (aFractional)
					break;
			}
		}
	}

	// the fragments in the air move; those whose life is over go back to the pool
	bool aHadLive = !mLive.empty();
	for (std::list<ParticleFragment*>::iterator anItr = mLive.begin(); anItr != mLive.end();)
	{
		ParticleFragment* aFragment = *anItr;
		aFragment->Update(*mFragment, theDelta);
		if (aFragment->mDead)
		{
			anItr = mLive.erase(anItr);
			mPool.push_back(aFragment);
		}
		else
			++anItr;
	}
	return aHadLive;
}

// 0x474700
bool ParticleEffect::IsFinished() const
{
	return mEmitter->mDuration < mAge && mLive.empty();
}

// 0x4747d0
void ParticleEffect::Draw(Graphics* g, bool theRotateFaded, int theOffsetX, int theOffsetY) const
{
	g->SetDrawMode(mFragment->mDrawMode);
	g->SetColorizeImages(mFragment->mColorize);
	for (std::list<ParticleFragment*>::const_iterator anItr = mLive.begin(); anItr != mLive.end(); ++anItr)
		(*anItr)->Draw(g, theRotateFaded, *mFragment, mColor, theOffsetX, theOffsetY);
	g->SetColorizeImages(false);
	g->SetDrawMode(0);
}
