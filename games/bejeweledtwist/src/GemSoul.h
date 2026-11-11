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
	float mRetargetWait = 0;         // +0x660: the wait to a new target
	float mTargetX = 0;              // +0x664
	float mTargetY = 0;              // +0x668
	float mVelX = 0;                 // +0x66c
	float mVelY = 0;                 // +0x670
	int mDrawX = 0;                  // +0x674
	int mDrawY = 0;                  // +0x678
	Piece* mPiece = nullptr;         // +0x67c: the gem, drawn at the soul until it blasts or after 200 updates
	Piece* mHomePiece = nullptr;     // +0x680: the cell it goes back to in state 2 (its +0x20 and +0x28)
	float mBrightness = 0;           // +0x684: its brightness (Hyperspace fades it out in state 7)
	Image* mOuter = nullptr;         // +0x688: IMAGE_ELEMENTALS_OUTER_*
	Image* mInner = nullptr;         // +0x68c
	Image* mCore = nullptr;          // +0x690
	Image* mSheen = nullptr;         // +0x694
	Image* mParticle = nullptr;      // +0x698
	Image* mBeamSmall = nullptr;     // +0x69c
	Image* mBeamMedium = nullptr;    // +0x6a0
	Image* mBeamLarge = nullptr;     // +0x6a4
	float mBeamPhase[12] = {};       // +0x6a8: twelve beams' places, rising by mBeamRate, back to 0 past 1
	float mBeamRate[12] = {};        // +0x6d8: twelve beams' speeds, 0 when a beam is idle
	int mBeamSize[12] = {};          // +0x708: twelve beams' sizes, 0 to 2 each
	int mState = 0;                  // +0x738: 0 coming out, 1 swirling, 2 going back to mHomePiece
	bool mIsHome = false;            // +0x73c: back at mHomePiece
	float mReturn = 1.0f;            // +0x740: how far the soul has come back (the curve mReturnCurve gives it in state 2)
	float mBeamLength = 0;           // +0x744: 0, never read
	CurvedVal mSpaceFade;            // +0x748: set by the ctor (0x7e32c8); the 3D draw reads it
	float mCoreSize = 0;             // +0x7b8: 0, never read
	float mSpacePos[3] = {};         // +0x7bc: Direct3D's place in space, from the hyperspace's camera (+0x14c0)
	float mSpaceGoal[3] = {};        // +0x7c8: the place in space it follows
	float mSpaceVel[3] = {};         // +0x7d4: its speed in space
	CurvedVal mFlightClock;          // +0x7e0: the clock of its flight back (mReturnCurve follows it)
	int mTurnBackX = 0;              // +0x850: where it was when it turned back (x, y)
	int mTurnBackY = 0;              // +0x854: and y
	CurvedVal mReturnCurve;          // +0x858: how far it has come back, along mFlightClock
	int mRectX = 0;                  // +0x8c8: the area it swirls in (x, y, width, height)
	int mRectY = 0;                  // +0x8cc
	int mRectWidth = 0;              // +0x8d0
	int mRectHeight = 0;             // +0x8d4
	float mSwirlRadiusStep = 0;      // +0x8d8: 0, never read
	float mSwirlGrowth = 0.02f;      // +0x8dc: 0.02, never read
	float mAngle = 0;                // +0x8e0
	float mAngleSpeed = 0;           // +0x8e4
	float mSwirlAngle = 0;           // +0x8e8: a random angle that turns by mSwirlSpeed
	float mSwirlSpeed = 0;           // +0x8ec: its turn an update
	float mRadius = 0;               // +0x8f0
	bool mIsVisible = true;          // +0x8f4: the soul is drawn; Hyperspace clears it late in a jump
	bool mFirst = false;             // +0x8f5: the board had no souls before it

	// (the original's two virtuals have no overrides, so the port's are plain)
	explicit GemSoul(int theType);   // 0x70188f
	~GemSoul();                      // 0x7011ee: deletes mPiece
	// 0x701c2b: back to thePiece's cell on the new board (state 2), over mFlightClock's curve
	void SetTarget(Piece* thePiece);
	void Update();                   // 0x700909 (slot 0)
	void Draw(Graphics* g);          // 0x70120d (slot 1)
};

}  // namespace Sexy
