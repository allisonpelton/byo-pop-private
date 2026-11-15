#ifndef __PIECE_H__
#define __PIECE_H__

// Piece.h: the things a board holds or throws around: Piece (a gem, 0xe0 bytes), Effect (an explosion frame, 0x14 bytes)
// and Shard (a particle, 0x70 bytes). Offsets in the comments are the original's.

#include "Color.h"

namespace Sexy
{

// a gem. mType: 0 to 6 are the seven colours; the others are special gems (see Board.h)
struct Piece
{
	int				mType;          // +0x00
	float			mX;             // +0x04 position in the 640x480 board space, not scaled
	float			mY;             // +0x08
	float			mZ;             // +0x0c
	int				mCol;           // +0x10 column on the board
	int				mRow;           // +0x14 row on the board
	int				mVX;            // +0x18 (an int in the original: the game over fall draws a random integer here)
	float			mVY;            // +0x1c falling speed
	float			mVZ;            // +0x20
	float			mRot;           // +0x24
	float			mVRot;          // +0x28
	bool			mLightningHit;  // +0x2c the lightning of a hyper cube has reached the gem
	float			mLightningProgress; // +0x30 the lightning's progress through the gem (0 to 1)
	float			mGlow[9];       // +0x34 the eight light levels around the gem (AddPointLight adds to them) and, at +0x54, the ninth that DrawBoard fills
	int				mCountdown;     // +0x58 the counter of a special gem (saved)
	int				mMoveTick;      // +0x5c the update count at which the gem was made or last swapped (the newest gem of a set gets the power gem)
	int				mHypercubeTick; // +0x60 the update count at which the gem became a hyper cube (-1 for never)
	int				mTypeBeforeHypercube; // +0x64 the gem's colour before it became a hyper cube: FindSets counts it as that colour in the same update
	int				mJitterX;       // +0x68 how far the bomb gem shakes
	int				mJitterY;       // +0x6c
	bool			mFlame;         // +0x70 a flag that is saved with the gem (flame gem)
	int				mFlameTick;     // +0x74 the update count at which the gem became a flame gem (it does not explode in that update)
	int				mExplodeDelay;  // +0x78 updates until the flame or bomb gem explodes (a chain, 10 each)
	int				mSwapOffsetX;   // +0x7c where the gem is from the middle of the swap
	int				mSwapOffsetY;   // +0x80
	int				mSwapDX;        // +0x84 where the gem is going during a swap (-1, 0, 1)
	int				mSwapDY;        // +0x88
	bool			mGemHovered;    // +0x8c the mouse is over the gem: its glow rises
	double			mGemHoverGlow;  // +0x90 how bright the glow is, 0 to 1
	float			mGemHoverFrame; // +0x98 the frame of the glow's animation, 0 to 10
	bool			mInSet;         // +0x9c the gem is in a set that is being cleared (it shrinks)
	int				mShrinkStep;    // +0xa0 how far the gem has shrunk, 0 to 6
	int				mGemFrame;      // +0xa4 the frame of the gem's picture that the hint and the selected gem use (0: the plain gem)
	int				mHintTime;      // +0xa8
	int				mHintFrame;     // +0xac
	int				mHintMoveX;     // +0xb0 where the hinted gem goes: the columns and rows to move (a gem moving to an empty square)
	int				mHintMoveY;     // +0xb4
	int				mHyperRole;     // +0xb8 1: the hyper cube of a swap, 2: the gem it was swapped with
	int				mHyperDelay;    // +0xbc updates before the hyper cube fades in
	float			mHyperFade;     // +0xc0 0 to 1: how far the hyper cube has appeared
	unsigned char	mRest[0x1c];    // +0xc4..+0xe0 (not touched by the constructor)

	Piece();        // 0x5947a7
};

// 0x594721 (the size is 0x14: the last int is not set by the constructor)
struct Effect
{
	int				mKind;          // +0x00 0: an explosion (drawn from IMAGE_EXPLOSION)
	int				mX;             // +0x04
	int				mY;             // +0x08
	int				mFrame;         // +0x0c
	int				mDelay;         // +0x10 updates to wait before the animation starts

	// 0x594721 (the original's constructor does not set mDelay; the port zeroes it)
	Effect() { mKind = 0; mX = 0; mY = 0; mFrame = 0; mDelay = 0; }
};

// a particle (0x7c bytes, constructor 0x594731). mKind: 0 an image shard of a gem, 1 a white sparkle, 2 a spark
// (drawn from IMAGE_SPARKLE with the frame), 3 a swirl that orbits a point, 5 a plain falling spark.
struct Shard
{
	int				mKind;          // +0x00
	Color			mColor;         // +0x04
	float			mX;             // +0x14
	float			mY;             // +0x18
	float			mY2;            // +0x1c (the kind 3 swirl moves it with mY)
	int				mFrame;         // +0x20 the animation frame, 0 to 0x27
	int				mFrameDelay;    // +0x24 updates per frame
	float			mVX;            // +0x28
	float			mVY;            // +0x2c
	float			mWeight;        // +0x30 how strongly gravity pulls (1.0)
	float			mTrailChance;   // +0x34 chance per update of dropping a spark
	float			mTrailStep;     // +0x38 added to the chance every update
	float			mPosX;          // +0x3c the swirl's drawn position
	float			mPosY;          // +0x40
	float			mCenterX;       // +0x44 the point the swirl orbits
	float			mCenterY;       // +0x48
	float			mAngle;         // +0x4c
	bool			mCycleColor;    // +0x50 the colour runs through the hues
	int				mHue;           // +0x54
	float			mBaseX;         // +0x58 the swirl's x before the sideways swing
	int				mAge;           // +0x5c
	int				mImageOffset;   // +0x60
	float			mGravity;       // +0x64 added to mVY every update
	bool			mFading;        // +0x68
	float			mAlpha;         // +0x6c
	float			mFrequency;     // +0x70 the swirl's swing
	float			mAmplitude;     // +0x74
	float			mLimit;         // +0x78

	Shard();        // 0x594731
};

// the ripple of an exploding gem (0x20 bytes, built on the stack by ExplodeAtHelper and SmallExplodeAt)
struct ShockWave
{
	float			mX;             // +0x00
	float			mY;             // +0x04
	float			mRadius;        // +0x08 45
	float			mWidth;         // +0x0c 20
	float			mProgress;      // +0x10 0 to 1
	bool			mGrowing;       // +0x14 (never set by the original: its stack slot decides)
	float			mGrowStep;      // +0x18 0.1
	float			mFadeStep;      // +0x1c 0.05
};

// a flash of lightning between two gems (0x8c bytes, constructor 0x5989ae)
struct Lightning
{
	float			mPoint[8][4];   // +0x00 eight points along the line, each stored twice (x, y, x, y)
	float			mProgress;      // +0x80 0 to 1: how far the flash has faded
	float			mSwingX;        // +0x84 how far the line swings sideways
	float			mSwingY;        // +0x88

	// 0x5989ae
	Lightning() { for (int i = 0; i < 8; i++) for (int j = 0; j < 4; j++) mPoint[i][j] = 0; mProgress = 0; mSwingX = 0; mSwingY = 0; }
};

}

#endif
