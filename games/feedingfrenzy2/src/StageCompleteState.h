#ifndef __FEEDINGFRENZY2_STAGECOMPLETESTATE_H__
#define __FEEDINGFRENZY2_STAGECOMPLETESTATE_H__

// The application's state after a stage is won (FeedingFrenzyTwo::StageCompleteState, 0x20 bytes, vtable 0x55ae2c
// (feedingfrenzy2); its constructor 0x4a4660 (feedingfrenzy2)): the stage's end shown and the food bank kept, the game
// moved on to the next stage (or, after the last, the mode's furthest stage set and its saved game cleared); then the
// next stage, the menu after the last, the options or the question whether to quit the game, by the screen's buttons.
// Its suspend and resume do nothing.

#include "FeedingFrenzyTwo.h"

class StageCompleteScreen;

class StageCompleteState : public AppState
{
public:
	explicit StageCompleteState(FeedingFrenzyTwo* theApp) : AppState(theApp), mScreen(nullptr), isGameFinished(false)
	{
	}

	// 0x04 (0x49aef0 (feedingfrenzy2))
	void enter() override;
	// 0x08 (0x4a4890 (feedingfrenzy2)): the screen let go of
	void exit() override;
	// 0x0c (0x4915f0 (feedingfrenzy2)): while the window has the focus, the sound system's update (0x434c80
	// (feedingfrenzy2), not yet translated) and the screen's count
	void update(engine::UpdateContext* theContext) override;
	// 0x10 (0x49e150 (feedingfrenzy2)): stageCompleteContinue goes on, stageCompleteOptions opens the options over the
	// screen, stageCompleteQuit asks whether to leave the game, and the debug buttons' actions step the fun facts
	void action(const std::string& theAction) override;

private:
	engine::Ref<StageCompleteScreen> mScreen;  // +0x10: the normal stage's end, or the bonus stage's
	bool isGameFinished;                       // +0x14: the stage was the mode's last
};

#endif
