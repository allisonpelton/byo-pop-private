#include "QuitDialog.h"
#include "Board.h"
#include "StatusBar.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "YesNoDialog.h"
#include "DDImage.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{
	const int DIALOG_ID = HeavyWeaponApp::DIALOG_QUIT;
	const int DIALOG_WIDTH = 300;
	const int DIALOG_HEIGHT = 0xd2;
	const int SCREEN_CENTER_X = 0x140;
	const int SCREEN_CENTER_Y = 0xf0;
	const int CONTENT_INSET = 0x32;
	const int BUTTON_WIDTH = 200;
	const int BUTTON_HEIGHT = 0x1e;
	const int RESUME_Y = 0x82;
	const int MAIN_MENU_Y = 0x5a;
	const int OPTIONS_Y = 0x32;
	const int CONFIRM_WIDTH = 0x140;
	const int CONFIRM_TALL_HEIGHT = 0xf0;	// the box that warns the progress is lost
	const int CONFIRM_SHORT_HEIGHT = 0xb4;
}

// 0x41f500: no heading or text, three green buttons; the board is told to stop for the box
QuitDialog::QuitDialog(HeavyWeaponApp* theApp) :
	Dialog2004(theApp->mSprites[IMAGE_DIALOG]->mImage, NULL, DIALOG_ID, true, "", "", "", BUTTONS_NONE)
{
	mApp = theApp;
	SetButtonFont(theApp->mFonts[FONT_NORMAL]);
	SetHeaderFont(theApp->mFonts[FONT_RUBBERSTAMPLET20]);
	SetLinesFont(theApp->mFonts[FONT_NORMAL]);
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0));
	SetColor(COLOR_LINES, Color(0x80, 0xc0, 0));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mContentInsets = Insets(CONTENT_INSET, CONTENT_INSET, CONTENT_INSET, CONTENT_INSET);
	Color aTint(0x80, 0xc0, 0x40);
	mResumeButton = new BracketButton(mApp, mApp, HeavyWeaponApp::BUTTON_QUIT_CANCEL);
	mResumeButton->SetTint(aTint);
	mResumeButton->mLabel = GT(0x52d17c);
	mMainMenuButton = new BracketButton(mApp, this, BUTTON_MAIN_MENU);
	mMainMenuButton->SetTint(aTint);
	mMainMenuButton->mLabel = GT(0x52d170);
	mOptionsButton = new BracketButton(mApp, mApp, HeavyWeaponApp::BUTTON_OPTIONS);
	mOptionsButton->SetTint(aTint);
	mOptionsButton->mLabel = GT(0x52d168);
	mWidth = DIALOG_WIDTH;
	mHeight = DIALOG_HEIGHT;
	mApp->mBoard->Suspend();
	Resize(SCREEN_CENTER_X - mWidth / 2, SCREEN_CENTER_Y - mHeight / 2, mWidth, mHeight);
}

// 0x41f4d0 (body 0x41f410)
QuitDialog::~QuitDialog()
{
	delete mResumeButton;
	delete mOptionsButton;
	delete mMainMenuButton;
}

// 0x41f310 slot 1
void QuitDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mResumeButton);
	theManager->AddWidget(mMainMenuButton);
	theManager->AddWidget(mOptionsButton);
}

// 0x41f490 slot 2
void QuitDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mResumeButton);
	theManager->RemoveWidget(mOptionsButton);
	theManager->RemoveWidget(mMainMenuButton);
}

// 0x41f350 slot 13: the buttons in a column, OPTIONS on top
void QuitDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	Rect aRect = GetInsetRect();
	mResumeButton->Resize(aRect.mWidth / 2 - 100 + mX, mY + RESUME_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	mMainMenuButton->Resize(aRect.mWidth / 2 - 100 + mX, mY + MAIN_MENU_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	mOptionsButton->Resize(aRect.mWidth / 2 - 100 + mX, mY + OPTIONS_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

// 0x42ca70 listener slot 4
void QuitDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x41f8b0 listener slot 5: MAIN MENU opens the box that asks for confirmation (the longer one warns that the mission's
// progress is lost); its YES leaves the game for the main menu, its NO closes it
void QuitDialog::ButtonDepress(int theId)
{
	HeavyWeaponApp* anApp = mApp;
	anApp->PlaySample(SOUND_BUTTONUP, 0);
	if (theId != BUTTON_MAIN_MENU)
	{
		if (theId == BUTTON_YES)
		{
			anApp->KillDialog(CONFIRM_DIALOG_ID);
			anApp->KillDialog(DIALOG_ID);
			anApp->mBoard->mStatusBar->mQuitDialog = NULL;
			anApp->ShowMainMenu();
			anApp->mWidgetManager->RemoveWidget(anApp->mBoard);
			anApp->SafeDeleteWidget(anApp->mBoard);
			anApp->mBoard = NULL;
		}
		else if (theId == BUTTON_NO)
			anApp->KillDialog(CONFIRM_DIALOG_ID);
		return;
	}
	YesNoDialog* aConfirm;
	if ((anApp->mBoard->mTankDeadTime == 0) || (anApp->mLives != 0))
		aConfirm = new YesNoDialog(anApp, this, GT(0x52d1dc), GT(0x52d188), CONFIRM_WIDTH, CONFIRM_TALL_HEIGHT, 0, 0);
	else
		aConfirm = new YesNoDialog(anApp, this, GT(0x52d1dc), GT(0x52d1e8), CONFIRM_WIDTH, CONFIRM_SHORT_HEIGHT, 0, 0);
	anApp->AddDialog(CONFIRM_DIALOG_ID, aConfirm);
}
