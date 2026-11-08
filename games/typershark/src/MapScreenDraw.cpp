// MapScreenDraw.cpp: MapScreen::Draw (0x447400) and the easing it uses (ScrollSmoother, inlined three times in the
// original's Draw). See MapScreen.cpp for the rest of the class and the quirks.

#include "MapScreen.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "HyperlinkWidget.h"
#include "GameText.h"
#include "Graphics.h"
#include "Common.h"
#include "Font.h"
#include "ImageFont.h"
#include "Image.h"
#include "Res.h"
#include "compat2004/Compat2004.h"
#include <math.h>

using namespace Sexy;

// the constructor 0x458d10 zeroes the ring (twelve dwords per six doubles), the value, the index, the unused double and the
// previous target of each of the three
ScrollSmoother::ScrollSmoother()
{
	for (int i = 0; i < 6; i++)
		mRing[i] = 0.0;
	mValue = 0.0;
	mIndex = 0;
	mUnused = 0.0;
	mPrevious = 0.0;
}

// The easing that Draw (0x447400) repeats for the map's x, the map's y and the prompt's x: the target's change since the
// last draw goes into a ring of six, the average of the ring is the speed, and the eased value moves by the speed until the
// move would be about to pass the target (|speed| not above the distance that is left after two moves), where it takes the
// target. The test is the x87 compare of |average| with |average + (average + value) - target| (jp taken when the first is
// greater or the two are unordered): the value keeps moving then, otherwise it is set to the target.
void ScrollSmoother::Step(int theTarget)
{
	double aPrevious = mPrevious;
	mPrevious = (double)theTarget;
	mRing[mIndex] = (double)theTarget - aPrevious;
	mIndex = (mIndex + 1) % 6;

	double aSum = 0.0;
	double* aRing = mRing;
	int aCount = 6;
	do
	{
		aSum = aSum + *aRing;
		aRing = aRing + 1;
		aCount = aCount - 1;
	} while (aCount != 0);

	double anAverage = aSum * 0.16666666666666666;
	double aMoved = anAverage + mValue;
	double aTarget = (double)theTarget;
	double aLeft = fabs((anAverage + aMoved) - aTarget);
	if (fabs(anAverage) <= aLeft)
		aMoved = aTarget;
	mValue = aMoved;
}

// 0x447400 slot 16. The map picture is drawn at the eased scroll position (the eased values only move while the final
// scroll runs or the prompt slides in). Over it: the black bar with "Level N" and what to do next (not on the final
// panel); on the final panel the black box with Gumbo, "LEVEL N", the story and the rating; on the map the route dots (the
// ones behind the ship dim and white, the sailed ones green, the ones of the other ways dim), a stamp on each site sailed
// through, and the ship. Last the arch over the bottom, the black strip behind the continue link, the "Type the letter..."
// prompt, and the old tooltip.
void MapScreen::Draw(Graphics* g)
{
	if (mScrolling != false)
	{
		mSmoothX.Step(mScrollX);
		mSmoothY.Step(mScrollY);
	}
	if (mContinueLink->mVisible == false && mFlag234 == false)
		mSmoothText.Step(mPromptX);

	g->DrawImage(mApp->mImages[IMAGE_MAPBG], (int)mSmoothX.mValue, (int)mSmoothY.mValue);
	g->SetColor(Color(0, 0, 0, 0xb4));
	if (mFinal == false)
		g->FillRect(0, 0, mWidth, 0x1e);
	g->SetFont(mApp->mFontMyriadCondensed14Bold);
	g->SetColor(Color(0xff, 0xff, 0xff));
	std::string aLevelText = StrFormat(GTC(0x0052fad4), mField1f8);
	if (mFinal == false)
		g->DrawString(aLevelText, 10, 0x13);
	g->SetFont(mApp->mFontRockwell13);
	g->SetColor(Color(0, 0xff, 0));
	std::string aPromptText;
	if (mFlag224 != false)
		aPromptText = GT(0x0052fab4) + mApp->GetShipName();		// 0x4292b0
	else
		aPromptText = GT(0x0052fa90);

	if (mFlag234 == false)
	{
		if (mFinal == false)
			g->DrawString(aPromptText, mApp->mFontMyriadCondensed14Bold->StringWidth(aLevelText) + 0x28, 0x13);
	}

	if (mFinal != false)
	{
		if (mFlag234 == false)
		{
			g->SetColor(Color(0, 0, 0, 0xb4));
			g->FillRect(100, 0x3c, 0x1b8, 0xcc);
			g->SetFont(mApp->mFontCourierFinalDraft15Bold);
			g->SetColor(Color(0xff, 0xff, 0xff));
			Rect aGumboRect(mGumboFrame * 0x3c, 0, 0x3c, 0x3c);
			g->DrawImage(mApp->mImages[IMAGE_GUMBO_SWIM], 0x80, 0x69, aGumboRect);
			WriteString2004(g, StrFormat(GTC(0x0052fa7c), mField1f8), 0x78, 0x5f, 0x190, 0, true, 0, -1);
			g->SetFont(mApp->mFontRockwell13);
			WriteWordWrapped2004(g, Rect(0xcd, 0x73, 0x145, 0xc8), GT(0x0052f9f8),
				mApp->mFontCourierFinalDraft15Bold->GetHeight(), -1);
			WriteWordWrapped2004(g, Rect(0xcd, 0xe1, 0x145, 0xc8),
				StrFormat(GTC(0x0052f9d0), mDifficultyName.c_str(), (int)mDifficultyPercent), 0xc, -1);
		}

		// 0x447cd3: Gumbo's ship shows once the map has scrolled all the way
		Image* aMapImage = mApp->mImages[IMAGE_MAPBG];
		if (mScrollX == mApp->mWidth - aMapImage->mWidth && mScrollY == mApp->mHeight - aMapImage->mHeight)
		{
			Rect aShipRect(mShipFrame * 0x32, 0, 0x32, 0x2d);
			g->DrawImage(mApp->mImages[IMAGE_MAPSHIPCOLOR], 0x1e6, 0x193, aShipRect);
		}
	}
	else
	{
		// 0x447a28: the route dots. 0..mDotsDone dim white, mDotsDone..mDotIndex green (the ship has sailed them), then the
		// rest of the route and the dots of the other ways dimmer
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xa8));
		int i = 0;
		if (0 < mDotsDone)
		{
			do
			{
				ProfilePair* aDot = mVector1c4[i];
				g->DrawImage(mApp->mImages[IMAGE_POINT], aDot->mFirst, aDot->mSecond);
				i++;
			} while (i < mDotsDone);
		}
		g->SetColorizeImages(false);
		i = mDotsDone;
		if (i < mDotIndex)
		{
			do
			{
				ProfilePair* aDot = mVector1c4[i];
				g->DrawImage(mApp->mImages[IMAGE_POINT_GREEN], aDot->mFirst, aDot->mSecond);
				i++;
			} while (i < mDotIndex);
		}
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, 0x80));
		unsigned int aDotIndex = mDotIndex;
		while (true)
		{
			if (aDotIndex >= mVector1c4.size())
				break;
			ProfilePair* aDot = mVector1c4[aDotIndex];
			g->DrawImage(mApp->mImages[IMAGE_POINT_GREEN], aDot->mFirst, aDot->mSecond);
			aDotIndex++;
		}
		g->SetColorizeImages(false);
		g->SetColor(Color(0xff, 0xff, 0xff, 0x80));
		g->SetColorizeImages(true);
		unsigned int aOtherIndex = 0;
		while (true)
		{
			if (aOtherIndex >= mVector198.size())
				break;
			ProfilePair* aDot = mVector198[aOtherIndex];
			g->DrawImage(mApp->mImages[IMAGE_POINT_GREEN], aDot->mFirst, aDot->mSecond);
			aOtherIndex++;
		}
		g->SetColorizeImages(false);

		// a stamp on every site sailed through but the ship's (the ship's is the last, or the one before while it sails on)
		int aStamps;
		if (mHeadingOn == false)
			aStamps = (int)mVector1d4.size() - 1;
		else
			aStamps = (int)mVector1d4.size() - 2;
		i = 0;
		if (0 < aStamps)
		{
			do
			{
				MapNode* aNode = mVector1d4[i];
				Image* aStamp = mApp->mImages[IMAGE_MAPSHIP];
				g->DrawImage(aStamp, aNode->mX - aStamp->mWidth / 2, aNode->mY - aStamp->mHeight / 2);
				i++;
			} while (i < aStamps);
		}
		int aShipSite = aStamps;
		if (aStamps < 0)
			aShipSite = 0;
		Rect aShipRect(mShipFrame * 0x32, 0, 0x32, 0x2d);
		MapNode* aShipNode = mVector1d4[aShipSite];
		g->DrawImage(mApp->mImages[IMAGE_MAPSHIPCOLOR], aShipNode->mX - 0x19, aShipNode->mY - 0x16, aShipRect);
	}

	// 0x447d42: the arch lies over the bottom of the picture and moves with it
	g->DrawImage(mApp->mImages[IMAGE_MAPSCREEN_ARCH], (int)(mSmoothX.mValue + 443.0), (int)(mSmoothY.mValue + 341.0));
	g->SetColor(Color(0, 0, 0, 0xb4));
	g->FillRect(Rect(0, mContinueLink->mY, mApp->mWidth, mApp->mHeight - mContinueLink->mY));

	if (mContinueLink->mVisible == false && mFlag234 == false && mApp->GetDialog(DIALOG_MESSAGE) == NULL)
	{
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(0xff, 0xff, 0xff));
		std::string aTypeText = GT(0x0052f988);
		g->DrawString(aTypeText, (int)mSmoothText.mValue, g->GetFont()->GetAscent() + mContinueLink->mY + 6);
	}

	// 0x447e92: the old tooltip box (the original compares the text with the empty string at 0x5297a0; nothing sets it)
	if (mTipText != "")
	{
		int aTextWidth = mApp->mFontRockwell13->StringWidth(mTipText);
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(0, 0, 0));
		int aX = mTipX - aTextWidth / 2;
		int aFontHeight = mApp->mFontRockwell13->mHeight;
		int aY = (mTipY - aFontHeight) - 5;
		if (aX < 0)
			aX = 0;
		if (mApp->mWidth < aX + 2 + aTextWidth)
			aX = (mApp->mWidth - aTextWidth) - 2;
		if (aY < 0)
			aY = 0;
		if (mApp->mHeight < aFontHeight + 2 + aY)
			aY = (mApp->mHeight - aFontHeight) - 2;
		g->FillRect(Rect(aX, aY, aTextWidth + 4, aFontHeight + 4));
		g->SetColor(Color(0xff, 0xff, 0xe1));
		g->FillRect(Rect(aX + 1, aY + 1, aTextWidth + 2, aFontHeight + 2));
		g->SetColor(Color(0, 0, 0));
		g->DrawString(mTipText, aX + 2, mApp->mFontRockwell13->GetAscent() + aY + 2);
	}
}
