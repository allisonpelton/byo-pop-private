#ifndef __BOOKWORM_PIECE_H__
#define __BOOKWORM_PIECE_H__

// Piece (Board.cpp): one letter tile. Grid tiles live in Board::mBoard, the tiles of a submitted word in a
// FlyingPieceList (which owns them once it is created).
// Original: class Sexy::Piece, sizeof 0xf0, vftable 0x4f9ac8 (1 slot: scalar deleting destructor 0x404050).
// Offsets in the comments are absolute; Save/Load write the 191-byte record field by field, so nothing
// depends on the port's layout. The methods are in Board.cpp.
// Behaviour spec: re/bookworm/analysis/board_logic.md 2.2 and 6.2.

#include <cstdio>
#include <list>

namespace Sexy
{

// Piece::mType (+0xe8); WordScore::mTypeCount index. Board.h includes this header (NAMES.md 4.2).
enum TileType
{
	TILE_NORMAL = 0,                // tile
	TILE_GREEN = 1,                 // tile_green: 5% of new normal tiles in Refill
	TILE_GOLD = 2,                  // tile_gold: 6..8 letter words
	TILE_FIRE = 3,                  // tile_fire: burns the tile below, game over at the bottom
	TILE_SAPPHIRE = 4,              // tile_sapphire
	TILE_DIAMOND = 5,               // tile_diamond
	NUM_TILE_TYPES = 6
};

// Number of Piece objects alive (debug counter, never read). 0x534720
extern int gLivePieceCount;

class Piece
{
public:
	enum
	{
		SAVE_SIZE = 191             // Save/Load record
	};

	// +0x00 vfptr, +0x04 padding
	double	mX;                     // +0x08 screen x of the top-left corner (resting: Board::ColX)
	double	mY;                     // +0x10 screen y (resting: Board::RowY)
	double	mFlySpeed;              // +0x18 FlyingPieceList: step factor of both x and y (NAMES.md D4)
	double	mVY;                    // +0x20 fall velocity, +0.2 per tick
	int		mUnk28;                 // +0x28 saved only
	int		mUnk2c;                 // +0x2c saved only
	int		mCol;                   // +0x30 0..6
	int		mRow;                   // +0x34 0..7 (0 = top)
	bool	mUnk38;                 // +0x38 saved only
	// +0x39..+0x3b padding
	int		mUnk3c;                 // +0x3c saved only, ctor 1
	bool	mUnk40;                 // +0x40 saved only
	bool	mUnk41;                 // +0x41 saved only
	// +0x42..+0x47 padding
	double	mUnk48;                 // +0x48 saved only
	double	mUnk50;                 // +0x50 saved only
	int		mUnk58;                 // +0x58 saved only
	int		mUnk5c;                 // +0x5c saved only
	bool	mUnk60;                 // +0x60 saved only
	bool	mUnk61;                 // +0x61 saved only
	// +0x62..+0x63 padding
	int		mUnk64;                 // +0x64 saved only
	int		mUnk68;                 // +0x68 saved only
	int		mUnk6c;                 // +0x6c saved only
	bool	mUnk70;                 // +0x70 saved only
	// +0x71..+0x77 padding
	double	mUnk78;                 // +0x78 saved only
	double	mUnk80;                 // +0x80 saved only
	int		mUnk88;                 // +0x88 saved only
	int		mUnk8c;                 // +0x8c saved only
	int		mUnk90;                 // +0x90 saved only
	int		mUnk94;                 // +0x94 saved only
	bool	mUnk98;                 // +0x98 saved only
	// +0x99..+0x9b padding
	int		mUnk9c;                 // +0x9c saved only
	int		mUnka0;                 // +0xa0 saved only
	char	mLetter;                // +0xa4 'A'..'Z' ('Q' is "Qu"), ctor 0xff
	char	mNextLetter;            // +0xa5 letter shown after a flip (scramble, special tile), ctor 0xff
	bool	mSelected;              // +0xa6 in Board::mSelection
	// +0xa7 padding
	double	mScale;                 // +0xa8 1.0; FlyingPieceList shrinks it
	bool	mBurning;               // +0xb0 burning away (charred by the fire tile above), sinks 57 px
	// +0xb1..+0xb3 padding
	int		mBurnProgress;          // +0xb4 0..57 pixels sunk
	int		mBurnDelay;             // +0xb8 ticks before sinking starts (20 at StartBurn)
	bool	mPendingFire;           // +0xbc action mode: turns into a fire tile soon
	// +0xbd..+0xbf padding
	int		mUnkc0;                 // +0xc0 saved only
	bool	mFlicker;               // +0xc4 frame animation on (board burned)
	// +0xc5..+0xc7 padding
	int		mFlickerFrame;          // +0xc8 0..7
	bool	mShaking;               // +0xcc jitter (tile under a fire tile, bottom fire tile)
	// +0xcd..+0xcf padding
	double	mShakeAmount;           // +0xd0 jitter amplitude
	bool	mFlipping;              // +0xd8 letter flip / type change animation
	// +0xd9..+0xdf padding
	double	mFlipProgress;          // +0xe0 0..1, +0.01 per tick
	int		mType;                  // +0xe8 TileType
	int		mBurnCountdown;         // +0xec advances left before the fire tile above burns it

public:
	Piece();                                                    // 0x403f20 (++gLivePieceCount)
	virtual ~Piece();                                           // 0x402ff0 body, 0x404050 slot 0 (--gLivePieceCount)

	void			SetType(int theType, bool resetCountdown);  // 0x403000 (only 0 <= theType < 6)
	void			StartBurn();                                // 0x403030
	void			ResetBurnCountdown();                       // 0x404080
	static void		Deselect(Piece* thePiece);                  // 0x403f10 (std::for_each helper)
	void			Save(FILE* theFile);                        // 0x403050 (191 bytes)
	void			Load(FILE* theFile);                        // 0x403370
};

typedef std::list<Piece*> PieceList;

}

#endif
