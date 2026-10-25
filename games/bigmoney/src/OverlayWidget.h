#ifndef __BIGMONEY_OVERLAYWIDGET_H__
#define __BIGMONEY_OVERLAYWIDGET_H__

// OverlayWidget.cpp: the full-screen left bar frame and the floating logo drawn over the game
// selector; GameSelector fades the frame with mAlpha (the logo stays opaque).
// Original: class Sexy::OverlayWidget : public Widget, sizeof 0x90 (new in the GameSelector ctor
// 0x4107c0, GameSelector::mOverlay), vftable 0x4fb96c (54 slots; overrides 0 ~Widget, 13 Draw,
// 44 IsPointVisible = `return true` stub). Offsets are absolute (Widget is 0x80 bytes in 1.2).
// Behaviour spec: re/bigmoney/analysis/board_draw.md section 5.

#include "Widget.h"

namespace Sexy
{

class WinBMApp;
class Graphics;

class OverlayWidget : public Widget
{
public:
	WinBMApp*		mApp;                   // +0x80
	// +0x84 pad
	double			mAlpha;                 // +0x88 left bar alpha 0..1 (GameSelector); ctor 1.0

public:
	OverlayWidget(WinBMApp* theApp);                                // 0x417400
	virtual ~OverlayWidget();                                       // (scalar deleting dtor 0x4174d0) slot 0

	virtual void	Draw(Graphics* g) override;                              // 0x417440 slot 13
};

}

#endif
