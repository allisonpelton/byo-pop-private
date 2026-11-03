#ifndef __PIXELUS_HELPSCREEN_H__
#define __PIXELUS_HELPSCREEN_H__

// HelpScreen.cpp: "How to play Pixelus" (Pixelus.exe 0x40e1a0 constructor over three child widgets 0x40ddd0, 0x40df90 and
// 0x40da40, Draw 0x40e360, Resize 0x40db80, vftable 0x50d038; shown by 0x4328c0, closed by 0x432c20). A full-screen
// picture with two windows: on the left two small boards play the slide-a-tile demonstrations on a timeline of 950
// updates, on the right the picture (the puzzle of levels/helpscreen.txt) is completed. The check box at the bottom is
// "Don't show this screen again": its state goes to the player's profile (mFlag2D, the opposite) when the screen goes.
//
// port: the demonstration is the port's own: the tiles of the picture slide in one after the other from the left and the
// right window shows the picture growing with them; the original plays two scripted throws and pulls on real boards.

#include "Widget.h"
#include "ButtonListener.h"
#include "CheckboxListener.h"
#include <vector>

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class Checkbox;
class Grid;
class WidgetManager;

class HelpScreen : public Widget, public ButtonListener, public CheckboxListener
{
public:
	enum
	{
		ID_OK = 0,
		CELL = 15,					// the cells of the demonstration
		LEFT_X = 19,				// the two windows of the picture
		RIGHT_X = 330,
		WINDOW_Y = 146,
		LEFT_W = 283,
		RIGHT_W = 290,
		WINDOW_H = 265,
		LEFT_VISIBLE_H = 155,		// the two plaques cover the rest of the left window
		BOARD_X = 99,				// the board of the left window starts where the rack of the tiles ends
		FADE_UPDATES = 130,			// a fade from or to black, 1.5 alpha an update up to 200
		FIRST_SLIDE = 40,
		SLIDE_GAP = 30,				// between the starts of two tiles
		SLIDE_UPDATES = 40,
		HOLD_UPDATES = 160
	};

	PixelusApp*		mApp;
	DialogButton*	mOKButton;			// +0x70
	Checkbox*		mDontShowBox;		// +0x6c
	Grid*			mGrid;				// the puzzle of the picture
	std::vector<int> mOrder;			// its wanted cells (row * 32 + column) in the order the tiles come
	int				mTimer;
	int				mDemoFrame;		// the original's demo grid runs a clock of its own (0x40d320), 950 updates long
	int				mCycle;				// updates of the whole demonstration

public:
	HelpScreen(PixelusApp* theApp);	// 0x40e1a0
	virtual ~HelpScreen();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x40dc60
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x40dce0
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x40db80
	virtual void	Update() override;												// 0x40dd50
	virtual void	Draw(Graphics* g) override;										// 0x40e360
	virtual void	ButtonDepress(int theId) override;								// 0x40ddc0

private:
	void			DrawDark(Graphics* g, int theX, int theWidth, int theAlpha);
};

}

#endif
