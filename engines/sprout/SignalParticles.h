#ifndef __SPROUT_SIGNALPARTICLES_H__
#define __SPROUT_SIGNALPARTICLES_H__

// The signal build's particles (Pizza Frenzy's; Iggle Pop's Mac build names Particle, ParticleStream, ParticleSystem,
// ParticleStreamDef, ParticleSystemDef and ParticleSystemDocHandler). They differ from Word Harmony's (Particles.h),
// whose classes keep their names: the signal build keeps a definition of what a file says, makes each system from it,
// and every system, stream and particle is a node of its own.
//
// A file's document (a particleSystem element and a particleStream element for each stream, holding one element for
// each group of the stream's settings) is read into a ParticleSystemDef of ParticleStreamDefs. A system
// (SignalParticleSystem, 0x140 bytes, constructor 0x479470 (pizzafrenzy)) is made from a definition: it makes a stream
// (SignalParticleStream, 0x22c bytes, constructor 0x4942e0 (pizzafrenzy)) for each of the definition's streams, and
// the stream makes a pool of particles (SignalParticle, 0x16c bytes, constructor 0x498140 (pizzafrenzy)) when it is
// made. A stream emits its particles as its children: a particle is free while the pool's reference is its only one.
// Each of a stream's ranges is a pair (x the low end, y the high end) that every particle draws its own value from.
//
// Left out: a stream's "parent" attribute, which would place its particles from a node the application finds by name
// (0x461f40 (pizzafrenzy)); no file of the game has one, so no particle has such a node.

#include "Node.h"
#include "Xml.h"
#include <string>
#include <vector>

namespace engine
{

class Image;
class SignalParticleStream;

// What the sequence attribute of a stream says: the pictures of its pool in order, or at random (neither differs in
// what is made), or an animation (a sequence of pictures that plays)
enum
{
	PARTICLE_SEQUENCE_RANDOM = 0,
	PARTICLE_SEQUENCE_SEQUENTIAL = 1,
	PARTICLE_SEQUENCE_ANIMATION = 2
};

// A stream as a file says it (0x11c bytes, constructor 0x47de40 (pizzafrenzy)); the system copies it into a stream
class ParticleStreamDef : public Object
{
public:
	ParticleStreamDef();  // 0x47de40 (pizzafrenzy)

	// 0x47e2a0 (pizzafrenzy): finds the pictures <path>0, <path>1, ... (a .png, or else a .jpg) up to the first
	// missing one, once; whether there is one
	bool loadImages();

	Point mOffset;                  // +0x0c
	float mEmitRate;                // +0x14: particles a second
	long mMaxParticles;             // +0x18: 0 for no limit
	int mSequence;                  // +0x1c
	float mAnimationRate;           // +0x20: pictures a second of an animation
	Point mVelocityX;               // +0x24
	Point mVelocityY;               // +0x2c
	Point mAcceleration;            // +0x34
	Point mPositionX;               // +0x3c
	Point mPositionY;               // +0x44
	Point mStartScale;              // +0x4c
	Point mEndScale;                // +0x54
	Point mRotation;                // +0x5c: of the velocity, in degrees
	Point mJitterPeriodX;           // +0x64
	Point mJitterSizeX;             // +0x6c
	Point mJitterPeriodY;           // +0x74
	Point mJitterSizeY;             // +0x7c
	bool mConstantScale;            // +0x84: the end scale is the start scale
	bool mConstantOpacity;          // +0x85: the end opacity is the start opacity
	bool mBlackHole;                // +0x86
	float mBlackHoleAcceleration;   // +0x88
	Point mBlackHolePosition;       // +0x8c
	Point mLifeTime;                // +0x94
	Point mStartOpacity;            // +0x9c
	Point mEndOpacity;              // +0xa4
	Point mForce;                   // +0xac
	std::string mImagePath;         // +0xbc: the pictures are <path>0, <path>1, ...
	std::string mParent;            // +0xd8: the node the stream's particles are placed from (nothing reads it)
	int mSmoothType;                // +0xf4: the particles' smooth type ("filter" X, Y or XY: 1)
	int mGroupRelative;             // +0xf8: offsetType "groupRelative"
	float mAutoStop;                // +0xfc: seconds of emitting (-1 none)
	float mStartDelay;              // +0x100: seconds before emitting (-1 none)
	std::vector<Ref<Image>> mImages;  // +0x108: the pictures, by loadImages
};

// A system as a file says it (0x2c bytes, constructor 0x47ddb0 (pizzafrenzy))
class ParticleSystemDef : public Object
{
public:
	ParticleSystemDef();  // 0x47ddb0 (pizzafrenzy)

	// 0x47e480 (pizzafrenzy): each stream's pictures are found; whether every stream has one
	bool loadImages();

	float mAutoStop;                          // +0x0c: seconds of emitting (-1 none)
	float mWarmup;                            // +0x10: seconds the system runs when it starts
	std::vector<Ref<ParticleStreamDef>> mStreams;  // +0x18
};

// The particle file's reader (0x20 bytes, constructor 0x47bc40 (pizzafrenzy), vtable 0x50329c (pizzafrenzy)): a
// particleSystem element starts the definition and each particleStream element a stream's, in which an element for
// each group of settings sets them
class ParticleSystemDocHandler : public XmlHandler
{
public:
	ParticleSystemDocHandler() {}  // 0x47bc40 (pizzafrenzy)

	// 0x47be40 (pizzafrenzy)
	ParticleSystemDef* getSystemDef() { return mSystemDef; }

	// 0x47c9b0 (pizzafrenzy): a settings element outside a stream is passed over
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;
	// 0x47be70 (pizzafrenzy): the stream is complete at its end
	void endElement(const std::string& theName) override;

private:
	void startSystem(const XmlAttributes& theAttributes);  // 0x47c820 (pizzafrenzy)
	void startStream(const XmlAttributes& theAttributes);  // 0x47c130 (pizzafrenzy)

	Ref<ParticleStreamDef> mStreamDef;  // +0x14: the stream being read
	Ref<ParticleSystemDef> mSystemDef;  // +0x18
};

// 0x41de80 (pizzafrenzy): a file's definition, read through the application's stream, with its pictures found (null
// when the file holds no system)
Ref<ParticleSystemDef> loadParticleSystemDef(const std::string& theFile);

// A particle (0x16c bytes): a container holding one picture; it moves in the screen's coordinates (it is drawn with
// its own position in place of the one its parents give), so a stream that moves leaves its particles where they were
class SignalParticle : public Container
{
public:
	SignalParticle();  // 0x498140 (pizzafrenzy)

	using Node::setPosition;
	void setPosition(float x, float y) override { mPosition.set(x, y); }  // 0x18: 0x4811d0 (pizzafrenzy)
	void applyTransform(Graphics* g) override;                           // 0x64: 0x497e40 (pizzafrenzy)
	void update(UpdateInfo& theInfo) override;                           // 0x94: 0x497f00 (pizzafrenzy)

	SignalParticleStream* mStream;  // +0x128
	Point mVelocity;                // +0x12c (pixels a second)
	float mAcceleration;            // +0x134: along the velocity
	float mStartScale;              // +0x138
	float mEndScale;                // +0x13c
	float mStartOpacity;            // +0x140
	float mEndOpacity;              // +0x144
	Point mForce;                   // +0x148: added to the velocity each second
	bool mBlackHole;                // +0x150: pulled towards the stream's black hole
	float mLife;                    // +0x154 (seconds)
	float mLifeLeft;                // +0x158: the stream removes the particle when it goes below 0
	int mGroupRelative;             // +0x160: placed like any node, in its stream's coordinates
};

class SignalParticleStream : public Container
{
public:
	SignalParticleStream();  // 0x4942e0 (pizzafrenzy)

	std::string getName() override { return "ParticleStream"; }  // 0x60: 0x493aa0 (pizzafrenzy)
	void update(UpdateInfo& theInfo) override;                   // 0x94: 0x494100 (pizzafrenzy)
	void layout() override;                                      // 0x98: 0x473aa0 (pizzafrenzy)

	// 0x4939f0 (pizzafrenzy) is the destructor: the pool's references go with it

	// 0x4944b0 (pizzafrenzy): the stream is made from the definition (0x4790a0 (pizzafrenzy)), and makes its pool
	void setUp(ParticleStreamDef* theDef);
	// 0x493770 (pizzafrenzy): starts emitting, or waits for the start delay when there is one
	void start();
	// 0x4937c0 (pizzafrenzy)
	void stop() { mEmitting = false; }
	// 0x4937d0 (pizzafrenzy): not emitting or waiting to, and no particle left
	bool isDone();
	// 0x493810 (pizzafrenzy): every particle in the air leaves
	void clearParticles();
	// 0x493d90 (pizzafrenzy): sends a free particle off from (x, y) (the stream's position on the screen)
	void emit(float x, float y);

	float mEmitRate;                 // +0x128: particles a second
	int mMaxParticles;               // +0x12c: 0 for no limit
	Point mVelocityX;                // +0x130
	Point mVelocityY;                // +0x138
	Point mAcceleration;             // +0x140
	Point mPositionX;                // +0x148: from the stream's position
	Point mPositionY;                // +0x150
	Point mStartScale;               // +0x158
	Point mEndScale;                 // +0x160
	Point mRotation;                 // +0x168: of the velocity, in degrees
	Point mJitterPeriodX;            // +0x170
	Point mJitterSizeX;              // +0x178
	Point mJitterPeriodY;            // +0x180
	Point mJitterSizeY;              // +0x188
	bool mConstantScale;             // +0x190
	bool mConstantOpacity;           // +0x191
	bool mBlackHole;                 // +0x192
	float mBlackHoleAcceleration;    // +0x194
	Point mBlackHolePosition;        // +0x198: from the stream's position on the screen
	Point mLifeTime;                 // +0x1a0
	Point mStartOpacity;             // +0x1a8
	Point mEndOpacity;               // +0x1b0
	Point mForce;                    // +0x1b8
	std::string mImagePath;          // +0x1c0
	int mSmoothType;                 // +0x1dc
	std::vector<Ref<SignalParticle>> mPool;  // +0x1f0 (counted): a particle is free when this is its only reference
	float mEmitCredit;               // +0x200: the particles owed; starts at 1, so the first update emits one
	bool mEmitting;                  // +0x204
	int mGroupRelative;              // +0x208: offsetType "groupRelative"
	int mSequence;                   // +0x20c (nothing reads it)
	float mAutoStop;                 // +0x210: seconds of emitting (-1 none)
	float mAutoStopLeft;             // +0x214
	float mStartDelay;               // +0x218: seconds before emitting (-1 none)
	float mStartDelayLeft;           // +0x21c
};

class SignalParticleSystem : public Container
{
public:
	SignalParticleSystem();  // 0x479470 (pizzafrenzy)

	std::string getName() override { return "ParticleSystem"; }  // 0x60: 0x479510 (pizzafrenzy)
	void update(UpdateInfo& theInfo) override;                   // 0x94: 0x478f60 (pizzafrenzy)
	void layout() override;                                      // 0x98: 0x473aa0 (pizzafrenzy)

	// 0x479330 (pizzafrenzy): the system's streams are those of the definition (a stream for each)
	void setUp(ParticleSystemDef* theDef);
	// 0x478fe0 (pizzafrenzy): the streams start (those in the air first leave when it was not emitting), the time to
	// the automatic stop, and the warm-up seconds run at once
	void start();
	// 0x478ef0 (pizzafrenzy): every stream stops emitting; its particles run their course
	void stop();
	// 0x478f20 (pizzafrenzy): every stream is done
	bool isDone();

private:
	// 0x4790a0 (pizzafrenzy)
	void addStream(ParticleStreamDef* theDef);

	bool mEmitting;         // +0x128
	bool mRemoveWhenDone;   // +0x129: the system asks to be removed when it is done (nothing sets it)
	float mAutoStop;        // +0x12c: seconds of emitting (-1 none)
	float mAutoStopLeft;    // +0x130
	float mWarmup;          // +0x134: seconds of play at once when it starts
};

} // namespace engine

#endif
