#ifndef __PIZZAFRENZY_TIP_H__
#define __PIZZAFRENZY_TIP_H__

// The tip a customer leaves (0x16c bytes, constructor 0x42f050, vtable 0x4fc558) and the citizen award the police's
// arrest brings (constructor 0x455cc0, vtable 0x500ae0): a note of cash that comes up where the order was served, waits
// for the player's click, flashes in its last seconds and is lost. A click takes its money; the city map keeps the tips
// that wait (CityMap.h) and the mode makes them (GameMode.h, orderFinished).
//
// Left out: the splats that fly to the tip jar when a tip is taken (the splat factory's "cashTipSplat" with the amount
// and "tipSplat" with the picture): the money is added as the click ends, where the original adds it when the second
// splat has arrived.

#include "Node.h"
#include "Selector.h"
#include "State.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class CityMap;

class TipPopup : public engine::Container
{
public:
	TipPopup();            // 0x42f050
	~TipPopup() override;  // 0x42ef80

	// slot 0x12c: 0x42ec80: the multiplier of the tip (3 for a fast delivery, 2, or 1) and the topping that was
	// delivered; the money is the topping's upgrade's worth times the multiplier, and the picture is the multiplier's
	virtual void setValue(float theMultiplier, Topping* theTopping);
	// slot 0x130: 0x42e940: the two pictures of the tip's board, plain and with the mouse over it
	virtual void makeBoard();
	// slot 0x134: 0x42eac0: the tip comes up
	virtual void start();
	// slot 0x138: 0x42f160: the player takes the tip
	virtual void take();
	// slot 0x13c: 0x42e4c0: the mode lets the tip go and adds its money to the tips
	virtual void finish();
	// 0x42e510: the tip stands on the map at the point, which keeps it
	void setMap(CityMap* theMap, const engine::Point& thePosition);
	// 0x42e560: a frame of the tip: its states', and its node's
	void updateTip(engine::UpdateInfo& theInfo);
	// slot 0xf4: 0x42e540: the map lets go of the tip when its node has left its parent
	void clearParent(engine::Node* theParent) override;
	// slot 0x94 (the base's): 0x42f780 is slot 0x144 of the vehicles; the tip's own update is the node's
	bool isWaiting() const { return mWaiting; }  // 0x42f680 (the map reads +0x160)
	void setWaiting(bool theWaiting) { mWaiting = theWaiting; }  // 0x42f670 (false)
	int getAmount() const { return mAmount; }
	engine::StateMgr mStates;  // +0x144

protected:
	// slot 0xa8: 0x42e4f0: a tip that waits is taken by the click
	void onMouseDownEvent(engine::MouseEvent& theEvent) override;
	// slot 0xe0: 0x42ee40: with the board lit and a sound
	void onMouseEnterEvent(engine::MouseEvent& theEvent) override;
	// slot 0xec: 0x42eb30
	void onMouseExitEvent(engine::MouseEvent& theEvent) override;

	float mMultiplier;                        // +0x138
	int mAmount;                              // +0x13c: the money of the tip
	engine::Ref<engine::Selector> mBoard;     // +0x140
	CityMap* mMap;                            // +0x15c
	bool mWaiting;                            // +0x160: the tip waits for the click
};

// The citizen award (0x16c bytes, constructor 0x455cc0, vtable 0x500ae0): the tip of an arrest, 500 whatever the
// topping; the player's click on it takes it the same way, with another sound
class CitizenAward : public TipPopup
{
public:
	// slot 0x12c: 0x42d630: the money is 500 and the picture is the award's
	void setValue(float theMultiplier, Topping* theTopping) override;
	// slot 0x138: 0x42d830
	void take() override;
};

// The tip comes up (0x1c bytes, vtable 0x4fc20c): over a fifth of a second it grows from half its size and fades in
class TipAppearState : public engine::State
{
public:
	explicit TipAppearState(TipPopup* theTip) : mTip(theTip) {}  // 0x42d5a0
	// 0x04: 0x42e590
	void enter() override;
	// 0x0c: 0x42e5c0
	void update(engine::UpdateInfo& theInfo) override;
private:
	TipPopup* mTip;  // +0x0c
	float mTimeLeft = 0;  // +0x10
};

// The tip waits (0x1c bytes, vtable 0x4fc498): five seconds
class TipWaitState : public engine::State
{
public:
	explicit TipWaitState(TipPopup* theTip) : mTip(theTip) {}  // 0x42e790
	// 0x04: 0x42e650
	void enter() override;
	// 0x0c: 0x42eb50
	void update(engine::UpdateInfo& theInfo) override;
private:
	TipPopup* mTip;  // +0x0c
	float mTimeLeft = 0;  // +0x10
};

// The tip flashes (0x1c bytes, vtable 0x4fc4ac): its last five seconds
class TipFlashState : public engine::State
{
public:
	explicit TipFlashState(TipPopup* theTip) : mTip(theTip) {}  // 0x42e820
	// 0x04: 0x42e660
	void enter() override;
	// 0x08: 0x42e6f0
	void exit() override;
	// 0x0c: 0x42ebf0
	void update(engine::UpdateInfo& theInfo) override;
private:
	TipPopup* mTip;  // +0x0c
	float mTimeLeft = 0;  // +0x10
};

// The tip is lost (0x1c bytes, vtable 0x4fc4c0): a fifth of a second to grow and fade, and the mode lets it go
class TipLostState : public engine::State
{
public:
	explicit TipLostState(TipPopup* theTip) : mTip(theTip) {}  // 0x42e8b0
	// 0x04: 0x42eef0
	void enter() override;
	// 0x0c: 0x42e700
	void update(engine::UpdateInfo& theInfo) override;
private:
	TipPopup* mTip;  // +0x0c
	float mTimeLeft = 0;  // +0x10
};

} // namespace pizzafrenzy

#endif
