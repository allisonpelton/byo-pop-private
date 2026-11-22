// BonusTarzan: the monkey attack (0x411c60..0x4123e0 in the original, 0x32c bytes). When it is won ("MONKEY ATTACK") the
// bugs are slowed and three monkeys swing in on vines from above, one every 75 frames, each over a bug picked at random
// from those high on the screen (or over a random place along the top when none is left to pick): a hand at the end of
// each monkey picks up the first bug that comes near it (the bug's picture is left on the hand and the bug is killed).
// Each swing is 330 frames; the bonus is put away when the second swing has ended.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class Anim;
class BugObj;

const int TARZAN_MONKEYS = 3;

class BonusTarzan : public Bonus
{
public:
	ObjBase* mMonkeys[TARZAN_MONKEYS]; // +0x2d8: the swinging monkeys, children of the level's object layer, each with one swing track
	ObjBase* mHands[TARZAN_MONKEYS];   // +0x2e4: each monkey's hand (a child of the monkey): it takes the picture of the bug it picks up
	BugObj* mTargets[TARZAN_MONKEYS];  // +0x2f0: the bugs the monkeys were sent over (a bug is not picked twice)
	bool mCaught[TARZAN_MONKEYS];      // +0x2fc: the hand has picked up a bug
	int mTimers[TARZAN_MONKEYS];       // +0x300: frames until the monkey's yell plays (80 from its being sent); -1 for none
	int mSounds[TARZAN_MONKEYS];       // +0x30c: the yell of each (the sounds 84 to 86, by the monkey's number)
	int mTargetCount;                  // +0x318: how many of mTargets are filled
	bool mEnded;                       // +0x31c: End has been called
	int mMonkeysOut;                  // +0x320: the monkeys sent (Start clears it, SendMonkey counts it up), counted down as each swing ends
	int mDelay;                        // +0x324: frames until the next monkey is sent (75 after each)
	bool mSending;                     // +0x328: monkeys are still being sent

	static Anim* sTarzanAnim;          // 0x60e1d0: "images/bonus_tarzan"

	BonusTarzan();                            // 0x411c60 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x411c70
	bool Update() override;                   // 0x411cd0
	void Notify(ObjBase* theObj) override;    // 0x411de0
	void Reset() override;                    // 0x411e40
	void End() override;                      // 0x411eb0
	void Start() override;                    // 0x411ee0

private:
	bool SendMonkey();                        // 0x411fa0 (ESI the bonus): sends the next monkey; false when there is none to send
	void BuildMonkeys();                      // 0x412140
};

} // namespace Sexy
