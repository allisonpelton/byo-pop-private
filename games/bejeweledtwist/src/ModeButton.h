// Sexy::ModeButton (0x1c0 bytes, a TwistButton; 0x7207a8-0x7212b3, constructor 0x721c3b): one of the mode select's four
// buttons (Mandala). Behind a door that slides open when the mode is unlocked it shows the mode's picture with its tag
// (the saved game's level, the challenges' share) under it; it takes clicks only while the door is open.
#pragma once

#include "TwistButton.h"

namespace Sexy {

class ModeButton : public TwistButton {
public:
	SexyString mTagText;         // +0x160: under the picture (the saved level, the challenges' share)
	float mBrightness = 1.0f;    // +0x17c: of the button's colour, from 64 to 255
	int mMode = 0;               // +0x180: the game mode it starts
	int mPictureId = -1;         // +0x184: the picture over the button (a resource id), -1 none
	int mTagId = -1;             // +0x188: the tag beside mTagText (a resource id), -1 none
	SexyString mDescription;     // +0x18c: the mode select's text while the mouse is over it
	bool mHidden = false;        // +0x1a8: Update does nothing (until the doors first open)
	Image* mDoor;                // +0x1ac: IMAGE_PLAYSUBMENU_DOOR, its cels from shut to open
	int mButtonId = -1;          // +0x1b0: the button image's resource id
	int mDoorState = 0;          // +0x1b4: 0 shut, 1 opening, 2 open, 3 closing
	int mDoorCel = 0;            // +0x1b8
	float mDoorOpen = 0;         // +0x1bc: from 0 shut to 1 open

	ModeButton(int theId, Image* theImage, ButtonListener* theListener);  // 0x721c3b

	void Update() override;                              // 0x7207f5
	void Draw(Graphics* g) override;                     // 0x7212b3
	void MouseDown(int x, int y, int theClickCount) override;  // 0x7207df: only while the door is open
	using TwistButton::MouseDown;
	void PlayMouseOverSound() override;                  // 0x7207a8

	void SetDoorOpen(bool theOpen, bool theNow);         // 0x720910
};

}  // namespace Sexy
