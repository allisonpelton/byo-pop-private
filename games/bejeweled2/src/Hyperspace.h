#ifndef __HYPERSPACE_H__
#define __HYPERSPACE_H__

// Hyperspace.h: the jump through hyperspace between two levels, drawn over the board (Sexy::Hyperspace, 0x5c5a68-0x5c8000,
// the object is 0x3b94 bytes). Board::DoHyperspace puts it over the board; the board follows mPhase, mReady and mDone.
// With 3D acceleration the jump is a tunnel of 24 rings of 24 points that fly towards the player (DoUpdate, Draw3dPortal);
// without it, a picture of the board that is stretched and shaken with lines of stars over it (Update2d, Draw2dPortal).
// The offsets in the comments are the original's, from the start of the object.

#include "Widget.h"
#include <vector>

namespace Sexy
{

class WinDMApp;
class Graphics;

// a point on a ring of the tunnel (0x18 bytes): its place on the screen and, for the two layers of the tunnel (the
// picture and its lines), where it is in the picture. DoUpdate scrolls the v coordinates.
struct TunnelPoint
{
	float			mX, mY;             // +0x00
	float			mU[2];              // +0x08
	float			mV[2];              // +0x10
};

// a ring of the tunnel (0x26c bytes): where the middle of the ring is now (Cur) and where it will be when the step is
// over (Next), the place in between (Lerp), the middle and the size on the screen, and its 24 points
struct TunnelRing
{
	float			mCurX, mCurY;       // +0x00
	float			mNextX, mNextY;     // +0x08
	float			mLerpX, mLerpY;     // +0x10
	float			mScreenX, mScreenY; // +0x18
	float			mSize;              // +0x20 the radius on the screen (not set until DoUpdate has run)
	float			mCurZ;              // +0x24
	float			mNextZ;             // +0x28
	TunnelPoint		mPoints[24];        // +0x2c
};

// a line of stars of the 2D jump (Sexy::WarpLine, 0x14 bytes): from the middle of the screen out to the edge, at x on a
// row of the screen
struct WarpLine
{
	int				mX;                 // +0x00 the place at the top of the screen
	float			mA;                 // +0x04 how far one end of the line has gone (towards the edge)
	float			mB;                 // +0x08 and the other end
	float			mVelA;              // +0x0c the speed of the first end
	float			mVelB;              // +0x10 the speed of the second end
};

class Hyperspace : public Widget
{
public:
	bool			m3D;                // +0x88 Is3DAccelerated: the tunnel is drawn with triangles (else the 2D jump)
	WinDMApp*		mApp;               // +0xdc
	TunnelRing		mRings[24];         // +0xe0 mRings[0] is the nearest and mRings[23] the farthest
	float			mWaveA;             // +0x3b00 the phases of the sines that sway the far ring (D and F never change)
	float			mWaveB;             // +0x3b04
	float			mWaveC;             // +0x3b08
	float			mWaveD;             // +0x3b0c
	float			mWaveE;             // +0x3b10
	float			mWaveF;             // +0x3b14
	float			mCenterX, mCenterY; // +0x3b18 where the first ring is, smoothed: the tunnel is drawn round it
	float			mCenter2X, mCenter2Y; // +0x3b20 the same for the second ring (it is only followed, never used)
	int				mStartDelay;        // +0x3b28 updates left until the tunnel starts to fly (200)
	float			mProgress;          // +0x3b2c 0 at the start of the jump, 1 when the tunnel is over
	bool			mReady;             // +0x3b30 the jump has begun (the board's picture is gone)
	int				mPhase;             // +0x3b34 0 before, 1 when mReady is set, 2 once Draw3dPortal has seen it, 3 set by Board
	int				mFadeDelay;         // +0x3b38 updates left until the flash (120)
	float			mFade;              // +0x3b3c the white flash over the screen
	float			mStep;              // +0x3b40 how far the rings are on their way from Cur to Next (0 to 1)
	bool			mDone;              // +0x3b44 the jump is over: Board removes it
	int				mDoneDelay;         // +0x3b48 updates left after mProgress has reached 1
	// the 2D jump
	float			mPosA;              // +0x3b54 how far the glow has moved from the middle (the "A" flight)
	float			mSpeedA;            // +0x3b58
	float			mPosB;              // +0x3b5c the second glow
	float			mSpeedB;            // +0x3b60
	int				mFrame;             // +0x3b64 updates of the 2D jump
	float			mStarAcc;           // +0x3b68 lines of stars that are owed (a new one at 1)
	float			mStarRate;          // +0x3b6c lines of stars made per update
	float			mStarSpeed;         // +0x3b70
	std::vector<WarpLine> mLines;       // +0x3b74
	float			mJitter;            // +0x3b84 how far the picture is shaken
	float			mStretch;           // +0x3b88 how much the picture is stretched (down from 1)
	float			mStretchSpeed;      // +0x3b8c
	bool			mNeedBgCache;       // +0x3b90 the picture of the board has to be made again

public:
	Hyperspace(WinDMApp* theApp);       // 0x5c72d1
	virtual ~Hyperspace();              // 0x5c7033

	virtual void	Update() override;                      // 0x5c7fd3
	virtual void	Draw(Graphics* g) override;             // 0x5c6beb

	void			DoUpdate();                             // 0x5c7957
	void			Update2d();                             // 0x5c7532
	void			Draw2dPortal(Graphics* g);              // 0x5c5a68
	void			Draw3dPortal(Graphics* g);              // 0x5c620e
};

}

#endif
