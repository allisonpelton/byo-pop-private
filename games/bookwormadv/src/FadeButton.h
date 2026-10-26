// Sexy::FadeButton (0x1a8 bytes, constructor 0x441df0, vtable 0x7ac3f4; its PopAnimListener part at +0x120): the
// game's button. A picture of three or four cels (normal, lit, pressed, disabled) that fades into its new look
// when the mouse comes, goes, presses or lets go (each one chosen by a flag), can glow on its own, can be drawn
// stretched to its size, and draws its label in its first colour, or its second while the mouse is over it.
// port: the tooltip it keeps (a TooltipWidget at +0x18c, shown above it while the mouse is over it) and the
// animation it can play over itself (a PopAnim at +0x158) are not ported yet; nothing ported gives it either.
#pragma once

#include "ButtonWidget.h"
#include "PopAnim.h"

namespace Sexy {

class Image;

class FadeButton : public ButtonWidget, public PopAnimListener {
public:
	int mFade = 255;                 // +0x124, the new look's alpha over the old
	bool mClicked = false;           // +0x128, let go of while the mouse was on it (until it comes back)
	bool mGlow = false;              // +0x12a, glows on its own (additively, between mGlowMin and mGlowMax)
	bool mStretch = false;           // +0x12b, the cel drawn stretched to the button's size
	bool mHideMouseWhenDisabled = false;  // +0x12c
	int mGlowAlpha = 0;              // +0x130
	int mGlowMax = 0;                // +0x134
	int mGlowMin = 0;                // +0x138
	int mGlowDelta = 0;              // +0x13c
	Widget* mOwner = nullptr;        // +0x178, the widget that made it (the book's mini-game buttons)
	bool mFadeOnDisable = false;     // +0x17c: each of these starts the fade from 0 when it happens
	bool mFadeOnEnter = false;       // +0x17d
	bool mFadeOnLeave = false;       // +0x17e
	bool mFadeOnDown = false;        // +0x17f
	bool mFadeOnUp = false;          // +0x180
	int mFadeEvery = 1;              // +0x184, updates between fade steps
	int mFadeSpeed = 27;             // +0x188, alpha a step
	int mLabelOffsetY = 0;           // +0x1a4

	FadeButton(int theId, ButtonListener* theListener);

	// 0x4be490: the game's usual button, on a picture of cels side by side (normal, lit, pressed and, when it
	// has a fourth, disabled), sized to a cel, with FONT_LARGE_OUTLINE unless it has a font, its label white
	// and yellow while lit, and every fade on at 20 a step. With no picture: as wide as its label.
	void SetupImage(Image* theImage);

	// Widget
	void Update() override;                  // 0x4421d0
	void Draw(Graphics* g) override;         // 0x442310, 0x442340
	void SetDisabled(bool isDisabled) override;  // 0x442a90
	void MouseEnter() override;              // 0x442060
	void MouseLeave() override;              // 0x4420c0
	using ButtonWidget::MouseDown;
	using ButtonWidget::MouseUp;
	void MouseDown(int x, int y, int theClickCount) override;  // 0x442110
	void MouseUp(int x, int y) override;                       // 0x442150, before the button's MouseUp

	// ButtonWidget: 0x442b10, stretched when mStretch
	void DrawButtonImage(Graphics* g, Image* theImage, const Rect& theRect, int x, int y) override;

private:
	void FadeStep();  // 0x442190
};

}  // namespace Sexy
