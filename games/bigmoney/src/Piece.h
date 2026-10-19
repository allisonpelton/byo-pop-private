#ifndef __BIGMONEY_PIECE_H__
#define __BIGMONEY_PIECE_H__

// Piece (Board.cpp): one coin, money bag, dollar bomb, or a dying / flying / flung remnant of one.
// Grid pieces live in Board::mGrid, pieces that left the grid in Board::mLoosePieces.
// Original: class Sexy::Piece, sizeof 0xa8, vftable 0x4f5430 (1 slot: scalar deleting destructor
// 0x401d10). Offsets in the comments are absolute; savegame.dat stores the pieces field by field
// (Board::WritePiece / ReadPiece, a 108-byte record), not raw, so nothing depends on the port's layout.
// Behaviour spec: re/bigmoney/analysis/board_logic.md section 2.1.

namespace Sexy
{

// Piece::mSpecialType (+0x64), meaningful when Piece::mIsSpecial
enum PieceType
{
	PIECE_MONEYBAG = 0,
	PIECE_POOF = 1,             // removed coin under a bag, pushed-off or flung special: 14 frames (sparkle)
	PIECE_DOLLAR_BOMB = 2,      // 32-frame animation (dollar)
	PIECE_EXPLODING = 3         // detonated bomb / coins of its colour: 14 frames (explosion)
};

class Piece
{
public:
	enum
	{
		NUM_COIN_COLORS = 5,    // mColor 0..4 = red, blue, yellow, green, magenta (gCoinColors)
		SAVE_SIZE = 108         // savegame.dat record (ReadPiece 0x401120 / WritePiece 0x4012b0)
	};

	// +0x00 vfptr
	double	mX;              // +0x08 pixel x of the top-left corner (resting: Board::ColX(mCol))
	double	mY;              // +0x10 pixel y (resting: Board::RowY(mRow))
	double	mVX;             // +0x18 x velocity (column slide, flung coins, dropping specials 0.5)
	double	mVY;             // +0x20 y velocity, gravity +0.14/tick
	int		mShakeX;         // +0x28 draw offset written by the Action overflow shake; never reset
	int		mShakeY;         // +0x2c
	int		mCol;            // +0x30 grid column 0..13
	int		mRow;            // +0x34 grid row 0..14 (0 = top; next-row pieces 15 or 14; level end rows+2)
	bool	mHighlighted;    // +0x38 in the hovered group (>= mMinGroup)
	int		mGroupSize;      // +0x3c size of its same-colour group (FindGroups); ctor 1
	bool	mDanger;         // +0x40 column-top danger flash phase (SetDangerFlash)
	bool	mBlinkHidden;    // +0x41 life-timer blink phase; DrawPiece skips the piece when set
	// +0x42..+0x47 (padding)
	double	mSpin;           // +0x48 spin phase of a flung coin
	double	mSpinSpeed;      // +0x50 spin speed (FlingAllCoins)
	int		mColor;          // +0x58 coin colour 0..4; also a dollar bomb's colour
	int		mMark;           // +0x5c flood-fill scratch mark
	bool	mIsSpecial;      // +0x60 not a clickable coin (bag, bomb, poof, explosion)
	bool	mFallThrough;    // +0x61 leaves the board when it lands (bag collected / bomb detonated)
	// +0x62..+0x63 (padding)
	int		mSpecialType;    // +0x64 PieceType
	int		mImageSet;       // +0x68 1 = draw from IMAGE_HORZCOINS, else IMAGE_COINS
	int		mFrame;          // +0x6c animation frame
	bool	mFlying;         // +0x70 linear flight mStart -> mTarget (combo coins to the meter, bags to the holder)
	// +0x71..+0x77 (padding)
	double	mFlyT;           // +0x78 flight progress 0..1
	double	mFlySpeed;       // +0x80 per-tick increment of mFlyT
	int		mStartX;         // +0x88 (int)mX at launch
	int		mStartY;         // +0x8c
	int		mTargetX;        // +0x90
	int		mTargetY;        // +0x94
	bool	mDying;          // +0x98 removed coin playing its vanish animation
	// +0x99..+0x9b (padding)
	int		mLifeTimer;      // +0x9c > 0: counts down in UpdatePhysics, deleted at 0, blinks below 200 (never set > 0: dead feature)
	int		mUnusedA0;       // +0xa0 only the ctor, SaveGame and LoadGame touch it
	// +0xa4 (padding, sizeof 0xa8)

public:
	Piece();                 // 0x401000
	virtual ~Piece();        // 0x4010b0 (scalar deleting destructor 0x401d10, slot 0)
};

extern int gPieceCount;      // 0x52d860 live Piece objects (ctor ++, dtor --; never read)

}

#endif
