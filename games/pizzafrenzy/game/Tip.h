#ifndef __PIZZAFRENZY_TIP_H__
#define __PIZZAFRENZY_TIP_H__

// The tip a customer leaves (0x16c bytes, constructor 0x42f050, vtable 0x4fc558) and the citizen award the police's
// arrest brings (constructor 0x455cc0, vtable 0x500ae0): a note of cash that comes up where the order was served, waits
// for the player's click, flashes in its last seconds and is lost. A click takes its money; the city map keeps the tips
// that wait (CityMap.h) and the mode makes them (GameMode.h, orderFinished).
//
// Taking a tip makes two splats (the splat factory's "cashTipSplat" with the amount and "tipSplat" with the picture,
// which flies to the tip jar); the tip's money is added when the second has arrived.

#include "Node.h"
#include "Selector.h"
#include "State.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class CityMap;

class TipPopup : public engine::Container, public engine::HasSlots
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
	// slot 0x138: 0x42f160: the player takes the tip: its money flies to the tip jar as a splat, which finishes the
	// tip when it arrives
	virtual void take();
	// slot 0x13c: 0x42e4c0: the mode lets the tip go and adds its money to the tips
	virtual void finish();
	// the two splats of a tip that is taken (0x42f160, 0x42d830)
	void flyToTipJar(const std::string& theAmountText, const std::string& thePicture);
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
	// the balloon's states take the board, which opens when the tip drifts
	void openBoard() { mBoard->select(0, 0.0f, true); }

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

// The balloon tip (0x16c bytes, constructor 0x42dc40, vtable 0x4fc298): the tip the prize blimp drops (PrizeBlimp.h).
// It
// is worth ten for each of the multiplier, whatever the topping, hangs under a balloon, flies off to one side as it
// comes up, and then drifts down swaying until the player takes it or it is off the screen.
class BalloonTip : public TipPopup
{
public:
	BalloonTip() {}  // 0x42dc40

	// slot 0x98: 0x42e090: the bounds are the balloon's alone
	void layout() override;
	// slot 0x12c: 0x42e110
	void setValue(float theMultiplier, Topping* theTopping) override;
	// slot 0x130: 0x42de60: the shadow, and the balloon's three pictures: closed (the tip comes down on it), open and
	// lit
	void makeBoard() override;
	// slot 0x134: 0x42e1d0: the balloon flies off
	void start() override;
};

// The balloon flies off (0x1c bytes, constructor 0x42dd90, vtable 0x4fc3f8): a fraction of a second under a pull
// downwards, to either side
class BalloonFlyState : public engine::State
{
public:
	explicit BalloonFlyState(TipPopup* theTip) : mTip(theTip), mTimeLeft(0) {}  // 0x42dd90
	// 0x04: 0x42dbc0
	void enter() override;
	// 0x0c: 0x42e240
	void update(engine::UpdateInfo& theInfo) override;
private:
	TipPopup* mTip;           // +0x0c
	engine::Point mVelocity;  // +0x10
	float mTimeLeft;          // +0x18
};

// The balloon drifts down (0x20 bytes, constructor 0x42dcc0, vtable 0x4fc3e4): the tip waits for the click the whole
// way
class BalloonDriftState : public engine::State
{
public:
	explicit BalloonDriftState(TipPopup* theTip) : mTip(theTip) {}  // 0x42dcc0
	// 0x04: 0x42e360
	void enter() override;
	// 0x0c: 0x42db30
	void update(engine::UpdateInfo& theInfo) override;
private:
	TipPopup* mTip;           // +0x0c
	engine::Point mVelocity;  // +0x10
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
