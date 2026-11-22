// BonusMagicArrows: the magic arrows (0x40d740..0x40d7c0 in the original, 0x2d8 bytes: the base's size). When it is won
// ("MAGIC ARROWS") the monkey's rope shoots the second kind of arrows (rope mode 2) until the bonus is put away.
#pragma once

#include "BonusRopeMode.h"

namespace Sexy
{

class BonusMagicArrows : public BonusRopeMode
{
public:
	BonusMagicArrows();                       // 0x40d740 (the base's constructor, then the vtable)
	void Start() override;                    // 0x40d780
};

} // namespace Sexy
