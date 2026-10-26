// Sexy::FlashWidget (the framework's, 0x527ec0-0x529000) together with the game's MyFlashWidget (0x4824d0, vtable
// 0x7af71c): a widget that plays a Flash movie. The original hosted the Flash ActiveX control in it; the port plays
// the movie with SwfMovie (swf/) into an image of the widget's size, one movie frame every 100 / frame rate
// updates, so a test run plays the same frames every time.
#pragma once

#include <string>

#include "SwfMovie.h"
#include "Widget.h"

namespace Sexy {

class MemoryImage;

// The framework's FlashListener (vtable 0x7ac28c in the cut scene)
class FlashListener {
public:
	virtual ~FlashListener() {}
	// The movie stopped by itself (and the widget is not paused)
	virtual void FlashAnimEnded(int theId) {}
	// The movie's FSCommand: "ready", "done" for the game's cut scenes
	virtual void FlashCommand(int theId, const std::string& theCommand, const std::string& theParam) {}
};

class FlashWidget : public Widget, public SwfListener {
public:
	enum { STATE_IDLE, STATE_PLAYING, STATE_STOPPED };

	int mState = STATE_IDLE;                // +0x88
	int mId;                                // +0x8c
	FlashListener* mFlashListener;          // +0x90
	int mPauseCount = 0;                    // +0xb4
	bool mRedraw = true;                    // +0xcc: redrawn every frame (the cut scene's End stops it)

	FlashWidget(int theId, FlashListener* theListener);
	~FlashWidget() override;

	// 0x528240: loads the movie (a path in the game's folder) and plays it from its first frame, unless paused.
	// False when it cannot be played; the widget then stays idle.
	bool StartAnimation(const std::string& theFileName);
	// 0x528400 / 0x528450: pauses count, the movie plays when none are left
	void Pause();
	void Unpause();

	void Update() override;  // 0x528a50
	void Draw(Graphics* g) override;  // 0x528be0
	// MyFlashWidget's (0x482540): the mouse-ups go to the widget that shows the movie (the cut scene) too
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;

	void SwfCommand(const std::string& theCommand, const std::string& theArgs) override;

	Widget* mMouseOwner = nullptr;          // MyFlashWidget's +0x100

private:
	SwfMovie mMovie;
	MemoryImage* mImage = nullptr;
	int mFrameTime = 0;      // port: updates times the frame rate since the last frame
	bool mImageStale = true; // port: a frame went by since the image was drawn
};

}  // namespace Sexy
