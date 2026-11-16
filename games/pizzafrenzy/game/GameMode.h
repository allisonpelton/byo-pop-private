#ifndef __PIZZAFRENZY_GAMEMODE_H__
#define __PIZZAFRENZY_GAMEMODE_H__

// The game of a mode while a level plays (Iggle Pop's GameMode; the application keeps it at +0x110, and the game state
// 0x4029b0 hands it each frame's update). The speed mode's game (0x459510, vtable 0x500d38, 0x120 bytes) and the
// concentration mode's (0x45b3e0, vtable 0x500e68, 0x124 bytes) share most of their slots: the shared ones are this
// class's, and the slots the speed mode has of its own are its (SpeedGame.h).
//
// It holds the city's map, the game screen, the hud and the stats it plays on (set by slot 0x4), the level, and a
// state machine of its own (+0x30): the level's introduction (the titles), and play. Each of those states calls back
// into the mode (ModeStates.h). The customers and their orders, the pizzas and the deliveries are translated with
// them; the places that reach them here are the methods marked "translated next".
//
// Left out of the port for good, one line each: the memory mode (0x460610) and the concentration mode (0x45b3e0, and
// its splats at 0x4609f0, 0x460c40, 0x461030, 0x4611a0); the pizza designer and the level editor. The bonus round's
// game is BonusGame.h.

#include "CityMap.h"
#include "CouponButton.h"
#include "FadeContainer.h"
#include "GameMusicPlayer.h"
#include "GameScreen.h"
#include "GameStats.h"
#include "Hud.h"
#include "Level.h"
#include "Object.h"
#include "Order.h"
#include "Signal.h"
#include "Sound.h"
#include "State.h"
#include "Tip.h"

#include <deque>
#include <list>
#include <string>
#include <vector>

namespace pizzafrenzy
{

class CustomerPopup;
class Van;

// What a special event shows while it plays (the factory 0x44a490 makes one of the classes of 0x44a050 to 0x44b100 from
// the event's type): the mode keeps the popups in a list and updates the first
class EventPopup : public engine::Container
{
public:
	// slot 0x12c: the popup is for the kitchen that sent the pizza and the house of the event's customer
	virtual void begin(BuildingTile* theKitchen, CustomerTile* theTile) = 0;
	// slot 0x130: the popup is shown
	virtual void show() = 0;
	// slot 0x138: a frame of the popup
	virtual void updatePopup(engine::UpdateInfo& theInfo) = 0;
};

class GameMode : public engine::Object, public engine::HasSlots
{
public:
	GameMode();           // 0x459510 (speed), 0x45b3e0 (concentration)
	~GameMode() override;  // 0x45a2e0 (speed), 0x4596d0

	// 0x04: 0x459950 (speed), 0x45bbd0 (concentration): the game screen, the map, the hud and the stats it plays on
	virtual void setup(GameScreen* theScreen, CityMap* theMap, Hud* theHud, GameStats* theStats) = 0;
	// 0x08: 0x4569c0: the pizza being made and the hud, the stats' time, the slowed time, the screen and the states
	virtual void update(engine::UpdateInfo& theInfo);
	// 0x0c: 0x457760 (speed), 0x45b360 (concentration): a signal of the game: "customerHangup" (a customer gave up
	// waiting), "falseArrest" and "specialHangup" (an event's customer is gone)
	virtual void action(const std::string& theAction);
	// 0x10: 0x459cb0: the level is the one to play: its kitchens, boats and events, the city's picture, its music,
	// and the introduction's state
	virtual void setLevel(Level* theLevel);
	// 0x14: 0x459150: the events and the map's boat spawners are let go, and the level with them
	virtual void clearLevel();
	// 0x18: 0x457290 (speed): every counter and list of a level starts over
	virtual void reset();
	// 0x1c: 0x456c20: the introduction is over: the hud's vehicles are shown and play starts
	virtual void startPlay();
	// 0x20: 0x456320: the level is over: the music stops, the orders picked, the tiles' popups and the events' popups
	// are let go, and the tips go to the score
	virtual void finishLevel();
	// 0x24: 0x456c90: every tip is let go, and the houses' and kitchens' popups are taken off their tiles
	virtual void clearTiles();
	// 0x28: 0x457900: every customer waiting is sent away, and the waves' orders are let go (a customer that is sent
	// away unasked makes the "customerHangup" signal)
	virtual void hangUpAll(bool theQuiet);
	// 0x2c: 0x456300: the game is paused (the orders picked are let go and the tip jar's roll stops) or not
	virtual void setPaused(bool thePaused);
	// 0x30: 0x4d0470 (speed: an empty stub): the customers' orders were changed by an event
	virtual void ordersChanged() {}
	// 0x34: 0x458c40 (shared): the orders picked are let go
	virtual void clearPicks();
	// 0x38: 0x457490 (speed), 0x45b6e0 (concentration): the customers' waves, and what they wait for
	virtual void updateWaves(engine::UpdateInfo& theInfo);
	// 0x3c: 0x455fe0: the map's update (its tiles, boats and vehicles)
	virtual void updateMap(engine::UpdateInfo& theInfo);
	// 0x40: 0x456ac0: the level ends when the happiness is gone (the game is over), or when the customers and the
	// vehicles are done (with the prize blimp when every pizza ordered was delivered)
	virtual void checkLevelEnd();
	// 0x44: 0x45adb0: the house or the kitchen of the tile is clicked (the callers are
	// BuildingTile::onMouseDownEvent 0x41bd20 and the popups' clicks 0x454cd0): a house picks its order, a kitchen
	// sends the pizza for the orders picked or turns them away, the police take the customers of the events
	virtual void click(BuildingTile* theTile);
	// 0x54: 0x455ff0: the order of the popup on the tile is finished, after the seconds it was shown: the tip it
	// leaves comes up near the house (the caller is CustomerPopup::finish 0x453210)
	virtual void orderFinished(BuildingTile* theTile, Order* theOrder, float theSeconds);
	// 0x48: 0x456130 (speed), 0x45b370 (concentration): the right button lets go of the coupon held, or of the
	// orders picked
	virtual void rightClick();
	// 0x4c: 0x45a300: the pizza the kitchen made is given to the customer of the tile: the orders and the combo are
	// counted, the money of the pizzas and the combo's added, and the kitchen's van goes to the house (not for the
	// pizza the flag says is not to be sent)
	virtual void deliver(BuildingTile* theKitchen, CustomerTile* theTile, bool theSendVan);
	// 0x50: 0x455b20: the special event of the tile starts with the kitchen's pizza: its popup (EventPopups.h) is made,
	// shown, and the tile's popup is served
	virtual void startSpecialEvent(BuildingTile* theKitchen, CustomerTile* theTile);
	// 0x5c: 0x455d60: a tip comes up at the point for the order (the multiplier from the seconds it waited since the
	// last delivery: 3 under a second and a half, 2 under three, else 1); a tip made already is the citizen award's
	virtual void addTip(Order* theOrder, const engine::Point& thePoint, TipPopup* theTip);
	// 0x60: 0x4561a0: the tip stands on the map at the point, and the game screen shows it
	virtual void showTip(TipPopup* theTip, const engine::Point& thePoint);
	// 0x64: 0x4561d0: the tip is let go at the next update, and the wave's timer is held to a second and a half when
	// the houses are idle
	virtual void releaseTip(TipPopup* theTip);
	// slot 0x7c: 0x456240: the vehicle is drawn on the screen, in the air's group or the ground's
	virtual void addVehicleNode(Van* theVan);
	// slot 0x78: 0x456220: the vehicle is taken off its group, and the map lets go of it
	virtual void removeVehicleNode(Van* theVan);
	// 0x6c: 0x45ac60: the popup of an event is kept and shown, the one before it hidden
	virtual void addEventPopup(EventPopup* thePopup);
	// 0x70: 0x457a90: the popup of an event is let go
	virtual void removeEventPopup(EventPopup* thePopup);
	// 0x74: 0x457b00: the popups of the events are let go
	virtual void clearEventPopups();
	// 0x88: 0x457c50: the coupon is picked: it is held on the cursor when no order is picked, and with some picked it
	// is paid for with the tips (the price for each order) and the orders ask for its topping
	virtual void pickCoupon(CouponButton* theCoupon);
	// 0x8c: 0x457e80: the coupon held is used on the customer's order: it asks for the coupon's topping, and is paid
	virtual void useCoupon(CustomerPopup* thePopup);
	// 0x90: 0x456d10: the order picked is arrested by the police of the tile: their car goes to its house, the
	// customer is counted served, and the police ask for a pizza of their own when no more are pending
	virtual void arrest(BuildingTile* thePoliceTile, CustomerTile* theTile);
	// 0x68: 0x456bd0: the tips are added to the score, and when the houses are all idle the wave's timer is held to
	// a second and a half
	virtual void addTips(int theTips);
	// 0x80: 0x455b80: the time is slowed to the factor for the seconds
	virtual void slowTime(float theSeconds, float theFactor);
	// 0x84: 0x457b80: the low happiness warning is on or off (the hud's, and the music's)
	virtual void setLowHappinessWarning(bool theOn);
	// 0x94: 0x456410: the police leave when a customer of an event is not waiting for them
	virtual void dismissPolice();
	// 0x98: 0x456e50: some customer, picked or ordering, has an order of an event that wants no pizza
	virtual bool hasPendingSpecialOrder();
	// 0x9c: 0x4563d0: the police's house has a popup that is not the police's own
	virtual bool isPoliceBusy();
	// 0xa0: 0x4580f0: the level's titles are written, and the waves begin
	virtual void beginWaves();
	// 0xa8: 0x451610 (true)
	virtual bool isSpeedMode() { return true; }
	// 0xac: 0x45a9a0: the next wave of the level: its orders are made and shuffled
	virtual void startNextWave();
	// 0xb0: 0x457100: the order is given a house, a customer, a wait and a topping where it has none; false when
	// there is no house free
	virtual bool assignOrder(Order* theOrder);
	// 0xb4: 0x459b90: the order is given a place and goes to its house, and the pizzas it asks for are counted
	virtual bool placeOrder(Order* theOrder);
	// 0xb8: 0x459fc0: the last customer of the wave is gone: the wave's timer is held to what it can be
	virtual void waveDone();

	// the number of orders picked
	int getPickCount() { return (int)mPicks.size(); }
	// port (the test commands' bot): the houses whose orders are picked, the first picked first
	const std::vector<engine::Ref<CustomerTile>>& getPicks() const { return mPicks; }
	// port (the test commands "win" and "lose"): the level ends through checkLevelEnd's own paths at the next update:
	// a win has the waves done, no customer waiting, no pizza on its way and every pizza ordered delivered; a loss has
	// no happiness left
	void endLevelForTest(bool theWin);

	// 0x456390: the combo is broken: the topping and the length are cleared, and the map shows none
	void resetCombo();

	// 0x456260: the happiness is changed by the number, between 0 and 100
	void addHappiness(int theChange);

	// the mode's own states: the introduction and play (ModeStates.h)
	engine::StateMgr mStates;  // +0x30

	CityMap* getMap() { return mMap; }
	GameScreen* getScreen() { return mScreen; }
	Hud* getHud() { return mHud; }
	GameStats* getStats() { return mStats; }
	Level* getLevel() { return mLevel; }
	// the level's titles (the node "titles" of the game screen's layout)
	engine::FadeContainer* getTitles() { return mTitles; }

protected:
	engine::Ref<CityMap> mMap;                 // +0x1c
	engine::Ref<GameScreen> mScreen;           // +0x20
	engine::Ref<Hud> mHud;                     // +0x24
	engine::Ref<engine::FadeContainer> mTitles;  // +0x28
	engine::Ref<GameStats> mStats;             // +0x2c
	engine::Ref<GameMusicClip> mMusicClip;     // +0x68: the level's music
	float mMusicVolume;                        // +0x6c: the music clip's volume
	Level* mLevel;                             // +0x70: the level that plays
	std::vector<engine::Ref<EventTimer>> mEventTimers;  // +0x58: the level's special events
	size_t mWaveIndex;                         // +0x74: the wave that plays (the original's iterator over the level's)
	engine::FloatPair mSpawnDelay;             // +0x78: the wave's seconds between one customer and the next
	engine::FloatPair mWaitTime;               // +0x80: the wave's seconds a customer waits
	// +0x48: the popups of the special events that play, the first one updated
	std::vector<engine::Ref<EventPopup>> mEventPopups;
	std::vector<engine::Ref<CustomerTile>> mPicks;  // +0x9c: the houses whose orders are picked
	bool mWavesDone;                           // +0xac: the level's orders have all begun
	engine::Ref<CouponButton> mCoupon;         // +0xb0: the coupon held
	float mSlowFactor;                         // +0xb4: how much of a frame's time passes while the time is slowed
	float mSlowTime;                           // +0xb8: the seconds the time stays slowed
	engine::Sound* mSlowSound;                 // +0xbc: the voice "special_timeEffect", which plays while it is
	std::list<engine::Ref<Order>> mWaveOrders;    // +0xc0: the orders of the wave that are not placed yet
	std::list<engine::Ref<Order>> mActiveOrders;  // +0xcc: the orders placed, the oldest first
	float mWaveTimer;                          // +0xd8: the seconds before the next wave may start
	float mSpawnTimer;                         // +0xdc: the seconds before the next order is placed
	float mSecondsSinceDelivery;               // +0xe0: the seconds since the last delivery
	float mSpeedBonusTime;                     // +0xe4: the seconds the speedy delivery bonus lasts
	int mWaveCustomersLeft;                    // +0xe8: the customers of the level that have not left yet
	int mOrdersBegun;                          // +0xec: the orders the waves have made
	int mDeliveryStreak;                       // +0xf0: the deliveries in a row, ten of which add happiness
	bool mRightClickHandled;                   // +0xf4: the click's own handler took the right button
	Topping* mComboTopping;                    // +0xf8: the topping of the last pizza delivered
	// +0x100: the pizzas on their way to a house, the oldest first, each with the kitchen that made it; mDeliveryTimer
	// is the seconds to the next
	struct QueuedDelivery
	{
		engine::Ref<CustomerTile> mTile;
		engine::Ref<BuildingTile> mKitchen;
	};
	std::deque<QueuedDelivery> mDeliveries;
	float mDeliveryTimer;                      // +0x114: 0.4 seconds from one delivery to the next

	// 0x4569c0's first call: slot 0x138 of the first popup of an event in the list at +0x48
	void updatePopups(engine::UpdateInfo& theInfo);
	// 0x45ad20: the house is picked (the customer's popup is told) and the order cursor shows the orders picked
	void pickOrder(CustomerTile* theTile);
	// 0x4571e0: the order cursor shows the orders picked, from the last (none when none is picked)
	void refreshPickCursor();
	// 0x417cb0: an event timer for each of the level's events
	void makeEvents();
	// 0x459150: the event timers are let go
	void clearEvents();
	// 0x459e20: the wave's orders are drawn in a new order and numbered
	void shuffleOrders();
	// 0x458060: the picks are let go, and so are the coupon held and the order cursor
	void releasePicks();
	// 0x455cb0: the right button is pressed
	void onRightDown(engine::Point& thePoint) { rightClick(); }
	// 0x459330: the right button's signal is connected to onRightDown
	void connectRightButton();
};

} // namespace pizzafrenzy

#endif
