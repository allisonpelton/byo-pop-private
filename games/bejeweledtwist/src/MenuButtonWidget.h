// Sexy::MenuButtonWidget (0x238 bytes, a TwistButton; 0x69d3d9-0x69dea8): the buttons of the main menu and its
// panels. A button sits under a cover (a strip of cels, the doors) that opens while mGlowOn is set and closes again;
// it can flip to a second image (the profile button's "continue" picture) and show a number under its label.
#pragma once

#include "CurvedVal.h"
#include "TwistButton.h"

namespace Sexy {

class WinTwistApp;

class MenuButtonWidget : public TwistButton {
public:
	SexyString mString160;          // +0x160: (not yet seen in use)
	SexyString mToolTip;            // +0x17c: what the menu says about the button
	WinTwistApp* mApp;              // +0x19c
	bool mShowCount = false;        // +0x1a0: mCount is drawn under the label
	int mCount = 0;                 // +0x1a4
	float mScale = 1.0f;            // +0x1a8: of the offset of the image in the button (not of the image)
	bool mGlowOn = false;           // +0x1ac: the cover opens (and the mouse is noticed)
	float mGlow = 0;                // +0x1b0: how far the cover is open
	Image* mCover = nullptr;        // +0x1b4: the cover's cels, from closed to open
	Image* mImages[2] = {};         // +0x1b8: the button's image and its flip side (mFlipped picks one)
	CurvedVal mFlip;                // +0x1c0: the cover closes and opens again while the image flips
	bool mFlipped = false;          // +0x230

	MenuButtonWidget(int theId, Image* theImage, ButtonListener* theListener);  // 0x69d634
	~MenuButtonWidget() override;                                              // 0x69d6dd

	void Update() override;              // 0x69d48b
	void Draw(Graphics* g) override;     // 0x69d706
	void MouseEnter() override;          // 0x69d5ff
	Rect GetInsetRect() override;        // 0x69d613

	void SetFlipImage(Image* theImage) { mImages[1] = theImage; }      // 0x69d3d9
	void SetGlowOn(bool on) { mGlowOn = on; mMouseVisible = on; }     // 0x69d3e9
	void Flip();                                                       // 0x69d3fc: shows the other image
	// 0x69dea8: shows the flip side (or the image) by the cover's curve, or at once
	void SetFlipped(bool theFlipped, bool theNow);
};

}  // namespace Sexy
