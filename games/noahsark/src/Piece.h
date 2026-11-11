#ifndef __NOAH_PIECE_H__
#define __NOAH_PIECE_H__

// One cell of the board: an animal, a crate, a block or an empty space. Original: a 0x68-byte object made by
// the `operator new(0x68)` blocks in Board.cpp (WinNoah.exe 0x401a50 and many other places); offsets below
// are the original's. The grid holds one Piece per cell; a piece that moves (falls, slides along a path, is
// shifted by a new row) is copied into the cell it reaches (0x4060d0 is the copy), so the port moves the
// object itself where the original copies and deletes.

#include <vector>

namespace Sexy
{

// Piece::mType
enum PieceType
{
	PIECE_ANIMAL_LAST = 12,      // 0..12: the animal kinds (row index in images/animals; the kind's sound is SOUND_WINPIG + kind)
	NUM_ANIMALS = 13,
	PIECE_EMPTY = 14,            // 0xe
	PIECE_BLOCK = 15,            // 0xf: block.jpg, an obstacle that stays where it is
	PIECE_CRATE = 16             // 0x10: crate.jpg, an obstacle that falls (strategy) with the animals
};

// A point of a piece's path (0x20 bytes in the original: pixel x, pixel y, cell x, cell y, and at +0x18 the kind of bend or arrow
// that the trail's builder 0x402ec0 works out and nothing reads: the port keeps neither the field nor the working out)
struct PathNode
{
	int				mPixelX;                // +0x00
	int				mPixelY;                // +0x04
	int				mCellX;                 // +0x0c
	int				mCellY;                 // +0x10
};

struct Piece
{
	int				mX;                     // +0x00 column 0..8
	int				mY;                     // +0x04 row 0..8 (0 is the top)
	int				mDestX;                 // +0x08
	int				mDestY;                 // +0x0c
	float			mPixelX;                // +0x10 (cell pixel + mOffsetX is where it is drawn)
	float			mPixelY;                // +0x14
	int				mType;                  // +0x18 PieceType
	int				mAlpha;                 // +0x1c 0xff
	int				mOffsetX;               // +0x20 0; pixels the piece is off its cell, decays to 0
	int				mOffsetY;               // +0x24 -0x1db: a new piece starts above the board and falls
	int				mFallSpeed;             // +0x28 0
	bool			mSelected;              // +0x2c
	bool			mRemoved;               // +0x2d being taken out (the animal shrinks away)
	bool			mFading;                // +0x2e
	bool			mFlash;                 // +0x2f flashes grey (hint)
	bool			mMoving;                // +0x30 walking along mPath towards mPartner
	bool			mPending;               // +0x31 a falling piece that has been released
	int				mPathIndex;             // +0x34 next node of mPath
	bool			mFacingRight;           // +0x38 1
	Piece*			mPartner;               // +0x3c the animal this one walks to
	double			mScale;                 // +0x40 50.0
	std::vector<PathNode*>	mPath;          // +0x48 (the original's std::vector of PathNode*)
	bool			mInSwap;            // +0x58 one of the two animals of the puzzle swap in progress (the swap back finds them by it)
	bool			mMatch;                 // +0x59 part of a run of three (puzzle mode)
	int				mBlink;                 // +0x5c 0..4

	Piece();
	~Piece();
	Piece(const Piece& theOther);           // 0x4060d0: a copy (the path is deep copied)
private:
	Piece& operator=(const Piece&);
};

}

#endif
