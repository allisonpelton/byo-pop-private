// The application's dialog colours and the pictures made in them (the dialogs, the end-of-game buttons and the help
// screens tint the same grey pictures with the colour of the game mode they belong to).

#include "AstroPopApp.h"
#include "AstroDialog.h"
#include "Board.h"
#include "Font.h"
#include "DialogButton.h"
#include "WidgetManager.h"
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

// The tint of a dialog set's inset picture: the original reads six colour globals (0x5a4ff0, 0x5a50a0, 0x5a5040,
// 0x5a50d0, 0x5a5020, 0x5a5050); a set above 5 gets the default colour.
Color AstroPopApp::GetDialogInsetColor(int theSet)
{
	switch (theSet)
	{
	case 0:		return Color(35, 115, 40, 255);
	case 1:		return Color(40, 70, 80, 255);
	case 2:		return Color(130, 35, 20, 255);
	case 3:		return Color(130, 140, 20, 255);
	case 4:		return Color(30, 60, 255, 255);
	case 5:		return Color(150, 0, 150, 255);
	}
	return Color();
}

// 0x411ba0: a box in the colours of a set (theType 0 .. 5). The width is 0x14a and the height that of the box picture
// unless they are given (-1 for none); the height grows to fit the lines when none is given. An empty button label of
// a yes/no or OK/cancel box gets the usual word. theAnimated false makes the box settled at once. A running game is
// paused. (The original takes one more argument, a flag it does not read; the port leaves it out.)
AstroDialog* AstroPopApp::MakeDialog(int theType, bool theAnimated, int theId, int theWidth, int theHeight, int theButtonMode,
									 const std::string& theHeader, const std::string& theLines, const std::string& theYes,
									 const std::string& theNo)
{
	mOpenDialogs++;

	int aWidth = 0x14a;
	int aHeight = IMAGE_DIALOG->mHeight;
	if (theWidth != -1)
		aWidth = theWidth;
	if (theHeight != -1)
		aHeight = theHeight;

	AstroDialog* aDialog = new AstroDialog(this, theId, theHeader, theButtonMode, true, GetDialogInsetColor(theType),
										   GetDialogColor(theType));
	aDialog->mDialogLines = theLines;

	std::string aYes = theYes;
	std::string aNo = theNo;
	if (aDialog->mYesButton != NULL)
	{
		aDialog->mYesButton->mLabel = aYes;
		aDialog->mYesButton->mHasTransparencies = true;
	}
	if (theButtonMode == 2 || theButtonMode == 1)
	{
		if (aYes.empty())
			aYes = (theButtonMode == 1) ? "Yes" : "OK";
		if (aNo.empty())
			aNo = (theButtonMode == 1) ? "No" : "Cancel";
		aDialog->mYesButton->mLabel = aYes;
		aDialog->mYesButton->mHasTransparencies = true;
		aDialog->mNoButton->mLabel = aNo;
		aDialog->mNoButton->mHasTransparencies = true;
	}
	else if (theButtonMode != 0)
		aDialog->mYesButton->mHasTransparencies = true;

	StyleDialog(aDialog);

	// the size: the lines at the width, the title and the button on top and under them
	Graphics g((Image*) NULL);
	g.SetFont(GetFontById(0xb0));
	int aTextHeight = aDialog->GetWordWrappedHeight(&g, (aWidth - aDialog->mContentInsets.mRight) - aDialog->mContentInsets.mLeft,
													aDialog->mDialogLines, GetFontById(0xb0)->GetHeight());
	if (aHeight < aTextHeight)
		aHeight = aTextHeight;
	if (theHeight == -1)
	{
		aHeight = ((IMAGE_DIALOG_BTN->mHeight + aDialog->mSpaceAfterHeader + aDialog->mContentInsets.mBottom +
					aDialog->mContentInsets.mTop + aTextHeight) - aDialog->mHeaderFont->GetAscentPadding()) + 0x14 +
				  aDialog->mHeaderFont->GetAscent();
	}
	if (aHeight < IMAGE_DIALOG->mHeight)
		aHeight = IMAGE_DIALOG->mHeight;
	int aMinWidth = aDialog->mHeaderFont->StringWidth(aDialog->mDialogHeader) + aDialog->mContentInsets.mRight + 10 +
					aDialog->mContentInsets.mLeft;
	if (aWidth < aMinWidth)
		aWidth = aMinWidth;

	aDialog->Resize(mWidth / 2 - aWidth / 2, mHeight / 2 - aHeight / 2, aWidth, aHeight);
	if (!theAnimated)
		aDialog->SetAnimated(false);
	if (mBoard != NULL)
		mBoard->Pause(true, false);
	AddDialog(theId, aDialog);
	mWidgetManager->SetFocus(aDialog);
	return aDialog;
}

// A box with the word OK, made where the original has one (the first colour set, the id 0x6c)
void AstroPopApp::ShowMessageDialog(const std::string& theHeader, const std::string& theText)
{
	MakeDialog(1, false, 0x6c, -1, -1, 3, theHeader, theText, "OK", std::string());
}
