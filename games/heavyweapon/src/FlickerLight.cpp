#include "FlickerLight.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Sprite.h"
#include "SoundInstance.h"
#include "SoundManager.h"

using namespace Sexy;

namespace
{
	const int FLICKER_ODDS = 200;			// a flicker starts one update in this many
	const int FLICKER_MIN_LENGTH = 10;
	const int FLICKER_LENGTH_RANGE = 0x41;	// its length is 10..74 updates
	const int FLICKER_BRIGHTNESS_RANGE = 0x20;
	const int FLICKER_BRIGHTNESS_BASE = 0x80;
	const int STEADY_BRIGHTNESS = 0xc0;		// the glow's grey level between flickers
}

// 0x423d90
FlickerLight::FlickerLight(HeavyWeaponApp* theApp, Sprite* theSprite)
{
	mApp = theApp;
	mSprite = theSprite;
	mFlickerLeft = 0;
	mSound = NULL;
}

// 0x423db0: the destructor body, which releases a buzz still playing
FlickerLight::~FlickerLight()
{
	if (mSound != NULL)
		mSound->Release();
}

// 0x423dc0: the glow, added to the picture, grey at its brightness (steady when it is not flickering)
void FlickerLight::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	int aLevel = (mFlickerLeft == 0) ? STEADY_BRIGHTNESS : mBrightness;
	g->SetColor(Color(aLevel, aLevel, aLevel, 0xff));
	mSprite->Draw(g, 0, 0, false, 0, 0, false);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x423e50: a flicker starts at random (the first draw is always made, the second only when one starts); each update of
// one draws a new brightness; the buzz stops with its last update
void FlickerLight::Update()
{
	int aDraw = Rand();
	if ((aDraw % FLICKER_ODDS == 0) && (mFlickerLeft == 0))
	{
		mFlickerLeft = Rand() % FLICKER_LENGTH_RANGE + FLICKER_MIN_LENGTH;
		mSound = mApp->mSoundManager->GetSoundInstance(SOUND_SHORTING);
		if (mSound != NULL)
			mSound->Play(true, true);
	}
	if (mFlickerLeft != 0)
	{
		mFlickerLeft--;
		mBrightness = Rand() % FLICKER_BRIGHTNESS_RANGE + FLICKER_BRIGHTNESS_BASE;
		if ((mFlickerLeft == 0) && (mSound != NULL))
		{
			mSound->Release();
			mSound = NULL;
		}
	}
}
