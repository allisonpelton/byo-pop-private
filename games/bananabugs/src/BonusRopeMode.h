// BonusRopeMode: a bonus that changes how the rope shoots while it lasts (the magic arrows, 0x40d740, and the spear
// craft, 0x410d60). The original has no class between them and the base: the two share the one Reset and End
// (0x410df0 and 0x410e00, which lie in the spear craft's range). The rope mode is put back to 0 whenever it is put away.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class BonusRopeMode : public Bonus
{
public:
	void Reset() override;   // 0x410df0: put away, and the rope shoots as it did
	void End() override;     // 0x410e00: the same as Reset
};

} // namespace Sexy
