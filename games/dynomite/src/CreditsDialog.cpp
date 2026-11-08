#include "CreditsDialog.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x42fed0
CreditsDialog::CreditsDialog(FossilizedApp* theApp) :
	Dialog2004(theApp->mPicDialog->mImage, theApp->mPicDbutton->mImage, 0x16, true, GT(0x4fc4a8), "", GT(0x4fc63c),
		Dialog::BUTTONS_FOOTER)	// the header and footer are the texts themselves, not string ids
{
	mApp = theApp;
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetButtonFont(mApp->mWhiteFontSmall);
	SetHeaderFont(mApp->mWhiteFont);
	SetLinesFont(mApp->mWhiteFontSmall);
}

// 0x42ea10
CreditsDialog::~CreditsDialog()
{
}

// 0x42eaa0
int CreditsDialog::GetPreferredHeight(int theWidth)
{
	return 0x15e;
}

// 0x4302b0
void CreditsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mWhiteFontSmall);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->SetColor(Color(0xff, 0xff, 200, 0xff));
	g->DrawString(GT(0x4fc6f4), 0xaf - g->StringWidth(GT(0x4fc6f4)), 100);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->DrawString(GT(0x4fc67c), 0xb9, 100);	// port: the original's link to the designer's site
	g->SetColor(Color(0xff, 0xff, 200, 0xff));
	g->DrawString(GT(0x4fc6e4), 0xaf - g->StringWidth(GT(0x4fc6e4)), 0x78);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->DrawString(GT(0x4fc6d8), 0xb9, 0x78);
	g->SetColor(Color(0xff, 0xff, 200, 0xff));
	g->DrawString(GT(0x4fc6d0), 0xaf - g->StringWidth(GT(0x4fc6d0)), 0x8c);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->DrawString(GT(0x4fc6d8), 0xb9, 0x8c);
	g->DrawString(GT(0x4fc6c0), 0xb9, 0xa0);
	g->SetColor(Color(0xff, 0xff, 200, 0xff));
	g->DrawString(GT(0x4fc6b8), 0xaf - g->StringWidth(GT(0x4fc6b8)), 0xb4);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->DrawString(GT(0x4fc674), 0xb9, 0xb4);	// port: the original's link to the musician's site
	g->SetColor(Color(0xff, 0xff, 200, 0xff));
	g->DrawString(GT(0x4fc6a8), 0xaf - g->StringWidth(GT(0x4fc6a8)), 200);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->DrawString(GT(0x4fc69c), 0xb9, 200);
	g->SetColor(Color(0xff, 0xff, 200, 0xff));
	g->DrawString(GT(0x4fc694), 0xaf - g->StringWidth(GT(0x4fc694)), 0xdc);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->DrawString(GT(0x4fc688), 0xb9, 0xdc);
}
