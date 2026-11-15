#ifndef __PIZZAFRENZY_GAMESTATS_H__
#define __PIZZAFRENZY_GAMESTATS_H__

// What the game that is being played has counted so far (Iggle Pop's GameStats, 0x78 bytes, constructor 0x41a930; the
// application keeps it at +0x120): the score and the time of the game, the level of each topping the player owns and
// the counters the modes keep. A game starts with it reset (0x41a7b0), or resumed from the mode's saved game
// (0x41a900). The modes and the screens that show it are translated next; the fields below are the ones the reset
// and the loading state reach, and the counters it zeroes are named for their place.

#include "Object.h"
#include "TileEntries.h"
#include "UserGameData.h"

#include <map>

namespace pizzafrenzy
{

class GameStats : public engine::Object
{
public:
	GameStats();  // 0x41a930

	// 0x41a7b0: the counters start over, and the toppings the player has are taken from the player's data
	void reset();
	// 0x41a900: reset, then the score and the time the mode's saved game had
	void resume(UserGameData* theUserData, int theMode);

	int mScore;                                  // +0x0c
	float mElapsedTime;                          // +0x10
	std::map<Topping*, int> mToppingLevels;      // +0x14: the level of each topping the player has
	int mCountA;                                 // +0x20: nothing translated reads it
	int mCountB;                                 // +0x24: nothing translated reads it
	int mCountC;                                 // +0x28: starts at 50; nothing translated reads it
	int mCountD;                                 // +0x2c: nothing translated reads it
	int mCountE;                                 // +0x34: nothing translated reads it
	int mCountF;                                 // +0x38: nothing translated reads it
	int mCountG;                                 // +0x3c: nothing translated reads it
	int mCountH;                                 // +0x40: nothing translated reads it
	std::map<int, int> mTallies;                 // +0x44: nothing translated reads it
	int mCountI;                                 // +0x50: nothing translated reads it
	int mCountJ;                                 // +0x54: nothing translated reads it
	int mCountK;                                 // +0x58: nothing translated reads it
	int mCountL;                                 // +0x5c: nothing translated reads it
	int mCountM;                                 // +0x60: nothing translated reads it
	int mCountN;                                 // +0x64: nothing translated reads it
	int mCountO;                                 // +0x68: nothing translated reads it
	bool mStarted;                               // +0x6c: set by the reset; nothing translated reads it

private:
	// 0x41a6a0: the pizzas the player designed count as toppings, with the level the player has of each (0 for none)
	void addDesignedPizzas();
};

} // namespace pizzafrenzy

#endif
