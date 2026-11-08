// The application's dialog colours and the pictures made in them (the dialogs, the end-of-game buttons and the help
// screens tint the same grey pictures with the colour of the game mode they belong to).

#include "AstroPopApp.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

// 0x409110: the tint of a dialog set. The original reads six colour globals (0x5a5030, 0x5a50c0, 0x5a50e0,
// 0x5a5000, 0x5a50b0, 0x5a5010) and answers the default colour for a set above 5.
Color AstroPopApp::GetDialogColor(int theSet)
{
	switch (theSet)
	{
	case 0:		return Color(150, 165, 0, 255);
	case 1:		return Color(50, 140, 120, 255);
	case 2:		return Color(150, 170, 0, 255);
	case 3:		return Color(140, 170, 0, 255);
	case 4:		return Color(205, 205, 0, 255);
	case 5:		return Color(160, 0, 210, 255);
	}
	return Color();
}

// 0x408f00: the pictures of the dialog's inset (the help one when theHelp is set) and of its buttons, each drawn into a
// new image of its own size in the colour it is given. The caller owns both images.
void AstroPopApp::MakeDialogImages(const Color& theInsetColor, const Color& theButtonColor, MemoryImage** theInsetImage,
								   MemoryImage** theButtonImage, bool theHelp)
{
	Image* anInsetSource = theHelp ? IMAGE_DIALOG_INSET_HELP : IMAGE_DIALOG_INSET;

	MemoryImage* anInset = new MemoryImage(this);
	anInset->Create(anInsetSource->mWidth, anInsetSource->mHeight);
	{
		Graphics g(anInset);
		g.SetColorizeImages(true);
		g.SetColor(theInsetColor);
		g.DrawImage(anInsetSource, 0, 0);
		g.SetColorizeImages(false);
	}

	MemoryImage* aButton = new MemoryImage(this);
	aButton->Create(IMAGE_DIALOG_BTN->mWidth, IMAGE_DIALOG_BTN->mHeight);
	{
		Graphics g(aButton);
		g.SetColorizeImages(true);
		g.SetColor(theButtonColor);
		g.DrawImage(IMAGE_DIALOG_BTN, 0, 0);
		g.SetColorizeImages(false);
	}

	*theInsetImage = anInset;
	*theButtonImage = aButton;
}
