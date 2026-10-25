#ifndef __PIECE_H__
#define __PIECE_H__

#include <cstddef>

namespace Sexy
{

// One gem on the board. A 0x78-byte non-polymorphic struct (no vtable, no RTTI). Board::SaveGame
// writes all 0x78 bytes raw into savegame.dat and Board::LoadGame reads them back, so the exact
// layout (including padding) is part of the save format; the explicit padding members keep it
// identical on every target. The original ctor (0x4c9aec) leaves the padding and +0x60..+0x77
// uninitialised; the port zero-initialises everything.
// port(variant): the code-mod fields follow the 0x78-byte record (SAVE_SIZE); the save still writes and
// reads exactly the record (games/bejeweled/docs/CODE_MODS_DESIGN.md 2.4).
struct Piece
{
	int		mColor;          // +0x00 gem color 0..6 (Rand()%7); gemsheet row = mColor*104
	int		mPad04;          // +0x04 (padding before the double; raw garbage in original saves)
	double	mX;              // +0x08 screen x of the top-left corner; resting value GetColX(mCol)
	double	mY;              // +0x10 screen y of the top-left corner; resting value GetRowY(mRow)
	int		mCol;            // +0x18 grid column 0..7
	int		mRow;            // +0x1c grid row 0..7 (0 = top)
	int		mVelX;           // +0x20 game-over explosion x velocity (Rand()%7-3), added to mX each tick in state 5
	int		mPad24;          // +0x24 (padding)
	double	mVelY;           // +0x28 fall velocity: +0.18/tick in states 0 and 9, +0.15/tick in state 5
	int		mSwapOffX;       // +0x30 (int)(mX - Board::mSwapCenterX) at swap start; negated for the swap back
	int		mSwapOffY;       // +0x34 (int)(mY - Board::mSwapCenterY)
	bool	mGlowing;        // +0x38 mouse-over glow active
	char	mPad39[7];       // +0x39 (padding)
	double	mGlowAlpha;      // +0x40 glow crossfade 0..1: -0.012/tick while glowing, +0.045/tick more while hovered (max 1.0)
	int		mGlowFrame;      // +0x48 glow animation frame 0..14 (glow row src x = mGlowFrame*52 + sx)
	bool	mDying;          // +0x4c matched / swallowed by the vortex: drawn from WinDMApp::mGemShrinkImages
	char	mPad4D[3];       // +0x4d (padding)
	int		mDyingFrame;     // +0x50 shrink frame 0..5; the piece is deleted when it would reach 6
	int		mSpinFrame;      // +0x54 spin frame 0..14 (normal row src x = mSpinFrame*52 + 2); 0 = still
	int		mSparkleCount;   // +0x58 hint sparkle countdown (DoHint sets 58; -1 every 5 ticks)
	int		mSparkleFrame;   // +0x5c hint sparkle frame counter (+1 every 5 ticks while mSparkleCount > 0)
	double	mHoleAngle;      // +0x60 vortex polar angle (radians) around the board center (406,219)
	double	mHoleAngVel;     // +0x68 vortex angular velocity
	double	mHoleRadius;     // +0x70 vortex radius, -1 per tick, clamped at 0

	enum { SAVE_SIZE = 0x78 };  // port(variant): the raw savegame.dat record; the layout up to here is unchanged
	// port(variant): not part of the raw save record
	int		mKind;           // +0x78 special gem: 0 normal, 1 flame, 2 lightning (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.8)
	int		mKindFrame;      // +0x7c special gem overlay animation frame (cosmetic)

	Piece();                 // 0x4c9aec
};

static_assert(offsetof(Piece, mKind) == Piece::SAVE_SIZE, "the variant tail starts after the 0x78-byte savegame.dat record");
static_assert(offsetof(Piece, mX) == 0x08, "Piece layout");
static_assert(offsetof(Piece, mVelY) == 0x28, "Piece layout");
static_assert(offsetof(Piece, mGlowAlpha) == 0x40, "Piece layout");
static_assert(offsetof(Piece, mDyingFrame) == 0x50, "Piece layout");
static_assert(offsetof(Piece, mHoleRadius) == 0x70, "Piece layout");

}

#endif
