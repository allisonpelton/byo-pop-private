#include "CreditsDialog.h"

#include "Application.h"
#include "Graphics.h"
#include "Image.h"
#include "NcButton.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The box is as tall as the picture and this much
const int HEIGHT_OVER_PICTURE = 0x78;

// The layout starts from the picture's width and this much
const int WIDTH_OVER_PICTURE = 0x64;

// Where the picture is drawn in the box
const int PICTURE_X = 0x32;
const int PICTURE_Y = 0x32;

// The "Ok" button is this far in from the sides and this far up from the bottom
const int BUTTON_INSET = 0x50;
const int BUTTON_BOTTOM_MARGIN = 0x1e;

// The id of the button
const int BUTTON_OK = 0;

}

// 0x44f3e0
CreditsDialog::CreditsDialog() :
	NcMovingDialog(IMAGE_CREDITS->GetHeight() + HEIGHT_OVER_PICTURE, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_CREDITS, true, "",
		"", "Credits", Dialog::BUTTONS_NONE)
{
	mOkButton = MakeNcButton(BUTTON_OK, this, "Ok");
}

// 0x44f0a0
CreditsDialog::~CreditsDialog()
{
	if (mOkButton != nullptr)
		delete mOkButton;
}

// 0x44f120
void CreditsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aButtonHeight = mOkButton->mComponentImage->GetHeight();
	mOkButton->Resize(theX + BUTTON_INSET, theY - aButtonHeight - BUTTON_BOTTOM_MARGIN + theHeight, theWidth - 2 * BUTTON_INSET,
		aButtonHeight);
}

// 0x44f1e0
void CreditsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mOkButton);
}

// 0x44f220
void CreditsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mOkButton);
}

// 0x44f260
void CreditsDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	g->DrawImage(IMAGE_CREDITS, PICTURE_X, PICTURE_Y);
}

// 0x44f2d0
int CreditsDialog::GetPreferredHeight(int theWidth)
{
	return IMAGE_CREDITS->GetHeight() + HEIGHT_OVER_PICTURE;
}

// 0x44f290: only the box's own button is heard, and it takes the box away
void CreditsDialog::ButtonDepress(int theId)
{
	if (theId == BUTTON_OK)
		CloseWith(BUTTON_OK);
}

// 0x44f2b0
void CreditsDialog::Closed()
{
	if (mCloseResult == BUTTON_OK)
		gApp->KillDialog(this);
}

// 0x44f5d0
CreditsDialog* Sexy::MakeCreditsDialog()
{
	CreditsDialog* aDialog = new CreditsDialog();
	LayoutDialog(aDialog, IMAGE_CREDITS->GetWidth() + WIDTH_OVER_PICTURE);
	int aWidth = aDialog->mWidth;
	int aHeight = aDialog->GetPreferredHeight(aWidth);
	aDialog->Resize((gApp->mWidth - aWidth) / 2, -aHeight, aWidth, aHeight);
	gApp->AddDialog(DIALOG_CREDITS, aDialog);
	return aDialog;
}
