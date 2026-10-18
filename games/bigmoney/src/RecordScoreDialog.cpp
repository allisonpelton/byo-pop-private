#include "RecordScoreDialog.h"
#include "WinBMApp.h"
#include "Res.h"

#include "Graphics.h"
#include "ImageFont.h"
#include "EditWidget.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x417730
RecordScoreDialog::~RecordScoreDialog()
{
	delete mNameWidget;
}

// 0x4177a0
int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	return Dialog::GetPreferredHeight(theWidth) + EXTRA_HEIGHT;
}

// 0x4177b0
void RecordScoreDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameWidget);
	theWidgetManager->SetFocus(mNameWidget);
}

// 0x4177e0
void RecordScoreDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameWidget);
}

// 0x417810
void RecordScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mNameWidget->Resize(mX + 48, mY + mHeight - 110, mWidth - 96, 24);
}

// 0x417860
void RecordScoreDialog::EditWidgetText(int theId, const std::string& theString)
{
	// Enter acts as OK (2003), without Dialog::ButtonDepress and without a click sound
	mApp->ButtonDepress(2000 + mId);
}

// 0x4178b0
void RecordScoreDialog::DrawEditBox(Graphics* g, const Rect& theRect)
{
	Image* anEditBoxImage = mApp->mImages[IMAGE_EDITBOX];
	g->DrawImage(anEditBoxImage, theRect.mX, theRect.mY, Rect(0, 0, 32, 32));
	g->DrawImage(anEditBoxImage, theRect.mX + theRect.mWidth - 32, theRect.mY, Rect(64, 0, 32, 32));

	// The middle tiles come after the right cap and may overdraw it (the clip ends at the right edge)
	Graphics aMiddleG(*g);
	aMiddleG.ClipRect(theRect.mX + 32, theRect.mY, theRect.mWidth - 32, 32);
	for (int i = 0; i < (theRect.mWidth - 33) / 32; i++)
		aMiddleG.DrawImage(anEditBoxImage, theRect.mX + 32 + i * 32, theRect.mY, Rect(32, 0, 32, 32));
}

// 0x417a30
RecordScoreDialog::RecordScoreDialog(WinBMApp* theApp, const std::string& theLines) :
	MoneyDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_RECORD_SCORE, false,
		theApp->GetString("RSDLG_HIGH_SCORE"), theLines, "", BUTTONS_OK_CANCEL)
{
	// Modeless: the game-over board stays usable while it is up
	mApp = theApp;
	SetButtonFont(mApp->mFont3);
	SetHeaderFont(mApp->mFont1);
	SetLinesFont(mApp->mFont3);
	mContentInsets = Insets(38, 38, 38, 38);
	SetColor(COLOR_LINES, Color(255, 173, 17));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 255, 0));

	mNameWidget = new EditWidget(NAME_EDIT_ID, this);
	mNameWidget->SetFont(mApp->mFont3, NULL);
	mNameWidget->SetColors(gRecordScoreEditColors, 5);
	mNameWidget->mMaxPixels = NAME_MAX_PIXELS;
	mNameWidget->SetText(mApp->mLastScoreName, true);
}

// 0x417d00
void RecordScoreDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);

	Rect aRect = mNameWidget->GetRect();
	DrawEditBox(g, Rect(aRect.mX - mX - 4, aRect.mY - mY - 4, aRect.mWidth + 8, aRect.mHeight + 8));

	g->SetFont(mApp->mFont3);
	g->SetColor(Color(255, 173, 17));
	g->DrawString(mApp->GetString("ENTER_YOUR_NAME"), mNameWidget->mX - mX, mNameWidget->mY - mY - 8);
}
