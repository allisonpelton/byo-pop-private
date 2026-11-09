// Sexy::Gates (0x3d0 bytes, a TwistWidget and a ButtonListener; 0x6fead4-0x700413, app +0xc70): the main menu's
// two halves around the spiral button, with the title above and five buttons on the lower half. Open, the halves
// slide apart off the screen (with sparks along their seams); closed, they lock together and the menu works.
#pragma once

#include "ButtonListener.h"
#include "CurvedVal.h"
#include "TwistWidget.h"

namespace Sexy {

class Logo;
class MenuButtonWidget;

// Sexy::TopGate and Sexy::BottomGate (0x150 bytes, TwistWidgets): the halves. They draw nothing (the Gates draw
// IMAGE_MAINMENU_UI_MERGED's cels where they are) and hold the buttons.
class TopGate : public TwistWidget {
public:
	Image* mGateImage = nullptr;  // +0x148
	TopGate();                    // 0x6fe5d9
};

class BottomGate : public TwistWidget {
public:
	Image* mGateImage = nullptr;  // +0x148
	BottomGate();                 // 0x6fe5f8
};

class Gates : public TwistWidget, public ButtonListener {
public:
	CurvedVal mPos;               // +0x150: the halves' opening; drives the others
	CurvedVal mSlide;             // +0x1c0: the inner rings' slide (90 at 1600x1200)
	CurvedVal mSparks;            // +0x230: how many sparks per update
	CurvedVal mOuterGlow;         // +0x2a0: the glow of the halves' edges
	CurvedVal mInnerGlow;         // +0x310: the glow of the inner rings
	TopGate* mTop;                // +0x380
	BottomGate* mBottom;          // +0x384
	int mOpenCount = 0;           // +0x390: set to 0 by the constructor, never read
	float mRestFraction = 0;      // +0x394: 0.85 whenever the halves stand still (never read)
	int mTopY = 0;                // +0x398: the top half's y when closed
	int mBottomY = 0;             // +0x39c
	float mOffset;                // +0x3a0: how far the halves are apart
	float mSpeed = 0;             // +0x3a4
	bool mOpen = true;            // +0x3a8
	float mFlash = 0;             // +0x3ac: an additive flash over the inner rings, fading out
	MenuButtonWidget* mHelpButton;     // +0x3b0: "more games" when the properties give a MoreGamesURL
	MenuButtonWidget* mProfileButton;  // +0x3b4
	MenuButtonWidget* mScoresButton;   // +0x3b8
	MenuButtonWidget* mOptionsButton;  // +0x3bc
	MenuButtonWidget* mQuitButton;     // +0x3c0
	bool mButtonsOn;              // +0x3c4: the buttons' covers open once the gates are shut
	Logo* mLogo;                  // +0x3c8
	bool mFadeLogo = false;       // +0x3cc: the title fades while the gates open

	Gates();            // 0x700413
	~Gates() override;  // 0x6fea23

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6fed9d
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6fee1e
	void Update() override;                                             // 0x6ff4f7
	void Draw(Graphics* g) override;                                    // 0x6fef03
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x6fee3d
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x6fead4
	using Widget::Resize;

	void ButtonPress(int theId, int theClickCount) override;  // 0x6ff368: to the menu
	void ButtonDepress(int theId) override;                   // 0x6ff38c: to the menu
	using ButtonListener::ButtonPress;

	void SetButtonsOn(bool on);              // 0x6ff3e6
	void SetOpen(bool theOpen, bool theNow); // 0x6ffeb3
	static int GetClosedTopY();              // 0x6fee2a: the top half's offset when open
};

}  // namespace Sexy
