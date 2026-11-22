// Sexy::MainMenu (vtable 0x86c35c (bejeweled3), 0x7a0 bytes, made by 0x6a54b0): the first screen. It is the
// loading screen too: until the loading thread ends it shows "Loading...", and the player's name under the picture.
// This is the 2D mode's menu: the flat background, the PLAY, RECORDS and OPTIONS buttons and the texts.
// port: the original's curves (the logos' fade, the buttons' arrival), the particles, the tips, the 3D branch and the
// buttons' crystal balls (CrystalBall, 0x78ab50, with their effects) are not translated yet: the buttons are plain.
#pragma once

#include "Widget.h"

#include <string>

namespace Sexy {

// One of the three menu buttons. port: stands in for the original's CrystalBall (vtable 0x86e2c4?, 0x678 bytes,
// 0x78ab50), whose drawing is not translated: it shows its text, lit while the mouse is over it.
class MainMenuButton : public Widget {
public:
	std::string mText;                  // the button's text, from the properties' string ids 0x11d to 0x11f
	bool mHighlighted = false;          // the mouse is over it

	explicit MainMenuButton(const std::string& theText);

	void Draw(Graphics* g) override;
	void MouseEnter() override;
	void MouseLeave() override;
};

class MainMenu : public Widget {
public:
	MainMenuButton* mPlayButton = nullptr;      // +0x6e4 (param_1[0x1b9]): string id 0x11f
	MainMenuButton* mOptionsButton = nullptr;   // +0x6e8 (param_1[0x1ba]): string id 0x11d
	MainMenuButton* mRecordsButton = nullptr;   // +0x6ec (param_1[0x1bb]): string id 0x11e

	MainMenu();                                 // 0x6a54b0 (bejeweled3)
	~MainMenu() override;

	void Draw(Graphics* g) override;            // 0x6a0ca0 (bejeweled3)
	void Update() override;                     // 0x6a3530 (bejeweled3)
};

}  // namespace Sexy
