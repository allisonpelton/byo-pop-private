#include "MenuScreen.h"
#include "FadeButton.h"
#include "UserProfileMgr.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "ButtonWidget.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Image.h"
#include "DDImage.h"
#include "ImageFont.h"
#include "compat2004/Compat2004.h"
#include "Res.h"

using namespace Sexy;

// 0x4436e0: the six buttons, in the order adventure (id 0x64), abyss (0x65), typing tutor (0x66), quit (0x68), high scores
// (0x67), options (0x6b), then the two links (0x69, 0x6a); the listener of each is the app. The buttons are FadeButtons when
// the app is set for a fast CPU (mFastCPU, +0x8ec; the port always sets it) and plain buttons otherwise
MenuScreen::MenuScreen(WinWSApp* theApp)
{
	mApp = theApp;
	mButtons[BUTTON_ADVENTURE] = NULL;	// the original clears the first five fields (+0x5c..+0x6c); +0x70 is stored last
	mButtons[BUTTON_ABYSS] = NULL;
	mButtons[BUTTON_TUTOR] = NULL;
	mButtons[BUTTON_QUIT] = NULL;
	mButtons[BUTTON_HIGH_SCORES] = NULL;

	if (!mApp->mFastCPU)
	{
		mButtons[BUTTON_ADVENTURE] = new ButtonWidget(ADVENTURE_BUTTON_ID, mApp);
		mButtons[BUTTON_ABYSS] = new ButtonWidget(ABYSS_BUTTON_ID, mApp);
		mButtons[BUTTON_TUTOR] = new ButtonWidget(TUTOR_BUTTON_ID, mApp);
		mButtons[BUTTON_QUIT] = new ButtonWidget(QUIT_BUTTON_ID, mApp);
		mButtons[BUTTON_HIGH_SCORES] = new ButtonWidget(HIGH_SCORES_BUTTON_ID, mApp);
		mButtons[BUTTON_OPTIONS] = new ButtonWidget(OPTIONS_BUTTON_ID, mApp);
	}
	else
	{
		mButtons[BUTTON_ADVENTURE] = new FadeButton(ADVENTURE_BUTTON_ID, mApp);		// 0x443440
		mButtons[BUTTON_ABYSS] = new FadeButton(ABYSS_BUTTON_ID, mApp);
		mButtons[BUTTON_TUTOR] = new FadeButton(TUTOR_BUTTON_ID, mApp);
		mButtons[BUTTON_QUIT] = new FadeButton(QUIT_BUTTON_ID, mApp);
		mButtons[BUTTON_HIGH_SCORES] = new FadeButton(HIGH_SCORES_BUTTON_ID, mApp);
		mButtons[BUTTON_OPTIONS] = new FadeButton(OPTIONS_BUTTON_ID, mApp);
	}

	mNotYouLink = new HyperlinkWidget(NOT_YOU_LINK_ID, mApp);
	mStatsLink = new HyperlinkWidget(STATS_LINK_ID, mApp);
}

// 0x4410f0 (deleting destructor 0x441e70, slot 0): of each of the six buttons that exists, the copied normal picture goes
// first, then the button (the over and down pictures are the app's); then the two links
MenuScreen::~MenuScreen()
{
	ButtonWidget** aButtonPtr = &mButtons[0];
	int aCount = NUM_BUTTONS;
	do
	{
		if (*aButtonPtr != NULL)
		{
			Image* anImage = (*aButtonPtr)->mButtonImage;
			if (anImage != NULL)
				delete anImage;
			if (*aButtonPtr != NULL)
				delete *aButtonPtr;
			*aButtonPtr = NULL;
		}
		aButtonPtr++;
		aCount--;
	} while (aCount != 0);

	if (mNotYouLink != NULL)
		delete mNotYouLink;
	if (mStatsLink != NULL)
		delete mStatsLink;
}

// 0x4482d0 slot 1: the buttons are placed over the selector picture: each gets, as its normal picture, a copy of the
// part of IMAGE_GAMESELECTOR under it, and the app's over and down pictures (the buttons only point at those). Adventure
// (0x12f, 0x20), abyss (0x12f, 0x88) and typing tutor (0x12f, 0xf0) are 0x136 by 0x65; quit is 0x62 by 0x53 at (0x1ff,
// 0x16a); high scores (0x133, 0x16a) and options (0x133, 0x194) are 0xcc by 0x29. All have the hand cursor. For a fast CPU
// the fading is set up on each button (27 per step, every update, on entering, leaving, pressing and releasing). The two
// links are white when the pointer is over them and brown otherwise, 12 point bold, with a 1 pixel underline, as wide as
// their text and 10 more; then the buttons join the manager (the links first)
void MenuScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	ButtonWidget* aButton = mButtons[BUTTON_ADVENTURE];
	aButton->Resize(0x12f, 0x20, 0x136, 0x65);
	Rect aRect(0x12f, 0x20, 0x136, 0x65);
	aButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_GAMESELECTOR], aRect);
	aButton->mOverImage = mApp->mImages[IMAGE_BUTTON_ADVENTURE_OVER];
	aButton->mDownImage = mApp->mImages[IMAGE_BUTTON_ADVENTURE_DOWN];
	aButton->mDoFinger = true;

	aButton = mButtons[BUTTON_ABYSS];
	aButton->Resize(0x12f, 0x88, 0x136, 0x65);
	aRect = Rect(0x12f, 0x88, 0x136, 0x65);
	aButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_GAMESELECTOR], aRect);
	aButton->mOverImage = mApp->mImages[IMAGE_BUTTON_ABYSS_OVER];
	aButton->mDownImage = mApp->mImages[IMAGE_BUTTON_ABYSS_DOWN];
	aButton->mDoFinger = true;

	aButton = mButtons[BUTTON_TUTOR];
	aButton->Resize(0x12f, 0xf0, 0x136, 0x65);
	aRect = Rect(0x12f, 0xf0, 0x136, 0x65);
	aButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_GAMESELECTOR], aRect);
	aButton->mOverImage = mApp->mImages[IMAGE_BUTTON_TUTOR_OVER];
	aButton->mDownImage = mApp->mImages[IMAGE_BUTTON_TUTOR_DOWN];
	aButton->mDoFinger = true;

	aButton = mButtons[BUTTON_QUIT];
	aButton->Resize(0x1ff, 0x16a, 0x62, 0x53);
	aRect = Rect(0x1ff, 0x16a, 0x62, 0x53);
	aButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_GAMESELECTOR], aRect);
	aButton->mOverImage = mApp->mImages[IMAGE_BUTTON_QUIT_OVER];
	aButton->mDownImage = mApp->mImages[IMAGE_BUTTON_QUIT_DOWN];
	aButton->mDoFinger = true;

	aButton = mButtons[BUTTON_HIGH_SCORES];
	aButton->Resize(0x133, 0x16a, 0xcc, 0x29);
	aRect = Rect(0x133, 0x16a, 0xcc, 0x29);
	aButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_GAMESELECTOR], aRect);
	aButton->mOverImage = mApp->mImages[IMAGE_BUTTON_HALL_OVER];
	aButton->mDownImage = mApp->mImages[IMAGE_BUTTON_HALL_DOWN];
	aButton->mDoFinger = true;

	aButton = mButtons[BUTTON_OPTIONS];
	aButton->Resize(0x133, 0x194, 0xcc, 0x29);
	aRect = Rect(0x133, 0x194, 0xcc, 0x29);
	aButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_GAMESELECTOR], aRect);
	aButton->mOverImage = mApp->mImages[IMAGE_BUTTON_OPTIONS_OVER];
	aButton->mDownImage = mApp->mImages[IMAGE_BUTTON_OPTIONS_DOWN];
	aButton->mDoFinger = true;

	if (mApp->mFastCPU)
	{
		int i = 0;
		int aCount = NUM_BUTTONS;
		do
		{
			FadeButton* aFadeButton = (FadeButton*) mButtons[i];
			aFadeButton->mFadeStep = 0x1b;
			aFadeButton->mFadeInterval = 1;
			aFadeButton->mFadeOnEnter = true;
			aFadeButton->mFadeOnLeave = true;
			aFadeButton->mFadeOnDown = true;
			aFadeButton->mFadeOnUp = true;
			i++;
			aCount--;
		} while (aCount != 0);
	}

	mNotYouLink->SetVisible(true);
	mNotYouLink->mUnderlineSize = 1;
	mNotYouLink->SetFont(mApp->mFontMyriadCondensed12Bold);
	mNotYouLink->mOverColor = Color(0xff, 0xff, 0xff);
	mNotYouLink->mColor = Color(0x8c, 0x3c, 0);
	mNotYouLink->mLabel = GT(0x0052fb0c);
	mNotYouLink->SetDisabled(false);
	mNotYouLink->Resize(0x4b, 0x4e, mNotYouLink->mFont->StringWidth(mNotYouLink->mLabel) + 10, 0x1e);
	theWidgetManager->AddWidget(mNotYouLink);
	mNotYouLink->mDoFinger = true;

	mStatsLink->SetVisible(true);
	mStatsLink->mUnderlineSize = 1;
	mStatsLink->SetFont(mApp->mFontMyriadCondensed12Bold);
	mStatsLink->mOverColor = Color(0xff, 0xff, 0xff);
	mStatsLink->mColor = Color(0x96, 0x46, 0x14);
	mStatsLink->mLabel = GT(0x0052fae8);
	mStatsLink->SetDisabled(false);
	mStatsLink->Resize(0x3e, 0x67, mStatsLink->mFont->StringWidth(mStatsLink->mLabel) + 10, 0x1e);
	theWidgetManager->AddWidget(mStatsLink);
	mStatsLink->mDoFinger = true;

	int i = 0;
	int aCount = NUM_BUTTONS;
	do
	{
		theWidgetManager->AddWidget(mButtons[i]);
		i++;
		aCount--;
	} while (aCount != 0);
}

// 0x4411d0 slot 2: the six buttons, then the two links, leave the manager
void MenuScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);

	int i = 0;
	int aCount = NUM_BUTTONS;
	do
	{
		theWidgetManager->RemoveWidget(mButtons[i]);
		i++;
		aCount--;
	} while (aCount != 0);

	theWidgetManager->RemoveWidget(mNotYouLink);
	theWidgetManager->RemoveWidget(mStatsLink);
}

// 0x441190 slot 4: the six buttons follow the screen; the links are left as they are
void MenuScreen::SetVisible(bool isVisible)
{
	Widget::SetVisible(isVisible);

	int i = 0;
	int aCount = NUM_BUTTONS;
	do
	{
		mButtons[i]->SetVisible(isVisible);
		i++;
		aCount--;
	} while (aCount != 0);
}

// 0x448860 slot 16: the selector picture; then, in white Rockwell 13, the greeting centred in a 0xfa wide column at
// (0x23, 0x37): "Welcome to Typer Shark!" when no player is chosen (the current name is empty), otherwise "Welcome to Typer
// Shark," with the name centred in the same column one font height and 0x3a lower
void MenuScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_GAMESELECTOR], 0, 0);
	g->SetFont(mApp->mFontRockwell13);
	g->SetColor(Color(0xff, 0xff, 0xff));

	SexyString aName = mApp->mProfileMgr->GetCurrentUserName();		// 0x4150a0
	SexyString aGreeting;
	if (aName == "")
		aGreeting = GT(0x0052fb44);
	else
		aGreeting = GT(0x0052fb2c);
	WriteString2004(g, aGreeting, 0x23, 0x37, 0xfa, 0, true, 0, -1);

	if (aName != "")
		WriteString2004(g, aName, 0x23, mApp->mFontRockwell13->GetHeight() + 0x3a, 0xfa, 0, true, 0, -1);
}
