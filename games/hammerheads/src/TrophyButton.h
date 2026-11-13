#ifndef __HAMMERHEADS_TROPHYBUTTON_H__
#define __HAMMERHEADS_TROPHYBUTTON_H__

// One shelf place of the trophy room: the trophy, half the size of its cel, lit when the mouse is over it (it flashes when it has
// just been won); a trophy the player has not won is a faint dark shape. The original does not delete the shape's image (a leak).
// Original: class Sexy::TrophyButton : public DialogButton (0x158 bytes; the constructor is 0x45fb20).

#include "DialogButton.h"

namespace Sexy
{

class MemoryImage;

class TrophyButton : public DialogButton
{
public:
	int				mTrophy;			// +0x130: the cel of IMAGE_TROPHIES
	bool			mEarned;			// +0x134
	MemoryImage*	mShadowImage;		// +0x138: for a trophy not won: the trophy as a shape of one tenth black (null when it is won)
	double			mFlash;				// +0x140: counts down from mFlashStart while the trophy flashes after being won
	double			mFlashStart;		// +0x148

public:
	// theWidth and theHeight: the size of the button (the trophy is stretched to fit it)
	TrophyButton(int theId, ButtonListener* theListener, int theTrophy, bool theEarned, int theWidth, int theHeight);	// 0x45fb20
	virtual ~TrophyButton();

	// 0x45fb00
	void			StartFlash(double theTime);

	virtual void	UpdateF(float theFrac) override;							// 0x45fae0
	virtual void	Draw(Graphics* g) override;									// 0x45fd10
};

}

#endif
