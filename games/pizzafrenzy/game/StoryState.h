#ifndef __PIZZAFRENZY_STORYSTATE_H__
#define __PIZZAFRENZY_STORYSTATE_H__

// The state that plays a level's story (0x20 bytes, constructor 0x402ba0, vtable 0x4f954c): the story screen is shown
// over the story's music and, once the screen manager is idle, the story starts. When the story is done the level goes
// on to the toppings it gives; when the player ends the game from the story, the game is over and the menu comes back.

#include "Level.h"
#include "PizzaFrenzy.h"
#include "State.h"

namespace pizzafrenzy
{

class StoryState : public engine::State
{
public:
	// 0x402ba0: the level the story is of
	StoryState(PizzaFrenzy* theApp, Level* theLevel) :
		mApp(theApp), mStarted(false), mStoryDone(false), mGameEnded(false), mLevel(theLevel)
	{
	}

	// 0x04: 0x406560
	void enter() override;
	// 0x0c: 0x408360
	void update(engine::UpdateInfo& theInfo) override;
	// 0x10: 0x4038a0
	void action(const std::string& theAction) override;

private:
	PizzaFrenzy* mApp;  // +0x0c
	bool mStarted;      // +0x10: the story has started on the screen
	bool mStoryDone;    // +0x11: the story screen said "storyDone"
	bool mGameEnded;    // +0x12: the story screen said "endGame"
	Level* mLevel;      // +0x14: nothing reads it
};

} // namespace pizzafrenzy

#endif
