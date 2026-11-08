#ifndef __TYPERSHARK_DIFFICULTYDIALOG_H__
#define __TYPERSHARK_DIFFICULTYDIALOG_H__

// DifficultyDialog.cpp: "DIFFICULTY" (dialog 0x309, made by WinWSApp::ButtonDepress when a new game starts): five buttons
// (ids 600 to 604, the app's ButtonDepress takes the choice) and a footer button that cancels. The button that suits the
// user's typing speed (the average of the last ten speeds) has the highlighted picture, and the text "Recommended
// difficulty:" is drawn above it (the buttons from that one down are moved down by two lines of the font to make room).
// Original: class Sexy::DifficultyDialog : public KeyDialog, public EditListener, vftable 0x52e46c (52 slots, slot 20 is
// KeyDialog's KeyChar, slot 50 GetPreferredHeight), ButtonListener vftable 0x52e44c (+0x58), sizeof 0x158, constructor
// 0x43f0f0, destructor body 0x43a330 (deleting destructor 0x43a440). The EditListener base at +0x138 (vftable 0x52e438,
// four empty slots) is never used and is not ported.

#include "KeyDialog.h"

namespace Sexy
{

class WinWSApp;
class Font;
class DialogButton;
class Graphics;
class WidgetManager;

class DifficultyDialog : public KeyDialog
{
public:
	enum
	{
		NUM_BUTTONS = 5,
		FIRST_BUTTON_ID = 600				// 600 easy .. 604 X-treme (the app reads mDifficulty = id - 600)
	};

	Font*					mFont;						// +0x13c the Rockwell 13 font (its height places the buttons and the text)
	WinWSApp*				mAppCopy;					// +0x140 the same pointer as KeyDialog's mApp
	DialogButton*			mButtons[NUM_BUTTONS];		// +0x144, +0x148, +0x14c, +0x150, +0x154 the five difficulty buttons

public:
	DifficultyDialog(WinWSApp* theApp);														// 0x43f0f0
	virtual ~DifficultyDialog() override;													// 0x43a330 (deleting 0x43a440), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x437e20 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x437e80 slot 2
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x437ee0 slot 13
	virtual void			Draw(Graphics* g) override;								// 0x43ac70 slot 16
	virtual int				GetPreferredHeight(int theWidth) override;				// 0x437e00 slot 50

	using Widget::Resize;
};

}

#endif
