#ifndef __PIZZAFRENZY_CUSTOMERPOPUP_H__
#define __PIZZAFRENZY_CUSTOMERPOPUP_H__

// What stands on a customer's house while the customer waits (0x1a0 bytes, constructor 0x452220, vtable 0x5003b8): the
// customer's face, the pizzas the order asks for and the button the player clicks, and the states a customer goes
// through. The time the customer waits runs down while the popup is in a state that waits; the last two seconds make
// it flash, and at the end the customer hangs up.

#include "Modifiers.h"
#include "Order.h"
#include "PizzaPopup.h"
#include "Selector.h"

namespace pizzafrenzy
{

class CustomerPopup : public PizzaPopup
{
public:
	CustomerPopup();            // 0x452220
	~CustomerPopup() override;  // 0x452b10

	// slot 0x12c: 0x454480: the base's, then the three modifiers, the face's selector with its two sides, the button,
	// the customer's pictures and the pizzas' holder
	void setOwner(BuildingTile* theTile) override;
	// slot 0x134: 0x4527c0: the picture of an event's customer shows with the selection box
	void setSelectionVisible(bool theVisible) override;
	// slot 0x138: 0x4d14f0
	int getKind() override { return POPUP_CUSTOMER; }
	// slot 0x13c: 0x453510: the customer waits again (dimmed when the popup was made so)
	void rest() override;
	// slot 0x144: 0x452e20: the mouse is over it
	void hover() override;
	// slot 0x148: 0x452ed0: the order is picked
	void pick() override;
	// slot 0x14c: 0x453140: the order is being served: the modifiers are let go and the popup takes no click
	void serve() override;
	// slot 0x150: 0x452f70: the pick is let go: the order's pizzas show, and the customer waits again
	void unpick() override;
	// slot 0x160: 0x4532e0: the popup closes (the base's closing state); the mode is not told
	void hangUp() override;
	// slot 0x18c: 0x452890
	Order* getOrder() override { return mOrder; }

	// slot 0x168: 0x453e50: the order is shown; the flag dims the popup (it takes no click) and is kept
	virtual void setOrder(Order* theOrder, bool theDimmed);
	// slot 0x16c: 0x452c10: another order of the same customer replaces the shown one; the popup waits again when asked
	virtual void replaceOrder(Order* theOrder, bool theWait);
	// slot 0x170: 0x452d20: the topping of the order is not known until the pictures have run through the toppings
	// of the map and stopped on it
	virtual void startChameleon(Order* theOrder, bool theQuiet);
	// slot 0x174: 0x454050: the order cursor shows the orders picked: a card for each, this order's pizzas on the last,
	// and the indicator of how many more orders can be picked with it
	virtual void buildPickCards(engine::Container* theCursor);
	// slot 0x178: 0x451e50: one piece of the indicator, in its lit picture on the cursor and its flashing one in the
	// holder that flashes when a click is not taken (the kind is 0 for the left end, 1 for the middle, 2 for the right
	// end and 3 for the indicator of a single order)
	virtual void addIndicator(engine::Container* theCursor, engine::Container* theFlashHolder, float theX, float theY,
	                          int theKind);
	// slot 0x17c: 0x452b30: whether the customer is a chameleon; the customer waits again
	virtual void setChameleon(bool theChameleon);
	// slot 0x180: 0x453210: the order is finished: the mode is told, and the popup closes
	virtual void finish();
	// slot 0x184: 0x453020: the popup flashes (impatient) or hangs up
	virtual void setImpatient(bool theImpatient);
	// slot 0x188: 0x452840: the colour flash starts, for a popup that takes a click
	virtual void flash();
	// slot 0x190: 0x453390: the order is hidden behind the question mark and the customer waits
	virtual void hideOrder();
	// slot 0x194: 0x4528a0: the question mark goes, and the order shows
	virtual void revealOrder();

	// 0x452660: the pictures of the order's pizzas (one of the topping's for each), or the question mark's while it
	// shows, are put in the holder
	void showOrder(engine::Container* theHolder, Order* theOrder);
	// 0x453930: the sound of a customer who arrives: the popup's, or the customer's own greeting
	void playGreeting();
	// 0x453460: the seconds before a chameleon changes again run down
	void tickChameleon(engine::UpdateInfo& theInfo);

	// the states' own
	float mElapsed = 0;         // +0x184: the seconds the order has been shown
	float mWaitLeft = 0;        // +0x188: the seconds the customer still waits
	float mChameleonTimer = 0;  // +0x18c: the seconds before a chameleon changes its order; 0 when it does not
	bool mChameleon = false;    // +0x194
	bool mDimmed = false;       // +0x195
	engine::Ref<engine::Container> mOrderHolder;        // +0x150: the pizzas of the order
	engine::Ref<engine::OpacityModifier> mOpacityFlash;  // +0x17c: the last seconds
	engine::Ref<engine::ColorFlashModifier> mColorFlash;  // +0x180

	engine::Ref<Order> mOrder;                       // +0x14c
	engine::Ref<engine::Selector> mFace;             // +0x154: the button's side (0) and the customer's side (1)
	engine::Ref<engine::Selector> mSpecial;          // +0x158: the order's side (0) and the event's customer (1)
	engine::Ref<engine::ImageNode> mButton;          // +0x15c
	engine::Ref<engine::ImageNode> mFaceImage;       // +0x160: the customer's picture on the button's side
	engine::Ref<engine::ImageNode> mFaceImageBack;   // +0x164: the same on the other side
	engine::Ref<engine::ImageNode> mQuestionMark;         // +0x168: the question mark
	engine::Ref<engine::ImageNode> mEventImage;      // +0x16c: the event's customer
	engine::Ref<engine::Container> mFront;           // +0x170: the order's side
	engine::Ref<engine::Container> mBack;            // +0x174: the customer's side
	engine::Ref<engine::ScaleModifier> mScaleFlash;  // +0x178: nothing translated reads it
};

// The customer waits (0x1c bytes, vtable 0x500590): the wait runs down, and in its last two seconds the customer is
// impatient
class WaitingState : public PopupState
{
public:
	explicit WaitingState(PizzaPopup* theOwner) : PopupState(theOwner) {}
	// 0x04: 0x4528c0: the button's side shows
	void enter() override;
	// 0x0c: 0x453620
	void update(engine::UpdateInfo& theInfo) override;
};

// The last two seconds (0x1c bytes, vtable 0x5005ac): the popup's opacity flashes; the customer hangs up when the wait
// is over
class ImpatientState : public PopupState
{
public:
	explicit ImpatientState(PizzaPopup* theOwner) : PopupState(theOwner) {}
	// 0x04: 0x4528f0
	void enter() override;
	// 0x08: 0x452950
	void exit() override;
	// 0x0c: 0x453730
	void update(engine::UpdateInfo& theInfo) override;
};

// The customer hangs up (0x20 bytes, vtable 0x5005c8): half a second to shrink and fade, a signal to the mode, and a
// sound; the popup takes no click
class HangupState : public PopupState
{
public:
	explicit HangupState(PizzaPopup* theOwner) : PopupState(theOwner) {}
	// 0x04: 0x453a10
	void enter() override;
	// 0x0c: 0x452100
	void update(engine::UpdateInfo& theInfo) override;
	bool isClickable() override { return false; }  // 0x4529d0
	bool isPicked() override { return false; }

private:
	float mTimeLeft = 0;  // +0x14
};

// The order is picked (0x1c bytes, vtable 0x5005e4): the customer's side shows
class PickedState : public PopupState
{
public:
	explicit PickedState(PizzaPopup* theOwner) : PopupState(theOwner) {}
	// 0x04: 0x452980
	void enter() override;
	// 0x0c: 0x452060
	void update(engine::UpdateInfo& theInfo) override;
	bool isClickable() override { return false; }  // 0x4529d0
};

// The order is being served (0x1c bytes, vtable 0x500600): only the seconds the order has been shown count
class ServedState : public PopupState
{
public:
	explicit ServedState(PizzaPopup* theOwner) : PopupState(theOwner) {}
	// 0x0c: 0x452090
	void update(engine::UpdateInfo& theInfo) override;
	bool isClickable() override { return false; }
	bool isPicked() override { return false; }
};

// The mouse is over the popup (0x1c bytes, vtable 0x50061c)
class HoverState : public PopupState
{
public:
	explicit HoverState(PizzaPopup* theOwner) : PopupState(theOwner) {}
	// 0x0c: 0x452060
	void update(engine::UpdateInfo& theInfo) override;
};

// The chameleon changes (0x34 bytes, vtable 0x500638): the order's topping goes through the toppings of the map's
// kitchens, seven of them, one every twentieth of a second, and stops on a topping (the one after its own in the list
// when none is given)
class ChameleonState : public PopupState
{
public:
	ChameleonState(PizzaPopup* theOwner, Topping* theTarget, bool theQuiet);  // 0x452a40
	// 0x04: 0x453840
	void enter() override;
	// 0x08: 0x453dd0
	void exit() override;
	// 0x0c: 0x453c10
	void update(engine::UpdateInfo& theInfo) override;
	// slots 0x14 and 0x18: the base's 0x451610 (true): the changing customer takes a click and a pick

private:
	float mStepTime = 0;    // +0x10: the seconds before the next topping
	int mStepsLeft = 0;     // +0x14
	size_t mNext = 0;       // +0x18: the place of the next topping in the list (the original's iterator)
	Topping* mTarget;       // +0x20: the topping it stops on
	bool mQuiet;            // +0x28: no sound for each topping
};

// The popup is dimmed (0x1c bytes, vtable 0x500654): its colour is a little darker, and it takes no click
class DimmedState : public PopupState
{
public:
	explicit DimmedState(PizzaPopup* theOwner) : PopupState(theOwner) {}
	// 0x04: 0x4520b0
	void enter() override;
	// 0x08: 0x4520e0
	void exit() override;
	bool isClickable() override { return false; }  // 0x4529d0
};

} // namespace pizzafrenzy

#endif
