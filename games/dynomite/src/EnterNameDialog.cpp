#include "EnterNameDialog.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"

#ifdef __EMSCRIPTEN__
#include "WebProfiles.h"
#endif

using namespace Sexy;

// 0x52b280: the edit box's colours (r, g, b, a): no background or outline, the text pale blue, the selection white
// with black text
static int gNameEditColors[5][4] =
{
	{0, 0, 0, 0}, {0, 0, 0, 0}, {0xf0, 0xf0, 0xff, 0xff}, {0xff, 0xff, 0xff, 0xff}, {0, 0, 0, 0xff}
};

// 0x413f50
EnterNameDialog::EnterNameDialog(FossilizedApp* theApp, HighScores* theHighScores) :
	Dialog2004(theApp->mPicDialog->mImage, theApp->mPicDbutton->mImage, 6, true,
		theApp->GetString("YOU_GOT_A_HIGH_SCORE"), "", "", Dialog::BUTTONS_NONE)
{
	mHighScores = theHighScores;
	mApp = theApp;
	SetButtonFont(mApp->mWhiteFontSmall);
	SetHeaderFont(mApp->mWhiteFont);
	SetLinesFont(mApp->mWhiteFontSmall);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(3, Color(0xff, 0xff, 0xff));

	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mWhiteFontSmall);
	mNameEdit->SetColors(gNameEditColors, 5);
	mNameEdit->SetText(mApp->mLastScoreName);
	mNameEdit->mMaxChars = 0x14;
	mNameEdit->mMaxPixels = 0x73;
#ifdef __EMSCRIPTEN__
	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with the site's player
	Byopop::SiteType("dynomite", mNameEdit, false);
#endif
	mNameEdit->FocusCursor(false);

	mOkButton = new DialogButton(NULL, 100, mApp);
	mOkButton->mComponentImage = mApp->mPicDbutton->mImage;
	mOkButton->mDoFinger = true;
	mOkButton->mLabel = mApp->GetString("OKAY");
	mOkButton->SetFont(mApp->mWhiteFontSmall);
	mOkButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mOkButton->SetColor(1, Color(0xff, 0xff, 0xff));

	mCancelButton = new DialogButton(NULL, 0x65, mApp);
	mCancelButton->mComponentImage = mApp->mPicDbutton->mImage;
	mCancelButton->mDoFinger = true;
	mCancelButton->mLabel = mApp->GetString("DIALOG_BUTTON_CANCEL");
	mCancelButton->SetFont(mApp->mWhiteFontSmall);
	mCancelButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mCancelButton->SetColor(1, Color(0xff, 0xff, 0xff));
}

// 0x413d40
EnterNameDialog::~EnterNameDialog()
{
	if (mNameEdit != NULL)
		delete mNameEdit;
	if (mOkButton != NULL)
		delete mOkButton;
	if (mCancelButton != NULL)
		delete mCancelButton;
}

// 0x413dd0
int EnterNameDialog::GetPreferredHeight(int theWidth)
{
	return 200;
}

// 0x413de0
void EnterNameDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->AddWidget(mOkButton);
	theWidgetManager->AddWidget(mCancelButton);
	theWidgetManager->SetFocus(mNameEdit);
}

// 0x413e30
void EnterNameDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
	theWidgetManager->RemoveWidget(mOkButton);
	theWidgetManager->RemoveWidget(mCancelButton);
}

// 0x413e70
void EnterNameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 0x2f, mY + 0x62, 0x11e, 0x18);
	mOkButton->Resize(mX + 0x5d, mY + 0x80, 0x60, 0x20);
	mCancelButton->Resize(mX + 0xc0, mY + 0x80, 0x60, 0x20);
}

// 0x4145c0: "enter your name" over a dark strip under the edit box
void EnterNameDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mWhiteFontSmall);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mApp->GetString("ENTER_YOUR_NAME"), 0x32, 0x5c);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(mApp->GetString("ENTER_YOUR_NAME"), 0x30, 0x5a);
	g->SetColor(Color(0, 0, 0, 0x80));
	g->FillRect(0x2e, 0x61, 0x11e, 0x1a);
}

// 0x413f00: Return in the edit box is OK
void EnterNameDialog::EditWidgetText(int theId, const SexyString& theString)
{
	mApp->ButtonDepress(100);
}
