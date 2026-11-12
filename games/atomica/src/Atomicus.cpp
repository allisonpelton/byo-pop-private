// The adviser on the machine's monitor (see Atomicus.h).

#include "Atomicus.h"
#include "AtomicaApp.h"
#include "platform/GameOptions.h"
#include "Graphics.h"
#include "GameText.h"

using namespace Sexy;

// 0x40b200 / 0x421c40
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
	mPending(true),
	mLipCursor(NULL),
	mLipFrame(0),
	mLipPos(0),
	mLipStep(0),
	mLipTarget(0),
	mLipTick(0),
	mLipCountdown(0),
	mLipSound(NULL)
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
	if ((100 < mLookTick) && !mBlinking && !mTalking && !mPending && (mApp->mRand.Get(800, false) == 3))
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
		mBrightness = (mBrightness != 0xff) ? 0xff : 0xfa;
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
	LipUpdate();
	for (size_t i = 0; i < mMessages.size(); )
	{
		if (--mMessages[i].mCount == 0)
		{
			if (!mTalking)
			{
				if (!mOnlyLevelUp || (mMessages[i].mSound == mApp->mSndLevelUp))
					LipStart(mMessages[i].mSound, mMessages[i].mLipData);
				mMessages.erase(mMessages.begin() + i);
			}
			else
			{
				mMessages[i].mCount = mMessages[i].mDelay;
				i++;
			}
		}
		else
			i++;
	}
}

// 0x421e10: the clip is due; its sound starts at the next update
void Atomicus::LipStart(Sound* theSound, unsigned int theLipData)
{
	static const int sNone[1] = {-1};
	mLipCursor = LipSyncData(theLipData);
	if (mLipCursor == NULL)
		mLipCursor = sNone;
	LipNext();
	mLipCountdown = 1;
	mLipSound = theSound;
	mPending = true;
}

// 0x421d00: the sound starts when its count runs out, and the mouth then moves a step an update (more when the clock is
// ahead by whole tenths of a second) until the keyframe's target is reached
void Atomicus::LipUpdate()
{
	unsigned int aPrevious = mLipTick;
	mLipTick = mApp->GetClockTime();
	if ((0 < mLipCountdown) && (--mLipCountdown == 0))
	{
		mLipSound->Play();
		mTalking = true;
		mPending = false;
	}
	if (mTalking)
	{
		unsigned int aNow = mApp->GetClockTime();
		float aPos = mLipPos;
		float aFrame = mLipFrame;
		// fixes mod, "lipSync": one step an update, whatever the clock says
		bool aOneStep = Platform::FixOn("lipSync");
		do
		{
			aPrevious += 100;
			aPos += mLipStep;
			aFrame += mLipStep;
		} while (!aOneStep && ((int) aPrevious < (int) aNow));
		mLipPos = aPos;
		mLipFrame = aFrame;
		if (mLipStep <= 0.0f)
		{
			if (aFrame < mLipTarget)
				mLipFrame = mLipTarget;
			if ((int) aPos < (int) mLipTarget)
				LipNext();
		}
		else
		{
			if (mLipTarget < aFrame)
				mLipFrame = mLipTarget;
			if ((int) aPos > (int) mLipTarget)
				LipNext();
		}
	}
}

// 0x421c70: the next keyframe: a mouth picture (in fives), the direction the mouth moves and for how many updates
void Atomicus::LipNext()
{
	int aPicture = *mLipCursor++;
	mLipPos = (float) aPicture;
	if (aPicture == -1)
	{
		mTalking = false;
		mPending = false;
		return;
	}
	float aDirection = (float) mLipCursor[0];
	int aCount = mLipCursor[1];
	mLipCursor += 2;
	if (0.0f <= aDirection)
		mLipPos = mLipPos * 5.0f;
	else
		mLipPos = (mLipPos + 1.0f) * 5.0f - 1.0f;
	mLipFrame = mLipPos;
	mLipTarget = aDirection * 4.0f + mLipPos;
	float aStep = (1.0f / (float) aCount) * aDirection;
	mLipStep = aStep + aStep;
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

// 0x40b5a0 / 0x40b1e0
void Atomicus::Say(int theMessage)
{
	if (mMuted)
		return;
	Sound* aSound = NULL;
	int aDelay = 0x19;
	unsigned int aLipData = 0;
	switch (theMessage)
	{
	case 0: aDelay = 0x32; aSound = mApp->mSndBombReady; aLipData = 0x4cb560; break;
	case 1:
	case 2: aDelay = 10; aSound = mApp->mSndBiohazard; aLipData = 0x4cb618; break;
	case 3: aDelay = 5; aSound = mApp->mSndBonusMolecule; aLipData = 0x4cbbf8; break;
	case 4: aDelay = 0x32; aSound = mApp->mSndBonusReady; aLipData = 0x4cbbf8; break;
	case 5: aDelay = 5; aSound = mApp->mSndBonusUpgraded; aLipData = 0x4cbbf8; break;
	case 6: aDelay = 0x32; aSound = mApp->mSndGetReady; aLipData = 0x4cbbf8; break;
	case 8: aDelay = 1; aSound = mApp->mSndGo; aLipData = 0x4cb478; mOnlyLevelUp = false; break;
	case 9: aDelay = 1; aSound = mApp->mSndAtomicBomb; aLipData = 0x4cb4c0; break;
	case 0xa: aSound = mApp->mSndFire; aLipData = 0x4cba28; break;
	case 0xb: aSound = mApp->mSndIce; aLipData = 0x4cba70; break;
	case 0xc: aSound = mApp->mSndLightning; aLipData = 0x4cbae8; break;
	case 0xd: aSound = mApp->mSndWind; aLipData = 0x4cbaac; break;
	case 0xe: aSound = mApp->mSndEther; aLipData = 0x4cb9e8; break;
	case 0xf: aSound = mApp->mSndGameOver; aLipData = 0x4cbb28; break;
	case 0x10:
		if (mApp->mMode == 0)
		{
			mMessages.clear();
			mOnlyLevelUp = true;
		}
		aSound = mApp->mSndLevelUp;
		aLipData = 0x4cbb98;
		break;
	default:
		return;
	}
	Message aMessage;
	aMessage.mDelay = aDelay;
	aMessage.mCount = aDelay;
	aMessage.mSound = aSound;
	aMessage.mLipData = aLipData;
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
	if (!mTalking && !mPending && (mApp->mRand.Get(3, false) == 0))
	{
		mLookTick = 0;
		mLookRight = 1 - mLookRight;
	}
	if (theCombo <= 8)
	{
		Message aMessage;
		static const unsigned int sLipData[7] = {0x4cb180, 0x4cb1c8, 0x4cb238, 0x4cb2a8, 0x4cb320, 0x4cb398, 0x4cb408};
		aMessage.mDelay = 0x1e;
		aMessage.mCount = 0x1e;
		aMessage.mSound = mApp->mSndCombo[theCombo - 2];
		aMessage.mLipData = sLipData[theCombo - 2];
		mMessages.push_back(aMessage);
	}
}

// 0x40b9f0: the monitor shows the head (the mouth is centred on the head's width, not its own: 0x40bdfd to 0x40be09); it is
// drawn into the monitor buffer, whose alpha then comes from the mask, every other row dimmed at random (the static), and the
// buffer is drawn at the monitor
void Atomicus::Draw(Graphics* g, int theX, int theY)
{
	Sprite* aMask = mApp->mMonitorMask;
	Sprite* aBuffer = mApp->mMonitorBuffer;
	int aWidth = aBuffer->mWidth;
	int aHeight = aBuffer->mHeight;
	Graphics aWindow(aBuffer->mImage);
	aWindow.SetColor(Color(0xff, 0xff, 0xff, 0xff));
	aWindow.ClipRect(0, 0, 1000, 1000);
	aWindow.SetColorizeImages(false);
	mApp->mMonitorBlank->Draw(&aWindow, aWidth / 2 - mApp->mMonitorBlank->mWidth / 2, aHeight / 2 - mApp->mMonitorBlank->mHeight / 2);
	aWindow.SetColorizeImages(true);
	aWindow.SetColor(Color(0xff, 0xff, 0xff, mBrightness));
	int aGaze = (int) mGaze;
	// the coat follows the head's gaze, and while the head nods (mBlinking, set by Blink) the nod's frame lifts it; the head is
	// the picture of the gaze, or of the nod's frame (turned to the right or to the left)
	static const int sNodY[8] = {-1, 3, 5, 7, 12, 6, 4, -1};
	Sprite* aCoat = mApp->mCoat;
	int aNod = (int) mBlinkFrame;
	int aCoatY = aHeight / 2 - aCoat->mHeight / 2 + 0x5a + mOffset;
	if (mBlinking)
		aCoatY = aHeight / 2 - ((sNodY[aNod] + (((sNodY[aNod] >> 31) & 3))) >> 2) - (aCoat->mHeight >> 1) + 0x5a + mOffset;
	aCoat->Draw(&aWindow, (int) ((double) (aWidth / 2 + 10) - (double) mGaze * 2.0) - (aCoat->mWidth >> 1), aCoatY);
	int aStep = ((mOffset == 0) ? 5 : 0) - aGaze;
	Sprite* aHead = mApp->mSmallHead[aGaze];
	if (!mBlinking)
		aHead->Draw(&aWindow, aStep * 2 - (aHead->mWidth >> 1) + aWidth / 2, aHeight / 2 - (aHead->mHeight >> 1) + mOffset);
	else if (mLookRight == 1)
	{
		Sprite* aNodHead = mApp->mRightNod[aNod];
		aNodHead->Draw(&aWindow, aStep * 2 - (aNodHead->mWidth >> 1) + aWidth / 2,
			aHeight / 2 + mOffset + (sNodY[aNod] - (aNodHead->mHeight >> 1)));
	}
	else
	{
		Sprite* aNodHead = mApp->mLeftNod[aNod];
		aNodHead->Draw(&aWindow, aStep * 2 - 4 - (aNodHead->mWidth >> 1) + aWidth / 2,
			aHeight / 2 + mOffset + (sNodY[aNod] - (aNodHead->mHeight >> 1)));
	}
	if (mTalking)
	{
		int aFrame = (int) mLipFrame;
		Sprite* aMouth = (aGaze == 10) ? mApp->mMouth[aFrame] : mApp->mMouthLeft[aFrame];
		aMouth->Draw(&aWindow, aWidth / 2 + ((aGaze == 10) ? 0x2e : 0xe) + ((((mOffset == 0) ? 5 : 0) - aGaze) * 2) - aHead->mWidth / 2,
			aHeight / 2 - aHead->mHeight / 2 + 0x54 + mOffset);
	}
	aWindow.SetColorizeImages(false);

	const char* aMaskBits = (const char*) aMask->GetBits();
	char* aBufferBits = (char*) aBuffer->GetBits();
	for (int y = 0; y < aHeight; y++)
	{
		bool aDim = mApp->mRand.Get(mNoise, false) == 0;
		for (int x = 0; x < aWidth; x++)
		{
			aBufferBits[3] = aDim ? (char) (*aMaskBits >> 1) : *aMaskBits;
			aMaskBits += 4;
			aBufferBits += 4;
		}
	}
	aBuffer->BitsChanged();
	aBuffer->Draw(g, theX, theY + 0xd);
}
