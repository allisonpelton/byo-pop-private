#ifndef __PIZZAFRENZY_PIZZAPOPUP_H__
#define __PIZZAFRENZY_PIZZAPOPUP_H__

// What stands on a building: the kitchen's popup with the topping it makes, the customer's with the order
// (CustomerPopup.h) and the police's. Each is a Container the tile puts on its effects node (Tiles.h), and has a
// machine of states (PopupStates.h) that decides what it looks like and whether it takes a click.

#include "ImageNode.h"
#include "Node.h"
#include "PopupStates.h"
#include "TextItem.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class BuildingTile;
class CustomerTile;
class Order;

// What a popup stands for (the number of its slot 0x138)
enum PopupKind
{
	POPUP_CUSTOMER = 0,
	POPUP_KITCHEN = 1,
	POPUP_POLICE = 2
};

// The base of the popups (0x150 bytes, constructor 0x455340, vtable 0x500798)
class PizzaPopup : public engine::Container
{
public:
	PizzaPopup();           // 0x455340
	~PizzaPopup() override;  // 0x454f40

	// slot 0x80: 0x454c40: a byte of its own is set in place of the mouse flag
	void setEnabled(bool theEnabled) override { mEnabledFlag = theEnabled; }
	// slot 0x98: 0x455110: the bounds are the rectangle united with the selection box's, scaled and moved to the
	// position
	void layout() override;
	// slot 0xf4: 0x454d60: the tile lets go of the popup when it has left its parent
	void clearParent(engine::Node* theParent) override;

	// slot 0x12c: 0x455000: the tile it stands on; the selection box is made
	virtual void setOwner(BuildingTile* theTile);
	// slot 0x130: 0x454c60: the popup takes a click, which its state says
	virtual bool isClickable();
	// slot 0x134: 0x455190: the selection box is shown or hidden
	virtual void setSelectionVisible(bool theVisible);
	// slot 0x138: 0x4ac97d (none here)
	virtual int getKind() = 0;
	// slot 0x13c: 0x4ac97d (none here): the popup goes back to waiting for a click
	virtual void rest() = 0;
	// slot 0x140: 0x452050 (the same in all three): the popup is taken off its parent at the next update
	virtual void leave();
	// slot 0x144: 0x455260: the mouse is over it: the selection box shows, and a sound plays
	virtual void hover();
	// slot 0x148: 0x454d30: its look is the plain one
	virtual void pick();
	// slot 0x14c: 0x454d50: the selection box is hidden
	virtual void serve();
	// slot 0x150: 0x454d50: the selection box is hidden (the mouse is gone, or the pick is let go)
	virtual void unpick();
	// slot 0x154: 0x454ca0: its state is one the pick can be let go in
	virtual bool isPicked();
	// slot 0x158: 0x454c90: the popup is going away
	virtual bool isLeaving() { return mLeaving; }
	// slot 0x160: 0x4551c0: the popup shrinks and fades out, and leaves
	virtual void hangUp();
	// slot 0x164: 0x454c50: a frame of the popup, from its tile's
	virtual void tick(engine::UpdateInfo& theInfo);
	// slot 0x168 (the kitchen's): the topping a kitchen's popup makes, none for another
	virtual Topping* getTopping() { return nullptr; }
	// slot 0x18c (the customer's): the order of a customer's popup, none for another
	virtual Order* getOrder() { return nullptr; }

	void setLeaving(bool theLeaving) { mLeaving = theLeaving; }

	PopupStateMgr mStates;  // +0x134

protected:
	// slot 0xa8: 0x454cd0: the click is the tile's
	void onMouseDownEvent(engine::MouseEvent& theEvent) override;
	// slot 0xe0: 0x454cf0
	void onMouseEnterEvent(engine::MouseEvent& theEvent) override;
	// slot 0xec: 0x454d10
	void onMouseExitEvent(engine::MouseEvent& theEvent) override;

	BuildingTile* mOwner;                      // +0x128
	engine::Ref<engine::ImageNode> mSelection;  // +0x12c: the box around the popup the mouse is over
	bool mLeaving;                             // +0x130: set while the popup closes or hangs up
	bool mEnabledFlag;                         // +0x131: set with the owner and by setEnabled; nothing reads it
};

// A kitchen's popup (0x16c bytes, constructor 0x451330, vtable 0x500088): the topping the kitchen makes, on a button,
// with the price of its next upgrade
class PizzaKitchenPopup : public PizzaPopup
{
public:
	PizzaKitchenPopup() {}  // 0x451330

	// slot 0x12c: 0x451ab0: the button, the topping's holder, the price's number, and the popup comes up
	void setOwner(BuildingTile* theTile) override;
	// slot 0x138: 0x4513d0
	int getKind() override { return POPUP_KITCHEN; }
	// slot 0x13c: 0x451850
	void rest() override;
	// slot 0x144: 0x451650: with some order picked the mouse over it shows the selection box
	void hover() override;
	// slot 0x148: 0x451710
	void pick() override;
	// slots 0x14c and 0x150: 0x4517b0
	void serve() override { unpick(); }
	void unpick() override;
	// slot 0x168: 0x452890
	Topping* getTopping() override { return mTopping; }

	// 0x4519b0: the topping it makes: its picture and the price of the upgrade the player has the level of
	void setTopping(Topping* theTopping);

private:
	// 0x451920: the number of the price, after a dollar sign
	void setPrice(int thePrice);

	engine::Ref<Topping> mTopping;                  // +0x14c
	engine::Ref<engine::Container> mToppingHolder;  // +0x150: the topping's picture
	engine::Ref<engine::ImageNode> mButton;         // +0x154
	engine::Ref<engine::Container> mPriceHolder;    // +0x158
	engine::Ref<engine::TextItem> mPrice;           // +0x15c
};

// The police's popup (0x158 bytes, constructor 0x4554c0, vtable 0x500928): the button of the police on a customer's
// house
class PolicePopup : public PizzaPopup
{
public:
	PolicePopup() {}  // 0x4554c0

	// slot 0x12c: 0x455650: the button, and the popup comes up
	void setOwner(BuildingTile* theTile) override;
	// slot 0x138: 0x455540
	int getKind() override { return POPUP_POLICE; }
	// slot 0x13c: 0x455a40
	void rest() override;
	// slot 0x144: 0x4557a0
	void hover() override;
	// slot 0x148: 0x455860
	void pick() override;
	// slot 0x14c: 0x455900: the selection box is hidden, and the popup closes
	void serve() override;
	// slot 0x150: 0x4559a0
	void unpick() override;
	// slot 0x168: 0x455410: the police's car goes from the popup's tile to the house of the customer to arrest
	virtual void sendCar(CustomerTile* theTile);
private:
	engine::Ref<engine::ImageNode> mButton;  // +0x14c
};

} // namespace pizzafrenzy

#endif
