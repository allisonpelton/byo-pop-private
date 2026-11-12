#include "Sticker.h"

#include "Application.h"
#include "DrawUtil.h"
#include "Floater.h"
#include "Graphics.h"
#include "Image.h"
#include "Popper.h"
#include "Res.h"
#include "Score.h"
#include "Stats.h"

using namespace Sexy;

// 0x591d60 (through the exe's table of picture globals): the kinds of sticker in the order the boards number them
static Image** const sStickerImages[NUM_STICKER_KINDS] =
{
	&IMAGE_STICKER_FLOWER, &IMAGE_STICKER_MUSHROOM, &IMAGE_STICKER_MUSHROOM2, &IMAGE_STICKER_SHELL1, &IMAGE_STICKER_SHELL2,
	&IMAGE_STICKER_SHELL3, &IMAGE_STICKER_SHELL4, &IMAGE_STICKER_NUT1, &IMAGE_STICKER_NUT2, &IMAGE_STICKER_DEADWOOD,
	&IMAGE_STICKER_SNAIL, &IMAGE_STICKER_PUDDLE, &IMAGE_STICKER_BUCKET, &IMAGE_STICKER_EDELWEISS, &IMAGE_STICKER_MISTLETOE,
	&IMAGE_STICKER_SNOWMAN, &IMAGE_STICKER_LADYBIRD, &IMAGE_STICKER_RATTLESNAKE, &IMAGE_STICKER_CACTUS, &IMAGE_STICKER_COWSKULL,
	&IMAGE_STICKER_SLIPPER, &IMAGE_STICKER_CUP, &IMAGE_STICKER_ROLLERS, &IMAGE_STICKER_BALL, &IMAGE_STICKER_CONE,
	&IMAGE_STICKER_OIL, &IMAGE_STICKER_SCREW, &IMAGE_STICKER_OLDBOX
};

// 0x426700
Image* Sexy::GetStickerImage(int theKind)
{
	return *sStickerImages[theKind];
}

// 0x443160: the picture is centred on the point
Sticker::Sticker(double theX, double theY, int theKind) :
	SimpleObject(8),
	mKind(theKind),
	mHit(false),
	mCenterX(theX),
	mCenterY(theY)
{
	Image* anImage = GetStickerImage(mKind);
	mBounds.mX = ToInt(mCenterX) - anImage->mWidth / 2;
	mBounds.mY = ToInt(mCenterY) - anImage->mHeight / 2;
	mBounds.mWidth = anImage->mWidth;
	mBounds.mHeight = anImage->mHeight;
}

Sticker::~Sticker()
{
}

// 0x442f70
void Sticker::FixedUpdate()
{
	Object::FixedUpdate();
	if (gApp->mShopScreen == nullptr && mActive == mHit)
		mActive = !mHit;
}

// 0x442fa0
void Sticker::Draw(Graphics* g)
{
	if (mHit)
		return;

	Graphics aG(*g);
	Image* anImage = GetStickerImage(mKind);
	aG.DrawImage(anImage, ToInt(mCenterX - (double)anImage->mWidth * 0.5), ToInt(mCenterY - (double)anImage->mHeight * 0.5));
}

// 0x4432e0: the mask is put down at the point (given as the mask's corner, so the picture's middle is added), and a solid
// pixel of the picture under any point of it is a hit
bool Sticker::Hit(int theX, int theY, const std::vector<Point>& theMask, int theWidth, int theHeight)
{
	int aBoundsY = mBounds.mY;
	int aBoundsHeight = mBounds.mHeight + theHeight * 2;
	int aBoundsX = mBounds.mX;
	int aBoundsWidth = mBounds.mWidth + theWidth * 2;
	int aX = theX + GetStickerImage(mKind)->mWidth / 2;
	int aY = theY + GetStickerImage(mKind)->mHeight / 2;

	if (aBoundsX - theWidth <= aX && aX < aBoundsWidth + (aBoundsX - theWidth) && aBoundsY - theHeight <= aY &&
		aY < aBoundsHeight + (aBoundsY - theHeight))
	{
		double aPos[2] = { mCenterX, mCenterY };
		int aXY[2] = { aX, aY };
		return MaskHitsImage(aPos, aXY, theMask, GetStickerImage(mKind), 0, 0);
	}
	return false;
}

// 0x443050
void Sticker::Strike(double theX, double theY)
{
	mHit = true;
	mActive = false;
	gApp->PlaySample(SOUND_HAMMER_PLOP_2);

	double aDirection[2] = { 0.0, -0.1 };
	MakePopper(theX + 45.0, theY + 20.0, 3, aDirection);

	if (!gApp->IsShopOpen() && !gApp->IsChallengeOpen())
		GetScore()->AddPoints(500, ToInt(mCenterX), ToInt(mCenterY - 10.0), 2);
}

// 0x443120
void Sticker::SetHit(bool theHit)
{
	mHit = theHit;
}

// 0x443130
void Sticker::GetStrikePoint(double* thePoint) const
{
	thePoint[0] = mCenterX - 44.0;
	thePoint[1] = mCenterY - 24.0;
}

// 0x442da0
void Sexy::MakeSpeedSticker(int theTier, double theX, double theY)
{
	GetStats()->AddInt(STAT_ROUND_SPEED_HITS, 1);

	int aPoints = 0;
	switch (theTier)
	{
	case 0:
		aPoints = 250;
		break;
	case 1:
		aPoints = 500;
		break;
	case 2:
		aPoints = 750;
		break;
	case 3:
		aPoints = 1000;
		break;
	case 4:
		aPoints = 2000;
		break;
	}

	Floater* aBanner = MakeFloater(FLOATER_SHATTER, theX, theY, "", aPoints, "");
	switch (theTier)
	{
	case 0:
		aBanner->SetImage(IMAGE_SPEED_BONUS);
		gApp->PlaySample(SOUND_UI_SPEED_BONUS_1);
		break;
	case 1:
		aBanner->SetImage(IMAGE_SPEED_BONUS_SUPER);
		gApp->PlaySample(SOUND_UI_SPEED_BONUS_2);
		break;
	case 2:
		aBanner->SetImage(IMAGE_SPEED_BONUS_HYPER);
		gApp->PlaySample(SOUND_UI_SPEED_BONUS_3);
		break;
	case 3:
		aBanner->SetImage(IMAGE_SPEED_BONUS_ULTRA);
		gApp->PlaySample(SOUND_UI_SPEED_BONUS_4);
		break;
	case 4:
		aBanner->SetImage(IMAGE_SPEED_BONUS_GIGA);
		gApp->PlaySample(SOUND_UI_SPEED_BONUS_5);
		break;
	}
}
