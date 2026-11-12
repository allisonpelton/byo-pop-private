#include "GnomeStrawberry.h"

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
#include "ValueList.h"

using namespace Sexy;

// 0x43de80
GnomeStrawberry::GnomeStrawberry() :
	Creature(KIND_STRAWBERRY),
	mDrawX(0.0),
	mDrawY(0.0),
	mPulse(0.0)
{
}

// 0x43dec0
GnomeStrawberry::~GnomeStrawberry()
{
}

// 0x43e030
void Sexy::StartStrawberryPower(double theDelay, double theTimer)
{
	GetGameplay()->mStrawberryDelay = theDelay;
	GetGameplay()->mStrawberryTimer = theTimer;
	gApp->PlaySample(SOUND_VARIOUS_SQUISH_STRAWBERRY);
	gApp->PlaySample(SOUND_LUCKY_CLOVER_PICKEN_AND_ACTIVATED);
}

// 0x43c6e0
void GnomeStrawberry::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mDrawX = thePos[0] - mAnchorX;
	mDrawY = (thePos[1] - mAnchorY) - mHeight;
	mPulse = fabs(sin((double)mAge * 0.02));
}

// 0x43ded0: the strawberry, and over it its picture again, added to what is there, as bright as the glow
void GnomeStrawberry::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
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
	aG.DrawImage(IMAGE_GNOME_BONUS_STRAWBERRY, (int)aPos[0], (int)aPos[1]);
}

// 0x43e0a0
bool GnomeStrawberry::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit && !IsFrozen())
	{
		Rect aClip = mCubby->GetEffectRect();
		ScoreDisplayPos aSign = GetStrawberrySignPos();
		MakeFloaterShrinking(mDrawX, mDrawY, aSign.mX, aSign.mY, IMAGE_GNOME_BONUS_STRAWBERRY, 0, 0, mExposed, aClip.mX,
			aClip.mY, aClip.mWidth, aClip.mHeight);
		StartStrawberryPower(0.75, 15.0);
	}
	return aHit;
}

// 0x43e170
std::string GnomeStrawberry::SaveExtra()
{
	ValueList aList;
	aList.Add(mDrawX);
	aList.Add(mDrawY);
	aList.Add(mPulse);
	return aList.ToString();
}

// 0x43e220
void GnomeStrawberry::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mDrawX = aList.NextDouble();
	mDrawY = aList.NextDouble();
	mPulse = aList.NextDouble();
}
