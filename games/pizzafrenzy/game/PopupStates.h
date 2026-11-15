#ifndef __PIZZAFRENZY_POPUPSTATES_H__
#define __PIZZAFRENZY_POPUPSTATES_H__

// The machine each popup on a building has, and the states the kitchen's popup and the police's have (the customer's
// has its own, CustomerPopup.h). A state is made for the popup it works on, which owns the machine; a popup's frame
// is its machine's update.

#include "State.h"

namespace pizzafrenzy
{

class PizzaPopup;

// A popup's state (0x1c bytes, constructor 0x451580, vtable 0x500218). The popup asks it whether a click or a pick is
// taken in the state it is in.
class PopupState : public engine::State
{
public:
	explicit PopupState(PizzaPopup* theOwner) : mOwner(theOwner) {}  // 0x451580

	// slot 0x14: 0x451610 (true): the popup takes the mouse
	virtual bool isClickable() { return true; }
	// slot 0x18: 0x451610 (true): the pick of the popup can be let go
	virtual bool isPicked() { return true; }

protected:
	PizzaPopup* mOwner;  // +0x0c
};

// The machine of a popup (0x14 bytes, vtable 0x50076c, constructor 0x470750 on the engine's)
class PopupStateMgr : public engine::StateMgr
{
public:
	// slot 0x04: 0x454e60: the engine's way to change the state does nothing here, the popup's own does
	bool changeState(engine::State* theState) override { return false; }
	// 0x0c: the state is kept for the length of its own update, which can change the state
	void update(engine::UpdateInfo& theInfo) override;
	// slot 0x14: 0x454ee0: the old state's exit runs and it is let go, and the new one is counted and entered
	virtual bool setState(engine::State* theState);
	// slot 0x18: 0x411960
	PopupState* getCurrent() { return static_cast<PopupState*>(getState()); }
};

// A popup coming up (0x20 bytes, vtable 0x5001fc): over a fifth of a second it grows from half its size and fades in,
// and then the popup rests
class AppearState : public PopupState
{
public:
	AppearState(PizzaPopup* theOwner, bool theClickable) : PopupState(theOwner), mClickable(theClickable) {}

	// 0x04: 0x454d90
	void enter() override;
	// 0x0c: 0x454da0
	void update(engine::UpdateInfo& theInfo) override;
	// slot 0x14: 0x451620: the popup takes the mouse while it comes up when the state was made so
	bool isClickable() override { return mClickable; }

private:
	float mTimeLeft = 0;  // +0x10
	bool mClickable;      // +0x14
};

// A popup at rest (0x18 bytes, vtable 0x500a94): its full size and fully opaque
class RestState : public PopupState
{
public:
	explicit RestState(PizzaPopup* theOwner) : PopupState(theOwner) {}

	// 0x04: 0x451310
	void enter() override;
};

// A popup going away (0x1c bytes, vtable 0x500574): over half a second it shrinks and fades out, flagged as going,
// and then it leaves; it takes no click
class ClosingState : public PopupState
{
public:
	explicit ClosingState(PizzaPopup* theOwner) : PopupState(theOwner) {}

	// 0x04: 0x454e30
	void enter() override;
	// 0x08: 0x454e50
	void exit() override;
	// 0x0c: 0x454e70
	void update(engine::UpdateInfo& theInfo) override;
	// slots 0x14 and 0x18: 0x4529d0 (false)
	bool isClickable() override { return false; }
	bool isPicked() override { return false; }

private:
	float mTimeLeft = 0;  // +0x10
};

} // namespace pizzafrenzy

#endif
