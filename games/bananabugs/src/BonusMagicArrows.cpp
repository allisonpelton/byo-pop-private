#include "BonusMagicArrows.h"

#include "Player.h"

#include <cstring>

using namespace Sexy;

// The rope mode of the magic arrows (RopeObj reads it)
const int ROPE_MODE_MAGIC_ARROWS = 2;

// 0x40d740: the kind is given by Setup
BonusMagicArrows::BonusMagicArrows()
{
}

// 0x40d780
void BonusMagicArrows::Start()
{
	strcpy(mName, "MAGIC ARROWS");
	Bonus::Start();
	Player::SetRopeMode(ROPE_MODE_MAGIC_ARROWS);
}
