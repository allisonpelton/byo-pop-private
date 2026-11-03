// The adviser on the machine's monitor (see Atomicus.h).

#include "Atomicus.h"
#include "AtomicaApp.h"
#include "Graphics.h"

using namespace Sexy;

// 0x40b200
Atomicus::Atomicus(AtomicaApp* theApp) :
	mApp(theApp),
	mMuted(false),
	mOffset(600),
	mBrightness(0xff),
	mArrive(0xdc),
	mLookRight(0),
	mLookTick(0),
	mGaze(0),
	mNoise(100),
	mNoiseStep(-1),
	mBlinking(false),
	mBlinkFrame(0),
	mBlinkCount(0),
	mOnlyLevelUp(false),
	mTalking(false),
	mTalkTick(0)
{
}

// 0x40b2a0
Atomicus::~Atomicus()
{
}

// 0x40b330
void Atomicus::Update()
{
	mNoise += mNoiseStep;
	if (mNoise < 1)
	{
		mNoiseStep = mApp->mRand.Get(3, false) + 1;
		mNoise = 1;
	}
	if (500 < mNoise)
		mNoiseStep = -1 - mApp->mRand.Get(3, false);
	if (mTalking)
		mBlinking = false;
	mLookTick++;
	if ((100 < mLookTick) && !mBlinking && !mTalking && (mApp->mRand.Get(800, false) == 3))
	{
		mLookTick = 0;
		mLookRight = 1 - mLookRight;
	}
	if (0 < mArrive)
	{
		if ((0x53 < mArrive) && (0 < mOffset))
			mOffset -= 4;
		if (mArrive == 0x29)
			mOffset += 0x14;
		if ((mArrive < 0x29) && (0 < mOffset))
			mOffset -= 3;
		if (mOffset < 0)
			mOffset = 0;
		mArrive--;
	}
	if (mApp->mRand.Get(0x32, false) == 5)
		mBrightness = (mBrightness != 0xff) ? 0xff : 0xfb;
	if (mLookRight == 1)
	{
		if (mGaze < 10.0f)
			mGaze += 0.5f;
	}
	else if (0.0f < mGaze)
		mGaze -= 0.5f;
	if (mBlinking)
	{
		mBlinkFrame += 0.2f;
		if (7.99f < mBlinkFrame)
		{
			mBlinkFrame -= 8.0f;
			if (--mBlinkCount < 1)
				mBlinking = false;
		}
	}
	if (mTalking)
	{
		if (--mTalkTick < 1)
			mTalking = false;
	}
	for (size_t i = 0; i < mMessages.size(); )
	{
		if (--mMessages[i].mDelay == 0)
		{
			if (!mTalking)
			{
				if (!mOnlyLevelUp || (mMessages[i].mSound == mApp->mSndLevelUp))
				{
					mMessages[i].mSound->Play();
					mTalking = true;
					mTalkTick = mMessages[i].mTalk;
				}
				mMessages.erase(mMessages.begin() + i);
			}
			else
			{
				mMessages[i].mDelay = 1;
				i++;
			}
		}
		else
			i++;
	}
}

// 0x40b550
void Atomicus::Blink()
{
	if (!mBlinking && ((mGaze == 10.0f) || (mGaze == 0.0f)))
	{
		mBlinking = true;
		mBlinkFrame = 0;
		mBlinkCount = mApp->mRand.Get(2, false) + 2;
	}
}

// 0x40b5a0
void Atomicus::Say(int theMessage)
{
	if (mMuted)
		return;
	Sound* aSound = NULL;
	int aDelay = 0x19;
	int aTalk = 120;
	switch (theMessage)
	{
	case 0: aDelay = 0x32; aSound = mApp->mSndBombReady; break;
	case 1:
	case 2: aDelay = 10; aSound = mApp->mSndBiohazard; break;
	case 3: aDelay = 5; aSound = mApp->mSndBonusMolecule; break;
	case 4: aDelay = 0x32; aSound = mApp->mSndBonusReady; break;
	case 5: aDelay = 5; aSound = mApp->mSndBonusUpgraded; break;
	case 6: aDelay = 0x32; aSound = mApp->mSndGetReady; break;
	case 8: aDelay = 1; aSound = mApp->mSndGo; aTalk = 30; mOnlyLevelUp = false; break;
	case 9: aDelay = 1; aSound = mApp->mSndAtomicBomb; break;
	case 0xa: aSound = mApp->mSndFire; break;
	case 0xb: aSound = mApp->mSndIce; break;
	case 0xc: aSound = mApp->mSndLightning; break;
	case 0xd: aSound = mApp->mSndWind; break;
	case 0xe: aSound = mApp->mSndEther; break;
	case 0xf: aSound = mApp->mSndGameOver; break;
	case 0x10:
		if (mApp->mMode == 0)
		{
			mMessages.clear();
			mOnlyLevelUp = true;
		}
		aSound = mApp->mSndLevelUp;
		break;
	default:
		return;
	}
	Message aMessage;
	aMessage.mDelay = aDelay;
	aMessage.mSound = aSound;
	aMessage.mTalk = aTalk;
	mMessages.push_back(aMessage);
}

// 0x40c000: the voice for a combo
void Atomicus::Comment(int theCombo)
{
	if (theCombo == 1)
	{
		Blink();
		return;
	}
	if (theCombo < 2)
		return;
	if (!mTalking && !mBlinking && (mApp->mRand.Get(3, false) == 0))
	{
		mLookTick = 0;
		mLookRight = 1 - mLookRight;
	}
	if (theCombo <= 8)
	{
		Message aMessage;
		aMessage.mDelay = 0x1e;
		aMessage.mSound = mApp->mSndCombo[theCombo - 2];
		aMessage.mTalk = 90;
		mMessages.push_back(aMessage);
	}
}

// 0x40b9f0: the monitor shows the head (the mouth is centred on the head's width, not its own: 0x40bdfd to 0x40be09); it is drawn into a window of the monitor's size
void Atomicus::Draw(Graphics* g, int theX, int theY)
{
	Sprite* aMask = mApp->mMonitorMask;
	int aWidth = aMask->mWidth;
	int aHeight = aMask->mHeight;
	Graphics aWindow(*g);
	aWindow.Translate(theX, theY + 0xd);
	aWindow.ClipRect(0, 0, aWidth, aHeight);
	aWindow.SetColorizeImages(false);
	mApp->mMonitorBlank->Draw(&aWindow, aWidth / 2 - mApp->mMonitorBlank->mWidth / 2, aHeight / 2 - mApp->mMonitorBlank->mHeight / 2);
	aWindow.SetColorizeImages(true);
	aWindow.SetColor(Color(0xff, 0xff, 0xff, mBrightness));
	int aGaze = (int) mGaze;
	if (aGaze > 10)
		aGaze = 10;
	Sprite* aCoat = mApp->mCoat;
	aCoat->Draw(&aWindow, aWidth / 2 + 10 - aCoat->mWidth / 2, aHeight / 2 - aCoat->mHeight / 2 + 0x5a + mOffset);
	Sprite* aHead = mApp->mSmallHead[aGaze];
	int aHeadX = ((((mOffset == 0) ? 5 : 0) - aGaze) * 2) - aHead->mWidth / 2 + aWidth / 2;
	int aHeadY = aHeight / 2 - aHead->mHeight / 2 + mOffset;
	aHead->Draw(&aWindow, aHeadX, aHeadY);
	if (mTalking)
	{
		int aFrame = 10 + mApp->mRand.Get(20, false);
		Sprite* aMouth = (aGaze == 10) ? mApp->mMouth[aFrame] : mApp->mMouthLeft[aFrame];
		aMouth->Draw(&aWindow, aWidth / 2 + ((aGaze == 10) ? 0x2e : 0xe) + ((((mOffset == 0) ? 5 : 0) - aGaze) * 2) - aHead->mWidth / 2,
			aHeight / 2 - aHead->mHeight / 2 + 0x54 + mOffset);
	}
	aWindow.SetColorizeImages(false);
}
