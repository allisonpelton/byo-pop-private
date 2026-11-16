#ifndef __SPROUT2_PARTICLESYSTEMDOCHANDLER_H__
#define __SPROUT2_PARTICLESYSTEMDOCHANDLER_H__

// A particle effect's file, read into its definition: a <particleSystem> of <particleStream>s, each stream with its
// images and the ranges its particles' speeds, sizes, opacities and lives are drawn from.

#include "DocHandler.h"
#include "Geometry.h"
#include "KeyInterpolator.h"
#include "Object.h"
#include "Surface.h"

#include <string>
#include <vector>

namespace engine
{

// How a stream's particles take their images (ParticleStreamDef +0x1c)
enum ParticleSequence
{
	PARTICLE_SEQUENCE_RANDOM = 0,      // the images handed round the pool in turn, the particles taken at random
	PARTICLE_SEQUENCE_SEQUENTIAL = 1,  // "sequential": read, but nothing treats it apart from the first
	PARTICLE_SEQUENCE_ANIMATION = 2    // "anim": each particle an animation (AnimImage) of the path
};

// Where a stream's particles live (ParticleStreamDef +0x11c)
enum ParticleOffsetType
{
	PARTICLE_OFFSET_INDEPENDENT = 0,     // emitted where the stream is in the scene and left there as it moves
	PARTICLE_OFFSET_GROUP_RELATIVE = 1   // "groupRelative": placed in the stream's space, moving with it
};

// One stream's definition (ParticleStreamDef, 0x150 bytes, its constructor 0x476870 (feedingfrenzy2))
class ParticleStreamDef : public Object
{
public:
	ParticleStreamDef();

	// 0x461410 (feedingfrenzy2): the images path0.png (or path0.jpg), path1, ... up to the first missing number,
	// each put at the pivot and given the blend mode; true when it found one
	bool loadImages();

	// +0x0c, +0x10: the "offset" pair is read here and nothing reads it, so it is left untranslated
	float mEmitRate;                    // +0x14: particles a second
	int mMaxParticles;                  // +0x18: at most this many at once, 0 for no limit
	int mSequence;                      // +0x1c: ParticleSequence
	float mAnimationRate;               // +0x20: the frames a second of an animation's particles
	FloatRange mVelocityX;              // +0x24
	FloatRange mVelocityY;              // +0x2c
	FloatRange mAcceleration;           // +0x34: along the velocity
	FloatRange mPositionX;              // +0x3c: from the stream's position
	FloatRange mPositionY;              // +0x44
	FloatRange mStartScale;             // +0x4c
	FloatRange mEndScale;               // +0x54
	FloatRange mRotation;               // +0x5c: the velocity turned, by "degrees" the original takes as radians
	FloatRange mJitterPeriodX;          // +0x64
	FloatRange mJitterAmplitudeX;       // +0x6c
	FloatRange mJitterPeriodY;          // +0x74
	FloatRange mJitterAmplitudeY;       // +0x7c
	bool mConstantScale;                // +0x84: the end scale the start one
	bool mConstantOpacity;              // +0x85: the end opacity the start one
	bool mBlackHole;                    // +0x86: the particles pulled towards mBlackHolePosition
	float mBlackHoleAcceleration;       // +0x88
	Vector2f mBlackHolePosition;        // +0x8c: in the stream's space
	FloatRange mLifeTime;               // +0x94: seconds
	RectI mBounds;                      // +0x9c: a particle outside it (from the stream's position) dies, when not empty
	FloatRange mStartOpacity;           // +0xac
	FloatRange mEndOpacity;             // +0xb4
	Vector2f mExternalForce;            // +0xbc: added to the velocity each second
	std::string mPath;                  // +0xcc: the images' path without the number and extension
	std::string mParentName;            // +0xe8: the named component independent particles are placed relative to
	int mFilterMode;                    // +0x104: the particles' (FilterMode)
	// +0x108 to +0x118 are left unset by the original's constructor (the port gives them <resource>'s defaults);
	// every file's <resource> sets them
	int mBlendMode;                     // +0x108: the images' (BlendMode)
	int mPivot;                         // +0x10c: the images' (Pivot)
	bool mAbsolutePivot;                // +0x110: mPivotPoint, rather than mPivot, is the images' anchor
	Vector2f mPivotPoint;               // +0x114
	int mOffsetType;                    // +0x11c: ParticleOffsetType
	float mAutoStop;                    // +0x120: seconds of emitting, when above 0
	float mStartDelay;                  // +0x124: seconds before emitting, when above 0
	std::vector<FloatKey> mOpacityKeys; // +0x128: the opacity over a particle's life, multiplying the range's
	std::vector<Ref<Surface>> mImages;  // +0x138
};

// A particle effect's definition (ParticleSystemDef, 0x2c bytes, its constructor 0x483bf0 (feedingfrenzy2))
class ParticleSystemDef : public Object
{
public:
	float mAutoStop = -1.0f;                         // +0x0c: seconds before every stream stops, when above 0
	float mWarmup = 0.0f;                            // +0x10: seconds run at once when the effect starts
	std::vector<Ref<ParticleStreamDef>> mStreams;    // +0x14
};

// The reader of a particle effect's file (ParticleSystemDocHandler, through BaseDocHandler, which passes the
// elements it knows to the slots of its own table; constructor 0x47dde0, destructor 0x47bf40 (feedingfrenzy2))
class ParticleSystemDocHandler : public AbstractDocHandler
{
public:
	// 0x00 (0x450dc0 (feedingfrenzy2))
	void startElement(const std::string& theName, Params& theAttributes) override;
	// 0x04 (0x4612d0 (feedingfrenzy2)): the end of a stream
	void endElement(const std::string& theName) override;

	ParticleSystemDef* getSystem() { return mSystem.get(); }

private:
	// 0x04 of its own table (0x45d200 (feedingfrenzy2)): <particleStream emitRate maxParticles sequence animRate
	// offsetType parent autoStop startDelay offset>
	void readStream(Params& theAttributes);
	// 0x08 (0x426200 (feedingfrenzy2)): <particleSystem autoStop warmup>
	void readSystem(Params& theAttributes);
	// 0x0c (0x42fe40 (feedingfrenzy2)): <opacityKey alpha value>
	void readOpacityKey(Params& theAttributes);
	// Part of 0x450dc0 (feedingfrenzy2): <resource path filter alphaType pivot abspivot>
	void readResource(Params& theAttributes);

	Ref<ParticleStreamDef> mStream;   // +0x30: the stream being read
	Ref<ParticleSystemDef> mSystem;   // +0x34
};

} // namespace engine

#endif
