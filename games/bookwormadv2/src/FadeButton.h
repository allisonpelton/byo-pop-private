// Sexy::FadeButton (0x1a8 bytes, constructor 0x436e60, vtable 0x6fa374; its PopAnimListener part at +0x120): the
// game's button. A picture of three or four cels (normal, lit, pressed, disabled) that fades into its new look
// when the mouse comes, goes, presses or lets go (each one chosen by a flag), can glow on its own, can be drawn
// stretched to its size, and draws its label in its first colour, or its second while the mouse is over it.
// It can play an animation over itself, again and again (SetEffect: the battle's attack button glows while a word
// can attack; the treasure screen's continue button, while it can be pressed).
// port: the tooltip it keeps is not ported: the constructor makes an empty TooltipWidget (+0x18c) as a child, which
// MouseEnter and MouseLeave show and hide (0x4370d0, 0x437130), SetDisabled hides (0x437b00), a resize puts above the
// button (0x437990, 0x437a30) and the manager keeps in front (0x437960, 0x437ae0), but each of those does something
// only to a tooltip with a line of text or a font, and nothing in the exe gives a button's one either (no code
// reaches the field but these, and no script can reach a FadeButton), so it never shows.
#pragma once

#include <string>

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
	bool mEffectOff = true;          // +0x12d: the animation neither plays nor shows
	int mGlowAlpha = 0;              // +0x130
	int mGlowMax = 0;                // +0x134
	int mGlowMin = 0;                // +0x138
	int mGlowDelta = 0;              // +0x13c
	int mEffectX = 0;                // +0x140: where the animation is drawn, from the button's corner
	int mEffectY = 0;                // +0x144
	Rect mEffectRect;                // +0x148: the animation is clipped to it (from mEffectX, mEffectY) unless empty
	PopAnim* mEffect = nullptr;      // +0x158
	std::string mEffectLabel;        // +0x15c: where it plays from
	Widget* mOwner = nullptr;        // +0x178, the widget that made it (the book's mini-game buttons)
	bool mFadeOnDisable = false;     // +0x17c: each of these starts the fade from 0 when it happens
	bool mFadeOnEnter = false;       // +0x17d
	bool mFadeOnLeave = false;       // +0x17e
	bool mFadeOnDown = false;        // +0x17f
	bool mFadeOnUp = false;          // +0x180
	int mFadeEvery = 1;              // +0x184, updates between fade steps
	bool mDrawnByOwner = false;      // +0x181: Draw skips it, the owner calls DrawFade (translated to the button)
	int mFadeSpeed = 27;             // +0x188, alpha a step
	Rect mSavedDisabledRect;         // +0x194: the board's (ScrambleUseNormalForDisabled)
	int mLabelOffsetY = 0;           // +0x1a4

	FadeButton(int theId, ButtonListener* theListener);
	~FadeButton() override;  // 0x437020

	// 0x49f420: the game's usual button, on a picture of cels side by side (normal, lit, pressed and, when it
	// has a fourth, disabled), sized to a cel, with FONT_LARGE_OUTLINE unless it has a font, its label white
	// and yellow while lit, and every fade on at 20 a step. With no picture: as wide as its label.
	void SetupImage(Image* theImage);
	// 0x437c60: the animation (a .pam) that plays over the button, from theLabel, drawn at (-31, theY); off until
	// EnableEffect turns it on
	void SetEffect(const std::string& theFileName, int theY, const std::string& theLabel);
	// 0x437d40: back to its first frame when theReset; playing from its label when theOn, else off
	void EnableEffect(bool theReset, bool theOn);
	// the animation goes on or holds where it is (the board's pause and focus write PopAnim::mPlaying)
	void SetEffectPlaying(bool isPlaying);
	// 0x4373b0: the button (and its animation), translated to its place when mDrawnByOwner
	void DrawFade(Graphics* g);

	// Widget
	void Update() override;                  // 0x437240
	void Draw(Graphics* g) override;         // 0x437380: DrawFade unless the owner draws it
	void SetDisabled(bool isDisabled) override;  // 0x437b00
	void MouseEnter() override;              // 0x4370d0
	void MouseLeave() override;              // 0x437130
	void MouseMove(int x, int y) override;   // 0x437d90: the owner hears it too, in its own coordinates
	using ButtonWidget::MouseDown;
	using ButtonWidget::MouseUp;
	void MouseDown(int x, int y, int theClickCount) override;  // 0x437180
	void MouseUp(int x, int y) override;                       // 0x4371c0, before the button's MouseUp

	// PopAnimListener: the animation reached a frame that stops it (listener slot 0, 0x437d20): it plays again from
	// its label while it is on
	void PopAnimStopped(int theId) override;

	// ButtonWidget: 0x437b70, stretched when mStretch
	void DrawButtonImage(Graphics* g, Image* theImage, const Rect& theRect, int x, int y) override;

private:
	void FadeStep();  // 0x437200
	void DrawLook(Graphics* g);
};

}  // namespace Sexy
