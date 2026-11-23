#ifndef __SPROUT2_MOUSE_H__
#define __SPROUT2_MOUSE_H__

// The mouse a game steers by (engine::Mouse; the engine's slot 0x100 gives it, the window's dxdriver::MessageMouse at
// the engine's +0x2a8 (feedingfrenzy2)). While acquired, the cursor is hidden and held at the window's centre: each
// frame its move away from the centre is a motion, and the buttons are told by number (0 left, 1 right) instead of
// through the engine's mouse signals.

#include "Geometry.h"
#include "sigslot.h"

namespace engine
{

class Mouse
{
public:
	virtual ~Mouse() {}

	// 0x04 (0x4a1f00 (feedingfrenzy2)): the cursor hidden, held in the window and put at its centre; whether
	// it is acquired
	virtual bool acquire() = 0;
	// 0x08 (0x4a1ed0 (feedingfrenzy2)): the cursor given back as it was
	virtual void release() = 0;
	// 0x0c (0x4ff067 (feedingfrenzy2)): the button held now
	virtual bool isButtonDown(int theButton) = 0;

	sigslot::signal1<const Vector2<int>&> mMotionSignal;  // +0x0c: the move since the last, in the window's pixels
	sigslot::signal1<int> mButtonDownSignal;              // +0x1c: the button pressed
	sigslot::signal1<int> mButtonUpSignal;                // +0x2c: the button let go
};

} // namespace engine

#endif
