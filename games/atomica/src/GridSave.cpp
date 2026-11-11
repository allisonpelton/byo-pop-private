// The field of Atomica Deluxe, part 4: the bonus levels' set-up, the saved game and the hints (see Grid.h).

#include "Grid.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "BonusMolecules.h"
#include "Effects.h"
#include "HintBox.h"

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
				anAtom->mFormsByItself = true;
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
				anAtom->mFormsByItself = true;
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
		anAtom->mFormsByItself = true;
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
	mBonusLevel = true;
}

namespace
{
	// how many atoms of the list are still pre-atoms (thePre) or already formed
	int CountPre(PtrList<Atom>& theAtoms, bool thePre)
	{
		int aCount = 0;
		theAtoms.Reset();
		Atom* anAtom = theAtoms.Next();
		while (anAtom != NULL)
		{
			if (anAtom->mPre == thePre)
				aCount++;
			anAtom = theAtoms.Next();
		}
		return aCount;
	}
}

// 0x40d450: the hint boxes of the first levels and of each new kind of atom, each shown once in a game (the action mode has
// none). A box is not shown if its hint was shown, or ticked away, in an earlier game.
void Grid::CheckHints()
{
	if (mApp->mMode == 1)
		return;
	Board* aBoard = mApp->mBoard;
	switch (aBoard->mMeter.mLevel)
	{
	case 1:
		if (!mHintShown[0] && (mAtoms.Size() != 0) && (CountPre(mAtoms, true) == 0))
			new HintBox(mApp, this, 0);
		if (!mHintShown[1] && (aBoard->mMeter.mValue == 1.0f) && (mGroups.Size() == 0))
			new HintBox(mApp, this, 1);
		break;
	case 2:
		if (!mHintShown[2] && (CountPre(mAtoms, false) == 0xd))
			new HintBox(mApp, this, 2);
		if (!mHintShown[3] && (mCombo == 2) && (mGroups.Size() == 0))
			new HintBox(mApp, this, 3);
		if (!mHintShown[4] && (mCombo == 3) && (mGroups.Size() == 0))
			new HintBox(mApp, this, 4);
		break;
	case 3:
		if (!mHintShown[0xd])
		{
			int aCount = 0;
			mAtoms.Reset();
			Atom* anAtom = mAtoms.Next();
			while (anAtom != NULL)
			{
				if ((anAtom->mKind == KIND_BIOHAZARD) && (anAtom->mFade == 0xff))
					aCount++;
				anAtom = mAtoms.Next();
			}
			if (aCount != 0)
				new HintBox(mApp, this, 0xd);
		}
		break;
	case 4:
		if (!mHintShown[0xe] && (CountPre(mAtoms, false) == 10))
			new HintBox(mApp, this, 0xe);
		break;
	case 5:
		if (!mHintShown[0x17] && (CountPre(mAtoms, true) == 0) && (0 < mAtoms.Size()))
			new HintBox(mApp, this, 0x17);
		break;
	case 6:
		if (!mHintShown[0x10] && (CountPre(mAtoms, false) == 5))
			new HintBox(mApp, this, 0x10);
		break;
	case 8:
		if (!mHintShown[0x18] && (CountPre(mAtoms, true) == 3))
			new HintBox(mApp, this, 0x18);
		break;
	case 0xc:
		if (!mHintShown[0x19] && (CountPre(mAtoms, true) == 3))
			new HintBox(mApp, this, 0x19);
		break;
	case 0xe:
		if (!mHintShown[0x1a] && (CountPre(mAtoms, true) == 3))
			new HintBox(mApp, this, 0x1a);
		break;
	case 0x13:
		if (!mHintShown[0x1b] && (CountPre(mAtoms, true) == 3))
			new HintBox(mApp, this, 0x1b);
		break;
	}
	// the bomb is ready
	if (!mHintShown[5] && !aBoard->mButtonBomb->mDisabled && (aBoard->mBombFrame >= 24.0f))
		new HintBox(mApp, this, 5);
	// the bonus molecule's sign is up
	if (!mHintShown[6] && (aBoard->mBonus->mX >= 0))
		new HintBox(mApp, this, 6);
	bool aShownElementHint = false;
	if (!mHintShown[7] && mElementReady)
	{
		new HintBox(mApp, this, 7);
		aShownElementHint = true;
	}
	// an element atom wants to act: the box for its element
	if ((mReadyElement != 0) && !aShownElementHint)
	{
		int anIndex = mReadyElement - 1;
		if ((anIndex >= 0) && (anIndex < 5) && !mHintShown[8 + anIndex])
			new HintBox(mApp, this, 8 + anIndex);
		mReadyElement = 0;
	}
	// the field is filling up (not on a bonus level)
	if (!mHintShown[0xf] && (0x24 < CountAtoms(false)) && ((aBoard->mMeter.mLevel == 0) || (aBoard->mMeter.mLevel % 5 != 0)))
		new HintBox(mApp, this, 0xf);
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
	fputc(mCheckMolecules ? 1 : 0, f);
	fwrite(&mCombo, 4, 1, f);
	fputc(mNoMoveLeft ? 1 : 0, f);
	fputc(mGameOver ? 1 : 0, f);
	fwrite(&mGameOverTick, 4, 1, f);
	fputc(mFieldRevealed ? 1 : 0, f);
	fputc(mRevealSaid ? 1 : 0, f);
	fputc(mComboEnded ? 1 : 0, f);
	fwrite(&mGeiger1, 4, 1, f);
	fwrite(&mGeiger2, 4, 1, f);
	fwrite(&mBiohazards, 4, 1, f);
	fwrite(&mLevelUpdates, 4, 1, f);
	fwrite(&mLevelSeconds, 4, 1, f);
	fwrite(&mLevelAtoms, 4, 1, f);
	fwrite(&mLevelMolecules, 4, 1, f);
	fwrite(&mLevelBestCombo, 4, 1, f);
	fwrite(&mLevelBonusMolecules, 4, 1, f);
	fwrite(&mLevelBonusUpgrades, 4, 1, f);
	fwrite(&mRoundTimer, 4, 1, f);
	fwrite(&mRoundTime, 4, 1, f);
	fwrite(&mCountDownTick, 4, 1, f);
	fwrite(&mChromiumChance, 4, 1, f);
	fwrite(&mBiohazardChance, 4, 1, f);
	fwrite(&mAtomsPerRound, 4, 1, f);
	fputc(mLevelSetUp ? 1 : 0, f);
	fwrite(&mMeterFillStep, 4, 1, f);
	fwrite(&mRounds, 4, 1, f);
	fwrite(mHintShown, 0xff, 1, f);
	fputc(mElementReady ? 1 : 0, f);
	fputc(mReadyElement, f);
	fputc(mBonusLevel ? 1 : 0, f);
	fputc(mUnsolvable ? 1 : 0, f);
	fwrite(&mBonusTick, 4, 1, f);
	fwrite(&mArrowX, 4, 1, f);
	fwrite(&mArrowY, 4, 1, f);
	fwrite(&mMagnetTick, 4, 1, f);
	fwrite(&mRadiation[0][0], 4, 1, f);	// the original writes 8 bytes from +0x4d0, so these four go twice
	fwrite(mRadiation, 0x48, 1, f);
	fputc(mRadiationRound ? 1 : 0, f);
	fwrite(&mRadiationBlinkTimer, 4, 1, f);
	fputc(mRadiationBlink ? 1 : 0, f);
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
			anAtom = new ElementAtom(this, 0, true);
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
	mCheckMolecules = fgetc(f) != 0;
	fread(&mCombo, 4, 1, f);
	mNoMoveLeft = fgetc(f) != 0;
	mGameOver = fgetc(f) != 0;
	fread(&mGameOverTick, 4, 1, f);
	mFieldRevealed = fgetc(f) != 0;
	mRevealSaid = fgetc(f) != 0;
	mComboEnded = fgetc(f) != 0;
	fread(&mGeiger1, 4, 1, f);
	fread(&mGeiger2, 4, 1, f);
	fread(&mBiohazards, 4, 1, f);
	fread(&mLevelUpdates, 4, 1, f);
	fread(&mLevelSeconds, 4, 1, f);
	fread(&mLevelAtoms, 4, 1, f);
	fread(&mLevelMolecules, 4, 1, f);
	fread(&mLevelBestCombo, 4, 1, f);
	fread(&mLevelBonusMolecules, 4, 1, f);
	fread(&mLevelBonusUpgrades, 4, 1, f);
	fread(&mRoundTimer, 4, 1, f);
	fread(&mRoundTime, 4, 1, f);
	fread(&mCountDownTick, 4, 1, f);
	fread(&mChromiumChance, 4, 1, f);
	fread(&mBiohazardChance, 4, 1, f);
	fread(&mAtomsPerRound, 4, 1, f);
	mLevelSetUp = fgetc(f) != 0;
	fread(&mMeterFillStep, 4, 1, f);
	fread(&mRounds, 4, 1, f);
	fread(mHintShown, 0xff, 1, f);
	mElementReady = fgetc(f) != 0;
	mReadyElement = (unsigned char) fgetc(f);
	mBonusLevel = fgetc(f) != 0;
	mUnsolvable = fgetc(f) != 0;
	fread(&mBonusTick, 4, 1, f);
	fread(&mArrowX, 4, 1, f);
	fread(&mArrowY, 4, 1, f);
	fread(&mMagnetTick, 4, 1, f);
	fread(&mRadiation[0][0], 4, 1, f);	// (overwritten by the next read)
	fread(mRadiation, 0x48, 1, f);
	mRadiationRound = fgetc(f) != 0;
	fread(&mRadiationBlinkTimer, 4, 1, f);
	mRadiationBlink = fgetc(f) != 0;
	mApp->mBoard->mNeedleTarget = (float) (mCombo * 0xc);
	mSelected = NULL;
}
