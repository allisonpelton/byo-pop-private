// Sexy::Piece (0x53c bytes, vtable 0x7e99e0 with 4 slots; 0x728c19-0x72fc8e): a gem on the board, with its kind
// (the flags at +0x84), its colour, its place and the effects it draws.
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
		FLAG_DROPPER = 1,        // sinks a row at a time (DropFlaggedGems); kept when it turns special
		FLAG_LOCKED = 2,         // in a gem lock (IMAGE_GEMLOCK_*)
		FLAG_ELECTRO = 3,        // an electro gem (0x729774)
		FLAG_POWER = 4,          // a power gem, which blasts (0x7298ec, 0x729363)
		FLAG_FIXED = 5,          // the twist cannot move it (IsTwistable); nothing sets it
		FLAG_ICE = 6,            // an ice gem, which shatters (0x729995)
		FLAG_ANGEL = 7,          // an angel gem (0x729893)
		FLAG_DEVIL = 8,          // a devil gem (IMAGE_DEVILGEM)
		FLAG_BOMB = 9,           // a bomb with a counter (IMAGE_BOMBGEMS)
		FLAG_COAL = 10,          // coal (IMAGE_COAL, IMAGE_SNACKERS)
		FLAG_TUTORIAL_MARK = 11, // set on a tutorial script's gems of kind 11; nothing reads it
		FLAG_DOOM = 12,          // a doom gem: a devil gem under a lock (IMAGE_DOOMLOCK)
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

	int mVtableLink = 0;               // +0x4: never set or read (the vtable neighbour in the original)
	int mColor = 0;                    // +0x8: 0..6 the gems' colours (power gems keep theirs modulo 7)
	float mX = 0;                      // +0xc
	float mY = 0;                      // +0x10
	float mTurnAmount = 0;             // +0x14: zero unless the gem is mid-turn; a twist's stop clears it, nothing raises it
	int mFallDelay = 0;                // +0x18: never set or read
	bool mIsSettled = true;            // +0x1c: true; the board is not done while a gem has it cleared
	int mCol = 0;                      // +0x20
	int mLastCol = -1;                 // +0x24: the column the gem was settled in (a twist and a settled drop set it)
	int mRow = 0;                      // +0x28
	int mLastRow = -1;                 // +0x2c: the row the gem was settled in; a match checks it against mRow
	int mBurstRow = -1;                // +0x30: the row a burst is drawn at, with mLastCol
	float mFallSpeed = 0;              // +0x34: the fall's speed
	bool mHasPulsingLight = false;     // +0x38: the gem throws a pulsing light by its glow
	float mGlow = 0;                   // +0x3c
	std::vector<Arc> mArcs;       // +0x40
	float mShineSlots[8] = {};         // +0x50: summed for the shine (slot 3)
	float mShineGlow = 0;              // +0x70: the pulsing light's strength, used by the shine
	float mShakeX = 0;                 // +0x74
	float mShakeY = 0;                 // +0x78
	float mOffsetX = 0;                // +0x7c
	float mOffsetY = 0;                // +0x80
	unsigned int mFlags = 0;           // +0x84
	int mBusyTime = 0;                 // +0x88: while above 0 the gem takes no part in matching (1000 while a special gem is made)
	int mLockCount = 0;                // +0x8c: the gem lock's strength (saved)
	int mTwistMoveX = 0;               // +0x90: how far the twist moves it across, in pixels
	int mTwistMoveY = 0;               // +0x94: how far the twist moves it down, in pixels
	int mTwistDCol = 0;                // +0x98
	int mTwistDRow = 0;                // +0x9c: +0x98 and +0x9c, the column and row the twist moves it by
	int mTwistStartX = 0;              // +0xa0: where the twist started it (x, y)
	int mTwistStartY = 0;              // +0xa4: where the twist started it (y)
	int mTwistOffsetX = 0;             // +0xa8: and from the twist's centre
	int mTwistOffsetY = 0;             // +0xac: and y; its place from the twist's centre
	bool mIsTwisting = false;          // +0xb0: the gem is turned by the twist in progress
	bool mIsShining = false;           // +0xb1: the shine runs on the gem
	double mShineFade = 0;             // +0xb8: the shine's strength, up by 0.045 while the gem is special, down by 0.012 otherwise
	float mShinePos = 0;                 // +0xc0: the shine's place along the gem, 0 to 10
	bool mBreaking = false;            // +0xc4: coal or a gem breaking up (saved)
	int mBreakFrame = 0;               // +0xc8: the frame of the break-up
	int mBreakTick = 0;                // +0xcc: a breaking gem's updates; every third is a frame
	float mCounterFlash = 0;           // +0xd0 (saved as a whole number): a bomb's counter flash, 1 when the count is about to go down
	float mCounterSparks = 0;          // +0xd4 (saved as a whole number): the strength of the sparks that rise from a bomb's counter
	float mBreakShakeX = 0;            // +0xd8: never set or read
	float mBreakShakeY = 0;            // +0xdc: never set or read
	int mFruitTimer = 0;               // +0xe0: 120 for a new fruit (MakeFruitGem), 0 when a twist starts
	float mRingAngle = -1.0f;          // +0xe4: the turn of the ring of sparks round the gem, -1 when off
	float mRingDelay = 0;              // +0xe8: the wait before the ring starts
	bool mHasSparkle = false;          // +0xec: a sparkle fades in on the gem after mSparkleDelay
	float mSparkleAlpha = 0;           // +0xf0: rises to 1 while mHasSparkle is set, after mSparkleDelay updates
	int mSparkleDelay = 0;             // +0xf4: updates before the sparkle starts
	bool mIsTwistCorner = false;       // +0xf8: one of the four gems half a gem from the twist's centre
	float mFlashLevel = 0;               // +0xfc: a flash that runs down by 0.005 an update (nothing sets it)
	int mBombCount = 0;                // +0x100: a bomb's counter (saved)
	int mBombLimit = 0;                // +0x104: never set or read
	int mBombPhase = 0;                // +0x108: a bomb's ticking phase, down from 255 each update
	int mMatchCount = 0;               // +0x10c: the matches the gem is in (FindMatches)
	int mBreakDelay = 0;               // +0x110: updates before the gem breaks (15 for each match it comes after)
	bool mHasDropped = false;          // +0x114: moved down by DropPieces
	bool mBlasted = false;             // +0x115: a power gem that has gone off (0x729363)
	bool mFruitCounted = false;        // +0x116 (saved): a fruit gem the matches have counted
	int mFlameLevel = 0;               // +0x118: a devil gem's flames while it is made (its scale times 8)
	int mFruitGlowStep = 0;            // +0x11c: counts the glow pulses of a fruit gem (0 to 15)
	float mFruitGlow = 0;              // +0x120
	float mCoalTimer = 0;                // +0x124: the wait before a coal's next ember
	MemoryImage* mCountImage = nullptr;   // +0x128: a bomb's counter (MakeCountImage)
	MemoryImage* mCountImage2 = nullptr;  // +0x12c: the same with a red outline
	CurvedVal mCountPulse;             // +0x130: the counter's pulse below 9
	CurvedVal mCountSizeCurve;         // +0x1a0: the bomb or devil counter's bump in size when its count changes
	float mHaloScale = 0;              // +0x210: never set or read
	int mTrailX[8] = {};               // +0x224: eight values, never set or read
	int mTrailY[8] = {};               // +0x244: eight values, never set or read
	float mGrowth = 0;                 // +0x284: never raised above 0 (the port does not port what it would do)
	float mBirthScale = 1.0f;            // +0x288: below 1 while a fruit grows
	int mOrigColor = 0;                // +0x28c (saved)
	float mAuraScaleX = 0;             // +0x290: never set or read
	float mAuraScaleY = 0;             // +0x294: never set or read
	float mAuraTime = 0;               // +0x298: runs down by 0.01 an update; the aura goes out when it is done
	float mAura = 0;                   // +0x29c: the strength of the gem's aura (follows the glow, up to 1)
	float mAuraAngle = 0;              // +0x2a0: the direction the aura shakes the gem in
	float mDrawScale = 1.0f;           // +0x2a4
	float mDrawTurn = 0;               // +0x2a8: never set or read
	ColorCycle mColorCycle;            // +0x2ac
	bool mFirstDropDone = false;       // +0x2e0 (saved): set once the gem has settled in a game that is not a puzzle
	float mDevilGlow = 0;              // +0x2e4: a devil gem's glow (DrawBack); for the others their shockwave (DrawFront)
	int mJustMade = 0;                 // +0x2e8: 2 when a special gem was just made
	int mMatchSerial = -1;             // +0x2ec: the serial of the match the gem is in, which its points go to
	int mBreakCause = 0;               // +0x2f0: -0x65 when an electro strike breaks it, -0x67 for coal
	bool mIsInMatch = false;           // +0x2f4: the gem is in a match this turn
	bool mHasPendingCount = false;     // +0x2f5: a bomb's count is waiting to be taken down
	bool mIsDrawn = true;              // +0x2f6: true; the gem is drawn
	float mCoalBurn = 0;               // +0x2f8: how far a coal has burnt, 0 to 1
	float mCoalBurnTarget = 0;         // +0x2fc: the burn the coal flickers towards
	int mFlameCount = 0;               // +0x300: never set or read
	float mFadeAngles[4] = {};           // +0x304: a random angle for each of the four fading colours
	int mFadeStarts[4] = {};           // +0x314: four values, never set or read
	Color mFadeColors[4];              // +0x324: each step 12 a draw towards mFadeTargets (slot 1)
	Color mFadeTargets[4];             // +0x364
	int mFadeSlots[4] = {-1, -1, -1, -1};   // +0x3a4: -1 or 0 for each fading colour, never read
	int mFadeSlotCount = 1;            // +0x3b4: 1, never read
	float mFadeOffsets[4][2] = {};     // +0x3b8: four pairs, never set or read
	float mHaloAngles[4] = {};         // +0x3d8: random, 0 to 6.28
	float mEmbers[64] = {};              // +0x3e8: a coal's embers, 32 for each cel
	float mEmberWait = 0;              // +0x4e8: the wait before the next ember (RandSigned() * 2 at first)
	int mBombTickSide = -1;            // +0x4ec: a random quarter for a bomb's tick, -1 when none
	int mUpdateCnt = 0;                // +0x4f0
	float mGlow2 = 0;                  // +0x4f4
	int mDrawDetail = 2;               // +0x4f8: 2 (8 with Direct3D)
	int mLockState = 0;                // +0x4fc (saved): 1 while the gem is locked
	float mLockFlash = 0;              // +0x500: falls by 0.027 an update
	int mLockTick = 0;                 // +0x504
	int mLockShake = 0;                // +0x508
	float mPendingDX = 0;              // +0x50c: added to mX while mPendingTime runs
	float mPendingDY = 0;              // +0x510: added to mY
	float mPendingTime = 0;            // +0x514: when mPendingKind turns the gem
	int mPendingKind = 0;              // +0x518: the flag (as a bit) the gem becomes once mPendingTime is up
	bool mIsKept = false;              // +0x51c
	bool mHasBroken = false;           // +0x51d (saved): the gem has been broken by a strike
	bool mCanMove = true;              // +0x51e (saved)
	int mId = 0;                       // +0x520: from a running count, the board's map of pieces
	bool mIsRescued = false;           // +0x524: never set or read
	void* mHelper = nullptr;           // +0x528: an object with a vtable, dropped once its slot 9 says false
	bool mDimmed = false;              // +0x52c: drawn at half brightness
	float mAlpha = 1.0f;               // +0x530
	bool mIsOff = false;               // +0x534: a flag the update reads to skip the gem
	bool mIsLive = true;               // +0x535: true; false while the gem is held out of play
	bool mIsCounted = true;            // +0x536: true; cleared once the gem is taken into a match
	bool mHasBurst = false;            // +0x537: the gem's burst has been made
	int mStrikeDelay = 0;              // +0x538: updates until an electro strike can break it again (0x10 after one)

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
	void Blast();                              // 0x729363: a power gem goes off
	void BreakLock(bool theType);              // 0x72952f: the gem lock breaks (BreakSpecial, stat 0x48)
	bool CanMove() const;                      // 0x728cf9
	void SetColor(int theColor);               // 0x729563
	bool IsSpecial();                          // 0x729a0a: a power, electro, fruit, ice or angel gem
	// 0x728d15: a spark flying out at theAngle (theBig: a bigger one, black or red, that the caller adds); none
	// while the board blocks sparks or for a devil gem
	Effect* AddSpark(bool theBig, float theSpeed, float theAngle);
	// 0x728f69: a shard of the gem (effect 10), into the board's second manager (+0x164c) if theSecondManager
	void AddShards(float theSpeed, bool theSecondManager);
	// 0x729237: where a bomb's counter is drawn, theRise from 0 (low) to 1
	void GetCounterPos(int* theX, int* theY, float theRise);
	float GetRadius(float theAngle);           // 0x7292a1: the gem's outline at theAngle, from the exe's table
	void Load(Buffer* theBuffer);              // 0x72957f
	void Save(Buffer* theBuffer);              // 0x72969e
	// 0x729774, 0x729893, 0x7298ec, 0x729995: the gem becomes an electro, angel, power or ice gem, at once
	// or theDelay later
	void MakeElectro(float theDelay);
	void MakeAngel(float theDelay);
	void MakePower(float theDelay);
	void MakeIce(float theDelay);
	void MakeCountImage();                     // 0x72bab1: draws mBombCount into mCountImage and mCountImage2

private:
	// Parts of Update (0x72c67b), split off in the port
	void UpdatePowerSparks();
	void UpdateDevilSmoke();
	void UpdateElectro();
	void AddBreakDust(Effect* theEffect);
};


extern int gNextPieceId;  // 0x84f674: the next piece's id (-1 stops the count; saved with the game)
}
