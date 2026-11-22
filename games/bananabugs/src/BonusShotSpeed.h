// BonusShotSpeed: the speed shot (0x410ca0..0x410d60 in the original, 0x2d8 bytes: the base's size). When it is won
// ("SPEED SHOT") the monkey's shots come twice as fast (the speed scale is halved) until the bonus ends; it is put away
// as soon as it has started, and the scale is put back by End (the original's Reset, slot 16, is the base's).
#pragma once

#include "Bonus.h"

namespace Sexy
{

class BonusShotSpeed : public Bonus
{
public:
	BonusShotSpeed();                         // 0x410ca0 (the base's constructor, then the vtable)
	void End() override;                      // 0x410d40: put away, and the shots as they were
	void Start() override;                    // 0x410ce0
};

} // namespace Sexy
