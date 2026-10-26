#include "BonusText.h"

#include "Color.h"
#include "Font.h"
#include "Graphics.h"
#include "SexyAppBase.h"

using namespace JeffLib;
using namespace Sexy;

BonusText::BonusText(const std::string& theText, Font* theFont, float x, float y, float theSpeed, int theFadeDelay)
	: mFadeDelay(theFadeDelay), mSpeed(theSpeed)
{
	mX = x > -1.0f ? x : (float)((gSexyAppBase->mWidth - theFont->StringWidth(theText)) / 2);
	mY = y > -1.0f ? (float)theFont->GetAscent() + y : (float)((gSexyAppBase->mHeight - theFont->GetHeight()) / 2);
	mText = theText;
	mFont = theFont;
	mWidth = theFont->StringWidth(mText);
}

BonusText::BonusText(const std::string& theText, Font* theFont, float x, float theTargetY, int theHoldUpdates,
                     float theSpeed)
	: mX(x), mY(480.0f), mFadeDelay(0), mHasTarget(true), mTargetX(x), mTargetY(theTargetY),
	  mHoldUpdates(theHoldUpdates), mSpeed(theSpeed)
{
	mText = theText;
	mFont = theFont;
	mWidth = theFont->StringWidth(mText);
}

void BonusText::Update()
{
	mUpdateCnt++;
	if (mDead)
		return;
	if (!mHasTarget) {
		mY -= mSpeed;
	} else if (mY > mTargetY) {
		mY -= mSpeed;
		if (mY <= mTargetY) {
			mY = mTargetY;
			mHoldStart = mUpdateCnt;
		}
	} else if (mY < mTargetY || mUpdateCnt - mHoldStart >= mHoldUpdates) {
		// (at the target: it waits there for mHoldUpdates)
		mY -= mSpeed;
	}
	if (mY < (float)-mFont->mHeight)
		mDead = true;
	if (!mWhite) {
		mLife--;
		mHue = (mHue + 7) % 255;
		if (mLife < 1) {
			mLife = 0;
			if (mAlpha < 1)
				mDead = true;
		}
	}
	if (mHoldUpdates == 0x7fffffff && --mFadeDelay < 1) {
		mAlpha -= 2;
		if (mAlpha < 0) {
			mAlpha = 0;
			if (mWhite || mLife < 1)
				mDead = true;
		}
	}
}

void BonusText::Draw(Graphics* g)
{
	if (mDead)
		return;
	// (the original also worked out an alpha from the life left, min(mLife / 18, 1), which the colour's alpha
	// then replaced)
	g->SetFont(mFont);
	Color aColor = mWhite ? Color(255, 255, 255) : Color((int)(gSexyAppBase->HSLToRGB(mHue, 255, 128) & 0xffffff));
	aColor.mAlpha = mAlpha;
	g->SetColor(aColor);
	g->DrawString(mText, (int)mX, (int)((float)mFont->GetAscent() + mY));
}
