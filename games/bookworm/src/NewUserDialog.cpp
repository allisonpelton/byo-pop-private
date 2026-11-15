#include "NewUserDialog.h"
#include "BookWormApp.h"
#include "Res.h"
#include "GameText.h"

#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x52f530: background and outline transparent, text (240,240,255), selection white, selected text black
static int gNewUserEditColors[5][4] =
{
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{240, 240, 255, 255},
	{255, 255, 255, 255},
	{0, 0, 0, 255}
};

// 0x42e480 (body; scalar deleting dtor 0x42e620). The game selector is re-enabled by the app (2022/3022).
NewUserDialog::~NewUserDialog()
{
	delete mNameEdit;
}

// 0x42e4f0 (slot 50): room for the edit box and its caption
int NewUserDialog::GetPreferredHeight(int theWidth)
{
	return Dialog2004::GetPreferredHeight(theWidth) + EXTRA_HEIGHT;
}

// 0x42e510 (slot 1)
void NewUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->SetFocus(mNameEdit);
}

// 0x42e550 (slot 2)
void NewUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
}

// 0x42e580 (slot 13): the edit box 110 px above the bottom edge (OK / CANCEL are at Y + H - 69)
void NewUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 48, mHeight - 110 + mY, mWidth - 96, 24);
}

// 0x42e5d0 (EditListener, +0x120 slot 0): Enter in the edit box is exactly the OK path (without the click)
void NewUserDialog::EditWidgetText(int theId, const SexyString& theString)
{
	mApp2->ButtonDepress(mId + ID_YES_2004);
}

// 0x42e5f0 (EditListener, +0x120 slot 2; folded with GetStringDialog's): the CRT's isalpha in the C locale
// (A-Z, a-z) or a space; digits and punctuation are refused, so the special list lines "(...)" and '\'
// can never be names. The EditWidget applies it to pasted text too.
bool NewUserDialog::AllowChar(int theId, SexyChar theChar)
{
	// port: explicit ranges instead of the host isalpha (its locale may accept more)
	unsigned char aChar = (unsigned char) theChar;
	bool isAlpha = ((aChar >= 'A') && (aChar <= 'Z')) || ((aChar >= 'a') && (aChar <= 'z'));
	if ((!isAlpha) && (theChar != ' '))
		return false;
	return true;
}

// 0x42e650: the three-slice frame from IMAGE_EDITBOX (three 32x32 cells: left cap, middle, right cap). Only
// theRect's x, y and width are used: the frame is 32 high.
void NewUserDialog::DrawEditBox(Graphics* g, const Rect& theRect)
{
	Image* anEditBoxImage = mApp2->mImages[IMAGE_EDITBOX];
	g->DrawImage(anEditBoxImage, theRect.mX, theRect.mY, Rect(0, 0, 32, 32));
	g->DrawImage(anEditBoxImage, theRect.mWidth - 32 + theRect.mX, theRect.mY, Rect(64, 0, 32, 32));

	// The middle tiles. The clip ends at the right edge, not at the right cap, and the tiles come after the
	// cap, so the last one overdraws part of it (8 px at width 312): kept
	Graphics aClipG(*g);
	aClipG.ClipRect(theRect.mX + 32, theRect.mY, theRect.mWidth - 32, 32);
	for (int i = 0; i < (theRect.mWidth - 33) / 32; i++)
		aClipG.DrawImage(anEditBoxImage, theRect.mX + 32 + i * 32, theRect.mY, Rect(32, 0, 32, 32));
}

// 0x42e7c0. The creator (BookWormApp::DoNewUserDialog) disables the game selector, then sizes it to
// (238, 174, 400, GetPreferredHeight(400)).
NewUserDialog::NewUserDialog(BookWormApp* theApp, const SexyString& theDialogLines) :
	MoneyDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_NEW_USER, true,
		GameText(GT_NEW_USER_HEADER), theDialogLines, "", BUTTONS_OK_CANCEL),
	mApp2(theApp),
	mNameEdit(NULL)
{
	SetHeaderFont(theApp->mFonts[FONT_VERDANA18BOLDCOLOR]);
	SetButtonFont(mApp2->mFonts[FONT_VERDANA12BOLD]);
	SetLinesFont(mApp2->mFonts[FONT_VERDANA10BOLD]);
	mContentInsets = Insets(38, 38, 38, 38);
	Dialog2004::SetColor(COLOR_LINES, Color(255, 255, 255));
	Dialog2004::SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));    // the hilite stays white; the header too

	mNameEdit = new EditWidget(NAME_EDIT_ID, this);
	mNameEdit->SetFont(mApp2->mFonts[FONT_VERDANA12BOLD], NULL);
	mNameEdit->SetColors(gNewUserEditColors, 5);
	mNameEdit->mMaxPixels = NAME_MAX_PIXELS;
	mNameEdit->mMaxChars = NAME_MAX_CHARS;
}

// 0x42ea10 (slot 16)
void NewUserDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);

	// The frame 4 px around the edit widget, dialog-relative
	Rect aRect = mNameEdit->GetRect();
	aRect.mY = aRect.mY - mY - 4;
	aRect.mX = aRect.mX - mX - 4;
	aRect.mHeight += 8;
	aRect.mWidth += 8;
	DrawEditBox(g, aRect);

	// The caption, its baseline 8 px above the edit widget
	g->SetFont(mApp2->mFonts[FONT_VERDANA10BOLD]);
	g->SetColor(Color(255, 173, 17));
	g->DrawString(GameText(GT_ENTER_YOUR_NAME), mNameEdit->mX - mX, mNameEdit->mY - mY - 8);
}
