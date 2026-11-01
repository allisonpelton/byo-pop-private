// Sexy::Shockwave (vtable 0x7f1664, 0x7491ca-0x74956f) and Sexy::MatchShockwave (0x50 bytes, vtable 0x7f16bc,
// 0x74a3c8-0x74bd88): waves that push the board's gems aside for a moment. The board keeps them at +0x1660 and
// updates them in 0x6af497. Being ported: so far the MatchShockwave, which a match sends through the gems around it,
// and the parts of the Shockwave it uses (the Shockwave's own update, 0x749511, and its gem search, 0x749295, come
// with the shockwaves that use them).
#pragma once

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
	virtual void Update() {}         // 0x749511
	virtual void Draw(Graphics* g) {}  // 0x74956f (the MatchShockwave's, 0x749693, draws nothing too)
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

}  // namespace Sexy
