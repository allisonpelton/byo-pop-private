#ifndef __FEEDINGFRENZY2_LEVELLOADSTATE_H__
#define __FEEDINGFRENZY2_LEVELLOADSTATE_H__

// The application's state while a stage loads (FeedingFrenzyTwo::LevelLoadState, 0x38 bytes, vtable 0x55a890
// (feedingfrenzy2); its constructor 0x492920 (feedingfrenzy2)): a load screen shown (the map for a game's stage, the
// plain load screen for a menu's), then the stage loaded by its loader (Stage::StageLoader, a thread of its own in the
// original; the port loads at once, as the splash does) while the screen's bar follows, then the stage's intro or the
// play. Translated so far: the screens, the load, the bar, the stage put back under the screens and the intro; the
// music, the pause screen's title and the play are not yet translated.

#include "FeedingFrenzyTwo.h"
#include "Image.h"

// Where the stage's loader is (the loader's +0x10)
enum StageLoadStatus
{
	STAGE_LOAD_NOT_STARTED = 0,
	STAGE_LOAD_RUNNING = 1,
	STAGE_LOAD_FAILED = 2,   // the stage's file could not be read (the play goes on to the next state all the same)
	STAGE_LOAD_DONE = 3
};

class LevelLoadState : public AppState
{
public:
	// 0x492920 (feedingfrenzy2): the stage's entry, and the state to go to once it is loaded, null for the stage's
	// intro or its play
	LevelLoadState(FeedingFrenzyTwo* theApp, const StageManifest::Stage* theEntry, AppState* theNextState);

	// 0x04 (0x409910 (feedingfrenzy2)): the load's frame rate, the sounds stopped, a game's stage kept in the
	// player's record, and the load screen shown with its bar emptied
	void enter() override;
	// 0x08 (0x48ddd0 (feedingfrenzy2)): the game's frame rate again, the sounds let go of
	void exit() override;
	// 0x0c (0x455b40 (feedingfrenzy2)): the load started once the screen is shown, the bar filled, then the next
	// state
	void update(engine::UpdateContext* theContext) override;
	// (0x20 (0x4a4830 (feedingfrenzy2)), the loader told to stop and waited for while it runs, is not needed: the
	// port's load is over before the state can leave)

private:
	// 0x496cf0 (feedingfrenzy2): the stage taken out from under the screens and its loader started
	void startLoading();
	// The loader's run (0x48a810 (feedingfrenzy2), on its own thread): the stage hidden and unloaded, the actors'
	// library read again, the stage loaded from the entry and given its time and canSuck, then shown
	void load();

	bool mLoading;                         // +0x10: the loader started
	const StageManifest::Stage* mEntry;    // +0x14
	float mShownProgress;                  // +0x1c: the bar's, 0 to 1
	int mLoadStatus;                       // +0x20: the loader (a Ref), here its +0x10, a StageLoadStatus
	engine::GenericScreen* mScreen;        // +0x24: the load screen
	engine::Image* mLoadBar;               // +0x28: the screen's "loadBar", null for none
	engine::Ref<AppState> mNextState;      // +0x2c
};

#endif
