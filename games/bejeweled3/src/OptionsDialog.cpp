// Sexy::OptionsDialog (bejeweled3, 0x681370). See OptionsDialog.h.
#include "OptionsDialog.h"

#include "Bej3App.h"
#include "Board.h"
#include "HintDialog.h"
#include "DialogButton.h"
#include "Res.h"

namespace Sexy {

namespace {

// The exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure)
{
	return gApp->mArtRes * theMeasure / 0x4b0;
}

// port: the framework's DialogButton draws the three cels of the picture (rest, over, pressed) from these rectangles
void SetCels(DialogButton* theButton)
{
	theButton->mNormalRect = IMAGE_DIALOG_BUTTON->GetCelRect(0);
	theButton->mOverRect = IMAGE_DIALOG_BUTTON->GetCelRect(1);
	theButton->mDownRect = IMAGE_DIALOG_BUTTON->GetCelRect(2);
}

}  // namespace

// 0x681370 (bejeweled3): a Bej3Dialog with id 0x15 and the header of string 0x133 ("OPTIONS"), modal when it is the game screen's,
// sized 1000 by 640 (704 when in the game, 1200ths of the art) at (350, 250). In the game it has BACK (id 0, right of the middle) and
// MAIN MENU (id 1, left of it; QUEST MENU, string 0x136, when the app's +0xbd8 is not 0, a Quest game) on its lower edge, each 300 wide.
// The HOW TO PLAY button (id 2, string 0x134, 340 wide, made while the board's +0x2b44 is not -1) stands above the other two, and the dialog is 80 higher then;
// it opens the help dialog (0x696100). port: left out until their step: the three sliders (ids 0 to 2, 0x5a26a0) and the checkboxes (ids 4 to 10: the art size, 3D and the
// others), which the exe puts on the dialog's face between the header and the buttons, and the first screen's version of the
// dialog (made by 0x6a3830 with 0). The Quest menu is not in the port
OptionsDialog::OptionsDialog(bool theInGame) :
	Bej3Dialog(DIALOG_ID, theInGame, gApp->GetString("307", "OPTIONS"), "", "", BUTTONS_NONE)
{
	Resize(Scaled(0x15e), Scaled(0xfa), Scaled(1000), Scaled(theInGame ? 0x2c6 : 0x280));
	if (!theInGame)
		return;
	const int aButtonWidth = Scaled(300);
	const int aButtonHeight = IMAGE_DIALOG_BUTTON->GetCelHeight();   // the picture is three cels high: at rest, over, pressed
	if (gApp->mBoard != nullptr && gApp->mBoard->mHelpId != -1) {
		mHowToPlayButton = new DialogButton(IMAGE_DIALOG_BUTTON, BUTTON_HOW_TO_PLAY, this);
		mHowToPlayButton->SetFont(FONT_DIALOG_BUTTONS);
		mHowToPlayButton->mLabel = gApp->GetString("308", "HOW TO PLAY");   // string 0x134
		SetCels(mHowToPlayButton);
		mHowToPlayButton->Resize(mWidth / 2 - Scaled(0xaa), mHeight - aButtonHeight - mContentInsets.mBottom, Scaled(0x154), aButtonHeight);
		AddWidget(mHowToPlayButton);
		mHeight += Scaled(0x50);
	}
	const int aButtonY = mHeight - aButtonHeight - mContentInsets.mBottom;

	mBackButton = new DialogButton(IMAGE_DIALOG_BUTTON, BUTTON_BACK, this);
	mBackButton->SetFont(FONT_DIALOG_BUTTONS);
	mBackButton->mLabel = gApp->GetString("309", "BACK");   // string 0x135
	SetCels(mBackButton);
	mBackButton->Resize(mWidth / 2 + Scaled(10), aButtonY, aButtonWidth, aButtonHeight);
	AddWidget(mBackButton);

	mMainMenuButton = new DialogButton(IMAGE_DIALOG_BUTTON, BUTTON_MAIN_MENU, this);
	mMainMenuButton->SetFont(FONT_DIALOG_BUTTONS);
	mMainMenuButton->mLabel = gApp->GetString("311", "MAIN MENU");   // string 0x137
	SetCels(mMainMenuButton);
	mMainMenuButton->Resize(mWidth / 2 - Scaled(0x136), aButtonY, aButtonWidth, aButtonHeight);
	AddWidget(mMainMenuButton);
}

OptionsDialog::~OptionsDialog()
{
	delete mBackButton;
	delete mMainMenuButton;
	delete mHowToPlayButton;
}

// 0x697460 (bejeweled3): the click sound and the Dialog's (0x769f60), then BACK closes the dialog, MAIN MENU asks the game screen
// (app +0xbe8) to leave for the first screen (slot 232, 0x756730) and closes it too; HOW TO PLAY adds the help dialog of the board's help (the dialog stays)
void OptionsDialog::ButtonDepress(int theId)
{
	Bej3Dialog::ButtonDepress(theId);
	if (theId == BUTTON_BACK)
		StartClose();
	else if (theId == BUTTON_MAIN_MENU && gApp->mBoard != nullptr) {
		gApp->mBoard->LeaveToMenu();
		StartClose();
	}
	else if (theId == BUTTON_HOW_TO_PLAY && gApp->mBoard != nullptr)
		gApp->AddDialog(new HelpDialog(gApp->mBoard->mHelpId, 1));
}

}  // namespace Sexy
