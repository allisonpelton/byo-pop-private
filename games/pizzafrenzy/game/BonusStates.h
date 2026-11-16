#ifndef __PIZZAFRENZY_BONUSSTATES_H__
#define __PIZZAFRENZY_BONUSSTATES_H__

// The states of the bonus round (BonusGame.h): the next pizza comes in, the player plays it, its toppings are scored,
// and at the end the round's title shows (the introduction is the speed mode's, ModeStates.h); and the application's
// state that holds the round (0x402a40).

#include "BonusGame.h"
#include "PizzaFrenzy.h"
#include "State.h"

namespace pizzafrenzy
{

// The player's turn (0x45c290, constructor 0x45c290, vtable 0x500f64)
class BonusPlayState : public engine::State
{
public:
	explicit BonusPlayState(BonusGame* theGame) : mGame(theGame), mVoice(nullptr), mHurryPlayed(false) {}  // 0x45c290

	// 0x04: 0x460390: the game takes the clicks, the waiting sound loops, and the right button takes a topping back
	void enter() override;
	// 0x08: 0x45dd10
	void exit() override;
	// 0x0c: 0x45c5e0: the time runs, and at five seconds the hurry sound plays; the pizza is scored when it is out
	void update(engine::UpdateInfo& theInfo) override;
	// 0x10: 0x460010: a button of a topping picks it
	void action(const std::string& theAction) override;

private:
	BonusGame* mGame;       // +0x0c
	engine::Sound* mVoice;  // +0x10: the sound that waits, which loops, and the hurry
	bool mHurryPlayed;      // +0x14
};

// The scoring of a pizza (0x24 bytes, constructor 0x45c320): each topping of the target is scored in turn, a tenth of
// a second apart, and the pizzas go out
class BonusScoreState : public engine::State
{
public:
	// 0x45c320
	explicit BonusScoreState(BonusGame* theGame) : mGame(theGame), mTimer(0), mStage(STAGE_START), mIndex(0) {}

	// 0x04: 0x460040: the seconds left are counted
	void enter() override;
	// 0x0c: 0x45eb00
	void update(engine::UpdateInfo& theInfo) override;

private:
	enum
	{
		STAGE_START = 0,   // the first topping is scored after half a second
		STAGE_SCORING = 1, // a topping at a time
		STAGE_LEAVING = 2, // the pizzas turn to go out
		STAGE_OUT = 3      // the pizzas go out, and the next pizza or the end follows
	};

	BonusGame* mGame;  // +0x0c
	float mTimer;      // +0x10
	int mStage;        // +0x14
	size_t mIndex;     // +0x18: the target topping that is scored next (an iterator in the original)
};

// The next pizza coming in (0x38 bytes, constructor 0x45e030): the target pizza comes in, its toppings show one by
// one, and the player's pizza with the buttons follows
class BonusNextPizzaState : public engine::State
{
public:
	explicit BonusNextPizzaState(BonusGame* theGame) :
		mGame(theGame), mTimer(0), mStage(0), mShown(0), mCount(0)  // 0x45e030
	{
	}

	// 0x04: 0x45d030: the pizza is the next of the level's (the end when there is none)
	void enter() override;
	// 0x0c: 0x45d9e0
	void update(engine::UpdateInfo& theInfo) override;

private:
	BonusGame* mGame;  // +0x0c
	float mTimer;      // +0x20
	int mStage;        // +0x24
	int mShown;        // +0x28: the target toppings shown
	int mCount;        // +0x2c: the target toppings
};

// The round's end (0x24 bytes, constructor 0x45c3c0): the title shows for two and a half seconds and goes for two,
// and the application is told the level is over
//
// Left out, as the speed mode's ModeEndState leaves it: the title (the burst text 0x42b800, made in 0x45d1b0 and let
// go in 0x45d4b0), which says the round is over (the string table's 0xda for a round with no seconds left, 0xd0 with
// a topping missed, 0xd1 with every topping perfect, 0xd9 otherwise)
class BonusEndState : public engine::State
{
public:
	explicit BonusEndState(BonusGame* theGame) : mGame(theGame), mTimer(0), mTitleLeaving(false) {}  // 0x45c3c0

	// 0x04: 0x45d1b0
	void enter() override;
	// 0x0c: 0x45d4b0
	void update(engine::UpdateInfo& theInfo) override;

private:
	BonusGame* mGame;     // +0x0c
	float mTimer;         // +0x14
	bool mTitleLeaving;   // +0x18
};

// The application's state while the round plays (0x18 bytes, constructor 0x402a40, vtable 0x4f9524): the decorating
// screen is shown, the game is saved, and each frame is the round's game's. The game's actions pause the round or end
// it (which unlocks the pizza designer's toppings).
class BonusGameState : public engine::State
{
public:
	explicit BonusGameState(PizzaFrenzy* theApp) : mApp(theApp) {}  // 0x402a40

	// 0x04: 0x4026e0
	void enter() override;
	// 0x0c: 0x402720
	void update(engine::UpdateInfo& theInfo) override;
	// 0x10: 0x407790
	void action(const std::string& theAction) override;

private:
	PizzaFrenzy* mApp;  // +0x0c
};

} // namespace pizzafrenzy

#endif
