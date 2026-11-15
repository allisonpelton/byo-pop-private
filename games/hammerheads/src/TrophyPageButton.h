#ifndef __HAMMERHEADS_TROPHYPAGEBUTTON_H__
#define __HAMMERHEADS_TROPHYPAGEBUTTON_H__

// What the two pages of the trophy room have in common: the page that announces a new trophy (TrophyAppearButton) and the page
// that shows one of the trophies the player has (TrophyCloseUpButton) are buttons as big as the screen. Each fades in over the
// first second of its time, and its "Click to continue" button is ready when the click progress has run to one. The original has
// them as two classes with the same code and the same fields in the same places (+0x130, +0x138, +0x148); the port has the one
// base, and the code that draws what they share.
// Original: class Sexy::TrophyAppearButton and class Sexy::TrophyCloseUpButton, both : public DialogButton (0x150 bytes).

#include "DialogButton.h"

namespace Sexy
{

class Graphics;

class TrophyPageButton : public DialogButton
{
public:
	int				mTrophy;			// +0x130: the trophy on the page (the cel of IMAGE_TROPHIES)
	double			mTime;				// +0x138: the page's own time: 0 at the start; it fades in by 1 and the light turns with it
	double			mClickProgress;		// +0x148: 0 to 1; the page can be left when it is 1

public:
	TrophyPageButton(int theId, ButtonListener* theListener);

	virtual void	UpdateF(float theFrac) override;							// 0x45ffc0

protected:
	// 0x460120: the trophy's name and what it was for, and the "Click to continue" button turning in from below (it stands
	// theOffsetX and theOffsetY further when it is pressed)
	void			DrawTrophyInfo(Graphics* g, int theOffsetX, int theOffsetY);
};

// 0x591390, 0x5915e0: the name of a trophy, and what it is for (the tables the exe builds at its start)
const char* GetTrophyName(int theTrophy);
const char* GetTrophyDescription(int theTrophy);

// 0x45fe60: the blue of the page over the whole screen (it fades in by theFade when theBlend), then, past one in theTime, the light
// turning in the middle (it fades in until two)
void DrawTrophyBackdrop(Graphics* g, double theTime, double theFade, bool theBlend);

}

#endif
