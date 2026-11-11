#include "BonusText.h"
#include "Graphics.h"
#include "Font.h"

using namespace Sexy;

// 0x4621a0: gold (255, 180, 60) and opaque, state 0, rising by 0.5 per tick (the speed in x is the 0 that GameObject's
// constructor leaves)
BonusText::BonusText(const std::string& theText, Font* theFont) :
	mText(theText)
{
	mRed = 0xff;
	mAlpha = 0xff;
	mFont = theFont;
	mState = 0;
	mGreen = 0xb4;
	mBlue = 0x3c;
	mVelY = 0.5;
}

BonusText::~BonusText()
{
}

// 0x460fd0 slot 1: the text moves by the velocity and is finished (state 1) when it has gone above the top of the
// screen by its font's height. The colour fades by 2 a tick: first the blue to 0, then the green down to 100 and below
// (the blue stays 0), then the alpha (the green is set to 0) until it is 0, when the text is finished
void BonusText::Update()
{
	if (mState != 1)
	{
		mX = mVelX + mX;
		mY = mY - mVelY;
		if (mY < (double)(-mFont->mHeight))
			mState = 1;

		if (mBlue > 0)
		{
			mBlue -= 2;
			if (mBlue < 0)
				mBlue = 0;
			return;
		}
		if (mGreen > 100)
		{
			mBlue = 0;
			mGreen -= 2;
			if (mGreen < 0)
				mGreen = 0;
			return;
		}
		if (mAlpha > 0)
		{
			mAlpha -= 2;
			if (mAlpha < 0)
				mAlpha = 0;
			mGreen = 0;
			return;
		}
		mAlpha = 0;
		mState = 1;
	}
}

// 0x4610c0 slot 2: the text in its colour at the position (no history is recorded)
void BonusText::Draw(Graphics* g)
{
	if (mState != 1)
	{
		g->SetFont(mFont);
		g->SetColor(Color(mRed, mGreen, mBlue, mAlpha));
		g->DrawString(mText, (int)mX, (int)mY);
	}
}
