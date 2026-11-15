#ifndef __PIXELUS_PARTICLES_H__
#define __PIXELUS_PARTICLES_H__

// Particles.cpp: the Nuclide layer's particle effects (Pixelus.exe 0x43d000-0x43f800). The application builds the table of
// them once (0x43f3a0): picture defs, which say what one particle looks like and how it moves (0x43e600), and emitter
// defs, which say how many particles an effect throws and how (0x43f260). An effect (0x43ebe0) is an emitter def, a
// picture def, a colour and a place; it runs from the interface's updates of 0.01 s (0x43eda0) and is drawn by 0x43e7c0.
// Every draw of a random number is the application's generator's (0x431090 gives a number from 0 up to 1, 0x4310c0 three
// numbers from -0.5 up to 0.5), so what an effect does is the same in every run.
//
// port: the table is built from the exe's code (the bytes it stores), not read from a file; the picture strips are cut by
// their frame (0x436c50: the original trims the empty border of each and draws it at an offset, which draws the same pixels).

#include <string>
#include <list>
#include <vector>

namespace Sexy
{

class Graphics;
class Image;
class PixelusApp;

struct ParticleVec
{
	double x, y, z;
	ParticleVec() : x(0), y(0), z(0) {}
	ParticleVec(double theX, double theY, double theZ) : x(theX), y(theY), z(theZ) {}
};

// the five numbers of 0x42fa00 / 0x42fa30: how strongly an effect throws over its time (0x42fa60)
struct ParticleCurve
{
	double		mAttack;			// +0x00 time to rise from 0 to 1
	double		mDecay;				// +0x08 time to fall to mSustain
	double		mSustain;			// +0x10
	double		mRelease;			// +0x18 time to fall to 0 at the end
	double		mStart;				// +0x20 time the curve starts at (0 before it)
	double		Eval(double theAge, double theDuration) const;
};

// 0xe8 bytes (0x43e600): what a particle looks like and how it moves
struct ParticleImageDef
{
	std::string		mName;				// +0x00
	Image*			mImage;				// the strip of mFrames pictures side by side
	int				mFrames;
	int				mPivotX, mPivotY;	// the middle of a picture
	int				mAnimMode;			// +0x44 0 the first picture, 1 once, 2 repeating, 3 back and forth
	bool			mRandomFrame;		// +0x48 starts at a random picture
	double			mFramesPerSecond;	// +0x50
	bool			mRandomAngle;		// +0x58 starts turned at random
	double			mTurnRate;			// +0x60 turns a second
	bool			mFades;				// +0x68 fades out after mFadeStart (colours the picture)
	double			mFadeStart;			// +0x70
	int				mDrawMode;			// +0x78
	double			mLifetime;			// +0x80 in the particle's own seconds
	ParticleVec		mGravity;			// +0x88
	double			mSwirl;				// +0xa0 acceleration at right angles to the way it flies
	double			mSpeedUp;			// +0xa8 acceleration along it
	double			mDragX;				// +0xb8
	double			mDragY;				// +0xc0
	double			mDrag;				// +0xd8
};

// 0x108 bytes (0x43f260): how an effect throws its particles
struct ParticleEmitterDef
{
	std::string		mName;				// +0x00
	int				mCount;				// +0x40 particles ready at most
	double			mRate;				// +0x48 a second at full strength
	ParticleCurve	mCurve;				// +0x50
	int				mMode;				// +0x78 which way they go: 0 the effect's, 1 against it, 2 away from the place, 3 mDirection
	ParticleVec		mOffset;			// +0x80 the box they start in
	ParticleVec		mDirection;			// +0x98
	ParticleVec		mSpread;			// +0xb0 how much they stray from the way
	double			mSpeed;				// +0xc8
	double			mSpeedSpread;		// +0xd0
	ParticleVec		mGravity;			// +0xd8 for the effect itself
	double			mDrag;				// +0xf0
	double			mDuration;			// +0xf8
	double			mRateSpread;		// +0x100
};

// 0x50 bytes (0x43e160)
struct Particle
{
	ParticleVec		mPos;				// +0x00
	ParticleVec		mVel;				// +0x18
	double			mAge;				// +0x30
	double			mAgeRate;			// +0x38
	double			mAngle;				// +0x40
	int				mFrameStart;		// +0x48
	bool			mDead;				// +0x4c
};

class ParticleLibrary
{
public:
	std::vector<ParticleImageDef*>		mImageDefs;
	std::vector<ParticleEmitterDef*>	mEmitterDefs;

public:
	ParticleLibrary(PixelusApp* theApp);		// 0x43f3a0
	~ParticleLibrary();

	ParticleImageDef*		FindImageDef(const char* theName);		// 0x43df80
	ParticleEmitterDef*		FindEmitterDef(const char* theName);	// 0x43e020
};

// 0x68 bytes (0x43ebe0)
class ParticleEffect
{
public:
	PixelusApp*				mApp;
	ParticleEmitterDef*		mEmitter;			// +0x00
	ParticleImageDef*		mImageDef;			// +0x04
	std::vector<Particle*>	mFree;				// +0x08 ready to be thrown
	std::list<Particle*>	mActive;			// +0x0c
	ParticleVec				mPos;				// +0x10
	ParticleVec				mVel;				// +0x28
	double					mAge;				// +0x40
	double					mPending;			// +0x48 particles owed
	int						mColor[4];			// +0x50 red, green, blue, alpha
	double					mRateFactor;		// +0x60 how fast its particles age

public:
	ParticleEffect(PixelusApp* theApp, ParticleEmitterDef* theEmitter, ParticleImageDef* theImageDef, const int theColor[4],
		const ParticleVec& thePos, const ParticleVec& theVel);
	~ParticleEffect();

	bool		Update(double theStep);		// 0x43eda0: false once nothing is left in the air
	void		Draw(Graphics* g, bool theRotateFading);	// 0x43e7c0

private:
	double		Rand01();					// 0x431090
	ParticleVec	RandVec();					// 0x4310c0
	void		UpdateParticle(Particle* theParticle, double theStep);	// 0x43e250
	void		DrawParticle(Graphics* g, const Particle* theParticle, bool theRotateFading);	// 0x43e480
};

}

#endif
