// BonusDropper: a bonus that the monkey holds and puts down on the board with the first button (the fly paper, 0x40c700,
// and the mine, 0x40d7c0). The original has no class between them and the base: the two share the one Place (0x40db20,
// which lies in the mine's range) and keep the same two tracks in the same two fields.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class BonusDropper : public Bonus
{
public:
	int mHeldTrack;          // +0x2d8: the track the bonus rests in while it is held
	int mDropTrack;          // +0x2dc: the track it takes when it is put down

	bool Place() override;   // 0x40db20: the bonus is put down where it is, at the angle it was turned to
};

} // namespace Sexy
