#ifndef __PIECE_H__
#define __PIECE_H__

// Piece.h: the things a board holds or throws around: Piece (a gem, 0xe0 bytes), Effect (an explosion frame, 0x14 bytes)
// and Shard (a particle, 0x70 bytes). Offsets in the comments are the original's. Fields whose use is not understood
// yet are named after their offset (mXX).

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
	bool			m2C;            // +0x2c
	float			m30;            // +0x30 the lightning's progress through the gem (0 to 1)
	float			mGlow[9];       // +0x34 the eight light levels around the gem (AddPointLight adds to them) and, at +0x54, the ninth that DrawBoard fills
	int				m58;            // +0x58 the counter of a special gem (saved)
	int				m5C;            // +0x5c
	int				m60;            // +0x60 -1
	int				m64;            // +0x64
	int				m68;            // +0x68
	int				m6C;            // +0x6c
	bool			m70;            // +0x70 a flag that is saved with the gem (flame gem)
	int				m74;            // +0x74
	int				m78;            // +0x78
	int				m7C;            // +0x7c
	int				m80;            // +0x80
	int				mSwapDX;        // +0x84 where the gem is going during a swap (-1, 0, 1)
	int				mSwapDY;        // +0x88
	bool			m8C;            // +0x8c
	double			m90;            // +0x90
	float			m98;            // +0x98
	bool			m9C;            // +0x9c
	int				mA0;            // +0xa0
	int				mA4;            // +0xa4 the hint shows on this gem
	int				mHintTime;      // +0xa8
	int				mHintFrame;     // +0xac
	int				mB0;            // +0xb0
	int				mB4;            // +0xb4
	int				mB8;            // +0xb8
	int				mBC;            // +0xbc
	float			mC0;            // +0xc0
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

	Lightning() { for (int i = 0; i < 8; i++) for (int j = 0; j < 4; j++) mPoint[i][j] = 0; mProgress = 0; mSwingX = 0; mSwingY = 0; }
};

}

#endif
