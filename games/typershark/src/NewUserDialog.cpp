#include "NewUserDialog.h"
#include "WinWSApp.h"
#include "GameText.h"

#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "WidgetManager.h"

using namespace Sexy;

// Not translated (forwarders to Dialog's own functions; nothing to override): MoneyDialog's constructor 0x4382c0,
// destructor body 0x438310, deleting destructor 0x438f70, Draw 0x438330 (slot 16 is NewUserDialog's own) and Update
// 0x438340 (slot 17); the deleting destructor of this class 0x438fa0 (the scalar deleting thunk of 0x438360)

// 0x55fd30: the edit box's five colours (background and outline clear, text (240,240,255), selection white, selected
// text black)
static int gNewUserEditColors[5][4] =
{
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{240, 240, 255, 255},
	{255, 255, 255, 255},
	{0, 0, 0, 255}
};

// 0x43bcc0: MoneyDialog (0x4382c0) first. OK / CANCEL, header "NEW USER", the prompt as the lines, no footer. The creator
// (WinWSApp::ShowNewUserDialog 0x414270) sizes it and recolours the header and the button text.
NewUserDialog::NewUserDialog(WinWSApp* theApp, const SexyString& theDialogLines) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_NEW_USER, true, GT(0x0052e884),
		theDialogLines, "", Dialog::BUTTONS_OK_CANCEL),
	mApp(theApp),
	mNameEdit(NULL)
{
	SetButtonFont(theApp->mFontRockwell13);
	SetHeaderFont(mApp->mFontRockwell13Bold);
	SetLinesFont(mApp->mFontRockwell13);
	mTextAlign = -1;
	mContentInsets.mLeft = 0x28;
	mContentInsets.mRight = 0x28;
	mContentInsets.mTop = 0x14;
	mContentInsets.mBottom = 0x28;
	SetColor(COLOR_LINES, Color(0, 0, 0));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));

	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mFontRockwell13, NULL);
	mNameEdit->SetColors(gNewUserEditColors, 5);
	mNameEdit->mMaxPixels = 0xf0;
	mNameEdit->mMaxChars = 0x18;
}

// 0x438360 (body; the scalar deleting destructor is 0x438fa0): the edit box, then MoneyDialog's body
NewUserDialog::~NewUserDialog()
{
	delete mNameEdit;
}

// 0x4383d0 slot 50: room for the edit box
int NewUserDialog::GetPreferredHeight(int theWidth)
{
	return Dialog2004::GetPreferredHeight(theWidth) + 0x38;
}

// 0x4383f0 slot 1
void NewUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->SetFocus(mNameEdit);
}

// 0x438430 slot 2
void NewUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
}

// 0x438460 slot 13: the edit box 0x6e pixels above the bottom edge (the buttons are laid out by Dialog::Resize)
void NewUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 0x30, (mHeight - 0x6e) + mY, mWidth - 0x60, 0x18);
}

// 0x4384b0 EditListener (+0x120) slot 0: Enter in the edit box is the OK path, without the click
void NewUserDialog::EditWidgetText(int theId, const SexyString& theString)
{
	mApp->ButtonDepress(mId + ID_YES_2004);
}

// 0x438350 EditListener (+0x120) slot 2 (NaziEditListener::vf02, shared by the base class): every character but '^', the
// colour-code character of the text writers, may be typed or pasted
bool NewUserDialog::AllowChar(int theId, SexyChar theChar)
{
	return theChar != '^';
}

// 0x438fd0: the three-slice frame from IMAGE_EDITBOX (three 32 x 32 cells: left cap, middle, right cap). Only theRect's
// x, y and width are used: the frame is 32 high.
void NewUserDialog::DrawEditBox(Graphics* g, const Rect& theRect)
{
	Image* anEditBoxImage = mApp->mImages[IMAGE_EDITBOX];
	g->DrawImage(anEditBoxImage, theRect.mX, theRect.mY, Rect(0, 0, 0x20, 0x20));
	g->DrawImage(anEditBoxImage, (theRect.mWidth - 0x20) + theRect.mX, theRect.mY, Rect(0x40, 0, 0x20, 0x20));

	// The middle tiles. The clip ends at the right edge, not at the right cap, and the tiles come after the cap, so the
	// last one can overdraw part of it: kept
	Graphics aClipG(*g);
	aClipG.ClipRect(theRect.mX + 0x20, theRect.mY, theRect.mWidth - 0x20, 0x20);
	int i = 0;
	while (i < (theRect.mWidth - 0x21) / 32)
	{
		aClipG.DrawImage(anEditBoxImage, theRect.mX + 0x20 + i * 32, theRect.mY, Rect(0x20, 0, 0x20, 0x20));
		i++;
	}
}

// 0x4392d0 slot 16: the dialog, then the frame 4 pixels around the edit widget (dialog-relative coordinates). Unlike
// BookWorm's, no caption is drawn: the prompt is the dialog's lines. The font and colour set at the end are not used
// by anything that follows.
void NewUserDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);		// MoneyDialog::Draw (0x438330)

	Rect aRect = mNameEdit->GetRect();
	aRect.mX = (aRect.mX - mX) - 4;
	aRect.mWidth = aRect.mWidth + 8;
	aRect.mHeight = aRect.mHeight + 8;
	aRect.mY = (aRect.mY - mY) - 4;
	DrawEditBox(g, aRect);

	g->SetFont(mApp->mFontRockwell13);
	g->SetColor(Color(0x4b, 0x4b, 0x4b));
}
