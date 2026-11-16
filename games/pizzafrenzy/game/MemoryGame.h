#ifndef __PIZZAFRENZY_MEMORYGAME_H__
#define __PIZZAFRENZY_MEMORYGAME_H__

// The memory mode's game (0x120 bytes, constructor 0x460610, vtable 0x5011a0, 47 slots; the application makes it for
// mode 1, "Memory Mode"). It is the speed mode's game with its own rules for who is served: the customers must be
// served in the order they came, the first customer first. A pizza for any other customer (or for a topping the picked
// orders did not ask for) sends every customer away. Its slots 0x4 (0x459950), 0x8, 0xc, 0xa8 and the rest are the
// speed mode's; the slots 0xac (0x4612c0) and 0xb4 (0x461560) are copies of the speed mode's 0x45a9a0 and 0x459b90 and
// do the same, so they are not overridden here.

#include "SpeedGame.h"

namespace pizzafrenzy
{

class MemoryGame : public SpeedGame
{
public:
	MemoryGame() {}  // 0x460610
	// 0x00: 0x460670 (the destructor, which the base's is)

	// 0x1c: 0x4606a0: the introduction is over: play starts, and the hud shows no coupon buttons
	void startPlay() override;
	// 0x38: 0x460710: the speed mode's waves, but the next wave comes when every house is idle, and the customers come
	// at least 0.35 seconds apart
	void updateWaves(engine::UpdateInfo& theInfo) override;
	// 0x44: 0x461030: a kitchen clicked with orders picked that ask for another topping sends every customer away
	void click(BuildingTile* theTile) override;
	// 0x4c: 0x460c40: the customer's order must be the oldest one; the pizzas are paid at ten (fifteen for a topping
	// that uses its picture as the preview) with no upgrade, and no frenzy or combo's bonus
	void deliver(BuildingTile* theKitchen, CustomerTile* theTile, bool theSendVan) override;
	// 0x50: 0x4611a0: an event's customer who is not the oldest sends every customer away
	void startSpecialEvent(BuildingTile* theKitchen, CustomerTile* theTile) override;
	// 0x90: 0x4609f0: the customer arrested must be the oldest, or every customer is sent away
	void arrest(BuildingTile* thePoliceTile, CustomerTile* theTile) override;
	// 0xb8: 0x460690: the combo's length starts over, and the base's
	void waveDone() override;

private:
	// port: the order is the oldest of those placed (the customer whose turn it is)
	bool isOldest(Order* theOrder);
	// port (the code 0x460c40, 0x4609f0, 0x4611a0 and 0x461030 share): a splat comes up on the building
	void showSplat(BuildingTile* theBuilding, const char* theName);
	// port (the code 0x460c40 and 0x4609f0 share): the order is the next in the combo: it counts, and the sound of the
	// combo plays
	void countInOrder(Order* theOrder);
};

} // namespace pizzafrenzy

#endif
