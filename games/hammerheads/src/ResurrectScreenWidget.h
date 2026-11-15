#ifndef __HAMMERHEADS_RESURRECTSCREENWIDGET_H__
#define __HAMMERHEADS_RESURRECTSCREENWIDGET_H__

// The page that comes before a new classic or Tough Cookie game when the player has more than the starting restart point: a photo
// button for each point (the start, then the levels where a game was saved; the original lays out at most two rows of three),
// and a click starts the game at that point.
// Original: class Sexy::ResurrectScreenWidget : public Widget, public ButtonListener (0x9c bytes; the constructor is 0x45d800,
// the destructor 0x45d480). Its constructor takes a flag that the game never sets (the one call of Application::ResurrectOrPlay
// passes false); the page the flag would make, with a "Back to Game" button and the line "You have a new savepoint!", is not
// here, and neither is the original's button pointer at +0x84, which nothing ever sets.

#include <vector>

#include "ButtonListener.h"
#include "Widget.h"

namespace Sexy
{

class ResurrectButton;

class ResurrectScreenWidget : public Widget, public ButtonListener
{
public:
	enum
	{
		BUTTON_FIRST_POINT = 2		// the button of the restart point n is 2 + n
	};

	std::vector<ResurrectButton*> mPointButtons;	// +0x8c: one for each restart point of the mode

public:
	ResurrectScreenWidget();														// 0x45d800
	virtual ~ResurrectScreenWidget();												// 0x45d480

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x45d700
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x45d770
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x45d540
	virtual void	Draw(Graphics* g) override;										// 0x45dae0
	virtual void	ButtonDepress(int theId) override;								// 0x45d410 (the ButtonListener part)
};

}

#endif
