// BoltObj: the bolt of light that runs down a pipe when an arrow of the rope reaches the end (0x405fb0..0x4066e0 in the
// original, 0x520 bytes). It is not an object of the scene itself but a set of 160 short pieces of one, each a 28 pixel
// picture of "images/bolt" turned and stretched to join the next, laid along the line between two places in steps of 16
// pixels and pushed sideways by a random zigzag (the bolt's shape, made again every few frames).
#pragma once

#include "ObjBase.h"

namespace Sexy
{

const int BOLT_PIECES = 160;

class BoltObj
{
public:
	bool mShown;                      // +0x0: the bolt has been drawn since it was last stopped
	int mCountdown;                   // +0x4: frames left before the shape is made again
	int mRedrawFrames;                // +0x8: frames between two shapes (the constructor's argument)
	float mFromX;                     // +0xc: the two ends
	float mFromY;                     // +0x10
	float mToX;                       // +0x14
	float mToY;                       // +0x18
	bool mHasEnds;                    // +0x1c: SetEnds has been called since the last Stop
	ObjBase* mPieces[BOLT_PIECES];    // +0x20
	// +0x2a0: how far each step of the line is pushed sideways, in units of 24 pixels. The original's array ends at
	// +0x520 and its fill writes two places past the last step; the port has room for them.
	float mOffsets[BOLT_PIECES + 2];

	explicit BoltObj(int theRedrawFrames); // 0x405fb0
	void Stop();                           // 0x406280 (EAX the bolt)
	void SetEnds(int theFromX, int theFromY, int theToX, int theToY); // 0x4062b0 (EAX the bolt)
	void UpdateBolt();                     // 0x4062e0 (EAX the bolt)

private:
	void MakeShape(int theSteps);          // 0x406070 (EBX the bolt)
};

} // namespace Sexy
