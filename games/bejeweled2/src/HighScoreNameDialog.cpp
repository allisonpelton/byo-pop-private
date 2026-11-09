// HighScoreNameDialog.cpp: HighScoreNameDialog (HighScoreNameDialog.h).
#include "HighScoreNameDialog.h"

#include "EditWidget.h"
#include "Graphics.h"
#include "Res.h"
#include "WidgetManager.h"
#include "WinDMApp.h"

using namespace Sexy;

// 0x5d3382: a PodDialog with the OK and cancel buttons (dialog 10) and an edit field that starts with the last name entered
HighScoreNameDialog::HighScoreNameDialog(WinDMApp* theApp, const SexyString& theLines) :
	PodDialog(theApp, DIALOG_HIGH_SCORE_NAME, true, theApp->GetString("RECORDSCOREDLG_HIGH_SCORE"), theLines, "", Dialog::BUTTONS_OK_CANCEL)
{
	// the colors of the edit field (red, green, blue, alpha): clear background and outline, light text, white
	// highlight with black text on it (the exe's data at 0x66b680, the same as the user dialogs')
	static int sEditColors[5][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}};
	mEdit = new EditWidget(0, this);
	mEdit->SetFont(FONT_CONTINUUM_MED_14, nullptr);
	mEdit->SetColors(sEditColors, 5);
	mEdit->mMaxPixels = 0x68;
	mEdit->SetText(theApp->mLastScoreName, true);
}

// 0x5d31e4
HighScoreNameDialog::~HighScoreNameDialog()
{
	delete mEdit;
}

// 0x5d323b
int HighScoreNameDialog::GetPreferredHeight(int theWidth)
{
	return PodDialog::GetPreferredHeight(theWidth) + gApp->ScaleNum(0x4e, 0);
}

// 0x5d3265
void HighScoreNameDialog::AddedToManager(WidgetManager* theManager)
{
	PodDialog::AddedToManager(theManager);
	theManager->AddWidget(mEdit);
	theManager->SetFocus(mEdit);
}

// 0x5d3293
void HighScoreNameDialog::RemovedFromManager(WidgetManager* theManager)
{
	PodDialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mEdit);
}

// 0x5d32b6: the edit field sits at the bottom of the dialog, above the buttons
void HighScoreNameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	PodDialog::Resize(theX, theY, theWidth, theHeight);
	int aHeight = gApp->ScaleNum(0x20, 0);
	int aWidth = ((mWidth - mContentInsets.mRight) - mContentInsets.mLeft) - gApp->ScaleNum(0x14, 0);
	int aY = (mHeight + mY) - gApp->ScaleNum(0x8a, 0);
	mEdit->Resize(gApp->ScaleNum(10, 0) + mX + mContentInsets.mLeft, aY, aWidth, aHeight);
}

// 0x5d334d: Enter in the edit field presses the OK button (through the app, as the buttons do)
void HighScoreNameDialog::EditWidgetText(int theId, const SexyString& theString)
{
	mApp->ButtonDepress(mId + 2000);
}

// 0x5d34c8: the box around the edit field and the request above it
void HighScoreNameDialog::Draw(Graphics* g)
{
	PodDialog::Draw(g);
	Rect aRect = mEdit->GetRect();
	aRect.mX -= mX;
	aRect.mY -= mY;
	int aGrowX = gApp->ScaleNum(4, 0);
	int aGrowY = gApp->ScaleNum(4, 0);
	aRect = aRect.Inflate(aGrowX, aGrowY);
	DrawEditBox(g, aRect);

	g->SetFont(FONT_2);
	g->SetColor(Color(255, 255, 0));
	SexyString aRequest = mApp->GetString("RECORDSCOREDLG_ENTER_YOUR_NAME");
	g->DrawString(aRequest, mEdit->mX - mX, (mEdit->mY - mY) - gApp->ScaleNum(0xc, 0));
}
