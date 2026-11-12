#include "Hearts.h"

#include "Application.h"
#include "DrawUtil.h"
#include "Floater.h"
#include "Gameplay.h"
#include "Graphics.h"
#include "Image.h"
#include "Rect.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

static ObjectRef gHeartsRef;		// 0x597fe0

// 0x449b50
Hearts::Hearts() :
	Object(0x80),
	mCapacityFlash(0.0),
	mShownHealth(-1.0)
{
}

// 0x449ba0
Hearts::~Hearts()
{
}

// 0x449990
Hearts* Sexy::GetHearts()
{
	return dynamic_cast<Hearts*>(gHeartsRef.Get());
}

// 0x44a190
Hearts* Sexy::MakeHearts()
{
	Hearts* aHearts = new Hearts();
	aHearts->SetName("");
	gHeartsRef = ObjectRef(aHearts);
	return aHearts;
}

// 0x4499c0
void Sexy::SetHealth(double theHealth)
{
	GetStats()->Set(STAT_HEALTH, theHealth);
}

// 0x4499e0
double Sexy::GetPlayerHealth()
{
	return GetStats()->Get(STAT_HEALTH);
}

// 0x4499f0
double Sexy::GetHealthCapacity()
{
	return GetStats()->Get(STAT_HEALTH_CAPACITY);
}

// 0x449bd0
int Sexy::GetHeartsX()
{
	if (gApp->mShopScreen != nullptr)
		return 100;

	int aCapacity = ToInt(GetStats()->Get(STAT_HEALTH_CAPACITY));
	int aCelWidth = IMAGE_INGAMEUI_HEARTS->GetCelWidth();
	return (gApp->mWidth - (aCelWidth - 2) * aCapacity) / 2 + 0xe;
}

// 0x449db0
void Hearts::GetHealthEndPos(double* thePos)
{
	double aHealth = GetStats()->Get(STAT_HEALTH);
	if (!mFlights.empty())
		aHealth = mFlights[0].mHealth;

	Rect aCelRect = IMAGE_INGAMEUI_HEARTS->GetCelRect(0);
	thePos[0] = (double)GetHeartsX() + (aHealth + 0.5) * (double)aCelRect.mWidth;
	thePos[1] = (double)aCelRect.mHeight * 0.5 + 5.0;
}

// 0x449e60
void Hearts::GetNewHeartPos(double* thePos)
{
	int aCapacity = ToInt(GetStats()->Get(STAT_HEALTH_CAPACITY));
	if (mCapacityFlash > 0.0)
		aCapacity = aCapacity - 1;

	Rect aCelRect = IMAGE_INGAMEUI_HEARTS->GetCelRect(0);
	thePos[0] = (double)GetHeartsX() + ((double)aCapacity + 0.5) * (double)aCelRect.mWidth;
	thePos[1] = (double)aCelRect.mHeight * 0.5 + 5.0;
}

// 0x44a000
void Hearts::Update(float theFrac)
{
	Object::Update(theFrac);

	// the gains that are still on their way; one whose time is up is taken out (the next one is not looked at in this step)
	if (!mFlights.empty())
	{
		for (int i = 0; i < (int)mFlights.size(); i++)
		{
			if (mFlights[i].mTime > 0.0)
			{
				mFlights[i].mTime = mFlights[i].mTime - (double)theFrac * 0.01;
				if (mFlights[i].mTime <= 0.0)
				{
					mFlights[i].mTime = 0.0;
					mFlights.erase(mFlights.begin() + i);
				}
			}
		}
	}

	if (mCapacityFlash > 0.0)
	{
		mCapacityFlash = mCapacityFlash - (double)theFrac * 0.01;
		if (mCapacityFlash <= 0.0)
			mCapacityFlash = 0.0;
	}

	// the row's health follows the real one (the first gain still on its way holds it back): up by 0.03 a step, down by 0.02
	double aTarget = GetStats()->Get(STAT_HEALTH);
	if (!mFlights.empty())
		aTarget = mFlights[0].mHealth;

	if (mShownHealth != -1.0)
	{
		if (mShownHealth < aTarget)
		{
			if (mShownHealth + 0.03 <= aTarget)
				aTarget = mShownHealth + 0.03;
			mShownHealth = aTarget;
			return;
		}
		if (mShownHealth <= aTarget)
			return;
		if (aTarget <= mShownHealth - 0.02)
			aTarget = mShownHealth - 0.02;
	}
	mShownHealth = aTarget;
}

// 0x4497e0: the hearts of the row: an empty heart for each place (flashing while the health is gone), then the full ones, then
// the part of the one the health ends in (flashing too when the health is less than one)
static void DrawHeartRow(Graphics* g, int theCapacity, double theHealth, int theX, int theY, int theTick, Image* theHearts,
	int theOverlap)
{
	for (int i = 0; i < theCapacity; i++)
	{
		Rect aCel = theHearts->GetCelRect(1);
		int aCellX = (aCel.mWidth - theOverlap) * i + theX;
		g->DrawImage(theHearts, aCellX, theY, aCel);
		if (theHealth == 0.0 && theTick % 0x32 < 0x19)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImage(theHearts, aCellX, theY, aCel);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	int aFull = ToInt(theHealth);
	int aDrawn = 0;
	for (; aDrawn < aFull; aDrawn++)
	{
		Rect aCel = theHearts->GetCelRect(0);
		g->DrawImage(theHearts, (aCel.mWidth - theOverlap) * aDrawn + theX, theY, aCel);
	}

	Rect aPartCel = theHearts->GetCelRect(0);
	int aPartWidth = ToInt((theHealth - (double)aFull) * (double)aPartCel.mWidth);
	if (aPartWidth > 0)
	{
		int aPartX = (aPartCel.mWidth - theOverlap) * aDrawn + theX;
		aPartCel.mWidth = aPartWidth;
		g->DrawImage(theHearts, aPartX, theY, aPartCel);
		if (theHealth < 1.0 && theTick % 0x32 < 0x19)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			aPartCel.mX = aPartCel.mX + theHearts->GetCelWidth();
			g->DrawImage(theHearts, aPartX, theY, aPartCel);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
}

// 0x449c40
void Hearts::Draw(Graphics* g)
{
	if (mDead)
		return;

	int aX = GetHeartsX();
	double aShown = mShownHealth;
	if (aShown > 0.0 && aShown < 0.15)
		aShown = 0.15;

	int aCapacity = ToInt(GetStats()->Get(STAT_HEALTH_CAPACITY));
	int aRowWidth = (IMAGE_INGAMEUI_HEARTS->GetCelWidth() - 2) * aCapacity;
	int aBoxWidth = aRowWidth + 0x28;

	if (gApp->mShopScreen == nullptr)
	{
		g->DrawImageBox(Rect(GetHeartsX() - (aBoxWidth - aRowWidth) / 2, 0, aBoxWidth, IMAGE_HEARTBACK->mHeight), IMAGE_HEARTBACK);
	}

	if (mCapacityFlash > 0.0)
		aCapacity = aCapacity - 1;

	DrawHeartRow(g, aCapacity, aShown, aX, 5, mTickCount, IMAGE_INGAMEUI_HEARTS, 2);
	g->SetColorizeImages(false);
	g->DrawImage(IMAGE_INGAMEUI_HEALTH, GetHeartsX() + aRowWidth - IMAGE_INGAMEUI_HEALTH->mWidth - 0x26, 0x2a);
}

// 0x44a270
void Hearts::ChangeHealth(int theChange, double theAmount)
{
	double aHealthBefore = GetStats()->Get(STAT_HEALTH);

	switch (theChange)
	{
	case HEALTH_MISS:
		if (!gApp->GetCheatFlag(0))
		{
			double aHealth = GetStats()->Get(STAT_HEALTH);
			if (0.0 <= aHealth - 0.3333333333333333)
				aHealth = GetStats()->Get(STAT_HEALTH) - 0.3333333333333333;
			else
				aHealth = 0.0;
			SetHealth(aHealth);
			gApp->PlaySample(SOUND_UI_HEART_LOST);
			for (size_t i = 0; i < mFlights.size(); i++)
			{
				double aFlightHealth = mFlights[i].mHealth - 0.3333333333333333;
				if (aFlightHealth < 0.0)
					aFlightHealth = 0.0;
				mFlights[i].mHealth = aFlightHealth;
			}
		}
		break;
	case HEALTH_ESCAPE:
		if (!gApp->GetCheatFlag(1))
		{
			double aLoss = theAmount * 0.5;
			double aHealth = GetStats()->Get(STAT_HEALTH);
			if (0.0 <= aHealth - aLoss)
				aHealth = GetStats()->Get(STAT_HEALTH) - aLoss;
			else
				aHealth = 0.0;
			SetHealth(aHealth);
			gApp->PlaySample(SOUND_UI_HEART_LOST);
			for (size_t i = 0; i < mFlights.size(); i++)
			{
				double aFlightHealth = mFlights[i].mHealth - aLoss;
				if (aFlightHealth < 0.0)
					aFlightHealth = 0.0;
				mFlights[i].mHealth = aFlightHealth;
			}

			double aCombo = GetStats()->Get(STAT_COMBO);
			if (1.0 < aCombo)
			{
				aCombo = aCombo - 0.1;
				if (aCombo <= 1.0)
					aCombo = 1.0;
				GetStats()->Set(STAT_COMBO, aCombo);
			}
		}
		break;
	case HEALTH_GAIN:
		if (GetGameplay()->mPhase != Gameplay::PHASE_OVER)
		{
			HealthFlight aFlight = { GetStats()->Get(STAT_HEALTH), 0.75 };
			mFlights.push_back(aFlight);

			double aHealth = GetStats()->Get(STAT_HEALTH);
			double aCapacity = GetStats()->Get(STAT_HEALTH_CAPACITY);
			if (theAmount * 0.5 + aHealth <= aCapacity)
				SetHealth(GetStats()->Get(STAT_HEALTH) + theAmount * 0.5);
			else
				SetHealth(GetStats()->Get(STAT_HEALTH_CAPACITY));
		}
		break;
	case HEALTH_NEW_HEART:
		mCapacityFlash = 1.0;
		GetStats()->Set(STAT_HEALTH_CAPACITY, GetStats()->Get(STAT_HEALTH_CAPACITY) + 1.0);
		break;
	}

	// health lost: a heart breaks at the end of what is left
	if (GetStats()->Get(STAT_HEALTH) < aHealthBefore)
	{
		double aPos[2];
		GetHearts()->GetHealthEndPos(aPos);
		MakeFloater(FLOATER_BROKEN_HEART, aPos[0], aPos[1], nullptr, 0, "");
	}
}
