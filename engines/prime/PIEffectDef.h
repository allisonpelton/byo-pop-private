// The definition of a particle effect (Sexy::PIEffect's loaded .ppf file): textures, emitters and layers. A definition
// holds no running state; PIEffect keeps that. Every field has the offset it has in Bejeweled 3.exe, and the fields
// that only the file's old format, the editor or LoadState/SaveState reach are not stored (PIEffect.h lists them).
#pragma once

#include "Color.h"
#include "PIValue.h"

#include <string>
#include <vector>

namespace Sexy {

// 0x48 bytes (ctor 0x56df50 (bejeweled3)): a texture the particles draw: the image's name and how its frames lie in it
struct PITexture {
	std::string mName;           // +0x04
	int mFrameCount;             // +0x40: the frames in the image
	int mColumnCount;            // +0x3c: the frames across (the file's second count)
	bool mHasMargin;             // +0x44: the image's frames have a 1-pixel margin on each side
	std::string mImageId;        // +0x24: the resource id the image is found by
	PITexture() : mFrameCount(1), mColumnCount(1), mHasMargin(false) {}
};

// 0xac0 bytes (ctor 0x5699d0 (bejeweled3)): one kind of particle an emitter makes
struct PIParticleDef {
	std::string mName;                   // +0x08
	int mTextureIndex;                   // +0x20: which of the effect's textures

	PIValue mLife;                       // +0x024: seconds a particle lives (and, with mLifeVariation, how it varies)
	PIValue mNumber;                     // +0x080: how many to make
	PIValue mSizeX;                      // +0x0dc
	PIValue mVelocity;                   // +0x138
	PIValue mWeight;                     // +0x194
	PIValue mSpin;                       // +0x1f0
	PIValue mMotionRandomness;           // +0x24c
	PIValue mBounce;                     // +0x2a8
	PIValue mLifeVariation;              // +0x304
	PIValue mNumberVariation;            // +0x360
	PIValue mSizeXVariation;             // +0x3bc
	PIValue mVelocityVariation;          // +0x418
	PIValue mWeightVariation;            // +0x474
	PIValue mSpinVariation;              // +0x4d0
	PIValue mMotionRandomnessVariation;  // +0x52c
	PIValue mBounceVariation;            // +0x588
	PIValue mSizeXOverLife;              // +0x5e4
	PIValue mVelocityOverLife;           // +0x640
	PIValue mWeightOverLife;             // +0x69c
	PIValue mSpinOverLife;               // +0x6f8
	PIValue mMotionRandomnessOverLife;   // +0x754
	PIValue mBounceOverLife;             // +0x7b0
	PIValue mVisibility;                 // +0x80c: the alpha over life
	PIValue mAngle;                      // +0x868
	PIValue mAngleVariation;             // +0x8c4
	PIValue mSizeY;                      // +0x920
	PIValue mSizeYVariation;             // +0x97c
	PIValue mSizeYOverLife;              // +0x9d8

	double mOriginX;                     // +0xa38: where in the texture the particle turns, as a fraction
	double mOriginY;                     // +0xa40
	bool mSizeYFollowsX;                 // +0xa48
	bool mDrawInSecondPass;              // +0xa49
	bool mSingleParticle;                // +0xa4a: one particle kept alive instead of a stream (size from the emitter)
	bool mSecondPassOnly;                // +0xa4b
	bool mAttachToEmitter;               // +0xa4c: the particle's position and angle follow the emitter
	int mFrameOffset;                    // +0xa50: -1 for a random frame, else added to the particle's frame
	bool mRandomStartFrame;              // +0xa54
	float mAttachRatio;                  // +0xa58: how much of the emitter's motion an attached particle takes
	bool mFlipX;                         // +0xa5c
	bool mFlipY;                         // +0xa5d
	int mColorCycles;                    // +0xa60: the colour curve repeats this many times plus one over a life
	int mAlphaCycles;                    // +0xa64: the same for the alpha curve
	bool mColorFromLifeRatio;            // +0xa68
	bool mColorByIndex;                  // +0xa69
	bool mUseInstanceColor;              // +0xa6a
	bool mInheritParentTint;                       // +0xa6b
	bool mUseInstanceAlpha;              // +0xa6c
	bool mTintWithLayerColor;            // +0xa6d
	int mColorIndexDivisor;              // +0xa70
	bool mColorStepped;                  // +0xa75
	bool mAngleFromEmitter;              // +0xa76
	bool mAlignToMotion;                 // +0xa77
	bool mAlignToEmitterAngle;           // +0xa78
	bool mRandomStartAngle;              // +0xa79
	int mAlignOffsetDegrees;             // +0xa7c
	int mStartAngleDegrees;              // +0xa80
	int mRandomAngleRange;               // +0xa84
	int mRandomAngleBase;                // +0xa88

	struct ColorPoint {   // 8 bytes
		unsigned int mColor;   // +0x00: 0xAARRGGBB, alpha always 0xFF
		float mTime;           // +0x04
	};
	struct AlphaPoint {   // 8 bytes
		int mAlpha;            // +0x00
		float mTime;           // +0x04
	};
	std::vector<ColorPoint> mColorPoints;   // +0xa8c
	std::vector<AlphaPoint> mAlphaPoints;   // +0xaa4

	PIParticleDef() : mTextureIndex(0), mOriginX(0.0), mOriginY(0.0), mSizeYFollowsX(false), mDrawInSecondPass(false),
		mSingleParticle(false), mSecondPassOnly(false), mAttachToEmitter(false), mFrameOffset(0),
		mRandomStartFrame(false), mAttachRatio(0.0f), mFlipX(false), mFlipY(false), mColorCycles(0), mAlphaCycles(0),
		mColorFromLifeRatio(false), mColorByIndex(false), mUseInstanceColor(false), mInheritParentTint(false),
		mUseInstanceAlpha(false), mTintWithLayerColor(false), mColorIndexDivisor(0), mColorStepped(false),
		mAngleFromEmitter(false), mAlignToMotion(false), mAlignToEmitterAngle(false), mRandomStartAngle(false),
		mAlignOffsetDegrees(0), mStartAngleDegrees(0), mRandomAngleRange(0), mRandomAngleBase(0) {}
};

// 0xf50 bytes (ctor 0x56dfb0 (bejeweled3)): a particle emitter, which makes the particles of its definitions
struct PIEmitter {
	std::string mName;                   // +0x004
	PIValue mLife;                       // +0x01c
	PIValue mNumber;                     // +0x078
	PIValue mVelocity;                   // +0x0d4
	PIValue mWeight;                     // +0x130
	PIValue mSpin;                       // +0x18c
	PIValue mMotionRandomness;           // +0x1e8
	PIValue mBounce;                     // +0x244
	PIValue mValueA;                     // +0x2a0: the one value of the 42 that nothing in the update or draw reads
	PIValue mLifeScale;                  // +0x2fc
	PIValue mNumberScale;                // +0x358
	PIValue mSizeXScale;                 // +0x3b4
	PIValue mSizeYScale;                 // +0x410
	PIValue mVelocityScale;              // +0x46c
	PIValue mWeightScale;                // +0x4c8
	PIValue mSpinScale;                  // +0x524
	PIValue mMotionRandomnessScale;      // +0x580
	PIValue mBounceScale;                // +0x5dc
	PIValue mScale;                      // +0x638
	PIValue mAlphaFactor;                // +0x694
	PIValue mValueB;                     // +0x6f0
	PIValue mTintFactor;                 // +0x74c
	PIValue mAngle;                      // +0x7a8
	PIValue mAngleVariation;             // +0x804
	PIValue mLifeVariation;              // +0x860
	PIValue mValueC;                     // +0x8bc
	PIValue mSizeXVariation;             // +0x918
	PIValue mSizeYVariation;             // +0x974
	PIValue mVelocityVariation;          // +0x9d0
	PIValue mWeightVariation;            // +0xa2c
	PIValue mSpinVariation;              // +0xa88
	PIValue mMotionRandomnessVariation;  // +0xae4
	PIValue mBounceVariation;            // +0xb40
	PIValue mValueD;                     // +0xb9c
	PIValue mNumberOverLife;             // +0xbf8
	PIValue mSizeOverLife;               // +0xc54
	PIValue mValueE;                     // +0xcb0
	PIValue mVelocityOverLife;             // +0xd0c
	PIValue mWeightOverLife;             // +0xd68
	PIValue mSpinOverLife;               // +0xdc4
	PIValue mMotionRandomnessOverLife;   // +0xe20
	PIValue mBounceOverLife;             // +0xe7c
	PIValue mScaleOverLife;              // +0xed8
	std::vector<PIParticleDef> mParticleDefs;   // +0xf34
	bool mIsSuperEmitter;                       // +0xf4c
	bool mOldestInFront;                   // +0xf4d: its particle definitions are added in front of the list
	bool mHasLife;                       // +0xf4e: the life value has keyframes
	PIEmitter() : mIsSuperEmitter(false), mOldestInFront(false), mHasLife(false) {}
};

// 0x7a8 bytes (ctor 0x577250 (bejeweled3)): one use of an emitter in a layer
struct PIEmitterInstanceDef {
	std::string mName;                   // +0x004
	int mStartOffset;                    // +0x01c
	int mEmitterIndex;                   // +0x020: which of the effect's emitters
	int mShape;                          // +0x024: 1 line, 2 ellipse, 3 grid, 4 circle
	bool mFlagA;                         // +0x028
	bool mFlagB;                         // +0x029
	int mShapeCount;                     // +0x02c: the points a line, circle or grid has (0 for the whole shape)
	int mShapeRows;                      // +0x030
	bool mHasFreeEmitters;               // +0x034
	std::vector<int> mFreeEmitterIndices;   // +0x038: emitters spawned by this one's particles
	bool mFlagC;                         // +0x050
	PIValue2D mPosition;                 // +0x058
	Color mColor;                        // +0x080: 0 to 255 for each of red, green, blue; alpha 255
	std::string mMaskImageName;          // +0x090 (the image itself is a SharedImageRef in the original)
	PIValue mLifeScale;                  // +0x0b8
	PIValue mNumberScale;                // +0x114
	PIValue mSizeXScale;                 // +0x170
	PIValue mVelocityScale;              // +0x1cc
	PIValue mWeightScale;                // +0x228
	PIValue mSpinScale;                  // +0x284
	PIValue mMotionRandomnessScale;      // +0x2e0
	PIValue mBounceScale;                // +0x33c
	PIValue mScale;                      // +0x398
	PIValue mTint;                       // +0x3f4
	PIValue mAlpha;                      // +0x450
	PIValue mAngle;                      // +0x4ac
	PIValue mAngleVariation;             // +0x508
	PIValue mValueG;                     // +0x564
	PIValue mRotation;                   // +0x5c0
	PIValue mShapeWidth;                 // +0x61c
	PIValue mShapeHeight;                // +0x678
	PIValue mSizeYScale;                 // +0x6d4
	PIValue mValueH;                     // +0x730
	std::vector<PIValue2D> mPoints;      // +0x798: the points a line (or the polygon of a shape) goes through
	PIEmitterInstanceDef() : mStartOffset(0), mEmitterIndex(0), mShape(0), mFlagA(false), mFlagB(false), mShapeCount(0),
		mShapeRows(0), mHasFreeEmitters(false), mFlagC(false) {}
};

// 0x178 bytes: a force field that bends the particles passing through its polygon
struct PIDeflector {
	std::string mName;                   // +0x04
	float mBounce;                       // +0x1c
	float mHits;                         // +0x20
	float mAllowedDirection;             // +0x24
	bool mFlagA;                         // +0x28
	PIValue2D mPosition;                 // +0x30
	PIValue mActive;                     // +0x90
	PIValue mAngle;                      // +0xec
	std::vector<PIValue2D> mPoints;      // +0x154
	PIDeflector() : mBounce(0.0f), mHits(0.0f), mAllowedDirection(0.0f), mFlagA(false) {}
};

// 0x2f8 bytes: a polygon particles cannot pass
struct PIBlocker {
	std::string mName;                   // +0x04
	PIValue2D mPosition;                 // +0x20
	PIValue mActive;                     // +0x80
	PIValue mScale;                      // +0xdc
	std::vector<PIValue2D> mPoints;      // the polygon's corners
};

// A force that pushes the particles of its layer
struct PIForce {
	std::string mName;
	bool mFlagA;
	PIValue2D mPosition;
	PIValue mCurves[7];
	PIForce() : mFlagA(false) {}
};

// 0x150 bytes: a layer of an effect, with its emitters, deflectors, blockers and forces
struct PILayerDef {
	std::string mName;                   // +0x04
	std::vector<PIEmitterInstanceDef> mEmitterInstances;   // +0x1c
	std::vector<PIDeflector> mDeflectors;                  // +0x34
	std::vector<PIForce> mForces;                          // +0x4c
	std::vector<PIBlocker> mBlockers;                      // +0x64
	PIValue2D mPosition;                 // +0x80
	double mOriginX;                     // +0xe0: the position at time 0
	double mOriginY;                     // +0xe8
	PIValue mAngle;                      // +0xf0
	PILayerDef() : mOriginX(0.0), mOriginY(0.0) {}
};

// 0x6c bytes (ctor 0x5657b0 (bejeweled3)): what a .ppf file holds
struct PIEffectDef {
	std::vector<PIEmitter> mEmitters;    // +0x04
	std::vector<PITexture> mTextures;    // +0x1c
	std::vector<PILayerDef> mLayers;     // +0x34
};

}
