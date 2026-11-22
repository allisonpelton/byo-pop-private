#include "BonusSpearCraft.h"

#include "Player.h"

#include <cstring>

using namespace Sexy;

// The rope mode of the flaming arrows (RopeObj reads it)
const int ROPE_MODE_FLAMING_ARROWS = 1;

// 0x410d60: the kind is given by Setup
BonusSpearCraft::BonusSpearCraft()
{
}

// 0x410da0
void BonusSpearCraft::Start()
{
	strcpy(mName, "FLAMING ARROWS");
	Bonus::Start();
	Player::SetRopeMode(ROPE_MODE_FLAMING_ARROWS);
}
