#include "GnomePowerUp.h"

#include <cmath>

#include "Application.h"
#include "Color.h"
#include "Cubby.h"
#include "Floater.h"
#include "Gameplay.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "Score.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43c690
GnomePowerUp::GnomePowerUp() :
	Creature(KIND_POWER_UP),
	mDrawX(0.0),
	mDrawY(0.0),
	mPulse(0.0)
{
}

// 0x43c6d0
GnomePowerUp::~GnomePowerUp()
{
}

// 0x43c6e0
void GnomePowerUp::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mDrawX = thePos[0] - mAnchorX;
	mDrawY = (thePos[1] - mAnchorY) - mHeight;
	mPulse = fabs(sin((double)mAge * 0.02));
}

// 0x43c730: the bottle, and over it its picture again, added to what is there, as bright as the glow
void GnomePowerUp::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
{
	Graphics aG(*g);
	DrawShadow(&aG, theX, theY);

	double aTopLeftX = theX - mAnchorX;
	double aTopLeftY = (theY - mAnchorY) - mHeight;
	int aFlash = FlashAlpha(theFlashTime);
	DrawBody(&aG, aTopLeftX, aTopLeftY, 0, 0, aFlash, 1.0);

	double aPos[2];
	GetPositionInCell(aPos, theX, theY);
	aG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aG.SetColorizeImages(true);
	aG.SetColor(Color(255, 100, 255, (int)(mPulse * 255.0)));
	aG.DrawImage(IMAGE_GNOME_POWERUP, (int)aPos[0], (int)aPos[1]);
}

// 0x43c8b0
bool GnomePowerUp::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit && !IsFrozen())
	{
		GetStats()->AddInt(STAT_POWER_BOTTLES, 1);

		Rect aClip = mCubby->GetEffectRect();
		ScoreDisplayPos aSign = GetPowerUpSignPos();
		MakeFloaterShrinking(mDrawX, mDrawY, aSign.mX, aSign.mY, IMAGE_GNOME_POWERUP, 0, 0, mExposed, aClip.mX, aClip.mY,
			aClip.mWidth, aClip.mHeight);
		GetGameplay()->mDoubleHitDelay = 0.75;
		GetGameplay()->mDoubleHitTimer = 20.0;
		gApp->PlaySample(SOUND_LUCKY_CLOVER_PICKEN_AND_ACTIVATED);
	}
	return aHit;
}

// 0x43c9b0
std::string GnomePowerUp::SaveExtra()
{
	ValueList aList;
	aList.Add(mDrawX);
	aList.Add(mDrawY);
	aList.Add(mPulse);
	return aList.ToString();
}

// 0x43ca60
void GnomePowerUp::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mDrawX = aList.NextDouble();
	mDrawY = aList.NextDouble();
	mPulse = aList.NextDouble();
}
