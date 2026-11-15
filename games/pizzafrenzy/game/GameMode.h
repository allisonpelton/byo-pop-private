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

#include "CityMap.h"
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

#include <deque>
#include <list>
#include <string>
#include <vector>

namespace pizzafrenzy
{

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
	// 0x20: 0x456320: the level is over: the music stops and the tips go to the score; translated next
	virtual void finishLevel() {}
	// 0x28: 0x457900: every customer waiting is sent away, and the waves' orders are let go (a customer that is sent
	// away unasked makes the "customerHangup" signal)
	virtual void hangUpAll(bool theQuiet);
	// 0x2c: 0x456300: the game is paused (the orders picked are let go and the tip jar's roll stops) or not
	virtual void setPaused(bool thePaused);
	// 0x34: 0x458c40 (shared): the orders picked are let go
	virtual void clearPicks();
	// 0x38: 0x457490 (speed), 0x45b6e0 (concentration): the customers' waves, and what they wait for
	virtual void updateWaves(engine::UpdateInfo& theInfo);
	// 0x3c: 0x455fe0: the map's update (its tiles, boats and vehicles)
	virtual void updateMap(engine::UpdateInfo& theInfo);
	// 0x40: 0x456ac0: the level ends when the happiness is gone, or when the customers and the vehicles are done
	virtual void checkLevelEnd();
	// 0x44: the house or the kitchen of the tile is clicked; translated next (the click handler: the callers are
	// BuildingTile::onMouseDownEvent 0x41bd20 and the popups' clicks 0x454cd0)
	virtual void click(BuildingTile* theTile) {}
	// 0x54: the order of the popup on the tile is finished, after the seconds it was shown; translated next with the
	// delivery (the caller is CustomerPopup::finish 0x453210)
	virtual void orderFinished(BuildingTile* theTile, Order* theOrder, float theSeconds) {}
	// 0x48: 0x456130 (speed), 0x45b370 (concentration): the right button lets go of the coupon held, or of the
	// orders picked
	virtual void rightClick();
	// 0x4c: 0x45a300: the pizza is given to the customer of the tile; translated next
	virtual void deliver(engine::Node* thePizza, Tile* theTile, bool theFlag);
	// 0x50: 0x455b20: the special event of the tile starts; translated next
	virtual void startSpecialEvent(engine::Node* theNode, Tile* theTile);
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
	std::vector<engine::Ref<CustomerTile>> mPicks;  // +0x9c: the houses whose orders are picked
	bool mWavesDone;                           // +0xac: the level's orders have all begun
	// +0xb0: the coupon held (0x138 lets it go, 0x144 is its topping, 0x148 its price); translated next
	engine::Ref<engine::Object> mCoupon;
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
	// +0x100: the pizzas on their way to a house, the oldest first, each with the house; its timer is the
	// seconds to the next (translated next)
	struct QueuedDelivery
	{
		engine::Ref<Tile> mTile;
		engine::Ref<engine::Node> mPizza;
	};
	std::deque<QueuedDelivery> mDeliveries;
	float mDeliveryTimer;                      // +0x114: 0.4 seconds from one delivery to the next

	// 0x138 of the first pizza being made (the list at +0x48): translated next
	void updatePizzas(engine::UpdateInfo& theInfo);
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
