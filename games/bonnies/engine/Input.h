#ifndef __BONNIES_INPUT_H__
#define __BONNIES_INPUT_H__

// The input state (0x42b610-0x42b7a0, the object 0x481ab4): the mouse and the keyboard as SDL reports them, and a
// mouse position the game can pin (mPinned).

#include <cstdint>

namespace bonnies
{

class Input
{
public:
	// 0x42b610
	Input();
	// 0x42b640: whether the left button is down
	static bool leftButtonDown();
	// 0x42b650: the mouse position in screen fractions
	void getMousePos(float* theX, float* theY);
	// 0x42b6f0: whether theKey (an SDL keysym) is down
	static bool isKeyDown(int theKey);
	// 0x42b730: whether any key is down
	static bool anyKeyDown();
	// 0x42b760, 0x42b780
	float mouseX();
	float mouseY();

	uint32_t mDownTime;  // +0x00: when a button last went down or up
	float mDownX;      // +0x04: and where
	float mDownY;      // +0x08
	float mPinnedX;    // +0x0c
	float mPinnedY;    // +0x10
	int mSpareFirst;   // +0x14: cleared at the start; nothing reads it
	int mSpareSecond;  // +0x18: cleared at the start; nothing reads it
	int mSpareThird;   // +0x1c: cleared at the start; nothing reads it
	int mPinned;       // +0x20: getMousePos answers mPinnedX, mPinnedY
	int mSkipMotion;   // +0x24: the next mouse motion is dropped
	int mSpareBlock[0x13d]; // +0x28: cleared at the start; nothing reads it
};

extern Input* gInput;  // 0x481ab4

} // namespace bonnies

#endif
