// The full-screen widgets of Rocket Mania Deluxe (see Screens.h).

#include "Screens.h"
#include "Dialogs.h"
#include "RocketApp.h"
#include "TalkingHead.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// LoadingScreen

// 0x4104b0
LoadingScreen::LoadingScreen(RocketApp* theApp) :
	mApp(theApp),
	mProgress(0.0),
	mLinkShown(false)
{
	// port: the original makes a second link, "register now", for the unregistered game and hides it for a
	// registered one; this one is the play link, which starts hidden until the game has loaded
	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mFonts[FONT_DOMO14_BOLD]);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(0xb4, 0, 0);
	mPlayLink->mOverColor = Color(0xeb, 0x7d, 5);
	mPlayLink->mUnderlineSize = 0;
}

// 0x410ab0
LoadingScreen::~LoadingScreen()
{
	delete mPlayLink;
}

// 0x4117b0 / 0x4117f0
void LoadingScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

void LoadingScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4111b0
void LoadingScreen::Update()
{
	Widget::Update();

	// the link's text and place are set again on every update
	mPlayLink->mLabel = GT(0x004e54d8);
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize(0x140 - (aWidth + 8) / 2, 0x1b6, aWidth + 8, 0x1a);

	// port: the original looks at the loading thread's progress every fourth update; the port looks at every one, so that
	// the play link shows the update the game has loaded (fast loading, a deviation listed in GAME_NOTES.md)
	if (!mLinkShown)
	{
		mProgress = mApp->GetLoadingThreadProgress();
		if (mProgress == 1.0 && mApp->mLoaded)
		{
			mPlayLink->SetVisible(true);
			mLinkShown = true;
			mProgress = 1.0;
		}
		MarkDirty();
	}
}

// 0x410bb0
void LoadingScreen::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	g->SetColor(Color(0, 0, 0));
	g->DrawImage(anImages[IMAGE_LOADINGGRAPHIC], 0, 0);

	int aBarWidth = anImages[IMAGE_LOADINGBAR]->GetWidth();
	int aBarX = (mWidth - aBarWidth) / 2;
	g->DrawImage(anImages[IMAGE_LOADINGBAREMPTY], aBarX, 0x186);
	g->SetFont(mApp->mFonts[FONT_ARIAL8]);

	g->SetColor(Color(100, 100, 100, 180));
	if (mProgress >= 1.0)
		g->DrawImage(anImages[IMAGE_LOADINGBAR], aBarX, 0x186);
	else if (mProgress > 0.0)
	{
		int aFilled = (int) (mProgress * aBarWidth);
		g->DrawImage(anImages[IMAGE_LOADINGBAR], aBarX, 0x186, Rect(0, 0, aFilled, 0x34));
		// the bar's leading edge: 16 columns that fade out, added on top
		for (int i = 0; i < 16; i++)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(0xff, 0xff, 0xff, (int) (cos(i * 0.0981746875) * 255.0)));
			g->DrawImage(anImages[IMAGE_LOADINGBAR], aBarX + aFilled + i, 0x186, Rect(aFilled + i, 0, 1, 0x34));
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	// the line under the bar: the thanks (empty with NoReg) or, on the first run, the wait for the sounds
	SexyString aLine;
	if (!mApp->GetBoolean("NoReg", false))
		aLine = GT(0x004e5258);
	if (mApp->mTimesExecuted == 0 && mProgress < 1.0)
		aLine = GT(0x004e5234);
	if (!mPlayLink->mVisible)
	{
		g->SetFont(mApp->mFonts[FONT_ARIAL10_BOLD]);
		int aX = 0x140 - g->GetFont()->StringWidth(aLine) / 2;
		g->SetColor(Color(0xaa, 10, 10));
		g->DrawString(aLine, aX, 0x1c2);
	}
}

// 0x411830: the play link
void LoadingScreen::ButtonDepress(int theId)
{
	mApp->LoadingScreenDone();
}

// ---------------------------------------------------------------------------------------------------------
// GameSelector

// the hint texts' addresses (the pointer table at 0x4e44d0): nothing hovered, the three modes, the lower buttons
static const unsigned int gHintTexts[7] =
{
	0x004e45e0, 0x004e45b0, 0x004e4590, 0x004e4550, 0x004e4530, 0x004e4514, 0x004e44ec
};

static DialogButton* MakeMenuButton(RocketApp* theApp, int theId, ButtonListener* theListener, const SexyString& theLabel,
	const Color& theColor, int theY)
{
	DialogButton* aButton = new DialogButton(theApp->mImages[IMAGE_DARKBUTTON], theId, theListener);
	aButton->SetFont(theApp->mFonts[FONT_DOMO14_BOLD]);
	aButton->SetColor(ButtonWidget::COLOR_LABEL, theColor);
	aButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(0xff, 0xff, 0xff));
	aButton->mLabel = theLabel;
	aButton->mMouseInsets = Insets(-10, -10, -10, -10);
	aButton->mDoFinger = true;
	aButton->Resize(4, theY, 0x86, 0x23);
	return aButton;
}

// 0x404e30
GameSelector::GameSelector(RocketApp* theApp) :
	mApp(theApp),
	mEffects(NULL),
	mAlpha(0xff),
	mHint(0),
	mDifficulty(-1),
	mMode(-1),
	mYOffset(0)
{
	Color aModeColor(0x97, 0xef, 0xc2);
	Color aMenuColor(0x97, 0xc2, 0xef);

	// the buttons stand one under the other: classic, arcade, strategy, then high scores, options and quit
	mModeButtons[BUTTON_CLASSIC] = MakeMenuButton(mApp, BUTTON_CLASSIC, this, RocketApp::ModeName(1), aModeColor, 0xa7);
	mModeButtons[BUTTON_ARCADE] = MakeMenuButton(mApp, BUTTON_ARCADE, this, RocketApp::ModeName(0), aModeColor, 0xd2);
	mModeButtons[BUTTON_STRATEGY] = MakeMenuButton(mApp, BUTTON_STRATEGY, this, RocketApp::ModeName(2), aModeColor, 0xfd);
	mHighScoresButton = MakeMenuButton(mApp, BUTTON_HIGH_SCORES, this, GT(0x004e464c), aMenuColor, 0x130);
	mOptionsButton = MakeMenuButton(mApp, BUTTON_OPTIONS, this, GT(0x004e4644), aMenuColor, 0x15b);
	mQuitButton = MakeMenuButton(mApp, BUTTON_QUIT, this, GT(0x004e463c), aMenuColor, 0x186);

	// the leaves: one effect of kind 2 that makes leaves for ever (its life is -1)
	mEffects = new EffectManager(mApp);
	Effect* aLeaves = mEffects->Add(2, true);
	aLeaves->mLife = -1;

	HeadBesideMenu();
}

GameSelector::~GameSelector()
{
	delete mEffects;
	for (int i = 0; i < 3; i++)
		delete mModeButtons[i];
	delete mHighScoresButton;
	delete mOptionsButton;
	delete mQuitButton;
}

// 0x405d80
void GameSelector::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mModeButtons[BUTTON_ARCADE]);
	theWidgetManager->AddWidget(mModeButtons[BUTTON_CLASSIC]);
	theWidgetManager->AddWidget(mModeButtons[BUTTON_STRATEGY]);
	theWidgetManager->AddWidget(mQuitButton);
	theWidgetManager->AddWidget(mHighScoresButton);
	theWidgetManager->AddWidget(mOptionsButton);
}

// 0x405e40
void GameSelector::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mModeButtons[BUTTON_ARCADE]);
	theWidgetManager->RemoveWidget(mModeButtons[BUTTON_CLASSIC]);
	theWidgetManager->RemoveWidget(mModeButtons[BUTTON_STRATEGY]);
	theWidgetManager->RemoveWidget(mQuitButton);
	theWidgetManager->RemoveWidget(mHighScoresButton);
	theWidgetManager->RemoveWidget(mOptionsButton);
}

// 0x405bd0: the hint under the picture follows the hovered button and fades in
void GameSelector::Update()
{
	Widget::Update();

	mEffects->Update();     // 0x41ab70

	// at update 100 the talking head welcomes the player: the first time in the run with the welcome line, later with the
	// line about launching fireworks
	if (mUpdateCnt == 100)
	{
		if (!mApp->mWelcomed)
		{
			mApp->mHead->Say(SOUND_VOICE_WELCOME_TO_ROCKET_MANIA_03);
			mApp->mHead->SetMood(0x10);
			mApp->mWelcomed = true;
		}
		else
		{
			mApp->mHead->Say(SOUND_VOICE_LET_S_LAUNCH_SOME_FIREWORKS_01);
			mApp->mHead->SetMood(1);
		}
	}

	if (mAlpha < 0xff)
	{
		mAlpha += 10;
		MarkDirty();
	}

	// the hovered button decides the hint; the lower three keep it while the pointer is over them
	DialogButton* aButtons[6] = {mModeButtons[BUTTON_ARCADE], mModeButtons[BUTTON_CLASSIC], mModeButtons[BUTTON_STRATEGY],
		mHighScoresButton, mOptionsButton, mQuitButton};
	bool anyOver = false;
	for (int i = 0; i < 5; i++)
	{
		if (aButtons[i]->mIsOver)
		{
			anyOver = true;
			if (mHint != i + 1)
			{
				mHint = i + 1;
				MarkDirty();
				mAlpha = 0;
			}
		}
	}
	if (aButtons[5]->mIsOver)
	{
		if (mHint != 6)
		{
			mHint = 6;
			MarkDirty();
			mAlpha = 0;
		}
	}
	else if (!anyOver && (mHint != 0))
	{
		mHint = 0;
		MarkDirty();
		mAlpha = 0;
	}
}

// 0x4059d0
void GameSelector::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_SELECTBACK], 0, mYOffset);

	if ((mApp->GetDialog(DIALOG_DIFFICULTY) == NULL) && (mApp->GetDialog(DIALOG_CONTINUE) == NULL) &&
		(mApp->GetDialog(DIALOG_REGISTER_TO_LOAD) == NULL))
	{
		// (the original lets the alpha reach 260; it is kept at 255 here)
		int anAlpha = (mAlpha > 0xff) ? 0xff : mAlpha;
		g->SetFont(mApp->mFonts[FONT_VERDANA12_BOLD]);
		g->SetColor(Color(0, anAlpha, anAlpha, anAlpha));
		const SexyString& aText = GT(gHintTexts[mHint]);
		g->DrawString(aText, 0x140 - g->GetFont()->StringWidth(aText) / 2, 0x1cc);
	}

	mEffects->Draw(g, false);       // 0x41abb0: the leaves
}

// 0x405ec0: every button of the selector beeps when it is pressed
void GameSelector::ButtonPress(int theId)
{
	(void) theId;
	mApp->ButtonPress(0);
}

// 0x406220
void GameSelector::ButtonDepress(int theId)
{
	switch (theId)
	{
	case BUTTON_ARCADE:
	case BUTTON_CLASSIC:
	case BUTTON_STRATEGY:
		mMode = theId;
		if (mApp->HasSavedGame(mMode))
			ShowContinueDialog();
		else
			ShowDifficultyDialog();
		break;
	case BUTTON_QUIT:
		mApp->ShowQuitDialog();
		break;
	case BUTTON_HIGH_SCORES:
		mApp->ShowHighScores(1);
		break;
	case BUTTON_OPTIONS:
		mApp->ShowOptions(true);
		break;
	}
}

// 0x405fb0
void GameSelector::ShowDifficultyDialog()
{
	DifficultyDialog* aDialog = new DifficultyDialog(mApp);
	aDialog->SetColor(0, mApp->mDialogColors[0]);
	aDialog->SetColor(1, mApp->mDialogColors[1]);
	aDialog->SetColor(3, mApp->mDialogColors[2]);
	aDialog->mContentInsets = Insets(0x24, 0x24, 0x24, 0x24);
	aDialog->Resize(5, (mHeight - 0x1a9) / 2, 0x1a4, 0x1a9);
	mApp->AddDialog(DIALOG_DIFFICULTY, aDialog);
	HeadBesideDialog();
}

// 0x4060d0
void GameSelector::ShowContinueDialog()
{
	ContinueDialog* aDialog = new ContinueDialog(mApp, mMode);
	aDialog->SetColor(0, mApp->mDialogColors[0]);
	aDialog->SetColor(1, mApp->mDialogColors[1]);
	aDialog->SetColor(3, mApp->mDialogColors[2]);
	aDialog->mContentInsets = Insets(0x24, 0x24, 0x24, 0x24);
	aDialog->Resize(5, (mHeight - 0x1a9) / 2, 0x1a4, 0x1a9);
	mApp->AddDialog(DIALOG_CONTINUE, aDialog);
	HeadBesideDialog();
}

// slot 44, 0x405ed0: the dialogs call this with the chosen mode and difficulty
void GameSelector::StartGame(bool theContinuing)
{
	mApp->mMode = mMode;
	mApp->mDifficulty = mDifficulty;
	// (the original counts the start in the trial game's counter when it is not registered)

	bool aHintShown = false;
	if (!theContinuing)
	{
		// the first game of a mode opens that mode's explanation, which starts the game when it is closed
		if (mApp->mMode == 1)
			aHintShown = mApp->ShowModeHint(2, false);
		else if (mApp->mMode == 2)
			aHintShown = mApp->ShowModeHint(1, false);
		else if (mApp->mMode == 0)
			aHintShown = mApp->ShowModeHint(0, false);
	}
	if (!aHintShown)
		mApp->StartBoard(theContinuing);

	for (int i = 0; i < 3; i++)
		mModeButtons[i]->SetDisabled(true);
	mQuitButton->SetDisabled(true);
	mHighScoresButton->SetDisabled(true);
	mOptionsButton->SetDisabled(true);
}

// 0x405950: the talking head waits at (150, 65) beside the menu, without its shadow
void GameSelector::HeadBesideMenu()
{
	mApp->mHead->Place(150, 65);
	mApp->mHead->SetShadow(false);
}

// 0x405980: while a dialog is up it moves to (400, 75), in front of the dialog
void GameSelector::HeadBesideDialog()
{
	mApp->mHead->Place(400, 75);
	mApp->mHead->SetShadow(false);
	mWidgetManager->BringToFront(mApp->mHead);
}
