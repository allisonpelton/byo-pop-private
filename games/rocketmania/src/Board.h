#ifndef __ROCKETMANIA_BOARD_H__
#define __ROCKETMANIA_BOARD_H__

// The game of Rocket Mania Deluxe: the Board (RocketManiaGame 0x41ae80-0x429ed0, 141 functions) and the objects it
// keeps. The original's field offsets are given as comments (board+0x..., piece+0x...); the port's base classes have
// other sizes, so code never depends on them. Fields keep their offset in the name (m5c) until the code that uses them
// has been read; the ones that are understood have a name and a comment. The saved game (savea/b/c.dat) writes the
// fields in the order of 0x41c830, which is the order of the lists below.
//
// docs/GAME_NOTES.md has the map of what is where.

#include "Widget.h"
#include "ButtonListener.h"
#include "Common.h"
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class RocketApp;
class MTRand;
class Graphics;
class Buffer;

// The 0xb8-byte object of the original (constructor 0x41ad90): a tile of the grid, one of the pieces that wait or
// move along the edges, a flame or a rocket. Saved fields are written by 0x41c5a0 and read by 0x41b430.
struct Piece
{
	int			mId;            // +0x00 the unique number (lists are searched by it, saves keep it)
	bool		m04;            // +0x04 (set once the references of a loaded piece are resolved, 0x41b690)
	double		mX;             // +0x08
	double		mY;             // +0x10
	double		m18;            // +0x18 starts as a random number from -5 to 5
	double		m20;            // +0x20
	bool		m28;            // +0x28
	double		m30;            // +0x30
	double		m38;            // +0x38
	int			m40, m44, m48, m4c;     // +0x40
	int			mColumn;        // +0x50
	int			mRow;           // +0x54
	int			m58, m5c;       // +0x58
	bool		m60, m61;       // +0x60
	int			mType;          // +0x64 7 at the start; 1 a piece in a grid, 5 a piece that waits
	int			mKind;          // +0x68 the picture's row: 0 to 4 (the generator 0x42a590 draws it), -1 none
	int			mRotation;      // +0x6c the picture's column: 0 to 3, -1 none
	int			mBorn;          // +0x70 the update count at which it was made (saved as an age)
	int			m74, m78, m7c;  // +0x74
	double		m80;            // +0x80
	int			m88;            // +0x88 the board's update count at the creation (0x41ad90's argument)
	int			m8c, m90;       // +0x8c, +0x90 pointers to other pieces in the original (ids in a save)
	bool		m94;            // +0x94
	int			m98;            // +0x98
	int			m9c, ma0, ma4, ma8, mac, mb0, mb4;  // +0x9c
	Piece();
};

// 0x42a510, 0x74 bytes: draws the next piece of the board by the tables at 0x4e6800/0x4e6828 (not translated yet)
class PieceGenerator;

class Board : public Widget, public ButtonListener
{
public:
	RocketApp*		mApp;               // +0x4dc
	MTRand*			mRand;              // +0x110 (0x9c4 bytes in the original: the 2003 Mersenne twister)

	// ------------------------------------------------------------------ saved state, in the file's order
	int				mState;             // +0x5c 0 playing, 1 game over, 2 finished (saved), 4/5/6/7 end-of-level screens
	int				m60;                // +0x60
	double			m68;                // +0x68
	int				mLevel;             // +0x70 the level number counted from 0 (0x42a460 reads it)
	int				m74, m78, m7c, m80; // +0x74
	bool			m84;                // +0x84
	int				m88;                // +0x88
	int				m8c[14];            // +0x8c
	Piece*			mGrid[9][6];        // +0x114 column by column, six rows each
	Piece*			mLeft[6];           // +0x1ec down the left edge
	Piece*			m204;               // +0x204
	Piece*			mTop[9];            // +0x208 along the top edge
	Piece*			mBottom[9];         // +0x22c along the bottom edge
	int				m250[9];            // +0x250
	int				m280[9];            // +0x280
	int				m2a4[9];            // +0x2a4
	bool			m2c8[9];            // +0x2c8
	bool			m2d1[9];            // +0x2d1
	int				m2dc, m2e0, m2e4;   // +0x2dc
	bool			m2e8;               // +0x2e8
	int				m2ec, m2f0, m2f4, m2f8;     // +0x2ec
	double			m300, m308, m310, m318, m320;   // +0x300
	int				m328[6];            // +0x328
	bool			m340[6];            // +0x340
	int				m348, m34c, m350;   // +0x348
	double			m358;               // +0x358
	int				m360, m364, m368, m36c, m370, m374;     // +0x360
	int				m108, m10c;         // +0x108
	int				m380, m378, m37c;   // +0x380
	int				m384[30];           // +0x384
	bool			m3fc[30];           // +0x3fc
	bool			m41a[30];           // +0x41a
	bool			m438, m439;         // +0x438
	int				m43c, m440, m444, m448, m44c, m450, m454, m458;     // +0x43c
	bool			m45c;               // +0x45c
	int				m460, m464;         // +0x460
	int				m468[10];           // +0x468
	int				m490, m494;         // +0x490
	bool			m498, m499;         // +0x498
	int				m49c, m4a0, m4a4;   // +0x49c
	bool			m4a8, m4a9;         // +0x4a8
	int				m4ac, m4b0, m4b4, m4b8, m4bc;   // +0x4ac
	bool			m4c0;               // +0x4c0
	int				m4c4;               // +0x4c4
	bool			m4c8, m4c9;         // +0x4c8
	double			m4d0;               // +0x4d0
	bool			m4d8, m4d9, m4da;   // +0x4d8
	int				mFc, m100, m104;    // +0xfc
	int				mC4, mC8, mCc, mD0, mD4, mD8, mDc, mE0, mE4, mE8, mEc, mF0, mF4, mF8;   // +0xc4

	// ------------------------------------------------------------------ not saved
	int				m4e4;               // +0x4e4 the rows (6)
	int				m4e8;               // +0x4e8 the columns (9)
	int				m4f4, m4f8;         // +0x4f4 the pointer's place
	int				m4fc, m4e0, m51c;   // +0x4fc
	std::list<Piece*>	mObjects;       // +0x278 (count at +0x27c) the pieces that move about
	std::list<Piece*>	m504;           // +0x504 pieces read from a save, until their references are set
	std::list<int*>		m528;           // +0x528

public:
	Board(RocketApp* theApp, bool theContinuing);
	virtual ~Board();

	// Widget
	virtual void	Update() override;
	virtual void	Draw(Graphics* g) override;
	virtual void	ButtonDepress(int theId) override;

	// 0x41b660: a piece of m504 by its id
	Piece*			FindLoadedPiece(int theId);
	// 0x41b690: the references of the pieces read from a save
	void			ResolveLoadedPieces();
};

}

#endif
