#ifndef __PIZZAFRENZY_SPLASHSTATE_H__
#define __PIZZAFRENZY_SPLASHSTATE_H__

// The first state (0x1c bytes, constructor 0x402860, vtable 0x4f94e8): the splash screen shows for a second, and then
// the game is loaded one stage a frame, each frame the screen manager is not switching. Each stage makes what its
// field of the application is still missing (a manifest, a screen and its transitions) and widens the splash
// screen's load bar to its share. The stages, in order, with the load bar's share each leaves (the stores to the
// frame's argument slot, which the export does not show: 0x40a43f to 0x40bec5):
//   the tile manifest 0.05, the main menu 0.1, the options 0.15, the pause menu 0.2, the three confirmations of
//   quitting, leaving a game and deleting a player 0.25, the restore confirmation 0.3, the game's screens and managers
//   0.4, the decorating and designer screens 0.43, the pizzas 0.44, the player's pizza screens 0.45, the story 0.55,
//   the designer's unlock notice 0.6, the player select 0.65, the new player entry 0.7, the high scores 0.75, the
//   global post 0.8, the topping screens 0.85, the music player 0.9, the speed levels 0.95 and the city select 1.0,
//   after which the next state starts. The port has the stages but the global post (which is online), which has a
//   comment in its place.

#include "State.h"

#include <string>

namespace engine
{
class GenericScreen;
class Transition;
}

namespace pizzafrenzy
{

class PizzaFrenzy;

class SplashState : public engine::State
{
public:
	SplashState(PizzaFrenzy* theApp) : mApp(theApp), mDelay(0) {}  // 0x402860

	// 0x04: 0x402400: the splash screen, at once
	void enter() override;
	// 0x0c: 0x40a330
	void update(engine::UpdateInfo& theInfo) override;

private:
	// The stages' common steps: the screen's layout, its transitions and its actions to the application
	void setUpScreen(engine::GenericScreen* theScreen, const char* theLayout, engine::Transition* theIn,
	                 engine::Transition* theOut);
	// The slide most of the dialogs come in and go out by: from above the screen, in 0.2 seconds
	static engine::Transition* makeDialogSlide();
	// The slide of the three confirmations: from below the screen
	static engine::Transition* makeConfirmationSlide();
	// The same slide, from the point
	static engine::Transition* makeSlide(float theFromX, float theFromY);

	PizzaFrenzy* mApp;  // +0x0c
	float mDelay;       // +0x10: the splash screen's second
};

} // namespace pizzafrenzy

#endif
