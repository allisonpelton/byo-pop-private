// BonusCoin: the bonus hidden under a cell of the ground (0x408ed0..0x40af90 in the original, 0x498 bytes). A level has
// one (LevelObj::mBonusCoin), made with the level. From the fourth level on it is placed under a cell (PlaceBonus); a
// glowing coin of the bonus's picture then shows through the cell's gap, a ring of 72 sparks lighting one after the
// other round it. When the cell is cut (FillObj's landed event, Uncover) the coin flies to where the bonus's kind sends
// it (the monkey, the score board's monkeys or off the screen, see InitBonusTable), and on its arrival the bonus it
// was showing is started (or, when the level has gone as far as it must, or no bug is left, a bunch of bananas is
// paid out instead). The coin makes one object of each of the bonuses the player may have (the profile's blocked
// kinds are left out) and shows them in turn, one every 864 frames, when there are two or more.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;
class Bonus;
class BonusEffect;
class FillObj;

const int COIN_RAYS = 72;           // the sparks of the ring, five degrees apart
const int COIN_BONUS_KINDS = 19;    // a coin makes one of each kind

// How a bonus's coin flies off when its cell is cut (the table InitBonusTable fills, by kind)
enum CoinFlight
{
	COIN_FLIES_TO_MONKEY = 0,       // to the monkey (the player); the bonus effect plays at the monkey
	COIN_FLIES_TO_SCORE_BOARD = 1,  // to the score board's monkeys (the pea shooters)
	COIN_FLIES_UP = 2,              // off the top of the screen, at the centre (the rain and the hurricane)
	COIN_FLIES_DOWN = 3,            // to the bottom, left of the centre (the earthquake)
	COIN_FLIES_UP_RIGHT = 4,        // off the top to the right (Tarzan)
	COIN_FLIES_UP_LEFT = 5          // off the top to the left (the disco)
};

// What the coin is doing (BonusCoin::mPhase)
enum CoinPhase
{
	COIN_UNDER_CELL = 0,            // hidden under the cell, showing its bonus
	COIN_UNCOVERED = 1              // the cell has been cut: flying, then the bonus starts
};

class BonusCoin : public ObjBase
{
public:
	int mColumn;                    // +0x27c: the cell it lies under
	int mRow;                       // +0x280
	FillObj* mCell;                 // +0x284: the cell that covers it (FillObj::SetBonus)
	bool mIdle;                     // +0x288: put away by ResetCurrent; Update then does nothing
	int mTick;                      // +0x28c: frames since the last spark was lit (12 each)
	int mRayIndex;                  // +0x290: the spark to light next
	ObjBase* mSwayObj;              // +0x294: the picture of the bonus, which sways
	float mTargetX;                 // +0x298: where the coin flies to (9999: nowhere)
	float mTargetY;                 // +0x29c
	ObjBase* mChase;                // +0x2a0: what the coin flies to, if an object (the monkey or the score board)
	float mAngle;                   // +0x2a4: the way to the target, which the coin's orbit follows
	int mPhase;                     // +0x2a8: CoinPhase
	int mCount;                     // +0x2ac: how many bonuses it holds
	int mCurrentIndex;              // +0x2b0: the one it shows (and will start)
	float mSwayDegrees;             // +0x2b4: the sway of the picture (as Bonus's)
	AnimKey* mFlyKey;               // +0x2b8: the fly track's first key, whose duration and end are set at the uncover
	AnimKey* mArriveKey;            // +0x2bc: the fly track's last key, whose end is event 3 (arrived)
	int mAppearTrack;               // +0x2c0: the picture's track: appear and sway
	int mFadeTrack;                 // +0x2c4: the picture's track: shrink and go (ends with event 4)
	int mWaitTrack;                 // +0x2c8: the coin's own track, started when a bonus has started
	int mFlyTrack;                  // +0x2cc: the coin's own track: fly, then event 3
	int mUnused2d0;                 // +0x2d0: nothing sets or reads it
	int mUnused2d4;                 // +0x2d4: set to 700 by Place; nothing reads it
	bool mFlyToPlayer;              // +0x2d8: the coin is flying to the monkey (the effect then plays on the player's)
	Bonus* mBonuses[COIN_BONUS_KINDS]; // +0x2dc: the bonuses it holds, children of the coin
	int mKinds[COIN_BONUS_KINDS];   // +0x328: their kinds (BonusKind), in the same order
	ObjBase* mRays[COIN_RAYS];      // +0x374: the ring of sparks
	BonusEffect* mEffect;           // +0x494: the effect shown when the coin arrives (at the monkey's own when flying to it)

	static BonusCoin* sCurrent;     // 0x60e1f4: the coin that has been placed
	static Anim* sBonusesAnim;      // 0x60e1f0: "images/bonuses", one cel for each kind
	static int sFlight[COIN_BONUS_KINDS]; // 0x60e1f8: how each kind's coin flies off (CoinFlight)

	BonusCoin();                              // 0x408ed0
	~BonusCoin() override;                    // 0x409470 (the scalar deleting destructor; the body 0x409490)
	void Notify(ObjBase* theObj) override;    // 0x4094b0 (slot 4)
	bool Update() override;                   // 0x4095f0 (slot 5)

	void Place(int theColumn, int theRow);    // 0x409970 (EAX the row, the column pushed): puts the bonus under that cell
	void Uncover();                           // 0x409bf0: the cell has been cut
	static void ResetCurrent();               // 0x40af00 (a guessed name): puts the coin that is placed away

private:
	void ResetRays();                         // 0x409b90 (a guessed name): the sparks are put back dark and a ring bursts
	void BuildTracks();                       // 0x409dd0
	void BuildSway();                         // 0x40a080
	void BuildRays();                         // 0x40a650
};

// 0x40af90: loads the bonuses' picture and fills the table of how each kind's coin flies off
void InitBonusTable();

} // namespace Sexy
