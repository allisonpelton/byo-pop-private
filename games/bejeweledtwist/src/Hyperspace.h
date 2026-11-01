// Sexy::Hyperspace (0x15e0 bytes, a TwistWidget; 0x70e341-0x71f10e): the space behind every screen and the jump
// between levels. With Direct3D it flies a camera through the level's world (planets, a saucer, the sidebar's tube);
// in 2D it draws the still picture of space and, for the jumps, flashes and a tunnel of its own.
//
// port: so far the 2D drawing of space outside a jump (the loader and the menus). The jumps (mTransition 8 to 14,
// 0x711b65, 0x70f836, the white flash +0x1320 and the fades +0x5d8), the level's world (0x7181b3's curves) and the
// camera (the rest of Update 0x715c21) come with the board.
#pragma once

#include "CurvedVal.h"
#include "TwistWidget.h"

namespace Sexy {

class Hyperspace : public TwistWidget {
public:
	int mLevel = 0;              // +0x148: the board's level - 1
	int mState = 0;              // +0x14c: 0 idle, 6 and 7 after a jump, 10 done, ...
	int mTransition = 0;         // +0x150: the jump SetTransition last set up
	bool mFlag155 = false;       // +0x155
	bool mIs3D = false;          // +0x156: drawn with Direct3D
	bool mFlag157 = false;       // +0x157: set by the mode select's new game (Mandala::StartMode)
	bool mFlag158 = false;       // +0x158
	bool mFlag159 = true;        // +0x159
	bool mFlag15A = false;       // +0x15a: set when a board takes it (0x6a5fa3)
	bool mFlag15B = true;        // +0x15b: the jump's first half is still to come (transition 8)
	bool mFlag15C = false;       // +0x15c: set while Update skipped the jump
	bool mFlag15E = false;       // +0x15e: holds the jump (set from outside, not yet known)
	CurvedVal mUnknown258;       // +0x258: the board's brightness through a jump (Board 0x6a6a07)
	int mUnknownC58 = 0;         // +0xc58: the trial's limit after transition 14 (left out)
	CurvedVal mUnknown12B0;      // +0x12b0: the board's brightness while state 11 (transition 12) runs
	CurvedVal mUnknown1570;      // +0x1570: the jump's length
	int mWorld = 4;              // +0xc5c: the level's world (0x7181b3 picks it by level)
	CurvedVal mFlash;            // +0x1320: the white flash of a jump
	double mWarmUp = 0;          // +0x1078: rises to 1 while idle
	Image* mUnknown160 = nullptr;  // +0x160: the old level's picture during a 2D jump (the VizManager's)
	int mUnknown164 = -1;        // +0x164: its id

	Hyperspace();                // 0x714f98
	~Hyperspace() override;      // 0x714e6e

	void Update() override;                  // 0x715c21
	void Draw(Graphics* g) override;         // 0x713e2d
	void DrawOverlay(Graphics* g) override;  // 0x713d82
	using Widget::DrawOverlay;

	// 0x7181b3: sets up the jump theTransition (0 none) and the level's world
	void SetTransition(int theTransition);
	// 0x70e623: the VizManager's picture (and 3D visualizer) for the board's game and level
	void ChooseViz(bool theDraw);
	// 0x711b65: the tunnel of a jump; in 2D, while the VizManager is hidden, the level's picture (mUnknown160 for the
	// first half) faded in by mUnknown1570 and centred at the art's width
	void DrawTunnel(Graphics* g);
	// 0x70f836: the old level's picture (mUnknown160) at mUnknown12B0's alpha, at the end of a jump
	void DrawOldPicture(Graphics* g);
};

}  // namespace Sexy
