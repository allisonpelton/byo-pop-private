#ifndef __FEEDINGFRENZY2_GAMEPLAYSTATE_H__
#define __FEEDINGFRENZY2_GAMEPLAYSTATE_H__

// The application's state while a stage is played (FeedingFrenzyTwo::GamePlayState, 0x1c bytes, vtable 0x55a8c8
// (feedingfrenzy2); its constructor 0x49b390 (feedingfrenzy2)): the stage started and run each frame, paused by the
// pause popup (the window losing the focus brings it up too), and once the stage's own machine has no state left the
// stage's end: complete (or a bonus stage over), or lost, with continues left or none.

#include "FeedingFrenzyTwo.h"

class GamePlayState : public AppState
{
public:
	explicit GamePlayState(FeedingFrenzyTwo* theApp) : AppState(theApp), mSpeed(NORMAL_SPEED) {}

	// 0x04 (0x499130 (feedingfrenzy2)): the stage started at the normal speed, the screens taken away and the stage
	// marked in play
	void enter() override;
	// 0x08 (0x4a1eb0 (feedingfrenzy2)): the engine's mouse let go (its slot 8)
	void exit() override;
	// 0x0c (0x434d70 (feedingfrenzy2)): without the focus the pause popup; else the frame's time slowed by the speed
	// and the stage run, and once the stage's machine is empty its end
	void update(engine::UpdateContext* theContext) override;
	// 0x10 (0x4a0a80 (feedingfrenzy2)): pause, reload (the stage loaded again) and slowmo (the speed's next step)
	void action(const std::string& theAction) override;
	// 0x14 (0x4a1670 (feedingfrenzy2)): left for a popup, the stage paused
	void suspend() override;
	// 0x18 (0x5361c9 (feedingfrenzy2)): back from a popup, the stage going on and marked in play again
	void resume() override;
	// 0x1c (0x499730 (feedingfrenzy2)): P, Esc and Space pause; F pauses and switches between the window and the full
	// screen (not yet translated). (R, S and M are tested for and do nothing.)
	void keyDown(int theKey) override;

private:
	// The speed the play starts at, and the slowest the slowmo action steps to before it starts again
	static constexpr float NORMAL_SPEED = 1.0f;
	static constexpr float SLOWEST_SPEED = 16.0f;

	// 0x4a0ac0 (feedingfrenzy2): the pause screen as a popup over the play
	void showPause();
	// 0x543631 (feedingfrenzy2): the continues left in the offer to go on, shown as a popup in the play's place
	void showContinue();
	// 0x543747 (feedingfrenzy2): the mode's saved game cleared, and the game over shown as a popup in the play's place
	// with the score, the player's rank header and the rank the food bank earns
	void showGameOver();
	// 0x4a1e80 (feedingfrenzy2): the engine's mouse taken (its slot 4): the cursor hidden and its motion steering
	void acquireMouse();

	float mSpeed;      // +0x10: what the frame's time is divided by, 1 to 16 by the slowmo action
};

#endif
