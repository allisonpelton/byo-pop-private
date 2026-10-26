// The dialogs of Rocket Mania Deluxe (see Dialogs.h).

#include "Dialogs.h"
#include "Board.h"
#include "RocketApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cstdio>

using namespace Sexy;

// the game's dialog buttons: the Showcard font, the label of the given text
static DialogButton* MakeDialogButton(RocketApp* theApp, int theId, ButtonListener* theListener, const SexyString& theLabel)
{
	DialogButton* aButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTON], theId, theListener);
	aButton->SetFont(theApp->mFonts[FONT_SHOWCARD22]);
	aButton->mLabel = theLabel;
	return aButton;
}

// ---------------------------------------------------------------------------------------------------------
// DifficultyDialog

// 0x404380
DifficultyDialog::DifficultyDialog(RocketApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_DIFFICULTY, true,
		GT(0x004e44b8), "", "", BUTTONS_NONE),
	mApp(theApp)
{
	SetButtonFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	SetHeaderFont(mApp->mFonts[FONT_SHOWCARD22]);
	SetLinesFont(mApp->mFonts[FONT_VERDANA12_BOLD]);

	mButtons[0] = MakeDialogButton(mApp, 0, this, RocketApp::DifficultyNameUpper(1));
	mButtons[1] = MakeDialogButton(mApp, 1, this, RocketApp::DifficultyNameUpper(2));
	mButtons[2] = MakeDialogButton(mApp, 2, this, RocketApp::DifficultyNameUpper(3));
	mButtons[3] = MakeDialogButton(mApp, 3, this, GT(0x004e44b0));
}

DifficultyDialog::~DifficultyDialog()
{
	for (int i = 0; i < 4; i++)
		delete mButtons[i];
}

// 0x404960
void DifficultyDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	for (int i = 0; i < 4; i++)
		theWidgetManager->AddWidget(mButtons[i]);
}

void DifficultyDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 4; i++)
		theWidgetManager->RemoveWidget(mButtons[i]);
}

// 0x404a00: colour 3 is the buttons' label colour
void DifficultyDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		for (int i = 0; i < 4; i++)
			if (mButtons[i] != NULL)
				mButtons[i]->SetColor(ButtonWidget::COLOR_LABEL, theColor);
	}
}

// 0x404a70: the buttons hang from the bottom
void DifficultyDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aWidth = theWidth - 0x40;
	mButtons[0]->Resize(mX + 0x20, mY + theHeight - 300, aWidth, 0x32);
	mButtons[1]->Resize(mX + 0x20, mY + theHeight - 0xf0, aWidth, 0x32);
	mButtons[2]->Resize(mX + 0x20, mY + theHeight - 0xb4, aWidth, 0x32);
	mButtons[3]->Resize(mX + 0x20, mY + theHeight - 0x55, aWidth, 0x28);
}

// 0x404b20: the picture and the title (the dialog's own header text is not drawn)
void DifficultyDialog::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mComponentImage);
	g->SetFont(mApp->mFonts[FONT_SHOWCARD22]);
	g->SetColor(mApp->mDialogColors[0]);
	const SexyString& aTitle = GT(0x004e44c4);
	g->DrawString(aTitle, mWidth / 2 - g->GetFont()->StringWidth(aTitle) / 2, 0x41);
}

// 0x404cd0: every button but the dialog's own plays the click
void DifficultyDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + 2000) && (theId != mId + 3000))
		mApp->ButtonPress(0);
}

// 0x404d10: EASY, MEDIUM and HARD start the game, CANCEL goes back to the menu
void DifficultyDialog::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
	case 1:
	case 2:
		mApp->mGameSelector->mDifficulty = theId + 1;
		break;
	case 3:
		mApp->KillDialog(DIALOG_DIFFICULTY);
		mApp->mGameSelector->ResetHead();
		return;
	default:
		Dialog2004::ButtonDepress(theId);
		return;
	}
	mApp->KillDialog(DIALOG_DIFFICULTY);
	mApp->mGameSelector->StartGame(false);
}

// ---------------------------------------------------------------------------------------------------------
// ContinueDialog

// 0x402630
ContinueDialog::ContinueDialog(RocketApp* theApp, int theMode) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_CONTINUE, true,
		GT(0x004e4250), "", "", BUTTONS_NONE),
	mApp(theApp),
	mMode(theMode),
	mSavedDifficulty(0),
	mSavedScore(0),
	mSavedLevel(0)
{
	SetButtonFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	SetHeaderFont(mApp->mFonts[FONT_SHOWCARD22]);
	SetLinesFont(mApp->mFonts[FONT_VERDANA12_BOLD]);

	mNewButton = MakeDialogButton(mApp, 0, this, GT(0x004e4244));
	mContinueButton = MakeDialogButton(mApp, 1, this, GT(0x004e4230));

	mApp->ReadSavedGame(mMode, &mSavedDifficulty, &mSavedScore, &mSavedLevel);
}

ContinueDialog::~ContinueDialog()
{
	delete mNewButton;
	delete mContinueButton;
}

// 0x402aa0
void ContinueDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNewButton);
	theWidgetManager->AddWidget(mContinueButton);
}

void ContinueDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNewButton);
	theWidgetManager->RemoveWidget(mContinueButton);
}

// 0x402a50
void ContinueDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		if (mNewButton != NULL)
			mNewButton->SetColor(ButtonWidget::COLOR_LABEL, theColor);
		if (mContinueButton != NULL)
			mContinueButton->SetColor(ButtonWidget::COLOR_LABEL, theColor);
	}
}

// 0x402b20
void ContinueDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNewButton->Resize(mX + 0x20, mY + theHeight - 0x96, theWidth - 0x40, 0x32);
	mContinueButton->Resize(mX + 0x20, mY + theHeight - 0x5a, theWidth - 0x40, 0x32);
}

// 0x402ba0: the title, the four lines of the question and what the saved game holds
void ContinueDialog::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mComponentImage);

	g->SetFont(mApp->mFonts[FONT_SHOWCARD22]);
	g->SetColor(mApp->mDialogColors[0]);
	const SexyString& aTitle = GT(0x004e4320);
	g->DrawString(aTitle, mWidth / 2 - g->GetFont()->StringWidth(aTitle) / 2, 0x41);

	g->SetColor(mApp->mDialogColors[1]);
	g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
	static const struct { unsigned int mText; int mY; } aLines[4] =
	{
		{0x004e42fc, 0x7d}, {0x004e42e0, 0x9b}, {0x004e42c8, 0xb9}, {0x004e42a0, 0xeb}
	};
	for (int i = 0; i < 4; i++)
	{
		const SexyString& aLine = GT(aLines[i].mText);
		g->DrawString(aLine, mWidth / 2 - g->GetFont()->StringWidth(aLine) / 2, aLines[i].mY);
	}

	// what the saved game holds: strategy has no levels
	g->SetFont(mApp->mFonts[FONT_ARIAL10]);
	char aBuf[256];
	const char* aDifficulty = RocketApp::DifficultyName(mSavedDifficulty).c_str();
	if (mMode == 2)
		snprintf(aBuf, sizeof(aBuf), GTC(0x004e4284), mSavedScore, aDifficulty);
	else
		snprintf(aBuf, sizeof(aBuf), GTC(0x004e425c), mSavedLevel + 1, mSavedScore, aDifficulty);
	SexyString aSummary = aBuf;
	g->DrawString(aSummary, mWidth / 2 - g->GetFont()->StringWidth(aSummary) / 2, mHeight - 0x14);
}

// 0x403340
void ContinueDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if ((theId != mId + 2000) && (theId != mId + 3000))
		mApp->ButtonPress(0);
}

// 0x403380: NEW GAME asks for a difficulty, CONTINUE SAVED GAME loads the game
void ContinueDialog::ButtonDepress(int theId)
{
	if (theId == 0)
	{
		mApp->KillDialog(DIALOG_CONTINUE);
		mApp->mGameSelector->ShowDifficultyDialog();
	}
	else if (theId == 1)
	{
		// (the original opens the register-to-load dialog here for an unregistered game)
		// 0x403570: the saved game's mode and difficulty are the ones the game starts with
		mApp->mGameSelector->mMode = mMode;
		mApp->mGameSelector->mDifficulty = mSavedDifficulty;
		mApp->KillDialog(DIALOG_CONTINUE);
		mApp->mGameSelector->ResetHead();
		mApp->mGameSelector->StartGame(true);
		// the board that was just made takes the saved game, which is then deleted
		if (mApp->mBoard != NULL)
			mApp->mBoard->Load();
		mApp->DeleteSavedGame(mMode);
	}
	else
		Dialog2004::ButtonDepress(theId);
}
