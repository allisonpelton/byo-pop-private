#include "CreditsDialog.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
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

	mDesignLink = new HyperlinkWidget(0, this);
	mDesignLink->SetFont(mApp->mWhiteFontSmall);
	mDesignLink->mColor = Color(200, 0xff, 200);
	mDesignLink->mLabel = GT(0x4fc67c);
	mDesignLink->mOverColor = Color(0, 0xff, 0);
	mMusicLink = new HyperlinkWidget(1, this);
	mMusicLink->SetFont(mApp->mWhiteFontSmall);
	mMusicLink->mColor = Color(200, 0xff, 200);
	mMusicLink->mLabel = GT(0x4fc674);
	mMusicLink->mOverColor = Color(0, 0xff, 0);
}

// 0x42ea10
CreditsDialog::~CreditsDialog()
{
	mApp->SafeDeleteWidget(mDesignLink);
	mApp->SafeDeleteWidget(mMusicLink);
	mDesignLink = NULL;
	mMusicLink = NULL;
}

// 0x42eaa0
int CreditsDialog::GetPreferredHeight(int theWidth)
{
	return 0x15e;
}

// 0x42eab0
void CreditsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mDesignLink);
	theWidgetManager->AddWidget(mMusicLink);
}

// 0x4410b0
void CreditsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mDesignLink);
	theWidgetManager->RemoveWidget(mMusicLink);
}

// 0x42eae0
void CreditsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mDesignLink->Resize(mX + 0xa5, mY + 0x55, 100, 0x14);
	mMusicLink->Resize(mX + 0x9e, mY + 0xa5, 100, 0x14);
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
	g->SetColor(Color(0xff, 0xff, 200, 0xff));
	g->DrawString(GT(0x4fc6a8), 0xaf - g->StringWidth(GT(0x4fc6a8)), 200);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->DrawString(GT(0x4fc69c), 0xb9, 200);
	g->SetColor(Color(0xff, 0xff, 200, 0xff));
	g->DrawString(GT(0x4fc694), 0xaf - g->StringWidth(GT(0x4fc694)), 0xdc);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->DrawString(GT(0x4fc688), 0xb9, 0xdc);
}

// 0x430b90
void CreditsDialog::ButtonDepress(int theId)
{
	Dialog2004::ButtonDepress(theId);
	if (theId == 0)
		mApp->OpenURL(GT(0x4fc728), false);
	else if (theId == 1)
		mApp->OpenURL(GT(0x4fc704), false);
}
