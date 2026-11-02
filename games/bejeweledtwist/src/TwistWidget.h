// Sexy::TwistWidget (0x148 bytes, vtable 0x7f... 70 slots; 0x755cc9-0x756750): the widget most of the game's screens
// are made of. It can show a tooltip after the mouse rests on it, draw a picture behind its children and zoom in when
// it appears (in 2D a scale about its centre).
#pragma once

#include "Color.h"
#include "CurvedVal.h"
#include "Widget.h"

namespace Sexy {

class Image;

class TwistWidget : public Widget {
public:
	SexyString mToolTip;         // +0x88
	float mToolTipAlpha = 0;     // +0xa4: fades in while the mouse is over the widget
	float mToolTipDelay = 0;     // +0xa8: never set above 0, so the tooltip starts fading in at once
	bool mMouseOver = false;     // +0xac
	bool mNoticeClicks = false;  // +0xad: a click sets mClicked
	bool mFrozen = false;        // +0xae: Update does nothing
	int mUnknownB0 = -1;         // +0xb0
	Image* mImage = nullptr;     // +0xb4: drawn at 0, 0 in mImageColor
	Color mImageColor;           // +0xb8
	bool mUnknownC8 = false;     // +0xc8
	bool mClicked = false;       // +0xc9
	bool mUnknownCA = false;     // +0xca
	bool mFlushOverlays = false; // +0xcb: DrawAll first draws the manager's deferred overlays
	bool mZooming = false;       // +0xcc: mZoom runs
	bool mZoomIn2D = false;      // +0xcd: without it 2D mode skips the zoom
	CurvedVal mZoom;             // +0xd0: the scale while it appears
	float mAlpha = 1.0f;         // +0x140: of the tooltip
	const char* mName = nullptr; // +0x144: for the debug output

	TwistWidget();                                                  // 0x7565a3
	~TwistWidget() override;                                        // 0x75666f

	void Update() override;                                         // 0x755eec
	void Draw(Graphics* g) override;                                // 0x75602c
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;       // 0x755cd2
	void MouseEnter() override;                                     // 0x75668f
	void MouseLeave() override;                                     // 0x7566ec
	void MouseMove(int x, int y) override;                          // 0x755e92
	void MouseDown(int x, int y, int theClickCount) override;       // 0x755ebe
	using Widget::MouseDown;

	void ResetToolTipDelay() { mToolTipDelay = 0; }                 // 0x755cc9
};

}  // namespace Sexy
