#include "Effects.h"
#include "Graphics.h"
#include "Image.h"
#include "Color.h"
#include "Rect.h"
#include "Compat2004.h"

using namespace Sexy;

FrameEffect::FrameEffect(Image* theImage, int theFrames, int theX, int theY, double theInterval, double theFadeTime)
{
	mFade = 0;
	mImage = theImage;
	mInterval = theInterval;
	mFrames = theFrames;
	mFadeTime = theFadeTime;
	mX = theX;
	mY = theY;
	mTime = 0;
	mCur = 0;
	mNext = 1;
}

// the cross-fade runs over the last mFadeTime seconds of each interval
void FrameEffect::UpdateFade()
{
	if (mFadeTime > 0)
	{
		double aStart = mInterval - mFadeTime;
		if (aStart < mTime)
		{
			mFade = (mTime - aStart) / mFadeTime;
			return;
		}
	}
	mFade = 0;
}

void FrameEffect::AdvanceFrame()
{
	mCur = mNext;
	mTime -= mInterval;
	mNext = (mNext + 1) % mFrames;
}

// 0x4374a0
void FrameEffect::Update(double theDelta)
{
	mTime += theDelta;
	while (mInterval < mTime)
		AdvanceFrame();
	UpdateFade();
}

// 0x437770
void FrameEffect::Draw(Graphics* g)
{
	int aWidth = mImage->GetWidth() / mFrames;
	int aHeight = mImage->GetHeight();
	g->DrawImage(mImage, mX, mY, Rect(mCur * aWidth, 0, aWidth, aHeight));
	if (mFade > 0)
	{
		int anAlpha = (int) (mFade * 255);
		if (anAlpha < 0)
			anAlpha = 0;
		if (anAlpha > 255)
			anAlpha = 255;
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawImage(mImage, mX, mY, Rect(mNext * aWidth, 0, aWidth, aHeight));
		g->SetColorizeImages(false);
	}
}

PingPongEffect::PingPongEffect(Image* theImage, int theFrames, int theX, int theY, double theInterval, double theFadeTime) :
	FrameEffect(theImage, theFrames, theX, theY, theInterval, theFadeTime)
{
	mStep = 1;
}

// 0x437590
void PingPongEffect::Update(double theDelta)
{
	mTime += theDelta;
	while (mInterval < mTime)
	{
		mCur = mNext;
		mNext += mStep;
		mTime -= mInterval;
		if (mNext == 0)
			mStep = 1;
		else if (mNext == mFrames - 1)
			mStep = -1;
	}
	UpdateFade();
}

SparkleEffect::SparkleEffect(Image* theImage, int theFrames, int theX, int theY, double theMinWait, double theMaxWait,
	double theInterval, double theFadeTime) :
	FrameEffect(theImage, theFrames, theX, theY, theInterval, theFadeTime)
{
	mMinWait = theMinWait;
	mMaxWait = theMaxWait;
	mPlaying = false;
	mWait = RandomWait();
}

// a random time between the two limits, in 32 steps
double SparkleEffect::RandomWait()
{
	return (mMaxWait - mMinWait) * (double) (CrtRand() & 31) * 0.03225806451612903 + mMinWait;
}

// 0x4376d0
void SparkleEffect::Update(double theDelta)
{
	if (mPlaying)
	{
		// the cycle ends when the frame that fades in is the first again and the interval is up
		if ((mNext == 0) && (mInterval < theDelta + mTime))
		{
			mPlaying = false;
			mWait = RandomWait();
		}
		FrameEffect::Update(theDelta);
		if (mPlaying)
			return;
	}
	mTime = 0;
	mWait -= theDelta;
	if (mWait < 0)
		mPlaying = true;
}

EffectList::~EffectList()
{
	Clear();
}

void EffectList::Clear()
{
	for (size_t i = 0; i < mEffects.size(); i++)
		delete mEffects[i];
	mEffects.clear();
}

void EffectList::Update(double theDelta)
{
	for (size_t i = 0; i < mEffects.size(); i++)
		mEffects[i]->Update(theDelta);
}

void EffectList::Draw(Graphics* g)
{
	for (size_t i = 0; i < mEffects.size(); i++)
		mEffects[i]->Draw(g);
}
