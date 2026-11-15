#ifndef __HAMMERHEADS_TROPHYAPPEARBUTTON_H__
#define __HAMMERHEADS_TROPHYAPPEARBUTTON_H__

// The page that announces a trophy the player has just won: the trophy falls in from above, spinning, onto a blue screen, and
// then swells while the light turns behind it; under it, the name, what it was for, and a "Click to continue" button (the page
// is the button: any click on it, once it is ready, goes on to the next trophy won or to the trophy room).
// Original: class Sexy::TrophyAppearButton : public DialogButton (0x150 bytes; the constructor is 0x45f5e0).

#include "TrophyPageButton.h"

namespace Sexy
{

class TrophyAppearButton : public TrophyPageButton
{
public:
	bool			mBlend;				// +0x140: the blue fades in over what is behind (the first page: the game), instead of being solid

public:
	// theTrophy: the one the first page shows (-1 for none)
	TrophyAppearButton(int theId, ButtonListener* theListener, int theTrophy);	// 0x45f5e0

	// 0x45f590: the page starts again for another trophy
	void			Start(int theTrophy, bool theBlend);

	virtual void	Draw(Graphics* g) override;									// 0x45f6a0
};

}

#endif
