#include "MessageDialog.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "GameText.h"
#include "Sprite.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{
	const int SCREEN_CENTER_X = 0x140;
	const int SCREEN_CENTER_Y = 0xf0;
	const int CONTENT_INSET = 0x19;
	const int BUTTON_WIDTH = 100;
	const int BUTTON_HEIGHT = 0x1e;
	const int BUTTON_BOTTOM = 0x3c;		// the button's top is this far above the box's bottom
}

// 0x429b00: a box of the dialog picture, with no buttons of the framework's: its own OK button, green, closes it
MessageDialog::MessageDialog(HeavyWeaponApp* theApp, int theWidth, int theHeight, const std::string& theHeader,
	const std::string& theLines) :
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
	mOkButton = new BracketButton(mApp, this, BUTTON_OK);
	mOkButton->SetTint(Color(0x40, 0xc0, 0x40));
	mOkButton->mLabel = GT(0x52d4f8);
	mHeight = theHeight;
	mWidth = theWidth;
	Resize(SCREEN_CENTER_X - theWidth / 2, SCREEN_CENTER_Y - theHeight / 2, theWidth, theHeight);
}

// 0x429ad0 (body 0x4299f0)
MessageDialog::~MessageDialog()
{
	delete mOkButton;
}

// 0x429950 slot 1
void MessageDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mOkButton);
}

// 0x429a50 slot 2
void MessageDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mOkButton);
}

// 0x429980 slot 13: the OK button sits at the foot of the box, in the middle
void MessageDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	Rect aRect = GetInsetRect();
	mOkButton->Resize(aRect.mWidth / 2 - 0x32 + mX, mHeight - BUTTON_BOTTOM + mY, BUTTON_WIDTH, BUTTON_HEIGHT);
}

// 0x429a80 listener slot 4
void MessageDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x429aa0 listener slot 5: OK closes the box
void MessageDialog::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	mApp->KillDialog(DIALOG_ID);
}
