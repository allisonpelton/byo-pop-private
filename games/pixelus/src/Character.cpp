#include "Character.h"
#include "PixelusApp.h"
#include "Widget.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Res.h"

using namespace Sexy;

namespace
{
	struct Step
	{
		int		mPose;			// -1 ends the sequence
		int		mTime;
	};

	// 0x5105e0: five sequences of at most 50 steps, 8 bytes each
	const Step gAnimations[Character::NUM_ANIMATIONS][Character::ANIMATION_ENTRIES] =
	{
		{ { 2, 10 }, { 0, 30 }, { 2, 10 }, { 3, 100 }, { -1, 0 } },
		{ { 2, 10 }, { 1, 30 }, { -1, 0 } },
		{ { 3, 20 }, { 1, 30 }, { -1, 0 } },
		{ { 2, 8 }, { 3, 20 }, { 2, 8 }, { 1, 100 }, { -1, 0 } },
		{ { 0, 30 }, { 1, 30 }, { 2, 10 }, { -1, 0 } }
	};
}

// 0x42c4a0: mode 0 and 1 name the tutorial's first two hints; mode 2 picks one of the six that follow at random. Any
// other mode (the temple passes the picture of the map behind it) leaves the first one as it was in memory: the original
// reads it uninitialised, the port starts it at the first of the random ones.
SpeechBubble::SpeechBubble(PixelusApp* theApp, int theGod, Widget* theOwner, int theMode)
{
	mApp = theApp;
	mGod = theGod;
	mOwner = theOwner;
	mTip = 3;
	if (theMode == 0)
		mTip = 1;
	else if (theMode == 1)
		mTip = 2;
	else if (theMode == 2)
		mTip = theApp->AppRandNext() % 6 + 3;
	int aRand = theApp->AppRandNext();
	mNextTip = aRand % 6 + 3;
	if (mNextTip == mTip)
		mNextTip = (aRand % 6 + 1) % 6 + 3;
	mPhase = 0;
	mPeriod = 1000;
	mDelay = 100;
	mAlpha = -1;
	mLeaving = false;
}

// 0x42c590
void SpeechBubble::Update()
{
	if (mDelay != 0)
	{
		mDelay--;
		return;
	}
	if (mTip == -1)
		return;

	mPhase += mLeaving ? 2 : 1;
	if (mPeriod < mPhase)
	{
		if (!mLeaving)
		{
			mTip = mNextTip;
			int aRand = mApp->AppRandNext();
			mNextTip = aRand % 6 + 3;
			if (mNextTip == mTip)
				mNextTip = (aRand % 6 + 1) % 6 + 3;
			mDelay = 500;
		}
		else
		{
			mTip = 0;
			mDelay = 0;
			mLeaving = false;
		}
		mPhase = 0;
	}
	int anAlpha;
	if (mPhase < 0x80)
		anAlpha = mPhase * 2;
	else if (mPeriod - mPhase < 0x80)
		anAlpha = (mPeriod - mPhase) * 2;
	else
		anAlpha = 0xff;
	if (anAlpha != mAlpha)
	{
		mAlpha = anAlpha;
		if (mOwner != NULL)
			mOwner->MarkDirty();
	}
}

// 0x53d4f8: the texts of the bubbles, nine to a god (the first of them, for the tip 0 that follows a bubble, is drawn as
// the original does)
static const unsigned int gBubbleTexts[10][9] =
{
	{ 0x5105a4, 0x510564, 0x510530, 0x510508, 0x5104c8, 0x510498, 0x510468, 0x510434, 0x51040c },
	{ 0x5103f4, 0x5103cc, 0x5103ac, 0x51037c, 0x510350, 0x510314, 0x5102e4, 0x5102ac, 0x510280 },
	{ 0x510260, 0x510234, 0x510204, 0x5101cc, 0x5101a0, 0x51016c, 0x51013c, 0x510110, 0x5100d4 },
	{ 0x5100b0, 0x51007c, 0x510050, 0x510024, 0x50fff4, 0x50ffc0, 0x50ff98, 0x50ff60, 0x50ff4c },
	{ 0x50ff38, 0x50ff04, 0x50fed4, 0x50fe94, 0x50fe58, 0x50fe2c, 0x50fdf4, 0x50fdbc, 0x50fd80 },
	{ 0x50fd5c, 0x50fd28, 0x50fcec, 0x50fcb8, 0x50fc94, 0x50fc6c, 0x50fc30, 0x50fc24, 0x50fbec },
	{ 0x50fbd0, 0x50fbac, 0x50fb80, 0x50fb58, 0x50fb38, 0x50fb14, 0x50fb00, 0x50fadc, 0x50faa8 },
	{ 0x50fa8c, 0x50fa60, 0x50fa30, 0x50fa08, 0x50f9d0, 0x50f9b0, 0x50f988, 0x50f958, 0x50f918 },
	{ 0x50f8ec, 0x50f8b0, 0x50f884, 0x50f858, 0x50f838, 0x50f7f8, 0x50f7e0, 0x50f7ac, 0x50f77c },
	{ 0x50f754, 0x50f720, 0x50f6f8, 0x50f6dc, 0x50f6b0, 0x50f688, 0x50f64c, 0x50f618, 0x50f5e8 },
};

// 0x431c70: how many lines the text takes in a box of the given width (the same wrapping as the drawing's)
static int CountLines(ImageFont* theFont, int theWidth, const std::string& theText)
{
	int aLines = 0, aLineStart = 0, aWidth = 0, aSpace = -1;
	char aPrev = 0;
	int aLength = (int) theText.length();
	int aPos = 0;
	while (aPos < aLength)
	{
		char aChar = theText[aPos];
		int anAfter = aPos;
		if (aChar == ' ')
			aSpace = aPos;
		else if (aChar == '\n')
		{
			aWidth = theWidth + 1;
			aSpace = aPos;
			anAfter = aPos + 1;
		}
		aWidth += theFont->CharWidthKern(aChar, aPrev);
		aPrev = aChar;
		if (aWidth > theWidth)
		{
			if (aSpace == -1)
			{
				if (anAfter < aLineStart + 1)
					anAfter++;
			}
			else
				anAfter = aSpace + 1;
			aSpace = -1;
			aWidth = 0;
			aPrev = 0;
			aLines++;
			aLineStart = anAfter;
		}
		else
			anAfter++;
		aPos = anAfter;
	}
	if (aLineStart < aLength)
		aLines++;
	return aLines;
}

// 0x42c930
void SpeechBubble::Draw(Graphics* g)
{
	if (mDelay != 0 || mTip == -1)
		return;
	int anAlpha;
	if (mPhase < 0x80)
		anAlpha = mPhase * 2;
	else if (mPeriod - mPhase < 0x80)
		anAlpha = (mPeriod - mPhase) * 2;
	else
		anAlpha = 0xff;
	DrawText(g, mTip, anAlpha);
}

// 0x42c660: the bubble at the god's side and what it says, the lines centred in its box by their number
void SpeechBubble::DrawText(Graphics* g, int theTip, int theAlpha)
{
	int aX = 2, aY = 0x32;
	if (mGod == 0)
	{
		aX = 0x1e;
		aY = 0x46;
	}
	if (theAlpha != 0xff || theTip == 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(theTip == 0 ? Color(255, 128, 128, theAlpha) : Color(255, 255, 255, theAlpha));
	}
	g->DrawImage(mApp->mImages[IMAGE_SPEECHBUBBLE], aX, aY);
	if (theAlpha == 0xff)
	{
		g->SetColorizeImages(false);
		g->SetColor(Color(0, 0, 0));
	}
	else
		g->SetColor(Color(0, 0, 0, theAlpha));

	ImageFont* aFont = mApp->mFonts[FONT_POSEIDON];
	g->SetFont(aFont);
	int aSpacing = aFont->GetLineSpacing() - 4;
	const std::string& aText = GT(gBubbleTexts[mGod][theTip]);
	Rect aRect(aX + 0x14, aY + 0x18, 0x7d, 0x78);
	switch (CountLines(aFont, aRect.mWidth, aText))
	{
	case 1:
		aRect.mY += aSpacing;
		break;
	case 2:
		aRect.mY += aSpacing / 2;
		break;
	case 4:
		aRect.mY -= aSpacing / 2;
		aSpacing -= 1;
		break;
	case 5:
		aRect.mY += 8 - aSpacing;
		aSpacing -= 5;
		break;
	}
	WriteWordWrapped2004(g, aRect, aText, aSpacing, 0);
	g->SetColorizeImages(false);
}

// 0x42c550: the text goes now
void SpeechBubble::Hurry()
{
	int aPeriod = mPeriod;
	mLeaving = true;
	if (mDelay != 0)
	{
		mDelay = 0;
		mPhase = aPeriod;
		return;
	}
	if (mPhase < aPeriod - 0x80)
	{
		if (mPhase < 0x81)
			mPhase = aPeriod - mPhase;
		else
			mPhase = aPeriod - 0x80;
	}
}

// 0x42e110
Character::Character(PixelusApp* theApp, int theGod, Widget* theOwner, int theMode)
{
	mApp = theApp;
	mTick = 0;
	mAngle = 0;
	mSpeed = 0.0008;
	mGod = theGod;
	mAnimation = 0;
	mAnimTick = 0;
	mAnimEntry = 0;
	mPose = 2;
	mFade = 0xff;
	mPrevPose = 2;
	mBlink = -1;
	mBlinkAngle = 1.5;
	mBubble = theOwner != NULL ? new SpeechBubble(theApp, theGod, theOwner, theMode) : NULL;
}

Character::~Character()
{
	delete mBubble;
}

// 0x42df40
void Character::Update(Widget* theWidget)
{
	double aSpeed = mAngle + mSpeed;
	mTick++;
	mAngle = aSpeed;
	mSpeed = mSpeed - aSpeed * 0.0004;
	mAngle = mAngle * 0.99;
	if (mApp->AppRandNext() % 300 == 0)
		mSpeed = mSpeed + 0.0005;

	if (mAnimation != NO_ANIMATION)
	{
		mAnimTick++;
		if (gAnimations[mAnimation][mAnimEntry].mTime * 2 <= mAnimTick)
		{
			mAnimEntry++;
			mAnimTick = 0;
			int aPose = gAnimations[mAnimation][mAnimEntry].mPose;
			if (aPose == -1)
			{
				mAnimation = NO_ANIMATION;
				aPose = 2;
			}
			if (aPose != mPose)
			{
				mPrevPose = mPose;
				mPose = aPose;
				mFade = 0;
			}
		}
	}

	if (mFade < 0xff)
	{
		mFade += 0x18;
		if (mFade > 0xfe)
			mFade = 0xff;
	}

	int aRand = mApp->RandNext();
	if (aRand % 250 == 0 && mAnimation == NO_ANIMATION)
	{
		aRand = mApp->RandNext();
		mAnimEntry = 0;
		mAnimTick = 0;
		mAnimation = aRand % 5;
		int aPose = gAnimations[mAnimation][0].mPose;
		if (aPose != mPose)
		{
			mPrevPose = mPose;
			mPose = aPose;
			mFade = 0;
		}
	}

	if (theWidget != NULL)
		theWidget->MarkDirty();

	if (mGod == 9)
	{
		if (mBlink == -1)
		{
			if (mApp->RandNext() % 400 == 0)
			{
				mBlink = 0;
				if ((mApp->RandNext() & 1) == 0)
					mBlinkAngle = (mApp->RandNext() % 200) * 0.001 + 1.4;
				else
					mBlinkAngle = 0.10000000149011612 - (mApp->RandNext() % 200) * 0.001;
			}
		}
		else
		{
			mBlink++;
			if (mBlink > 100)
				mBlink = -1;
		}
	}

	if (mBubble != NULL)
		mBubble->Update();
}

// 0x42de40: a click on the figure hurries its bubble
void Character::Poke()
{
	if (mBubble != NULL)
		mBubble->Hurry();
}
