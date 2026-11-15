#ifndef __HAMMERHEADS_RESURRECTBUTTON_H__
#define __HAMMERHEADS_RESURRECTBUTTON_H__

// A restart point on the page that offers them (ResurrectScreenWidget): the photo of the point with "Start on Level n" under it.
// While the pointer is over it (or it is pressed) a card fades in over the photo with what the player had at the point: hearts,
// coins and the hammer.
// Original: class Sexy::ResurrectButton : public NcButton (0x170 bytes; the constructor is 0x45d040).

#include "NcButton.h"

namespace Sexy
{

class ResurrectButton : public NcButton
{
public:
	int				mLevel;				// +0x148: the level the game starts at (the point's level and one more)
	double			mHealth;			// +0x150: the health at the point, in hearts
	int				mHearts;			// +0x158: the hearts the health fills (the capacity)
	int				mCoins;				// +0x15c: the coins in the purse
	int				mHammerLevel;		// +0x160: the hammer, from 0
	bool			mOnFire;			// +0x164: the hammer is on fire
	double			mFade;				// +0x168: 0 to 255, the card's opacity: up while the pointer is over the button, down otherwise

public:
	ResurrectButton(Image* theComponentImage, int theId, ButtonListener* theListener);	// 0x45d040

	// 0x45cff0
	void			SetShown(int theLevel, double theHealth, int theHearts, int theCoins, int theHammerLevel, bool theOnFire);

	virtual void	Update() override;												// 0x45cf60
	virtual void	Draw(Graphics* g) override;										// 0x45d0c0
};

}

#endif
