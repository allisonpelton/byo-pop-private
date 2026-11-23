#ifndef __FEEDINGFRENZY2_STAGEINTROSTATE_H__
#define __FEEDINGFRENZY2_STAGEINTROSTATE_H__

// The application's state before a stage is played (FeedingFrenzyTwo::StageIntroState, 0x18 bytes, vtable 0x55ae74
// (feedingfrenzy2); its constructor 0x49d1b0 (feedingfrenzy2)): the stage's intro screen shown, then the play, the
// options or the question whether to quit the game, by the screen's buttons. Its exit, suspend and resume do nothing.

#include "FeedingFrenzyTwo.h"

class StageIntroState : public AppState
{
public:
	explicit StageIntroState(FeedingFrenzyTwo* theApp) : AppState(theApp) {}

	// 0x04 (0x494170 (feedingfrenzy2)): the intro screen set for the game's stage and shown
	void enter() override;
	// 0x0c (0x421cd0 (feedingfrenzy2)): the sound system's update (0x434c80 (feedingfrenzy2)): not yet translated
	void update(engine::UpdateContext* theContext) override {}
	// 0x10 (0x498c20 (feedingfrenzy2)): stageStartContinue plays the stage, stageStartOptions opens the options over
	// the intro, stageStartQuit asks whether to leave the game
	void action(const std::string& theAction) override;
};

#endif
