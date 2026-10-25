#include "HighScoreDialog.h"
#include "HighScoreWidget.h"
#include "WinDMApp.h"
#include "Res.h"

#include "Graphics.h"
#include "ImageFont.h"

using namespace Sexy;

// 0x4ce44f
HighScoreDialog::~HighScoreDialog()
{
	// Re-saves hiscores.dat
	delete mHighScoreWidget;
}

// 0x4ce495
int HighScoreDialog::GetPreferredHeight(int theWidth)
{
	return PREFERRED_HEIGHT;
}

// 0x4ce4bc
HighScoreDialog::HighScoreDialog(WinDMApp* theApp) :
	Dialog(theApp->mImages[IMAGE_DIALOGBOX], theApp->mImages[IMAGE_BOXBUTTON], DIALOG_HIGH_SCORES, true,
		theApp->GetString("HSDLG_HIGH_SCORES"), _S(""), theApp->GetString("HSDLG_DONE"), BUTTONS_FOOTER)
{
	// Never shown: it only loads (one Rand() for its tip) and owns the tables
	mHighScoreWidget = new HighScoreWidget(theApp);
	mApp = theApp;
	SetButtonFont(mApp->mMainFont);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mMainFont);
}

// 0x4ce60b
void HighScoreDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	g->SetFont(mApp->mMainFont);

	for (int aMode = 0; aMode < HighScoreWidget::NUM_HIGH_SCORE_MODES; aMode++)
	{
		int aCenterX = mWidth / 2 + ((aMode != 0) ? 90 : -90);
		g->SetFont(mApp->mMainFont);

		g->SetColor(Color(255, 255, 128));
		HighScoreWidget::DrawHeading(g, mApp->GetString((aMode == 0) ? "HSDLG_NORMAL" : "HSDLG_TIMED"), aCenterX, 73);
		HighScoreWidget::DrawHeading(g, mApp->GetString("HSDLG_TODAYS_HIGH_SCORE"), aCenterX, 99);

		g->SetColor(Color(255, 255, 0));
		if (mHighScoreWidget->mTodayScores[aMode] < 1)
			HighScoreWidget::DrawCenteredString(g, mApp->GetString("HSDLG_EMPTY"), aCenterX, 125);
		else
			HighScoreWidget::DrawScoreRow(g, mHighScoreWidget->mTodayNames[aMode], mHighScoreWidget->mTodayScores[aMode], aCenterX, 125);

		g->SetColor(Color(255, 255, 128));
		HighScoreWidget::DrawHeading(g, mApp->GetString("HSDLG_ALL_TIME_HIGH_SCORES"), aCenterX, 155);

		g->SetColor(Color(255, 255, 0));
		for (int i = 0; i < HighScoreWidget::NUM_HIGH_SCORES; i++)
			HighScoreWidget::DrawScoreRow(g, mHighScoreWidget->mNames[aMode][i], mHighScoreWidget->mScores[aMode][i], aCenterX, 179 + i * 17);
	}
}
