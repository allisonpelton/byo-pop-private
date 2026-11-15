#include "StatusBar.h"
#include "Board.h"
#include "BracketButton.h"
#include "Compat2004.h"
#include "DDImage.h"
#include "GameText.h"
#include "Graphics.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "QuitDialog.h"
#include "Res.h"
#include "Sprite.h"
#include "WidgetManager.h"

#include <stdio.h>

using namespace Sexy;

namespace
{
	const int BAR_HEIGHT = 30;
	const int MENU_BUTTON_X = 535;
	const int MENU_BUTTON_WIDTH = 100;
	const int RESERVE_TANKS_SHOWN = 2;			// at most two reserve tanks are drawn
	const int SURVIVAL_TICKS_PER_SECOND = 200;	// survival's clock counts 2 an update
	const double CHARGE_BAR_SCALE = 0.029333333333333333; // the charge bar's pixels for each update of charge
	const int CHARGE_BAR_HEIGHT = 18;
	const double PROGRESS_WIDTH = 119.0;		// the level progress marker's travel
	const double PROGRESS_X = 205.0;
}

// 0x42e120
StatusBar::StatusBar(HeavyWeaponApp* theApp)
{
	mApp = theApp;
	mQuitDialog = NULL;
	mMenuButton = new BracketButton(mApp, this, BUTTON_MENU);
	mMenuButton->SetTint(Color(0x20, 0x80, 0x40, 0xff));
	mMenuButton->mLabel = GT(0x52fad8);
	Widget::Resize(0, 0, 640, BAR_HEIGHT);
	mMenuButton->Resize(MENU_BUTTON_X, mY, MENU_BUTTON_WIDTH, BAR_HEIGHT);
}

// 0x42df10
StatusBar::~StatusBar()
{
	delete mMenuButton;
}

// 0x42dfa0
void StatusBar::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mBoard = mApp->mBoard;
	theWidgetManager->AddWidget(mMenuButton);
}

// 0x42df70
void StatusBar::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuButton);
}

// 0x42e010: the MENU button moves with the bar
void StatusBar::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mMenuButton->Resize(MENU_BUTTON_X, mY, MENU_BUTTON_WIDTH, BAR_HEIGHT);
}

// 0x42dfd0: once a campaign level's time is past its end, the bar slides up a pixel an update
void StatusBar::Update()
{
	Widget::Update();
	Board* aBoard = mBoard;
	if (aBoard->mLevelLength < aBoard->mLevelTime && !aBoard->mIsSurvival)
		Move(0, aBoard->mLevelLength - aBoard->mLevelTime);
}

// 0x42e050: MENU stops the game under the quit menu
void StatusBar::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	mQuitDialog = new QuitDialog(mApp);
	mApp->AddDialog(HeavyWeaponApp::DIALOG_QUIT, mQuitDialog);
	mApp->SetCursorImage(0, mApp->mCustomCursorsEnabled ? mApp->mSprites[IMAGE_CURSOR_POINTER]->mImage : NULL);
}

// 0x42e220
void StatusBar::Draw(Graphics* g)
{
	mApp->mSprites[IMAGE_STATUSBAR]->Draw(g, 0, 0, false, 0, 0, false);
	g->SetFont(mApp->mFonts[FONT_COMPUTER]);
	g->SetColor(Color(0, 255, 0));
	char aText[32];
	if (!mBoard->mIsSurvival)
	{
		sprintf(aText, "%i", mApp->mScore);
		int aReserveTanks = mApp->mLives;
		if (aReserveTanks > RESERVE_TANKS_SHOWN)
			aReserveTanks = RESERVE_TANKS_SHOWN;
		int x = 117;
		for (int i = 0; i < aReserveTanks; i++)
		{
			mApp->mSprites[IMAGE_TANKICON]->Draw(g, x, 7, false, 0, 0, false);
			x += 22;
		}
	}
	else
	{
		mApp->mSprites[IMAGE_STATUSCOVERS]->Draw(g, 108, 2, false, 0, 0, false);
		int aTime = mBoard->mLevelTime - mBoard->mTimePenalty;
		sprintf(aText, "%i:%02i:%02i", aTime / (SURVIVAL_TICKS_PER_SECOND * 3600),
			aTime / (SURVIVAL_TICKS_PER_SECOND * 60) % 60, aTime / SURVIVAL_TICKS_PER_SECOND % 60);
	}
	g->DrawString(aText, 10, 20);

	g->SetColorizeImages(true);
	int aCharge = mBoard->mMegaLaserTime;
	if (aCharge == 0)
	{
		g->SetColor(Color(0, 255, 0));
		int x = 438;
		for (int i = 0; i < mBoard->mMegaLaserPartsCaught; i++)
		{
			mApp->mSprites[IMAGE_MEGALASER]->Draw(g, x, 7, false, i, 0, false);
			x += 20;
		}
	}
	else
	{
		// the charge bar, as wide as the charge left, pulses yellow to red and back
		int aWidth = (int)((double)aCharge * CHARGE_BAR_SCALE);
		int aPulse = aCharge * 20 % 512;
		int aGreen = aPulse > 255 ? aPulse % 256 : 255 - aPulse % 256;
		g->SetColor(Color(255, aGreen, 0));
		g->DrawImage(mApp->mSprites[IMAGE_MEGAMETER]->mImage, 436, 6, Rect(0, 0, aWidth, CHARGE_BAR_HEIGHT));
	}
	g->SetColorizeImages(false);
	g->SetColor(Color(0, 255, 0));

	if (!mBoard->mIsSurvival)
	{
		g->Translate(65, 0);
		sprintf(aText, "M%i", mApp->mLevel + 1);
		WriteCenteredLine(g, 20, aText);
		g->Translate(-65, 0);
		mApp->mSprites[IMAGE_LEVEL_MAP]->Draw(g, 196, 6, false, 0, 0, false);
		int aProgressX = (int)(PROGRESS_WIDTH / (double)mBoard->mLevelLength * (double)mBoard->mLevelTime + PROGRESS_X);
		mApp->mSprites[IMAGE_MAPPOINTER]->Draw(g, aProgressX, 11, true, 0, 0, false);
	}
	else
		WriteString2004(g, GT(0x52c828), 209, 20, -1, -1, true, 0, -1);
}
