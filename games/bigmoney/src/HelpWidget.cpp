#include "HelpWidget.h"
#include "WinBMApp.h"
#include "Res.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"

using namespace Sexy;

// 0x410f20
HelpWidget::HelpWidget(WinBMApp* theApp)
{
	mApp = theApp;

	mStartLink = new HyperlinkWidget(START_LINK_ID, this);
	mStartLink->mLabel = mApp->GetString((mApp->mBoard == NULL) ? "CLICK_HERE_TO_START_GAME" : "CLICK_HERE_TO_CONTINUE_GAME");
	mStartLink->mDoFinger = true;
	mStartLink->SetFont(mApp->mFont1);
	mStartLink->mUnderlineSize = 0;
	mStartLink->mColor = Color(0, 249, 183);
	int aWidth = mStartLink->mFont->StringWidth(mStartLink->mLabel) + 4;
	mStartLink->Resize(406 - aWidth / 2, 411, aWidth, 26);

	// Never sized, so it is never shown or clicked (original).
	mMoreLink = new HyperlinkWidget(MORE_LINK_ID, this);
	mMoreLink->mLabel = mApp->GetString("CLICK_HERE_FOR_MORE_INSTRUCTIONS");
	mMoreLink->mDoFinger = true;
	mMoreLink->SetFont(mApp->mFont1Underlined);
	mMoreLink->mUnderlineSize = 0;
	mMoreLink->mColor = Color(217, 33, 153);
	// The original computes the label's StringWidth here and discards it.

	mDontShowLink = new HyperlinkWidget(DONT_SHOW_LINK_ID, this);
	mDontShowLink->mLabel = mApp->GetString("DONT_SHOW_THIS_SCREEN_AGAIN");
	mDontShowLink->mDoFinger = true;
	mDontShowLink->SetFont(mApp->mFont1);
	mDontShowLink->mUnderlineSize = 0;
	mDontShowLink->mColor = Color(98, 98, 98);
	mDontShowLink->mOverColor = Color(212, 212, 212);
	aWidth = mDontShowLink->mFont->StringWidth(mDontShowLink->mLabel) + 4;
	mDontShowLink->Resize(406 - aWidth / 2, 446, aWidth, 26);
	if (mApp->mBoard != NULL)
		mDontShowLink->SetVisible(false);

	mPage = 0;
}

// 0x410d80
HelpWidget::~HelpWidget()
{
	delete mStartLink;
	// port: the original deletes only mStartLink and leaks the other two links.
	delete mMoreLink;
	delete mDontShowLink;
}

// 0x410df0
void HelpWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mStartLink);
	theWidgetManager->AddWidget(mMoreLink);
	theWidgetManager->AddWidget(mDontShowLink);
}

// 0x410e30
void HelpWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mStartLink);
	theWidgetManager->RemoveWidget(mMoreLink);
	theWidgetManager->RemoveWidget(mDontShowLink);
}

// 0x410e70
void HelpWidget::OrderInManagerChanged()
{
	mWidgetManager->PutInfront(mDontShowLink, this);
	mWidgetManager->PutInfront(mMoreLink, this);
	mWidgetManager->PutInfront(mStartLink, this);
}

// 0x410eb0
void HelpWidget::ButtonPress(int theId)
{
	// Acts on mouse down.
	mApp->PlaySample(SOUND_BUTTONCLICK);

	if (theId == DONT_SHOW_LINK_ID)
		mApp->mDontShowHelp[mApp->mGameType] = true; // saved as DontShowHelp0..2; then as the start link
	else if (theId != START_LINK_ID)
		return;

	if (mApp->mBoard != NULL)
		mApp->RemoveHelp(); // back to the paused game
	else
		mApp->NewGame();
}

// 0x411590
void HelpWidget::Draw(Graphics* g)
{
	// Text goes through Widget::WriteString, which applies the ^RRGGBB^ colour codes in the strings.
	g->SetColor(Color(0, 0, 0));
	g->FillRect(0, 0, mWidth, mHeight);
	g->SetColor(Color(77, 77, 77));
	g->FillRect(0, 404, mWidth, 40);

	if (mPage == 0)
	{
		Image* aCircles = mApp->mImages[IMAGE_HELPCIRCLES];
		Image* anArrow = mApp->mImages[IMAGE_BIGARROW];

		if (mApp->mGameType == GAMETYPE_PUZZLE)
		{
			g->SetFont(mApp->mFont1Underlined);
			WriteString(g, mApp->GetString("HOW_TO_PLAY_BIG_MONEY_PUZZLE"), 210, 28, -1, -1, true, 0, -1);
			g->DrawImage(aCircles, 527, 40, Rect(0, 330, 104, 330));
			g->SetFont(mApp->mFont1);
			g->DrawImage(anArrow, 154, 52);
			WriteString(g, mApp->GetString("CLICK_ON_GROUPS_OF_X_OR_MORE"), 194, 93, -1, -1, true, 0, -1);
			WriteString(g, mApp->GetString("X_COINS_TO_COLLECT_THEM"), 194, 113, -1, -1, true, 0, -1);
			g->DrawImage(anArrow, 154, 162);
			WriteString(g, mApp->GetString("X_YOU_MUST_DROP_MONEYBAGS_OFF"), 194, 203, -1, -1, true, 0, -1);
			WriteString(g, mApp->GetString("X_THE_X_BOTTOM_X_TO_COLLECT_THEM"), 194, 223, -1, -1, true, 0, -1);
			g->DrawImage(anArrow, 154, 272);
			WriteString(g, mApp->GetString("X_THE_GAME_ENDS_WHEN_YOU_X_CANNOT"), 194, 313, -1, -1, true, 0, -1);
			WriteString(g, mApp->GetString("X_COLLECT_X_ANY_MORE_COINS"), 194, 333, -1, -1, true, 0, -1);
		}
		else
		{
			// Action and Strategy share the page; only the title differs.
			g->SetFont(mApp->mFont1Underlined);
			if (mApp->mGameType == GAMETYPE_ACTION)
				WriteString(g, mApp->GetString("X_HOW_TO_PLAY_BIG_MONEY_ACTION"), 210, 28, -1, -1, true, 0, -1);
			else
				WriteString(g, mApp->GetString("X_HOW_TO_PLAY_BIG_MONEY_STRATEGY"), 210, 28, -1, -1, true, 0, -1);
			g->DrawImage(aCircles, 527, 40, Rect(0, 0, 104, 330));
			g->SetFont(mApp->mFont1);
			g->DrawImage(anArrow, 154, 52);
			WriteString(g, mApp->GetString("X_CLICK_ON_GROUPS_OF_X_OR_MORE2"), 194, 93, -1, -1, true, 0, -1);
			WriteString(g, mApp->GetString("X_COINS_TO_COLLECT_THEM2"), 194, 113, -1, -1, true, 0, -1);
			g->DrawImage(anArrow, 154, 162);
			WriteString(g, mApp->GetString("X_COLLECTING_COINS_WILL_FILL_UP"), 194, 203, -1, -1, true, 0, -1);
			WriteString(g, mApp->GetString("X_THE_X_MONEY_METER_X_ON_THE_RIGHT"), 194, 223, -1, -1, true, 0, -1);
			g->DrawImage(anArrow, 154, 272);
			WriteString(g, mApp->GetString("X_WHEN_METER_IS_FULL_A_X_MONEYBAG"), 194, 313, -1, -1, true, 0, -1);
			WriteString(g, mApp->GetString("X_WILL_DROP__GRAB_IT_TO_ADVANCE"), 194, 333, -1, -1, true, 0, -1);
		}
	}
	// else (unreachable, nothing sets mPage): the original fetches GetStringVector("aHelpTexts") and
	// discards it without drawing anything.

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, 255));
	g->DrawImage(mApp->mImages[IMAGE_LEFTBAR], 0, 0);
	g->SetColorizeImages(false);
	g->DrawImage(mApp->mImages[IMAGE_FLOATLOGO], 15, 8);
}
