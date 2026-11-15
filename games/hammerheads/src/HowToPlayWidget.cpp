#include "HowToPlayWidget.h"

#include "Application.h"
#include "Checkbox.h"
#include "Color.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "NcButton.h"
#include "Res.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The movie, and the strip under it
const char* const MOVIE_FILE = "images\\howtoplay.swf";
const int MOVIE_WIDTH = 800;
const int MOVIE_HEIGHT = 500;
const int STRIP_HEIGHT = 100;
const Color STRIP_COLOR(15, 200, 11);

// The button's width, and how far above the bottom of the screen its top is (less the height of its font)
const int BUTTON_WIDTH = 0x14f;
const int BUTTON_RAISE = 0x14;

// With a check box, the button is left of the middle by this, and the box is right of it by this
const int BUTTON_SHIFT_LEFT = 100;
const int CHECKBOX_SHIFT_RIGHT = 0x113;

// Where the check box's text starts
const int CHECKBOX_TEXT_X = 0x21c;
const int CHECKBOX_TEXT_Y = 0x235;

}

int HowToPlayWidget::sCount = 0;

// 0x456c70
HowToPlayWidget::HowToPlayWidget(bool theWithNewGame)
{
	mButton = nullptr;
	mCheckbox = nullptr;

	if (!theWithNewGame)
		mButton = MakeNcButton(BUTTON_DONE, this, "Done", nullptr, 3);
	else
	{
		mButton = MakeNcButton(BUTTON_PLAY, this, "Click here to play", nullptr, 3);
		mCheckbox = new Checkbox(IMAGE_CHECKBOX_UNCHECKED, IMAGE_CHECKBOX_CHECKED, CHECKBOX_DONT_SHOW, this);
		mCheckbox->mChecked = !gApp->mUserManager->GetCurrentUser()->mShowHowToPlay;
	}

	mFlash = new FlashWidget(0, this);
	mFlash->mPauseWhenInactive = true;
	mFlash->mMouseVisible = false;
	mFlash->StartAnimation(MOVIE_FILE);

	gApp->PauseGame(true);
	sCount++;
}

// 0x4568d0: the voice says that the game is ready when the box that asks for it is up behind the page
HowToPlayWidget::~HowToPlayWidget()
{
	delete mButton;
	delete mCheckbox;
	delete mFlash;
	sCount--;
	if (gApp->GetDialog(DIALOG_READY_GO) != nullptr)
		gApp->PlaySample(SOUND_VOICE_BONUS_READY);
}

// 0x456bd0
bool HowToPlayWidget::IsActive()
{
	return sCount > 0;
}

// 0x4569a0
void HowToPlayWidget::Update()
{
	Widget::Update();
	MarkDirty();
}

// 0x4569b0
void HowToPlayWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mButton);
	if (mCheckbox != nullptr)
		theWidgetManager->AddWidget(mCheckbox);
	theWidgetManager->AddWidget(mFlash);
}

// 0x456a00: the choice of the check box is the player's from now on
void HowToPlayWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mButton);
	if (mCheckbox != nullptr)
	{
		gApp->mUserManager->GetCurrentUser()->mShowHowToPlay = !mCheckbox->mChecked;
		theWidgetManager->RemoveWidget(mCheckbox);
	}
	theWidgetManager->RemoveWidget(mFlash);
}

// 0x456a70: the movie is at the top, in the middle; the button, and the check box if there is one, are at the bottom
void HowToPlayWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	int aButtonHeight = mButton->mFont->GetHeight();
	int aButtonX = (theWidth - BUTTON_WIDTH) / 2 + theX;
	int aButtonY = theY - aButtonHeight + theHeight;
	if (mCheckbox == nullptr)
		mButton->Resize(aButtonX, aButtonY - BUTTON_RAISE, BUTTON_WIDTH, aButtonHeight);
	else
	{
		mButton->Resize(aButtonX - BUTTON_SHIFT_LEFT, aButtonY - BUTTON_RAISE, BUTTON_WIDTH, aButtonHeight);
		mCheckbox->Resize(aButtonX + CHECKBOX_SHIFT_RIGHT, aButtonY, BUTTON_WIDTH, aButtonHeight);
	}

	mFlash->Resize((theWidth - MOVIE_WIDTH) / 2 + theX, theY, MOVIE_WIDTH, MOVIE_HEIGHT);
}

// 0x456fc0
void HowToPlayWidget::Draw(Graphics* g)
{
	g->SetColor(STRIP_COLOR);
	g->FillRect(Rect(0, MOVIE_HEIGHT, mWidth, STRIP_HEIGHT));

	if (mCheckbox != nullptr)
	{
		g->SetFont(FONT_EUROFURENCE14);
		g->SetColor(Color(0, 0, 0, 255));
		g->DrawString("Don't show this again", CHECKBOX_TEXT_X, CHECKBOX_TEXT_Y);
	}

	if (gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
		g->DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->mWidth, mHeight - IMAGE_TOUGH_COOKIE_STAMP->mHeight);
}

// 0x456b70: either button takes the page down and lets the game go on
void HowToPlayWidget::ButtonDepress(int theId)
{
	if (theId == BUTTON_DONE || theId == BUTTON_PLAY)
	{
		gApp->PauseGame(false);
		gApp->HideHowToPlay();
	}
}

// 0x456bb0
void HowToPlayWidget::CheckboxChecked(int theId, bool theChecked)
{
	gApp->PlaySample(SOUND_UI_CHECK);
}

// 0x456c00: the movie plays again from its start (the original also prints a line to the debugger)
void HowToPlayWidget::FlashAnimEnded(int theId)
{
	if (mFlash->mPauseCount == 0)
		mFlash->Restart();
}

// 0x456f10
HowToPlayWidget* Sexy::MakeHowToPlay(bool theWithNewGame)
{
	HowToPlayWidget* aWidget = new HowToPlayWidget(theWithNewGame);
	aWidget->Resize(0, 0, gApp->mWidth, gApp->mHeight);
	gApp->mWidgetManager->AddWidget(aWidget);
	gApp->mWidgetManager->SetFocus(aWidget);
	return aWidget;
}
