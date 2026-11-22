#include "BonusRopeMode.h"

#include "Player.h"

using namespace Sexy;

// The rope mode that shoots as it did (the monkey's own)
const int ROPE_MODE_NORMAL = 0;

// 0x410df0
void BonusRopeMode::Reset()
{
	Bonus::Reset();
	Player::SetRopeMode(ROPE_MODE_NORMAL);
}

// 0x410e00
void BonusRopeMode::End()
{
	Reset();
}
