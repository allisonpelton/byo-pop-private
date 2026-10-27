#include "MenuTalker.h"
#include "FossilizedApp.h"
#include "InchySmall.h"
#include "Sprite.h"
#include "Widget.h"

using namespace Sexy;

// 0x422d90
MenuTalker::MenuTalker(FossilizedApp* theApp, Widget* theOwner, int theMode)
{
	mApp = theApp;
	mOwner = theOwner;
	mTalk = new TalkSequence();
	mFrame = 1;
	mCountdown = 0xaf;
	mMode = theMode;
}

// 0x422e10
MenuTalker::~MenuTalker()
{
	delete mTalk;
}

// 0x422e30: he pops up when the countdown reaches 50 and starts talking when it reaches 1
void MenuTalker::Update()
{
	if (mCountdown == 1)
	{
		mOwner->MarkDirty();
		mTalk->Reset();
		GameSound* aSound;
		if (mMode == 1)
		{
			mTalk->AddStep(0, 5);
			mTalk->AddStep(1, 8);
			mTalk->AddStep(-1, 5);
			mTalk->AddStep(3, 10);
			mTalk->AddStep(0, 10);
			mTalk->AddStep(2, 10);
			mTalk->AddStep(0, 10);
			mTalk->AddStep(3, 10);
			mTalk->AddStep(1, 10);
			aSound = mApp->mSndXwelcometodynomite;
		}
		else
		{
			if (mMode != 2)
				goto count;
			mTalk->AddStep(2, 5);
			mTalk->AddStep(1, 0xf);
			mTalk->AddStep(3, 10);
			mTalk->AddStep(0, 0xf);
			mTalk->AddStep(2, 10);
			aSound = mApp->mSndXgameover;
		}
		mTalk->Play(aSound);
	}
count:
	mCountdown--;
	if (mCountdown == 0x32)
	{
		mOwner->MarkDirty();
		mFrame = 0;
		mApp->mSndInchypop->Play();
	}
	if (mCountdown < 0)
		mCountdown = 0;
	mTalk->Update();
	if (mTalk->mChanged == true)
	{
		mTalk->mChanged = false;
		mOwner->MarkDirty();
	}
}

// 0x422fb0
void MenuTalker::Draw()
{
	int x = 0x14;
	int y = 0x14;
	if (mMode == 2)
	{
		x = 0x28;
		y = 0x3c;
	}
	mApp->mInchyLargeCells[mFrame]->Draw(x, y);
	TalkStep* aStep = mTalk->mCurrent;
	if ((aStep != NULL) && (aStep->mMouth != -1))
		mApp->mInchyLargeMouthCells[aStep->mMouth]->Draw(x + 0x48, y + 0x58);
}
