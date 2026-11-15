#ifndef __HAMMERHEADS_NCBUTTON_H__
#define __HAMMERHEADS_NCBUTTON_H__

// The game's button (0x41c9b0-0x41d450): a DialogButton that plays a sound when it is pressed, is hit only where its picture
// has colour, and darkens along with the screen behind a dialog. It is made in three cels (up, over, down) laid side by side.

#include <string>

#include "DialogButton.h"

namespace Sexy
{

class NcButton : public DialogButton
{
public:
	int				mSound;				// +0x130: played when the button is pressed; -1 for none
	int				mUnused140;			// +0x140: -1000, never read
	bool			mShown;				// +0x144: drawn at all (true)
	bool			mDarkensWithFade;	// +0x146: draws darker while a dialog dims the screen (Application::mFade)

public:
	NcButton(Image* theComponentImage, int theId, ButtonListener* theListener);	// 0x41ca80
	virtual ~NcButton();

	virtual void	Draw(Graphics* g) override;										// 0x41cb00
	virtual void	MouseDown(int x, int y, int theBtnNum, int theClickCount) override;		// 0x41ca20
	virtual bool	IsPointVisible(int x, int y) override;							// 0x41c9b0
};

// 0x41d120: the three cels of a button picture laid out as the up, over and down rectangles, and the look of its label
void SetupButtonCels(ButtonWidget* theButton, int theNumCels);

// 0x41d450: a button with a label. With no picture it is a dialog button made from the dialog button strip; otherwise the
// picture is cut into theNumCels cels, and the button is as wide as one of them.
NcButton* MakeNcButton(int theId, ButtonListener* theListener, const std::string& theLabel, Image* theImage = nullptr, int theNumCels = 3);

}

#endif
