// Sexy::GemSoul (0x8f8 bytes, vtable 0x7e32b8: Update 0x700909 and Draw 0x70120d, no widget; 0x700909-0x701c2b): at a
// level's end each gem left on the board (Board::UpdateGame's jump, +0x26a8) leaves its cell as a soul that swirls
// over the board and away. The board keeps them at +0x26b0 (0x6a6eaa its size).
#pragma once

#include "CurvedVal.h"

namespace Sexy {

class Graphics;
class Image;
class Piece;

class GemSoul {
public:
	struct Particle {                // +0x8, 100 of them, 0x10 bytes each
		int mX = 0;                  // +0x0: in the outer image, from its middle
		int mY = 0;                  // +0x4
		float mT = 0;                // +0x8: 0 to 1 and back
		float mSpeed = 0;            // +0xc: 0 when it is to be placed again
	};

	Particle mParticles[100];        // +0x8
	int mUpdateCnt = 0;              // +0x648
	int mColor = 0;                  // +0x64c: the gem's colour (Piece +0x8)
	int mType = 0;                   // +0x650: the gem's flags it was made from; 0 for none of the elementals
	int mIndex = 0;                  // +0x654: how many souls the board had before it
	float mX = 0;                    // +0x658
	float mY = 0;                    // +0x65c
	float mUnknown660 = 0;           // +0x660: the wait to a new target
	float mTargetX = 0;              // +0x664
	float mTargetY = 0;              // +0x668
	float mVelX = 0;                 // +0x66c
	float mVelY = 0;                 // +0x670
	int mDrawX = 0;                  // +0x674
	int mDrawY = 0;                  // +0x678
	Piece* mPiece = nullptr;         // +0x67c: the gem, drawn at the soul until it blasts or after 200 updates
	Piece* mUnknown680 = nullptr;    // +0x680: the cell it goes back to in state 2 (its +0x20 and +0x28)
	float mUnknown684 = 0;           // +0x684: its brightness (Hyperspace fades it out in state 7)
	Image* mOuter = nullptr;         // +0x688: IMAGE_ELEMENTALS_OUTER_*
	Image* mInner = nullptr;         // +0x68c
	Image* mCore = nullptr;          // +0x690
	Image* mSheen = nullptr;         // +0x694
	Image* mParticle = nullptr;      // +0x698
	Image* mBeamSmall = nullptr;     // +0x69c
	Image* mBeamMedium = nullptr;    // +0x6a0
	Image* mBeamLarge = nullptr;     // +0x6a4
	float mUnknown6A8[12] = {};      // +0x6a8: rising by mUnknown6D8, back to 0 past 1
	float mUnknown6D8[12] = {};      // +0x6d8
	int mUnknown708[12] = {};        // +0x708: 0 to 2 each
	int mState = 0;                  // +0x738: 0 coming out, 1 swirling, 2 going back to mUnknown680
	bool mUnknown73C = false;        // +0x73c: back at mUnknown680
	float mUnknown740 = 1.0f;        // +0x740
	float mUnknown744 = 0;           // +0x744
	CurvedVal mUnknown748;           // +0x748: set by the ctor (0x7e32c8)
	float mUnknown7B8 = 0;           // +0x7b8
	float mUnknown7BC[3] = {};       // +0x7bc: Direct3D's place in space, from the hyperspace's camera (+0x14c0)
	float mUnknown7C8[3] = {};       // +0x7c8
	float mUnknown7D4[3] = {};       // +0x7d4
	CurvedVal mUnknown7E0;           // +0x7e0
	int mUnknown850 = 0;             // +0x850: where it was when it turned back (x, y)
	int mUnknown854 = 0;             // +0x854
	CurvedVal mUnknown858;           // +0x858
	int mRectX = 0;                  // +0x8c8: the area it swirls in (x, y, width, height)
	int mRectY = 0;                  // +0x8cc
	int mRectWidth = 0;              // +0x8d0
	int mRectHeight = 0;             // +0x8d4
	float mUnknown8D8 = 0;           // +0x8d8
	float mUnknown8DC = 0.02f;       // +0x8dc
	float mAngle = 0;                // +0x8e0
	float mAngleSpeed = 0;           // +0x8e4
	float mUnknown8E8 = 0;           // +0x8e8
	float mUnknown8EC = 0;           // +0x8ec
	float mRadius = 0;               // +0x8f0
	bool mUnknown8F4 = true;         // +0x8f4
	bool mFirst = false;             // +0x8f5: the board had no souls before it

	// (the original's two virtuals have no overrides, so the port's are plain)
	explicit GemSoul(int theType);   // 0x70188f
	~GemSoul();                      // 0x7011ee: deletes mPiece
	// 0x701c2b: back to thePiece's cell on the new board (state 2), over mUnknown7E0's curve
	void SetTarget(Piece* thePiece);
	void Update();                   // 0x700909 (slot 0)
	void Draw(Graphics* g);          // 0x70120d (slot 1)
};

}  // namespace Sexy
