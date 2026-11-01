// Sexy::Piece (0x53c bytes, vtable 0x7e99e0 with 4 slots; 0x728c19-0x72fc8e): a gem on the board, with its kind
// (the flags at +0x84), its colour, its place and the effects it draws. Fields still unnamed are named by their
// offset in the original's object.
#pragma once

#include "Color.h"
#include "ColorCycle.h"
#include "CurvedVal.h"
#include "Rect.h"

#include <vector>

namespace Sexy {

class Buffer;
class Effect;
class Graphics;
class Image;
class MemoryImage;

class Piece {
public:
	// The bits of mFlags (0x728c4b sets one, 0x728c91 reads one)
	enum {
		FLAG_FRUIT = 0,          // a fruit (IMAGE_FRUITS)
		FLAG_1 = 1,              // kept when a gem turns into a special gem
		FLAG_LOCKED = 2,         // in a gem lock (IMAGE_GEMLOCK_*)
		FLAG_ELECTRO = 3,        // an electro gem (0x729774)
		FLAG_POWER = 4,          // a power gem, which blasts (0x7298ec, 0x729363)
		FLAG_6 = 6,              // (0x729995)
		FLAG_7 = 7,              // (0x729893)
		FLAG_DEVIL = 8,          // a devil gem (IMAGE_DEVILGEM)
		FLAG_BOMB = 9,           // a bomb with a counter (IMAGE_BOMBGEMS)
		FLAG_COAL = 10,          // coal (IMAGE_COAL, IMAGE_SNACKERS)
		FLAG_12 = 12,
	};

	// 0x5c bytes: an electro gem's arc (the vector at +0x40), drawn by DrawFront
	struct Arc {
		float mStartAngle;     // +0x0: where it leaves the gem's outline
		float mStartSpeed;     // +0x4: added to mStartAngle each update
		float mEndAngle;       // +0x8: where it meets the outline again (or the other piece's)
		float mEndSpeed;       // +0xc
		bool mBright;          // +0x10: drawn straight across in the bright colours
		bool mToPiece;         // +0x11: ends on the piece mTargetId
		int mTargetId;         // +0x14
		float mOffsets[8];     // +0x18: each segment's offset from the outline
		float mSpeeds[8];      // +0x38: how fast each offset changes
		int mSegments;         // +0x58: the segments less one
	};
	// 0x1c bytes: the records of the vector at +0x214 (not yet known)
	struct Record1C { int mData[7]; };

	int mUnknown4 = 0;                 // +0x4
	int mColor = 0;                    // +0x8: 0..6 the gems' colours (power gems keep theirs modulo 7)
	float mX = 0;                      // +0xc
	float mY = 0;                      // +0x10
	float mUnknown14 = 0;              // +0x14
	int mUnknown18 = 0;                // +0x18
	bool mUnknown1C = true;            // +0x1c
	int mCol = 0;                      // +0x20
	int mUnknown24 = -1;               // +0x24
	int mRow = 0;                      // +0x28
	int mUnknown2C = -1;               // +0x2c
	int mUnknown30 = -1;               // +0x30
	int mUnknown34 = 0;                // +0x34
	bool mUnknown38 = false;           // +0x38
	float mGlow = 0;                   // +0x3c
	std::vector<Arc> mArcs;       // +0x40
	float mUnknown50[8] = {};          // +0x50: summed for the shine (slot 3)
	float mUnknown70 = 0;              // +0x70
	float mShakeX = 0;                 // +0x74
	float mShakeY = 0;                 // +0x78
	float mOffsetX = 0;                // +0x7c
	float mOffsetY = 0;                // +0x80
	unsigned int mFlags = 0;           // +0x84
	int mUnknown88 = 0;                // +0x88
	int mLockCount = 0;                // +0x8c: the gem lock's strength (saved)
	int mUnknown90 = 0;                // +0x90
	int mUnknown94 = 0;                // +0x94
	int mUnknown98 = 0;                // +0x98
	bool mUnknownB0 = false;           // +0xb0
	bool mUnknownB1 = false;           // +0xb1
	double mUnknownB8 = 0;             // +0xb8
	float mUnknownC0 = 0;                // +0xc0
	bool mBreaking = false;            // +0xc4: coal or a gem breaking up (saved)
	int mBreakFrame = 0;               // +0xc8: the frame of the break-up
	int mUnknownCC = 0;                // +0xcc: a breaking gem's updates; every third is a frame
	float mUnknownD0 = 0;              // +0xd0 (saved as a whole number)
	float mUnknownD4 = 0;              // +0xd4 (saved as a whole number)
	float mUnknownD8 = 0;              // +0xd8
	float mUnknownDC = 0;              // +0xdc
	float mUnknownE0 = 0;              // +0xe0
	float mUnknownE4 = -1.0f;          // +0xe4
	float mUnknownE8 = 0;              // +0xe8
	bool mUnknownEC = false;           // +0xec
	float mUnknownF0 = 0;              // +0xf0: rises to 1 while mUnknownEC is set, after mUnknownF4 updates
	int mUnknownF4 = 0;                // +0xf4
	bool mUnknownF8 = false;           // +0xf8
	float mUnknownFC = 0;                // +0xfc
	int mBombCount = 0;                // +0x100: a bomb's counter (saved)
	int mUnknown104 = 0;               // +0x104
	int mUnknown108 = 0;               // +0x108
	int mUnknown110 = 0;               // +0x110
	bool mUnknown114 = false;          // +0x114: moved down by DropPieces
	bool mBlasted = false;             // +0x115: a power gem that has gone off (0x729363)
	bool mUnknown116 = false;          // +0x116 (saved)
	int mUnknown118 = 0;               // +0x118
	int mUnknown11C = 0;               // +0x11c
	float mFruitGlow = 0;              // +0x120
	float mUnknown124 = 0;               // +0x124
	MemoryImage* mCountImage = nullptr;   // +0x128: a bomb's counter (MakeCountImage)
	MemoryImage* mCountImage2 = nullptr;  // +0x12c: the same with a red outline
	CurvedVal mCountPulse;             // +0x130: the counter's pulse below 9
	CurvedVal mCurve1A0;               // +0x1a0
	float mUnknown210 = 0;             // +0x210
	std::vector<Record1C> mRecords214; // +0x214
	int mUnknown224[8] = {};           // +0x224
	int mUnknown244[8] = {};           // +0x244
	float mUnknown284 = 0;             // +0x284
	float mScale288 = 1.0f;            // +0x288: below 1 while a fruit grows
	int mOrigColor = 0;                // +0x28c (saved)
	float mUnknown290 = 0;             // +0x290
	float mUnknown294 = 0;             // +0x294
	float mUnknown298 = 0;             // +0x298
	float mUnknown29C = 0;             // +0x29c
	float mUnknown2A0 = 0;             // +0x2a0
	float mDrawScale = 1.0f;           // +0x2a4
	float mUnknown2A8 = 0;             // +0x2a8
	ColorCycle mColorCycle;            // +0x2ac
	bool mUnknown2E0 = false;          // +0x2e0 (saved)
	float mDevilGlow = 0;              // +0x2e4: a devil gem's glow (DrawBack); for the others their shockwave (DrawFront)
	int mJustMade = 0;                 // +0x2e8: 2 when a special gem was just made
	int mUnknown2EC = -1;              // +0x2ec
	int mUnknown2F0 = 0;               // +0x2f0
	bool mUnknown2F4 = false;          // +0x2f4
	bool mUnknown2F5 = false;          // +0x2f5
	bool mUnknown2F6 = true;           // +0x2f6
	float mUnknown2F8 = 0;             // +0x2f8
	float mUnknown2FC = 0;             // +0x2fc
	int mUnknown300 = 0;               // +0x300
	float mUnknown304[4] = {};           // +0x304
	int mUnknown314[4] = {};           // +0x314
	Color mFadeColors[4];              // +0x324: each step 12 a draw towards mFadeTargets (slot 1)
	Color mFadeTargets[4];             // +0x364
	int mUnknown3A4[4] = {-1, -1, -1, -1};  // +0x3a4
	int mUnknown3B4 = 1;               // +0x3b4
	float mUnknown3B8[4][2] = {};      // +0x3b8
	float mHaloAngles[4] = {};         // +0x3d8: random, 0 to 6.28
	float mUnknown3E8[64] = {};          // +0x3e8
	float mUnknown4E8 = 0;             // +0x4e8: RandSigned() * 2
	int mUnknown4EC = -1;              // +0x4ec
	int mUpdateCnt = 0;                // +0x4f0
	float mGlow2 = 0;                  // +0x4f4
	int mUnknown4F8 = 2;               // +0x4f8: 2 (8 with Direct3D)
	int mUnknown4FC = 0;               // +0x4fc (saved)
	float mLockFlash = 0;              // +0x500: falls by 0.027 an update
	int mLockTick = 0;                 // +0x504
	int mLockShake = 0;                // +0x508
	float mPendingDX = 0;              // +0x50c: added to mX while mPendingTime runs
	float mPendingDY = 0;              // +0x510: added to mY
	float mPendingTime = 0;            // +0x514: when mPendingKind turns the gem
	int mPendingKind = 0;              // +0x518: the flag (as a bit) the gem becomes once mPendingTime is up
	bool mUnknown51C = false;          // +0x51c
	bool mUnknown51D = false;          // +0x51d (saved)
	bool mCanMove = true;              // +0x51e (saved)
	int mId = 0;                       // +0x520: from a running count, the board's map of pieces
	bool mUnknown524 = false;          // +0x524
	void* mUnknown528 = nullptr;       // +0x528: an object with a vtable, dropped once its slot 9 says false
	bool mDimmed = false;              // +0x52c: drawn at half brightness
	float mAlpha = 1.0f;               // +0x530
	bool mUnknown534 = false;          // +0x534
	bool mUnknown535 = true;           // +0x535
	bool mUnknown536 = true;           // +0x536
	bool mUnknown537 = false;          // +0x537
	int mUnknown538 = 0;               // +0x538

	Piece();                                   // 0x72c1fa
	virtual ~Piece();                          // 0x72ba2a
	virtual void Update();                     // 0x72c67b (slot 0)
	virtual void Draw(Graphics* g, bool theForce);  // 0x729b41 (slot 1)
	virtual void DrawBack(Graphics* g);        // 0x72af5c (slot 2)
	virtual void DrawFront(Graphics* g);       // 0x72fc8e (slot 3)

	void SetFlag(int theFlag, bool on);        // 0x728c4b
	bool IsFlagSet(int theFlag) const { return (mFlags & (1u << theFlag)) != 0; }  // 0x728c91
	void ClearFlag(int theFlag) { SetFlag(theFlag, false); }  // 0x728cb0
	void ClearSpecial();                       // 0x728cc1
	bool CanMove() const;                      // 0x728cf9
	void SetColor(int theColor);               // 0x729563
	bool IsSpecial();                          // 0x729a0a: a power, electro, fruit or type 6 or 7 gem
	// 0x728d15: a spark flying out at theAngle (theBig: a bigger one, black or red, that the caller adds); none
	// while the board blocks sparks or for a devil gem
	Effect* AddSpark(bool theBig, float theSpeed, float theAngle);
	void AddShards(float theSpeed);            // 0x728f69: a shard of the gem (effect 10)
	void AddDust();                            // 0x7290db: dust (effect 12)
	// 0x729237: where a bomb's counter is drawn, theRise from 0 (low) to 1
	void GetCounterPos(int* theX, int* theY, float theRise);
	float GetRadius(float theAngle);           // 0x7292a1: the gem's outline at theAngle, from the exe's table
	void Load(Buffer* theBuffer);              // 0x72957f
	void Save(Buffer* theBuffer);              // 0x72969e
	// 0x729774, 0x729893, 0x7298ec, 0x729995: the gem becomes an electro, type 7, power or type 6 gem, at once
	// or theDelay later
	void MakeElectro(float theDelay);
	void Make7(float theDelay);
	void MakePower(float theDelay);
	void Make6(float theDelay);
	void MakeCountImage();                     // 0x72bab1: draws mBombCount into mCountImage and mCountImage2

private:
	// Parts of Update (0x72c67b), split off in the port
	void UpdatePowerSparks();
	void UpdateDevilSmoke();
	void UpdateElectro();
	void AddBreakDust(Effect* theEffect);
};

}
