#include "InchySmall.h"
#include "Launcher.h"
#include "Dynomite.h"
#include "FossilizedApp.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x42be60
TalkSequence::TalkSequence()
{
	mPlaying = false;
	mCurrent = NULL;
	mSteps.Clear();
	mChanged = false;
	mDone = false;
}

// 0x42bfc0
TalkSequence::~TalkSequence()
{
	Reset();
}

// 0x42bec0
void TalkSequence::Reset()
{
	mSteps.ResetIter();
	TalkStep* aStep = mSteps.Next();
	while (aStep != NULL)
	{
		delete aStep;
		aStep = mSteps.Next();
	}
	mSteps.Clear();
	mPlaying = false;
	mCurrent = NULL;
	mCountdown = 0;
	mChanged = true;
}

// 0x42bf20
void TalkSequence::AddStep(int theMouth, int theFrames)
{
	TalkStep* aStep = new TalkStep;
	aStep->mMouth = theMouth;
	aStep->mFrames = theFrames;
	mSteps.Add(aStep);
}

// 0x42bf50
void TalkSequence::Play(GameSound* theSound)
{
	mSteps.ResetIter();
	mPlaying = true;
	mCountdown = 0;
	mDone = false;
	theSound->Play();
}

// 0x42bf80
void TalkSequence::Update()
{
	if (mPlaying && (--mCountdown < 1))
	{
		mCurrent = mSteps.Next();
		if (mCurrent == NULL)
		{
			mPlaying = false;
			mDone = true;
			mChanged = true;
			return;
		}
		mCountdown = mCurrent->mFrames;
		mChanged = true;
	}
}

// 0x423090
InchyBubble::InchyBubble(InchySmall* theInchy)
{
	mInchy = theInchy;
}

// 0x4230b0
InchyBubble::~InchyBubble()
{
}

// 0x4230c0
void InchyBubble::Draw(Graphics* g)
{
	SetDrawGraphics(g);
	int aSpeech = mInchy->mSpeech;
	if ((-1 < aSpeech) && (aSpeech < 7))
	{
		mInchy->mApp->mPicTextballoons_c2->Draw(0, 0);
		mInchy->mApp->mComboTextCells[mInchy->mSpeech]->Draw(0x1a, 0x13);
	}
}

// 0x423800
InchySmall::InchySmall(FossilizedApp* theApp, Widget* theOwner)
{
	mOwner = theOwner;
	mApp = theApp;
	mState = 0;
	mCounter = 0;
	mEvents.Clear();
	mBlink = false;
	mBlinkCounter = 0;
	mBubble = NULL;
	mSpeech = -1;
}

// 0x4238a0
InchySmall::~InchySmall()
{
	HideBubble();
	if (mBubble != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mBubble);
		mApp->SafeDeleteWidget(mBubble);
		mBubble = NULL;
	}
	mEvents.ResetIter();
	InchyEvent* anEvent = mEvents.Next();
	while (anEvent != NULL)
	{
		delete anEvent;
		anEvent = mEvents.Next();
	}
}

// 0x423010
void InchySmall::Rise(int theDelay)
{
	if (mState == 0)
	{
		InchyEvent* anEvent = new InchyEvent;
		anEvent->mDelay = theDelay;
		anEvent->mAction = 0;
		mEvents.Add(anEvent);
	}
}

// 0x423040
void InchySmall::Sink(int theDelay)
{
	if (mState == 5)
	{
		InchyEvent* anEvent = new InchyEvent;
		anEvent->mDelay = theDelay;
		anEvent->mAction = 1;
		mEvents.Add(anEvent);
	}
}

// 0x423070
void InchySmall::HideBubble()
{
	if (mBubble != NULL)
		mBubble->Resize(-500, -500, 0x78, 0x47);
}

// 0x423120
void InchySmall::CreateBubble()
{
	mBubble = new InchyBubble(this);
	mApp->mWidgetManager->AddWidget(mBubble);
}

// 0x423190
void InchySmall::Draw()
{
	switch (mState)
	{
	case 1:
	case 4:
		mApp->mInchyCells[0]->Draw(0x28, 0x139);
		return;
	case 2:
	case 3:
		mApp->mInchyCells[1]->Draw(0x28, 0x139);
		return;
	case 5:
		mApp->mInchyCells[2]->Draw(0x28, 0x139);
		if ((mTalk.mCurrent != NULL) && (mTalk.mCurrent->mMouth != -1))
			mApp->mInchyMouthCells[mTalk.mCurrent->mMouth]->Draw(0x4a, 0x163);
		if (mBlink == true)
			mApp->mPicInchysmallblink->Draw(0x45, 0x156);
	}
}

// 0x423250: rise, sink, or one of the seven calls (the mouth shapes are fitted to each voice)
void InchySmall::DoAction(int theAction)
{
	mSpeech = -1;
	switch (theAction)
	{
	case 0:
		mState = 1;
		mCounter = 3;
		mOwner->MarkDirty();
		break;
	case 1:
		mState = 3;
		mCounter = 3;
		mOwner->MarkDirty();
		break;
	case 2:
		mTalk.Reset();
		mTalk.AddStep(2, 0xf);
		mTalk.AddStep(0, 0x14);
		mTalk.Play(mApp->mSndXcombo);
		mSpeech = 0;
		break;
	case 3:
		mTalk.Reset();
		mTalk.AddStep(0, 0xf);
		mTalk.AddStep(3, 5);
		mTalk.AddStep(1, 0xf);
		mTalk.AddStep(3, 10);
		mTalk.AddStep(2, 5);
		mTalk.AddStep(1, 0xf);
		mTalk.AddStep(0, 0x23);
		mTalk.Play(mApp->mSndXsupercombo);
		mSpeech = 1;
		break;
	case 4:
		mTalk.Reset();
		mTalk.AddStep(2, 5);
		mTalk.AddStep(1, 0xf);
		mTalk.AddStep(3, 10);
		mTalk.AddStep(0, 5);
		mTalk.AddStep(2, 10);
		mTalk.AddStep(1, 0x19);
		mTalk.AddStep(0, 0x23);
		mTalk.Play(mApp->mSndXcambriancombo);
		mSpeech = 2;
		break;
	case 5:
		mTalk.Reset();
		mTalk.AddStep(0, 10);
		mTalk.AddStep(2, 10);
		mTalk.AddStep(1, 10);
		mTalk.AddStep(2, 10);
		mTalk.AddStep(1, 10);
		mTalk.AddStep(3, 10);
		mTalk.AddStep(1, 0x19);
		mTalk.AddStep(0, 0x23);
		mTalk.Play(mApp->mSndXtriassiccombo);
		mSpeech = 3;
		break;
	case 6:
		mTalk.Reset();
		mTalk.AddStep(0, 0xf);
		mTalk.AddStep(1, 0xf);
		mTalk.AddStep(3, 5);
		mTalk.AddStep(2, 0xf);
		mTalk.AddStep(3, 5);
		mTalk.AddStep(1, 0x19);
		mTalk.AddStep(0, 0x23);
		mTalk.Play(mApp->mSndXjurassiccombo);
		mSpeech = 4;
		break;
	case 7:
		mTalk.Reset();
		mTalk.AddStep(2, 0xf);
		mTalk.AddStep(3, 10);
		mTalk.AddStep(1, 0xf);
		mTalk.AddStep(3, 10);
		mTalk.AddStep(2, 0xf);
		mTalk.AddStep(3, 10);
		mTalk.AddStep(2, 0x14);
		mTalk.AddStep(0, 0x23);
		mTalk.Play(mApp->mSndXcretaceouscombo);
		mSpeech = 5;
		break;
	case 8:
		mTalk.Reset();
		mTalk.AddStep(2, 5);
		mTalk.AddStep(1, 10);
		mTalk.AddStep(2, 10);
		mTalk.AddStep(0, 0xf);
		mTalk.AddStep(3, 10);
		mTalk.AddStep(1, 0xf);
		mTalk.AddStep(3, 10);
		mTalk.AddStep(2, 0x14);
		mTalk.AddStep(0, 0x23);
		mTalk.Play(mApp->mSndXdynamitecombo);
		mSpeech = 6;
	}
	if (-1 < mSpeech)
	{
		HideBubble();
		mBubble->Resize(0x24, 0x10a, 0x78, 0x47);
		mApp->mWidgetManager->BringToFront(mBubble);
		mBubble->MarkDirty();
	}
}

// 0x423650
void InchySmall::Update()
{
	int aState = mState;
	if ((aState != 0) && (aState != 5) && (--mCounter < 1))
	{
		mCounter = 0;
		switch (aState)
		{
		case 1:
			mState = 2;
			mOwner->MarkDirty();
			mCounter = 3;
			break;
		case 2:
			mState = 5;
			mOwner->MarkDirty();
			break;
		case 3:
			mState = 4;
			mOwner->MarkDirty();
			mCounter = 3;
			break;
		case 4:
			mState = 0;
			mOwner->MarkDirty();
			break;
		}
	}
	if ((mState == 5) && (mTalk.mDone == true))
	{
		HideBubble();
		if (mSpeech == 6)
		{
			// the dynamite combo's call has ended: the board gets its stick of dynamite
			Dynomite* aBoard = mApp->mBoard;
			if ((aBoard == NULL) || (aBoard->mLauncher->mLoaded == NULL))
				goto blink;
			aBoard->MakeDynamite();
		}
		mTalk.mDone = false;
	}
blink:
	if (--mBlinkCounter < -0x32)
		mBlinkCounter = mApp->mRand.NextNoAssert() % 100 + 0xfa;
	bool wasBlinking = mBlink;
	int aCount = mBlinkCounter;
	if ((aCount < 0x15) || (0x1d < aCount))
		mBlink = (0 < aCount) && (aCount < 10);
	else
		mBlink = true;
	if (mBlink != wasBlinking)
		mOwner->MarkDirty();

	mEvents.ResetIter();
	InchyEvent* anEvent = mEvents.Next();
	while (anEvent != NULL)
	{
		int aDelay = anEvent->mDelay;
		anEvent->mDelay = aDelay - 1;
		if (aDelay - 1 < 1)
		{
			DoAction(anEvent->mAction);
			mEvents.Remove(anEvent);
			delete anEvent;
		}
		anEvent = mEvents.Next();
	}
	mTalk.Update();
	if (mTalk.mChanged == true)
	{
		mTalk.mChanged = false;
		mOwner->MarkDirty();
	}
}

// 0x423990: the bubble's call after theDelay updates (theLevel 1 combo ... 7 dynomite combo; 0 clears the text)
void InchySmall::Combo(int theDelay, int theLevel)
{
	if ((mSpeech == 6) && (mTalk.mPlaying == true))
		return;
	if (theLevel < 1)
	{
		mText = "";
		return;
	}
	InchyEvent* anEvent = new InchyEvent;
	anEvent->mDelay = theDelay;
	switch (theLevel)
	{
	case 1:
		anEvent->mAction = 2;
		mText = mApp->GetString("COMBO");
		break;
	case 2:
		anEvent->mAction = 3;
		mText = mApp->GetString("SUPER_COMBO");
		break;
	case 3:
		anEvent->mAction = 4;
		mText = mApp->GetString("CAMBRIAN_COMBO");
		break;
	case 4:
		anEvent->mAction = 5;
		mText = mApp->GetString("TRIASSIC_COMBO");
		break;
	case 5:
		anEvent->mAction = 6;
		mText = mApp->GetString("JURASSIC_COMBO");
		break;
	case 6:
		anEvent->mAction = 7;
		mText = mApp->GetString("CRETACIOUS_COMBO");
		break;
	case 7:
		anEvent->mAction = 8;
		mText = mApp->GetString("DYNOMITE_COMBO");
		break;
	default:
		anEvent->mAction = -1;	// port: the original leaves it unset; DoAction ignores what is not 0-8
		break;
	}
	mEvents.Add(anEvent);
}
