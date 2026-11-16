// NewUserDialog.cpp: see NewUserDialog.h. The original keeps its tweakable numbers in ModVal calls (each returns the number it was
// compiled with); they are named constants here, each with the line of NewUserDialog.cpp its call names. One is 0 and switches a
// block off, which is left out: the certificate never draws the edit box's frame (line 674, the text sits on the picture).

#include "NewUserDialog.h"

#include "Color.h"
#include "Common.h"
#include "EditWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "LoadingScreen.h"
#include "MainMenu.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "SexyMatrix.h"
#include "ThunderButton.h"
#include "ThunderCommon.h"
#include "ThunderballApp.h"
#include "WidgetManager.h"

#include <cctype>
#include <ctime>

using namespace Sexy;

static const int PLAIN_DIALOG_WIDTH = 300;				 // line 654: the plain dialog's width (the certificate takes its picture's)
static const int PLAIN_DIALOG_EXTRA_HEIGHT = 40;		 // line 660: what the plain dialog is taller than the framework's dialog
static const int DATE_FONT_ID = 0x1d;					 // line 676: FONT_PLAIN
static const int NAME_FONT_ID = 0x1e;					 // line 656: FONT_PLAIN2BIG
static const int NAME_COLOR = 0x60;						 // line 657: the name on the certificate (a dark blue)
static const int NAME_HILITE_COLOR = 0x80;				 // line 658
static const int NAME_HILITE_TEXT_COLOR = 0xffffff;		 // line 659
static const int DATE_COLOR = 0x80;						 // line 675
static const int DATE_CENTER_X = 135;					 // line 677: the date is centred here
static const int DATE_Y = 390;							 // line 678
static const int NO_BUTTON_COVER_COLOR = 0xd6cfc0;		 // line 679: the certificate's paper, drawn over the No button's place when
														 // it is hidden
static const int NO_BUTTON_COVER_X = 307;				 // (the rectangle itself is a literal of the original)
static const int NO_BUTTON_COVER_Y = 356;
static const int NO_BUTTON_COVER_WIDTH = 136;
static const int NO_BUTTON_COVER_HEIGHT = 50;
static const int OK_BUTTON_X = 210;						 // line 661
static const int OK_BUTTON_Y = 263;						 // line 662
static const int CERT_EDIT_X = 145;						 // line 663: the name's box on the certificate
static const int CERT_EDIT_Y = 180;						 // line 664
static const int CERT_EDIT_WIDTH = 300;					 // line 665
static const int CERT_EDIT_HEIGHT = 32;					 // line 666
static const int CERT_NO_BUTTON_X = 310;				 // line 668 (the No button of the certificate closes it)
static const int CERT_NO_BUTTON_Y = 365;				 // line 669
static const int CERT_NO_BUTTON_WIDTH = 150;			 // line 667
static const int PLAIN_EDIT_Y = 105;					 // line 670: the plain dialog's box
static const int PLAIN_EDIT_HEIGHT = 24;				 // line 671
static const int PLAIN_EDIT_INSET = 0x30;				 // its sides are this far inside the dialog's
static const int STAMP_END_TIME = 75;					 // line 672: updates after the stamp came down that the dialog goes away
static const int STAMP_SLIDE_TIME = 20;					 // line 680: the stamp is drawn large and shrinks to its size in these
static const float STAMP_START_EXTRA_SCALE = 2.0f;		 // line 681: how much larger it starts than its size
static const float STAMP_CENTER_X = 250.0f;				 // line 682
static const float STAMP_CENTER_Y = 305.0f;				 // line 683
static const int LOADING_SCREEN_START_DELAY = 20;		 // line 673: updates until the adventure starts after the stamp

// 0x4a9380
NewUserDialog::NewUserDialog(ThunderballApp* theApp, bool theRename, bool theCertificate, bool theCanCancel)
	: ThunderDialog(theRename ? DIALOG_RENAME_USER : DIALOG_CREATE_USER, true, theRename ? "Rename User" : "New User",
					"Please enter your name.", "", BUTTONS_OK_CANCEL)
{
	mApp = theApp;
	mCertificate = theCertificate;
	mWidth = theCertificate ? IMAGE_MM_CERTIFICATE->mWidth : PLAIN_DIALOG_WIDTH;
	mOkButton = nullptr;
	mStampTime = 0;

	mEditWidget = CreateEditWidget(0, this);
	if (mCertificate)
	{
		time_t aNow = time(nullptr);
		struct tm* aTime = localtime(&aNow);
		mDateText = StrFormat("%d / %d / %d", aTime->tm_mon + 1, aTime->tm_mday, aTime->tm_year + 1900);

		mEditWidget->SetFont(GetFontById(NAME_FONT_ID), nullptr);
		if (!theCanCancel)
			mNoButton->SetVisible(false);
		mYesButton->SetVisible(false); // the picture button OK takes its place

		mOkButton = MakeEmbeddedButton(Dialog::ID_YES, this, IMAGE_MM_CERTIFICATEOK, true);
		AddWidget(mOkButton);
		mEditWidget->SetColor(EditWidget::COLOR_TEXT, Color(NAME_COLOR));
		mEditWidget->SetColor(EditWidget::COLOR_HILITE, Color(NAME_HILITE_COLOR));
		mEditWidget->SetColor(EditWidget::COLOR_HILITE_TEXT, Color(NAME_HILITE_TEXT_COLOR));
	}
	DoNameWidthEnforce(mEditWidget);
}

// the edit widget is a child of the manager, not of the dialog, so the manager takes it off in RemovedFromManager; the dialog's
// own children (buttons) are deleted by the widget container
NewUserDialog::~NewUserDialog()
{
	delete mEditWidget;
}

// 0x4952e0
int NewUserDialog::GetPreferredHeight(int theWidth)
{
	if (mCertificate)
		return IMAGE_MM_CERTIFICATE->mHeight;
	return Dialog::GetPreferredHeight(theWidth) + PLAIN_DIALOG_EXTRA_HEIGHT;
}

// 0x495330
void NewUserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mEditWidget);
	theWidgetManager->SetFocus(mEditWidget);
}

// 0x495370
void NewUserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mEditWidget);
}

// 0x4953a0
void NewUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	if (mCertificate)
	{
		mOkButton->Move(OK_BUTTON_X, OK_BUTTON_Y);
		mEditWidget->Resize(CERT_EDIT_X + mX, CERT_EDIT_Y + mY, CERT_EDIT_WIDTH, CERT_EDIT_HEIGHT);
		mNoButton->Resize(CERT_NO_BUTTON_X + mX, CERT_NO_BUTTON_Y + mY, CERT_NO_BUTTON_WIDTH, mNoButton->mHeight);
		return;
	}
	mEditWidget->Resize(mX + PLAIN_EDIT_INSET, PLAIN_EDIT_Y + mY, mWidth - 2 * PLAIN_EDIT_INSET, PLAIN_EDIT_HEIGHT);
}

// 0x495530: the stamp comes down large and shrinks onto the certificate
void NewUserDialog::DrawOverlay(Graphics* g)
{
	if (mStampTime <= 0)
		return;

	Transform aTransform;
	if (mStampTime < STAMP_SLIDE_TIME)
	{
		float aScale = (float)(STAMP_START_EXTRA_SCALE * (1.0 - (double)mStampTime / STAMP_SLIDE_TIME) + 1.0);
		aTransform.Scale(aScale, aScale);
	}
	g->DrawImageTransform(IMAGE_MM_APPROVEDSTAMP, aTransform, STAMP_CENTER_X, STAMP_CENTER_Y);
}

// 0x495610
void NewUserDialog::EditWidgetText(int theId, const std::string& theString)
{
	mApp->DialogButtonDepress(mId, Dialog::ID_YES);
}

// 0x495630
bool NewUserDialog::AllowChar(int theId, char theChar)
{
	return isalnum((unsigned char)theChar) != 0 || theChar == ' ';
}

// 0x499d70
void NewUserDialog::Draw(Graphics* g)
{
	if (!mCertificate)
	{
		ThunderDialog::Draw(g);
		DrawEditBox(g, mEditWidget);
		return;
	}

	g->DrawImage(IMAGE_MM_CERTIFICATE, 0, 0);
	g->SetColor(Color(DATE_COLOR));
	g->SetFont(GetFontById(DATE_FONT_ID));
	g->DrawString(mDateText, DATE_CENTER_X - g->GetFont()->StringWidth(mDateText) / 2, DATE_Y);
	if (mNoButton->mVisible)
	{
		g->SetColor(Color(NO_BUTTON_COVER_COLOR));
		g->FillRect(NO_BUTTON_COVER_X, NO_BUTTON_COVER_Y, NO_BUTTON_COVER_WIDTH, NO_BUTTON_COVER_HEIGHT);
	}
	if (mStampTime > 0)
		DeferOverlay(0);
}

// 0x499ef0
void NewUserDialog::SetName(const std::string& theName)
{
	mEditWidget->SetText(theName, true);
	mEditWidget->mCursorPos = (int)theName.size();
	mEditWidget->mHilitePos = 0;
}

// 0x49cb00: after the stamp has been seen for a while the dialog leaves and the loading screen goes on to the adventure
void NewUserDialog::Update()
{
	ThunderDialog::Update();
	if (mStampTime == 0)
		return;

	mStampTime++;
	if (mStampTime != STAMP_END_TIME)
		return;

	if (mApp->mCurrentPlayer == nullptr)
	{
		mStampTime = 0;
		return;
	}
	mApp->KillDialog(DIALOG_USER);
	if (mApp->mLoadingScreen != nullptr)
		mApp->mLoadingScreen->mStartAdventureCountdown = LOADING_SCREEN_START_DELAY;
	mApp->DoScrollOff(mId);
	if (mApp->mMainMenu != nullptr)
		mWidgetManager->SetFocus(mApp->mMainMenu);
}

// 0x49f890
std::string NewUserDialog::GetName() const
{
	const std::string& aText = mEditWidget->mString;
	std::string aName;
	char aPrevious = ' '; // a space at the start counts as a doubled one
	for (size_t aCharIndex = 0; aCharIndex < aText.size(); aCharIndex++)
	{
		char aChar = aText[aCharIndex];
		if (aChar != ' ' || aPrevious != ' ')
			aName += aChar;
		aPrevious = aChar;
	}
	if (!aName.empty() && aName[aName.size() - 1] == ' ')
		aName.erase(aName.size() - 1);
	return aName;
}

// 0x495660
void NewUserDialog::StartStamp()
{
	if (mOkButton == nullptr)
		return;

	mOkButton->SetDisabled(true);
	if (mNoButton != nullptr)
		mNoButton->SetDisabled(true);
	mStampTime = 1;
	mApp->PlaySample(SOUND_STAMP);
}
