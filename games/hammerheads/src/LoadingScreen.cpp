#include "LoadingScreen.h"

#include <cmath>

#include "Application.h"
#include "Color.h"
#include "Creature.h"
#include "DrawUtil.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "Image.h"
#include "Res.h"
#include "ResourceManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The id of the play button
const int BUTTON_PLAY = 1;

// How fast each stage runs (the part of it that one unit of frame time covers; the distributor's also has its property)
const double DISTRIBUTOR_RATE = 0.01;
const double LOGO_RATE = 0.005;
const double TITLE_RATE = 0.006600000000000001;
const double BAR_FADE_RATE = 0.008199999999999999;

// The play button covers the screen: it starts a bit above it and is taller than it
const int PLAY_BUTTON_Y = -30;
const int PLAY_BUTTON_WIDTH = 800;
const int PLAY_BUTTON_HEIGHT = 0x4b4;

// How far the backdrop goes up as the title comes in and how far the bottom picture comes up (each as the part of the bottom picture's
// height and of the screen's)
const double BACKDROP_RISE = 0.6720000000000002;
const double BOTTOM_RISE = 0.6400000000000001;

// The bar and "click to play" rest at five sixths of the screen's height; the bar slides this far down to leave, and "click to play"
// comes from this far down
const int BAR_SLIDE = 300;

// The title is drawn this far above its picture's place, and falls in from this much further above
const int TITLE_RAISE = 0x13;
const double TITLE_DROP = -450.0;

// The hammer is this far from the left of the screen (its picture's middle is that of the screen) and this far above its place
const int HAMMER_X = 0x146;
const int HAMMER_RAISE = 0xf;

// The glow of the title is a cosine that starts at its low point (a hundred times pi from the top)
const double GLOW_PHASE = 314.1592653589793;

// 0x458140: the wipe is over, the menu comes
void LoadingWipeDone()
{
	gApp->LoadingScreenDone();
}

}

// 0x458ec0
LoadingScreen::LoadingScreen()
{
	mStageProgress = 0.0;
	mBarFade = 0.0;
	mTime = 0.0;
	mTimeAtTitle = 0.0;
	mPlayButton = nullptr;
	mCobranded = gApp->GetBoolean("Cobranded", false);
	mStage = mCobranded ? STAGE_DISTRIBUTOR_LOGO : STAGE_PUBLISHER_LOGO;
	mDistributorSpeed = gApp->GetDouble("DistributorAnimSpeed", 0.77);
}

// 0x459040: the screen's own pictures go with it
LoadingScreen::~LoadingScreen()
{
	delete mPlayButton;
	gApp->mResourceManager->DeleteResources("LoadingScreen");
}

// 0x459120
void LoadingScreen::Begin()
{
	mPlayButton = new HyperlinkWidget(BUTTON_PLAY, this);
	mPlayButton->SetFont(FONT_ARIAL10B);
	mPlayButton->mLabel = "";
	mPlayButton->mColor = Color(0, 0, 0);
	mPlayButton->mOverColor = Color(0, 0, 255);
	mPlayButton->mUnderlineSize = 1;
}

// 0x458b70: the button starts to work (it is not seen: its label is empty)
void LoadingScreen::LoadingFinished()
{
	mPlayButton->SetVisible(true);
	mPlayButton->SetDisabled(false);
	gApp->PlaySample(SOUND_UI_CLICK_TO_PLAY);
}

// 0x458150: the button is there from the start, but hidden and off
void LoadingScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mPlayButton->SetVisible(false);
	mPlayButton->SetDisabled(true);
	mPlayButton->Resize(mWidth / 2 - PLAY_BUTTON_WIDTH / 2, PLAY_BUTTON_Y, PLAY_BUTTON_WIDTH, PLAY_BUTTON_HEIGHT);
	mPlayButton->mDoFinger = true;
	theWidgetManager->AddWidget(mPlayButton);
}

// 0x4581d0
void LoadingScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayButton);
}

// 0x458c30: the stages run one after the other, each from 0 to 1; once the loading is done the bar fades
void LoadingScreen::UpdateF(float theFrac)
{
	mTime = theFrac + mTime;

	if (mCobranded)
	{
		if (mStage == STAGE_DISTRIBUTOR_LOGO)
		{
			double aProgress = theFrac * mDistributorSpeed * DISTRIBUTOR_RATE + mStageProgress;
			if (1.0 < aProgress)
				aProgress = 1.0;
			mStageProgress = aProgress;
			if (aProgress == 1.0)
			{
				mStage = STAGE_PUBLISHER_LOGO;
				mStageProgress = 0.0;
			}
		}
	}

	if (mStage == STAGE_PUBLISHER_LOGO)
	{
		double aProgress = theFrac * LOGO_RATE + mStageProgress;
		if (1.0 < aProgress)
			aProgress = 1.0;
		mStageProgress = aProgress;
		if (aProgress == 1.0)
		{
			mStage = STAGE_GAME_LOGO;
			mStageProgress = 0.0;
		}
	}

	if (mStage == STAGE_GAME_LOGO)
	{
		double aProgress = theFrac * LOGO_RATE + mStageProgress;
		if (1.0 < aProgress)
			aProgress = 1.0;
		mStageProgress = aProgress;
		if (aProgress == 1.0)
		{
			mStage = STAGE_BEFORE_TITLE;
			mStageProgress = 0.0;
		}
	}

	if (mStage == STAGE_BEFORE_TITLE)
	{
		mStage = STAGE_TITLE;
		mStageProgress = 0.0;
	}
	else if (mStage == STAGE_TITLE)
	{
		double aProgress = theFrac * TITLE_RATE + mStageProgress;
		if (1.0 < aProgress)
			aProgress = 1.0;
		mStageProgress = aProgress;
		if (aProgress == 1.0)
		{
			mTimeAtTitle = mTime;
			mStage = STAGE_LOADING;
			mStageProgress = 0.0;
		}
	}

	if (gApp->GetLoadingThreadProgress() == 1.0)
	{
		double aFade = theFrac * BAR_FADE_RATE + mBarFade;
		if (1.0 < aFade)
			aFade = 1.0;
		mBarFade = aFade;
	}

	MarkDirty();
}

// 0x458aa0
void LoadingScreen::Draw(Graphics* g)
{
	DrawBackdrop(g, mStageProgress);
	DrawTitleAndHammer(g, mStageProgress);
	DrawBottom(g, mStageProgress);
	DrawLoadingBar(g, mBarFade);
	DrawClickToPlay(g, mBarFade);
	DrawDistributorLogo(g, mStageProgress);
	DrawPublisherLogo(g, mStageProgress);
	DrawGameLogo(g, mStageProgress);
}

// 0x458c00
void LoadingScreen::MouseDown(int x, int y, int theClickCount)
{
	MarkDirty();
}

// 0x458bb0: the click plays its sound and the wipe to the menu starts
void LoadingScreen::ButtonDepress(int theId)
{
	gApp->PlaySample(SOUND_UI_BUTTON_CLICK);
	if (theId == BUTTON_PLAY)
		gApp->DoZoomWipe(LoadingWipeDone, WIPE_HAMMER);
}

// 0x458250: the picture behind everything goes up out of the screen as the title comes in
void LoadingScreen::DrawBackdrop(Graphics* g, double theProgress)
{
	double aValue;
	if (mStage < STAGE_TITLE)
		aValue = 0.0;
	else if (mStage > STAGE_TITLE)
		aValue = 1.0;
	else
		aValue = theProgress;

	int aY = -ToInt(Ease(aValue, 0.0, BACKDROP_RISE) * IMAGE_LOADING_BOTTOM->mHeight);
	g->DrawImage(IMAGE_LOADING_BACKDROP, 0, aY);
}

// 0x4582c0: the bar shows how much of the loading is done; when it is all done, it slides down off the screen
void LoadingScreen::DrawLoadingBar(Graphics* g, double theFade)
{
	if (gApp->GetLoadingThreadProgress() < 1.0)
		theFade = 0.0;
	double aProgress = gApp->GetLoadingThreadProgress();

	int aX = mWidth / 2 - IMAGE_LOADING_BAR->mWidth / 2;
	int aY = ToInt(Ease(theFade, 0.5, 0.5) * BAR_SLIDE + (double)(mHeight * 5 / 6));
	DrawFillBar(g, IMAGE_LOADING_BAR_EMPTY, IMAGE_LOADING_BAR, aX, aY, aProgress, 0xc, 0x10);
}

// 0x4583a0: once everything is loaded the picture turns in from below (a press of the button moves it two pixels)
void LoadingScreen::DrawClickToPlay(Graphics* g, double theFade)
{
	if (gApp->GetLoadingThreadProgress() != 1.0)
		return;

	int aPressed = mPlayButton->IsButtonDown() ? 2 : 0;
	double aTurn = (1.0 - Ease(theFade, 0.5, 0.5)) * 47.12388980384689;
	int aY = ToInt((double)(mHeight * 5 / 6 + BAR_SLIDE) - Ease(theFade, 0.5, 0.5) * BAR_SLIDE);
	g->DrawImageRotated(IMAGE_LOADING_CLICKTOPLAY, (mWidth - IMAGE_LOADING_CLICKTOPLAY->mWidth) / 2 + aPressed, aY + aPressed,
		aTurn);
}

// 0x4584a0: the distributor's logo is on from the start and fades out
void LoadingScreen::DrawDistributorLogo(Graphics* g, double theProgress)
{
	if (mStage != STAGE_DISTRIBUTOR_LOGO)
		return;

	double aPhase = theProgress * 4.0 - 2.0;
	if (1.0 <= aPhase)
	{
		int anAlpha = ToInt((1.0 - (fabs(aPhase) - 1.0)) * 255.0);
		if (anAlpha < 0xff)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		}
	}
	g->DrawImage(IMAGE_LOADING_DISTRIBUTOR_LOGO, 0, 0);
	g->SetColorizeImages(false);
}

// 0x458570: the publisher's logo fades in, stays, and fades out
void LoadingScreen::DrawPublisherLogo(Graphics* g, double theProgress)
{
	if (mStage != STAGE_PUBLISHER_LOGO)
		return;

	double aPhase = theProgress * 4.0 - 2.0;
	if (aPhase <= -1.0 || 1.0 <= aPhase)
	{
		int anAlpha = ToInt((1.0 - (fabs(aPhase) - 1.0)) * 255.0);
		if (anAlpha < 0xff)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		}
	}
	g->DrawImage(IMAGE_LOADING_PUBLISHER_LOGO, 0, 0);
	g->SetColorizeImages(false);
}

// 0x458640: the game's own logo, the same way
void LoadingScreen::DrawGameLogo(Graphics* g, double theProgress)
{
	if (mStage != STAGE_GAME_LOGO)
		return;

	double aPhase = theProgress * 4.0 - 2.0;
	if (aPhase <= -1.0 || 1.0 <= aPhase)
	{
		int anAlpha = ToInt((1.0 - (fabs(aPhase) - 1.0)) * 255.0);
		if (anAlpha < 0xff)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		}
	}
	g->DrawImage(IMAGE_LOADING_LOGO, 0, 0);
	g->SetColorizeImages(false);
}

// 0x458710: the title falls in from above (fading in) and the hammer under it swings and swells; once the title is in, a glow of
// itself pulses over it (where the screen is 3D accelerated: there the hammer is also drawn scaled)
void LoadingScreen::DrawTitleAndHammer(Graphics* g, double theProgress)
{
	if (mStage <= STAGE_BEFORE_TITLE)
		return;

	int aDrop = 0;
	if (mStage > STAGE_TITLE)
		theProgress = 1.0;
	else if (theProgress < 1.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, ToInt(theProgress * 255.0)));
		aDrop = ToInt(Ease(1.0 - theProgress, 0.2, 0.8) * TITLE_DROP);
	}

	if (gApp->Is3DAccelerated())
	{
		double aPulse = (cos(mTime * 0.05) + 1.0) * 0.5;
		g->DrawImage(IMAGE_LOADING_TITLE, 0, aDrop - TITLE_RAISE);

		if (theProgress == 1.0)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, ToInt((cos((mTime - mTimeAtTitle + GLOW_PHASE) * 0.01) + 1.0) * 100.0)));
			g->DrawImage(IMAGE_LOADING_TITLE, 0, aDrop - TITLE_RAISE);
			g->SetColorizeImages(false);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}

		double aScale = aPulse * 0.15 + 1.0;
		int aWidth = ToInt(IMAGE_LOADING_HAMMER->mWidth * aScale);
		int aHeight = ToInt(IMAGE_LOADING_HAMMER->mHeight * aScale);
		g->SetColorizeImages(false);
		int aBob = ToInt(cos(mTime * 0.11) * -10.0);
		int aSway = ToInt(sin(mTime * 0.05) * -5.0);
		int aX = IMAGE_LOADING_HAMMER->mWidth / 2 - aWidth / 2 - aSway + HAMMER_X;
		int aY = IMAGE_LOADING_HAMMER->mHeight / 2 - aHeight / 2 - aBob + aDrop - HAMMER_RAISE;
		g->DrawImage(IMAGE_LOADING_HAMMER, aX, aY, aWidth, aHeight);
		g->SetColorizeImages(false);
	}
	else
	{
		g->SetColorizeImages(false);
		g->DrawImage(IMAGE_LOADING_TITLE, 0, aDrop - TITLE_RAISE);
		float aX = (float)(sin(mTime * 0.05) * 5.0 + 326.0);
		float aY = (float)(cos(mTime * 0.11) * 10.0 + (double)aDrop + 363.0 - 378.0);
		g->DrawImageF(IMAGE_LOADING_HAMMER, aX, aY);
		g->SetColorizeImages(false);
	}
}

// 0x458a10: the picture on the bottom comes up (from a quarter of the title's coming in)
void LoadingScreen::DrawBottom(Graphics* g, double theProgress)
{
	if (mStage < STAGE_TITLE)
		return;
	if (mStage > STAGE_TITLE)
		theProgress = 1.0;
	else if (theProgress <= 0.25)
		return;

	int aY = ToInt((double)mHeight -
		Ease((theProgress - 0.25) * 1.3333333333333333, 0.0, BOTTOM_RISE) * IMAGE_LOADING_BOTTOM->mHeight);
	g->DrawImage(IMAGE_LOADING_BOTTOM, 0, aY);
}
