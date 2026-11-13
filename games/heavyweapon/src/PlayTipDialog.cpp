#include "PlayTipDialog.h"
#include "Board.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "Checkbox.h"
#include "DDImage.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"

using namespace Sexy;

namespace
{
	const int DIALOG_X = 0x8c;
	const int DIALOG_Y = 100;
	const int DIALOG_WIDTH = 0x168;
	const int DIALOG_HEIGHT = 0x118;
	const int CONTENT_INSET = 0x1e;
	const int BUTTON_WIDTH = 0x78;
	const int BUTTON_HEIGHT = 0x1e;
	const int BUTTON_BOTTOM = 0x3c;		// the button's top is this far above the box's bottom
	const int CHECKBOX_X = 0x28;
	const int CHECKBOX_SIZE = 0x1e;
	const int CHECKBOX_BOTTOM = 100;
	const int LABEL_X = 0x4b;
	const int LABEL_BOTTOM = 0x50;
}

// 0x423a30: the hint's text in the lines; the checkbox starts unticked whatever the player's flag says
PlayTipDialog::PlayTipDialog(HeavyWeaponApp* theApp, const std::string& theTip) :
	Dialog2004(theApp->mSprites[IMAGE_DIALOG]->mImage, NULL, CONSTRUCTED_ID, true, GT(0x52dbf8), theTip, "", BUTTONS_NONE)
{
	mApp = theApp;
	mBoard = theApp->mBoard;
	SetHeaderFont(theApp->mFonts[FONT_RUBBERSTAMPLET20]);
	SetLinesFont(theApp->mFonts[FONT_NORMAL]);
	SetColor(COLOR_LINES, Color(0xc0, 0xc0, 0xc0));
	mContentInsets = Insets(CONTENT_INSET, CONTENT_INSET, CONTENT_INSET, CONTENT_INSET);
	mOkButton = new BracketButton(mApp, this, 0);
	mOkButton->SetTint(Color(0x40, 0xc0, 0x40));
	mOkButton->mLabel = GT(0x52d4f8);
	mCheckbox = new Checkbox(mApp->mSprites[IMAGE_UNCHECKED]->mImage, mApp->mSprites[IMAGE_CHECKED]->mImage, 1, this);
	mCheckbox->mChecked = false;
	mWidth = DIALOG_WIDTH;
	mHeight = DIALOG_HEIGHT;
	Resize(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT);
}

// 0x4239b0 (body 0x4237f0)
PlayTipDialog::~PlayTipDialog()
{
	delete mOkButton;
	delete mCheckbox;
}

// 0x4238b0 slot 1
void PlayTipDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mOkButton);
	theManager->AddWidget(mCheckbox);
}

// 0x423870 slot 2: the board gets the focus back
void PlayTipDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mOkButton);
	theManager->RemoveWidget(mCheckbox);
	theManager->SetFocus(mBoard);
}

// 0x4238f0 slot 13: OK at the foot, the checkbox at its left
void PlayTipDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mOkButton->Resize(mWidth / 2 - BUTTON_WIDTH / 2 + mX, mHeight - BUTTON_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mCheckbox->Resize(mX + CHECKBOX_X, mHeight - CHECKBOX_BOTTOM + mY, CHECKBOX_SIZE, CHECKBOX_SIZE);
}

// 0x423ce0 slot 16: the checkbox's label
void PlayTipDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	WriteString2004(g, GT(0x52dc04), LABEL_X, mHeight - LABEL_BOTTOM, -1, -1, true, 0, -1);
}

// 0x423960 listener slot 4
void PlayTipDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
}

// 0x423980 listener slot 5: OK closes the box
void PlayTipDialog::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	mApp->KillDialog(DIALOG_ID);
}

// 0x4239e0: a click flips the hints flag whatever the box's state; the sound is the press's or the release's by the state
void PlayTipDialog::CheckboxChecked(int theId, bool checked)
{
	mApp->PlaySample(checked ? SOUND_BUTTONDOWN : SOUND_BUTTONUP, 0);
	mApp->CurrentUser().mFlags ^= UserRecord::FLAG_NO_TIPS;
}
