#ifndef __FEEDINGFRENZY_BACKGROUND_H__
#define __FEEDINGFRENZY_BACKGROUND_H__

// The node behind a stage that takes the mouse over its rectangle (BackgroundBackground, 0x124 bytes, constructor
// 0x42a340, vtable 0x4a5ce0) and gives each button's going down and coming up to the function its owner has set. The
// original's other two callbacks, for the mouse moving, are the screen saver's alone, and the stage's empty one
// (0x40dcf0) is left out.

#include "Node.h"

namespace feedingfrenzy
{

class Background : public engine::Container
{
public:
	// The buttons a callback is told of
	enum
	{
		BUTTON_LEFT = 0,
		BUTTON_RIGHT = 1
	};

	// The function a button's change goes to: the mouse's last place, the button, and what its owner gave with it
	typedef void (*ButtonCallback)(int theX, int theY, int theButton, void* theArg);

	Background(int theWidth, int theHeight);  // 0x42a340: the rectangle is the stage's whole

	void setDownCallback(ButtonCallback theCallback, void* theArg);  // 0x42a270
	void setUpCallback(ButtonCallback theCallback, void* theArg);    // 0x42a290

protected:
	void onMouseDownEvent(engine::MouseEvent& theEvent) override;   // 0x42a160
	void onMouseUpEvent(engine::MouseEvent& theEvent) override;     // 0x42a1a0
	void onRightDownEvent(engine::MouseEvent& theEvent) override;   // 0x42a0e0
	void onRightUpEvent(engine::MouseEvent& theEvent) override;     // 0x42a120
	void handleMouseMoveEvent(engine::MouseEvent& theEvent) override;  // 0x42a1e0: only the place is kept
	void onMouseExitEvent(engine::MouseEvent& theEvent) override;   // 0x42a250: a button held is let go

private:
	void callDown(int theButton);
	void callUp(int theButton);

	engine::Point mLastPoint;       // +0x11c: where the mouse was last
	ButtonCallback mDownCallback;   // +0x124
	void* mDownArg;                 // +0x128
	ButtonCallback mUpCallback;     // +0x12c
	void* mUpArg;                   // +0x130
	bool mButtonHeld;               // +0x144: a button is down
};

} // namespace feedingfrenzy

#endif
