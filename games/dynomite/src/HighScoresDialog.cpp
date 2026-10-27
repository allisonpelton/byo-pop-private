#include "HighScoresDialog.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "HighScores.h"
#include "ImageFont.h"
#include "Sprite.h"

using namespace Sexy;

// 0x421c50
HighScoresDialog::HighScoresDialog(FossilizedApp* theApp) :
	Dialog2004(theApp->mPicDialog->mImage, theApp->mPicDbutton->mImage, 4, true,
		theApp->GetString("HSDLG_HIGH_SCORES"), "", theApp->GetString("HSDLG_DONE"), Dialog::BUTTONS_FOOTER)
{
	mApp = theApp;
	SetButtonFont(theApp->mWhiteFontSmall);
	SetHeaderFont(mApp->mWhiteFont);
	SetLinesFont(mApp->mWhiteFontSmall);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	Dialog::SetColor(3, Color(0xff, 0xff, 0xff));
	mTables[0] = new HighScores(0);
	mTables[1] = new HighScores(1);
	mTables[2] = new HighScores(2);
	mTables[3] = new HighScores(3);
}

// 0x421b40
HighScoresDialog::~HighScoresDialog()
{
	if (mTables[0] != NULL)
		delete mTables[0];
	if (mTables[1] != NULL)
		delete mTables[1];
	if (mTables[2] != NULL)
		delete mTables[2];
	if (mTables[3] != NULL)
		delete mTables[3];
}

// 0x421bf0
int HighScoresDialog::GetPreferredHeight(int theWidth)
{
	return 400;
}

// 0x421ff0
void HighScoresDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mWhiteFontSmall);
	g->SetColor(Color(0xff, 0xff, 200));
	std::string aTitle = mApp->GetString("STOMPED");
	g->DrawString(aTitle, 0x7f - g->GetFont()->StringWidth(aTitle) / 2, 0x5c);
	mTables[0]->Draw(g, 0x2d, 100, 0xa5, 0x5a);
	aTitle = mApp->GetString("ENDLESS");
	g->DrawString(aTitle, 0x12e - g->GetFont()->StringWidth(aTitle) / 2, 0x5c);
	mTables[1]->Draw(g, 0xdc, 100, 0xa5, 0x5a);
	aTitle = mApp->GetString("FOSSIL");
	g->DrawString(aTitle, 0x7f - g->GetFont()->StringWidth(aTitle) / 2, 0xd4);
	mTables[2]->Draw(g, 0x2d, 0xdc, 0xa5, 0x5a);
	aTitle = mApp->GetString("HSDLG_TIME_TRIAL");
	g->DrawString(aTitle, 0x12e - g->GetFont()->StringWidth(aTitle) / 2, 0xd4);
	mTables[3]->Draw(g, 0xdc, 0xdc, 0xa5, 0x5a);
}

// 0x421c00
void HighScoresDialog::ButtonPress(int theId)
{
	mApp->mSndClick->Play();
	Dialog2004::ButtonPress(theId);
}
