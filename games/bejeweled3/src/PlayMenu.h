// Sexy::PlayMenu (vtable 0x86e38c (bejeweled3), 0x518 bytes, made by 0x680760): the screen PLAY opens, one crystal ball for each
// game mode. The main menu makes it in its constructor (at +0x270) and starts its fade when PLAY is pressed. It is also
// the listener (at +0xbc, vtable 0x86e4d4) of its eight balls (ids 0 to 7: CLASSIC, ZEN, LIGHTNING, QUEST, then the four
// that start locked: POKER, BUTTERFLIES, ICE STORM, DIAMOND MINE) and of two buttons: BACK (id 100) and QUIT (id 0x65).
// port: a click on a mode only logs which mode (0x69b360 starts the game); BACK and QUIT are logged the same way. The
// exe's Bej3Buttons (vtable 0x86498c, with an alpha at +0x168 that Update sets from the first aux curve) are the
// framework's ButtonWidgets here, so BACK and QUIT do not fade. Not translated: the draw of the main menu's background
// widget (+0x6e0) behind the balls, the music start in Update (the object at 0x8d7a80), the cover pictures of the balls,
// the list the exe keeps the buttons in (0x474fc0), the screens at app +0xbd8 and +0xbe8, and the click on a ball
// (0x69b360).
#pragma once

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "CrystalBall.h"
#include "CurvedVal.h"
#include "Widget.h"

#include <string>

namespace Sexy {

class MainMenu;

class PlayMenu : public Widget, public ButtonListener {
public:
	enum { BALL_COUNT = 8, FIRST_LOCKED_BALL = 4, ID_BACK = 100, ID_QUIT = 0x65 };

	MainMenu* mMainMenu;         // +0xc4: the menu that made it
	CrystalBall* mBalls[BALL_COUNT];   // +0xc8: the modes' balls
	CurvedVal mFadeCurve;        // +0xe8: 0 while hidden, up to 1 when PLAY is pressed (the menu behind hides at 1)
	CurvedVal mAuxCurveA;         // +0x158: set in the constructor by "b+0,1,0,1,####       8####  i~###"
	std::string mModeText;        // +0x1c8: what the mode the mouse is over is (its label, a wstring in the exe)
	bool mModeLocked = false;     // +0x1e4: the mode is locked: "SECRET GAME" is written over its text
	CurvedVal mAuxCurveB;         // +0x1e8: the label's alpha, 0 to 1
	CurvedVal mAuxCurves[4];      // +0x258 (spaced 0x70): how far the four locked balls have gone red
	CurvedVal mAuxCurveC;         // +0x418: the hint's alpha ("New to Bejeweled? Try Classic mode")
	int mSelectedBall = 0;        // +0x488: the ball that the +0x490 curve brings to the middle
	CurvedVal mHideCurve;        // +0x490: while it is 1 or more the main menu is not shown at all
	int mChosenMode = -1;        // +0x500: set to -1 by the constructor (what reads it is not translated)
	ButtonWidget* mBackButton;   // +0x504: id 100
	ButtonWidget* mQuitButton;   // +0x508: id 0x65
	int mFirstBallColor = 0;     // +0x50c: the first ball's text colour as the ctor leaves it, as an ARGB int

	explicit PlayMenu(MainMenu* theMainMenu);   // 0x680760 (bejeweled3)
	void Update() override;      // 0x696ad0 (bejeweled3)
	void Draw(Graphics* g) override;   // 0x689d20 (bejeweled3)

	void ButtonDepress(int theId) override;      // 0x69b360 (bejeweled3)
	void ButtonMouseEnter(int theId) override;   // 0x68e0e0 (bejeweled3)
	void ButtonMouseLeave(int theId) override;   // 0x6847b0 (bejeweled3)

	// 0x6899c0 (bejeweled3): the fade is up or on its way up
	bool IsActive();

	// 0x684660 (bejeweled3): the label and the locked balls' red fade out
	void HideLabel();
};

}  // namespace Sexy
