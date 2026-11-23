#ifndef __SPROUT2_PARTICLESYSTEM_H__
#define __SPROUT2_PARTICLESYSTEM_H__

// A particle effect (ParticleSystem): a container of streams, each emitting particles at its rate from a pool it
// fills when the effect is made. A particle is a container of one image, and moves, grows and fades over its life by
// the ranges its stream drew for it. Every range is drawn with the game's rand(), so an effect's draws are part of the
// game's sequence of random numbers.
//
// An independent stream's particles are emitted where the stream is in the scene and stay there as the stream moves:
// their positions are kept in the scene's space, or relative to a named component (the stream's parent), and their
// transform puts them there whatever the containers above add. A group-relative stream's particles are placed in the
// stream's space.

#include "Container.h"
#include "KeyInterpolator.h"
#include "ParticleSystemDocHandler.h"

#include <list>
#include <string>
#include <vector>

namespace engine
{

class ParticleStream;

// One particle (Particle, 0x14c bytes, its constructor 0x45c330, its vtable 0x55f784 (feedingfrenzy2)), its class name
// the container's
class Particle : public Container
{
public:
	// 0x45c330 (feedingfrenzy2)
	Particle();

	using Container::setPosition;
	// 0x18 (0x43c170 (feedingfrenzy2)): the position only, the bounds not marked
	void setPosition(float x, float y) override
	{
		mPosition.x = x;
		mPosition.y = y;
	}
	// 0x6c (0x444b00 (feedingfrenzy2)): an independent particle is drawn at its position in the scene (or from its
	// parent), whatever the containers above it added
	void applyTransform(DrawContext* theContext) override;
	// 0xa4 (0x43c7b0 (feedingfrenzy2)): moved, and its scale and opacity set by how much of its life is gone
	void update(UpdateContext& theContext) override;

	ParticleStream* mStream;            // +0xb8: the stream that emitted it (not counted)
	Vector2f mVelocity;                 // +0xbc: in pixels a second
	float mAcceleration;                // +0xc4: along the velocity
	float mStartScale;                  // +0xc8
	float mEndScale;                    // +0xcc
	float mStartOpacity;                // +0xd0
	float mEndOpacity;                  // +0xd4
	Vector2f mExternalForce;            // +0xd8
	bool mBlackHole;                    // +0xe0: pulled towards the stream's black hole
	float mLifeTime;                    // +0xe4: seconds
	float mLife;                        // +0xe8: the seconds left, below 0 when it is to go
	Component* mOffsetParent;           // +0xec: what an independent particle's position is relative to, null for the
	                                    // scene (not counted)
	int mOffsetType;                    // +0xf0: ParticleOffsetType
	bool mHasOpacityKeys;               // +0xf4
	FloatKeyInterpolator mOpacityKeys;  // +0xf8
};

// One stream of an effect (ParticleStream, 0x208 bytes, its constructor 0x46d090, its vtable 0x55e4f4
// (feedingfrenzy2)). Its children are the particles alive; the pool holds every particle it made, a particle being
// free while the pool's is its only reference.
class ParticleStream : public Container
{
public:
	// 0x46d090 (feedingfrenzy2)
	ParticleStream();

	// 0x68 (0x52026d (feedingfrenzy2))
	std::string getClassName() override { return "ParticleStream"; }
	// 0xa4 (0x43ca60 (feedingfrenzy2)): the particles out of their bounds or their life taken out, then as many emitted
	// as the rate gives since the last frame
	void update(UpdateContext& theContext) override;
	// 0xa8 (0x508d6d (feedingfrenzy2)): the particles' bounds emptied, its own left as they are (the effect's
	// computeBounds does not reach its streams)
	void computeBounds() override;

	// 0x45b130 (feedingfrenzy2): the pool filled with as many particles as can be alive at once (half again the
	// longest life's worth at the rate, or the auto-stop's worth if shorter), at most the stream's maximum
	void fillPool(ParticleStreamDef* theDef);
	// 0x41ead0 (feedingfrenzy2): emitting begun, now or after the start delay
	void start();
	// 0x4573f0 (feedingfrenzy2): every particle taken out and hidden
	void reset();
	// Inlined in 0x4376f0 (feedingfrenzy2): not emitting nor waiting to, with no particle alive
	bool isDone() const { return !mEmitting && !(mStartDelayCountdown > 0) && mChildren.empty(); }

	float mEmitRate;                    // +0xb8: particles a second
	int mMaxParticles;                  // +0xbc: alive at once, 0 for no limit
	FloatRange mVelocityX;              // +0xc0
	FloatRange mVelocityY;              // +0xc8
	FloatRange mAcceleration;           // +0xd0
	FloatRange mPositionX;              // +0xd8
	FloatRange mPositionY;              // +0xe0
	FloatRange mStartScale;             // +0xe8
	FloatRange mEndScale;               // +0xf0
	FloatRange mRotation;               // +0xf8
	FloatRange mJitterPeriodX;          // +0x100
	FloatRange mJitterAmplitudeX;       // +0x108
	FloatRange mJitterPeriodY;          // +0x110
	FloatRange mJitterAmplitudeY;       // +0x118
	bool mConstantScale;                // +0x120
	bool mConstantOpacity;              // +0x121
	bool mBlackHole;                    // +0x122
	float mBlackHoleAcceleration;       // +0x124
	Vector2f mBlackHolePosition;        // +0x128
	FloatRange mLifeTime;               // +0x130
	RectI mParticleBounds;              // +0x138: from the stream's position; empty for none
	FloatRange mStartOpacity;           // +0x148
	FloatRange mEndOpacity;             // +0x150
	Vector2f mExternalForce;            // +0x158
	std::string mPath;                  // +0x160: copied from the definition, read by nothing after
	int mPivot;                         // +0x17c: the same
	bool mAbsolutePivot;                // +0x180: the same
	Vector2f mPivotPoint;               // +0x184: the same
	int mFilterMode;                    // +0x18c: the same
	std::vector<Ref<Particle>> mPool;   // +0x1a0
	float mCarry;                       // +0x1b0: the part of a particle left over from the last frame
	bool mEmitting;                     // +0x1b4
	int mOffsetType;                    // +0x1b8: ParticleOffsetType
	int mSequence;                      // +0x1bc: ParticleSequence
	float mAutoStop;                    // +0x1c0
	float mAutoStopCountdown;           // +0x1c4
	float mStartDelay;                  // +0x1c8
	float mStartDelayCountdown;         // +0x1cc
	std::string mParentName;            // +0x1d0
	Component* mOffsetParent;           // +0x1ec: the component named mParentName when the stream started (not
	                                    // counted)
	std::vector<FloatKey> mOpacityKeys; // +0x1f0

private:
	// 0x4392c0 (feedingfrenzy2): a free particle from the pool, given its ranges' draws and added; nothing when 25
	// tries at random find none free
	void emit(Vector2f theBase);
};

// An effect (ParticleSystem, 0xd0 bytes, its constructor 0x478200, its vtable 0x55be84 (feedingfrenzy2))
class ParticleSystem : public Container
{
public:
	// 0x478200 (feedingfrenzy2)
	ParticleSystem();

	// 0x68 (0x52102a (feedingfrenzy2))
	std::string getClassName() override { return "ParticleSystem"; }
	// 0xa4 (0x43c0f0 (feedingfrenzy2)): stopped when its time is up, and marked for deletion when done if it is to be
	void update(UpdateContext& theContext) override;
	// 0xa8 (0x4237c0 (feedingfrenzy2)): empty bounds; its streams are not reached
	void computeBounds() override;

	// 0x481c10 (feedingfrenzy2): the effect's file read, its images loaded and its streams made (the original
	// returns nothing; false here only for a file without an effect, which the original does not survive)
	bool load(const std::string& thePath);
	// 0x472e80 (feedingfrenzy2): the streams made from the definition
	bool init(ParticleSystemDef* theDef);
	// 0x4306a0 (feedingfrenzy2): every stream emptied if the effect was stopped, then started, and the warmup run
	void start();
	// 0x4322f0 (feedingfrenzy2): every stream stops emitting
	void stop();
	// 0x4376f0 (feedingfrenzy2): every stream done
	bool isDone();
	// 0x544117 (feedingfrenzy2): the seconds run at once, in one update of the children (its own update skipped)
	void warmup(float theSeconds);
	// 0x47df40 (feedingfrenzy2): the bounds every stream's particles die outside of
	void setParticleBounds(const RectI& theBounds);
	// 0x42a650 (feedingfrenzy2): every stream's particles taken out
	void resetStreams();
	// Set in place by those that start an effect once (the stage end's counts, 0x540881 (feedingfrenzy2))
	void setDeleteWhenDone(bool theDeleteWhenDone) { mDeleteWhenDone = theDeleteWhenDone; }
	// 0x42c060 (feedingfrenzy2): no stream's particles placed relative to a named component until it starts again
	void clearOffsetParents();

private:
	// 0x462e60 (feedingfrenzy2): a stream made from its definition, its pool filled
	bool addStream(ParticleStreamDef* theDef);

	bool mStarted;              // +0xb8
	bool mDeleteWhenDone;       // +0xb9
	float mAutoStop;            // +0xbc: seconds before the streams stop, when above 0
	float mAutoStopCountdown;   // +0xc0
	float mWarmup;              // +0xc4
};

// A theme's copies of one effect (ParticleSystemPool, 0x4c bytes, its constructor 0x47dea0 (feedingfrenzy2)). A copy
// handed out is the pool's again once the pool holds its only reference.
class ParticleSystemPool : public Object
{
public:
	// 0x47dea0 (feedingfrenzy2): the count made at once
	ParticleSystemPool(ParticleSystemDef* theDef, int theCount);

	// 0x42a480 (feedingfrenzy2): a free copy, stopped and emptied when it came back; with none, a new one; null when
	// the definition makes none
	ParticleSystem* get();

private:
	// +0x0c: a string nothing sets or reads
	Ref<ParticleSystemDef> mDef;            // +0x28
	std::list<Ref<ParticleSystem>> mFree;   // +0x2c
	std::list<Ref<ParticleSystem>> mUsed;   // +0x38: handed out
};

} // namespace engine

#endif
