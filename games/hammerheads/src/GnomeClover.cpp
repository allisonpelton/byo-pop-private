#include "GnomeClover.h"

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

// 0x437d80
GnomeClover::GnomeClover() :
	Creature(KIND_CLOVER),
	mDrawX(0.0),
	mDrawY(0.0),
	mPulse(0.0)
{
}

// 0x437dc0
GnomeClover::~GnomeClover()
{
}

// 0x437f30
void Sexy::StartCloverPower(double theDelay)
{
	GetGameplay()->mCloverDelay = theDelay;
	GetGameplay()->mCloverTimer = 25.0;
	gApp->PlaySample(SOUND_LUCKY_CLOVER_PICKEN_AND_ACTIVATED);
}

// 0x43c6e0
void GnomeClover::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mDrawX = thePos[0] - mAnchorX;
	mDrawY = (thePos[1] - mAnchorY) - mHeight;
	mPulse = fabs(sin((double)mAge * 0.02));
}

// 0x437dd0: the clover, and over it its picture again, added to what is there, as bright as the glow
void GnomeClover::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
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
	aG.DrawImage(IMAGE_LUCKY_CLOVER, (int)aPos[0], (int)aPos[1]);
}

// 0x4380f0
bool GnomeClover::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit && !IsFrozen())
	{
		GetStats()->AddInt(STAT_CLOVERS, 1);

		Rect aClip = mCubby->GetEffectRect();
		ScoreDisplayPos aSign = GetCloverSignPos();
		MakeFloaterShrinking(mDrawX, mDrawY, aSign.mX, aSign.mY, IMAGE_LUCKY_CLOVER, 0, 0, mExposed, aClip.mX, aClip.mY,
			aClip.mWidth, aClip.mHeight);
		StartCloverPower(0.75);

		Floater* aBanner = MakeFloater(FLOATER_SHATTER, mHeadX + mDrawX, mHeadY + mDrawY, nullptr, 0, "");
		aBanner->SetImage(IMAGE_LUCKY);
		aBanner->SetSpeed(2.0);
	}
	return aHit;
}

// 0x437f90
std::string GnomeClover::SaveExtra()
{
	ValueList aList;
	aList.Add(mDrawX);
	aList.Add(mDrawY);
	aList.Add(mPulse);
	return aList.ToString();
}

// 0x438040
void GnomeClover::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mDrawX = aList.NextDouble();
	mDrawY = aList.NextDouble();
	mPulse = aList.NextDouble();
}
