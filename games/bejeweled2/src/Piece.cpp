// Piece.cpp: the constructors of the gem and the particle (Sexy::Piece 0x5947a7, Sexy::Shard 0x594731).

#include "Piece.h"
#include <string.h>

using namespace Sexy;

// 0x5947a7: the original leaves some fields uninitialised (the glow, the swap offsets, the last words); the port zeroes
// them all, since the gems are saved raw
Piece::Piece()
{
	memset(this, 0, sizeof(*this));
	mHypercubeTick = -1;
}

// 0x594731
Shard::Shard()
{
	mKind = 0;
	mColor = Color(0xff, 0xff, 0xff);
	mX = 0;
	mY = 0;
	mY2 = 0;
	mFrame = 0;
	mFrameDelay = 1;
	mVX = 0;
	mVY = 0;
	mWeight = 1.0f;
	mTrailChance = 0;
	mTrailStep = 0;
	mPosX = 0;
	mPosY = 0;
	mCenterX = 0;
	mCenterY = 0;
	mAngle = 0;
	mCycleColor = false;
	mHue = 0;
	mBaseX = 0;
	mAge = 0;
	mImageOffset = 0;
	mGravity = 0;
	mFading = false;
	mAlpha = 0;
	mFrequency = 0;
	mAmplitude = 0;
	mLimit = 0;
}

static_assert(sizeof(Piece) == 0xe0, "the gem is saved raw: its size must stay 0xe0");
static_assert(sizeof(Shard) == 0x7c, "Shard is 0x7c bytes");
