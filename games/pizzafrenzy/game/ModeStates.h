#ifndef __PIZZAFRENZY_MODESTATES_H__
#define __PIZZAFRENZY_MODESTATES_H__

// The states of a mode's game (GameMode.h): a level starts with its introduction (the titles come in, stay, and go
// out), and then plays; the level ends with the prize blimp when every pizza was delivered, and with its title, which
// the application takes on to the level's results or the game's end.

#include "GameMode.h"
#include "PrizeBlimp.h"
#include "State.h"

namespace pizzafrenzy
{

// The introduction (0x34 bytes, constructor 0x459220, vtable 0x500d1c): the node "titles" fades in, stays and fades
// out, and a click on the screen skips what is left of its wait
class ModeIntroState : public engine::State, public engine::HasSlots
{
public:
	explicit ModeIntroState(GameMode* theMode) : mMode(theMode), mTimeLeft(0), mStage(STAGE_WAIT) {}  // 0x459220

	// 0x04: 0x45a1a0: the titles are clear, and a click skips the wait
	void enter() override;
	// 0x08: 0x456540: the titles are hidden
	void exit() override;
	// 0x0c: 0x4564a0
	void update(engine::UpdateInfo& theInfo) override;

private:
	enum
	{
		STAGE_WAIT = 0,     // half a second before the titles come in
		STAGE_SHOWN = 1,    // three seconds with the titles in
		STAGE_FADING = 2    // a second for them to go out, and then play starts
	};

	// 0x455bb0: a click while the wait or the titles' stay is still on ends it
	void onMouseDown(engine::Point& thePoint);

	GameMode* mMode;  // +0x0c
	float mTimeLeft;  // +0x20
	int mStage;       // +0x28
};

// Play (0x18 bytes, constructor 0x4565d0, vtable 0x500c50): each frame the mode's waves, its map and its end check
class ModePlayState : public engine::State
{
public:
	explicit ModePlayState(GameMode* theMode) : mMode(theMode) {}  // 0x4565d0

	// 0x0c: 0x455be0
	void update(engine::UpdateInfo& theInfo) override;

private:
	GameMode* mMode;  // +0x0c
};

// The prize blimp's flight (0x20 bytes, constructor 0x456730, vtable 0x500c78): the kitchens' popups hang up, the
// coupon buttons leave, and the blimp flies over the city dropping tips; the level ends when it has gone, its seven and
// a half seconds are over and no tip is left on the map
class PrizeBlimpState : public engine::State
{
public:
	explicit PrizeBlimpState(GameMode* theMode) : mMode(theMode), mTimeLeft(0) {}  // 0x456730

	// 0x04: 0x4588b0
	void enter() override;
	// 0x0c: 0x456f40
	void update(engine::UpdateInfo& theInfo) override;

private:
	GameMode* mMode;                 // +0x0c
	engine::Ref<PrizeBlimp> mBlimp;  // +0x10
	float mTimeLeft;                 // +0x14
};

// The level's title (0x24 bytes, constructor 0x456660, vtable 0x500c64): the tip jar's roll stops, the level is
// finished, and the game screen stops taking the mouse while the title shows; then the application is told "levelEnd",
// or "gameOver" when the happiness ran out
//
// Left out: the title itself, the burst text (class 0x42b800, made in 0x458490 and let go in 0x45c240), which says the
// level is complete (the string table's 0xd0, with the prize blimp's flight 0xd1) or the game is over (0xd2)
class ModeEndState : public engine::State
{
public:
	// 0x456660: the game is over when the flag is set
	ModeEndState(GameMode* theMode, bool theGameOver) :
		mMode(theMode), mTimeLeft(0), mTitleLeaving(false), mGameOver(theGameOver)
	{
	}

	// 0x04: 0x458490
	void enter() override;
	// 0x0c: 0x4587a0
	void update(engine::UpdateInfo& theInfo) override;

private:
	GameMode* mMode;      // +0x0c
	float mTimeLeft;      // +0x14
	bool mTitleLeaving;   // +0x18: the title has been shown and goes
	bool mGameOver;       // +0x19
};

} // namespace pizzafrenzy

#endif
