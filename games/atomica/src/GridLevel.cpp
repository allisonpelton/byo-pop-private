// The field of Atomica Deluxe, part 3: the levels' set-ups and the end of a round (see Grid.h).

#include "Grid.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Atomicus.h"
#include "BonusMolecules.h"
#include "Effects.h"
#include "GameText.h"
#include "MusicInterface.h"

using namespace Sexy;

// 0x411de0: a new game starts on the field
void Grid::Reset(int theNumColors)
{
	mArrowX = 0;
	mArrowY = 0;
	mI4d0 = 0;
	mRadiationRound = false;
	mB4b4 = 0;
	mUnsolvable = false;
	mBonusTick = 0;
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		delete anAtom;
		anAtom = mAtoms.Next();
	}
	mAtoms.Clear();
	mNumColors = theNumColors;
	mB33d = false;
	mI364 = 0;
	mF394 = 300.0f;
	mF390 = 300.0f;
	mI398 = 0;
	mB335 = true;
	mSparkles.Clear();
	int aCol = 0;
	do
	{
		int aRow = 0;
		do
		{
			mCells[aCol][aRow] = NULL;
			if ((aRow == 0) || (aRow == 7))
			{
				// the sparks that light the first and last rows while the field is revealed
				for (int i = 0; i < 12; i++)
					mSparkles.Add(new Sparkle(mApp, aCol, aRow, i % 4));
			}
			mCellState[aCol][aRow] = -100000;
			aRow++;
		} while (aRow < 8);
		aCol++;
	} while (aCol < 9);
	mB33c = false;
	mChromiumChance = 0;
	mBiohazardChance = 0;
	mAtoms.Clear();
	mSelected = NULL;
	if (mApp->mMode == 0)
		StartLevel(mApp->mBoard->mMeter.mLevel);
	else
		SetupTimedLevel();
	mCombo = 0;
	FlushStats();
}

// 0x40d140: a round of the strategy mode is over (the atoms of the next one are due)
void Grid::EndRound()
{
	mRounds++;
	// (here the original counts the game as played, once its fourth round is over with one molecule made: the trial's count,
	// the base app's +0x440, which only the title screen's free plays read)
	if (mApp->mMode == 1)
		return;
	int aLevel = mApp->mBoard->mMeter.mLevel;
	if (aLevel == 1)
	{
		if (mRounds == 1)
		{
			int i = 0;
			do
			{
				Atom* anAtom = new Atom(this, i % mNumColors, false);
				if (i < 3)
					anAtom->mFlag_ac = true;
				anAtom->mHidden = true;
				anAtom->PlaceRandom();
				i++;
			} while (i < 6);
		}
	}
	else if ((aLevel % 5 == 0) && (0 < aLevel))
	{
		bool aNone = true;
		bool aSkip = false;
		if (0 < mNumColors)
		{
			int aType = 0;
			do
			{
				int aCount = 0;
				for (int aCol = 0; aCol < COLS; aCol++)
					for (int aRow = 0; aRow < ROWS; aRow++)
					{
						Atom* anAtom = mCells[aCol][aRow];
						if ((anAtom != NULL) && ((anAtom->mType == aType) || (anAtom->mKind == KIND_CHROMIUM)))
							aCount++;
					}
				if (3 < aCount)
					aNone = false;
				aType++;
			} while (aType < mNumColors);
			if (!aNone)
				aSkip = true;
		}
		if (!aSkip && !mUnsolvable)
		{
			mUnsolvable = true;
			mBonusTick = 0;
		}
	}
	if (mAtoms.Size() == 0)
	{
		mAtoms.Reset();
		Atom* anAtom = mAtoms.Next();
		while (anAtom != NULL)
		{
			if (anAtom->mPre)
			{
				anAtom->PlaceAt(anAtom->mCol, anAtom->mRow, false);
				anAtom->mPreStarted = true;
			}
			anAtom = mAtoms.Next();
		}
		if (mAtoms.Size() == 0)
		{
			int i = 0;
			do
			{
				Atom* aNewAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
				if (i < 3)
					aNewAtom->mFlag_ac = true;
				aNewAtom->mHidden = true;
				aNewAtom->PlaceRandom();
				i++;
			} while (i < 6);
		}
	}
	mB3a8 = false;
}

// 0x40f860: the set-up of a level of the strategy mode: the number of colours, the chances of the special atoms, the
// bomb's size (the board's mFD8), the atoms on the field at the start and the arrows of the magnet levels.
// Every fifth level (and the levels right after) are made by SetupBonusLevel and the music changes with them.
void Grid::StartLevel(int theLevel)
{
	Atom* anAtom;
	int i;
	int aArrowY = 0;
	mApp->mBoard->mMeter.mFrozen = false;
	mApp->mBoard->mBonus->mTarget = 3;
	mApp->mBoard->mMeter.mValue = 0;
	mB4c0 = false;
	if (theLevel % 5 == 0)
		mApp->mMusicInterface->PlayMusic(0, 0x18, false);
	else if ((theLevel % 5 == 1) && (theLevel != 1))
		mApp->mMusicInterface->PlayMusic(0, 0xc, false);
	mB3a8 = true;
	mAtomsPerRound = 3;
	mRounds = 0;
	mRadiationRound = false;
	switch (theLevel) {
	case 1:
		mNumColors = 3;
		mAtomsPerRound = 3;
		anAtom = new Atom(this, 0, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(4, 3, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(5, 3, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(5, 4, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(1, 5, true);
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mApp->mBoard->mMeter.mMax = 8.0f;
		mApp->mBoard->mBonus->mTarget = 1000;
		return;
	case 2:
		mNumColors = 4;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 12.0f;
		mChromiumChance = 0;
		mBiohazardChance = 0;
		anAtom = new Atom(this, 0, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(6, 2, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(7, 2, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(7, 3, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(5, 3, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(1, 1, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(2, 1, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(1, 2, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(3, 2, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(2, 5, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(2, 6, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(3, 6, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(3, 4, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		anAtom->PlaceAt(7, 6, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(0, 5, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(5, 6, true);
		anAtom = new Atom(this, 3, false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(5, 0, true);
		mApp->mBoard->mBonus->mTarget = 3;
		mApp->mBoard->mBonus->mRound = -3;
		return;
	case 3:
		mNumColors = 4;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 16.0f;
		mChromiumChance = 0;
		mBiohazardChance = 100.0f;
		anAtom = new Atom(this, 0, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(4, 4, true);
		mChromiumChance = 0;
		mBiohazardChance = 0;
		anAtom = new Atom(this, 0, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(4, 6, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(4, 2, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(6, 4, true);
		anAtom = new Atom(this, 0, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(2, 4, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(2, 2, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(2, 6, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(6, 2, true);
		anAtom = new Atom(this, 1, false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(6, 6, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(1, 1, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(1, 7, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(7, 1, true);
		anAtom = new Atom(this, 2, false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(7, 7, true);
		mChromiumChance = 0;
		mBiohazardChance = 3.0f;
		return;
	case 4:
		mNumColors = 5;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 20.0f;
		mChromiumChance = 100.0f;
		mBiohazardChance = 0;
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(3, 3, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(4, 3, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(5, 3, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(3, 4, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(4, 4, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(5, 4, true);
		mChromiumChance = 0;
		mBiohazardChance = 0;
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(1, 1, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(7, 1, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(1, 6, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceAt(7, 6, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(4, 1, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(4, 6, true);
		anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		anAtom->mHidden = true;
		anAtom->PlaceAt(4, 0, true);
		mChromiumChance = 5.0f;
		mBiohazardChance = 0.1f;
		return;
	case 5:
	case 10:
	case 15:
	case 20:
	case 25:
	case 30:
	case 35:
	case 40:
		SetupBonusLevel(theLevel);
		return;
	case 6:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 5;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 25.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
		mChromiumChance = 3.0f;
		mBiohazardChance = 1.0f;
		return;
	case 7:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 6;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 30.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
		mChromiumChance = 3.0f;
		mBiohazardChance = 2.0f;
		return;
	case 8:
		mAtomsPerRound = 3;
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 5;
		mApp->mBoard->mMeter.mMax = 12.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
		mChromiumChance = 5.0f;
		mBiohazardChance = 0;
		mArrowX = 0;
		goto LAB_00411b93;
	case 9:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 7;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 30.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
		goto LAB_00410bd9;
	case 11:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 7;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 35.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
LAB_00410bd9:
		mChromiumChance = 3.0f;
		mBiohazardChance = 3.0f;
		return;
	case 12:
		mAtomsPerRound = 3;
		mChromiumChance = 8.0f;
		mBiohazardChance = 90.0f;
		mNumColors = 4;
		mApp->mBoard->mMeter.mMax = 25.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
		mChromiumChance = 8.0f;
		mBiohazardChance = 25.0f;
		return;
	case 13:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 8;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 40.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
		mChromiumChance = 3.0f;
		goto LAB_00411d10;
	case 14:
		mRadiationRound = true;
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 7;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 16.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
		goto LAB_00411599;
	case 16:
		mAtomsPerRound = 3;
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 8;
		mApp->mBoard->mMeter.mMax = 45.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		mChromiumChance = 6.0f;
		goto LAB_00411d10;
	case 17:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 6;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 16.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
		mBiohazardChance = 0;
		mArrowX = 1;
		goto LAB_00411376;
	case 18:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 9;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 45.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		mChromiumChance = 7.0f;
		goto LAB_00411d10;
	case 19:
		mAtomsPerRound = 3;
		mChromiumChance = 0;
		mBiohazardChance = 100.0f;
		mNumColors = 6;
		mApp->mBoard->mMeter.mMax = 25.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			anAtom->mFlag_ac = true;
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 3);
		i = 0;
		mChromiumChance = 100.0f;
		mBiohazardChance = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 3) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 6);
		return;
	case 21:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 10;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 45.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		mChromiumChance = 2.0f;
		mBiohazardChance = 2.0f;
		return;
	case 22:
		mChromiumChance = 8.0f;
		mBiohazardChance = 90.0f;
		mNumColors = 6;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 25.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
		mChromiumChance = 8.0f;
		mBiohazardChance = 25.0f;
		return;
	case 23:
		mAtomsPerRound = 3;
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 11;
		mApp->mBoard->mMeter.mMax = 45.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		break;
	case 24:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 8;
		mAtomsPerRound = 3;
		i = 0;
		mApp->mBoard->mMeter.mMax = 16.0f;
		aArrowY = -1;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
		mBiohazardChance = 0;
		mArrowX = 0;
LAB_00411376:
		mChromiumChance = 6.0f;
		mArrowY = aArrowY;
		mApp->mBoard->mBonus->mTarget = 3000;
		return;
	case 26:
		mChromiumChance = 8.0f;
		mBiohazardChance = 90.0f;
		mNumColors = 7;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 25.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
		mChromiumChance = 8.0f;
		mBiohazardChance = 35.0f;
		return;
	case 27:
		mAtomsPerRound = 3;
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 11;
		mApp->mBoard->mMeter.mMax = 50.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		break;
	case 28:
		mRadiationRound = true;
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 8;
		mAtomsPerRound = 3;
		mApp->mBoard->mMeter.mMax = 25.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
LAB_00411599:
		for (int aCol = 0; aCol < COLS; aCol++)
			for (int aRow = 0; aRow < ROWS; aRow++)
				mRadiation[aCol][aRow] = false;
		mRadiation[4][1] = true;
		mRadiation[5][1] = true;
		mRadiation[4][2] = true;
		mRadiation[5][2] = true;
		mRadiation[1][2] = true;
		mRadiation[2][2] = true;
		mRadiation[1][3] = true;
		mRadiation[2][3] = true;
		mRadiation[1][4] = true;
		mRadiation[2][4] = true;
		mRadiation[5][5] = true;
		mRadiation[6][5] = true;
		mRadiation[7][5] = true;
		mRadiation[5][6] = true;
		mRadiation[6][6] = true;
		mRadiation[7][6] = true;
		mRadiation[2][6] = true;
		mRadiation[3][6] = true;
		mRadiation[2][7] = true;
		mRadiation[3][7] = true;
		return;
	case 29:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 9;
		mAtomsPerRound = 3;
		i = 0;
		mApp->mBoard->mMeter.mMax = 16.0f;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
		goto LAB_00411896;
	case 31:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 6;
		mAtomsPerRound = 4;
		mApp->mBoard->mMeter.mMax = 25.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		break;
	case 32:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 7;
		mAtomsPerRound = 4;
		mApp->mBoard->mMeter.mMax = 30.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		break;
	case 33:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 9;
		mAtomsPerRound = 4;
		i = 0;
		mApp->mBoard->mMeter.mMax = 20.0f;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
LAB_00411896:
		mChromiumChance = 6.0f;
		mBiohazardChance = 0;
		mArrowX = -1;
		mArrowY = 0;
		mApp->mBoard->mBonus->mTarget = 3000;
		return;
	case 34:
		mAtomsPerRound = 3;
		mChromiumChance = 8.0f;
		mBiohazardChance = 90.0f;
		mNumColors = 9;
		mApp->mBoard->mMeter.mMax = 35.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
		mChromiumChance = 8.0f;
		mBiohazardChance = 35.0f;
		return;
	case 36:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 8;
		mAtomsPerRound = 4;
		mApp->mBoard->mMeter.mMax = 40.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		break;
	case 37:
		mChromiumChance = 8.0f;
		mBiohazardChance = 90.0f;
		mNumColors = 11;
		mAtomsPerRound = 3;
		i = 0;
		mApp->mBoard->mMeter.mMax = 35.0f;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 9) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xc);
		mChromiumChance = 8.0f;
		mBiohazardChance = 45.0f;
		return;
	case 38:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 11;
		mAtomsPerRound = 4;
		i = 0;
		mApp->mBoard->mMeter.mMax = 20.0f;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 5) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 8);
		mChromiumChance = 6.0f;
		mBiohazardChance = 0;
		mArrowX = 0;
LAB_00411b93:
		mArrowY = 1;
		mApp->mBoard->mBonus->mTarget = 3000;
		return;
	case 39:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 9;
		mAtomsPerRound = 5;
		mApp->mBoard->mMeter.mMax = 40.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		break;
	case 41:
		mChromiumChance = 0;
		mBiohazardChance = 0;
		mNumColors = 11;
		mAtomsPerRound = 6;
		mApp->mBoard->mMeter.mMax = 40.0f;
		i = 0;
		do {
		anAtom = new Atom(this, i % mNumColors, false);
			if (i < 0xc) {
				anAtom->mFlag_ac = true;
			}
			anAtom->mHidden = true;
			anAtom->PlaceRandom();
			i = i + 1;
		} while (i < 0xf);
		break;
	default:
		goto switchD_default;
	}
	mChromiumChance = 2.0f;
LAB_00411d10:
	mBiohazardChance = 4.0f;
switchD_default:
	return;
}


// 0x40f730: the set-up of the action mode
void Grid::SetupTimedLevel()
{
	mApp->mBoard->mMeter.mFrozen = true;
	mApp->mBoard->mMeter.mMax = 150.0f;
	mApp->mBoard->mMeter.mValue = 0;
	if (mApp->mExpertMode == 0)
		mNumColors = 4;
	else
		mNumColors = 6;
	mAtomsPerRound = 3;
	mChromiumChance = 0;
	mBiohazardChance = 0;
	int i = 0;
	do
	{
		Atom* anAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
		if (i < 0xc)
			anAtom->mFlag_ac = true;
		anAtom->mHidden = true;
		anAtom->PlaceRandom();
		i++;
	} while (i < 0xf);
	mChromiumChance = 0;
	mBiohazardChance = 0;
	mF3ac = 0.04f;
	mRounds = 0;
}
