// BonusSpearCraft: the flaming arrows (0x410d60..0x410e10 in the original, 0x2d8 bytes: the base's size). When it is
// won ("FLAMING ARROWS") the monkey's rope shoots the first kind of fire arrows (rope mode 1) until the bonus is put
// away. Its Reset and End are BonusRopeMode's (shared with the magic arrows).
#pragma once

#include "BonusRopeMode.h"

namespace Sexy
{

class BonusSpearCraft : public BonusRopeMode
{
public:
	BonusSpearCraft();                        // 0x410d60 (the base's constructor, then the vtable)
	void Start() override;                    // 0x410da0
};

} // namespace Sexy
