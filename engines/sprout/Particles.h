#ifndef __SPROUT_PARTICLES_H__
#define __SPROUT_PARTICLES_H__

// The particles. A particle system (0xbc bytes, constructor 0x42b920 (wordharmony), vtable 0x483b70 (wordharmony)) is a
// container of streams read from a particle file. A stream (0x1b0 bytes, constructor 0x433390 (wordharmony), vtable
// 0x484518 (wordharmony)) makes its particles when it is read, a pool of them, and emits them as its children: a
// particle is free while the pool's reference is its only one. A particle (0xec bytes, constructor
// 0x435760 (wordharmony), vtable 0x484760 (wordharmony)) is a container holding one image; it moves in the screen's
// coordinates, so a stream that moves leaves its particles where they were. Each of the stream's ranges is a pair (x
// the low end, y the high end) that every particle draws its own value from.

#include "Node.h"
#include "Xml.h"
#include <string>
#include <vector>

namespace engine
{

class ParticleStream;

class Particle : public Container
{
public:
	Particle();  // 0x435760 (wordharmony)

	using Node::setPosition;
	void setPosition(float x, float y) override { mPosition.set(x, y); }  // 0x18: 0x42a400 (wordharmony)
	void applyTransform(Graphics* g) override;  // 0x3c: 0x435690 (wordharmony)
	void update(UpdateInfo& theInfo) override;  // 0x70: 0x4353b0 (wordharmony)

	ParticleStream* mStream;  // +0xa8
	Point mVelocity;          // +0xac (pixels a second)
	float mAcceleration;      // +0xb4: along the velocity
	float mStartScale;        // +0xb8
	float mEndScale;          // +0xbc
	float mStartOpacity;      // +0xc0
	float mEndOpacity;        // +0xc4
	Point mForce;             // +0xc8: added to the velocity each second
	bool mBlackHole;          // +0xd0: pulled towards the stream's black hole
	float mLife;              // +0xd4 (seconds)
	float mLifeLeft;          // +0xd8: the stream removes the particle when it goes below 0
	int mGroupRelative;       // +0xe0: placed like any node, in its stream's coordinates
};

// A particle's jitter (0x3c bytes, constructor 0x431930 (wordharmony), vtable 0x4843c4 (wordharmony)): moves the
// particle each frame by amount x cos(rate x time) on each axis whose amount is not 0 (a frame's step, not a position,
// so it adds up with the frames)
class JitterListener : public Object, public NodeListener
{
public:
	JitterListener(float theRateX, float theRateY, float theAmountX, float theAmountY);  // 0x431930 (wordharmony)

	// 0x431a00 (wordharmony)
	void update(UpdateInfo& theInfo, Node* theNode) override;

	float mRateX;    // +0x08
	float mRateY;    // +0x0c
	float mAmountX;  // +0x10
	float mAmountY;  // +0x14
	float mTime;     // +0x18
};

class ParticleStream : public Container
{
public:
	ParticleStream();             // 0x433390 (wordharmony)
	~ParticleStream() override;   // 0x432940 (wordharmony)

	void update(UpdateInfo& theInfo) override;  // 0x70: 0x4331b0 (wordharmony)
	void layout() override;                     // 0x74: 0x4328e0 (wordharmony)

	// 0x432880 (wordharmony): starts emitting, or the start delay when there is one, and the time to the automatic stop
	void startEmitting();
	// 0x4328d0 (wordharmony)
	void stopEmitting() { mEmitting = false; }
	// 0x44ea40 (feedingfrenzy): the particles in the air are taken off, and the pool's are hidden
	void clear();
	// 0x44e9f0 (feedingfrenzy): the node the stream's particles follow (null for none)
	void setTarget(Node* theNode) { mTarget = theNode; }
	// 0x432900 (wordharmony): not emitting or waiting to, and no particle left
	bool isDone();
	// 0x432e10 (wordharmony): sends a free particle off from thePosition (the stream's position on the screen)
	void emit(Point thePosition);
	// 0x4335b0 (wordharmony): makes the pool from the stream's images
	void makeParticles();

	float mEmitRate;                  // +0xa8: particles a second (5)
	int mMaxParticles;                // +0xac: 0 for no limit
	Point mVelocityX;                 // +0xb0
	Point mVelocityY;                 // +0xb8
	Point mAcceleration;              // +0xc0
	Point mPositionX;                 // +0xc8: from the stream's position
	Point mPositionY;                 // +0xd0
	Point mStartScale;                // +0xd8
	Point mEndScale;                  // +0xe0
	Point mRotation;                  // +0xe8: of the velocity, in degrees
	Point mJitterRateX;               // +0xf0
	Point mJitterAmountX;             // +0xf8
	Point mJitterRateY;               // +0x100
	Point mJitterAmountY;             // +0x108
	bool mConstantScale;              // +0x110: the end scale is the start scale
	bool mConstantOpacity;            // +0x111: the end opacity is the start opacity
	bool mBlackHole;                  // +0x112
	float mBlackHoleAcceleration;     // +0x114
	Point mBlackHolePosition;         // +0x118: from the stream's position on the screen
	Point mLifeTime;                  // +0x120
	Point mStartOpacity;              // +0x128
	Point mEndOpacity;                // +0x130
	Point mForce;                     // +0x138
	std::string mImagePath;           // +0x148: the images are <path>0, <path>1, ...
	int mFilter;                      // +0x164: the graphics' snap for the particles (Graphics::setSnap)
	// +0x168: a vector nothing fills
	std::vector<Particle*> mParticles;  // +0x178 (counted): the pool
	float mEmitCredit;                // +0x188: the particles owed; starts at 1, so the first update emits one
	bool mEmitting;                   // +0x18c
	int mGroupRelative;               // +0x190: offsetType "groupRelative"
	float mAutoStop;                  // +0x194: seconds of emitting (-1 none)
	float mAutoStopLeft;              // +0x198
	float mStartDelay;                // +0x19c: seconds before emitting (-1 none)
	float mStartDelayLeft;            // +0x1a0
	Node* mTarget;                    // +0x218 (feedingfrenzy): not counted
};

class ParticleSystem : public Container
{
public:
	ParticleSystem();  // 0x42b920 (wordharmony)

	void layout() override;  // 0x74: 0x4328e0 (wordharmony) (the stream's)

	// 0x42b6b0 (wordharmony): runs the streams for theSeconds at once (as one frame)
	void warmup(float theSeconds);
	// 0x42b6e0 (wordharmony)
	void startEmitting();
	// 0x441df0 (feedingfrenzy): every stream stops emitting, and the particles in the air finish their lives
	void stopEmitting();
	// 0x441ee0 (feedingfrenzy): every stream clears
	void clear();
	// 0x441ea0 (feedingfrenzy): the particles of a stream come out in the node's coordinates and follow it
	void setStreamTarget(int theIndex, Node* theNode);
	// 0x42b760 (wordharmony): every stream is done
	bool isDone();
	// 0x42b7a0 (wordharmony): reads the streams from a particle file of the archive
	bool load(std::string theFile);
};

// A pool of particle systems of one file (0xc8 bytes, constructor 0x42bb20 (wordharmony), vtable
// 0x483c68 (wordharmony)), its children
class ParticlePool : public Container
{
public:
	ParticlePool(std::string theFile, int theCount);  // 0x42bb20 (wordharmony)

	// 0x42ba00 (wordharmony): every system is done
	bool isDone();
	// 0x42ba40 (wordharmony): a system that is done, or else a new one
	ParticleSystem* getSystem();

	std::string mFile;  // +0xa8
};

// The particle file's reader (0x1c bytes, constructor 0x433870 (wordharmony), vtable 0x48462c (wordharmony)): a
// particleSystem element, and particleStream elements holding one element for each group of the stream's settings
class ParticleHandler : public XmlHandler
{
public:
	ParticleHandler();  // 0x433870 (wordharmony)

	void setSystem(ParticleSystem* theSystem) { mSystem = theSystem; }  // 0x4337f0 (wordharmony)

	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;  // 0x433ef0 (wordharmony)
	void endElement(const std::string& theName) override;  // 0x433940 (wordharmony)

private:
	void startStream(const XmlAttributes& theAttributes);  // 0x20: 0x433980 (wordharmony)

	ParticleStream* mStream;  // +0x14: the stream being read
	ParticleSystem* mSystem;  // +0x18
};

} // namespace engine

#endif
