#include "TitleScreen.h"
#include "GameText.h"
#include "Graphics.h"
#include "HeavyWeaponApp.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "Res.h"
#include "Sprite.h"
#include "WidgetManager.h"

#include <math.h>

using namespace Sexy;

namespace
{
	const int FADE_INTRO = 256;				// mFade while the intro plays
	const int FADE_STEP = 2;
	const double LOGO_STEP = 0.02;
	const double LOGO_DONE = 3.141;
	const int BAR_SLIDE_START = 4;
	const int BAR_SLIDE_STEP = 10;
	const int BAR_TRAVEL = 304;				// the bar's inside and slider move across 304 pixels
	const int BAR_Y = 0x1a6;				// the loading bar's centre, half the flash's alpha lower while it lasts
	const int BAR_CLIP_X = 168;				// the bar's inside is clipped to its frame's opening
	const int PLAY_BUTTON_Y = 405;
	const int PLAY_BUTTON_HEIGHT = 29;
	const int GLOW_PERIOD = 1570;			// the bar's glow pulses on the update count (0.04 radians an update)
	const int LOGO_X = 530;
	const int LOGO_Y = 260;
	const int INTRO_SONG_ORDER = 0x4f;
	const double MUSIC_FADE_OUT_SPEED = 0.004;
}

// 0x42ee70: the intro's music starts; the play button waits hidden
TitleScreen::TitleScreen(HeavyWeaponApp* theApp)
{
	mApp = theApp;
	mFlashWidget = new FlashWidget(0, this);
	mPlayButton = new HyperlinkWidget(BUTTON_PLAY, this);
	mPlayButton->mLabel = GT(0x52fd08);
	// port: the original passes SetFont a duplicate, which SetFont duplicates again; the first copy was never freed
	mPlayButton->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET20]);
	mPlayButton->mColor = Color(0xff, 0xc0, 0);
	mPlayButton->mOverColor = Color(0xff, 0xff, 0xff);
	mPlayButton->SetVisible(false);
	mPlayButton->mUnderlineSize = 0;
	mApp->mMusicInterface->PlayMusic(SONG_ATOMIC_TANK_79, INTRO_SONG_ORDER, true);
	mFade = FADE_INTRO;
	mLogoAngle = 0.0;
	mBarSlide = 0;
	TitleScreen::Resize(0, 0, 640, 480);
	mMouseVisible = false;
}

// 0x42e6b0 (deleting 0x42eb30): the title's own images go with it, and the voice calls the game's name
TitleScreen::~TitleScreen()
{
	delete mPlayButton;
	delete mFlashWidget;
	for (int anImage = IMAGE_TITLE; anImage <= IMAGE_LOADINGGLOW; anImage++)
	{
		delete mApp->mSprites[anImage];
		// port: the original leaves the freed pointers in the table; the port's app frees the table at its end
		mApp->mSprites[anImage] = NULL;
	}
	mApp->PlaySample(SOUND_V_ATOMICTANK, 0);
}

// 0x42f050 slot 1: the intro starts behind every other widget, and the screen takes the keyboard
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayButton);
	theWidgetManager->AddWidget(mFlashWidget);
	mFlashWidget->StartAnimation("images\\intro.swf");
	theWidgetManager->BringToBack(mFlashWidget);
	theWidgetManager->SetFocus(this);
}

// 0x42e760 slot 2
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayButton);
	theWidgetManager->RemoveWidget(mFlashWidget);
}

// 0x42e860 slot 13: the play button is as wide as its label, centred under the bar
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mFlashWidget->Resize(0, 0, 640, 480);
	int aWidth = mApp->mFonts[FONT_RUBBERSTAMPLET20]->StringWidth(mPlayButton->mLabel);
	mPlayButton->Resize(320 - aWidth / 2, PLAY_BUTTON_Y, aWidth, PLAY_BUTTON_HEIGHT);
}

// 0x42e7a0 slot 17
void TitleScreen::Update()
{
	Widget::Update();
	if (mFade < FADE_INTRO)
	{
		mFade -= FADE_STEP;
		if (mFade < 0)
			mFade = 0;
	}
	if (mFade == 0)
	{
		double anAngle = mLogoAngle;
		mLogoAngle = anAngle + LOGO_STEP;
		if (anAngle + LOGO_STEP > LOGO_DONE)
			mLogoAngle = LOGO_DONE;
	}
	if (mBarSlide != 0)
	{
		mBarSlide += BAR_SLIDE_STEP;
		if (mBarSlide > BAR_TRAVEL)
		{
			mBarSlide = BAR_TRAVEL;
			mPlayButton->SetVisible(true);
		}
	}
	MarkDirty();
}

// 0x42e840: called by the app once the loading is done
void TitleScreen::LoadingComplete()
{
	mBarSlide = BAR_SLIDE_START;
	mApp->PlaySample(SOUND_GUNPOWERUP, 0);
}

// 0x42eb60 slot 16
void TitleScreen::Draw(Graphics* g)
{
	if (mFade < FADE_INTRO)
	{
		mApp->mSprites[IMAGE_TITLE]->Draw(g, 0, 0, false, 0, 0, false);
		double aProgress = (double)mApp->mCompletedLoadingThreadTasks / (double)mApp->mNumLoadingThreadTasks;
		int aDrop = mFade / 2;
		int y = aDrop + BAR_Y;

		// the glow behind the bar pulses
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		int aGlow = (int)(cos((double)(mUpdateCnt % GLOW_PERIOD) * 0.04) * 64.0 + 191.0);
		g->SetColor(Color(aGlow, aGlow, aGlow));
		mApp->mSprites[IMAGE_LOADINGGLOW]->Draw(g, 320, y, true, 0, 0, false);
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

		// the bar fills with the loading, then its inside and slider slide off to the right
		if (mBarSlide < BAR_TRAVEL)
		{
			mApp->mSprites[IMAGE_LOADINGINSIDE]->Draw(g, 320, y, true, 0, 0, false);
			g->mClipRect = Rect(BAR_CLIP_X, 0, BAR_TRAVEL, 480);
			mApp->mSprites[IMAGE_LOADINGBULLET]->Draw(g, (int)(624.0 - aProgress * 304.0), aDrop + BAR_Y + 1, true,
				0, 0, false);
		}
		if (mBarSlide < 1)
			mApp->mSprites[IMAGE_LOADINGSLIDER]->Draw(g, (int)(320.0 - aProgress * 304.0), y, true, 0, 0, false);
		else
		{
			g->mClipRect = Rect(BAR_CLIP_X, 0, BAR_TRAVEL, 480);
			mApp->mSprites[IMAGE_LOADINGSLIDER]->Draw(g, mBarSlide + 16, y, true, 0, 0, false);
		}
		g->mClipRect = Rect(0, 0, 640, 480);
		mApp->mSprites[IMAGE_LOADINGFRAME]->Draw(g, 320, y, true, 0, 0, false);
		if (mFade > 0)
		{
			g->SetColor(Color(0xff, 0xff, 0xff, mFade));
			g->FillRect(0, 0, 640, 480);
		}
	}

	// the logo fades in, its glow brightening and dimming again
	if (mLogoAngle > 0.0)
	{
		g->SetColorizeImages(true);
		int anAlpha = (int)(sin(mLogoAngle * 0.5) * 512.0);
		if (anAlpha > 255)
			anAlpha = 255;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		mApp->mSprites[IMAGE_ATOMICTANK]->Draw(g, LOGO_X, LOGO_Y, true, 0, 0, false);
		int aGlow = (int)(sin(mLogoAngle) * 255.0);
		g->SetColor(Color(aGlow, aGlow, aGlow));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		mApp->mSprites[IMAGE_ATOMICTANK]->Draw(g, LOGO_X, LOGO_Y, true, 1, 0, false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}
}

// 0x42ea90 slot 19
void TitleScreen::GotFocus()
{
	if (!mIsOver || !mMouseVisible)
	{
		mHasFocus = true;
		mMouseVisible = true;
	}
}

// 0x42ea60 slot 28: a click with the focus skips the intro (a first click without it only takes it)
void TitleScreen::MouseDown(int x, int y, int theClickCount)
{
	if (mFlashWidget != NULL)
	{
		if (mHasFocus)
			FlashAnimEnded(0);
		mHasFocus = true;
	}
}

// 0x42eab0 (Flash listener slot 0): the intro is over: the flash, the love theme and the loading start
void TitleScreen::FlashAnimEnded(int theId)
{
	mApp->mWidgetManager->RemoveWidget(mFlashWidget);
	mApp->SafeDeleteWidget(mFlashWidget);
	mFlashWidget = NULL;
	mFade--;
	mApp->mMusicInterface->FadeOutAll(true, MUSIC_FADE_OUT_SPEED);
	if (!mApp->mMusicInterface->IsPlaying(SONG_LOVE_THEME))
		mApp->mMusicInterface->PlayMusic(SONG_LOVE_THEME, -1, true);
	mApp->StartLoadingThread();
}

// 0x42e8e0 (listener slot 4)
void TitleScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x42e900 (listener slot 5): "click here to play". The original's other cases answered the trial's boxes (register,
// quit, not now) and are left out with them.
void TitleScreen::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	if (theId == BUTTON_PLAY)
	{
		mApp->ShowMainMenu();
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
	}
}
