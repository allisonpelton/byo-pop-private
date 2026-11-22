// Emitter: the particle effects (0x427b60..0x428190 in the original, the presets 0x42fd80..0x4346e0). An emitter keeps
// a ring of particle objects, each a copy of the emitter's own picture and tracks; every mRate frames it sends a burst
// of mBurst of them out from its position into the particle layer, each at an angle, a jitter from the position and
// a depth just above the emitter's. A preset builds an emitter's picture and tracks and sets those numbers; an
// EmitterSet holds one emitter of each preset for one object.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;

const int EMITTER_MAX_PARTICLES = 512;

// How each particle's angle is chosen (Emitter::mAngleMode)
enum EmitterAngle
{
	EMITTER_ANGLE_OWN = 0,     // the emitter's own orbit angle
	EMITTER_ANGLE_SPREAD = 1,  // the burst spread evenly round the circle from the angle given
	EMITTER_ANGLE_JITTER = 2,  // the angle given, give or take mAngleJitter degrees
};

class Emitter : public ObjBase
{
public:
	int mDelay;              // +0x27c: frames before the emitter starts; its sound plays as the wait ends
	int mParticleCount;      // +0x280: the ring's size
	int mRate;               // +0x284: a burst every this many frames
	int mBurst;              // +0x288: particles a burst
	int mFrameCount;         // +0x28c
	int mDepth;              // +0x290: the particles' depth above the emitter's
	int mJitterX;            // +0x294: a particle's start, give or take this many pixels
	int mJitterY;            // +0x298
	int mAngleJitter;        // +0x29c: degrees
	int mRandomFrames;       // +0x2a0: a particle starts on a random cel below this, if above 0
	int mMadeWithFlag;       // +0x2a4: given by the constructor; nothing reads it
	int mUnused2a8;          // +0x2a8: set to 0 by the constructor; nothing reads it
	float mX;                // +0x2ac: the position its own updates emit from
	float mY;                // +0x2b0
	bool mEmitOnCall;        // +0x2b4: emits only when told (Emit), from the place given, not on its own updates
	int mAngleMode;          // +0x2b8: EmitterAngle
	int mSound;              // +0x2bc: played as the delay ends, or -1
	ObjBase* mParticles[EMITTER_MAX_PARTICLES]; // +0x2c8
	int mNextParticle;       // +0xac8

	static ObjBase* sLayer;  // 0x60e4e0: the particles' parent, just behind everything at depth 0
	static Anim* sParticlesAnim; // 0x60e550: images/particles, the presets' picture

	Emitter(ObjBase* theParent, bool theFlag);
	~Emitter();
	void Deactivate(ObjBase* theObj) override;
	bool Update() override;

	void SetSoundAtStart() { mSound = 0x53; }   // 0x427d10
	void SetEmitOnCall() { mEmitOnCall = true; } // 0x427d40
	void SetAngleOwn() { mAngleMode = EMITTER_ANGLE_OWN; } // 0x428030
	void Setup(int theParticleCount, int theRate, int theBurst, int theX, int theY, int theJitterX, int theJitterY,
		int theAngleJitter);
	void Emit(float theX, float theY, float theAngle);

	static void SetDepth(Emitter* theEmitter, float theDepth); // 0x427d20
	static void CreateLayer();                  // 0x428190
	static void LoadParticles();                // 0x4346e0
};


// One emitter of each preset, made on demand, for one object (0x42fd80; 0x38 bytes in the original)
struct EmitterSet
{
	Emitter* mFlame;             // +0x0
	ObjBase* mParent;            // +0x4: the emitters' parent
	Emitter* mStarRing;          // +0x8
	Emitter* mSwell;             // +0xc
	Emitter* mDust;              // +0x10
	Emitter* mSlowSwell;         // +0x14
	Emitter* mRisingSwell;       // +0x18
	Emitter* mShortSwell;        // +0x1c
	Emitter* mBlueSparks;        // +0x20
	Emitter* mRedSparks;         // +0x24
	Emitter* mSmoke;             // +0x28
	Emitter* mGreyRing;          // +0x2c
	Emitter* mPaleBurst;         // +0x30
	Emitter* mGoldBurst;         // +0x34

	EmitterSet(ObjBase* theParent);
	~EmitterSet();
	void MakeFlame(float theDepth); // 0x42fed0
	void MakeStarRing(float theDepth, int theCount); // 0x42fe80
	void MakeSwell(float theDepth); // 0x42ff10
	void MakeDust(float theDepth, int theCount); // 0x42ff50
	void MakeSlowSwell(float theDepth); // 0x42ffa0
	void MakeRisingSwell(float theDepth, int theCount); // 0x42ffe0
	void MakeShortSwell(float theDepth, int theCount); // 0x430030
	void MakeBlueSparks(float theDepth); // 0x430080
	void MakeRedSparks(float theDepth); // 0x4300c0
	void MakeSmoke(float theDepth); // 0x430100
	void MakeGreyRing(float theDepth); // 0x430140
	void MakePaleBurst(float theDepth); // 0x430180
	void MakeGoldBurst(float theDepth); // 0x4301c0
	void BurstFlame(float theX, float theY); // 0x4343d0
	void BurstStarRing(float theX, float theY); // 0x4343a0
	void BurstSwell(float theX, float theY); // 0x434410
	void BurstDust(float theX, float theY); // 0x434370
	void BurstSlowSwell(float theX, float theY); // 0x434460
	void BurstRisingSwell(float theX, float theY); // 0x4344b0
	void BurstShortSwell(float theX, float theY); // 0x434500
	void BurstBlueSparks(ObjBase* theParent, float theX, float theY); // 0x434550: under theParent, if given
	void BurstRedSparks(float theX, float theY); // 0x4345b0
	void BurstSmoke(float theX, float theY); // 0x434600
	void BurstGreyRing(float theX, float theY); // 0x434650
	void BurstPaleBurst(float theX, float theY); // 0x434680
	void BurstGoldBurst(float theX, float theY); // 0x4346b0
};

} // namespace Sexy
