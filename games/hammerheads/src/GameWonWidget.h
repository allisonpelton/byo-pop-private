#ifndef __HAMMERHEADS_GAMEWONWIDGET_H__
#define __HAMMERHEADS_GAMEWONWIDGET_H__

// The page that ends a game won on its last level: a picture, and in the box on it a farewell from the gnome (one for the classic
// game, one for Tough Cookie) scrolling up the box, then what the player did in the game, and "The End". A button at the bottom
// goes back to the main menu. The sound of a win comes a second (a hundred updates) after the page appears. Winning opens
// Tough Cookie to the player.
// Original: class Sexy::GameWonWidget : public Widget, public ButtonListener (0xa0 bytes; the constructor is 0x4534c0, the
// destructor 0x453290).

#include "ButtonListener.h"
#include "Widget.h"

namespace Sexy
{

class NcButton;

class GameWonWidget : public Widget, public ButtonListener
{
public:
	double			mScrollY;			// +0x88: where the first line of the text is in the page: 350 (under the box) at the start, down to -625 (all gone)
	double			mSoundDelay;		// +0x90: 1.0 at the start, counting down to the sound of the win
	NcButton*		mButton;			// +0x98: "Main Menu" (id 0)

public:
	GameWonWidget();																// 0x4534c0
	virtual ~GameWonWidget();														// 0x453290

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4533a0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x416010
	virtual void	Update() override;												// 0x453300
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x4533d0
	virtual void	Draw(Graphics* g) override;										// 0x453660
	virtual void	ButtonDepress(int theId) override;								// 0x453440 (the ButtonListener part)
};

// 0x4535c0: the page, as big as the application's window and in front, with the focus
GameWonWidget* MakeGameWon();

}

#endif
