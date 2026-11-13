#include "CutsceneScreen.h"
#include "Board.h"
#include "Graphics.h"
#include "HeavyWeaponApp.h"
#include "HighScoresScreen.h"
#include "MusicInterface.h"
#include "Res.h"
#include "WidgetManager.h"

#include <math.h>

using namespace Sexy;

namespace
{
	const double MUSIC_FADE_OUT_SPEED = 0.004;
	const double MUSIC_FADE_IN_SPEED = 0.002;
	const int MIDDLE_SONG_ORDER = 0x4f;
	const double FADE_START = 0.2;
	const double FADE_STEP = 0.05;
	const double FADE_COVERED = 1.57;		// the fade covers the screen: the armory goes in under it
	const double FADE_DONE = 3.141;
}

// 0x41f070
CutsceneScreen::CutsceneScreen(HeavyWeaponApp* theApp, bool isFinal)
{
	mApp = theApp;
	mCutscene = isFinal ? CUTSCENE_FINAL : CUTSCENE_MIDDLE;
	mFadeAngle = 0.0;
	mFlashWidget = new FlashWidget(0, this);
	if (mCutscene == CUTSCENE_MIDDLE)
	{
		mApp->mMusicInterface->FadeOutAll(true, MUSIC_FADE_OUT_SPEED);
		mApp->mMusicInterface->FadeIn(SONG_ATOMIC_TANK_79, MIDDLE_SONG_ORDER, MUSIC_FADE_IN_SPEED, false);
	}
	else
		mFlashWidget->mBkgColor = Color(0x99, 0x33, 0x33);
	Widget::Resize(0, 0, 640, 480);
	if (mFlashWidget != NULL)
		mFlashWidget->Resize(0, 0, 640, 480);
}

// 0x41f1a0 (body 0x41ed50)
CutsceneScreen::~CutsceneScreen()
{
	delete mFlashWidget;
}

// 0x41f1d0 slot 1: the movie starts behind every other widget, and the screen takes the keyboard
void CutsceneScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mFlashWidget);
	if (mCutscene == CUTSCENE_MIDDLE)
		mFlashWidget->StartAnimation("images\\middle.swf");
	else if (mCutscene == CUTSCENE_FINAL)
		mFlashWidget->StartAnimation("images\\final.swf");
	theWidgetManager->BringToBack(mFlashWidget);
	theWidgetManager->SetFocus(this);
}

// 0x41edb0 slot 2
void CutsceneScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	if (mFlashWidget != NULL)
		theWidgetManager->RemoveWidget(mFlashWidget);
}

// 0x41ede0 slot 13
void CutsceneScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	if (mFlashWidget != NULL)
		mFlashWidget->Resize(0, 0, 640, 480);
}

// 0x41ee20: the middle movie starts its fade; the final one ends the campaign on the high scores
void CutsceneScreen::EndCutscene()
{
	if (mCutscene == CUTSCENE_MIDDLE)
	{
		if (mFadeAngle == 0.0)
			mFadeAngle += FADE_START;
	}
	else if (mCutscene == CUTSCENE_FINAL)
	{
		int aRank = mApp->AddHighScore(false, true);
		if (aRank == Board::NO_RANK)
			mApp->ShowMainMenu();
		else
			mApp->mWidgetManager->AddWidget(new HighScoresScreen(mApp, false, aRank));
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
	}
}

// 0x41ef50 slot 16: the fade
void CutsceneScreen::Draw(Graphics* g)
{
	if (mFadeAngle > 0.0)
	{
		g->SetColor(Color(0, 0xff, 0, (int)(sin(mFadeAngle) * 255.0)));
		g->FillRect(0, 0, 640, 480);
	}
}

// 0x41efc0 slot 17: once the fade covers the screen the movie goes and the armory comes in under it
void CutsceneScreen::Update()
{
	if (mFadeAngle > 0.0)
	{
		double anAngle = mFadeAngle;
		mFadeAngle = anAngle + FADE_STEP;
		if (anAngle + FADE_STEP > FADE_COVERED && mApp->mArmory == NULL)
		{
			mApp->ShowArmory();
			mApp->mWidgetManager->RemoveWidget(mFlashWidget);
			// port: the original leaves the movie widget allocated
			mApp->SafeDeleteWidget(mFlashWidget);
			mFlashWidget = NULL;
			mMouseVisible = false;
			mApp->mWidgetManager->BringToFront(this);
		}
		if (mFadeAngle >= FADE_DONE)
		{
			mApp->mWidgetManager->RemoveWidget(this);
			mApp->SafeDeleteWidget(this);
		}
		MarkDirty();
	}
}

// 0x41ef10 slot 28: a click ends the movie once the screen has the focus (a first click without it only takes it)
void CutsceneScreen::MouseDown(int x, int y, int theClickCount)
{
	if (mHasFocus)
		EndCutscene();
	else
		mHasFocus = true;
}

// 0x41ef30 (listener slot 0): the middle movie fades out when it ends; the final one stays on its last frame
void CutsceneScreen::FlashAnimEnded(int theId)
{
	if (mCutscene != CUTSCENE_FINAL)
		mFadeAngle += FADE_START;
}
