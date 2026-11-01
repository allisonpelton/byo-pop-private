#ifndef __BONNIES_FLOATTEXT_H__
#define __BONNIES_FLOATTEXT_H__

// The floating texts and images (0x42af30-0x42b610, the list 0x481ab0): a text or a frame that moves, spins, scales
// and changes colour over its life, then is freed.

#include "Str.h"

namespace bonnies
{

struct Font;
struct Frame;

struct FloatText
{
	// 0x42b080
	FloatText();
	// 0x42b0f0: moves it by the time since the last update (not while the game is paused)
	void update();
	// 0x42b1c0
	void draw();

	uint32_t mLastTime;   // +0x00
	int mStarted;         // +0x04
	float mX;             // +0x08
	float mY;             // +0x0c
	float mAngle;         // +0x10
	float mProgress;      // +0x14: 0 to 1 over its life
	uint32_t mColor1;     // +0x18: the text's colour at the start
	uint32_t mColor2;     // +0x1c: its second colour at the start
	float mStartX;        // +0x20
	float mStartY;        // +0x24
	float mScale1;        // +0x28: the scale at the start
	float mSpin;          // +0x2c: degrees a second
	float mAccelX;        // +0x30
	float mAccelY;        // +0x34
	float mSpeedX;        // +0x38
	float mSpeedY;        // +0x3c
	float mGravity;       // +0x40
	uint32_t mEndColor1;  // +0x44
	uint32_t mEndColor2;  // +0x48
	float mScale2;        // +0x4c: the scale at the end
	int mDuration;        // +0x50: in milliseconds
	int mFadeTime;        // +0x54: when it starts to fade (none when negative)
	uint32_t mFlags;      // +0x58: drawText's
	Str mText;            // +0x5c
	Font* mFont;          // +0x64
	FloatText* mNext;     // +0x68
	FloatText* mPrev;     // +0x6c
	const Frame* mFrame;  // +0x70: drawn instead of a text
};

class FloatTexts
{
public:
	// 0x42af30
	FloatTexts();
	// 0x42af40
	void draw();
	// 0x42af60: adds theText at the end
	void add(FloatText* theText);
	// 0x42afa0: frees them all
	void clear();
	// 0x42b000: updates them and frees the finished ones
	void update();

	FloatText* mHead;  // +0x00
	FloatText* mTail;  // +0x04
};

extern FloatTexts* gFloatTexts;  // 0x481ab0

} // namespace bonnies

#endif
