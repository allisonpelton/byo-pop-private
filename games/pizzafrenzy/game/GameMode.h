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
#include "Signal.h"
#include "Sound.h"
#include "State.h"

#include <string>

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
	// 0x0c: 0x457760 (speed), 0x45b360 (concentration): a signal of the game ("customerHangup", "falseArrest",
	// "specialHangup"); translated next
	virtual void action(const std::string& theAction) {}
	// 0x10: 0x459cb0: the level is the one to play: its kitchens, boats and events, the city's picture, its music,
	// and the introduction's state
	virtual void setLevel(Level* theLevel);
	// 0x14: 0x459150: the events and the map's boat spawners are let go, and the level with them
	virtual void clearLevel();
	// 0x18: 0x457290: every counter and list of a level starts over; translated next
	virtual void reset() {}
	// 0x1c: 0x456c20: the introduction is over: the hud's vehicles are shown and play starts
	virtual void startPlay();
	// 0x20: 0x456320: the level is over: the music stops and the tips go to the score; translated next
	virtual void finishLevel() {}
	// 0x2c: 0x456300: the game is paused (the orders picked are let go and the tip jar's roll stops) or not
	virtual void setPaused(bool thePaused);
	// 0x34: 0x458c40 (shared): the orders picked are let go; translated next
	virtual void clearPicks() {}
	// 0x38: 0x457490 (speed), 0x45b6e0 (concentration): the customers' waves, and what they wait for; translated next
	virtual void updateWaves(engine::UpdateInfo& theInfo) {}
	// 0x3c: 0x455fe0: the map's update (its tiles, boats and vehicles)
	virtual void updateMap(engine::UpdateInfo& theInfo);
	// 0x40: 0x456ac0: the level ends when the happiness is gone, or when the customers and the vehicles are done;
	// translated next
	virtual void checkLevelEnd() {}
	// 0xa0: 0x4580f0: the level's customers start; translated next
	virtual void beginWaves() {}
	// 0x48: 0x456130 (speed), 0x45b370 (concentration): the right button; translated next
	virtual void rightClick() {}

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
	engine::FloatPair mSpawnDelay;             // +0x78: the wave's seconds between one customer and the next
	engine::FloatPair mWaitTime;               // +0x80: the wave's seconds a customer waits
	float mSlowFactor;                         // +0xb4: how much of a frame's time passes while the time is slowed
	float mSlowTime;                           // +0xb8: the seconds the time stays slowed
	engine::Sound* mSlowSound;                 // +0xbc: the voice "special_timeEffect", which plays while it is
	Topping* mComboTopping;                    // +0xf8: the topping of the last pizza delivered

	// 0x138 of the first pizza being made (the list at +0x48): translated next
	void updatePizzas(engine::UpdateInfo& theInfo);
	// 0x417cb0: the events of the level are made; translated next
	void makeEvents();
	// 0x44b3c0 on each of them: the events are let go; translated next
	void clearEvents();
	// 0x455cb0: the right button is pressed
	void onRightDown(engine::Point& thePoint) { rightClick(); }
	// 0x459330: the right button's signal is connected to onRightDown
	void connectRightButton();
};

} // namespace pizzafrenzy

#endif
