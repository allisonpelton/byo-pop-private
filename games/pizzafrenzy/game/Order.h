#ifndef __PIZZAFRENZY_ORDER_H__
#define __PIZZAFRENZY_ORDER_H__

// What a customer asks for (Order, 0x34 bytes, constructor 0x4196e0) and the special event of a level that makes one of
// its own (EventTimer, 0x1c bytes, constructor 0x44b490). A mode makes the orders of a wave (GameMode.h), shuffles
// them, and gives each a house to order from.

#include "DocHandler.h"
#include "Object.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class CustomerTile;
struct LevelEvent;

class Order : public engine::Object
{
public:
	Order() { clear(); }  // 0x4196e0
	// 0x452180: the fields are copied
	Order(const Order& theOrder) :
		engine::Object(),
		mTile(theOrder.mTile),
		mCharacter(theOrder.mCharacter),
		mTopping(theOrder.mTopping),
		mEvent(theOrder.mEvent),
		mPizzaCount(theOrder.mPizzaCount),
		mWaitTime(theOrder.mWaitTime),
		mSecondsSinceDelivery(theOrder.mSecondsSinceDelivery),
		mIndex(theOrder.mIndex)
	{
	}
	// 0x419690
	void clear();
	// whether the customer wants pizzas: an order of a special event that wants none (a thief) is not one to deliver
	bool asksForPizza() const { return mEvent == nullptr || mEvent->mDeliver; }

	CustomerTile* mTile;            // +0x0c: the house it is ordered from; none until the order is placed
	Character* mCharacter;          // +0x10: who orders
	Topping* mTopping;              // +0x14: the topping of one of the map's kitchens
	SpecialEvent* mEvent;           // +0x18: none for an ordinary customer
	int mPizzaCount;                // +0x1c: how many pizzas
	float mWaitTime;                // +0x20: the seconds the customer waits; 0 until the order is placed
	float mSecondsSinceDelivery;    // +0x24: the seconds since the last delivery, when the order was made
	int mIndex;                     // +0x28: the order of the wave's shuffle (the biggest number is the first drawn)
};

// A special event of a level, counted in waves (0x1c bytes)
class EventTimer : public engine::Object
{
public:
	explicit EventTimer(LevelEvent* theEvent);  // 0x44b490
	// 0x44b3c0: the waves it waits start over
	void reset();
	// 0x44b530: a wave starts; when the waves it waited are up, it makes its order (none until then)
	Order* tick();

private:
	// 0x44b3e0: the order of the event's character, none when the manifest has no such event or character
	Order* makeOrder();

	LevelEvent* mEvent;  // +0x0c
	int mWavesLeft;      // +0x10: the waves before the event happens
};

} // namespace pizzafrenzy

#endif
