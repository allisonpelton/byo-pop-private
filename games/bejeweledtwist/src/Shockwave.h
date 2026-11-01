// Sexy::Shockwave (vtable 0x7f1664, 0x7491ca-0x74956f) and Sexy::MatchShockwave (0x50 bytes, vtable 0x7f16bc,
// 0x74a3c8-0x74bd88): waves that push the board's gems aside for a moment. The board keeps them at +0x1660 and
// updates them in 0x6af497. Being ported: so far the MatchShockwave, which a match sends through the gems around it,
// and the BlastShockwave (0x104 bytes, vtable 0x7f1684), a power gem's blast.
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
	float mUnknown14 = 0;            // +0x14
	float mMaxRadius = 0;            // +0x18
	float mDuration = 1.0f;          // +0x1c
	float mTime = 0;                 // +0x20
	float mUnknown24 = 1.0f;         // +0x24
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

	int mUnknown34 = 0;              // +0x34: counts every 10 updates
	float mRings[20] = {};           // +0x38: the rings' radii
	float mRingAlphas[20] = {};      // +0x88
	float mRingWait = 0;             // +0xd8: before the next ring
	int mRingCount = 0;              // +0xdc
	Color mColor;                    // +0xe0: the gem's colour
	bool mStarting = true;           // +0xf0: while the constructor sets off the gems
	std::vector<Spark*> mSparks;     // +0xf4

	// 0x74b46e: blows up or sets off the gems within theRange gems of thePiece
	BlastShockwave(Piece* thePiece, int theRange, int theShape);
	~BlastShockwave() override;      // 0x74ac68
	void Update() override;          // 0x74bea6
	void Draw(Graphics* g) override; // 0x749c50
};

}  // namespace Sexy
