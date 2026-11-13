#include "ReadyGoDialog.h"

#include "Application.h"
#include "ChallengeScreen.h"
#include "GameScreen.h"
#include "GnomeBonus.h"
#include "GnomeClover.h"
#include "GnomeStrawberry.h"
#include "Insets.h"
#include "Res.h"
#include "Stats.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The box is this tall (a line of lines takes more) and this wide
const int DIALOG_HEIGHT = 200;
const int DIALOG_WIDTH = 0x12c;
const int LINES_EXTRA_HEIGHT = 0x5a;

// The room around the text: left, top, right and bottom
const Insets CONTENT_INSETS(0x28, 0x23, 0x28, 0x23);

// How the power-ups from the shop start: the ice cream's freeze lasts this long, the clover's delay and the strawberry's delay and
// length are these (the clover's own length is 25)
const double ICE_CREAM_SECONDS = 10.0;
const double CLOVER_DELAY = 0.0;
const double STRAWBERRY_DELAY = 0.0;
const double STRAWBERRY_SECONDS = 25.0;

}

// 0x45ccb0
ReadyGoDialog::ReadyGoDialog(const std::string& theHeader, const std::string& theLines) :
	NcMovingDialog(DIALOG_HEIGHT, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_READY_GO, true, theHeader, theLines, "Go!",
		Dialog::BUTTONS_FOOTER)
{
	mContentInsets = CONTENT_INSETS;
}

// 0x45cab0
ReadyGoDialog::~ReadyGoDialog()
{
}

// 0x45cc80
int ReadyGoDialog::GetPreferredHeight(int theWidth)
{
	return DIALOG_HEIGHT;
}

// 0x45caf0: the box goes, the game gets the keyboard, and the power-ups bought for the level start
void ReadyGoDialog::ButtonDepress(int theId)
{
	CloseWith(theId);

	Widget* aScreen = gApp->mChallengeScreen != nullptr ? static_cast<Widget*>(gApp->mChallengeScreen) : static_cast<Widget*>(gApp->mGameScreen);
	gApp->mWidgetManager->SetFocus(aScreen);
	gApp->PlaySample(SOUND_VOICE_BONUS_GO);

	if (GetStats()->GetInt(STAT_HAS_ICE_CREAM) != 0)
	{
		GetStats()->SetInt(STAT_HAS_ICE_CREAM, 0);
		StartIceCreamPower(ICE_CREAM_SECONDS);
	}
	if (GetStats()->GetInt(STAT_HAS_CLOVER) != 0)
	{
		GetStats()->SetInt(STAT_HAS_CLOVER, 0);
		StartCloverPower(CLOVER_DELAY);
	}
	if (GetStats()->GetInt(STAT_HAS_STRAWBERRY) != 0)
	{
		GetStats()->SetInt(STAT_HAS_STRAWBERRY, 0);
		StartStrawberryPower(STRAWBERRY_DELAY, STRAWBERRY_SECONDS);
	}
}

// 0x45cc10: the game is let go (the original lets go twice), and the crosshair comes back
void ReadyGoDialog::Closed()
{
	gApp->KillDialog(this);
	gApp->PauseGame(false);
	gApp->PauseGame(false);
	if (gApp->mGameScreen != nullptr && gApp->mChallengeScreen == nullptr)
		gApp->mGameScreen->ShowCrosshair();
}

// 0x45cde0
ReadyGoDialog* Sexy::MakeReadyGoDialog(const std::string& theHeader, const std::string& theLines)
{
	ReadyGoDialog* aDialog = new ReadyGoDialog(theHeader, theLines);
	LayoutDialog(aDialog, DIALOG_WIDTH);
	int aHeight = aDialog->GetPreferredHeight(DIALOG_WIDTH);
	if (!aDialog->mDialogLines.empty())
		aHeight += LINES_EXTRA_HEIGHT;
	aDialog->mRestY = (gApp->mHeight - aHeight) / 2;
	aDialog->Resize((gApp->mWidth - DIALOG_WIDTH) / 2, -aHeight, DIALOG_WIDTH, aHeight);
	gApp->AddDialog(DIALOG_READY_GO, aDialog);
	return aDialog;
}
