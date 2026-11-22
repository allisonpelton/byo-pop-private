// Sexy::PlayMenu (vtable 0x86e38c (bejeweled3), 0x518 bytes, made by 0x680760): the screen PLAY opens, one crystal ball for each
// game mode. The main menu makes it in its constructor (at +0x270) and starts its fade when PLAY is pressed.
// port: only what the main menu reads is here so far; the modes' balls, its Draw (0x689d20) and its buttons come next.
#pragma once

#include "CurvedVal.h"
#include "Widget.h"

namespace Sexy {

class MainMenu;

class PlayMenu : public Widget {
public:
	MainMenu* mMainMenu;         // +0xc4: the menu that made it
	CurvedVal mFadeCurve;        // +0xe8: 0 while hidden, up to 1 when PLAY is pressed (the menu behind hides at 1)
	CurvedVal mAuxCurveA;         // +0x158: set in the constructor by "b+0,1,0,1,####       8####  i~###"
	CurvedVal mAuxCurveB;         // +0x1e8
	CurvedVal mAuxCurves[4];      // +0x258 (spaced 0x70)
	CurvedVal mAuxCurveC;         // +0x418
	CurvedVal mHideCurve;        // +0x490: while it is 1 or more the main menu is not shown at all

	explicit PlayMenu(MainMenu* theMainMenu);   // 0x680760 (bejeweled3)
	void Update() override;      // 0x696ad0 (bejeweled3)

	// 0x6899c0 (bejeweled3): the fade is up or on its way up
	bool IsActive();
};

}  // namespace Sexy
