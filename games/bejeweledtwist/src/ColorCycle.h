// A colour that runs through a list of colours (0x34 bytes, no virtuals; 0x6e6e1b-0x6e70ad). The class has no RTTI
// name; the port calls it ColorCycle.
#pragma once

#include "Color.h"

#include <vector>

namespace Sexy {

class ColorCycle {
public:
	Color mColor;                  // +0x00: the current colour
	float mPos;                    // +0x10: where in the list, 0 to 1
	float mSpeed;                  // +0x14: a hundredth of it per update
	float mBrighten = 0;           // +0x18: added to red, green and blue (times 255)
	float mAlpha = 1.0f;           // +0x1c
	bool mLoop = true;             // +0x20: from the last colour back to the first
	std::vector<Color> mColors;    // +0x24

	ColorCycle();                  // 0x6e707c: the six colours of the rainbow
	void Reset();                  // 0x6e6e1b
	void SetPosition(float thePos);  // 0x6e704e
	void Update();                 // 0x6e6e3e
};

}  // namespace Sexy
