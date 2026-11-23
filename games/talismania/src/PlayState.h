#ifndef __TALISMANIA_PLAYSTATE_H__
#define __TALISMANIA_PLAYSTATE_H__

// The application's play state (AtlantisApp::PlayState, vtable 0x73b8ec, the member at the app's +0x19c): while a game
// mode runs, each update goes to it (0x4068d0 forwards the update to the mode's current state, and 0x42eb20 to the
// application's own). Entering and leaving do nothing.

#include "AtlantisApp.h"
#include "GameMode.h"
#include "LoadingState.h"

class PlayState : public AppState
{
public:
	explicit PlayState(AtlantisApp* theApp) : AppState(theApp) {}

	// 0x4068d0
	void update(engine::UpdateContext* theContext) override
	{
		if (mApp->mGameMode != nullptr)
			mApp->mGameMode->update(theContext);
	}
};

#endif
