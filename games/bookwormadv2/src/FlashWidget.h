// Sexy::FlashWidget (the framework's, 0x50f300-V1:0x529000) together with the game's MyFlashWidget (0x472a60, vtable
// 0x6fdb8c): a widget that plays a Flash movie. The original hosted the Flash ActiveX control in it; the port plays
// the movie with SwfMovie (swf/) into an image of the widget's size, one movie frame every 100 / frame rate
// updates, so a test run plays the same frames every time.
#pragma once

#include <string>

#include "SwfMovie.h"
#include "Widget.h"

namespace Sexy {

class MemoryImage;

// The framework's FlashListener (vtable 0x6fa20c in the cut scene)
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

	// V1:0x528240: loads the movie (a path in the game's folder) and plays it from its first frame, unless paused.
	// False when it cannot be played; the widget then stays idle.
	bool StartAnimation(const std::string& theFileName);
	// 0x50f750 / 0x50f7a0: pauses count, the movie plays when none are left
	void Pause();
	void Unpause();

	// the Flash control's TCurrentLabel and TGotoLabel on the movie (_level0): the label the movie is at, and a jump to
	// a label (false when the movie has none by that name); the movie goes on as it was, playing or stopped
	std::string GetCurrentLabel() const { return mMovie.GetCurrentLabel(); }
	bool GotoLabel(const std::string& theLabel);

	void Update() override;  // 0x50fa10
	void Draw(Graphics* g) override;  // 0x50fba0
	// MyFlashWidget's (0x472ab0): the mouse-ups go to the widget that shows the movie (the cut scene) too
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
