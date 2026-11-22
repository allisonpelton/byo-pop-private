// BonusAttack: the pea shooters (0x406fb0..0x407170 in the original, 0x3bc bytes). When it is won, every monkey on
// the score board that has no pea in the air and is dancing is told to shoot, and the bonus is hidden. The message
// shown is "PEA SHOOTERS". (A bonus of the player's second upgrade too: Player::UPGRADE_SECOND.)
#pragma once

#include "Bonus.h"

namespace Sexy
{

class BonusAttack : public Bonus
{
public:
	// +0x2d8..+0x39f and +0x3a8..+0x3bb are in the original's object and are never used
	bool mEnded;             // +0x3a0: End has been called
	int mCountdown;          // +0x3a4: frames until the bonus puts itself away; -99 for no countdown (nothing sets it to more)

	BonusAttack();                            // 0x406fb0 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x406fc0
	bool Update() override;                   // 0x406ff0
	void Notify(ObjBase* theObj) override;    // 0x407040
	void Reset() override;                    // 0x407060
	void End() override;                      // 0x407080
	void Start() override;                    // 0x4070b0
};

} // namespace Sexy
