// port(variant): the Extras dialog; see ExtrasDialog.h. Not part of the original game.

#include "ExtrasDialog.h"
#include "WinDMApp.h"
#include "Board.h"
#include "GameConfig.h"
#include "Res.h"

#include "DialogButton.h"
#include "EditWidget.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"
#include "platform/ModManager.h"

#include <cstdlib>

using namespace Sexy;

// The name field's colours (RecordScoreDialog.h)
static int gSeedEditColors[5][4] =
{
	{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}
};

// RecordScoreDialog::DrawEditBox's three-slice frame
static void DrawEditBox(Graphics* g, Image* theImage, const Rect& theRect)
{
	g->DrawImage(theImage, theRect.mX, theRect.mY, Rect(0, 0, 32, 32));
	g->DrawImage(theImage, theRect.mX + theRect.mWidth - 32, theRect.mY, Rect(64, 0, 32, 32));
	Graphics aMiddleG(*g);
	aMiddleG.ClipRect(theRect.mX + 32, theRect.mY, theRect.mWidth - 32, 32);
	for (int i = 0; i < (theRect.mWidth - 33) / 32; i++)
		aMiddleG.DrawImage(theImage, theRect.mX + 32 + i * 32, theRect.mY, Rect(32, 0, 32, 32));
}

static DialogButton* MakeDialogButton(WinDMApp* theApp, int theId, ButtonListener* theListener, const SexyString& theLabel)
{
	DialogButton* aButton = new DialogButton(theApp->mImages[IMAGE_BOXBUTTON], theId, theListener);
	aButton->SetFont(theApp->mMainFont);
	aButton->mLabel = theLabel;
	return aButton;
}

ExtrasDialog::ExtrasDialog(WinDMApp* theApp) :
	Dialog(theApp->mImages[IMAGE_DIALOGBOX], theApp->mImages[IMAGE_BOXBUTTON], DIALOG_EXTRAS, true,
		theApp->GetString("EXTRASDLG_EXTRAS", _S("EXTRAS")), _S(""), theApp->GetString("EXTRASDLG_DONE", _S("Done")), BUTTONS_FOOTER)
{
	mApp = theApp;
	SetButtonFont(mApp->mMainFont);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mMainFont);

	Board* aBoard = mApp->mBoard;
	mGameSeed = ((aBoard != NULL) && (aBoard->mOwnRng != NULL)) ? aBoard->mGameSeed : 0;
	mBadSeed = false;

	mSeedWidget = NULL;
	mPlaySeedButton = NULL;
	if (gGameConfig.mFeatures.mSeedEntry)
	{
		mSeedWidget = new EditWidget(SEED_EDIT_ID, this);
		mSeedWidget->SetFont(mApp->mMainFont, NULL);
		mSeedWidget->SetColors(gSeedEditColors, 5);
		mSeedWidget->mMaxChars = 10;
		if (mGameSeed != 0)
			mSeedWidget->SetText(StrFormat(_S("%u"), (unsigned) mGameSeed), true);
		mPlaySeedButton = MakeDialogButton(mApp, PLAY_SEED_BUTTON_ID, this, mApp->GetString("EXTRASDLG_PLAY_SEED", _S("Play this seed")));
	}

	mReplaysButton = NULL;
	if (gGameConfig.mFeatures.mReplays)
		mReplaysButton = MakeDialogButton(mApp, REPLAYS_BUTTON_ID, this, mApp->GetString("EXTRASDLG_REPLAYS", _S("Replays...")));

	// As Options decides its Mods button (OptionsDialog.cpp), which this one replaces
	mModsButton = NULL;
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (aMod->HasSettings())
		{
			mModsButton = MakeDialogButton(mApp, MODS_BUTTON_ID, this, mApp->GetString("EXTRASDLG_MODS", _S("Mods...")));
			break;
		}
	}
}

ExtrasDialog::~ExtrasDialog()
{
	delete mSeedWidget;
	delete mPlaySeedButton;
	delete mReplaysButton;
	delete mModsButton;
}

int ExtrasDialog::RowsHeight()
{
	return ((mSeedWidget != NULL) ? SEED_ROWS_HEIGHT : 0) + ((mGameSeed != 0) ? SEED_LINE_HEIGHT : 0) +
		((mReplaysButton != NULL) ? BUTTON_ROW_HEIGHT : 0) + ((mModsButton != NULL) ? BUTTON_ROW_HEIGHT : 0);
}

int ExtrasDialog::GetPreferredHeight(int theWidth)
{
	return TOP + RowsHeight() + BOTTOM;
}

void ExtrasDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	if (mSeedWidget != NULL)
	{
		theWidgetManager->AddWidget(mSeedWidget);
		theWidgetManager->AddWidget(mPlaySeedButton);
		theWidgetManager->SetFocus(mSeedWidget);
	}
	if (mReplaysButton != NULL)
		theWidgetManager->AddWidget(mReplaysButton);
	if (mModsButton != NULL)
		theWidgetManager->AddWidget(mModsButton);
}

void ExtrasDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	if (mSeedWidget != NULL)
	{
		theWidgetManager->RemoveWidget(mSeedWidget);
		theWidgetManager->RemoveWidget(mPlaySeedButton);
	}
	if (mReplaysButton != NULL)
		theWidgetManager->RemoveWidget(mReplaysButton);
	if (mModsButton != NULL)
		theWidgetManager->RemoveWidget(mModsButton);
}

void ExtrasDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	int aY = mY + TOP;
	if (mSeedWidget != NULL)
	{
		// The field's frame is the widget inflated by 4 (RecordScoreDialog::Draw)
		mSeedWidget->Resize(mX + 32, aY + 30, 136, 24);
		mPlaySeedButton->Resize(mX + 184, aY + 26, mWidth - 208, 32);
		aY += SEED_ROWS_HEIGHT;
	}
	if (mGameSeed != 0)
		aY += SEED_LINE_HEIGHT;
	if (mReplaysButton != NULL)
	{
		mReplaysButton->Resize(mX + 24, aY, mWidth - 48, 32);
		aY += BUTTON_ROW_HEIGHT;
	}
	if (mModsButton != NULL)
		mModsButton->Resize(mX + 24, aY, mWidth - 48, 32);
}

void ExtrasDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	g->SetFont(mApp->mMainFont);
	int aY = TOP;
	if (mSeedWidget != NULL)
	{
		g->SetColor(Color(255, 255, 0));
		g->DrawString(mApp->GetString("EXTRASDLG_SEED_LABEL", _S("Seed for the next game:")), 32, aY + 16);
		Rect aRect = mSeedWidget->GetRect();
		aRect.mX -= mX;
		aRect.mY -= mY;
		aRect.Inflate(4, 4);
		DrawEditBox(g, mApp->mImages[IMAGE_EDITBOX], aRect);
		if (mBadSeed)
		{
			g->SetColor(Color(255, 80, 80));
			g->DrawString(mApp->GetString("EXTRASDLG_BAD_SEED", _S("A seed is a number from 1 to 4294967295.")), 32, aY + 76);
		}
		aY += SEED_ROWS_HEIGHT;
	}
	if (mGameSeed != 0)
	{
		// The game-over screen's SEED colour (HighScoreWidget.cpp)
		g->SetColor(Color(255, 100, 220));
		SexyString aLine = mApp->GetString("EXTRASDLG_THIS_GAME", _S("This game: ")) + mApp->GetString("HSWIDGET_SEED", _S("SEED")) +
			StrFormat(_S(" %u"), (unsigned) mGameSeed);
		g->DrawString(aLine, (mWidth - mApp->mMainFont->StringWidth(aLine)) / 2, aY + 16);
	}
}

void ExtrasDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);
	if (theId == PLAY_SEED_BUTTON_ID)
		PlaySeed();
	else if (theId == REPLAYS_BUTTON_ID)
		mApp->ShowReplayBrowser();
	else if (theId == MODS_BUTTON_ID)
		mApp->OpenModMenu(); // over this dialog, which stays open
}

// Digits only; 1..4294967295 (no leading zero is needed: "007" is 7)
void ExtrasDialog::PlaySeed()
{
	const SexyString& aText = mSeedWidget->mString;
	unsigned long long aSeed = 0;
	bool isSeed = !aText.empty() && (aText.size() <= 10);
	for (size_t i = 0; isSeed && (i < aText.size()); i++)
	{
		isSeed = (aText[i] >= '0') && (aText[i] <= '9');
		aSeed = aSeed * 10 + (unsigned long long) (aText[i] - '0');
	}
	isSeed = isSeed && (aSeed >= 1) && (aSeed <= 0xffffffffULL);
	if (isSeed != !mBadSeed)
	{
		mBadSeed = !isSeed;
		MarkDirty();
	}
	if (isSeed)
		mApp->PlaySeed((uint32_t) aSeed);
}

void ExtrasDialog::EditWidgetText(int theId, const SexyString& theString)
{
	PlaySeed();
}

bool ExtrasDialog::AllowChar(int theId, SexyChar theChar)
{
	return (theChar >= '0') && (theChar <= '9');
}
