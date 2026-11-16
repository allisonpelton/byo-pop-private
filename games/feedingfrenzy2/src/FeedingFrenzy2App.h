#ifndef __FEEDINGFRENZY2_FEEDINGFRENZY2APP_H__
#define __FEEDINGFRENZY2_FEEDINGFRENZY2APP_H__

// The port's host for Feeding Frenzy 2 (no original counterpart: the original was the engine's Windows layer, its run
// 0x4383b0 (feedingfrenzy2)). The engine runs one frame per call, so the framework drives it: the widget runs a frame
// once the frame's time has come (the run loop's test of the clock), shows the 800x600 screen and turns the
// framework's input into the engine's signals (as the window procedure did). The engine's platform services are
// engines/sprout2/host's EngineImpl.

#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdint>

namespace engine
{
class EngineImpl;
class Surface;
}

namespace Sexy
{

class MemoryImage;

class FeedingFrenzy2Widget : public Widget
{
public:
	uint64_t mNextFrame;  // the run loop's time for the next frame, in the engine's ticks
	// The screen as the framework draws it: the display's pixels copied after each frame
	MemoryImage* mScreenImage;

	FeedingFrenzy2Widget();
	~FeedingFrenzy2Widget() override;

	void Update() override;
	void Draw(Graphics* g) override;
};

class FeedingFrenzy2App : public SexyAppBase
{
public:
	FeedingFrenzy2Widget* mWidget;
	engine::EngineImpl* mEngine;
	// The display's back buffer, which each frame is drawn into (the engine's +0x2a4, a DirectDraw surface)
	engine::Surface* mDisplay;
	bool mEngineRunning;
	int mFrames;  // port: the engine's frames
	uint64_t mLastFrameTicks;  // the clock when the last frame began (the engine's +0x2c8)

	FeedingFrenzy2App();
	~FeedingFrenzy2App() override;

	void Init() override;
	void Shutdown() override;

	// A frame of the engine (0x438700 (feedingfrenzy2))
	void RunFrame();
	// The run loop's frame period in the engine's ticks (the engine's +0x160: the clock's rate divided by the frame
	// rate)
	uint64_t FramePeriod() const;
};

extern FeedingFrenzy2App* gApp;

} // namespace Sexy

#endif
