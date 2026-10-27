#include "Kroney.h"
#include "BinFile.h"
#include "Dynomite.h"
#include "FossilizedApp.h"
#include "SoundInstance.h"
#include "Sprite.h"

using namespace Sexy;

// 0x42b740: thePic centred on x and on y cut to a whole pixel
static void DrawPicCentered(Pic* thePic, int x, float y)
{
	int aHeight = thePic->mHeight;
	thePic->Draw(x - (thePic->mWidth >> 1), (int) y - (aHeight >> 1));
}

// 0x42b770: the same for a cell
static void DrawCellCentered(Cell* theCell, int x, float y)
{
	int aHeight = theCell->mHeight;
	theCell->Draw(x - (theCell->mWidth >> 1), (int) y - (aHeight >> 1));
}

// 0x42b3a0
Kroney::Kroney()
{
	mFrame = 0.0f;
	mY = -100.0f;
	mSandFraction = 0.0f;
	mPeriod = 10.0f;
	mTimeLeft = 10.0f;
	mState = 0;
	mFirstFlip = false;
	mTalking = false;
	mHurry = false;
	// port: the original leaves these to Start (and the board)
	mBoard = NULL;
	mApp = NULL;
	mCallTime = 0.0f;
	mCallAlt = false;
}

// 0x42b3d0
Kroney::~Kroney()
{
}

// 0x42b3e0
void Kroney::Start(float thePeriod)
{
	mPeriod = thePeriod;
	mTimeLeft = thePeriod;
	mCallTime = (float) ((double) thePeriod - 2.0);
	mState = 1;
	mFrame = 0.0f;
	mFirstFlip = true;
	mCallAlt = false;
	mTalking = false;
}

// 0x42b420
bool Kroney::Update()
{
	if (mState == 0)
	{
		double aFrame = (double) mFrame + (double) 0.1f;
		mFrame = (float) aFrame;
		if (!(aFrame < 8.0))
			mFrame = (float) (aFrame - 8.0);
		Dynomite* aBoard = mBoard;
		if (aBoard->mField130 == true)
		{
			double aY = (double) mY - 20.0;
			mY = (float) aY;
			if (aY < -500.0)
				mY = -500.0f;
		}
		if (mY < 120.0f)
			mY = (float) ((double) mY + 1.0);
		if (aBoard->mFieldB0 != false)
		{
			float aWas = mTimeLeft;
			float aNow = (float) ((double) mTimeLeft - (double) 0.01f);
			mTimeLeft = aNow;
			if ((3.0f < aWas) && (aNow < 3.0f))
				mApp->mSndWhirleysiren->Play();
			if (mTimeLeft <= 0.0f)
			{
				mTalking = false;
				mState = 1;
				mFrame = 0.0f;
			}
			double aCall = (double) mCallTime - (double) 0.01f;
			mCallTime = (float) aCall;
			if (aCall < 0.0)
			{
				GameSound* aSound;
				if (mHurry == false)
					aSound = (mCallAlt == false) ? mApp->mSndXwhirleyscoming1 : mApp->mSndXwhirleyscoming2;
				else
					aSound = mApp->mSndXtimesrunningout;
				aSound->Play();
				mTalking = true;
				mCallAlt = mCallAlt != true;
				mCallTime = 10000.0f;
			}
		}
		if ((mTimeLeft < 5.0f) && (mTimeLeft > 3.0f))
		{
			if (mApp->mTicktockInstance->IsPlaying() == false)
				mApp->mTicktockInstance->Play(true, false);
		}
		else if (mApp->mTicktockInstance->IsPlaying() == true)
			mApp->mTicktockInstance->Stop();
		mSandFraction = (float) (1.0 - (double) mTimeLeft / (double) mPeriod);
	}
	else if (mState == 1)
	{
		double aFrame = (double) mFrame + (double) 0.3f;
		mFrame = (float) aFrame;
		if (!(aFrame < 14.0))
		{
			mTimeLeft = mPeriod;
			mFrame = 0.0f;
			mState = 0;
			mSandFraction = 0.0f;
			mCallTime = (float) ((double) mPeriod - 2.0);
			if (mFirstFlip == false)
				return true;
			mFirstFlip = false;
			return false;
		}
	}
	return false;
}

// 0x42b7c0: Kroney, the hourglass and its two sands (the top cut down by what has fallen, the bottom by what is
// left), his face (calmer past five seconds), the siren's light between 2.25 and 3 seconds, and his balloon
void Kroney::Draw()
{
	Dynomite* aBoard = mBoard;
	if (mState == 0)
	{
		Cell* aCell = mApp->mKroneyCells[(int) mFrame];
		aCell->Draw((aBoard->mField70 - (aCell->mWidth >> 1)) + 0xb + aBoard->mField6c,
			(int) ((double) mY - 4.0) - (aCell->mHeight >> 1));
		aCell = mApp->mKroneyHourglassCells[(int) mFrame];
		aCell->Draw((aBoard->mField70 - (aCell->mWidth >> 1)) + 0xb + aBoard->mField6c,
			(int) ((double) mY + 14.0) - (aCell->mHeight >> 1));

		int aFrame = (int) mFrame;
		Cell* aTop = mApp->mKroneySandTopCells[aFrame];
		int aTopHeight = aTop->mHeight;
		int aTopCut = (int) ((double) aTopHeight * (double) mSandFraction);
		int aBottomCut = (int) ((double) mApp->mKroneySandBottomCells[aFrame]->mHeight * (1.0 - (double) mSandFraction));
		int aTopWidth = aTop->mWidth;
		mApp->mKroneySandTopCells[(int) mFrame]->DrawPart(aBoard->mField70 - 10 + aBoard->mField6c,
			(int) (((double) mY - 19.0) + (double) aTopCut), 0, aTopCut, aTopWidth, aTopHeight - aTopCut);
		Cell* aBottom = mApp->mKroneySandBottomCells[(int) mFrame];
		int aBottomHeight = aBottom->mHeight;
		int aBottomWidth = aBottom->mWidth;
		mApp->mKroneySandBottomCells[(int) mFrame]->DrawPart(aBoard->mField70 - 10 + aBoard->mField6c,
			(int) (((double) mY + 11.0) + (double) aBottomCut), 0, aBottomCut, aBottomWidth, aBottomHeight - aBottomCut);

		if (mTimeLeft <= 5.0f)
			aCell = mApp->mKroneyFaceCells[(int) mFrame];
		else
			aCell = mApp->mKroneyFaceCells[(int) mFrame + 8];
		aCell->Draw((aBoard->mField70 - (aCell->mWidth >> 1)) - 0x1c + aBoard->mField6c,
			(int) ((double) mY - 15.0) - (aCell->mHeight >> 1));

		if ((2.25f < mTimeLeft) && (mTimeLeft < 3.0f))
			DrawPicCentered(mApp->mPicWhirleyalarm, mBoard->mField70 - 0x3b + mBoard->mField6c,
				(float) (((double) mY - 4.0) - 50.0));
		if (mTalking == true)
		{
			aCell = mApp->mPicTextballoons_c0;
			aBoard = mBoard;
			aCell->Draw((aBoard->mField70 - (aCell->mWidth >> 1)) - 0x3b + aBoard->mField6c,
				(int) (((double) mY - 4.0) - 50.0) - (aCell->mHeight >> 1));
			float aWordY = (float) ((((double) mY - 4.0) - 50.0) - 6.0);
			aBoard = mBoard;
			if (mHurry == false)
				DrawCellCentered(mApp->mPicTextwords_c0, aBoard->mField70 - 0x3c + aBoard->mField6c, aWordY);
			else
				DrawCellCentered(mApp->mPicTextwords_c5, aBoard->mField70 - 0x3c + aBoard->mField6c, aWordY);
		}
	}
	else if (mState == 1)
	{
		Cell* aCell = mApp->mKroneyFlipCells[(int) mFrame];
		aCell->Draw((aBoard->mField70 - (aCell->mWidth >> 1)) + 2 + aBoard->mField6c,
			(int) ((double) mY + 12.0) - (aCell->mHeight >> 1));
	}
}

// 0x42b630
void Kroney::SaveState(BinFile* theFile)
{
	theFile->WriteFloat(mFrame);
	theFile->WriteFloat(mY);
	theFile->WriteFloat(mSandFraction);
	theFile->WriteFloat(mTimeLeft);
	theFile->WriteFloat(mCallTime);
	theFile->WriteFloat(mPeriod);
	theFile->WriteInt(mState);
	theFile->WriteBool(mTalking);
	theFile->WriteBool(mFirstFlip);
	theFile->WriteBool(mCallAlt);
	theFile->WriteBool(mHurry);
}

// 0x42b6c0
void Kroney::LoadState(BinFile* theFile)
{
	mFrame = theFile->ReadFloat();
	mY = theFile->ReadFloat();
	mSandFraction = theFile->ReadFloat();
	mTimeLeft = theFile->ReadFloat();
	mCallTime = theFile->ReadFloat();
	mPeriod = theFile->ReadFloat();
	mState = theFile->ReadInt();
	mTalking = theFile->ReadBool();
	mFirstFlip = theFile->ReadBool();
	mCallAlt = theFile->ReadBool();
	mHurry = theFile->ReadBool();
}
