// The help screen (see HelpWidget.h).

#include "HelpWidget.h"
#include "AlchemyApp.h"
#include "Board.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "WidgetManager.h"
#include "Compat2004.h"

using namespace Sexy;

// 0x408a8a
HelpWidget::HelpWidget(AlchemyApp* theApp)
{
	mApp = theApp;

	mTutorialLink = new HyperlinkWidget(0, this);
	mTutorialLink->mLabel = mApp->GetString("TUTORIAL_LINK");
	mTutorialLink->SetFont(mApp->mFont1);
	mTutorialLink->Resize(0xac, 0x1a4, 0xf0, 0x14);
	mTutorialLink->mColor = Color(0, 0xff, 0);
	mTutorialLink->mOverColor = Color(0xff, 0xff, 0xff);
	mTutorialLink->mDoFinger = true;

	mReturnLink = new HyperlinkWidget(1, this);
	mReturnLink->mLabel = mApp->GetString(mApp->mBoard == NULL ? "RETURN_TO_GAME_SELECTION_LINK" : "RETURN_TO_GAME_LINK");
	mReturnLink->SetFont(mApp->mFont1);
	mReturnLink->Resize(0x1a6, 0x1a4, 200, 0x14);
	mReturnLink->mColor = Color(0, 0xff, 0);
	mReturnLink->mOverColor = Color(0xff, 0xff, 0xff);
	mReturnLink->mDoFinger = true;
}

// 0x408a6e
HelpWidget::~HelpWidget()
{
	delete mTutorialLink;
	delete mReturnLink;
}

// 0x4089fa
void HelpWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mTutorialLink);
	theWidgetManager->AddWidget(mReturnLink);
}

// 0x408a21
void HelpWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mTutorialLink);
	theWidgetManager->RemoveWidget(mReturnLink);
}

// 0x408a4d
void HelpWidget::ButtonDepress(int theId)
{
	if (theId == 0)
		mApp->ConfirmTutorial();
	else if (theId == 1)
		mApp->HideHelp();
}

// 0x408cd7: three boxes of three symbols each, in colours, under the labels RIGHT, RIGHT and WRONG
void HelpWidget::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(0x96, 0, 0x1ea, 0x1e0));
	g->SetColor(Color(0, 0, 0));
	g->FillRect(10, 0x27, 0x1d4, 0x1a0);
	g->SetColor(Color(0xff, 0xff, 0));
	g->SetFont(mApp->mFont1);
	WriteCenteredLine(g, 0x40, mApp->GetString("HOW_TO_PLAY"));
	WriteWordWrapped2004(g, Rect(0x1e, 0x4c, mWidth - 0x3c, -1), mApp->GetString("HOW_TO_PLAY2"), 0xf, 0);

	g->SetColorizeImages(true);

	// the first box
	g->SetColor(Color(0x5c, 0x5c, 0x5c));
	g->DrawRect(0x12, 0x7a, 0x81, 0x41);
	g->SetColor(Color(0, 0xff, 0));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0x16, 0x7b, Rect(0, 0, 0x32, 0x32));
	g->SetColor(Color(0, 0xff, 0));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0x3f, 0x7b, Rect(0x32, 0, 0x32, 0x32));
	g->SetColor(Color(0xff, 0x20, 0x20));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 100, 0x7b, Rect(0x32, 0, 0x32, 0x32));
	g->SetColor(Color(0, 0xff, 0));
	SexyString aRight = mApp->GetString("RIGHT");
	g->DrawString(aRight, (0x81 - g->GetFont()->StringWidth(aRight)) / 2 + 0x12, 0xb6);

	// the second box
	g->SetColor(Color(0x5c, 0x5c, 0x5c));
	g->DrawRect(0xae, 0x7a, 0x81, 0x41);
	g->SetColor(Color(0x40, 0x40, 0xff));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0xb0, 0x7b, Rect(0, 0, 0x32, 0x32));
	g->SetColor(Color(0x40, 0x40, 0xff));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0xd7, 0x7b, Rect(100, 0, 0x32, 0x32));
	g->SetColor(Color(0xff, 0, 0xff));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0xfe, 0x7b, Rect(100, 0, 0x32, 0x32));
	g->SetColor(Color(0, 0xff, 0));
	aRight = mApp->GetString("RIGHT");
	g->DrawString(aRight, (0x81 - g->GetFont()->StringWidth(aRight)) / 2 + 0xae, 0xb6);

	// the third box
	g->SetColor(Color(0x5c, 0x5c, 0x5c));
	g->DrawRect(0x14b, 0x7a, 0x81, 0x41);
	g->SetColor(Color(0xff, 0x20, 0x20));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0x14c, 0x7b, Rect(0x32, 0, 0x32, 0x32));
	g->SetColor(Color(0x40, 0x40, 0xff));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0x170, 0x7b, Rect(100, 0, 0x32, 0x32));
	g->SetColor(Color(0, 0xff, 0));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0x198, 0x7b, Rect(0, 0, 0x32, 0x32));
	g->SetColor(Color(0xff, 0, 0));
	SexyString aWrong = mApp->GetString("WRONG");
	g->DrawString(aWrong, (0x81 - g->GetFont()->StringWidth(aWrong)) / 2 + 0x14b, 0xb6);

	g->SetColorizeImages(false);

	// the blank block and the skull
	g->SetColor(Color(0xff, 0xff, 0));
	g->DrawImage(mApp->mImages[IMAGE_NULLBLOCK], 0x20, 0x11a);
	WriteWordWrapped2004(g, Rect(0x5d, 0x11d, 0x92, -1), mApp->GetString("STONES_WILD"), 0xf, -1);
	g->DrawImage(mApp->mImages[IMAGE_SKULLS], 0xfe, 0x11a, Rect(0, 0, 0x32, 0x32));
	WriteWordWrapped2004(g, Rect(0x13b, 0x11d, 0x92, -1), mApp->GetString("SKULLS_MAGIC"), 0xf, -1);

	WriteWordWrapped2004(g, Rect(0x1e, 200, mWidth - 0x3c, -1), mApp->GetString("ADJACENT_MUST_MATCH_COLOR_OR_SHAPE"), 0xf, 0);
	WriteWordWrapped2004(g, Rect(0x1e, 0xf0, mWidth - 0x3c, -1), mApp->GetString("COMPLETING_ROWS_OR_COLUMNS_RELEASES_THAT_SPACE"), 0xf, 0);
	WriteWordWrapped2004(g, Rect(0x28, 0x15c, mWidth - 0x50, -1), mApp->GetString("USING_FORGE_IS_LIMITED"), 0xf, 0);
}
