#include "GameMenu.h"

#include "Application.h"
#include "DrawUtil.h"
#include "Image.h"
#include "NcButton.h"
#include "Res.h"
#include "Screens.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The three buttons are this wide, one under the other, with a gap between
const int BUTTON_WIDTH = 0x145;
const int BUTTON_GAP = 5;
const int BUTTONS_TOP = 0x19;

// The box is this tall
const int MENU_HEIGHT = 0xf0;

// The ids of the buttons
const int BUTTON_CONTINUE = 0;
const int BUTTON_OPTIONS = 1;
const int BUTTON_MAIN_MENU = 2;

}

// 0x44f8d0
GameMenu::GameMenu() :
	NcMovingDialog(MENU_HEIGHT, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_GAME_MENU, true, "", "", "", Dialog::BUTTONS_NONE)
{
	mContinueButton = nullptr;
	mOptionsButton = nullptr;
	mMainMenuButton = nullptr;

	mContinueButton = MakeNcButton(BUTTON_CONTINUE, this, gApp->IsShopOpen() ? "Back To Shop" : "Continue Game");
	mOptionsButton = MakeNcButton(BUTTON_OPTIONS, this, "Options");
	mMainMenuButton = MakeNcButton(BUTTON_MAIN_MENU, this, "Main Menu");
	SetEscapeResult(BUTTON_CONTINUE);
}

// 0x44f690
GameMenu::~GameMenu()
{
	if (mContinueButton != nullptr)
		delete mContinueButton;
	if (mOptionsButton != nullptr)
		delete mOptionsButton;
	if (mMainMenuButton != nullptr)
		delete mMainMenuButton;
}

// 0x44f720: the buttons run down the box with "Main Menu" at the top and the way back to the game at the bottom
void GameMenu::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aButtonHeight = mContinueButton->mComponentImage->GetHeight();
	int aButtonX = (theWidth - BUTTON_WIDTH) / 2 + theX;
	int aButtonY = theY + BUTTONS_TOP;
	mMainMenuButton->Resize(aButtonX, aButtonY, BUTTON_WIDTH, aButtonHeight);
	aButtonY = aButtonHeight + aButtonY + BUTTON_GAP;
	mOptionsButton->Resize(aButtonX, aButtonY, BUTTON_WIDTH, aButtonHeight);
	aButtonY = aButtonHeight + aButtonY + BUTTON_GAP;
	mContinueButton->Resize(aButtonX, aButtonY, BUTTON_WIDTH, aButtonHeight);
}

// 0x44f7c0
void GameMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mContinueButton);
	theWidgetManager->AddWidget(mOptionsButton);
	theWidgetManager->AddWidget(mMainMenuButton);
}

// 0x44f800
void GameMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mContinueButton);
	theWidgetManager->RemoveWidget(mOptionsButton);
	theWidgetManager->RemoveWidget(mMainMenuButton);
}

// 0x44f8a0
int GameMenu::GetPreferredHeight(int theWidth)
{
	return MENU_HEIGHT;
}

// 0x44f840: the game goes on once the box has left; the other two open their boxes at once
void GameMenu::ButtonDepress(int theId)
{
	if (theId == BUTTON_CONTINUE)
		CloseWith(0);
	else if (theId == BUTTON_OPTIONS)
		MakeOptionsDialog();
	else if (theId == BUTTON_MAIN_MENU)
		gApp->DoLeaveGameDialog();
}

// 0x44f880
void GameMenu::Closed()
{
	if (mCloseResult == BUTTON_CONTINUE)
		gApp->EndGameMenu();
}

// 0x44fba0
GameMenu* Sexy::MakeGameMenu()
{
	GameMenu* aMenu = new GameMenu();
	LayoutDialog(aMenu, ToInt((double) gApp->mWidth * 0.5));
	int aWidth = aMenu->mWidth;
	int aHeight = aMenu->GetPreferredHeight(aWidth);
	aMenu->Resize((gApp->mWidth - aWidth) / 2, -aHeight, aWidth, aHeight);
	gApp->AddDialog(DIALOG_GAME_MENU, aMenu);
	return aMenu;
}
