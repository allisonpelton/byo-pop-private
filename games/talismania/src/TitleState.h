#ifndef __TALISMANIA_TITLESTATE_H__
#define __TALISMANIA_TITLESTATE_H__

// The title state (AtlantisApp::TitleState, vtable 0x73b8bc, enter 0x406e00): the main menu shown in the scene's "skymenu"
// layer. Translated so far: the main menu screen made from the options' layout and shown; the menu stacks, the other
// front-end screens (0x4039b0 makes them all), the hero mode's button state and the screen's actions are not yet.

#include "GenericScreen.h"
#include "LoadingState.h"

class TitleState : public AppState
{
public:
	explicit TitleState(AtlantisApp* theApp) : AppState(theApp) {}

	// 0x406e00
	void enter() override;
};

#endif
