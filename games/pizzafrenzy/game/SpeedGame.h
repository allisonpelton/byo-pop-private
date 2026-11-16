#ifndef __PIZZAFRENZY_SPEEDGAME_H__
#define __PIZZAFRENZY_SPEEDGAME_H__

// The speed mode's game (0x120 bytes, constructor 0x459510, vtable 0x500d38, 47 slots; the application makes it for
// mode 0). The slots it shares with the concentration mode's game are GameMode's (GameMode.h); the ones of its own
// are overridden here as they are translated.

#include "GameMode.h"

namespace pizzafrenzy
{

class SpeedGame : public GameMode
{
public:
	SpeedGame() {}  // 0x459510

	// 0x04: 0x459950: the screen, the map, the hud and the stats it plays on; the node "titles", the voice of the
	// slowed time, and the right button
	void setup(GameScreen* theScreen, CityMap* theMap, Hud* theHud, GameStats* theStats) override;
};

} // namespace pizzafrenzy

#endif
