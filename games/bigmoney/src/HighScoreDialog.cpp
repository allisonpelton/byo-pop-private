#include "HighScoreDialog.h"
#include "HighScoreWidget.h"
#include "WinBMApp.h"
#include "Res.h"

#include "Graphics.h"
#include "Font.h"
#include "ImageFont.h"

using namespace Sexy;

// 0x412280
HighScoreDialog::~HighScoreDialog()
{
	// Re-saves hiscores.dat from this dialog's copy of the tables. The copy was read when the dialog
	// opened, so this only loses a score if the dialog was opened before that score was added (e.g.
	// Options > Scores while the modeless RecordScoreDialog is still up). Kept as in the original;
	// the Board's widget writes its tables again when it is destroyed.
	delete mHighScoreWidget;
}

// 0x412310
HighScoreDialog::HighScoreDialog(WinBMApp* theApp) :
	MoneyDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_HIGH_SCORES, true,
		"", "", theApp->GetString("DONE"), BUTTONS_FOOTER)
{
	// Never shown: it only loads and owns the tables
	mHighScoreWidget = new HighScoreWidget(theApp);
	mApp = theApp;

	SetButtonFont(mApp->mFont3);
	SetHeaderFont(mApp->mFont1);
	SetLinesFont(mApp->mFont3);
	mContentInsets = Insets(36, 36, 36, 36);
	SetColor(COLOR_BUTTON_TEXT, Color(224, 183, 40)); // ShowHighScoreDialog sets it to green afterwards
}

// 0x4125c0
void HighScoreDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);
	g->SetFont(mApp->mFont3);

	static const char* const aTitleIds[HighScoreWidget::NUM_HIGH_SCORE_MODES] =
	{
		"ACTION_HIGH_SCORES", "STRATEGY_HIGH_SCORES", "PUZZLE_HIGH_SCORES"
	};

	for (int aMode = 0; aMode < HighScoreWidget::NUM_HIGH_SCORE_MODES; aMode++)
	{
		int aY = TABLE_Y + aMode * TABLE_PITCH;

		// (the original fetches all three titles on every pass and uses the one of this mode)
		g->SetFont(mApp->mFont3Underlined);
		g->SetColor(Color(255, 255, 255));
		SexyString aTitle = mApp->GetString(aTitleIds[aMode]);
		g->DrawString(aTitle, (mWidth - g->GetFont()->StringWidth(aTitle)) / 2, aY - 19);

		g->SetFont(mApp->mFont3);
		g->SetColor(Color(32, 255, 32));
		for (int i = 0; i < HighScoreWidget::NUM_HIGH_SCORES; i++)
		{
			int aRowY = aY + i * ROW_PITCH;
			g->DrawString(mHighScoreWidget->mNames[aMode][i], COLUMN_INSET, aRowY);
			SexyString aScore = StrFormat("%d", mHighScoreWidget->mScores[aMode][i]);
			g->DrawString(aScore, mWidth - g->GetFont()->StringWidth(aScore) - COLUMN_INSET, aRowY);
		}
	}
}

// 0x40ee80 (one body with CreditsDialog::GetPreferredHeight)
int HighScoreDialog::GetPreferredHeight(int theWidth)
{
	return PREFERRED_HEIGHT;
}
