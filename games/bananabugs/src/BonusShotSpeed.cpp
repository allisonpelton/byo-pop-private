#include "BonusShotSpeed.h"

#include "Player.h"

#include <cstring>

using namespace Sexy;

// The speed scale of the shots (the monkey's own is 1; the smaller, the quicker)
const float SHOT_SPEED_FAST = 0.5f;
const float SHOT_SPEED_NORMAL = 1.0f;

// 0x410ca0: the kind is given by Setup
BonusShotSpeed::BonusShotSpeed()
{
}

// 0x410ce0: the scale is set and the bonus is put away at once (the base's Reset, not the virtual one: it ends nothing)
void BonusShotSpeed::Start()
{
	strcpy(mName, "SPEED SHOT");
	Bonus::Start();
	Player::SetSpeedScale(SHOT_SPEED_FAST);
	Bonus::Reset();
}

// 0x410d40
void BonusShotSpeed::End()
{
	Bonus::Reset();
	Player::SetSpeedScale(SHOT_SPEED_NORMAL);
}
