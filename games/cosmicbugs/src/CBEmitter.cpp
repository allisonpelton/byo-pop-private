// Particle emitters and particles (0x40abf0..0x40c0d0)

#include "CBEmitter.h"
#include "CBLevel.h"

#include <cstring>

namespace cb
{

static ParticleSystem gSystems[4000];     // 0x62de20
static int gSystemNext = 0;               // 0x478618
static Particle gParticles[8000];         // 0x478620
static int gParticleNext = 0;             // 0x47861c

int Particle_Update(Particle* theParticle)
{
	theParticle->mOldX = theParticle->mX;
	theParticle->mOldY = theParticle->mY;
	int aResult = 1;
	theParticle->mX = theParticle->mVx + theParticle->mX;
	theParticle->mY = theParticle->mVy + theParticle->mY;
	if (theParticle->mFrameTime != 0)
	{
		int aFrameCount = theParticle->mFrameCount;
		if (0 < aFrameCount && (theParticle->mFrameTimer = theParticle->mFrameTimer + 1,
						 theParticle->mFrameTime <= theParticle->mFrameTimer))
		{
			theParticle->mFrame = theParticle->mFrame + 1;
			theParticle->mFrameTimer = 0;
			if (aFrameCount <= theParticle->mFrame)
			{
				if (theParticle->mOneShot == 0)
					theParticle->mFrame = 0;
				else
				{
					theParticle->mFrame = theParticle->mFrame + -1;
					aResult = 0;
				}
			}
		}
		if (aFrameCount != 0)
		{
			Image* anImage = Bank_ImageAt(&gArtBank, theParticle->mFirstImage + theParticle->mFrame);
			theParticle->mImage = anImage;
			if (theParticle->mMidHandle != 0)
				Image_SetMidHandle(anImage);
		}
	}
	float anAge = theParticle->mAge + 1.0f;
	theParticle->mAge = anAge;
	if (theParticle->mLifetime != 0.0f && theParticle->mLifetime <= anAge)
		return 0;
	return aResult;
}

void Particle_Draw(Particle* theParticle)
{
	float aProgress = theParticle->mAge / theParticle->mLifetime;
	double anAlpha = (double)(theParticle->mEndAlpha - theParticle->mStartAlpha) * (double)aProgress +
		(double)theParticle->mStartAlpha;
	if (255.0 < anAlpha)
		anAlpha = 255.0;
	theParticle->mAlpha = (int)anAlpha;
	theParticle->mR = (int)((double)(theParticle->mEndR - theParticle->mStartR) * (double)aProgress + (double)theParticle->mStartR);
	theParticle->mG = (int)((double)(theParticle->mEndG - theParticle->mStartG) * (double)aProgress + (double)theParticle->mStartG);
	theParticle->mB = (int)((double)(theParticle->mEndB - theParticle->mStartB) * (double)aProgress + (double)theParticle->mStartB);
	if (theParticle->mFrameTime == 0)
	{
		Animation* anAnim = theParticle->mAnim;
		if (anAnim == nullptr)
		{
			// a 64-bit fistp with the rounding set to truncate
			int aFrame = (int)(int64_t)((double)aProgress * (double)theParticle->mFrameCount);
			theParticle->mImage = Bank_ImageAt(&gArtBank, aFrame + theParticle->mFirstImage);
		}
		else
		{
			float aLast = (float)((double)theParticle->mFrameCount - 1.0);
			theParticle->mImage = Animation_Frame(anAnim, (int)((double)aProgress * (double)aLast));
		}
	}
	if (theParticle->mImage == nullptr)
		return;
	Image_SetMidHandle(theParticle->mImage);
	Obj_Draw(theParticle);
}

// The port clears the members past the base, which the original leaves as the allocation found them
void Emitter_Append(List<Obj>* theList)
{
	Emitter* anEmitter = new Emitter();
	Obj_Init(anEmitter);
	List_Append(theList, (Obj*)anEmitter);
}

// 0x40ae30
void Emitter_Setup(Emitter* theEmitter, Obj* theParent, int theX, int theY, float theXSpeed, float theYSpeed,
	int theFrequency, int theRotation, const char* theAnim, int theKind, float theStartAlpha, float theEndAlpha,
	float theTime)
{
	theEmitter->mParent = theParent;
	theEmitter->mKind = theKind;
	theEmitter->mAnim = Animation_Find(theAnim, &theEmitter->mFrameCount, &theEmitter->mFrameTime);
	theEmitter->mOldX = (float)(theX - Animation_Frame(theEmitter->mParent->mAnim, 0)->mW / 2);
	int aHeight = Animation_Frame(theEmitter->mParent->mAnim, 0)->mH;
	theEmitter->mFrequency = theFrequency;
	theEmitter->mRotation = theRotation;
	theEmitter->mOldY = (float)(theY - aHeight / 2);
	theEmitter->mVx = (float)((double)theXSpeed / 1000.0);
	theEmitter->mVy = (float)((double)theYSpeed / 1000.0);
	theEmitter->mStartAlpha = (int)theStartAlpha;
	theEmitter->mEndAlpha = (int)theEndAlpha;
	theEmitter->mTime = (int)theTime;
	theEmitter->mDrawKind = 4;
	theEmitter->mStartR = 0xff;
	theEmitter->mStartG = 0xff;
	theEmitter->mStartB = 0xff;
	theEmitter->mEndR = 0xff;
	theEmitter->mEndG = 0xff;
	theEmitter->mEndB = 0xff;
	theEmitter->mVariance = 0;
}

void Emitter_CopyPrev(Emitter* theEmitter, int theAngle)
{
	Emitter* aPrev = static_cast<Emitter*>(theEmitter->mPrev);
	theEmitter->mParent = aPrev->mParent;
	theEmitter->mKind = aPrev->mKind;
	theEmitter->mAnim = aPrev->mAnim;
	theEmitter->mFrameTime = aPrev->mFrameTime;
	theEmitter->mFrameCount = aPrev->mFrameCount;
	float anOffX = aPrev->mOldX;
	float anOffY = aPrev->mOldY;
	theEmitter->mFrequency = aPrev->mFrequency;
	theEmitter->mRotation = aPrev->mRotation;
	float aVx = aPrev->mVx;
	float aVy = aPrev->mVy;
	theEmitter->mOldX = (float)((double)CosDeg(theAngle) * anOffX - (double)SinDeg(theAngle) * anOffY);
	theEmitter->mOldY = (float)((double)SinDeg(theAngle) * anOffX + (double)CosDeg(theAngle) * anOffY);
	theEmitter->mVx = (float)((double)CosDeg(theAngle) * aVx - (double)SinDeg(theAngle) * aVy);
	theEmitter->mVy = (float)((double)SinDeg(theAngle) * aVx + (double)CosDeg(theAngle) * aVy);
	aPrev = static_cast<Emitter*>(theEmitter->mPrev);
	theEmitter->mStartAlpha = aPrev->mStartAlpha;
	theEmitter->mEndAlpha = aPrev->mEndAlpha;
	theEmitter->mTime = aPrev->mTime;
	theEmitter->mDrawKind = 4;
	theEmitter->mStartR = 0xff;
	theEmitter->mStartG = 0xff;
	theEmitter->mStartB = 0xff;
	theEmitter->mEndR = 0xff;
	theEmitter->mEndG = 0xff;
	theEmitter->mEndB = 0xff;
	theEmitter->mVariance = 0;
}

// The parent, colours and variance are not copied: the caller (0x408900) sets the parent, and the colours keep what
// the allocation left (the port's allocation clears them; they only matter to draw kinds 3 and 5, which emitters
// never use, and to how many random numbers the colour jitter draws)
void Emitter_Copy(Emitter* theEmitter, const Emitter* theFrom)
{
	Obj_Copy(theEmitter, theFrom);
	theEmitter->mFrequency = theFrom->mFrequency;
	theEmitter->mRotation = theFrom->mRotation;
	theEmitter->mStartAlpha = theFrom->mStartAlpha;
	theEmitter->mEndAlpha = theFrom->mEndAlpha;
	theEmitter->mTime = theFrom->mTime;
}

// The original repeats the whole call for either parent; the x and y scaling are leftovers of a screen size
void Emitter_FromXml(Emitter* theEmitter, const XmlTag* theTag)
{
	if (Xml_GetInt(theTag, "copy_prev") != 0)
	{
		Emitter_CopyPrev(theEmitter, Xml_GetInt(theTag, "rotation"));
		return;
	}
	// (a missing emit_type would crash the original; here it is not angular)
	const char* aType = Xml_GetString(theTag, "emit_type");
	int aKind = (aType != nullptr && std::strcmp(aType, "angular") == 0) ? EMITTER_ANGULAR : EMITTER_TRAILING;
	Obj* aParent;
	if (gLevels.mCurrent != nullptr && gLevels.mCurrent->mEnemies.mCurrent != nullptr)
		aParent = gLevels.mCurrent->mEnemies.mCurrent;
	else
		aParent = gEnemyDefs.mCurrent;
	float aTime = (float)Xml_GetFloat(theTag, "time");
	float anEndAlpha = (float)Xml_GetFloat(theTag, "end_alpha");
	float aStartAlpha = (float)Xml_GetFloat(theTag, "start_alpha");
	const char* anAnim = Xml_GetString(theTag, "animation");
	int aRotation = Xml_GetInt(theTag, "rotation");
	int aFrequency = Xml_GetInt(theTag, "frequency");
	float aYSpeed = (float)((Xml_GetInt(theTag, "y_speed") * SCREEN_HEIGHT) / SCREEN_HEIGHT);
	float aXSpeed = (float)((Xml_GetInt(theTag, "x_speed") * SCREEN_WIDTH) / SCREEN_WIDTH);
	int aY = (Xml_GetInt(theTag, "y") * SCREEN_HEIGHT) / SCREEN_HEIGHT;
	int aX = (Xml_GetInt(theTag, "x") * SCREEN_WIDTH) / SCREEN_WIDTH;
	Emitter_Setup(theEmitter, aParent, aX, aY, aXSpeed, aYSpeed, aFrequency, aRotation, anAnim, aKind, aStartAlpha,
		anEndAlpha, aTime);
}

void ParticleSystem_New()
{
	Level* aLevel = gLevels.mCurrent;
	ParticleSystem* aSystem = &gSystems[gSystemNext];
	List_Append(&aLevel->mSystems, aSystem);
	gSystemNext = gSystemNext + 1;
	List_Init(&aSystem->mParticles);
	if (3999 < gSystemNext)
		gSystemNext = 0;
}

// 0x40b1f0
void ParticleSystem_Add(ParticleSystem* theSystem, float theX, float theY, float theVx, float theVy,
	int theFirstImage, int theLastImage, int theFrameTime, int theStartR, int theStartG, int theStartB, int theEndR,
	int theEndG, int theEndB, int theStartAlpha, int theEndAlpha, int theLifetime, float theDrag, float theGravity,
	int theDrawKind)
{
	List_Append(&theSystem->mParticles, &gParticles[gParticleNext]);
	gParticleNext = gParticleNext + 1;
	if (7999 < gParticleNext)
		gParticleNext = 0;
	Particle* aParticle = theSystem->mParticles.mCurrent;
	aParticle->mImage = nullptr;
	aParticle->mKind = 0;
	aParticle->mState = 0;
	aParticle->mFrame = 0;
	aParticle->mFrameTimer = 0;
	aParticle->mMidHandle = 0;
	aParticle->mAge = 0.0f;
	aParticle->mAnim = nullptr;
	aParticle->mX = theX;
	aParticle->mY = theY;
	aParticle->mDepth = 8;
	aParticle->mOldX = theX;
	aParticle->mOldY = theY;
	aParticle->mVx = theVx;
	aParticle->mVy = theVy;
	aParticle->mFirstImage = theFirstImage;
	aParticle->mFrameCount = (theLastImage - theFirstImage) + 1;
	aParticle->mFrameTime = theFrameTime;
	aParticle->mStartR = theStartR;
	aParticle->mStartG = theStartG;
	aParticle->mStartB = theStartB;
	aParticle->mEndR = theEndR;
	aParticle->mEndG = theEndG;
	aParticle->mEndB = theEndB;
	aParticle->mStartAlpha = theStartAlpha;
	aParticle->mEndAlpha = theEndAlpha;
	aParticle->mLifetime = (float)theLifetime;
	aParticle->mDrag = theDrag;
	aParticle->mGravity = theGravity;
	aParticle->mDrawKind = theDrawKind;
	aParticle->mMidHandle = 1;
	aParticle->mDirty = 1;
	aParticle->mOwner = theSystem;
	if (theSystem->mParticles.mCurrent == theSystem->mParticles.mHead)
	{
		theSystem->mX = theX;
		theSystem->mY = theY;
		theSystem->mVx = theVx;
		theSystem->mVy = theVy;
	}
}

void ParticleSystem_Draw(ParticleSystem* theSystem)
{
	if (theSystem->mParticles.mCurrent != nullptr)
	{
		theSystem->mParticles.mCurrent = theSystem->mParticles.mHead;
		Particle_Draw(theSystem->mParticles.mHead);
	}
}

int Particle_Follow(Particle* theParticle)
{
	if (theParticle->mAge == 0.0f)
	{
		ParticleSystem* anOwner = theParticle->mOwner;
		theParticle->mX = anOwner->mX;
		theParticle->mY = anOwner->mY;
		theParticle->mVx = anOwner->mVx;
		theParticle->mVy = anOwner->mVy;
	}
	theParticle->mVx = theParticle->mDrag * theParticle->mVx;
	float aVy = theParticle->mDrag * theParticle->mVy;
	theParticle->mVy = aVy + theParticle->mGravity;
	int aResult = Particle_Update(theParticle);
	theParticle->mOwner->mX = theParticle->mX;
	theParticle->mOwner->mY = theParticle->mY;
	theParticle->mOwner->mVx = theParticle->mVx;
	theParticle->mOwner->mVy = theParticle->mVy;
	return aResult;
}

int ParticleSystem_Update(ParticleSystem* theSystem)
{
	List<Particle>* aList = &theSystem->mParticles;
	if (aList->mCurrent != nullptr)
	{
		aList->mCurrent = aList->mHead;
		if (Particle_Follow(aList->mHead) == 0)
		{
			List_Unlink(aList);
			if (aList->mCurrent == nullptr)
				return 0;
		}
	}
	return 1;
}

// 0x40b7d0's colour jitter: the colour moved by up to the variance and kept in 0..255, where (as in the original's
// clamp) each test draws a new random number
static int JitterColour(int theColour, int theVariance)
{
	if (RandRange(-theVariance, theVariance) + theColour >= 0 &&
		RandRange(-theVariance, theVariance) + theColour > 0xff)
		return 0xff;
	if (RandRange(-theVariance, theVariance) + theColour < 0)
		return 0;
	return RandRange(-theVariance, theVariance) + theColour;
}

void Emitter_Update(Obj* theObj)
{
	Emitter* anEmitter = static_cast<Emitter*>(theObj);
	float anAge = (float)((double)anEmitter->mAge + 1.0);
	anEmitter->mAge = anAge;
	int n = (int)anAge;
	if (n % anEmitter->mFrequency != 0)
		return;
	Obj* aParent = anEmitter->mParent;
	int aBaseX = (int)((double)aParent->mX + (double)anEmitter->mOldX);
	int aBaseY = (int)((double)aParent->mY + (double)anEmitter->mOldY);
	float aVx = anEmitter->mVx;
	float aVy = anEmitter->mVy;
	int aStartR = anEmitter->mStartR;
	int aStartG = anEmitter->mStartG;
	int aStartB = anEmitter->mStartB;
	int anEndR = anEmitter->mEndR;
	int anEndG = anEmitter->mEndG;
	int anEndB = anEmitter->mEndB;
	int aLast = anEmitter->mFrameCount - 1;
	int x;
	int y;
	if (anEmitter->mKind == EMITTER_ANGULAR)
	{
		int anAngle = (aParent->mFrame * anEmitter->mRotation) / aParent->mAnim->mCount;
		int aRx = (int)((double)CosDeg(anAngle) * anEmitter->mOldX - (double)SinDeg(anAngle) * anEmitter->mOldY);
		int aRy = (int)((double)SinDeg(anAngle) * anEmitter->mOldX + (double)CosDeg(anAngle) * anEmitter->mOldY);
		x = (int)((double)aRx + (double)anEmitter->mParent->mX);
		y = (int)((double)aRy + (double)anEmitter->mParent->mY);
		float aSx = anEmitter->mVx;
		float aSy = anEmitter->mVy;
		if (anEmitter->mVx < 0.0f)
		{
			// a spread: both speeds drawn from the x speed's range
			int aHigh = (int)(-(double)anEmitter->mVx * 1000.0);
			int aLow = (int)(1000.0 * (double)anEmitter->mVx);
			aSx = (float)((double)RandRange(aLow, aHigh) / 1000.0);
			aHigh = (int)(-(double)anEmitter->mVx * 1000.0);
			aLow = (int)(1000.0 * (double)anEmitter->mVx);
			aSy = (float)((double)RandRange(aLow, aHigh) / 1000.0);
		}
		aVx = (float)((double)CosDeg(anAngle) * aSx + (double)anEmitter->mParent->mVx - (double)SinDeg(anAngle) * aSy);
		aVy = (float)((double)SinDeg(anAngle) * aSx + (double)anEmitter->mParent->mVy + (double)CosDeg(anAngle) * aSy);
	}
	else
	{
		if (anEmitter->mKind == EMITTER_SCATTERED)
		{
			RandMod(360);
			float aVxScaled = (float)((double)aVx * 1000.0);
			float aVyScaled = (float)((double)aVy * 1000.0);
			int aHigh = (int)aVxScaled;
			aVx = (float)((double)RandRange((int)-aVxScaled, aHigh) / 1000.0 + (double)anEmitter->mParent->mVx);
			aHigh = (int)aVyScaled;
			aVy = (float)((double)RandRange((int)-aVyScaled, aHigh) / 1000.0 + (double)anEmitter->mParent->mVy);
			aHigh = (int)aVx;
			x = (int)((double)RandRange((int)-aVx, aHigh) / 1000.0 + (double)aBaseX);
			aHigh = (int)aVy;
			y = (int)((double)RandRange((int)-aVy, aHigh) / 1000.0 + (double)aBaseY);
		}
		else
		{
			aVx = aParent->mVx * 0.5f;
			aVy = aParent->mVy * 0.5f;
			x = (int)aParent->mX;
			y = (int)aParent->mY;
		}
		aStartR = JitterColour(anEmitter->mStartR, anEmitter->mVariance);
		aStartG = JitterColour(anEmitter->mStartG, anEmitter->mVariance);
		aStartB = JitterColour(anEmitter->mStartB, anEmitter->mVariance);
		anEndR = JitterColour(anEmitter->mEndR, anEmitter->mVariance);
		anEndG = JitterColour(anEmitter->mEndG, anEmitter->mVariance);
		anEndB = JitterColour(anEmitter->mEndB, anEmitter->mVariance);
	}
	ParticleSystem_New();
	ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, (float)x, (float)y, aVx, aVy, 0, aLast, 0, aStartR, aStartG,
		aStartB, anEndR, anEndG, anEndB, anEmitter->mStartAlpha, anEmitter->mEndAlpha, anEmitter->mTime, 0.999f, 0.0f, anEmitter->mDrawKind);
	gLevels.mCurrent->mSystems.mCurrent->mParticles.mCurrent->mAnim = anEmitter->mAnim;
	ParticleSystem* aSystem = gLevels.mCurrent->mSystems.mCurrent;
	aSystem->mParticles.mCurrent->mDepth = 0;
}

} // namespace cb
