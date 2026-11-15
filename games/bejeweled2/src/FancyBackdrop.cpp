// FancyBackdrop.cpp: FancyBackdrop (FancyBackdrop.h).
#include "FancyBackdrop.h"

#include "Common.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "TitleScreen.h"
#include "WinDMApp.h"

using namespace Sexy;

// 0x66b260: the places of the star sparkles, each (x, y) in the 1024x600 picture
static const int gSparklePos[12][2] = {
	{302, 195}, {826, 137}, {436, 258}, {826, 137}, {567, 346}, {496, 202},
	{59, 279}, {39, 117}, {496, 126}, {917, 506}, {640, 440}, {980, 209}
};

FancyBackdrop::FancyBackdrop(WinDMApp* theApp)
{
	mApp = theApp;
	mShowing = false;
	mSparkle = false;
	mStarScroll = 0;
	mPlanetScroll = 0;
	mFogX = 0;
	mFogFgX = 0;
	mCometX = 1400.0f;
	mCometY = 150.0f;
	mCometVX = -2.2f;
	mCometVY = 0.08f;
	mSparkleX = mSparkleY = mSparkleFrame = 0;
}

FancyBackdrop::~FancyBackdrop()
{
	for (size_t i = 0; i < mComets.size(); i++)
		delete mComets[i];
}

// 0x5ba3cb
void FancyBackdrop::Update()
{
	if (!mApp->mHasFocus)
	{
		if (mApp->mTitleScreen == NULL)
			return;
		if (mApp->mTitleScreen->mSettled)
			return;
	}

	Widget::Update();

	mPlanetScroll = mPlanetScroll + 0.023f;
	mStarScroll = mStarScroll + 0.04f;
	if (mStarScroll > 599.0f)
		mStarScroll = mStarScroll - 599.0f;
	mFogX = mFogX + 0.23f;
	if (mFogX > 1023.0f)
		mFogX = mFogX - 1023.0f;
	mFogFgX = mFogFgX + 0.51f;
	if (mFogFgX > 1023.0f)
		mFogFgX = mFogFgX - 1023.0f;

	// a sparkle starts at one of 12 places (in the first or the second copy of the star field) and plays 14 frames; one
	// that is not on the screen as it starts is dropped at once
	bool aEnd;
	if (!mSparkle)
	{
		int aPlace = Rand();
		mSparkle = true;
		aPlace %= 12;
		mSparkleX = gSparklePos[aPlace][0];
		mSparkleY = gSparklePos[aPlace][1];
		if (Rand() % 2 == 0)
			mSparkleY += 600;
		mSparkleFrame = 0;
		float aY = (float)mSparkleY - mStarScroll;
		aEnd = !(aY <= 535.0f) || (aY < 1.0f);
	}
	else
	{
		if (mUpdateCnt % 5 == 0)
			mSparkleFrame++;
		aEnd = (mSparkleFrame == 14);
	}
	if (aEnd)
		mSparkle = false;

	if (mShowing)
	{
		mCometVY = mCometVY + 0.0009f;
		mCometX = mCometVX + mCometX;
		mCometY = mCometY + mCometVY;

		Comet* aComet = new Comet();
		aComet->mX = mCometX;
		aComet->mY = mCometY;
		aComet->mLife = 0;
		aComet->mVX = ((float)(Rand() % 1000) / 500.0 - 1.0) * 0.06f;
		aComet->mVY = ((float)(Rand() % 1000) / 500.0 - 1.0) * 0.06f;
		mComets.push_back(aComet);

		for (int i = 0; i < (int)mComets.size(); i++)
		{
			Comet* aComet = mComets[i];
			double aLife = aComet->mLife + 0.005f;
			aComet->mLife = aLife;
			float aVY = aComet->mVY + 0.0005f;
			aComet->mVY = aVY;
			if (aLife >= 1.0)
			{
				delete aComet;
				mComets.erase(mComets.begin() + i);
				i--;
			}
			else
			{
				aComet->mX = aComet->mVX + aComet->mX;
				aComet->mY = aVY + aComet->mY;
			}
		}
	}

	if (mApp->mTitleScreen == NULL)
		MarkDirty();
}

void FancyBackdrop::Draw(Graphics* g)
{
	DrawBottom(g);
	DrawTop(g);
}

// 0x5b95ff
void FancyBackdrop::DrawTop(Graphics* g)
{
	if (!mApp->Is3DAccelerated())
	{
		g->DrawImage(IMAGE_MAIN_FG, 0, mHeight - IMAGE_MAIN_FG->mHeight);
	}
	else
	{
		g->DrawImage(IMAGE_MAIN_FOG_BANK, (int)mApp->ScaleNum(mFogX, 0.0f), mApp->ScaleNum(0x1a4, 0));
		g->DrawImage(IMAGE_MAIN_FOG_BANK, (int)mApp->ScaleNum(mFogX, 0.0f) - mApp->ScaleNum(0x400, 0), mApp->ScaleNum(0x1a4, 0));
		g->DrawImage(IMAGE_MAIN_FG, 0, mHeight - IMAGE_MAIN_FG->mHeight);
		g->DrawImage(IMAGE_MAIN_FOG_BANK_FG, (int)mApp->ScaleNum(mFogFgX, 0.0f), mApp->ScaleNum(0x1e3, 0));
		g->DrawImage(IMAGE_MAIN_FOG_BANK_FG, (int)mApp->ScaleNum(mFogFgX, 0.0f) - mApp->ScaleNum(0x400, 0), mApp->ScaleNum(0x1e3, 0));
	}
}

// 0x5b9983
void FancyBackdrop::DrawBottom(Graphics* g)
{
	bool is3d = mApp->Is3DAccelerated();

	g->DrawImage(IMAGE_MAIN_BKG, 0, 0);
	float aScroll = mApp->ScaleNum(mStarScroll, 0.0f);
	int aScrollInt = (int)aScroll;
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);

	if (!is3d)
	{
		g->DrawImage(IMAGE_MAIN_STARFIELD, 0, (int)(-mStarScroll));
		g->DrawImage(IMAGE_MAIN_STARFIELD, 0, mApp->ScaleNum(600, 0) - (int)mStarScroll);
	}
	else
	{
		// the star field moves by fractions of a pixel: the first copy is drawn from the scroll's whole row on
		Rect aSrc(0, aScrollInt, mApp->ScaleNum(0x400, 0), mApp->ScaleNum(600, 0) - aScrollInt);
		float aY = ((float)aScrollInt + 1.0f) - aScroll;
		g->DrawImageF(IMAGE_MAIN_STARFIELD, 0, aY, aSrc);
		g->DrawImageF(IMAGE_MAIN_STARFIELD, 0, mApp->ScaleNum(600.0f, 0.0f) - aScroll);
	}

	if (mSparkle)
	{
		Rect aSrc(mApp->ScaleNum(mSparkleFrame * 0x14, 0), 0, mApp->ScaleNumUp(0x14, 0), mApp->ScaleNumUp(0x14, 0));
		float aY = (float)mSparkleY - mStarScroll;
		if (!is3d)
			g->DrawImage(IMAGE_MAIN_STAR_SPARKLE, mApp->ScaleNum(mSparkleX, 0), (int)mApp->ScaleNum(aY, 0.0f), aSrc);
		else
			g->DrawImageF(IMAGE_MAIN_STAR_SPARKLE, (float)mApp->ScaleNum(mSparkleX, 0), mApp->ScaleNum(aY, 0.0f), aSrc);
	}

	for (int i = 0; i < (int)mComets.size(); i++)
	{
		Comet* aComet = mComets[i];
		g->SetColorizeImages(true);
		int v = (int)((1.0 - aComet->mLife) * 64.0);
		g->SetColor(Color(v, v, v));
		if (!is3d)
			g->DrawImage(IMAGE_MAIN_COMET, (int)mApp->ScaleNum(aComet->mX, 0.0f), (int)mApp->ScaleNum(aComet->mY, 0.0f));
		else
			g->DrawImageF(IMAGE_MAIN_COMET, mApp->ScaleNum(aComet->mX, 0.0f), mApp->ScaleNum(aComet->mY, 0.0f));
		g->SetColorizeImages(false);
	}

	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	float aPlanetY = (float)(450.0 - mPlanetScroll);
	if (!is3d)
		g->DrawImage(IMAGE_MAIN_PLANET_1, mApp->ScaleNum(0x348, 0), (int)mApp->ScaleNum(aPlanetY, 0.0f));
	else
		g->DrawImageF(IMAGE_MAIN_PLANET_1, mApp->ScaleNum(840.0f, 0.0f), mApp->ScaleNum(aPlanetY, 0.0f));
}
