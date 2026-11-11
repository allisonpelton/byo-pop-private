// Sexy::Shockwave (vtable 0x7f1664, 0x7491ca-0x74956f) and Sexy::MatchShockwave (0x50 bytes, vtable 0x7f16bc,
// 0x74a3c8-0x74bd88): waves that push the board's gems aside for a moment. The board keeps them at +0x1660 and
// updates them in 0x6af497. Being ported: so far the MatchShockwave, which a match sends through the gems around it,
// the BlastShockwave (0x104 bytes, vtable 0x7f1684), a power gem's blast, and the FruitShockwave (0x110 bytes, vtable
// 0x7f16a0), the bonus challenge's hint.
#pragma once

#include "Color.h"

#include <vector>

namespace Sexy {

class Graphics;
class Piece;

class Shockwave {
public:
	Piece* mPiece = nullptr;         // +0x4
	float mX = 0;                    // +0x8
	float mY = 0;                    // +0xc
	float mRadius = 0;               // +0x10
	float mRingSpeed = 0;            // +0x14: how far the rings and sparks grow in an update
	float mMaxRadius = 0;            // +0x18
	float mDuration = 1.0f;          // +0x1c
	float mTime = 0;                 // +0x20
	float mAlpha = 1.0f;             // +0x24: the rings' opacity (1; a short reach lowers it), fading as the wave goes
	int mUpdates = 0;                // +0x28
	int mShape = 0;                  // +0x2c: 1 for a square
	bool mDone = false;              // +0x30: the board deletes it

	Shockwave(Piece* thePiece, int theShape);  // 0x749200 (the port's MatchShockwave passes no piece)
	virtual ~Shockwave() {}          // 0x7491cd
	virtual void Update();           // 0x749511: grows to mMaxRadius over mDuration
	virtual void Draw(Graphics* g) {}  // 0x74956f (the MatchShockwave's, 0x749693, draws nothing too)
	virtual void OnGem(Piece* thePiece) {}  // 0x7491ca: a gem the wave reaches (the BlastShockwave's, 0x749572, too)
	virtual void FindGems();         // 0x749295: OnGem for each gem in reach that is not breaking or blasted
};

class MatchShockwave : public Shockwave {
public:
	// +0x34: the gems it pushes (0x74be51 adds one)
	struct Push {
		int mRow;
		int mCol;
		int mDX;
		int mDY;
	};
	std::vector<Push> mPushes;
	float mStrength = 20.0f;         // +0x44: the cascade's, up to 20
	float mAngle = 0;                // +0x48: 0 to pi over the pulse
	float mStep = 0.12566371f;       // +0x4c: the angle an update

	MatchShockwave(int theX, int theY);          // 0x74bcf3
	void SetStrength(float theCascade);          // 0x749617
	void AddPush(int theRow, int theCol, int theDX, int theDY) { mPushes.push_back({theRow, theCol, theDX, theDY}); }  // 0x74c89a
	void Update() override;                      // 0x74a3c8
};

class BlastShockwave : public Shockwave {
public:
	// 0x1c bytes: a spark thrown out from the blast
	struct Spark {
		float mX;                    // +0x0
		float mY;                    // +0x4
		float mDX = 0;               // +0x8
		float mDY = 0;               // +0xc
		float mGravity = 0;          // +0x10: 0.07 once it falls
		float mDelay;                // +0x14: before it falls
		float mAlpha = 1.0f;         // +0x18
	};

	int mTenUpdateCount = 0;         // +0x34: counts every 10 updates
	float mRings[20] = {};           // +0x38: the rings' radii
	float mRingAlphas[20] = {};      // +0x88
	float mRingWait = 0;             // +0xd8: before the next ring
	int mRingCount = 0;              // +0xdc
	Color mColor;                    // +0xe0: the gem's colour
	bool mStarting = true;           // +0xf0: while the constructor sets off the gems
	std::vector<Spark*> mSparks;     // +0xf4

	// 0x74b46e: blows up or sets off the gems within theRange gems of thePiece
	BlastShockwave(Piece* thePiece, int theRange, int theShape);
	~BlastShockwave() override;      // 0x74ac68, 0x74abc0: its body, inline here
	void Update() override;          // 0x74bea6
	void Draw(Graphics* g) override; // 0x749c50
};

// The bonus challenge's hint (0x110 bytes, vtable 0x7f16a0, constructor 0x74b94e): a ring spreads from the gem the hint
// points at while an arc of sparks curves from it to the tube. Direct3D also draws the arc as a ribbon (0x749e12).
class FruitShockwave : public Shockwave {
public:
	// 0x34 bytes (0x749c33): a spark of the burst (mKind 0), or one running along the arc to the tube (mKind 1)
	struct Particle {
		float mX = 0;                // +0x00
		float mY = 0;                // +0x04
		float mDX = 0;               // +0x08
		float mDY = 0;               // +0x0c
		float mAngle = 0;            // +0x10: along the arc
		float mRadius = 0;           // +0x14
		float mCenterX = 0;          // +0x18
		float mCenterY = 0;          // +0x1c
		float mAngleStep = 0;        // +0x20
		float mGravity = 0;          // +0x24
		float mWait = 0;             // +0x28: before the burst's spark falls
		float mAlpha = 1.0f;         // +0x2c
		int mKind = 0;               // +0x30
	};

	float mRings[20] = {};           // +0x34: the rings' radii (0 for one not in use)
	float mRingWait = 0;             // +0x84: before the next ring
	bool mIsStarted = false;         // +0x88: false, set by the constructor, never read
	int mColorIndex = 0;             // +0x8c: the hint gem's colour
	Color mColor;                    // +0x90: its tint
	int mState = 1;                  // +0xa0: 1 and 2 spread, 3 fades out
	int mHintRange = 0;              // +0xa4: the constructor's last argument (200 for the hint's wave); never read
	int mRingStep = 1;               // +0xac: 1, set by the constructor, never read
	float mStartX = 0;               // +0xb0: the gem
	float mStartY = 0;               // +0xb4
	float mEndX = 0;                 // +0xb8: where the arc goes (the tube's middle)
	float mEndY = 0;                 // +0xbc
	float mWidth = 150.0f;           // +0xc0: the ribbon's width (Direct3D)
	float mArcStart = 0;             // +0xc4: where the ribbon starts along the arc, 0 to 1
	float mArcEnd = 0;               // +0xc8: how far the arc has come, up to 1
	float mAngleStart = 0;           // +0xcc: the arc's angles about its centre
	float mAngleEnd = 0;             // +0xd0
	float mArcAlpha = 0;             // +0xd4: the arc's opacity (2.5 at first, falling by 0.015 an update)
	float mCenterX = 0;              // +0xd8: the arc's centre
	float mCenterY = 0;              // +0xdc
	float mArcRadius = 0;            // +0xe0
	int mUnlockStep = 1;             // +0xe4: counts the unlocking
	float mUnlockWait = 0;           // +0xe8: the wait to the next
	Color mColor2;                   // +0xec
	float mRingGlow = 0;             // +0xfc: the rings' glow, up by 0.02 an update while the wave spreads
	std::vector<Particle*> mParticles;  // +0x100

	// 0x74b94e: thePiece is the gem the hint points at, theTargetX and theTargetY where the arc goes
	FruitShockwave(Piece* thePiece, int theRange, int theTargetX, int theTargetY, int theHintRange);
	~FruitShockwave() override;      // 0x74bcd4 (the original keeps the particles in a vector it frees without them)
	void Update() override;          // 0x74c1aa
	void Draw(Graphics* g) override; // 0x749e12
	void OnGem(Piece* thePiece) override;  // 0x74958d
};

}  // namespace Sexy
