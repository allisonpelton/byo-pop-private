// Sexy::PlayButton (0x2e0 bytes, a TwistButton; 0x699435-0x69cd1e): the round button of the loader screen and the
// main menu. It draws the menu's rings and the turning spiral, with sparkles drifting into the middle, and the
// button's own image on top. Its owner sets the three curves that bring it in.
#pragma once

#include "CurvedVal.h"
#include "TwistButton.h"

#include <vector>

namespace Sexy {

class PlayButton : public TwistButton {
public:
	// A sparkle circling into the middle
	struct Sparkle {
		float mAngle;   // radians
		float mRadius;  // from the middle
		float mSpeed;   // the radius's change per update (negative)
		float mAlpha;
	};

	int mButtonId;                   // +0x160: the constructor's arguments again
	Image* mImage;                   // +0x164: drawn on top, cels 0 and 2
	ButtonListener* mButtonListener; // +0x168
	float mRotation = 0;             // +0x16c: the spiral's turn
	float mSpinSpeed = 1.0f;         // +0x170: the sparkles' speed, eased towards 1
	CurvedVal mAlpha;                // +0x178: the whole button's opacity
	CurvedVal mAppear;               // +0x1e8: its size, 1 when it is fully in
	CurvedVal mImageAppear;          // +0x258: the size of mImage
	float mGlow = 0;                 // +0x2c8: the highlight while the mouse is over it
	std::vector<Sparkle> mSparkles;  // +0x2cc
	bool mClickPending = false;      // +0x2dc: clicked while coming in: the click happens once it is in

	PlayButton(int theId, Image* theImage, ButtonListener* theListener);  // 0x69cbff
	~PlayButton() override;                                                // 0x69cc94

	void Update() override;                                                // 0x69cd1e
	void Draw(Graphics* g) override;                                       // 0x69a13c
	void MouseUp(int x, int y, int theClickCount) override;                // 0x6994b5
	using TwistButton::MouseUp;
	bool IsPointVisible(int x, int y) override;                            // 0x699435

	// 0x699516: the menu's rings about theX, theY, at mAlpha
	void DrawRings(Graphics* g, int theX, int theY, float theOuterRot, float theInnerRot, float theScale);
};

}  // namespace Sexy
