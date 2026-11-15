#ifndef __PIXELUS_HELPSCREEN_H__
#define __PIXELUS_HELPSCREEN_H__

// HelpScreen.cpp: "How to play Pixelus" (Pixelus.exe 0x40e1a0 constructor over three child widgets 0x40ddd0, 0x40df90 and
// 0x40da40, Draw 0x40e360, Resize 0x40db80, vftable 0x50d038; shown by 0x4328c0, closed by 0x432c20). A full-screen
// picture with two windows that take turns under a dark layer: on the left a small board (0x40ddd0, update 0x40d320, draw
// 0x40d740) plays a script of 950 updates (a pointer shows where the mouse is, the Tile Master throws three tiles and
// pulls them back), on the right a 16 by 16 board (0x40df90, update 0x40d8d0, draw 0x40e0b0) replays the shipped solution
// of levels/helpscreen.txt through the grid's own replay and fades the picture out and in to start again. The check box at
// the bottom is "Don't show this screen again": its state goes to the player's profile (mFlag2D, the opposite) when the
// screen goes.
//
// port: the Tile Master has no idle animation (the script's two nudges, frames 315 and 790, do nothing).

#include "Widget.h"
#include "ButtonListener.h"
#include "CheckboxListener.h"
#include <string>
#include <vector>

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class Checkbox;
class Grid;
class Board;
class WidgetManager;

class HelpScreen : public Widget, public ButtonListener, public CheckboxListener
{
public:
	enum
	{
		ID_OK = 0,
		LEFT_X = 19,				// the dark layers and the picture's windows
		RIGHT_X = 330,
		WINDOW_Y = 146,
		LEFT_W = 283,
		RIGHT_W = 290,
		WINDOW_H = 265,
		WIDGET_Y = 150,				// the two boards' widgets (0x40db80)
		MODE_IDLE = 0,				// the window waits under its dark layer
		MODE_RUN = 1,				// it is its turn
		MODE_DONE = 2				// its turn is over (the screen hands over to the other)
	};

	PixelusApp*		mApp;
	DialogButton*	mOKButton;			// +0x70
	Checkbox*		mDontShowBox;		// +0x6c
	Board*			mLeft;				// +0x60 the board of the left window (a widget of its own in the original)
	Board*			mRight;				// +0x64

	// the left window's state (0x40ddd0)
	int				mLeftMode;			// +0x98
	double			mLeftDark;			// +0x68 how dark the layer over it is (alpha, 0 to 200)
	double			mPointerFrom;		// +0x70 where the pointer starts and ends (x, in the widget)
	double			mPointerTo;			// +0x78
	double			mPointerWalk;		// +0x80 how far it has gone (0 to 1)
	double			mPointerStep;		// +0x88
	double			mGlow;				// +0x90 the pointer's flash at a click
	int				mLeftFrame;			// +0x9c updates of the script (it has 950)

	// the right window's state (0x40df90)
	int				mRightMode;			// +0x78
	double			mRightDark;			// +0x70
	int				mPictureAlpha;		// +0x64 the picture that covers the board between two runs
	int				mPicturePhase;		// +0x68 0 waiting, 1 fading out, 2 fading in
	std::string		mPuzzleText;		// the text of levels/helpscreen.txt
	int				mHold;				// +0x6c updates the finished board stays

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
	void			DrawDark(Graphics* g, int theX, int theWidth, double theDark);
	void			MovePointer(int theFromCol, int theToCol);						// 0x40d2d0
	void			UpdateLeft();													// 0x40d320
	void			UpdateRight();													// 0x40d8d0
	void			RestartRight();													// 0x40d840
};

}

#endif
