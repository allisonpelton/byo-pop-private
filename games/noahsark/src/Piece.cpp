#include "Piece.h"

using namespace Sexy;

// The 0x68-byte object's initial values (the `operator new(0x68)` blocks, e.g. WinNoah.exe 0x401a50)
Piece::Piece() :
	mX(0), mY(0), mDestX(0), mDestY(0), mPixelX(0), mPixelY(0),
	mType(0),
	mAlpha(0xff),
	mOffsetX(0),
	mOffsetY(-0x1db),
	mFallSpeed(0),
	mSelected(false), mRemoved(false), mFading(false), mFlash(false), mMoving(false), mPending(false),
	mPathIndex(0),
	mFacingRight(true),
	mPartner(NULL),
	mScale(50.0),
	mInSwap(false), mMatch(false),
	mBlink(0)
{
}

// port: a piece's path goes with it (the original frees the nodes at each place it deletes a piece)
Piece::~Piece()
{
	for (size_t i = 0; i < mPath.size(); i++)
		delete mPath[i];
}

// 0x4060d0: the copy keeps every field and gives the clone its own path
Piece::Piece(const Piece& theOther) :
	mX(theOther.mX), mY(theOther.mY), mDestX(theOther.mDestX), mDestY(theOther.mDestY),
	mPixelX(theOther.mPixelX), mPixelY(theOther.mPixelY),
	mType(theOther.mType), mAlpha(theOther.mAlpha),
	mOffsetX(theOther.mOffsetX), mOffsetY(theOther.mOffsetY), mFallSpeed(theOther.mFallSpeed),
	mSelected(theOther.mSelected), mRemoved(theOther.mRemoved), mFading(theOther.mFading), mFlash(theOther.mFlash),
	mMoving(theOther.mMoving), mPending(theOther.mPending),
	mPathIndex(theOther.mPathIndex),
	mFacingRight(theOther.mFacingRight),
	mPartner(theOther.mPartner),
	mScale(theOther.mScale),
	mInSwap(false), mMatch(false),
	mBlink(0)
{
	// port: the original's copy (0x4060d0) leaves the path empty and the match fields at their defaults; the
	// copies that move a falling piece (0x403d60) copy the path node by node
}
