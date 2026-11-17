// What a running Sexy::PIEffect keeps (PIEffect.h): a layer for each layer of the definition, in it an emitter instance
// for each use of an emitter, and the particles those make, in lists. The exe's names for these classes are PILayer,
// PIEmitterInstance, PIParticleInstance and PIFreeEmitterInstance (the symbols of the Mac build); the list and the
// per-particle-definition counters have none, and are named by what they do. The offsets are the exe's (its objects
// come from pools, here they are new and delete; the exe's list class and the debug iterators of its containers are
// left out, a vector there starts 0xc bytes into its 0x18 bytes).
#pragma once

#include "Color.h"
#include "PIEffectDef.h"
#include "SexyMatrix.h"

#include <vector>

namespace Sexy {

struct PIParticleDef;
struct PIEmitter;
struct PIFreeEmitterInstance;
class Image;

// 0x10 bytes: what an emitter instance (or a free emitter) keeps for one of the particle definitions of its emitter
struct PIParticleDefInstance {
	float mSpawnAccumulator;     // +0x00: how much of a particle is owed; one is made for each 1.0
	float mNumberVariation;      // +0x04: a random change of the number made, redrawn every 25th update
	int mParticleCount;          // +0x08: how many particles it has made (the next one's index)
	int mUpdateCount;            // +0x0c: how many updates it has had
	PIParticleDefInstance() : mSpawnAccumulator(0.0f), mNumberVariation(0.0f), mParticleCount(0), mUpdateCount(0) {}
};

// 0xf0 bytes (ctor 0x565540 (bejeweled3)): one particle
struct PIParticleInstance {
	PIParticleInstance* mPrev;           // +0x00
	PIParticleInstance* mNext;           // +0x04
	PIParticleDef* mParticleDef;         // +0x08
	PIEmitter* mEmitter;                 // +0x0c
	int mIndex;                          // +0x10: the number of the particle among those its definition made
	PIFreeEmitterInstance* mFreeEmitter; // +0x14: the free emitter that made it, null for the emitter instance's own
	double mPositionX;                   // +0x18: how far it has moved since it was made
	double mPositionY;                   // +0x20
	double mSpawnOffsetX;                // +0x28: where it started, from the emitter
	double mSpawnOffsetY;                // +0x30
	double mOriginX;                     // +0x38: where the emitter was when it was made
	double mOriginY;                     // +0x40
	double mAttachedX;                   // +0x48: the translation CalcParticleTransform used last
	double mAttachedY;                   // +0x50
	double mVelocityX;                   // +0x58
	double mVelocityY;                   // +0x60
	float mAngle;                        // +0x68: radians
	float mLifeVariation;                // +0x6c
	float mSizeXVariation;               // +0x70
	float mSizeYVariation;               // +0x74
	float mVelocityVariation;            // +0x78
	float mWeightVariation;              // +0x7c
	float mSpinVariation;                // +0x80
	float mMotionRandomnessVariation;    // +0x84
	float mBounceVariation;              // +0x88
	float mFreeEmitterNumberVariation;   // +0x8c: the last of the nine variations (a free emitter's, nothing else reads it)
	float mScale;                        // +0x90
	float mSizeX;                        // +0x94
	float mSizeY;                        // +0x98
	float mColorOffset;                  // +0x9c: a random 0 to 1 that picks the color along the curve
	float mEmitterRotation;              // +0xa0: the layer's rotation when it was made (attached particles)
	int mRandomFrame;                    // +0xa4: a random 0 to 0x7fff, for the frame
	SexyMatrix3 mTransform;              // +0xa8: where it is drawn (CalcParticleTransform)
	float mDrawArea;                     // +0xcc: the area scale of the matrix
	int mFrame;                          // +0xd0: which frame of the texture
	float mRandomValue;                  // +0xd4: a random -1 to 1 (the deflectors' bounce)
	float mAge;                          // +0xd8: seconds
	float mLife;                         // +0xdc: seconds
	float mLifeRatio;                    // +0xe0: age over life
	bool mDrawn;                         // +0xe4: it has been drawn once
	unsigned int mInstanceColor;         // +0xe8: the color the layer's picture gives it, 0xffffffff before
	PIParticleInstance();
	virtual ~PIParticleInstance() {}     // port: the exe's classes have none; this lets a free emitter be deleted as one
};

// the exe's list of particles (head, tail, count, and two flags): a doubly linked list through mPrev and mNext
struct PIParticleGroup {
	PIParticleInstance* mHead;           // +0x00
	PIParticleInstance* mTail;           // +0x04
	int mCount;                          // +0x08
	bool mIsFreeEmitterList;             // +0x0c: the particles are PIFreeEmitterInstance (a group of an emitter instance)
	bool mIsFreeEmitterParticles;        // +0x0d: the particles are those of a free emitter
	PIParticleGroup() : mHead(NULL), mTail(NULL), mCount(0), mIsFreeEmitterList(false), mIsFreeEmitterParticles(false) {}
	void Append(PIParticleInstance* theParticle);    // the exe's code in UpdateParticleDef
	void Prepend(PIParticleInstance* theParticle);
	void Unlink(PIParticleInstance* theParticle);    // UpdateParticleGroup's
	void DeleteAll();                                // ResetAnim's loops
};

// 0x118 bytes (ctor 0x5655c0 (bejeweled3)): an emitter that a particle of a super emitter starts: it is a particle
// itself, and makes particles of its own emitter
struct PIFreeEmitterInstance : public PIParticleInstance {
	std::vector<PIParticleDefInstance> mDefInstances;   // +0xf0 (the vector's data at +0xfc)
	PIParticleGroup mParticles;                         // +0x108
	PIFreeEmitterInstance();
	~PIFreeEmitterInstance() override { mParticles.DeleteAll(); }
};

// 0xd8 bytes (ctor 0x565630 (bejeweled3)): one use of an emitter in a layer. The exe also has, at +0x00 and +0x18,
// a vector and a list that nothing reads
struct PIEmitterInstance {
	PIEmitterInstanceDef* mDef;          // +0x28: the definition's use of the emitter
	bool mWasInRange;                    // +0x2c: its time was inside the span it runs in at the last update
	bool mCanEmit;                       // +0x2d: it may make particles (set by Update)
	std::vector<PIParticleDefInstance> mFreeDefInstances;   // +0x30 (data +0x3c): for a super emitter, one for each free emitter
	PIParticleGroup mFreeEmitters;       // +0x48: the free emitters it has made
	std::vector<PIParticleDefInstance> mDefInstances;       // +0x58 (data +0x64): one for each particle definition
	PIParticleGroup mParticles;          // +0x70: the particles it has made
	Color mColor;                        // +0x80
	SharedImageRef mMaskImage;           // +0x90
	SexyMatrix3 mTransform;              // +0x9c
	double mOffsetX;                     // +0xc0
	double mOffsetY;                     // +0xc8
	float mNumberScale;                  // +0xd0
	bool mVisible;                       // +0xd4
	explicit PIEmitterInstance(PIEmitterInstanceDef* theDef);
	~PIEmitterInstance();
	PIEmitterInstance(const PIEmitterInstance&) = delete;
	PIEmitterInstance& operator=(const PIEmitterInstance&) = delete;
	void DeleteParticles();
};

// 0x60 bytes (ctor 0x565710 (bejeweled3))
struct PILayer {
	PILayerDef* mDef;                    // +0x00: the definition's layer
	std::vector<PIEmitterInstance*> mEmitterInstances;   // +0x04 (data +0x10); pointers here so that a growing vector cannot move them
	bool mVisible;                       // +0x1c
	Color mColor;                        // +0x20
	Image* mTintImage;                   // +0x30: when set, particles take their color from it (nothing in the game sets it)
	int mTintOffsetX;                    // +0x34
	int mTintOffsetY;                    // +0x38
	explicit PILayer(PILayerDef* theDef);
	~PILayer();
	PILayer(const PILayer&) = delete;
	PILayer& operator=(const PILayer&) = delete;
};

}
