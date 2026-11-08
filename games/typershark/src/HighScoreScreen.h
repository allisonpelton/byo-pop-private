#ifndef __TYPERSHARK_HIGHSCORESCREEN_H__
#define __TYPERSHARK_HIGHSCORESCREEN_H__

// HighScoreScreen.cpp: the high score screen (the menu's Hall of Fame button): the picture IMAGE_HIGH_SCORE_BG with the two
// tables of the user manager written on it, ten lines each: on the left the adventure scores (name at x = 0x31, the score
// right-aligned at x = 0x108 and the label of the score's mode, 0 to 4, centred at x = 0x119 in 0x26 pixels) and on the
// right the abyss scores (name at x = 0x185, the depth right-aligned at x = 0x245), one font height apart from y = 0xa5.
// The "Close This Screen" button
// (id 0xccf) is the app's, made by ShowHighScoreScreen (WinWSAppScreens.cpp), which also makes the screen:
// new HighScoreScreen(this), Resize to the window, AddWidget. The screen reads the tables only when it draws.
// Original: class Sexy::HighScoreScreen : public Widget, vftable 0x52b0c4, sizeof 100. It has no constructor of its own:
// WinWSApp::ShowHighScoreScreen (0x41bbd0) builds it inline (operator new(100), Widget::Widget, the vftable, the app at
// +0x5c and its user manager at +0x58, the flag at +0x60 cleared). It has no destructor of its own either (slot 0 is the
// Widget deleting destructor, 0x482d00).
//
// Not ported (registration): Update (0x4470c0, slot 17) calls Widget::Update and, for a game that is not registered, once
// (the flag at +0x60) shows the register dialog with the text "Register the game now to record your high scores!"
// (0x41c1c0); the flag is not kept. The port has Widget::Update only.

#include "Widget.h"
#include <string>

namespace Sexy
{

class Font;
class UserProfileMgr;
class WinWSApp;

class HighScoreScreen : public Widget
{
public:
	UserProfileMgr*			mProfileMgr;	// +0x58 the app's user manager, whose two tables the screen draws
	WinWSApp*				mApp;			// +0x5c constructor argument
	// +0x60 bool: the registration prompt has been shown (not ported)

public:
	HighScoreScreen(WinWSApp* theApp);								// the inline constructor of 0x41bbd0

	virtual void			Draw(Graphics* g) override;				// 0x450110 slot 16

	// 0x447140: the name as it fits the column: a name no wider (in theFont) than the sample text 0x52f97c is in the app's
	// Rockwell 13 comes back as it is; for a wider one letters are taken one by one while what is taken is narrower than the
	// sample, then the last letter taken is dropped and the text 0x52c310 (three characters) is added
	std::string				FitName(const std::string& theName, Font* theFont);
};

}

#endif
