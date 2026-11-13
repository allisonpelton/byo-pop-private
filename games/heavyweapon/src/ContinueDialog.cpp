#include "ContinueDialog.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "DDImage.h"
#include "GameText.h"
#include "ImageFont.h"
#include "MainMenu.h"
#include "Sprite.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{
	const int CONTENT_INSET = 0x19;
	const int DIALOG_X = 0x3c;
	const int DIALOG_Y = 0x7d;
	const int DIALOG_WIDTH = 0x208;
	const int DIALOG_HEIGHT = 0xe6;
	const int BUTTON_WIDTH = 0x8c;
	const int BUTTON_HEIGHT = 0x1e;
	const int BUTTON_BOTTOM = 0x37;		// the buttons' tops are this far above the box's bottom
}

// 0x41d800: the text names the player's saved mission (its number and its region); the buttons report to the main menu
ContinueDialog::ContinueDialog(HeavyWeaponApp* theApp) :
	Dialog2004(theApp->mSprites[IMAGE_DIALOG]->mImage, NULL, DIALOG_ID, true, GT(0x52cd20), "", "", BUTTONS_NONE)
{
	mApp = theApp;
	int aBoss = mApp->GetBossIndex(mApp->CurrentUser().mLevel);
	mDialogLines = StrFormat(GTC(0x52ccd8), mApp->CurrentUser().mLevel + 1, mApp->mLevels[aBoss].mName.c_str());
	SetButtonFont(theApp->mFonts[FONT_NORMAL]);
	SetHeaderFont(theApp->mFonts[FONT_RUBBERSTAMPLET20]);
	SetLinesFont(theApp->mFonts[FONT_NORMAL]);
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0));
	SetColor(COLOR_LINES, Color(0xc0, 0xc0, 0xc0));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mContentInsets = Insets(CONTENT_INSET, CONTENT_INSET, CONTENT_INSET, CONTENT_INSET);

	ButtonListener* aListener = NULL;
	if (mApp->mMainMenu != NULL)
		aListener = mApp->mMainMenu;
	mContinueButton = new BracketButton(mApp, aListener, BUTTON_CONTINUE);
	mContinueButton->SetTint(Color(0x40, 0xc0, 0x40));
	mContinueButton->mLabel = GT(0x52cccc);
	mNewGameButton = new BracketButton(mApp, aListener, BUTTON_NEW_GAME);
	mNewGameButton->SetTint(Color(0x40, 0xc0, 0x40));
	mNewGameButton->mLabel = GT(0x52ccc0);
	mCancelButton = new BracketButton(mApp, aListener, BUTTON_CANCEL);
	mCancelButton->SetTint(Color(0xc0, 0x40, 0x40));
	mCancelButton->mLabel = GT(0x52ccb8);
	mWidth = DIALOG_WIDTH;
	mHeight = DIALOG_HEIGHT;
	Resize(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT);
}

// 0x41d7d0 (body 0x41d5f0)
ContinueDialog::~ContinueDialog()
{
	delete mContinueButton;
	delete mNewGameButton;
	delete mCancelButton;
}

// 0x41d680 slot 1
void ContinueDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mContinueButton);
	theManager->AddWidget(mNewGameButton);
	theManager->AddWidget(mCancelButton);
}

// 0x41d6c0 slot 2
void ContinueDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mContinueButton);
	theManager->RemoveWidget(mNewGameButton);
	theManager->RemoveWidget(mCancelButton);
}

// 0x41d700 slot 13: the three buttons in a row at the foot of the box
void ContinueDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mContinueButton->Resize(mWidth / 2 - 0xdc + mX, mHeight - BUTTON_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mNewGameButton->Resize(mWidth / 2 - 0x46 + mX, mHeight - BUTTON_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mCancelButton->Resize(mWidth / 2 + 0x50 + mX, mHeight - BUTTON_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
}
