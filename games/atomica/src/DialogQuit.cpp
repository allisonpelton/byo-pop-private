// The quit box (see Dialogs.h).

#include "Dialogs.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Grid.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x417b90 / 0x405f90
QuitDialog::QuitDialog(AtomicaApp* theApp) :
	GameDialog(theApp, false),
	mQuitAfter(false)
{
	mSaveQuit = AddButton(0);
	mCancel = AddButton(2);
	mQuit = AddButton(1);
	mApp->mPauseCount++;
}

// 0x417d30: the game ends when the box has gone, if the player chose to quit
QuitDialog::~QuitDialog()
{
	mApp->mQuitDialog = NULL;
	mApp->mPauseCount--;
	if (mApp->mPauseCount < 0)
		mApp->mPauseCount = 0;
	if (mQuitAfter)
		mApp->ShowMainMenu(false);
}

// 0x417db0
void QuitDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	mSaveQuit->Resize(mX + 0x5a, mY + 0x10e, 0xe8, 0x1b);
	mQuit->Resize(mX + 0x5a, mY + 0x12c, 0xe8, 0x1b);
	mCancel->Resize(mX + 0x60, mY + 0x154, 0xdc, 0x1b);
}

// 0x417e40: cancel closes the box; "save and quit" saves the game first (the port is always the registered game)
void QuitDialog::ButtonDepress(int theId)
{
	if (theId == 2)
	{
		mClosing = true;
		return;
	}
	if (theId == 0)
	{
		mClosing = true;
		mQuitAfter = true;
		mApp->SaveGame();
	}
	else if (theId == 1)
	{
		mClosing = true;
		mQuitAfter = true;
	}
}

// 0x417e90
void QuitDialog::ButtonPress(int theId)
{
	(void) theId;
	mApp->mSndCapClick->Play();
}

// 0x417ef0
void QuitDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	Font* aFont = mApp->mMonitorFont;
	Font* aSmall = mApp->mMonitorFontSmall;
	int aCenter = mWidth / 2 - 10;
	g->SetFont(aFont);
	g->SetColor(Color(0, 255, 0, 255));
	const std::string& aTitle = GT(0x4afb40);
	g->DrawString(aTitle, aCenter - aFont->StringWidth(aTitle) / 2, 0x9b);
	g->SetColor(Color(0, 255, 0, 0xaf));
	const std::string& aLine1 = GT(0x4afb28);
	g->DrawString(aLine1, aCenter - aFont->StringWidth(aLine1) / 2, 0xb4);
	int aY = aFont->GetHeight() + 0xb1;
	const std::string& aLine2 = GT(0x4afb14);
	g->DrawString(aLine2, aCenter - aFont->StringWidth(aLine2) / 2, aY);
	g->SetFont(aSmall);
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mGrid != NULL))
	{
		// the game in progress is kept
		static const unsigned int kLines[3] = {0x4afafc, 0x4afae0, 0x4afac8};
		int aLineY = aY + aFont->GetHeight();
		for (int i = 0; i < 3; i++)
		{
			const std::string& aLine = GT(kLines[i]);
			g->DrawString(aLine, aCenter - aSmall->StringWidth(aLine) / 2, aLineY);
			aLineY += aSmall->GetHeight() - 3;
		}
	}
	g->SetFont(aFont);
	ButtonWidget* aButtons[3] = {mSaveQuit, mQuit, mCancel};
	unsigned int aTexts[3] = {0x4afa68, 0x4afa50, 0x4afa3c};
	for (int i = 0; i < 3; i++)
	{
		ButtonWidget* aButton = aButtons[i];
		// 0x417ef0: the third button, "no", is the yellow one
		int aRed = (i == 2) ? 255 : 0;
		int aBlue = (i == 2) ? 100 : 0;
		g->SetColor(Color(aRed, 255, aBlue, 0x80));
		if (aButton->mIsOver)
			g->SetColor(Color(aRed, 255, aBlue, 255));
		if (aButton->mIsDown)
			g->SetColor(Color(aRed, 255, aBlue, 0xaf));
		g->FillRect(aButton->mX - mX, aButton->mY - mY, aButton->mWidth, aButton->mHeight);
		g->SetColor(Color(0, 0x32, 0, 255));
		const std::string& aText = GT(aTexts[i]);
		g->DrawString(aText, ((aButton->mWidth / 2 + aButton->mX) - aFont->StringWidth(aText) / 2) - mX,
			(aButton->mY - mY) + 0x14);
	}
}
