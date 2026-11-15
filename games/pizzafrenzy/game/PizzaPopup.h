#ifndef __PIZZAFRENZY_PIZZAPOPUP_H__
#define __PIZZAFRENZY_PIZZAPOPUP_H__

// What stands on a building: the kitchen's popup with the topping it makes (the customer's popup with the order is
// translated next). Each is a Container the tile puts on its effects node (Tiles.h).

#include "Node.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class BuildingTile;
class Order;

// The base of the popups (0x150 bytes, constructor 0x455340, vtable 0x500798)
class PizzaPopup : public engine::Container
{
public:
	PizzaPopup() : mOwner(nullptr), mFlag(false) {}  // 0x455340

	// slot 0x12c: 0x455000: the tile it stands on
	virtual void setOwner(BuildingTile* theTile) { mOwner = theTile; }
	// slot 0x138: 0x4d14f0 (0) here: 1 for a kitchen's popup that waits for a pizza of its topping and 2 for the
	// police's
	virtual int getState() { return 0; }
	// slot 0x158: 0x454c90: the byte at +0x130, which nothing translated sets
	virtual bool isFlagged() { return mFlag; }
	// slot 0x130: 0x454c60: the popup takes a click (its state is one that does); false here, where there are no
	// states yet
	virtual bool isClickable() { return false; }
	// slot 0x154: 0x454ca0: the popup is picked; false until the customer's popup is translated
	virtual bool isPicked() { return false; }
	// slot 0x150: 0x452f70: the pick is let go; translated next
	virtual void unpick() {}
	// slot 0x160: 0x4551c0: the popup hangs up; translated next
	virtual void hangUp() {}
	// slot 0x18c: 0x452890: the order of a customer's popup, none for another
	virtual Order* getOrder() { return nullptr; }
	// slot 0x164: a frame of the popup, from its tile's
	virtual void tick(engine::UpdateInfo& theInfo) {}
	// slot 0x168: the topping of a kitchen's popup, none for another
	virtual Topping* getTopping() { return nullptr; }

protected:
	BuildingTile* mOwner;  // +0x128
	bool mFlag;            // +0x130
};

// A kitchen's popup (0x16c bytes, constructor 0x451330, vtable 0x500088): the topping the kitchen makes
class PizzaKitchenPopup : public PizzaPopup
{
public:
	PizzaKitchenPopup() {}  // 0x451330

	// slot 0x138: 0x4513d0: a kitchen's popup stands for a kitchen
	int getState() override { return 1; }
	// slot 0x12c: 0x451ab0: the picture of the topping button and the number of the price are made; translated next
	void setOwner(BuildingTile* theTile) override;
	// 0x4519b0: the topping it makes; the picture and the price it shows are translated next
	void setTopping(Topping* theTopping);
	// slot 0x168: 0x452890
	Topping* getTopping() override { return mTopping; }

private:
	engine::Ref<Topping> mTopping;  // +0x14c
};

} // namespace pizzafrenzy

#endif
