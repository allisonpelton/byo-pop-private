// Sexy::Background (bejeweled3, vtable 0x863c0c, 0x310 bytes, made by 0x77dbb0): the animated picture behind the game screen. It is a
// widget (z-order -1) that owns a PopAnim (+0x12c), loaded from images\1200\backgrounds\<name>\<name>.pam, plays it from its first frame
// and draws it. The board makes one in its background setter (slot 164, 0x749c20) and adds it to the widget manager.
// port: the exe loads the animation on a worker thread (0x77f090 by the thread routine at 0x77f860, with the events at +0xc4 and +0xc8
// and a second thread at +0xd8 that loads the sounds of the animation's "playsample" commands, 0x77e8a0); the port loads it at once.
#pragma once

#include "CurvedVal.h"
#include "Widget.h"

#include <string>

namespace Sexy {

class PopAnim;

class Background : public Widget {
public:
	PopAnim* mAnim = nullptr;        // +0x12c (param_1[0x4b]): the animation, null before it is loaded
	bool mStarted = false;           // +0x130 (param_1[0x4c]): the animation has been put on its first frame and runs
	CurvedVal mSpeedCurve;           // +0x1a8: the speed the animation runs at (the constant 1 from the constructor, 0x77dbb0)
	float mFrameFraction = 0.0f;     // +0x1a0 (param_1[0x68]): the speed's sum; each time it goes above 1 the animation's Update runs and 1 comes off

	explicit Background(const std::string& theFileName);   // 0x77dbb0 (bejeweled3)
	~Background() override;
	// slot 22, 0x77f8e0 (bejeweled3): the animation is started once it is loaded and then run
	void Update() override;
	// slot 26, 0x77eb50 (bejeweled3): the animation, or black when there is none yet
	void Draw(Graphics* g) override;
};

}  // namespace Sexy
