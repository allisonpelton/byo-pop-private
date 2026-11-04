#ifndef __PIXELUS_COMPLETIONSCREEN_H__
#define __PIXELUS_COMPLETIONSCREEN_H__

// CompletionScreen.cpp: the full-screen picture that follows a puzzle's tablet when it ends the tutorial (Pixelus.exe
// 0x40bc40 constructor, 0x40bd80 Draw, 0x40b7d0 Update, 0x40b710 Resize, 0x40b760 button; vftable 0x50c8c8, made by
// 0x4326c0 / 0x432b60 from the tablet's button 0x419c20). Its mode (+0x64) is 1 or 3 for a temple, 2 or 4 for the game and
// 5 for the tutorial; only 5 is made so far.
//
// port: the picture, the header and the button; the original's animation (the rays and the figure's pose, which
// 0x40b7d0 steps with three counters) is not drawn.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class WidgetManager;

class CompletionScreen : public Widget, public ButtonListener
{
public:
	enum { ID_CONTINUE = 1, MODE_TUTORIAL = 5 };

	PixelusApp*		mApp;
	DialogButton*	mContinueButton;		// +0x60
	int				mMode;					// +0x64
	double			mProgress;				// +0x78 (0.05 an update to 1)

public:
	CompletionScreen(PixelusApp* theApp, int theMode);
	virtual ~CompletionScreen();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x40b710
	virtual void	Update() override;													// 0x40b7d0
	virtual void	Draw(Graphics* g) override;											// 0x40bd80
	virtual void	ButtonDepress(int theId) override;									// 0x40b760
};

}

#endif
