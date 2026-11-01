// Sexy::TwistButton (0x15c bytes, 75 slots; 0x7540d9-0x754506): the game's buttons. The button image is a strip of up
// to four cels (normal, over, down, disabled) stretched over the button, tinted with mColor, with an icon and a label
// centred on it; the mouse-over sound plays on entering.
#pragma once

#include "ButtonWidget.h"
#include "Color.h"

namespace Sexy {

class TwistButton : public ButtonWidget {
public:
	bool mDeferDraw = false;        // +0x128: drawn as an overlay at mDeferPriority
	int mDeferPriority = 0;         // +0x12c
	bool mPlayMouseOverSound = true;  // +0x130
	bool mHighlighted = false;      // +0x131: the over cel without the mouse
	Color mColor;                   // +0x134
	Image* mIcon = nullptr;         // +0x144
	int mIconOffsetX = 0;           // +0x148
	int mIconOffsetY = 0;           // +0x14c
	const char* mName = nullptr;    // +0x150: for the debug output
	int mLabelOffsetX = 0;          // +0x154
	int mLabelOffsetY = 0;          // +0x158

	TwistButton(int theId, Image* theImage, ButtonListener* theListener);  // 0x754172
	~TwistButton() override;                                               // 0x7540d9

	void Update() override;                // 0x75411e
	void Draw(Graphics* g) override;       // 0x754123
	void DrawOverlay(Graphics* g) override;  // 0x754147
	using Widget::DrawOverlay;
	void MouseEnter() override;            // 0x75448d
	void MouseLeave() override;            // 0x754506

	virtual void DrawButton(Graphics* g);  // 0x754273 (slot 73)
	virtual void PlayMouseOverSound();     // 0x7540e4 (slot 74)
};

}  // namespace Sexy
