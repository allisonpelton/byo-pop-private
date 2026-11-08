#include "GetSaveNameDialog.h"
#include "Dynomite.h"
#include "ButtonWidget.h"
#include "EditWidget.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include <cstring>

using namespace Sexy;

// 0x52b2d0: the edit box's colours, the same as the high score's name (EnterNameDialog)
static int gSaveNameEditColors[5][4] =
{
	{0, 0, 0, 0}, {0, 0, 0, 0}, {0xf0, 0xf0, 0xff, 0xff}, {0xff, 0xff, 0xff, 0xff}, {0, 0, 0, 0xff}
};

// 0x420fb0
GetSaveNameDialog::GetSaveNameDialog(FossilizedApp* theApp) :
	Dialog2004(theApp->mPicDialog->mImage, theApp->mPicDbutton->mImage, 8, true, theApp->GetString("GAME_DESCRIPTION"),
		"", "", Dialog::BUTTONS_NONE)
{
	mApp = theApp;
	SetButtonFont(mApp->mWhiteFontSmall);
	SetHeaderFont(mApp->mWhiteFont);
	SetLinesFont(mApp->mWhiteFontSmall);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(3, Color(0xff, 0xff, 0xff));

	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mWhiteFontSmall);
	mNameEdit->SetColors(gSaveNameEditColors, 5);
	mNameEdit->SetText(mApp->mSaveName);
	mNameEdit->mMaxChars = 0xf;
	mNameEdit->FocusCursor(false);

	mSaveButton = new ButtonWidget(700, mApp);
	mSaveButton->mButtonImage = mApp->mPicDbutton->mImage;
	mSaveButton->mDoFinger = true;
	mSaveButton->mLabel = mApp->GetString("SAVE");
	mSaveButton->SetFont(mApp->mWhiteFontSmall);
	mSaveButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mSaveButton->SetColor(1, Color(0xff, 0xff, 0xff));

	mCancelButton = new ButtonWidget(0x2bd, mApp);
	mCancelButton->mButtonImage = mApp->mPicDbutton->mImage;
	mCancelButton->mDoFinger = true;
	mCancelButton->mLabel = mApp->GetString("DIALOG_BUTTON_CANCEL");
	mCancelButton->SetFont(mApp->mWhiteFontSmall);
	mCancelButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mCancelButton->SetColor(1, Color(0xff, 0xff, 0xff));

	switch (mApp->mBoard->mGameType)
	{
	case 0:
		strcpy(mApp->mSaveMode, mApp->GetString("GETSAVENAMEDLG_STOMPED_PUZZLE").c_str());
		break;
	case 1:
		strcpy(mApp->mSaveMode, mApp->GetString("GETSAVENAMEDLG_ENDLESS_PUZZLE").c_str());
		break;
	case 2:
		strcpy(mApp->mSaveMode, mApp->GetString("GETSAVENAMEDLG_FOSSIL_PUZZLE").c_str());
		break;
	case 3:
		strcpy(mApp->mSaveMode, mApp->GetString("GETSAVENAMEDLG_TIME_TRIAL").c_str());
		break;
	}
}

// 0x420dc0
GetSaveNameDialog::~GetSaveNameDialog()
{
	if (mNameEdit != NULL)
		delete mNameEdit;
	if (mSaveButton != NULL)
		delete mSaveButton;
	if (mCancelButton != NULL)
		delete mCancelButton;
}

// 0x420e50
int GetSaveNameDialog::GetPreferredHeight(int theWidth)
{
	return 0x12c;
}

// 0x420e60
void GetSaveNameDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->AddWidget(mSaveButton);
	theWidgetManager->AddWidget(mCancelButton);
	theWidgetManager->SetFocus(mNameEdit);
}

// 0x420eb0
void GetSaveNameDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
	theWidgetManager->RemoveWidget(mSaveButton);
	theWidgetManager->RemoveWidget(mCancelButton);
}

// 0x420ef0
void GetSaveNameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 0x2f, mY + 0xb7, 0x11e, 0x18);
	mSaveButton->Resize(mX + 0x5d, mY + mHeight - 0x50, 0x60, 0x20);
	mCancelButton->Resize(mX + 0xc0, mY + mHeight - 0x50, 0x60, 0x20);
}

// 0x4218f0: the mode and the board's picture, then "enter a description" over a dark strip under the edit box
void GetSaveNameDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	// port: the original draws the tiny font and the picture with the graphics the save dialog's Draw left (0x4360f0),
	// a copy that the widget manager has already let go and that this dialog's copy took the place of on the stack
	SetDrawGraphics(g);
	mApp->mTinyFont->Draw(mApp->mSaveMode, 0xbb - mApp->mTinyFont->Width(mApp->mSaveMode) / 2, 0x50);
	mApp->mSavePic->Draw(0x9d, 0x5a);
	g->SetFont(mApp->mWhiteFontSmall);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mApp->GetString("ENTER_A_DESCRIPTION_FOR_YOUR_GAME"), 0x32, 0xb1);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(mApp->GetString("ENTER_A_DESCRIPTION_FOR_YOUR_GAME"), 0x30, 0xaf);
	g->SetColor(Color(0, 0, 0, 0x80));
	g->FillRect(0x2e, 0xb6, 0x11e, 0x1a);
}

// 0x420f80: Return in the edit box is Save
void GetSaveNameDialog::EditWidgetText(int theId, const SexyString& theString)
{
	mApp->ButtonDepress(700);
}
