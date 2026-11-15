#ifndef __HAMMERHEADS_HOWTOPLAYWIDGET_H__
#define __HAMMERHEADS_HOWTOPLAYWIDGET_H__

// The how-to-play page: a Flash movie (images\howtoplay.swf, 800 by 500) with a green strip under it, a button, and, on the page
// that comes with a new game, a check box "Don't show this again". The game is paused while it is up. Pressing the button takes
// the page down and goes on.
// Original: class Sexy::HowToPlayWidget : public Widget, public ButtonListener, public CheckboxListener, public FlashListener
// (0x98 bytes; the constructor is 0x456c70, the destructor 0x4568d0).

#include "ButtonListener.h"
#include "CheckboxListener.h"
#include "FlashWidget.h"
#include "Widget.h"

namespace Sexy
{

class Checkbox;
class NcButton;

class HowToPlayWidget : public Widget, public ButtonListener, public CheckboxListener, public FlashListener
{
public:
	enum
	{
		BUTTON_DONE = 0,			// the page that is opened by itself (theWithNewGame false)
		BUTTON_PLAY = 1,			// the page of a new game
		CHECKBOX_DONT_SHOW = 2
	};

	NcButton*		mButton;		// +0x8c: "Done" or "Click here to play"
	Checkbox*		mCheckbox;		// +0x90: only on the page of a new game
	FlashWidget*	mFlash;			// +0x94

	// DAT_00598088: how many pages there are (the level introduction waits while there is one)
	static int		sCount;

public:
	// theWithNewGame: the page of a new game, with the "Play" button and the check box (theWithNewGame false: just a "Done" button)
	explicit HowToPlayWidget(bool theWithNewGame);									// 0x456c70
	virtual ~HowToPlayWidget();														// 0x4568d0

	// 0x456bd0
	static bool		IsActive();

	virtual void	Update() override;												// 0x4569a0
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4569b0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x456a00
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x456a70
	virtual void	Draw(Graphics* g) override;										// 0x456fc0
	virtual void	ButtonDepress(int theId) override;								// 0x456b70 (the ButtonListener part)
	virtual void	CheckboxChecked(int theId, bool theChecked) override;			// 0x456bb0 (the CheckboxListener part)
	virtual void	FlashAnimEnded(int theId) override;								// 0x456c00 (the FlashListener part)
};

// 0x456f10: the page, as big as the application's window and in front, with the focus
HowToPlayWidget* MakeHowToPlay(bool theWithNewGame);

}

#endif
