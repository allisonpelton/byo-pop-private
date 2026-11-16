#include "BonusBubbleFreeLife.h"
#include "FishFood.h"
#include "GameSession.h"

namespace feedingfrenzy
{

// 0x419a60
BonusBubbleFreeLife::BonusBubbleFreeLife()
{
	mPngFrames = true;
}

// 0x419a30
void BonusBubbleFreeLife::onEnterStage()
{
	if (FishFood::instance()->getSession()->getMode() == GAME_MODE_TIMED)
		takeOutOfPlay();
}

} // namespace feedingfrenzy
