#include "HammerDisplay.h"

#include "Application.h"
#include "Avatar.h"
#include "Gameplay.h"
#include "Graphics.h"
#include "Image.h"
#include "Rect.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

static ObjectRef gHammerDisplayRef;		// 0x597fc0

// 0x4496d0
HammerDisplay::HammerDisplay() :
	Object(0x80),
	mUpgradeTimer(0.0),
	mUpgradeState(HAMMER_UPGRADE_NONE),
	mLevelsBought(0),
	mWasOnFire(false)
{
}

HammerDisplay::~HammerDisplay()
{
}

// 0x449240
HammerDisplay* Sexy::GetHammerDisplay()
{
	return dynamic_cast<HammerDisplay*>(gHammerDisplayRef.Get());
}

// 0x4496d0
HammerDisplay* Sexy::MakeHammerDisplay()
{
	HammerDisplay* aDisplay = new HammerDisplay();
	aDisplay->SetName("");
	gHammerDisplayRef = ObjectRef(aDisplay);
	return aDisplay;
}

// 0x4491b0
int HammerDisplay::GetLevel()
{
	int aLevel = GetStats()->GetInt(STAT_HAMMER_LEVEL);
	if (mUpgradeTimer > 0.0 && mUpgradeState == HAMMER_UPGRADE_LEVEL)
		aLevel = aLevel - mLevelsBought;
	return aLevel;
}

// 0x4491f0
bool HammerDisplay::IsOnFire()
{
	int aOnFire = GetStats()->GetInt(STAT_HAMMER_ON_FIRE);
	if (mUpgradeTimer > 0.0)
	{
		if (mUpgradeState == HAMMER_UPGRADE_LEVEL)
			return mWasOnFire;
		if (mUpgradeState == HAMMER_UPGRADE_FIRE)
			return false;
	}
	return aOnFire == 1;
}

// 0x449270, 0x4492f0, 0x449370: buying a level the hammer does not have yet; the upgrade also puts the fire out
void HammerDisplay::UpgradeToLevel(int theLevel)
{
	int aLevel = GetStats()->GetInt(STAT_HAMMER_LEVEL);
	if (aLevel < theLevel)
	{
		GetStats()->SetInt(STAT_HAMMER_LEVEL, theLevel);
		mUpgradeTimer = 1.0;
		mLevelsBought = theLevel - aLevel;
		mWasOnFire = GetStats()->GetInt(STAT_HAMMER_ON_FIRE) == 1;
		mUpgradeState = HAMMER_UPGRADE_LEVEL;
		GetStats()->SetInt(STAT_HAMMER_ON_FIRE, 0);
	}
}

// 0x4493f0
void HammerDisplay::SetOnFire()
{
	GetStats()->SetInt(STAT_HAMMER_ON_FIRE, 1);
	mUpgradeTimer = 1.0;
	mUpgradeState = HAMMER_UPGRADE_FIRE;
}

// 0x449420
void HammerDisplay::Update(float theFrac)
{
	if (mUpgradeTimer > 0.0)
	{
		mUpgradeTimer = mUpgradeTimer - (double)theFrac * 0.01;
		if (mUpgradeTimer <= 0.0)
			mUpgradeTimer = 0.0;
	}
}

// 0x449470
int Sexy::GetDamageOfLevel(int theLevel)
{
	int aDamage = GetGameplay()->mDoubleHitTimer > 0.0 ? 0x14 : 0;
	switch (theLevel)
	{
	case 0:
		return aDamage + 1;
	case 1:
		return aDamage + 2;
	case 2:
		return aDamage + 3;
	case 3:
		return aDamage + 6;
	}
	return aDamage;
}

// 0x4494d0
int Sexy::GetHammerDamage()
{
	return GetDamageOfLevel(GetStats()->GetInt(STAT_HAMMER_LEVEL));
}

// 0x449590: only the shop has the display; the shelf shows the fifth cel of the hammer's strip at three quarters of its size
void HammerDisplay::Draw(Graphics* g)
{
	if (gApp->mShopScreen == nullptr)
		return;

	Graphics aG(*g);
	Image* aHammer = GetHammerImage(GetLevel(), IsOnFire());
	if (!mDead)
	{
		aG.DrawImage(IMAGE_INGAMEUI_HAMMERBACK, 0, 0);
		int aHeight = (int)((double)aHammer->GetCelHeight() * 0.75);
		int aWidth = (int)((double)aHammer->GetCelWidth() * 0.75);
		aG.DrawImageCel(aHammer, Rect(-20, 0, aWidth, aHeight), 5, 0);
	}
}
