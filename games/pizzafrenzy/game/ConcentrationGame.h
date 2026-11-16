#ifndef __PIZZAFRENZY_CONCENTRATIONGAME_H__
#define __PIZZAFRENZY_CONCENTRATIONGAME_H__

// The concentration mode's game (0x124 bytes, constructor 0x45b3e0, vtable 0x500e68, 47 slots; the application makes it
// for mode 2, the menu's "Simon Says Mode"). A wave's customers all come at once, their orders shown and their houses
// dimmed (no click, and no wait), for as long as the player has to look at them: a fifteenth of a wait drawn from the
// wave's range for each customer. Then every order is hidden behind a question mark and the customers' waits begin,
// and the player must remember what each customer wanted. A wave has at most eight customers, and a customer waits
// three times as long (the shared code of GameMode.cpp reads the mode for both). A looping sound, "popup_wait", plays
// while the orders show.

#include "SpeedGame.h"

namespace pizzafrenzy
{

class ConcentrationGame : public SpeedGame
{
public:
	ConcentrationGame() : mWaitSound(nullptr) {}  // 0x45b3e0
	// 0x00: 0x45b440 (the destructor, which the base's is)

	// 0x04: 0x45bbd0: the speed mode's setup, the voice "popup_wait" (it loops), and the right button's connection a
	// second time (so that a click of it takes the orders' time away twice)
	void setup(GameScreen* theScreen, CityMap* theMap, Hud* theHud, GameStats* theStats) override;
	// 0x30: 0x45b4f0: an event changed the orders: the sound plays if it is not, and the time to look at them is drawn
	// again for the customers whose houses take a click
	void ordersChanged() override;
	// 0x38: 0x45b6e0: the next wave comes when every house is idle; its customers are all placed as soon as there are
	// houses for them, and when the time to look is up the orders are hidden
	void updateWaves(engine::UpdateInfo& theInfo) override;
	// 0x48: 0x45b370: the speed mode's, and the time to look at the orders is over
	void rightClick() override;
	// 0xa8: 0x4529d0 (false)
	bool isSpeedMode() override { return false; }
	// 0xb4: 0x45ba10: the order is placed with its popup dimmed
	bool placeOrder(Order* theOrder) override;

	// port (the test commands' state): the seconds the orders still show; none (0) once they are hidden
	float getShowSecondsLeft() const { return mSpawnTimer; }

private:
	// 0x45b4f0's and 0x45b6e0's: the sound plays while the orders show, and the music is half as loud
	void startWaitSound();
	// 0x45b590: the orders of the customers waiting are hidden, the sound stops, and the speed bonus is the
	// customers' still to come
	void hideOrders();

	engine::Sound* mWaitSound;  // +0x118: the voice "popup_wait"
};

} // namespace pizzafrenzy

#endif
