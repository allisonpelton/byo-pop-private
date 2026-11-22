// LoadingScreen.cpp: see LoadingScreen.h.
//
// The original keeps every tweakable number of this screen in a call that returns the number it was compiled with (ModValInt,
// ModValFloat: a table the programmers edited while the game ran). They are named constants here, each with the line of the
// original's source its call names.

#include "LoadingScreen.h"

#include "Compat2004.h"
#include "Font.h"
#include "InterpVal.h"
#include "GameMode.h"
#include "Graphics.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SexyMatrix.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "ThunderButton.h"
#include "ThunderballApp.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdlib>
#include <string>

using namespace Sexy;

// The original's constants are floats widened to double (pi is 3.1415927410125732, not 3.141592653589793).
static const double PI_AS_FLOAT = 3.1415927410125732;
static const double HALF_PI_AS_FLOAT = 1.5707963705062866;
static const double ONE_HUNDREDTH_AS_FLOAT = 0.009999999776482582;

// ---- timing (the update count is the widget's, 100 an update a second) ----
static const int LOGO_SPLASH_FADE_IN_END = 50;	 // the PopCap logo is full at this update
static const int LOGO_SPLASH_FADE_OUT_BEGIN = 150; // and starts to fade out
static const int LOGO_SPLASH_END = 200;			 // and is gone (black follows until the fade-in of the title at 300)
static const int BLACK_FADE_END = 300;			 // line 431: the black cover is clear
static const int BLACK_FADE_LENGTH = 100;		 // the cover fades over these updates after LOGO_SPLASH_END
static const int LOAD_START_UPDATE = 250;		 // line 396, 398: the bar may start to fill
static const int BAR_FULL_TIME_ONE = 550;		 // line 395, 402: after this update the screen redraws only when something moves
static const int PLAY_FLASH_LENGTH = 50;		 // the Play button flashes this many updates after it appears
static const int FLASH_PERIOD_HALF = 25;
static const int SOUND_UPDATE = 1;				 // the update the morning sound starts at
static const int YIELD_UPDATE = 0x50;			 // line: the main thread yields from here (80)

static const float LOAD_RATE = 0.001f;			 // line 397: bar share per update after LOAD_START_UPDATE (the time cap)
static const float NOT_DONE_PROGRESS = 0.9999f;	 // the bar cannot be full before the loading thread says it is done
static const int NUM_LIGHTS = 25;				 // the lights of the bar

// ---- the sun and the logo ----
static const int SUN_RISE_HEIGHT = 500;			 // line 435: how far the sun moves up as the bar fills
static const double SUN_BOTTOM_OFFSET = 214.0;	 // (a constant of the code, not a tweak)
static const int SUN_X = -0x24;
static const int SUN_ROTATION_CENTER_X = 394;	 // line 436
static const int SUN_ROTATION_CENTER_Y = 383;	 // line 437
static const int SUN_HEAD_X = 283;				 // line 438
static const int SUN_HEAD_Y = 307;				 // line 439 (below the sun's top)
static const int LOGO_X = 170;					 // line 440
static const int LOGO_REST_Y = 140;				 // line 441
static const int LOGO_START_OFFSET = 200;		 // line 442: the logo starts this far below LOGO_REST_Y
static const int LOGO_DIP = -60;				 // line 443: the fall passes this far above LOGO_REST_Y
static const int LOGO_DIP_UPDATE = 50;			 // line 444: when it reaches the top
static const int LOGO_REST_UPDATE = 70;			 // line 445: when it comes to rest
static const int BOUNCE_LENGTH = 10;			 // line 446: how long the logo bounces after the ball hits
static const int BOUNCE_HEIGHT = 8;				 // line 447
static const float BOUNCE_DECAY = 1.5f;			 // line 448
static const int HILL_Y = 0x11d;
static const int SHADOW_ALPHA_MAX = 120;		 // line 450
static const int SHADOW_COLOR = 0x101010;		 // line 449
static const int SHADOW_X = 162;				 // line 451
static const int SHADOW_Y = 290;				 // line 452
static const int OVERLAY_X = 201;				 // line 453
static const int OVERLAY_Y = 420;				 // line 454
static const int FLASH_BEGIN = 5;				 // line 455: the logo's flash after the hit
static const int FLASH_END = 25;				 // line 456
static const int FLASH_ALPHA = 128;				 // line 457
static const int LOADING_TEXT_Y = 555;			 // line 462
static const int LOADING_TEXT_COLOR = 0xffec91;	 // line 460
static const int DELUXE_X = 300;				 // line 463
static const int DELUXE_Y = 334;				 // line 464
static const int TM_X = 574;					 // line 465
static const int TM_Y = 310;					 // line 466

// ---- the lights ----
static const float LIGHT_SPACING = 13.9f;		 // line 427
static const int LIGHT_X = 224;					 // line 426
static const int LIGHT_Y = 447;					 // line 428

// ---- the Play button ----
static const int PLAY_BUTTON_X = 228;			 // line 383
static const int PLAY_BUTTON_Y = 446;			 // line 384
static const int PLAY_FLASH_TINT = 100;			 // line 429

// ---- the ball and its sparks ----
static const int BALL_START_X = 205;			 // line 385
static const int BALL_START_Y = -20;			 // line 386
static const float BALL_START_VX = 4.5f;		 // line 387
static const float BALL_GRAVITY = 0.2f;			 // line 388
static const int BALL_FLOOR_X = 840;			 // line 389: the ball is gone when it passes this
static const int BALL_HIT_Y = 170;				 // line 390: the ball hits the logo below this
static const float BALL_HIT_VY_FACTOR = 0.75f;	 // line 391 (it is reversed)
static const float BALL_HIT_VX_FACTOR = 1.0f;	 // line 392
static const int BALL_GONE_MARGIN = 20;			 // line 393
static const int BOUNCE_COUNT_LIMIT = 30;		 // line 394: the logo's bounce count restarts after this
static const int SPARK_FIRST = -7;				 // line 403
static const int SPARK_LAST = 7;				 // line 404
static const float SPARK_SPACING = 4.3f;		 // line 405, 408
static const int SPARK_RANDOM_RANGE = 30;		 // line 406, 409
static const int SPARK_OFFSET = 15;				 // line 407, 410
static const float SPARK_DRIFT = -0.071f;		 // line 411, 412
static const int SPARK_LIFE = 255;				 // line 413
static const int SPARK_FADE = 6;				 // line 414
static const float SPARK_PUSH = 3.0f;			 // line 415, 416
static const int SPARK_COLOR_RANGE = 128;		 // line 419 to 421
static const int SPARK_GLOW_COLOR = 0x20201;
static const int SPARK_GLOW_SIZE = 8;			 // line 424, 425
static const int SPARK_GLOW_OFFSET = 4;			 // line 422, 423
static const float SUN_SPIN_SPEED = 0.2f;		 // line 401

// ---------------------------------------------------------------------------------------------------------------------

// 0x4936b0: the table of the share of the load done after each of the loading thread's tasks (the original's switch).
float Sexy::GetLoadTime(int theCompletedTasks)
{
	static const float aLoadTimes[] = {
		0.0f, 0.007f, 0.013f, 0.018f, 0.022f, 0.023f, 0.417f, 0.418f,
		0.425f, 0.431f, 0.432f, 0.436f, 0.443f, 0.454f, 0.454f, 0.455f,
		0.455f, 0.455f, 0.455f, 0.455f, 0.455f, 0.455f, 0.461f, 0.461f,
		0.461f, 0.461f, 0.461f, 0.461f, 0.461f, 0.463f, 0.464f, 0.464f,
		0.464f, 0.464f, 0.465f, 0.465f, 0.465f, 0.465f, 0.465f, 0.465f,
		0.465f, 0.466f, 0.466f, 0.466f, 0.466f, 0.466f, 0.468f, 0.468f,
		0.468f, 0.468f, 0.468f, 0.469f, 0.469f, 0.469f, 0.47f, 0.472f,
		0.472f, 0.472f, 0.472f, 0.472f, 0.472f, 0.473f, 0.473f, 0.473f,
		0.474f, 0.474f, 0.475f, 0.476f, 0.477f, 0.477f, 0.478f, 0.478f,
		0.478f, 0.478f, 0.478f, 0.479f, 0.479f, 0.479f, 0.481f, 0.482f,
		0.482f, 0.482f, 0.484f, 0.484f, 0.485f, 0.485f, 0.486f, 0.486f,
		0.487f, 0.489f, 0.489f, 0.49f, 0.49f, 0.49f, 0.49f, 0.49f,
		0.49f, 0.49f, 0.491f, 0.492f, 0.492f, 0.492f, 0.492f, 0.493f,
		0.493f, 0.494f, 0.494f, 0.494f, 0.494f, 0.494f, 0.494f, 0.494f,
		0.494f, 0.495f, 0.495f, 0.495f, 0.495f, 0.495f, 0.497f, 0.497f,
		0.497f, 0.498f, 0.498f, 0.498f, 0.498f, 0.498f, 0.498f, 0.498f,
		0.498f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.501f,
		0.501f, 0.501f, 0.501f, 0.503f, 0.503f, 0.504f, 0.504f, 0.508f,
		0.511f, 0.513f, 0.513f, 0.513f, 0.513f, 0.514f, 0.514f, 0.516f,
		0.517f, 0.517f, 0.517f, 0.518f, 0.518f, 0.519f, 0.52f, 0.521f,
		0.522f, 0.522f, 0.533f, 0.534f, 0.544f, 0.544f, 0.544f, 0.546f,
		0.546f, 0.547f, 0.547f, 0.552f, 0.555f, 0.557f, 0.561f, 0.574f,
		0.59f, 0.593f, 0.597f, 0.597f, 0.6f, 0.6f, 0.6f, 0.6f,
		0.6f, 0.6f, 0.6f, 0.6f, 0.601f, 0.601f, 0.601f, 0.602f,
		0.622f, 0.622f, 0.63f, 0.63f, 0.633f, 0.635f, 0.641f, 0.648f,
		0.652f, 0.652f, 0.654f, 0.654f, 0.654f, 0.665f, 0.666f, 0.667f,
		0.675f, 0.676f, 0.679f, 0.682f, 0.683f, 0.684f, 0.71f, 0.71f,
		0.711f, 0.714f, 0.715f, 0.715f, 0.715f, 0.717f, 0.718f, 0.718f,
		0.72f, 0.733f, 0.733f, 0.746f, 0.748f, 0.749f, 0.75f, 0.75f,
		0.75f, 0.752f, 0.753f, 0.753f, 0.753f, 0.753f, 0.753f, 0.754f,
		0.754f, 0.754f, 0.755f, 0.755f, 0.755f, 0.756f, 0.756f, 0.766f,
		0.767f, 0.767f, 0.767f, 0.767f, 0.767f, 0.767f, 0.767f, 0.768f,
		0.768f, 0.768f, 0.77f, 0.77f, 0.77f, 0.771f, 0.773f, 0.779f,
		0.782f, 0.785f, 0.786f, 0.786f, 0.789f, 0.789f, 0.791f, 0.793f,
		0.794f, 0.801f, 0.807f, 0.808f, 0.813f, 0.815f, 0.818f, 0.821f,
		0.821f, 0.822f, 0.824f, 0.833f, 0.836f, 0.837f, 0.837f, 0.837f,
		0.837f, 0.839f, 0.841f, 0.842f, 0.842f, 0.842f, 0.843f, 0.843f,
		0.843f, 0.843f, 0.843f, 0.843f, 0.846f, 0.846f, 0.846f, 0.846f,
		0.846f, 0.846f, 0.847f, 0.847f, 0.847f, 0.849f, 0.849f, 0.849f,
		0.852f, 0.852f, 0.853f, 0.854f, 0.856f, 0.856f, 0.856f, 0.856f,
		0.856f, 0.857f, 0.857f, 0.858f, 0.859f, 0.86f, 0.86f, 0.861f,
		0.863f, 0.863f, 0.863f, 0.865f, 0.867f, 0.868f, 0.871f, 0.872f,
		0.874f, 0.876f, 0.878f, 0.881f, 0.882f, 0.885f, 0.886f, 0.886f,
		0.886f, 0.89f, 0.891f, 0.892f, 0.892f, 0.893f, 0.894f, 0.896f,
		0.898f, 0.898f, 0.898f, 0.905f, 0.914f, 0.92f, 0.929f, 0.936f,
		0.944f, 0.951f, 0.958f, 0.966f, 0.974f, 0.978f, 0.978f, 0.982f,
		0.984f, 0.985f, 0.987f, 0.988f, 0.989f, 0.991f, 0.992f, 0.993f,
		0.994f, 0.996f, 0.997f, 0.998f, 0.998f,
	};
	if (theCompletedTasks >= 0 && theCompletedTasks < (int)(sizeof(aLoadTimes) / sizeof(aLoadTimes[0])))
		return aLoadTimes[theCompletedTasks];
	return 1.0f;
}

// 0x4b1fe0
LoadingScreen::LoadingScreen(ThunderballApp* theApp)
{
	mApp = theApp;
	mLoadProgress = 0;
	mSunAngle = 0;
	mSunSpinCount = 0;
	mPlayFlashCount = 0;
	mDrawnProgress = 0;
	mStartAdventureCountdown = 0;
	mBallState = 0;
	mBallHitCount = 0;
	mBallX = 0;
	mBallY = 0;
	mBallVX = 0;
	mBallVY = 0;
	mLoaded = false;
	mFrame = 0;
	mNumLights = 0;

	// the Play button is a picture of two rows, the plain one above the one with the mouse over it
	mPlayButton = new ThunderButton(IMAGE_LOAD_PLAYNOW, 0, this);
	mPlayButton->mButtonImage = IMAGE_LOAD_PLAYNOW;
	mPlayButton->mComponentImage = nullptr;
	int aWidth = IMAGE_LOAD_PLAYNOW->GetWidth();
	int aHeight = IMAGE_LOAD_PLAYNOW->GetHeight() / 2;
	mPlayButton->mWidth = aWidth;
	mPlayButton->mHeight = aHeight;
	mPlayButton->mNormalRect = Rect(0, 0, aWidth, aHeight);
	mPlayButton->mOverRect = Rect(0, aHeight, aWidth, aHeight);
	mPlayButton->Move(PLAY_BUTTON_X, PLAY_BUTTON_Y);
	mPlayButton->SetVisible(false);
	AddWidget(mPlayButton);
}

// 0x4af650 (0x4a6e70)
LoadingScreen::~LoadingScreen()
{
	RemoveAllWidgets(true, false);
	mApp->mResourceManager->DeleteResources("Load"); // the screen's pictures go with it
	mApp->mYieldMainThread = false;
}

// 0x494370
void LoadingScreen::DoBallBounce()
{
	mApp->ClearUpdateBacklog(false);
	mApp->mYieldMainThread = false;
	mBallState = 1;
	mBallX = (float)BALL_START_X;
	mBallY = (float)BALL_START_Y;
	mBallVX = BALL_START_VX;
	mBallVY = 0;
	mBallHitCount = 0;
}

// 0x494420: light 0 and 24 are the ends of the row, 1 and 23 next to them
void LoadingScreen::DrawLight(Graphics* g, int theLight)
{
	int aCel;
	switch (theLight)
	{
	case 0:
		aCel = 4;
		break;
	case 1:
		aCel = 3;
		break;
	default:
		aCel = 2;
		break;
	case 0x17:
		aCel = 1;
		break;
	case 0x18:
		aCel = 0;
	}
	Rect aSrc = IMAGE_LOAD_LIGHTS->GetCelRect(aCel);
	double anOffset = (double)(LIGHT_SPACING * theLight);
	int aX = (int)((double)LIGHT_X + anOffset);
	g->DrawImage(IMAGE_LOAD_LIGHTS, aX, LIGHT_Y, aSrc);
}

// 0x494510: drawn over everything: the screen's frame, the Play button flash, the black cover and the PopCap logo
void LoadingScreen::DrawOverlay(Graphics* g)
{
	DrawScreenFrame(g);

	// the Play button glows for its first 50 updates: up for 25, then down
	if (mPlayFlashCount > 0 && mPlayFlashCount < PLAY_FLASH_LENGTH)
	{
		float aStep = (float)(mPlayFlashCount < FLASH_PERIOD_HALF ? mPlayFlashCount : PLAY_FLASH_LENGTH - mPlayFlashCount) / 25.0;
		float aTint = (float)((double)PLAY_FLASH_TINT * aStep);
		Graphics aButtonGraphics(*g);
		aButtonGraphics.SetColorizeImages(true);
		aButtonGraphics.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aValue = (int)aTint;
		aButtonGraphics.SetColor(Color(aValue, aValue, aValue));
		aButtonGraphics.Translate(mPlayButton->mX, mPlayButton->mY);
		mPlayButton->Draw(&aButtonGraphics);
	}

	// the black cover: opaque until update 200, then it fades out over the next 100
	if (mUpdateCnt < BLACK_FADE_END)
	{
		int anAlpha = 255;
		if (200 < mUpdateCnt)
			anAlpha = ((BLACK_FADE_END - mUpdateCnt) * 255) / BLACK_FADE_LENGTH;
		g->SetColor(Color(0, anAlpha));
		g->FillRect(0, 0, mWidth, mHeight);
	}

	// the PopCap logo: in over the first 50 updates, out over the 50 before update 200
	if (mUpdateCnt < LOGO_SPLASH_END)
	{
		int anAlpha = 255;
		if (mUpdateCnt < LOGO_SPLASH_FADE_IN_END)
			anAlpha = (mUpdateCnt * 255) / 50;
		else if (LOGO_SPLASH_FADE_OUT_BEGIN < mUpdateCnt)
			anAlpha = ((LOGO_SPLASH_END - mUpdateCnt) * 255) / 50;
		g->SetColor(Color(0xffffff, anAlpha));
		g->SetColorizeImages(true);
		g->DrawImage(IMAGE_LOAD_POPCAP, (mWidth - IMAGE_LOAD_POPCAP->GetWidth()) / 2, (mHeight - IMAGE_LOAD_POPCAP->GetHeight()) / 2);
		g->SetColorizeImages(false);
	}
}

// 0x4998c0: the sparks flicker: each is a coloured dot, and now and then a glow
void LoadingScreen::DrawParticles(Graphics* g)
{
	g->PushState();
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (size_t i = 0; i < mParticles.size(); i++)
	{
		const Particle& aParticle = mParticles[i];
		int aBlue = CrtRand() % SPARK_COLOR_RANGE;
		int aGreen = CrtRand() % SPARK_COLOR_RANGE;
		int aRed = CrtRand() % SPARK_COLOR_RANGE;
		g->SetColor(Color(aRed, aGreen, aBlue, aParticle.mLife));
		int aHeight = CrtRand() % 2 + 1;
		int aWidth = CrtRand() % 2 + 1;
		g->FillRect((int)((double)aParticle.mX - 1.0), (int)((double)aParticle.mY - 1.0), aWidth, aHeight);

		if (CrtRand() % 0xff < aParticle.mLife)
		{
			g->SetColor(Color(SPARK_GLOW_COLOR));
			g->FillRect((int)((double)aParticle.mX - SPARK_GLOW_OFFSET), (int)((double)aParticle.mY - SPARK_GLOW_OFFSET), SPARK_GLOW_SIZE,
				SPARK_GLOW_SIZE);
		}
	}
	g->PopState();
}

// 0x49f2d0: the sparks fall back (a pull to the 0.25 mark of their own speed) and fade
void LoadingScreen::UpdateParticles()
{
	size_t i = 0;
	while (i < mParticles.size())
	{
		Particle& aParticle = mParticles[i];
		aParticle.mLife -= SPARK_FADE;
		if (aParticle.mLife < 1)
		{
			// the last spark takes its place
			mParticles[i] = mParticles[mParticles.size() - 1];
			if (!mParticles.empty())
				mParticles.pop_back();
		}
		else
		{
			float aSpeedSquared = aParticle.mVX * aParticle.mVX + aParticle.mVY * aParticle.mVY;
			float aSin = (float)sin((double)aParticle.mAngle);
			double aPull = 0.25 - (double)aSpeedSquared;
			aParticle.mY = (float)((double)SPARK_PUSH * aPull * aSin + aParticle.mVY + aParticle.mY);
			float aCos = (float)cos((double)aParticle.mAngle);
			aParticle.mX = (float)((double)SPARK_PUSH * aPull * aCos + aParticle.mVX + aParticle.mX);
			i++;
		}
	}
}

// 0x4b3b90: a row of sparks across the ball's path where it hit the logo
void LoadingScreen::SpawnParticles()
{
	for (int i = SPARK_FIRST; i <= SPARK_LAST; i++)
	{
		float aAngle = atan2f(mBallVY, mBallVX);
		double aRandomX = (double)(CrtRand() % SPARK_RANDOM_RANGE);
		double aSideAngle = (double)aAngle + HALF_PI_AS_FLOAT;
		double aCos = cos(aSideAngle);

		Particle aParticle;
		aParticle.mX = (float)((double)SPARK_SPACING * aCos * (double)i + (double)mBallX + aRandomX - (double)SPARK_OFFSET);
		double aRandomY = (double)(CrtRand() % SPARK_RANDOM_RANGE);
		double aSin = sin(aSideAngle);
		aParticle.mY = (float)((double)SPARK_SPACING * aSin * (double)i + (double)mBallY + aRandomY - (double)SPARK_OFFSET);
		aParticle.mVX = (float)((double)SPARK_DRIFT * aCos * (double)i);
		aParticle.mVY = (float)((double)SPARK_DRIFT * aSin * (double)i);
		aParticle.mLife = SPARK_LIFE;
		aParticle.mAngle = aAngle;
		mParticles.push_back(aParticle);
	}
}

// 0x4b40b0
void LoadingScreen::UpdateBall()
{
	if (mBallState > 0 && mBallState < 3)
	{
		float aVelocityY = (float)((double)BALL_GRAVITY + (double)mBallVY);
		mBallVY = aVelocityY;
		mBallX = mBallVX + mBallX;
		mBallY = aVelocityY + mBallY;
		if (mBallY <= (float)BALL_HIT_Y || mBallState != 1)
		{
			if ((float)BALL_FLOOR_X < mBallX && (float)(BALL_GONE_MARGIN + BALL_FLOOR_X) < mBallX)
			{
				mApp->mYieldMainThread = !mApp->mLoadingThreadStarted;
				mBallState = 3;
			}
		}
		else
		{
			// the ball hits the logo: it bounces up and the logo shakes
			mBallHitCount = 1;
			mBallState = 2;
			mBallVY = (float)(-(double)BALL_HIT_VY_FACTOR * (double)mBallVY);
			mBallVX = (float)((double)BALL_HIT_VX_FACTOR * (double)mBallVX);
			if (SOUND_PEGHIT_LOW >= 0)
				mApp->PlaySample(SOUND_PEGHIT_LOW);
		}
		if (mBallX < (float)BALL_FLOOR_X)
			SpawnParticles();
		MarkDirty();
	}
}

// 0x4b4270
void LoadingScreen::Update()
{
	Widget::Update();
	float aLoad = GetLoadTime(mApp->mCompletedLoadingThreadTasks);
	if (mPlayFlashCount != 0)
		mPlayFlashCount++;

	if (mUpdateCnt == SOUND_UPDATE)
	{
		mApp->mYieldMainThread = false;
		mApp->ClearUpdateBacklog(false);
		if (SOUND_MORNING >= 0)
		{
			SoundInstance* aSound = mApp->mSoundManager->GetSoundInstance(SOUND_MORNING);
			if (aSound != nullptr)
				aSound->Play(false, true);
		}
	}
	else if (mUpdateCnt == YIELD_UPDATE && !mApp->mLoadingThreadStarted && mBallState == 0)
		mApp->mYieldMainThread = true;

	UpdateBall();
	UpdateParticles();

	// after the ball hit the logo it bounces for a moment
	if (mBallHitCount > 0)
	{
		mBallHitCount++;
		if (BOUNCE_COUNT_LIMIT < mBallHitCount)
			mBallHitCount = 0;
		MarkDirty();
	}

	// The bar waits for the PopCap logo to go by, and the level music loads in the meantime (the original does it in a thread of
	// its own and holds the bar at 99% until it is done; here it is loaded at once). The original has a second logo of a
	// distributor that holds the bar until update 550: not in this port.
	if (LOAD_START_UPDATE < mUpdateCnt)
	{
		if (0.0f < aLoad)
			mFrame++;
	}
	else
	{
		aLoad = 0.0f;
		if (1 < mUpdateCnt && mApp->mLevelMusicPending)
			mApp->LoadLevelMusic();
	}

	// the bar is full only when the loading thread says so
	if (mApp->mLoadingThreadCompleted)
		aLoad = 1.0f;
	else if (aLoad >= 1.0f)
		aLoad = NOT_DONE_PROGRESS;

	// and not faster than the time allows
	float aTimeLimit = (float)((double)LOAD_RATE * (double)(mUpdateCnt - LOAD_START_UPDATE));
	if (aTimeLimit < aLoad)
		aLoad = aTimeLimit;
	if (aLoad < 0.0f)
		aLoad = 0.0f;

	if (mLoadProgress < aLoad)
	{
		float aNext = (float)((double)mLoadProgress + ONE_HUNDREDTH_AS_FLOAT);
		mLoadProgress = aNext;
		if (aLoad < aNext)
			mLoadProgress = aLoad;
		mNumLights = (int)((double)mLoadProgress * (double)NUM_LIGHTS);
		MarkDirty();

		if (mLoadProgress >= 1.0f)
		{
			// everything is loaded: a screen saver or a replay goes on at once, the player gets the Play button
			mPlayFlashCount = 1;
			if (mApp->IsScreenSaver() || !mApp->mReplayFileName.empty())
			{
				if (!mApp->mReplayFileName.empty())
				{
					mApp->ShowReplay(false);
					return;
				}
				mApp->mGameMode = GAME_MODE_DEMO;
				mApp->ShowBoard(true, true);
				return;
			}
			mPlayButton->SetVisible(true);
			mLoaded = true;
		}
	}

	if (mDrawnProgress < mLoadProgress)
	{
		mDrawnProgress = mLoadProgress;
		MarkDirty();
	}
	if (mDrawnProgress >= 1.0f && 550 < mUpdateCnt)
	{
		mDrawnProgress = 1.0f;
		if (mBallState == 0)
			DoBallBounce();
	}

	// once it is all done the sun spins
	if (mDrawnProgress >= 1.0f)
	{
		mSunSpinCount++;
		mSunAngle = (float)((double)SUN_SPIN_SPEED + (double)mSunAngle);
		if (mApp->Is3DAccelerated())
			MarkDirty();
	}

	// a new player was named at the certificate: the adventure starts a little later
	if (mStartAdventureCountdown > 0)
	{
		mStartAdventureCountdown--;
		if (mStartAdventureCountdown == 0)
			mApp->StartAdventureGame();
	}

	// the screen draws every update while the logo is up and then only when something moves
	if (BAR_FULL_TIME_ONE < mUpdateCnt)
	{
		if (mParticles.empty())
			return;
	}
	MarkDirty();
}

// 0x4a7250
void LoadingScreen::Draw(Graphics* g)
{
	Widget::Draw(g);
	g->SetColor(Color(0));
	g->FillRect(0, 0, mWidth, mHeight);
	g->DrawImage(IMAGE_LOAD_BACK, 0, 0);

	// the sun rises as the load goes on; it is a turning glow with a face on top (without 3D it is only the glow)
	int aSunY = (int)((double)SUN_RISE_HEIGHT * (1.0 - (double)mDrawnProgress) - SUN_BOTTOM_OFFSET);
	if (g->Is3D() && mSunAngle != 0.0f)
	{
		double aRotation = (double)mSunAngle * PI_AS_FLOAT / 180.0;
		g->DrawImageRotated(IMAGE_SUNGLOW, SUN_X, aSunY, aRotation, SUN_ROTATION_CENTER_X, SUN_ROTATION_CENTER_Y);
		g->DrawImage(IMAGE_LOAD_SUNHEAD, SUN_HEAD_X, SUN_HEAD_Y + aSunY);
	}
	else
		g->DrawImage(IMAGE_SUNGLOW, SUN_X, aSunY);

	// The logo drops in on a parabola that starts LOGO_START_OFFSET below its place, is LOGO_DIP above it at update 50 and is in
	// place at update 70: y = quadratic * frame^2 + linear * frame + start, solved once from those two points.
	static bool isLogoCurveSolved = false;
	static float aLinear = 0;
	static float aQuadratic = 0;
	static float aStartY = 0;
	if (!isLogoCurveSolved)
	{
		aStartY = (float)(LOGO_START_OFFSET + LOGO_REST_Y);
		isLogoCurveSolved = true;
		float aDipFrame = (float)LOGO_DIP_UPDATE;
		float aRestFrame = (float)LOGO_REST_UPDATE;
		float aDipValue = (float)(LOGO_REST_Y + LOGO_DIP) - aStartY;
		float aRestValue = (float)LOGO_REST_Y - aStartY;
		// the two equations quadratic * f^2 + linear * f = value, solved by Cramer's rule (0x4942f0)
		float aDeterminant = aRestFrame * (aDipFrame * aDipFrame) - (aRestFrame * aRestFrame) * aDipFrame;
		if (aDeterminant != 0.0f)
		{
			aQuadratic = (aDipValue * aRestFrame - aRestValue * aDipFrame) / aDeterminant;
			aLinear = ((aDipFrame * aDipFrame) * aRestValue - aDipValue * (aRestFrame * aRestFrame)) / aDeterminant;
		}
	}
	float aFrame = (float)mFrame;
	float aLogoY = aQuadratic * aFrame * aFrame + aLinear * aFrame + aStartY;
	if (mFrame < LOGO_DIP_UPDATE)
		g->DrawImage(IMAGE_LOAD_LOGO, LOGO_X, (int)aLogoY);

	// when the ball hit it the logo bounces (it is pushed down by a decaying 8) and flashes
	int aBounce;
	if (mBallHitCount > 0 && mBallHitCount < BOUNCE_LENGTH)
		aBounce = (int)((double)BOUNCE_HEIGHT - (double)BOUNCE_DECAY * mBallHitCount);
	else
		aBounce = (int)0.0;
	int aLogoRestY = LOGO_REST_Y;
	if (mBallHitCount > 0 && mBallHitCount < BOUNCE_LENGTH)
		aLogoRestY += aBounce;

	g->DrawImage(IMAGE_LOAD_HILL, 0, HILL_Y);
	g->SetColorizeImages(true);
	int aShadowAlpha = (int)((double)SHADOW_ALPHA_MAX * mDrawnProgress * mDrawnProgress);
	g->SetColor(Color(SHADOW_COLOR, aShadowAlpha));
	g->DrawImage(IMAGE_LOAD_SHADOW, SHADOW_X, SHADOW_Y + aBounce);
	g->SetColorizeImages(false);
	g->DrawImage(IMAGE_LOAD_OVERLAY, OVERLAY_X, OVERLAY_Y);

	if (mFrame >= LOGO_DIP_UPDATE && mFrame < LOGO_REST_UPDATE)
		g->DrawImage(IMAGE_LOAD_LOGO, LOGO_X, (int)aLogoY);
	else if (mFrame >= LOGO_REST_UPDATE)
	{
		g->DrawImage(IMAGE_LOAD_LOGO, LOGO_X, aLogoRestY);
		if (mBallHitCount != 0)
		{
			g->SetColorizeImages(true);
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			float aFlash = InterpValF(mBallHitCount, FLASH_BEGIN, FLASH_END, 0.0f, 1.0f, 0.0f);
			g->SetColor(Color(0xffffff, (int)((double)FLASH_ALPHA * aFlash)));
			g->DrawImage(IMAGE_LOAD_LOGO, LOGO_X, aLogoRestY);
			g->SetColorizeImages(false);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	for (int i = 0; i < mNumLights; i++)
		DrawLight(g, i);

	// the line of text under the bar
	std::string aText;
	if (mApp->IsScreenSaver())
		aText = "Starting Screensaver";
	else if (!mApp->mReplayFileName.empty())
		aText = "Loading Replay...";
	else if (mLoadProgress < 1.0f)
		aText = "Loading...";
	if (!aText.empty())
	{
		g->SetFont(FONT_OVERLOAD16OUTLINE);
		g->SetColor(Color(LOADING_TEXT_COLOR));
		int aTextWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, (mWidth - aTextWidth) / 2, LOADING_TEXT_Y);
	}

	DrawParticles(g);

	if (mBallState > 0)
	{
		Transform aTransform;
		g->DrawImageTransform(IMAGE_LOAD_BALL, aTransform, mBallX, mBallY);
	}

	if (mLoaded)
	{
		g->DrawImage(IMAGE_LOAD_DELUXE, DELUXE_X, DELUXE_Y);
		g->SetColorizeImages(true);
		g->SetColor(Color(0));
		g->DrawImage(IMAGE_LOAD_TM, TM_X, TM_Y);
		g->SetColorizeImages(false);
	}

	DeferOverlay(0);
}

// 0x494890: B drops the ball (a testing key of the original)
void LoadingScreen::KeyChar(SexyChar theChar)
{
	if (theChar == 'b')
		DoBallBounce();
}

// 0x4948b0
void LoadingScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTON1);
}

// 0x494850: a game started from the keys below begins at the stage the level selection screen showed last, at its first level
void LoadingScreen::SetStartFromSelectedStage()
{
	if (mApp->mCurrentPlayer != nullptr)
	{
		int aStage = mApp->mCurrentPlayer->mLevelSelectStage;
		if (aStage < 0 || aStage > 9)
			aStage = 0;
		mApp->mStartStage = aStage;
		mApp->mStartLevel = 0;
	}
}

// 0x4948d0: the Play button. Holding a key while pressing it picks something else than the main menu: Ctrl goes straight to the
// board, H to the help screen, D and A start the game in two other modes (the original's own testing keys).
void LoadingScreen::ButtonDepress(int theId)
{
	WidgetManager* aManager = mWidgetManager;
	if (!aManager->mKeyDown[KEYCODE_CONTROL])
	{
		if (aManager->mKeyDown['H'])
		{
			mApp->ShowHelpScreen(false, false);
			return;
		}
		if (!aManager->mKeyDown['D'])
		{
			if (!aManager->mKeyDown['A'])
			{
				if (mApp->mCurrentPlayer == nullptr)
				{
					mPlayButton->SetDisabled(true);
					mApp->DoCreateUserDialog();
					return;
				}
				mApp->ShowMainMenu();
				return;
			}
			mApp->mGameMode = GAME_MODE_QUICK_PLAY;
		}
		else
			mApp->mGameMode = GAME_MODE_DUEL;
		SetStartFromSelectedStage();
	}
	mApp->ShowBoard(true, true);
}
