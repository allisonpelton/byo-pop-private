#include "YesNoDialog.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "DDImage.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{
	const int SCREEN_CENTER_X = 0x140;
	const int SCREEN_CENTER_Y = 0xf0;
	const int CONTENT_INSET = 0x19;
	const int BUTTON_WIDTH = 0x78;
	const int BUTTON_HEIGHT = 0x1e;
	const int BUTTON_BOTTOM = 0x37;		// the buttons' tops are this far above the box's bottom
}

// 0x41d320: the buttons' ids are the ones given, or 0x21 and 0x22 for a 0
YesNoDialog::YesNoDialog(HeavyWeaponApp* theApp, ButtonListener* theListener, const std::string& theHeader,
	const std::string& theLines, int theWidth, int theHeight, int theYesId, int theNoId) :
	Dialog2004(theApp->mSprites[IMAGE_DIALOG]->mImage, NULL, DIALOG_ID, true, theHeader, theLines, "", BUTTONS_NONE)
{
	mApp = theApp;
	SetButtonFont(theApp->mFonts[FONT_NORMAL]);
	SetHeaderFont(theApp->mFonts[FONT_RUBBERSTAMPLET20]);
	SetLinesFont(theApp->mFonts[FONT_NORMAL]);
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0));
	SetColor(COLOR_LINES, Color(0xc0, 0xc0, 0xc0));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mContentInsets = Insets(CONTENT_INSET, CONTENT_INSET, CONTENT_INSET, CONTENT_INSET);
	if (theYesId == 0)
		theYesId = DEFAULT_YES_ID;
	if (theNoId == 0)
		theNoId = DEFAULT_NO_ID;
	mYesButton2 = new BracketButton(mApp, theListener, theYesId);
	mYesButton2->SetTint(Color(0x40, 0xc0, 0x40));
	mYesButton2->mLabel = GT(0x52cbac);
	mNoButton2 = new BracketButton(mApp, theListener, theNoId);
	mNoButton2->SetTint(Color(0xc0, 0x40, 0x40));
	mNoButton2->mLabel = GT(0x52cba8);
	mHeight = theHeight;
	mWidth = theWidth;
	Resize(SCREEN_CENTER_X - theWidth / 2, SCREEN_CENTER_Y - theHeight / 2, theWidth, theHeight);
}

// 0x41d2f0 (body 0x41d230)
YesNoDialog::~YesNoDialog()
{
	delete mYesButton2;
	delete mNoButton2;
}

// 0x41d160 slot 1
void YesNoDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mYesButton2);
	theManager->AddWidget(mNoButton2);
}

// 0x41d2b0 slot 2
void YesNoDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mYesButton2);
	theManager->RemoveWidget(mNoButton2);
}

// 0x41d1a0 slot 13: the two buttons side by side at the foot of the box
void YesNoDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	Rect aRect = GetInsetRect();
	mYesButton2->Resize(aRect.mWidth / 2 - 0x7d + mX, mHeight - BUTTON_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
	mNoButton2->Resize(aRect.mWidth / 2 + 5 + mX, mHeight - BUTTON_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
}
