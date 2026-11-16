#ifndef __PIZZAFRENZY_BONUSGAME_H__
#define __PIZZAFRENZY_BONUSGAME_H__

// The bonus round's game (0x354 bytes, constructor 0x45f830, vtable 0x5010b8, 49 slots; the application makes it for
// the level that is a bonus round, which closes each city, and keeps it at +0x110). It plays on the decorating screen
// (res/screenLayouts/decoratePizza.xml, the application's +0xf4): the level lists pizzas (Level::mPizzas), and for each
// one the target pizza comes in with its toppings, the buttons of the toppings it has come in, and the player has the
// seconds the level gives for all of them (Level::mNumOrders) to put the toppings where the target has them. A
// button's click picks its topping, which follows the mouse as a cursor; a left click near the player's pizza puts it
// there (a number of the button counts down), and a right click takes the topping under it back. When the buttons
// are all at 0 (or the time is out), each topping of the target is scored by the nearest topping of the same kind
// the player put, and the next pizza comes. The score is in the stats (GameStats::mPerfectToppings and the other
// counts, mBonusSecondsLeft), which the summary (StoryScreen::setUpBonusSummary) pays.
//
// The slots of GameMode that the round does not override are the speed mode's, which nothing reaches here.
//
// Left out: the pizza designer and the editor, which share the screen and the topping class; the slot 0xa4 (0x45c500,
// which ends the round at once), which only the debug flag at 0x532908 calls. Lorenzo's text is made (0x45e140) and
// hidden, and nothing types it.

#include "GameMode.h"
#include "Interpolator.h"
#include "Pizza.h"
#include "PizzaTopping.h"
#include "Ramp.h"
#include "TextItem.h"
#include "TextTyper.h"
#include "ToppingButton.h"

#include <map>
#include <string>
#include <vector>

namespace pizzafrenzy
{

class BonusGame : public GameMode
{
public:
	BonusGame();           // 0x45f830
	~BonusGame() override;  // 0x45fa80, 0x45f010

	// 0x04: 0x459950 (the speed mode's, which the round does not reach: setStats is its setup)
	void setup(GameScreen* theScreen, CityMap* theMap, Hud* theHud, GameStats* theStats) override {}
	// 0x08: 0x45bd20: the timer group's motion, the stats' time and the states
	void update(engine::UpdateInfo& theInfo) override;
	// 0x0c: 0x45bcc0: the screen's action (a topping's button) goes to the state
	void action(const std::string& theAction) override;
	// 0x10: 0x45c700: the level's time, its pizzas, and its music
	void setLevel(Level* theLevel) override;
	// 0x14: 0x45bd90: the toppings on the pizzas are let go
	void clearLevel() override;
	// 0x1c: 0x45ea90: the introduction is over: the first pizza comes, and the timer comes in
	void startPlay() override;
	// 0x20: 0x460080: the music stops and no topping is picked
	void finishLevel() override;
	// 0x2c: 0x45ffa0: a pause lets go of the topping picked, and the end of it picks it again
	void setPaused(bool thePaused) override;

	// 0xbc: 0x45e140: the screen and the stats it plays on, their nodes and motions, and the introduction
	void setStats(GameScreen* theScreen, GameStats* theStats);
	// 0xc0: 0x4604e0: the target pizza's toppings and the buttons of the pizza are made, and its name shown
	void beginPizza(Pizza* thePizza);

	// the place of a node on the screen: its place and the places of the nodes it is in (the original's slot 0x10 adds
	// them up when asked)
	static engine::Point getAbsPosition(engine::Node* theNode);
	// 0x45fd50: the topping becomes the one the cursor shows (none lets go of the cursor)
	void selectTopping(Topping* theTopping);
	// 0x45c570: the pizza is done: its toppings are scored
	void endPizza();
	// 0x45c8c0: the target topping at the place is scored against the toppings the player put; false when it was
	// the last of them
	bool scoreTopping(size_t theIndex);
	// 0x45c6b0: every button is at 0
	bool allToppingsPlaced();
	// 0x45cf60: the button's number is shown, and the button is dimmed at 0
	void updateButton(Topping* theTopping);

	// What the round's phases are (the state that plays), for the test commands
	enum Phase
	{
		PHASE_INTRO,      // the titles show
		PHASE_ARRIVE,     // the next pizza and its buttons come in
		PHASE_PLAY,       // the player puts toppings
		PHASE_SCORE,      // the toppings are scored
		PHASE_END         // the round's end
	};
	Phase getPhase();

	float getTimeLeft() const { return mTimeLeft; }
	int getPizzasLeft() const { return mPizzasLeft; }
	bool isPlaying() const { return mPlaying; }
	Topping* getSelectedTopping() { return mSelectedTopping; }
	engine::Container* getUserGroup() { return mUserToppingGroup; }
	const std::vector<engine::Ref<PizzaTopping>>& getTargetToppings() const { return mTargetToppings; }
	const std::vector<engine::Ref<PizzaTopping>>& getUserToppings() const { return mUserToppings; }
	// the button of a topping, null for a topping the pizza does not have
	ToppingButton* getButton(Topping* theTopping);
	const std::map<Topping*, engine::Ref<ToppingButton>, ToppingOrder>& getButtons() const { return mToppingButtonMap; }

private:
	friend class BonusPlayState;
	friend class BonusScoreState;
	friend class BonusNextPizzaState;
	friend class BonusEndState;

	// 0x45be50: the ramps' times, and the motions of the pizzas, the timer and the buttons
	void setUpMotions();
	// 0x45f3a0: the target pizza's toppings, hidden
	void makeTargetToppings(Pizza* thePizza);
	// 0x4600e0: a button for each topping of the pizza, with the number of them it has
	void makeButtons(Pizza* thePizza);
	// 0x45f550: a topping of the picked kind is put on the player's pizza at the place
	void placeTopping(const engine::Point& thePlace, Topping* theTopping);
	// 0x45fe70: the topping of the player's nearest to the place, within 15 pixels, is taken back and picked
	void removeTopping(const engine::Point& thePlace);
	// 0x45bdb0: the cursor follows the mouse, and is small while the mouse is far from the player's pizza
	void onMouseMove(engine::Point& thePoint);
	// 0x45faa0: a click puts the picked topping on the player's pizza
	void onMouseDown(engine::Point& thePoint);
	// 0x460340: a click of the right button takes a topping back
	void onRightClick(engine::Point& thePoint);

	// the originals' own names: the pizzas and the toppings go on the groups of the screen
	engine::Ref<engine::Container> mUserPizza;             // +0x128: "userPizza"
	engine::Point mUserPizzaPlace;                         // +0x12c
	engine::Ref<engine::Container> mUserToppingGroup;      // +0x134: "userToppings"
	engine::Ref<engine::Container> mTargetPizza;           // +0x148: "targetPizza"
	engine::Ref<engine::Container> mTargetToppingGroup;    // +0x14c: "targetToppings"
	engine::Point mTargetPizzaPlace;                       // +0x150
	std::vector<engine::Ref<PizzaTopping>> mUserToppings;  // +0x11c: the ones the player put
	std::vector<engine::Ref<PizzaTopping>> mTargetToppings;  // +0x13c
	engine::Ref<engine::TextItem> mTimerText;              // +0x158: "timer"
	engine::Ref<engine::TextItem> mPizzaCountText;         // +0x15c: "pizzaCount"
	engine::Ref<engine::TextTyper> mLorenzoText;           // +0x160
	engine::Ref<engine::Container> mLorenzo;               // +0x164: "Lorenzo"
	engine::Ref<engine::Container> mToppingButtons;        // +0x16c: "toppingButtons"
	engine::Ref<Topping> mSelectedTopping;                 // +0x170
	engine::Ref<Topping> mLastTopping;                     // +0x174: the one picked when the game was paused
	engine::Ref<engine::ImageNode> mCursorTopping;         // +0x178
	engine::Ref<engine::TextItem> mCaption;                // +0x17c: "caption"
	engine::Ref<engine::Container> mTimerGroup;            // +0x180: "timerGroup"
	std::map<Topping*, engine::Ref<ToppingButton>, ToppingOrder> mToppingButtonMap;  // +0x184
	size_t mNextPizza;     // +0x190: the pizza of the level's list that comes next (an iterator in the original)
	float mTimeLeft;       // +0x194: the seconds the player has
	int mPizzasLeft;       // +0x198: the pizzas that have not come
	bool mPlaying;         // +0x19c: the player may put toppings

	engine::PositionInterpolator mTargetMotion;   // +0x1a0: the target pizza's way in
	engine::Ramp mTargetRamp;                     // +0x1f0
	engine::PositionInterpolator mUserMotion;     // +0x214: the player's pizza's
	engine::Ramp mUserRamp;                       // +0x264
	engine::PositionInterpolator mTimerMotion;    // +0x288: the timer group's
	engine::Ramp mTimerRamp;                      // +0x2d8
	engine::ScaleInterpolator mButtonsMotion;     // +0x2fc: the buttons' scale
};

} // namespace pizzafrenzy

#endif
