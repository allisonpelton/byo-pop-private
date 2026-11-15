#include "Particles.h"
#include "PixelusApp.h"
#include "Res.h"
#include "Graphics.h"
#include "Image.h"
#include <math.h>

using namespace Sexy;

static const double TWO_PI = 6.28318;			// 0x43e250 and 0x43e160 turn by this, not by pi times two

// 0x42fa60: the strength from 0 to 1 at an age of an effect that lasts theDuration
double ParticleCurve::Eval(double theAge, double theDuration) const
{
	if (theAge <= mStart || theDuration <= theAge)
		return 0.0;
	double aTime = theAge - mStart;
	if (aTime < mAttack)
		return aTime / mAttack;
	aTime -= mAttack;
	if (aTime < mDecay)
		return 1.0 - ((1.0 - mSustain) / mDecay) * aTime;
	aTime -= mDecay;
	double aSustainEnd = ((theDuration - mStart) - mAttack) - mDecay;
	if (aTime < aSustainEnd)
		return mSustain;
	return (1.0 - (aTime - aSustainEnd) / mRelease) * mSustain;
}

// ---- the table -------------------------------------------------------------------------------------------------

// 0x43e600: one picture def; the strip is cut into theFrames pictures side by side, each with its middle as the pivot
static ParticleImageDef* MakeImageDef(const char* theName, Image* theImage, int theAnimMode, bool theRandomFrame,
	int theFrames, double theFramesPerSecond, bool theRandomAngle, double theTurnRate, bool theFades, double theFadeStart,
	int theDrawMode, const ParticleVec& theGravity, double theSwirl, double theSpeedUp, double theDragX, double theDragY,
	double theDrag, double theLifetime)
{
	ParticleImageDef* aDef = new ParticleImageDef();
	aDef->mName = theName;
	aDef->mImage = theImage;
	aDef->mFrames = theFrames;
	aDef->mPivotX = theImage->GetWidth() / (theFrames * 2);
	aDef->mPivotY = theImage->GetHeight() / 2;
	aDef->mFramesPerSecond = theFramesPerSecond;
	aDef->mAnimMode = theAnimMode;
	aDef->mRandomFrame = theRandomFrame;
	if (theAnimMode != 0 && theFramesPerSecond == 0.0)
		aDef->mFramesPerSecond = (double) theFrames / theLifetime;
	aDef->mTurnRate = theTurnRate;
	aDef->mFadeStart = theFadeStart;
	aDef->mRandomAngle = theRandomAngle;
	aDef->mFades = theFades;
	aDef->mDrawMode = theDrawMode;
	aDef->mGravity = theGravity;
	aDef->mSwirl = theSwirl;
	aDef->mSpeedUp = theSpeedUp;
	aDef->mDragX = theDragX;
	aDef->mDragY = theDragY;
	aDef->mDrag = theDrag;
	aDef->mLifetime = theLifetime;
	return aDef;
}

// 0x43f260 gives the default; the rest of each is what 0x43f3a0 stores after it
static ParticleEmitterDef* MakeEmitterDef(const char* theName, int theCount, double theRate, double theCurveStart, int theMode,
	const ParticleVec& theOffset, const ParticleVec& theDirection, const ParticleVec& theSpread, double theSpeed,
	double theSpeedSpread, double theDuration)
{
	ParticleEmitterDef* aDef = new ParticleEmitterDef();
	aDef->mName = theName;
	aDef->mCount = theCount;
	aDef->mRate = theRate;
	aDef->mCurve.mAttack = 0;
	aDef->mCurve.mDecay = 0;
	aDef->mCurve.mSustain = 1.0;
	aDef->mCurve.mRelease = 0;
	aDef->mCurve.mStart = theCurveStart;
	aDef->mMode = theMode;
	aDef->mOffset = theOffset;
	aDef->mDirection = theDirection;
	aDef->mSpread = theSpread;
	aDef->mSpeed = theSpeed;
	aDef->mSpeedSpread = theSpeedSpread;
	aDef->mDrag = 0;
	aDef->mDuration = theDuration;
	aDef->mRateSpread = 0.5;
	return aDef;
}

ParticleLibrary::ParticleLibrary(PixelusApp* theApp)
{
	const ParticleVec aNone;
	const ParticleVec aUp(0, -1.0, 0);
	const ParticleVec aSmall(0.05, 0.05, 0);

	mEmitterDefs.push_back(MakeEmitterDef("Bloom", 100, 1500, 0, 2, ParticleVec(5, 1, 0), aNone, aSmall, 150, 300, 0.2));
	mEmitterDefs.push_back(MakeEmitterDef("Rumble", 20, 20, 0, 2, ParticleVec(5, 1, 0), aNone, aSmall, 100, 200, 1.0));
	mEmitterDefs.push_back(MakeEmitterDef("PauseBloom", 100, 1500, 1.0, 2, ParticleVec(5, 1, 0), aNone, aSmall, 150, 300, 1.2));
	mEmitterDefs.push_back(MakeEmitterDef("PauseBloomUp", 100, 1500, 1.0, 3, ParticleVec(40, 0, 0), aUp, aSmall, 300, 300, 1.2));

	// the puff's images stand still, so its image def is made here too
	mImageDefs.push_back(MakeImageDef("DustPuff", theApp->mImages[IMAGE_DUSTPUFF], 0, false, 1, 0.0, false, 0.0, true, 0.7, 0,
		ParticleVec(0, -30.0, 0), 0, 0, 4.0, 8.0, 0, 1.2));

	mEmitterDefs.push_back(MakeEmitterDef("UpPuff", 12, 120, 0, 3, ParticleVec(32, 12, 0), aUp, ParticleVec(0.2, 1.0, 0), 250, 100, 0.1));
	mEmitterDefs.push_back(MakeEmitterDef("ShacklesBlownApart", 100, 1500, 0.5, 2, ParticleVec(3, 3, 0), aNone, aSmall, 300, 50, 3.0));
	mImageDefs.push_back(MakeImageDef("ChainLink", theApp->mImages[IMAGE_CHAINLINK], 0, true, 1, 0.0, true, 1.0, false, 0, 0,
		ParticleVec(0, 400.0, 0), 0, 0, 0, 0, 0, 100.0));
	mEmitterDefs.push_back(MakeEmitterDef("Lock", 1, 1500, 0.5, 3, ParticleVec(3, 3, 0), aNone, aSmall, 100, 50, 3.0));
	mImageDefs.push_back(MakeImageDef("Lock", theApp->mImages[IMAGE_LOCKPUZZLE], 0, true, 1, 0.0, true, 0.5, false, 0, 0,
		ParticleVec(0, 400.0, 0), 0, 0, 0, 0, 0, 100.0));
}

ParticleLibrary::~ParticleLibrary()
{
	for (size_t i = 0; i < mImageDefs.size(); i++)
		delete mImageDefs[i];
	for (size_t i = 0; i < mEmitterDefs.size(); i++)
		delete mEmitterDefs[i];
}

ParticleImageDef* ParticleLibrary::FindImageDef(const char* theName)
{
	for (size_t i = 0; i < mImageDefs.size(); i++)
		if (mImageDefs[i]->mName == theName)
			return mImageDefs[i];
	return NULL;
}

ParticleEmitterDef* ParticleLibrary::FindEmitterDef(const char* theName)
{
	for (size_t i = 0; i < mEmitterDefs.size(); i++)
		if (mEmitterDefs[i]->mName == theName)
			return mEmitterDefs[i];
	return NULL;
}

// ---- an effect --------------------------------------------------------------------------------------------------

double ParticleEffect::Rand01()
{
	return mApp->AppRandNext() * 4.656612875245797e-10;
}

ParticleVec ParticleEffect::RandVec()
{
	double aFirst = Rand01();
	double aSecond = Rand01();
	double aThird = Rand01();
	return ParticleVec(aSecond - 0.5, aFirst - 0.5, aThird - 0.5);
}

// 0x43ebe0, and 0x43e160 for each of the particles it keeps ready
ParticleEffect::ParticleEffect(PixelusApp* theApp, ParticleEmitterDef* theEmitter, ParticleImageDef* theImageDef,
	const int theColor[4], const ParticleVec& thePos, const ParticleVec& theVel)
{
	mApp = theApp;
	mEmitter = theEmitter;
	mImageDef = theImageDef;
	mPos = thePos;
	mVel = theVel;
	for (int i = 0; i < 4; i++)
		mColor[i] = theColor[i];
	mAge = 0;
	mPending = 0;
	for (int i = 0; i < mEmitter->mCount; i++)
	{
		Particle* aParticle = new Particle();
		aParticle->mAge = 0;
		aParticle->mAgeRate = 1.0;
		aParticle->mFrameStart = 0;
		aParticle->mDead = false;
		if (mImageDef->mRandomFrame)
			aParticle->mFrameStart = (int) ((unsigned int) mApp->AppRandNext() % (unsigned int) mImageDef->mFrames);
		aParticle->mAngle = 0;
		if (mImageDef->mRandomAngle)
			aParticle->mAngle = Rand01() * TWO_PI;
		mFree.push_back(aParticle);
	}
	mRateFactor = (Rand01() - 0.5) * mEmitter->mRateSpread + 1.0;
}

ParticleEffect::~ParticleEffect()
{
	for (size_t i = 0; i < mFree.size(); i++)
		delete mFree[i];
	for (std::list<Particle*>::iterator anItr = mActive.begin(); anItr != mActive.end(); ++anItr)
		delete *anItr;
}

// 0x43e250
void ParticleEffect::UpdateParticle(Particle* p, double theStep)
{
	const ParticleImageDef* aDef = mImageDef;
	p->mAge += theStep * p->mAgeRate;
	if (p->mAge >= aDef->mLifetime)
	{
		p->mDead = true;
		return;
	}
	p->mVel.x += theStep * aDef->mGravity.x;
	p->mVel.y += theStep * aDef->mGravity.y;
	p->mVel.z += theStep * aDef->mGravity.z;
	if ((aDef->mSpeedUp > 0.0 || aDef->mSwirl > 0.0) && p->mVel.y * p->mVel.y + p->mVel.x * p->mVel.x > 0.0)
	{
		double aLength = sqrt(p->mVel.x * p->mVel.x + p->mVel.y * p->mVel.y + p->mVel.z * p->mVel.z);	// 0x42fb50
		ParticleVec aUnit = p->mVel;
		if (aLength > 1e-06)
		{
			aLength = 1.0 / aLength;
			aUnit.x *= aLength;
			aUnit.y *= aLength;
			aUnit.z *= aLength;
		}
		double aSpeedUp = theStep * aDef->mSpeedUp;
		p->mVel.x = aUnit.x * aSpeedUp + p->mVel.x;
		p->mVel.y = aSpeedUp * aUnit.y + p->mVel.y;
		p->mVel.z = aUnit.z * aSpeedUp + p->mVel.z;
		double aSwirl = theStep * aDef->mSwirl;
		p->mVel.x = -aUnit.y * aSwirl + p->mVel.x;
		p->mVel.y = aSwirl * aUnit.x + p->mVel.y;
		p->mVel.z = aSwirl * 0.0 + p->mVel.z;
	}
	double aFactor = 1.0 - theStep * aDef->mDragX;
	if (aFactor < 0.0)
		aFactor = 0.0;
	p->mVel.x = aFactor * p->mVel.x;
	aFactor = 1.0 - theStep * aDef->mDragY;
	if (aFactor < 0.0)
		aFactor = 0.0;
	p->mVel.y = aFactor * p->mVel.y;
	aFactor = 1.0 - theStep * aDef->mDrag;
	if (aFactor < 0.0)
		aFactor = 0.0;
	p->mVel.x = aFactor * p->mVel.x;
	p->mVel.y = aFactor * p->mVel.y;
	p->mPos.x = theStep * p->mVel.x + p->mPos.x;
	p->mPos.y = theStep * p->mVel.y + p->mPos.y;
	p->mPos.z = theStep * p->mVel.z + p->mPos.z;
	if (aDef->mTurnRate != 0.0)
		p->mAngle = theStep * aDef->mTurnRate * TWO_PI + p->mAngle;
}

// 0x43eda0
bool ParticleEffect::Update(double theStep)
{
	const ParticleEmitterDef* aDef = mEmitter;
	mAge += theStep;
	if (mAge < aDef->mDuration)
	{
		mVel.x += theStep * aDef->mGravity.x;
		mVel.y += theStep * aDef->mGravity.y;
		mVel.z += theStep * aDef->mGravity.z;
		double aDrag = theStep * aDef->mDrag;
		double aDragX = aDrag * mVel.x;
		double aDragY = aDrag * mVel.y;
		double aDragZ = aDrag * mVel.z;
		mVel.x -= aDragX;
		mVel.y -= aDragY;
		mVel.z -= aDragZ;
		mPos.x += theStep * mVel.x;
		mPos.y += theStep * mVel.y;
		mPos.z += theStep * mVel.z;

		mPending = aDef->mCurve.Eval(mAge, aDef->mDuration) * aDef->mRate * theStep + mPending;
		bool aFractional = false;
		while (mPending > 0.0 && !mFree.empty())
		{
			if (mPending < 1.0)
			{
				aFractional = true;
				if (mPending <= Rand01())
					break;
			}
			mPending -= 1.0;
			Particle* aParticle = mFree.back();
			mFree.pop_back();
			mActive.push_back(aParticle);

			double aSpeed = (Rand01() - 0.5) * aDef->mSpeedSpread + aDef->mSpeed;
			ParticleVec aPlace = RandVec();
			ParticleVec anOffset(aDef->mOffset.x * aPlace.x, aPlace.y * aDef->mOffset.y, aPlace.z * aDef->mOffset.z);
			ParticleVec aStray = RandVec();
			ParticleVec aStrayOffset(aDef->mSpread.x * aStray.x, aStray.y * aDef->mSpread.y, 0);
			ParticleVec aDirection;
			switch (aDef->mMode)
			{
			case 0:
				aDirection = mVel;
				break;
			case 1:
				aDirection = ParticleVec(-mVel.x, -mVel.y, -mVel.z);
				break;
			case 2:
				aDirection = anOffset;
				break;
			case 3:
				aDirection = aDef->mDirection;
				break;
			}
			aDirection.x += aStrayOffset.x;
			aDirection.y += aStrayOffset.y;
			aDirection.z += aStray.z * aDef->mSpread.z;
			double aLengthSquared = aDirection.x * aDirection.x + aDirection.y * aDirection.y + aDirection.z * aDirection.z;
			if (0.0 < aLengthSquared)
			{
				double aScale = aSpeed / sqrt(aLengthSquared);
				aDirection.x *= aScale;
				aDirection.y *= aScale;
				aDirection.z *= aScale;
			}
			// 0x43e210
			aParticle->mPos = ParticleVec(anOffset.x + mPos.x, anOffset.y + mPos.y, anOffset.z + mPos.z);
			aParticle->mVel = aDirection;
			aParticle->mDead = false;
			aParticle->mAge = 0;
			aParticle->mAgeRate = mRateFactor;
			if (aFractional)
				break;
		}
	}

	if (mActive.empty())
		return false;
	std::list<Particle*>::iterator anItr = mActive.begin();
	while (anItr != mActive.end())
	{
		Particle* aParticle = *anItr;
		UpdateParticle(aParticle, theStep);
		if (aParticle->mDead)
		{
			anItr = mActive.erase(anItr);
			mFree.push_back(aParticle);
		}
		else
			++anItr;
	}
	return true;
}

// 0x43e480
void ParticleEffect::DrawParticle(Graphics* g, const Particle* p, bool theRotateFading)
{
	const ParticleImageDef* aDef = mImageDef;
	int aX = (int) p->mPos.x;
	int aY = (int) p->mPos.y;
	if (aDef->mFades)
	{
		Color aColor(mColor[0], mColor[1], mColor[2], mColor[3]);
		if (aDef->mFadeStart <= p->mAge)
		{
			int aFade = (int) ((p->mAge - aDef->mFadeStart) * 255.0 / (aDef->mLifetime - aDef->mFadeStart));
			if (aFade > 0xff)
				aFade = 0xff;
			aColor = Color(mColor[0], mColor[1], mColor[2], ((0xff - aFade) * mColor[3]) >> 8);
		}
		g->SetColor(aColor);
	}
	int aFrame = 0;
	if (aDef->mAnimMode != 0)
	{
		aFrame = (int) (aDef->mFramesPerSecond * p->mAge);
		int aCount = aDef->mFrames;
		if (aDef->mAnimMode == 1)
		{
			if (aFrame >= aCount)
				aFrame = aCount - 1;
		}
		else if (aDef->mAnimMode == 2)
			aFrame = (p->mFrameStart + aFrame) % aCount;
		else if (aDef->mAnimMode == 3)
		{
			int aPeriod = aCount * 2 - 2;
			aFrame = (p->mFrameStart + aFrame) % aPeriod;
			if (aPeriod / 2 < aFrame)
				aFrame = aPeriod - aFrame;
		}
	}
	int aFrameWidth = aDef->mImage->GetWidth() / aDef->mFrames;
	Rect aSource(aFrame * aFrameWidth, 0, aFrameWidth, aDef->mImage->GetHeight());
	if (p->mAngle != 0.0 && (theRotateFading || !aDef->mFades || aDef->mFadeStart > aDef->mLifetime))
		g->DrawImageRotatedF(aDef->mImage, (float) (aX - aDef->mPivotX), (float) (aY - aDef->mPivotY), p->mAngle,
			(float) aDef->mPivotX, (float) aDef->mPivotY, &aSource);
	else
		g->DrawImage(aDef->mImage, aX - aDef->mPivotX, aY - aDef->mPivotY, aSource);
}

// 0x43e7c0: theRotateFading is true on a machine that draws with the graphics card (0x492d40)
void ParticleEffect::Draw(Graphics* g, bool theRotateFading)
{
	g->SetDrawMode(mImageDef->mDrawMode == 0 ? Graphics::DRAWMODE_NORMAL : Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(mImageDef->mFades);
	for (std::list<Particle*>::iterator anItr = mActive.begin(); anItr != mActive.end(); ++anItr)
		DrawParticle(g, *anItr, theRotateFading);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}
