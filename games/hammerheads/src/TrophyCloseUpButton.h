#ifndef __HAMMERHEADS_TROPHYCLOSEUPBUTTON_H__
#define __HAMMERHEADS_TROPHYCLOSEUPBUTTON_H__

// The page that shows one trophy of the room: the trophy grows from where it was on the room's shelf (and moves to the middle of the
// screen) over a blue screen with the light turning behind it, under its name, what it was for and a "Click to continue" button.
// Like the announcement, the page is the button.
// Original: class Sexy::TrophyCloseUpButton : public DialogButton (0x150 bytes; the constructor is 0x4600a0).

#include "TrophyPageButton.h"

namespace Sexy
{

class TrophyCloseUpButton : public TrophyPageButton
{
public:
	int				mFromX;				// +0x140: where the trophy's button was, which the page's trophy starts from
	int				mFromY;				// +0x144

public:
	TrophyCloseUpButton(int theId, ButtonListener* theListener);				// 0x4600a0

	// 0x460050: the page is about to be shown for a trophy, which was at the position
	void			Setup(int theTrophy, int theFromX, int theFromY);
	// 0x460080: the page starts from its beginning
	void			Restart();

	virtual void	Draw(Graphics* g) override;									// 0x460300
};

}

#endif
