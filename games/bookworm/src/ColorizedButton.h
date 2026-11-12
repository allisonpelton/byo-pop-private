#ifndef __BOOKWORM_COLORIZEDBUTTON_H__
#define __BOOKWORM_COLORIZEDBUTTON_H__

// ColorizedButton.cpp: an image button whose images are drawn colorized by a colour per state (normal,
// over, down, disabled) and that repaints on every hover and press change. Used by the level-up / game
// over panel's continue button (LevelUpWidget, id 6).
// Original: class Sexy::ColorizedButton : public ButtonWidget, sizeof 0xf0 (new in
// LevelUpWidget::AddedToManager 0x42bb10), vftable 0x4fbf7c (53 slots: ButtonWidget's 49 + 49..52).
// Offsets are absolute (the 2004 ButtonWidget is 0xe0 bytes). Behaviour spec: re/bookworm/analysis/screens.md.

#include "ButtonWidget.h"

#include <vector>

namespace Sexy
{

class Graphics;
class ButtonListener;

class ColorizedButton : public ButtonWidget
{
public:
	enum
	{
		COLOR_IMAGE_NORMAL = 0,
		COLOR_IMAGE_OVER = 1,
		COLOR_IMAGE_DOWN = 2,
		COLOR_IMAGE_DISABLED = 3
	};

	std::vector<Color> mImageColors;        // +0xe0 image colour per state (first +0xe4, last +0xe8, end +0xec); ctor: 4 x opaque white

public:
	ColorizedButton(int theId, ButtonListener* theListener);        // 0x41bf50
	virtual ~ColorizedButton();                                     // 0x41b650 (scalar deleting dtor 0x41bd60) slot 0
	virtual void	Draw(Graphics* g) override;                     // 0x41b6a0 slot 16
	virtual void	MouseEnter() override;                          // 0x41b3e0 slot 23
	virtual void	MouseLeave() override;                          // 0x41b400 slot 24
	virtual void	MouseDown(int x, int y, int theClickCount) override;     // 0x41b450 slot 27
	virtual void	MouseUp(int x, int y) override;                 // 0x41b420 slot 30
	virtual void	SetImageColors(int theColors[][4], int theNumColors); // 0x41bed0 slot 49
	// 0x41be50 slot 50 SetImageColors (3 values a colour) and 0x41b510 slot 51 SetImageColor: never called, not ported
	virtual const Color& GetImageColor(int theIdx);                 // 0x41b550 slot 52

	using ButtonWidget::MouseDown;
	using ButtonWidget::MouseUp;
};

}

#endif
