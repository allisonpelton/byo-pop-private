#include "TrophyRoomWidget.h"

#include <cmath>
#include <string>

#include "Application.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "NcButton.h"
#include "Res.h"
#include "TrophyAppearButton.h"
#include "TrophyButton.h"
#include "TrophyCloseUpButton.h"
#include "TrophyPageButton.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The back button: how wide it is, and how far its left edge is left of the middle and its top above the bottom
const int BACK_BUTTON_WIDTH = 0x17c;
const int BACK_BUTTON_LEFT = 0xbe;
const int BACK_BUTTON_RAISE = 4;

// The shelf: seven trophies to a row, each place this much wider than the trophy, and the top row this far down; the places of a row
// lie along the first half of a sine wave (the amplitude is this)
const int PLACES_PER_ROW = 7;
const int PLACE_GAP = 0x19;
const int SHELF_LEFT_MARGIN = 0xc;
const int SHELF_TOP = 0xcd;
const double WAVE_STEP = 0.16666666666666666;
const double WAVE_AMPLITUDE = 40.0;

// The trophy that stands in each place of the shelf, place by place from the top left
const int PLACE_TROPHY[NUM_TROPHIES] = { 0, 7, 18, 15, 17, 5, 6, 11, 8, 9, 19, 1, 12, 13, 2, 14, 16, 20, 3, 10, 4 };

// The trophies are shown at half the size of their cels
const double TROPHY_SCALE = 0.5;

// The text under the shelf: the name of the trophy that the mouse is over, and below it what it is for; or the line that says what to do
const int TEXT_Y = 0x1f4;
const int DESCRIPTION_SPACING = 0x19;
const int HINT_Y = 0x20d;

// How long a trophy won flashes in the shelf
const double FLASH_TIME = 3.0;

// 0x460570, 0x460580: the wipes' callbacks
void WipeToMainMenu()
{
	gApp->TrophyRoomToMainMenu();
}

void WipeToGame()
{
	gApp->TrophyRoomToGame();
}

}

// 0x460c00
TrophyRoomWidget::TrophyRoomWidget(int theMode, const std::vector<int>& theNewTrophies) :
	mNewTrophies(theNewTrophies)
{
	mBackButton = nullptr;
	mOverTrophy = -1;
	mMode = theMode;
	mSelected = nullptr;
	mNewIndex = -1;
	if (!mNewTrophies.empty())
		mNewIndex = 0;

	const char* aBackLabel = "";
	if (theMode == MODE_MENU)
		aBackLabel = "Back to Main Menu";
	else if (theMode == MODE_GAME)
	{
		aBackLabel = "Back to Game";
		for (size_t aNew = 0; aNew < mNewTrophies.size(); aNew++)
			gApp->mUserManager->GetCurrentUser()->AwardTrophy(mNewTrophies[aNew]);
	}
	mBackButton = MakeNcButton(BUTTON_BACK, this, aBackLabel, nullptr, 3);

	int aTrophyWidth = ToInt(IMAGE_TROPHIES->GetCelWidth() * TROPHY_SCALE);
	int aTrophyHeight = ToInt(IMAGE_TROPHIES->GetCelHeight() * TROPHY_SCALE);
	for (int aTrophy = 0; aTrophy < NUM_TROPHIES; aTrophy++)
	{
		bool anEarned = gApp->mUserManager->GetCurrentUser()->HasTrophy(aTrophy);
		mTrophyButtons.push_back(new TrophyButton(aTrophy + BUTTON_FIRST_TROPHY, this, aTrophy, anEarned, aTrophyWidth, aTrophyHeight));
	}

	mCloseUp = new TrophyCloseUpButton(BUTTON_CLOSE_UP, this);
	mAppear = new TrophyAppearButton(BUTTON_APPEAR, this, mNewIndex < 0 ? -1 : mNewTrophies[mNewIndex]);

	if (theMode == MODE_MENU)
	{
		ShowShelf(true);
		mAppear->mVisible = false;
	}
	else if (theMode == MODE_GAME)
	{
		ShowShelf(false);
		mAppear->mVisible = true;
	}
	if (theMode == MODE_MENU || theMode == MODE_GAME)
		ShowCloseUp(false);
}

// 0x460600
TrophyRoomWidget::~TrophyRoomWidget()
{
	delete mBackButton;
	for (size_t aTrophy = 0; aTrophy < mTrophyButtons.size(); aTrophy++)
		delete mTrophyButtons[aTrophy];
	delete mCloseUp;
	delete mAppear;
}

// 0x4608d0
void TrophyRoomWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mBackButton);
	for (size_t aTrophy = 0; aTrophy < mTrophyButtons.size(); aTrophy++)
		theWidgetManager->AddWidget(mTrophyButtons[aTrophy]);
	theWidgetManager->AddWidget(mCloseUp);
	theWidgetManager->AddWidget(mAppear);
}

// 0x460940
void TrophyRoomWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mBackButton);
	for (size_t aTrophy = 0; aTrophy < mTrophyButtons.size(); aTrophy++)
		theWidgetManager->RemoveWidget(mTrophyButtons[aTrophy]);
	theWidgetManager->RemoveWidget(mCloseUp);
	theWidgetManager->RemoveWidget(mAppear);
}

// 0x4606f0
void TrophyRoomWidget::ShowShelf(bool theShow)
{
	for (size_t aTrophy = 0; aTrophy < mTrophyButtons.size(); aTrophy++)
		mTrophyButtons[aTrophy]->mVisible = theShow;
	mBackButton->mVisible = theShow;
	mVisible = theShow;
}

// 0x4605a0
void TrophyRoomWidget::ShowCloseUp(bool theShow)
{
	mCloseUp->mVisible = theShow;
	mCloseUp->Restart();
}

// 0x460730: the page of the next trophy won
void TrophyRoomWidget::AnnounceNext()
{
	mNewIndex++;
	mAppear->Start(mNewTrophies[mNewIndex], false);
}

// 0x460760: the back button is at the bottom, in the middle; the trophies are in the places of the shelf; the pages cover the screen
void TrophyRoomWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	int aButtonHeight = mBackButton->mFont->GetHeight();
	mBackButton->Resize(theWidth / 2 - BACK_BUTTON_LEFT + theX, theY - aButtonHeight - BACK_BUTTON_RAISE + theHeight, BACK_BUTTON_WIDTH,
		aButtonHeight);

	int aTrophyWidth = mTrophyButtons[0]->mWidth;
	int aTrophyHeight = mTrophyButtons[0]->mHeight;
	for (int aPlace = 0; aPlace < NUM_TROPHIES; aPlace++)
	{
		int aColumn = aPlace % PLACES_PER_ROW;
		int aRow = aPlace / PLACES_PER_ROW;
		int aWave = ToInt(sin((double) aColumn * WAVE_STEP * 3.141592653589793) * WAVE_AMPLITUDE);
		int aWaveSign = aWave >> 31;
		mTrophyButtons[PLACE_TROPHY[aPlace]]->Resize(
			(aTrophyWidth + PLACE_GAP) * aColumn + (mWidth - (aTrophyWidth + PLACE_GAP) * PLACES_PER_ROW) / 2 + SHELF_LEFT_MARGIN,
			((aWave ^ aWaveSign) - aWaveSign) + SHELF_TOP + aRow * aTrophyHeight,
			aTrophyWidth, aTrophyHeight);
	}

	mCloseUp->Resize(0, 0, mWidth, mHeight);
	mAppear->Resize(0, 0, mWidth, mHeight);
}

// 0x460b40: which trophy the mouse is over (the text under the shelf follows it)
void TrophyRoomWidget::Update()
{
	Widget::Update();

	for (int aTrophy = 0; aTrophy < NUM_TROPHIES; aTrophy++)
	{
		if (mTrophyButtons[aTrophy]->mIsOver)
		{
			if (mOverTrophy == aTrophy)
				return;
			mOverTrophy = aTrophy;
			MarkDirty();
			return;
		}
	}

	if (mOverTrophy != -1)
	{
		mOverTrophy = -1;
		MarkDirty();
	}
}

// 0x4610a0: the name of a trophy, and under it what it was for
void TrophyRoomWidget::DrawTrophyText(Graphics* g, int theTrophy, int theY)
{
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_PTBANANASPLIT24);
	std::string aName = GetTrophyName(theTrophy);
	g->DrawString(aName, mWidth / 2 - g->StringWidth(aName) / 2, theY);

	g->SetFont(FONT_EUROFURENCE14BOLD);
	std::string aDescription = GetTrophyDescription(theTrophy);
	g->DrawString(aDescription, mWidth / 2 - g->StringWidth(aDescription) / 2, theY + DESCRIPTION_SPACING);
}

// 0x461290: the room, and under it the text that goes with the trophy the mouse is over, or what to do (nothing while a trophy is
// shown close up)
void TrophyRoomWidget::Draw(Graphics* g)
{
	Graphics aG(*g);
	aG.DrawImage(IMAGE_TROPHYROOMWIDGET, 0, 0);

	if (mOverTrophy != -1)
		DrawTrophyText(&aG, mOverTrophy, TEXT_Y);
	else if (!mCloseUp->mVisible)
	{
		int anEarned = 0;
		for (int aTrophy = 0; aTrophy < NUM_TROPHIES; aTrophy++)
		{
			if (mTrophyButtons[aTrophy]->mEarned)
				anEarned++;
		}

		std::string aHint = anEarned == 0 ? "You have no trophies" : "Click a trophy to examine it";
		aG.SetColor(Color(255, 255, 255, 255));
		aG.SetFont(FONT_EUROFURENCE14BOLD);
		aG.DrawString(aHint, mWidth / 2 - aG.StringWidth(aHint) / 2, HINT_Y);
	}

	if (mMode == MODE_GAME && gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
		aG.DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->mWidth, mHeight - IMAGE_TOUGH_COOKIE_STAMP->mHeight);
}

// 0x4609b0
void TrophyRoomWidget::ButtonDepress(int theId)
{
	if (theId == BUTTON_BACK)
	{
		if (mMode == MODE_MENU)
			gApp->DoZoomWipe(WipeToMainMenu, WIPE_HAMMER);
		else if (mMode == MODE_GAME)
			gApp->DoZoomWipe(WipeToGame, WIPE_HAMMER);
	}
	else if (theId == BUTTON_CLOSE_UP)
	{
		// the page of a trophy is left once its button is ready: the trophy's place shows it again
		if (mCloseUp->mClickProgress == 1.0)
		{
			gApp->PlaySample(SOUND_UI_BUTTON_CLICK);
			mCloseUp->mVisible = false;
			mCloseUp->Restart();
			if (mSelected != nullptr)
			{
				mSelected->mVisible = true;
				mSelected = nullptr;
			}
		}
	}
	else if (theId == BUTTON_APPEAR)
	{
		// the page of a trophy won is left once its button is ready: the page of the next one, or the shelf with the new ones flashing
		if (mAppear->mClickProgress == 1.0)
		{
			gApp->PlaySample(SOUND_UI_BUTTON_CLICK);
			if (mNewIndex < (int) mNewTrophies.size() - 1)
			{
				AnnounceNext();
				return;
			}

			mAppear->mVisible = false;
			ShowShelf(true);
			for (size_t aNew = 0; aNew < mNewTrophies.size(); aNew++)
				mTrophyButtons[mNewTrophies[aNew]]->StartFlash(FLASH_TIME);
		}
	}
	else if (theId >= BUTTON_FIRST_TROPHY && theId < NUM_TROPHIES + BUTTON_FIRST_TROPHY)
	{
		// a trophy that has been won is shown close up, from where it is
		TrophyButton* aButton = mTrophyButtons[theId - BUTTON_FIRST_TROPHY];
		if (aButton->mEarned)
		{
			aButton->mVisible = false;
			mSelected = aButton;
			mCloseUp->Setup(aButton->mTrophy, aButton->mX, aButton->mY);
			ShowCloseUp(true);
		}
	}
}
