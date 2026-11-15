#ifndef __HAMMERHEADS_PARTICLES_H__
#define __HAMMERHEADS_PARTICLES_H__

// The game's particle library (0x473860-0x475500): an emitter sends out fragments (small pictures) that fly, spin and
// fade; the definitions of the eight effects the game uses are rows in ParticleDefs.inc. Poppers (Popper.h) play them.
// The library has its own random number generator, so particles never disturb the draws of the game's Rand().

#include <list>
#include <string>
#include <vector>

#include "Color.h"
#include "MTRand.h"

namespace Sexy
{

class Graphics;
class Image;

struct ParticleVector
{
	double			mX;
	double			mY;
	double			mZ;
};

// 0x4407d0: the vector scaled to length 1, or the vector itself when it is shorter than 1e-6
ParticleVector NormalizeParticleVector(const ParticleVector& theVector);

// The generator behind the particles: the 1.2 framework's MTRand seeded with its default (4357), made once by the
// application (0x4b2a40) and read through 0x4b1b40 (31 bits per draw)
class ParticleRand
{
public:
	MTRand			mRand;

public:
	ParticleRand();											// 0x4b2a40

	int				Next();									// 0x4b1b40
	double			NextUnit();								// 0x473930: from 0 up to (not quite) 1
	ParticleVector	NextVector();							// 0x473960: three draws, each coordinate from -0.5 up to 0.5
};

// How hard an emitter sends fragments out over its duration (0x473890)
struct ParticleEnvelope
{
	double			mAttack;
	double			mDecay;
	double			mSustain;
	double			mRelease;
	double			mStart;

	double			Eval(double theTime, double theDuration) const;
};

// What the emitter does with the fragment it sends out: the direction it goes in
enum
{
	PARTICLE_DIR_EMITTER = 0,		// the way the emitter moves
	PARTICLE_DIR_OPPOSITE = 1,		// against it
	PARTICLE_DIR_SCATTER = 2,		// away from the emitter, along the offset the fragment starts at
	PARTICLE_DIR_FIXED = 3			// along mDirection
};

// 0x475280 (the defaults) and 0x4753c0 (the copy). The row order of ParticleDefs.inc is the order of the fields.
struct EmitterDef
{
	std::string		mName;
	int				mCount;				// +0x40: the number of fragments it owns; it never has more in the air
	double			mRate;				// +0x48: fragments a second at an envelope of 1
	ParticleEnvelope mEnvelope;			// +0x50
	int				mDirMode;			// +0x78
	ParticleVector	mScatter;			// +0x80: how far from the emitter a fragment may start (times a random vector)
	ParticleVector	mDirection;			// +0x98: the fixed direction of mode 3
	ParticleVector	mJitter;			// +0xb0: added to the direction before it is scaled to the speed
	double			mSpeed;				// +0xc8
	double			mSpeedJitter;		// +0xd0
	ParticleVector	mGravity;			// +0xd8
	double			mDrag;				// +0xf0
	double			mDuration;			// +0xf8: the time the emitter sends fragments out
	double			mLifeJitter;		// +0x100: how much a fragment's life differs from the definition's
};

// 0x4745d0
struct FragmentDef
{
	std::string		mName;
	Image*			mImage;				// +0x40: a strip of frames
	int				mAnimMode;			// +0x44: 0 and 2 loop, 1 stops at the last frame, 3 goes there and back
	bool			mRandomStartFrame;	// +0x48
	double			mFrameRate;			// +0x50
	bool			mRandomAngle;		// +0x58
	double			mSpin;				// +0x60: turns a second
	bool			mColorize;			// +0x68: the fragment is drawn in a colour and fades out
	double			mFadeStart;			// +0x70: the age it starts to fade at
	int				mDrawMode;			// +0x78: 0 normal, 1 additive
	double			mLife;				// +0x80
	ParticleVector	mAcceleration;		// +0x88
	double			mTangentialAccel;	// +0xa0
	double			mRadialAccel;		// +0xa8
	double			mDampingX;			// +0xb8
	double			mDampingY;			// +0xc0
	double			mDamping;			// +0xd8
};

// 0x475500: the definitions, looked up by name. Made once the images are loaded.
class ParticleDefs
{
public:
	std::vector<EmitterDef>		mEmitters;
	std::vector<FragmentDef>	mFragments;

public:
	ParticleDefs();											// 0x475500
	const EmitterDef*	FindEmitter(const char* theName) const;	// 0x473f50
	const FragmentDef*	FindFragment(const char* theName) const;	// 0x473eb0
};

// One fragment in flight (0x474090-0x474700)
class ParticleFragment
{
public:
	ParticleVector	mPos;				// +0x00
	ParticleVector	mVel;				// +0x18
	double			mAge;				// +0x30
	double			mRateScale;			// +0x38: the effect's life scale
	double			mAngle;				// +0x40
	int				mFrameOffset;		// +0x48
	bool			mDead;				// +0x4c

public:
	ParticleFragment(ParticleRand& theRand, const FragmentDef& theDef);		// 0x474090
	void			Init(const ParticleVector& thePos, const ParticleVector& theVel, double theRateScale);	// 0x474140
	void			Update(const FragmentDef& theDef, double theDelta);	// 0x474180
	void			Draw(Graphics* g, bool theRotateFaded, const FragmentDef& theDef, const Color& theColor, int theOffsetX,
						int theOffsetY) const;								// 0x4743b0
};

// One emitter at work (0x474bf0-0x474dc0)
class ParticleEffect
{
public:
	const EmitterDef*	mEmitter;		// +0x00
	const FragmentDef*	mFragment;		// +0x04
	std::vector<ParticleFragment>	mFragments;	// +0x08: every fragment it owns
	std::vector<ParticleFragment*>	mPool;		// +0x08: those not in the air (the last one goes out first)
	std::list<ParticleFragment*>	mLive;		// +0x0c: those in the air, in the order they went out
	ParticleVector	mPos;				// +0x10
	ParticleVector	mVel;				// +0x28
	double			mAge;				// +0x40
	double			mSpawnAccumulator;	// +0x48: fragments owed
	Color			mColor;				// +0x50
	double			mLifeScale;			// +0x60
	bool			mStopped;			// +0x68: never set; the emitter would send nothing more out

public:
	ParticleEffect(ParticleRand& theRand, const EmitterDef* theEmitter, const FragmentDef* theFragment,
		const Color& theColor, const ParticleVector& thePos, const ParticleVector& theVel);	// 0x474bf0
	ParticleEffect(const ParticleEffect&) = delete;			// the pool and the live list point into mFragments
	ParticleEffect&	operator=(const ParticleEffect&) = delete;

	bool			Update(ParticleRand& theRand, double theDelta);			// 0x474dc0: true when fragments were in the air
	bool			IsFinished() const;										// 0x474700: out of time and nothing in the air
	void			Draw(Graphics* g, bool theRotateFaded, int theOffsetX, int theOffsetY) const;	// 0x4747d0
};

}

#endif
