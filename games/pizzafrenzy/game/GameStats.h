#ifndef __PIZZAFRENZY_GAMESTATS_H__
#define __PIZZAFRENZY_GAMESTATS_H__

// What the game that is being played has counted so far (Iggle Pop's GameStats, 0x78 bytes, constructor 0x41a930; the
// application keeps it at +0x120): the score and the time of the game, the level of each topping the player owns and
// the counters the modes keep. A game starts with it reset (0x41a7b0), or resumed from the mode's saved game
// (0x41a900). The modes and the screens that show it are translated next; the fields below are the ones the reset
// and the loading state reach, and the counters it zeroes are named for their place.

#include "Level.h"
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

	// 0x41a610: the upgrade of the topping the player has the level of (the last one for a level past the list)
	Upgrade* getUpgrade(Topping* theTopping);

	// 0x41a740: a level starts: the number of orders and the goal are the level's, the level's counters start over
	// and each kitchen the level gives no topping is given one of the player's
	void startLevel(Level* theLevel);
	// 0x419c90: the counters of a level start over (not the number of orders)
	void resetLevelCounters();

	// 0x419b90: the tips are added to the tip money, which is never under 0
	void addTips(int theTips);
	// 0x41a670: how many orders for the topping can be picked together: one more than the player's level of it (one in
	// the concentration mode)
	int getPickLimit(Topping* theTopping);

	// 0x419d30: the mode's saved game takes the score and the time, and the player's toppings are the game's
	void saveTo(UserGameData* theUserData, int theMode);

	int mScore;                                  // +0x0c
	float mElapsedTime;                          // +0x10
	ToppingMap<int> mToppingLevels;      // +0x14: the level of each topping the player has
	int mPizzasDelivered;                        // +0x20: the pizzas the customers have been given; the level ends with the
	                                             // prize blimp when they are all (0x456ac0, 0x45a300)
	int mPizzasOrdered;                          // +0x24: the pizzas the customers have ordered (0x459b90, 0x456d10)
	// +0x28: the happiness meter, which starts at 50; the hud makes a burst of stars (the splat "happinessBonus")
	// when it is over 99 (0x4397d0)
	int mHappiness;
	int mCustomersServed;                        // +0x2c: counted up when an order is served and down when a customer
	                                             // makes the next order (0x45a300, 0x456d10)
	// +0x30: the hud's orders (0x439650: a slip for each 15, rounded up, and none shown at 0); the number of orders
	// the level has, less one for each customer that leaves (0x456d10, 0x457760)
	int mOrderCount = 0;
	// +0x34: what the hud compares with the price of a vehicle to show whether it can be bought (0x439b00, 0x44eac0);
	// the meaning of the number (the tips) is not confirmed
	int mTipMoney;
	int mTipsEarned;                             // +0x38: 1000 where the level is ended by hand (0x456460); the meaning of
	                                             // the number is not confirmed
	int mWavesStarted;                           // +0x3c: counted up when a wave starts (0x45a9a0)
	// +0x40: how many pizzas of the same topping the player has delivered in a row (the kitchens' signs show it)
	int mComboLength;
	// +0x44: the longest combo of each topping this game (0x45a300), which an upgrade's combo is compared with
	ToppingMap<int> mBestCombos;
	int mCountI;                                 // +0x50: nothing translated reads it
	int mCountJ;                                 // +0x54: nothing translated reads it
	int mCountK;                                 // +0x58: nothing translated reads it
	int mCountL;                                 // +0x5c: nothing translated reads it
	int mCountM;                                 // +0x60: nothing translated reads it
	int mLevelOrders;                            // +0x64: the level's number of orders (0x41a740); nothing reads it
	int mLevelGoal;                              // +0x68: the level's goal (0x41a740); nothing reads it
	bool mStarted;                               // +0x6c: set by the reset; nothing translated reads it

private:
	// 0x41a220: every kitchen of the level is given one of the player's toppings, none twice while there are more
	// toppings than kitchens; the toppings the kitchens have are the player's picks, which the player keeps
	void pickKitchenToppings(Level* theLevel);
	// 0x41a6a0: the pizzas the player designed count as toppings, with the level the player has of each (0 for none)
	void addDesignedPizzas();
};

} // namespace pizzafrenzy

#endif
