#include "RecordScoreDialog.h"
#include "WinDMApp.h"
#include "Res.h"

#include "Graphics.h"
#include "ImageFont.h"
#include "EditWidget.h"
#include "WidgetManager.h"
#ifdef __EMSCRIPTEN__
#include "WebProfiles.h"
#endif

using namespace Sexy;

// 0x4d2728
RecordScoreDialog::~RecordScoreDialog()
{
	delete mNameWidget;
}

// 0x4d2778
int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	return Dialog::GetPreferredHeight(theWidth) + EXTRA_HEIGHT;
}

// 0x4d278a
void RecordScoreDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameWidget);
	theWidgetManager->SetFocus(mNameWidget);
}

// 0x4d27b8
void RecordScoreDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameWidget);
}

// 0x4d27db
void RecordScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mNameWidget->Resize(mX + 48, mY + mHeight - 100, mWidth - 96, 24);
}

// 0x4d2822
void RecordScoreDialog::EditWidgetText(int theId, const SexyString& theString)
{
	// Enter acts as OK, without Dialog::ButtonDepress and without a click sound
	mApp->ButtonDepress(2000 + mId);
}

// 0x4d2857
void RecordScoreDialog::DrawEditBox(Graphics* g, const Rect& theRect)
{
	Image* anEditBoxImage = mApp->mImages[IMAGE_EDITBOX];
	g->DrawImage(anEditBoxImage, theRect.mX, theRect.mY, Rect(0, 0, 32, 32));
	g->DrawImage(anEditBoxImage, theRect.mX + theRect.mWidth - 32, theRect.mY, Rect(64, 0, 32, 32));

	// The middle tiles come after the right cap and may overdraw it (clip ends at the right edge)
	Graphics aMiddleG(*g);
	aMiddleG.ClipRect(theRect.mX + 32, theRect.mY, theRect.mWidth - 32, 32);
	for (int i = 0; i < (theRect.mWidth - 33) / 32; i++)
		aMiddleG.DrawImage(anEditBoxImage, theRect.mX + 32 + i * 32, theRect.mY, Rect(32, 0, 32, 32));
}

// 0x4d29b6
RecordScoreDialog::RecordScoreDialog(WinDMApp* theApp, const SexyString& theLines) :
	Dialog(theApp->mImages[IMAGE_DIALOGBOX], theApp->mImages[IMAGE_BOXBUTTON], DIALOG_RECORD_SCORE, true,
		theApp->GetString("RECORDSCOREDLG_HIGH_SCORE"), theLines, _S(""), BUTTONS_OK_CANCEL)
{
	mApp = theApp;
	SetButtonFont(mApp->mMainFont);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mMainFont);

	mNameWidget = new EditWidget(NAME_EDIT_ID, this);
	mNameWidget->SetFont(mApp->mMainFont, NULL);
	mNameWidget->SetColors(gRecordScoreEditColors, 5);
	mNameWidget->mMaxPixels = NAME_MAX_PIXELS;
	mNameWidget->SetText(mApp->mLastScoreName, true);
#ifdef __EMSCRIPTEN__
	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with the site's player
	Byopop::SiteType("bejeweled", mNameWidget, true);
#endif
}

// 0x4d2b34
void RecordScoreDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	Rect aRect = mNameWidget->GetRect();
	aRect.mY -= mY;
	aRect.mX -= mX;
	aRect.Inflate(4, 4);
	DrawEditBox(g, aRect);

	g->SetFont(mApp->mMainFont);
	g->SetColor(Color(255, 255, 0));
	g->DrawString(mApp->GetString("RECORDSCOREDLG_ENTER_YOUR_NAME"), mNameWidget->mX - mX, mNameWidget->mY - mY - 8);
}
