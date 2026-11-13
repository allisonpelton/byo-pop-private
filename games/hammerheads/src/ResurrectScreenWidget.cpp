#include "ResurrectScreenWidget.h"

#include "Application.h"
#include "Color.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "NcButton.h"
#include "ResurrectButton.h"
#include "Res.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The pictures of the points' buttons: the exe takes them from its table of pictures from IMAGE_PHOTO01 on, so a sixth point (a
// game that has reached the fifth bonus level) has the high score background, and beyond it the wipe pictures
Image** const POINT_PICTURES[] =
{
	&IMAGE_PHOTO01, &IMAGE_PHOTO02, &IMAGE_PHOTO03, &IMAGE_PHOTO04, &IMAGE_PHOTO05, &IMAGE_HIGHSCOREBACKGROUND, &IMAGE_ZOOMWIPEA_HAMMER,
	&IMAGE_ZOOMWIPEB_HAMMER, &IMAGE_ZOOMWIPEA_TROPHY, &IMAGE_ZOOMWIPEB_TROPHY
};

// The photo of a button is three cels wide (the up, over and down looks)
const int PICTURE_CELS = 3;

// Where the numbers of a restart point are in its saved statistics (the order of Stats::GetSaved)
const int SAVED_COIN_BALANCE = 0;
const int SAVED_HEALTH = 1;
const int SAVED_HEALTH_CAPACITY = 2;
const int SAVED_HAMMER_LEVEL = 3;
const int SAVED_HAMMER_ON_FIRE = 4;

// The buttons are three to a row, in at most two rows. The gap between the buttons of a row is what the width has left after three
// buttons and the margin; up to three buttons are in the middle of the window's height, and more start this far from the top, with
// the second row a row spacing lower; all of them are this much above that line
const int BUTTONS_PER_ROW = 3;
const int ROWS = 2;
const int SIDE_MARGIN = 100;
const int TOP_OF_TWO_ROWS = 0x4b;
const int BUTTON_RAISE = 0x1a;
const int ROW_SPACING = 0xfa;

// The title
const int TITLE_Y = 0x32;

// The first valid id of a button that starts a game, and the one after the last
const int ID_END = 0xc;

// The wipe that ends the page: the game starts at the chosen point once the screen is covered (0x45d400)
void WipeToRestartPoint()
{
	gApp->ResurrectDone();
}

}

// 0x45d800: a button for each restart point of the mode being played
ResurrectScreenWidget::ResurrectScreenWidget()
{
	User* aUser = gApp->mUserManager->GetCurrentUser();
	int aCount = aUser->GetRestartPointCount(gApp->mGameMode);
	for (int i = 0; i < aCount; i++)
	{
		Image* aPicture = *POINT_PICTURES[i];
		ResurrectButton* aButton = new ResurrectButton(aPicture, BUTTON_FIRST_POINT + i, this);
		aButton->mSound = SOUND_UI_BUTTON_CLICK;
		aButton->Resize(aButton->mX, aButton->mY, aPicture->mWidth / PICTURE_CELS, aPicture->mHeight);
		SetupButtonCels(aButton, PICTURE_CELS);
		aButton->mButtonImage = aPicture;
		aButton->mComponentImage = nullptr;

		const RestartPoint* aPoint = aUser->GetRestartPoint(i);
		aButton->SetShown(aPoint->mLevel + 1, aPoint->mStats[SAVED_HEALTH], ToInt(aPoint->mStats[SAVED_HEALTH_CAPACITY]),
			ToInt(aPoint->mStats[SAVED_COIN_BALANCE]), ToInt(aPoint->mStats[SAVED_HAMMER_LEVEL]),
			aPoint->mStats[SAVED_HAMMER_ON_FIRE] != 0.0);
		mPointButtons.push_back(aButton);
	}
}

// 0x45d480
ResurrectScreenWidget::~ResurrectScreenWidget()
{
	for (int i = 0; i < (int)mPointButtons.size(); i++)
		delete mPointButtons[i];
}

// 0x45d700
void ResurrectScreenWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	for (int i = 0; i < (int)mPointButtons.size(); i++)
		theWidgetManager->AddWidget(mPointButtons[i]);
}

// 0x45d770
void ResurrectScreenWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < (int)mPointButtons.size(); i++)
		theWidgetManager->RemoveWidget(mPointButtons[i]);
}

// 0x45d540: the buttons go three to a row in the middle of the window, a second row of the rest under the first
void ResurrectScreenWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	int aCellWidth = IMAGE_PHOTO01->mWidth / PICTURE_CELS;
	int aCount = (int)mPointButtons.size();
	bool aOneRow = aCount <= BUTTONS_PER_ROW;
	int aGap = (gApp->mWidth - aCellWidth * BUTTONS_PER_ROW - SIDE_MARGIN) / 2;
	int aTop = aOneRow ? (gApp->mHeight - IMAGE_PHOTO01->mHeight) / 2 : TOP_OF_TWO_ROWS;
	int anIndex = 0;
	for (int aRow = 0; aRow < ROWS; aRow++)
	{
		for (int aColumn = 0; aColumn < BUTTONS_PER_ROW; aColumn++)
		{
			if (aCount <= anIndex)
				break;

			int aRowCount;
			if (aRow == 0)
				aRowCount = aOneRow ? aCount : BUTTONS_PER_ROW;
			else
				aRowCount = aCount - BUTTONS_PER_ROW;

			mPointButtons[anIndex]->Move(
				(aGap + aCellWidth) * aColumn + ((gApp->mWidth - (aRowCount - 1) * aGap) - aRowCount * aCellWidth) / 2,
				aTop - BUTTON_RAISE + aRow * ROW_SPACING);
			anIndex++;
		}
	}
}

// 0x45dae0
void ResurrectScreenWidget::Draw(Graphics* g)
{
	Graphics aG(*g);
	aG.DrawImage(IMAGE_RESURRECTSCREENWIDGET, 0, 0);
	aG.SetFont(FONT_PTBANANASPLIT27);
	aG.SetColor(Color(255, 255, 255, 255));
	WriteCenteredLine(&aG, TITLE_Y, "Choose Your Starting Level");

	if (gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
		aG.DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->mWidth, mHeight - IMAGE_TOUGH_COOKIE_STAMP->mHeight);
}

// 0x45d410: a button of a point starts the game there, behind a wipe
void ResurrectScreenWidget::ButtonDepress(int theId)
{
	if (BUTTON_FIRST_POINT <= theId && theId < ID_END)
	{
		gApp->mStartLevel = theId - BUTTON_FIRST_POINT;
		gApp->DoZoomWipe(WipeToRestartPoint, WIPE_HAMMER);
	}
}
