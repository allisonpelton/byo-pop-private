// Bonus: the base of the nineteen bonuses a level may give (0x4066e0..0x406dd0 in the original), and the manager of
// the one that is running. A bonus is an invisible object that keeps the state of its bonus (it is held by the monkey,
// then placed or started, then runs and ends), shows a message when it starts and carries the objects it needs. The
// monkey holds one (Player::mHeld), so this is also the held thing of the player.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Graphics;

// The bonus kinds, as the original numbers them (the class of each is named after the bonus; 13 has none)
enum BonusKind
{
	BONUS_SHOT_SPEED = 0,    // BonusShotSpeed 0x410ca0
	BONUS_BOMB = 1,          // BonusBomb 0x4080c0: "CLICK TO DROP COCOBOMB"
	BONUS_FLY_PAPER = 2,     // BonusFlyPaper 0x40c700
	BONUS_ZAPPER = 3,        // BonusZapper 0x4123e0
	BONUS_SWAT = 4,          // BonusSwat 0x410e10
	BONUS_SPEAR_CRAFT = 5,   // BonusSpearCraft 0x410d60
	BONUS_MAGIC_ARROWS = 6,  // BonusMagicArrows 0x40d740
	BONUS_MISSILE = 7,       // BonusMissile 0x40e550
	BONUS_CANNON = 8,        // BonusCannon 0x407170: "CLICK TO FIRE CANNON"
	BONUS_RAIN = 9,          // BonusRain 0x410660
	BONUS_HURRICANE = 10,    // BonusHurricane 0x40d090
	BONUS_EARTHQUAKE = 11,   // BonusEarthquake 0x40b7c0
	BONUS_PARROT = 12,       // BonusParrot 0x40eff0
	BONUS_TARZAN = 14,       // BonusTarzan 0x411c60
	BONUS_DISCO = 15,        // BonusDisco 0x40b0d0
	BONUS_PEA_SHOOTERS = 16, // BonusAttack 0x406fb0: "PEA SHOOTERS"
	BONUS_ZUMA = 17,         // BonusZuma 0x412f20
	BONUS_MINE = 18,         // BonusMine 0x40d7c0
	BONUS_KINDS = 19         // the table's places (13 stays empty)
};

// What a bonus is doing (Bonus::mState; the names are guesses from what is done in each)
enum BonusState
{
	BONUS_IDLE = 0,
	BONUS_WAITING = 1,       // lies in wait, swaying (Update); End leaves it alone
	BONUS_HELD = 2,          // the monkey holds it
	BONUS_PLACED = 4,        // put down
	BONUS_ENDING = 5,        // has been ended and runs out
	BONUS_RUNNING = 6        // started (Start)
};

// The key events a bonus's tracks raise (AnimKey::mEventId), which Bonus::Notify answers
enum BonusEvent
{
	BONUS_EVENT_SEEN = 1,    // sets mKeyEventOneSeen
	BONUS_EVENT_END = 2      // the bonus is put away (Reset)
};

class Bonus : public ObjBase
{
public:
	bool mEndsWhenDone;      // +0x27c: kinds 0, 5 and 12: CancelIfDone ends it once it is running
	bool mHeldCall;          // +0x27d: set by BecomeHeld's argument; nothing in the first half reads it
	int mGridColumn;         // +0x280: the place of the grid BonusZuma's Update last found itself over; nothing reads it
	int mGridRow;            // +0x284
	int mState;              // +0x288: BonusState
	int mKind;               // +0x28c: BonusKind
	int mRotation;           // +0x290: how the held thing lies (RotateHeld's or FlipHeld's result); 1 to start with
	int mUnused294;          // +0x294: set by SetUnused294, which nothing calls
	bool mKeyEventOneSeen;   // +0x29c: set by the key event 1; cleared by Start
	char mName[35];          // +0x29d: the message shown when it starts
	int mUnused2c0;          // +0x2c0
	float mSwayDegrees;      // +0x2c4: the sway of a bonus waiting to be picked up (a guess at its meaning)
	int mUnused2c8[4];       // +0x2c8: the original's base is 0x2d8 bytes; nothing is known to use this

	static Bonus* sCurrent;  // 0x60e1ec: the bonus that is held or running
	static bool sRunning;    // 0x60e17d: set by Start, cleared by Reset and End; nothing reads it

	Bonus();                                  // 0x4066e0
	virtual ~Bonus();                         // 0x410670 (the scalar deleting destructor; the base's body 0x4067c0)
	bool Update() override;                   // 0x406870
	void Notify(ObjBase* theObj) override;    // 0x406950
	void Draw(Graphics* g) override;          // 0x406980

	virtual void Setup(int theKind);          // slot 14 (+0x38) 0x4067d0: the kind's emitters; whether it ends when done
	virtual void SetUnused294(int theValue);  // slot 15 (+0x3c) 0x4069b0
	virtual void Reset();                     // slot 16 (+0x40) 0x4069c0: put away
	virtual void End();                       // slot 17 (+0x44) 0x406aa0: the bonus is over
	virtual bool EndWhenBugsGone();           // slot 18 (+0x48) 0x406ac0: whether it ended with the last bug (no: the base)
	virtual void Start();                     // slot 19 (+0x4c) 0x406ad0: the bonus is won
	virtual bool Rotate();                    // slot 20 (+0x50) 0x406be0: turns the held thing (kind 7 turns it over)
	virtual bool Place();                     // slot 21 (+0x54) 0x406c20: puts it down; false when it may not go there

	bool EndOnLastBug();                      // 0x412570: End, and yes (the EndWhenBugsGone of the kinds 2, 3, 10, 12 and 17; it lies in BonusZapper's range)
	bool UpdateUpright();                     // 0x40c760: the Update of BonusBomb and BonusFlyPaper (it lies in the second half's range)
	void DropAndReset();                      // 0x412530: lets go of what is held, then the base's Reset (the Reset of the kinds 3 and 4, which lie in BonusZapper's range)
	bool PutDown(int theDropTrack);           // the body of Place of the droppers (0x40db20), the zapper (0x412670) and Zuma (0x413460) after the sound: the held bonus goes to the object layer where it is and starts its drop track
	void BecomeHeld(bool theFlag);            // 0x406c30 (AL): held, not turned, in the state 2

	static void Cancel();                     // 0x406cd0: lets go of what is held and ends the running bonus
	static void CancelIfDone();               // 0x406d90: ends a running bonus of the kinds that end when done
	static void OnAllBugsDead();              // 0x406d40: the last bug in play is gone
	static void ClearAll();                   // 0x406c60: Cancel, and the bonus under the ground is reset too (a guess at the name)
};

void PreloadBonusPictures(); // 0x406dd0

} // namespace Sexy
