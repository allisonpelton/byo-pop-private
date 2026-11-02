// Sexy::OrbitText (0xd4 bytes, a Widget; vtable 0x7e7c2c, 0x724e91-0x72585a): a text that fades in and out a few
// times in the colours of the rainbow. The challenge menu's planet shows one as its tip for a night challenge's new
// record (ChallengeButton::MakeTip). With Direct3D the text runs along a ring of strips (0x725059-0x725672, not
// ported); in 2D it is drawn flat with a shadow.
#pragma once

#include "ColorCycle.h"
#include "Common.h"
#include "Rect.h"
#include "Widget.h"

namespace Sexy {

class Font;
class TextImage;

class OrbitText : public Widget {
public:
	TextImage* mText = nullptr;  // +0x88: the text, 16 pixels of room round it
	bool mDone = false;          // +0x8c: faded out for the last time (mText is gone)
	float mPos = -1.0f;          // +0x90: runs from -1 to 1 for each showing; drawn while below 1
	float mSpeed = 0.01f;        // +0x94: mPos's step an update
	ColorCycle mCycle;           // +0x98: the text's colour
	float mAlpha = 0;            // +0xcc
	int mCount = 1;              // +0xd0: the showings left

	// 0x725732: theText in theFont, the widget theRect's size, centred on theRect's corner
	OrbitText(Font* theFont, const SexyString& theText, const Rect& theRect);
	~OrbitText() override;  // 0x725812

	void Update() override;          // 0x724e91
	void Draw(Graphics* g) override; // 0x724f7c
};

}  // namespace Sexy
