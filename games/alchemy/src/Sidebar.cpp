// The side panel (see Sidebar.h).

#include "Sidebar.h"
#include "WinAlchApp.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Compat2004.h"

#include <cstdio>

using namespace Sexy;

// 0x40f987
Sidebar::Sidebar(WinAlchApp* theApp)
{
	mApp = theApp;
	mFill = 0;
	mHold = 0;
	mBubbleScroll = 0;
	mWaveFrame = 0;
	mWaveX = 0;
	mScoreAnim = 0;
	mFlashing = false;
	mFlashOn = false;
	mOnTarget = true;
}

// 0x40f8bc
void Sidebar::FlashLava(bool isFlashing)
{
	mFlashing = isFlashing;
	if (!isFlashing)
		mFlashOn = false;
}

// 0x40f8d3: the gauge is a button for the forge
void Sidebar::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	if ((mApp->mBoard != NULL) && (x > 0x26) && (x < 0x75) && (y > 0x142) && (y < 0x19b))
		mApp->mBoard->Discard();
}

// 0x40fa93
void Sidebar::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0));
	g->FillRect(0x25, 0x141, 0x48, 0x5b - mFill);
	g->DrawImage(mApp->mImages[IMAGE_FORGEBACK2], 0x28, 0x147);   // the disassembly: the decompiler had dropped the position

	g->SetColor(Color(255, 0, 0));
	if (mFill > 0)
	{
		Graphics aClipG(*g);
		aClipG.ClipRect(0x25, 0x19e - mFill, 0x48, mFill);
		aClipG.DrawImage(mApp->mImages[IMAGE_BUBBLES2], 0x25, 0x19e - mBubbleScroll - mFill);
		aClipG.DrawImage(mApp->mImages[IMAGE_BUBBLES2], 0x25, 0x205 - mBubbleScroll - mFill);

		Rect aWaveRect(0, mWaveFrame * 5, 0x6a, 5);
		g->DrawImage(mApp->mImages[IMAGE_WAVE1], 0x25 - mWaveX, 0x199 - mFill, aWaveRect);
		g->DrawImage(mApp->mImages[IMAGE_WAVE1], 0x8f - mWaveX, 0x199 - mFill, aWaveRect);

		for (std::list<Bubble>::iterator anItr = mBubbles.begin(); anItr != mBubbles.end(); ++anItr)
		{
			Bubble& aBubble = *anItr;
			if (!aBubble.mBurst)
				g->DrawImage(mApp->mImages[IMAGE_TINYBUBBLE1], aBubble.mX + 0x25, aBubble.mY - mFill + 0x19e);
			else
				g->DrawImage(mApp->mImages[IMAGE_BUBBLEBURST], aBubble.mX + 0x25, aBubble.mY - mFill + 0x19e,
					Rect(aBubble.mFrame * 10, 0, 10, 0xe));
		}
	}

	if (mFlashOn)
	{
		g->SetColor(Color(255, 255, 255, 128));
		g->FillRect(0x25, 0x141, 0x48, 0x5b);
	}

	// the backdrop's left part has the window for the tank
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(0, 0, 0x96, 0x1e0));

	g->SetColor(Color(255, 255, 255));
	g->SetFont(mApp->mScoreFont);
	char aBuf[256];
	sprintf(aBuf, "%d", (int) mScoreAnim);
	SexyString aScore = aBuf;
	g->DrawString(aScore, 0x49 - g->GetFont()->StringWidth(aScore) / 2, 0x43);

	// the skill and the mode labels, cut from glassbuttons: 3 rows of 20 for the skill, then the mode's two
	g->DrawImage(mApp->mImages[IMAGE_GLASSBUTTONS], 0x26, 0x95, Rect(0, mApp->mSkillLevel * 0x14 - 0x14, 0x42, 0x14));
	g->DrawImage(mApp->mImages[IMAGE_GLASSBUTTONS], 0x26, 0xb9,
		Rect(0, (mApp->mTimeTrial ? 0x14 : 0) + 0x3c, 0x42, 0x14));
}

// 0x40fead
void Sidebar::Update()
{
	Widget::Update();

	if (mFlashing && (mUpdateCnt % 8 == 0))
	{
		mFlashOn = !mFlashOn;
		MarkDirty();
	}

	Board* aBoard = mApp->mBoard;
	double aRate = 0.0;
	int aScore = 0;
	int aTarget = 0;
	if (aBoard != NULL)
	{
		aRate = aBoard->mRate;
		aScore = aBoard->mScore;
		double aProgress = aBoard->mProgress;
		if (aProgress > 3.0)
			aProgress = 3.0;
		aTarget = (int) (aProgress * 30.333333333333332);
	}

	mOnTarget = aTarget == mFill;
	if (mHold > 0)
		mHold--;
	else if (aTarget > mFill)
	{
		mFill++;
		MarkDirty();
	}
	else if (aTarget < mFill)
	{
		mFill--;
		MarkDirty();
	}

	if (mFill > 0)
	{
		if (mUpdateCnt % 8 == 0)
		{
			mBubbleScroll = (mBubbleScroll + 1) % 0x67;
			MarkDirty();
		}

		if (CrtRand() % 0x28 == 0)
		{
			Bubble aBubble;
			aBubble.mX = CrtRand() % 0x3e;
			aBubble.mY = 0x5b;
			aBubble.mSpeed = (CrtRand() % 1000) * 0.0002 + 0.2;
			mBubbles.push_back(aBubble);
		}

		for (std::list<Bubble>::iterator anItr = mBubbles.begin(); anItr != mBubbles.end();)
		{
			Bubble& aBubble = *anItr;
			bool wasErased = false;
			aBubble.mAccum += aBubble.mSpeed;
			if (aBubble.mAccum > 1.0)
			{
				do
				{
					if (!aBubble.mBurst)
					{
						aBubble.mY--;
						if (aBubble.mY < -1)
						{
							aBubble.mAccum = 0;
							aBubble.mBurst = true;
							aBubble.mY -= 4;
							aBubble.mSpeed = 0.4;
						}
					}
					else
					{
						aBubble.mFrame++;
						if (aBubble.mFrame > 7)
							wasErased = true;
					}
					aBubble.mAccum -= 1.0;
				} while (aBubble.mAccum > 1.0);
			}
			if (wasErased)
				anItr = mBubbles.erase(anItr);
			else
				++anItr;
		}

		if (mUpdateCnt % 8 == 0)
		{
			mWaveFrame = (mWaveFrame + 1) % 7;
			MarkDirty();
		}
		if (mUpdateCnt % 8 == 0)
		{
			mWaveX = (mWaveX + 1) % 0x6a;
			MarkDirty();
		}
	}

	// the shown score runs after the real one
	if (aRate == 0.0)
	{
		if (mScoreAnim != 0.0)
		{
			mScoreAnim = 0.0;
			MarkDirty();
		}
		return;
	}

	double aReal = (double) aScore;
	double aDiff = aReal - mScoreAnim;
	double aStep = 0;
	bool doStep = false;
	if (aDiff < aRate * 150.0)
	{
		if (aDiff >= aRate * 100.0)
		{
			aStep = aRate + aRate;
			doStep = true;
		}
		else if ((aRate * 50.0 <= aDiff) || ((mScoreAnim < aReal) && (mUpdateCnt % 2 == 0)))
		{
			aStep = aRate;
			doStep = true;
		}
	}
	else
	{
		aStep = aRate * 3.0;
		doStep = true;
	}
	if (doStep)
	{
		mScoreAnim += aStep;
		MarkDirty();
	}

	aDiff = mScoreAnim - aReal;
	if (aDiff < aRate * 150.0)
	{
		if (aDiff < aRate * 100.0)
		{
			if (aDiff < aRate * 50.0)
			{
				if (aReal >= mScoreAnim)
					return;
				if (mUpdateCnt % 2 != 0)
					return;
			}
			aStep = aRate;
		}
		else
			aStep = aRate + aRate;
	}
	else
		aStep = aRate * 3.0;
	mScoreAnim -= aStep;
	MarkDirty();
}
