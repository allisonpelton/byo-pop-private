// The effects overlay (see Effects.h).

#include "Effects.h"
#include "AlchemyApp.h"
#include "Graphics.h"
#include "ImageFont.h"

#include <cstdio>

using namespace Sexy;

// 0x40858e
EffectsWidget::EffectsWidget(AlchemyApp* theApp)
{
	mApp = theApp;
	mShowHelpArrow = false;
	mHasTransparencies = true;
	mMouseVisible = false;   // port: the overlay never takes the mouse (the original's hit test finds nothing to do)
}

void EffectsWidget::Clear()
{
	mPopups.clear();
	mSparkles.clear();
}

// 0x408938
void EffectsWidget::AddScorePopup(int theX, int theY, int theValue)
{
	ScorePopup aPopup;
	aPopup.mX = theX;
	aPopup.mY = theY;
	aPopup.mLife = 0x6e;
	aPopup.mValue = theValue;
	aPopup.mAlpha = 1.0;
	mPopups.push_back(aPopup);
}

// 0x408970
void EffectsWidget::AddSparkle(int theX, int theY, bool isFalling, bool isBig)
{
	Sparkle aSparkle;
	aSparkle.mX = theX;
	aSparkle.mY = theY;
	aSparkle.mFrame = 0;
	aSparkle.mFalls = isFalling;
	aSparkle.mBig = isBig;
	mSparkles.push_back(aSparkle);
}

// 0x40844f
void EffectsWidget::Update()
{
	Widget::Update();

	for (std::list<ScorePopup>::iterator anItr = mPopups.begin(); anItr != mPopups.end();)
	{
		ScorePopup& aPopup = *anItr;
		if (mUpdateCnt % 3 == 0)
		{
			MarkDirty();
			if (mApp->mOverFont->GetAscent() / 2 + 2 < aPopup.mY)
				aPopup.mY--;
		}
		double anAlpha = aPopup.mLife * 0.05555555555555555;
		if (anAlpha > 1.0)
			anAlpha = 1.0;
		if (anAlpha != aPopup.mAlpha)
		{
			aPopup.mAlpha = anAlpha;
			MarkDirty();
		}
		aPopup.mLife--;
		if (aPopup.mLife < 1)
		{
			anItr = mPopups.erase(anItr);
			MarkDirty();
		}
		else
			++anItr;
	}

	for (std::list<Sparkle>::iterator anItr = mSparkles.begin(); anItr != mSparkles.end();)
	{
		Sparkle& aSparkle = *anItr;
		if (mUpdateCnt % 3 == 0)
		{
			MarkDirty();
			aSparkle.mFrame++;
			if (aSparkle.mFrame >= 14)
			{
				anItr = mSparkles.erase(anItr);
				continue;
			}
		}
		if ((mUpdateCnt % 4 == 0) && aSparkle.mFalls)
		{
			aSparkle.mY++;
			MarkDirty();
		}
		++anItr;
	}
}

// 0x4086e6
void EffectsWidget::Draw(Graphics* g)
{
	if (mShowHelpArrow)
		g->DrawImage(mApp->mImages[IMAGE_HELPARROW], 0x62, 0x15e);

	for (std::list<ScorePopup>::iterator anItr = mPopups.begin(); anItr != mPopups.end(); ++anItr)
	{
		g->SetFont(mApp->mOverFont);
		g->SetColor(Color(255, 255, 255, (int) (anItr->mAlpha * 255.0)));
		char aBuf[32];
		sprintf(aBuf, "%d", anItr->mValue);
		SexyString aText = aBuf;
		int aX = anItr->mX - g->GetFont()->StringWidth(aText) / 2;
		int aY = g->GetFont()->GetAscent() / 2 + anItr->mY;
		g->DrawString(aText, aX, aY);
	}

	for (std::list<Sparkle>::iterator anItr = mSparkles.begin(); anItr != mSparkles.end(); ++anItr)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		if (!anItr->mBig)
			g->DrawImage(mApp->mImages[IMAGE_TINYSPARKLE], anItr->mX - 5, anItr->mY - 5,
				Rect(anItr->mFrame * 10, 0, 10, 10));
		else
			g->DrawImage(mApp->mImages[IMAGE_SMSPARKLE], anItr->mX - 10, anItr->mY - 10,
				Rect(anItr->mFrame * 20, 0, 20, 20));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}
