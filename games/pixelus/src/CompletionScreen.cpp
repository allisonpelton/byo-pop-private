#include "CompletionScreen.h"
#include "PixelusApp.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "WidgetManager.h"

using namespace Sexy;

CompletionScreen::CompletionScreen(PixelusApp* theApp, int theMode)
{
	mApp = theApp;
	mMode = theMode;
	mProgress = 0;
	mContinueButton = new SoundButton(mApp, mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], ID_CONTINUE, this);
	mContinueButton->mLabel = GT(0x50c9a0);
	StyleButton(mApp, mContinueButton, 3);
}

CompletionScreen::~CompletionScreen()
{
	delete mContinueButton;
}

void CompletionScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mContinueButton);
}

void CompletionScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mContinueButton);
}

void CompletionScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aHeight = mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS]->GetHeight();
	mContinueButton->Resize(theX + 0x19b, theY + theHeight - aHeight + 1, 0x96, aHeight);
}

void CompletionScreen::Update()
{
	Widget::Update();
	if (mProgress < 1.0)
		mProgress += 0.05;
	MarkDirty();
}

void CompletionScreen::Draw(Graphics* g)
{
	bool aGame = mMode == 2 || mMode == 4;
	g->DrawImage(mApp->mImages[aGame ? IMAGE_GAMEDONE : IMAGE_HAPPYENDINGBACKDROP], 0, 0);
	Font* aFont = mApp->mFonts[FONT_PRAITOR36];
	g->SetFont(aFont);
	g->SetColor(Color(255, 255, 255, 255));
	std::string aHeader = mMode == 1 || mMode == 3 ? "YOU COMPLETED THE TEMPLE!" : aGame ? "YOU COMPLETED THE GAME!" : "YOU COMPLETED THE TUTORIAL!";
	g->DrawString(aHeader, (mWidth - aFont->StringWidth(aHeader)) / 2, 0x32);
}

// 0x40b760: for the tutorial (mode 5) the way on is the story, as for a puzzle that opens a temple
void CompletionScreen::ButtonDepress(int theId)
{
	if (theId == ID_CONTINUE)
		mApp->ContinueAfterCompletion();
}
