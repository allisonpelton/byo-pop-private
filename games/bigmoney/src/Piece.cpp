#include "Piece.h"

using namespace Sexy;

int Sexy::gPieceCount = 0;

// 0x401000
Piece::Piece()
{
	gPieceCount++;
	mX = 0.0;
	mY = 0.0;
	mVX = 0.0;
	mVY = 0.0;
	mShakeX = 0;
	mShakeY = 0;
	mCol = 0;
	mRow = 0;
	mHighlighted = false;
	mGroupSize = 1;
	mDanger = false;
	mBlinkHidden = false;
	mSpin = 0.0;
	mSpinSpeed = 0.0;
	mColor = 0;
	mMark = 0;
	mIsSpecial = false;
	mFallThrough = false;
	mSpecialType = PIECE_MONEYBAG;
	mImageSet = 0;
	mFrame = 0;
	mFlying = false;
	mFlyT = 0.0;
	mFlySpeed = 0.0;
	mStartX = 0;
	mStartY = 0;
	mTargetX = 0;
	mTargetY = 0;
	mDying = false;
	mLifeTimer = 0;
	mUnusedA0 = 0;
}

// 0x4010b0
Piece::~Piece()
{
	gPieceCount--;
}
