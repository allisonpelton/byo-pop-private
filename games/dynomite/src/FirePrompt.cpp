#include "FirePrompt.h"
#include "Dynomite.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"

using namespace Sexy;

// 0x431fc0
FirePrompt::FirePrompt(FossilizedApp* theApp, Dynomite* theBoard)
{
	mApp = theApp;
	mBoard = theBoard;
	mAlpha = 0xff;
	mFade = -1;
	mOwnLine1 = "";
	mOwnLine2 = "";
	mShowOwn = true;
	mPulses = 4.0f;
	Reset();
}

// 0x431f80
FirePrompt::~FirePrompt()
{
}

// 0x431e50: the alpha pulses between 128 and 255; each time it turns at the bottom a pulse is counted, and after
// five the lines change
void FirePrompt::Update()
{
	if (mBoard->mField16c == false)
	{
		int aFade = mFade;
		mAlpha = mAlpha + aFade * 4;
		if (mAlpha < 0x80)
		{
			mFade = 1;
			if (aFade != 1)
			{
				mPulses = mPulses - 1.0f;
				if (mPulses < 0.0f)
				{
					mPulses = 4.0f;
					mShowOwn = mShowOwn != true;
				}
			}
		}
		if (mAlpha > 0xff)
		{
			mAlpha = 0xff;
			mFade = -1;
		}
	}
	else if (0 < mAlpha)
	{
		mAlpha = mAlpha - 2;
		if (mAlpha < 0)
			mAlpha = 0;
	}
}

// 0x431ee0
void FirePrompt::Reset()
{
	mAlpha = 0;
	mFade = 1;
}

// 0x432090: centred on the launcher at 340 and 360, with a shadow 2 to the right and below
void FirePrompt::Draw()
{
	if ((mAlpha != 0) && (mBoard->mField244 != true) && (mApp->mInTutorial != true))
	{
		Graphics* g = GetDrawGraphics();
		g->SetFont(mApp->mWhiteFont);
		std::string aLine1 = mApp->GetString("FIRE_YOUR_FIRST_EGG_TO_BEGIN1");
		std::string aLine2 = mApp->GetString("FIRE_YOUR_FIRST_EGG_TO_BEGIN2");
		if ((mShowOwn == true) && (mOwnLine1 != ""))
		{
			aLine1 = mOwnLine1;
			aLine2 = mOwnLine2;
		}
		int aHalf1 = mApp->mWhiteFont->StringWidth(aLine1) >> 1;
		int aHalf2 = mApp->mWhiteFont->StringWidth(aLine2) >> 1;
		g->SetColor(Color(0, 0, 0, mAlpha));
		g->DrawString(aLine1, (mBoard->mField70 - aHalf1) + 2, 0x156);
		g->DrawString(aLine2, (mBoard->mField70 - aHalf2) + 2, 0x16a);
		g->SetColor(Color(0, 200, 0xff, mAlpha));
		g->DrawString(aLine1, mBoard->mField70 - aHalf1, 0x154);
		g->DrawString(aLine2, mBoard->mField70 - aHalf2, 0x168);
	}
}
