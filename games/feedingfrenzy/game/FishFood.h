#ifndef __FEEDINGFRENZY_FISHFOOD_H__
#define __FEEDINGFRENZY_FISHFOOD_H__

// The game's controller, FishFood (0x260 bytes, constructor 0x430ee0, vtable 0x4a6640). It is the application's
// controller (engine::App's +0x188): the run loop asks it for the window's configuration (configure, 0x42b8e0), then
// has it set the game up (init, 0x430b80).

#include "App.h"

#include <string>

namespace feedingfrenzy
{

// The window and the screen it shows (the controller's configure, 0x42b8e0, says them)
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int FRAME_RATE = 30;
const int WINDOW_ICON = 5;

class FishFood : public engine::Controller
{
public:
	FishFood();            // 0x430ee0
	~FishFood() override;  // 0x431310

	void configure(engine::AppConfig& theConfig) override;  // 0x04: 0x42b8e0
	void init() override;                                   // 0x08: 0x430b80
	void shutdown() override;
};

} // namespace feedingfrenzy

#endif
