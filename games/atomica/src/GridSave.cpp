// The field of Atomica Deluxe, part 4: the bonus levels' set-up, the saved game and the hints (see Grid.h).

#include "Grid.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "BonusMolecules.h"
#include "Effects.h"

#include <cstdio>

using namespace Sexy;

// 0x40dd10: the set-up of a bonus level (every fifth): the field holds groups of four atoms of each colour (placed so that
// no molecule is made), some groups of two, and four more atoms, the last two chromium; the level's goal is the number
// of atoms on the field less one.
void Grid::SetupBonusLevel(int theLevel)
{
	mNumColors = theLevel / 5 + 3;
	if (0xc < mNumColors)
		mNumColors = 0xc;
	mApp->mBoard->mBonus->mRound = -1000;
	bool aOver29 = 0x1d < theLevel;
	bool aOver24 = 0x18 < theLevel;
	mChromiumChance = 0;
	mBiohazardChance = 0;
	mAtomsPerRound = 0;
	int aType = 0;
	int aMaxType = 0;
	int aGroups = (int) aOver29 + (int) aOver24 + (int) (0x13 < theLevel) + 8 + (int) (0xe < theLevel);
	Atom* anAtom;
	int aCol;
	int aRow;
	if (aGroups != 0)
	{
		do
		{
			int aCount = 4;
			do
			{
				anAtom = new Atom(this, aType, false);
				anAtom->mHidden = true;
				anAtom->mFlag_ac = true;
				do
				{
					do
					{
						aCol = mApp->mRand.Get(9, false);
						aRow = mApp->mRand.Get(8, false);
					} while (mCells[aCol][aRow] != NULL);
					anAtom->PlaceAt(aCol, aRow, true);
					if ((TypesInRect(aCol, aRow, aCol + 1, aRow + 1) != 1) && (TypesInRect(aCol - 1, aRow - 1, aCol, aRow) != 1) &&
						(TypesInRect(aCol, aRow, aCol - 1, aRow + 1) != 1) && (TypesInRect(aCol, aRow - 1, aCol + 1, aRow) != 1))
						break;
					anAtom->RemoveFromCell();
				} while (true);
				aCount--;
			} while (aCount != 0);
			aType++;
			if (mNumColors <= aType)
				aType = 0;
			if (aMaxType <= aType)
				aMaxType = aType;
			aGroups--;
		} while (aGroups != 0);
	}
	int aPairs = mApp->mRand.Get(5, false) + 1;
	if (0 < aPairs)
	{
		do
		{
			int aPairType = mApp->mRand.Get(aMaxType, false);
			int aCopies = 2;
			do
			{
				anAtom = new Atom(this, aPairType, false);
				anAtom->mHidden = true;
				anAtom->mFlag_ac = true;
				do
				{
					do
					{
						aCol = mApp->mRand.Get(9, false);
						aRow = mApp->mRand.Get(8, false);
					} while (mCells[aCol][aRow] != NULL);
					anAtom->PlaceAt(aCol, aRow, true);
					if ((TypesInRect(aCol, aRow, aCol + 1, aRow + 1) != 1) && (TypesInRect(aCol - 1, aRow - 1, aCol, aRow) != 1) &&
						(TypesInRect(aCol, aRow, aCol - 1, aRow + 1) != 1) && (TypesInRect(aCol, aRow - 1, aCol + 1, aRow) != 1))
						break;
					anAtom->RemoveFromCell();
				} while (true);
				aCopies--;
			} while (aCopies != 0);
			aPairs--;
		} while (aPairs != 0);
	}
	int aLast = 0;
	do
	{
		int aLastType = mApp->mRand.Get(aMaxType, false);
		if (1 < aLast)
			mChromiumChance = 100.0f;
		anAtom = new Atom(this, aLastType, false);
		anAtom->mHidden = true;
		anAtom->mFlag_ac = true;
		do
		{
			do
			{
				aCol = mApp->mRand.Get(9, false);
				aRow = mApp->mRand.Get(8, false);
			} while (mCells[aCol][aRow] != NULL);
			anAtom->PlaceAt(aCol, aRow, true);
			if ((TypesInRect(aCol, aRow, aCol + 1, aRow + 1) != 1) && (TypesInRect(aCol - 1, aRow - 1, aCol, aRow) != 1) &&
				(TypesInRect(aCol, aRow, aCol - 1, aRow + 1) != 1) && (TypesInRect(aCol, aRow - 1, aCol + 1, aRow) != 1))
				break;
			anAtom->RemoveFromCell();
		} while (true);
		aLast++;
	} while (aLast <= 3);
	mChromiumChance = 0;
	mApp->mBoard->mBonus->mTarget = 3000;
	mAtoms.Reset();
	Atom* anItem = mAtoms.Next();
	while (anItem != NULL)
	{
		anItem->mDead = false;
		anItem = mAtoms.Next();
	}
	int aNumAtoms = 0;
	for (aCol = 0; aCol < COLS; aCol++)
		for (aRow = 0; aRow < ROWS; aRow++)
			if (mCells[aCol][aRow] != NULL)
				aNumAtoms++;
	mApp->mBoard->mMeter.mMax = (float) (aNumAtoms - 1);
	mApp->mBoard->mMeter.mValue = 0;
	mB4c0 = true;
}

// 0x40d450: the hint boxes of the first levels and of each new kind of atom
// port: the hint boxes are not ported yet
void Grid::CheckHints()
{
}

// 0x40c9a0
void Grid::Save(FILE* f)
{
	fwrite(&mPulse, 4, 1, f);
	fwrite(&mPulseStep, 4, 1, f);
	fwrite(&mSelectGlow, 4, 1, f);
	int aCount = mAtoms.Size();
	fwrite(&aCount, 4, 1, f);
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		unsigned char anElement = anAtom->IsElement() ? 1 : 0;
		fwrite(&anElement, 1, 1, f);
		anAtom->Save(f);
		anAtom = mAtoms.Next();
	}
	fwrite(mCellState, 0x120, 1, f);
	fwrite(&mCursorX, 4, 1, f);
	fwrite(&mCursorY, 4, 1, f);
	fwrite(&mCursorColor, 0x10, 1, f);
	fwrite(&mNumColors, 4, 1, f);
	fputc(mB32c ? 1 : 0, f);
	fwrite(&mCombo, 4, 1, f);
	fputc(mNoMoveLeft ? 1 : 0, f);
	fputc(mLevelOver ? 1 : 0, f);
	fwrite(&mOverTick, 4, 1, f);
	fputc(mB33c ? 1 : 0, f);
	fputc(mB33d ? 1 : 0, f);
	fputc(mB358 ? 1 : 0, f);
	fwrite(&mGeiger1, 4, 1, f);
	fwrite(&mGeiger2, 4, 1, f);
	fwrite(&mI364, 4, 1, f);
	fwrite(&mStat374, 4, 1, f);
	fwrite(&mStat378, 4, 1, f);
	fwrite(&mStat37c, 4, 1, f);
	fwrite(&mStat380, 4, 1, f);
	fwrite(&mStat384, 4, 1, f);
	fwrite(&mStat388, 4, 1, f);
	fwrite(&mStat38c, 4, 1, f);
	fwrite(&mF390, 4, 1, f);
	fwrite(&mF394, 4, 1, f);
	fwrite(&mI398, 4, 1, f);
	fwrite(&mChromiumChance, 4, 1, f);
	fwrite(&mBiohazardChance, 4, 1, f);
	fwrite(&mAtomsPerRound, 4, 1, f);
	fputc(mB3a8 ? 1 : 0, f);
	fwrite(&mF3ac, 4, 1, f);
	fwrite(&mRounds, 4, 1, f);
	fwrite(mHintShown, 0xff, 1, f);
	fputc(mB4b3 ? 1 : 0, f);
	fputc(mB4b4, f);
	fputc(mB4c0 ? 1 : 0, f);
	fputc(mUnsolvable ? 1 : 0, f);
	fwrite(&mBonusTick, 4, 1, f);
	fwrite(&mArrowX, 4, 1, f);
	fwrite(&mArrowY, 4, 1, f);
	fwrite(&mI4d0, 4, 1, f);
	fwrite(mRadiation, 0x48, 1, f);
	fputc(mRadiationRound ? 1 : 0, f);
	fwrite(&mI520, 4, 1, f);
	fputc(mB524 ? 1 : 0, f);
}

// 0x40cd10
void Grid::Load(FILE* f)
{
	fread(&mPulse, 4, 1, f);
	fread(&mPulseStep, 4, 1, f);
	fread(&mSelectGlow, 4, 1, f);
	int aCount = 0;
	fread(&aCount, 4, 1, f);
	for (int i = 0; i < aCount; i++)
	{
		unsigned char anElement = 0;
		fread(&anElement, 1, 1, f);
		Atom* anAtom;
		if (anElement == 0)
			anAtom = new Atom(this, 0, false);
		else
			anAtom = new ElementAtom(this, 0);
		anAtom->Load(f);
	}
	fread(mCellState, 0x120, 1, f);
	for (int aCol = 0; aCol < COLS; aCol++)
		for (int aRow = 0; aRow < ROWS; aRow++)
			if (mCellState[aCol][aRow] < 0)
				mCellState[aCol][aRow] = 0;
	fread(&mCursorX, 4, 1, f);
	fread(&mCursorY, 4, 1, f);
	fread(&mCursorColor, 0x10, 1, f);
	fread(&mNumColors, 4, 1, f);
	mB32c = fgetc(f) != 0;
	fread(&mCombo, 4, 1, f);
	mNoMoveLeft = fgetc(f) != 0;
	mLevelOver = fgetc(f) != 0;
	fread(&mOverTick, 4, 1, f);
	mB33c = fgetc(f) != 0;
	mB33d = fgetc(f) != 0;
	mB358 = fgetc(f) != 0;
	fread(&mGeiger1, 4, 1, f);
	fread(&mGeiger2, 4, 1, f);
	fread(&mI364, 4, 1, f);
	fread(&mStat374, 4, 1, f);
	fread(&mStat378, 4, 1, f);
	fread(&mStat37c, 4, 1, f);
	fread(&mStat380, 4, 1, f);
	fread(&mStat384, 4, 1, f);
	fread(&mStat388, 4, 1, f);
	fread(&mStat38c, 4, 1, f);
	fread(&mF390, 4, 1, f);
	fread(&mF394, 4, 1, f);
	fread(&mI398, 4, 1, f);
	fread(&mChromiumChance, 4, 1, f);
	fread(&mBiohazardChance, 4, 1, f);
	fread(&mAtomsPerRound, 4, 1, f);
	mB3a8 = fgetc(f) != 0;
	fread(&mF3ac, 4, 1, f);
	fread(&mRounds, 4, 1, f);
	fread(mHintShown, 0xff, 1, f);
	mB4b3 = fgetc(f) != 0;
	mB4b4 = (unsigned char) fgetc(f);
	mB4c0 = fgetc(f) != 0;
	mUnsolvable = fgetc(f) != 0;
	fread(&mBonusTick, 4, 1, f);
	fread(&mArrowX, 4, 1, f);
	fread(&mArrowY, 4, 1, f);
	fread(&mI4d0, 4, 1, f);
	fread(mRadiation, 0x48, 1, f);
	mRadiationRound = fgetc(f) != 0;
	fread(&mI520, 4, 1, f);
	mB524 = fgetc(f) != 0;
	mApp->mBoard->mF0ac = (float) (mCombo * 0xc);
	mSelected = NULL;
}
