#ifndef __PIZZAFRENZY_MODESTATES_H__
#define __PIZZAFRENZY_MODESTATES_H__

// The states of a mode's game (GameMode.h): a level starts with its introduction (the titles come in, stay, and go
// out), and then plays.

#include "GameMode.h"
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

} // namespace pizzafrenzy

#endif
