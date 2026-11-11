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
void Grid::Save(FILE* theFile)
{
	fwrite(&mPulse, 4, 1, theFile);
	fwrite(&mPulseStep, 4, 1, theFile);
	fwrite(&mSelectGlow, 4, 1, theFile);
	int aCount = mAtoms.Size();
	fwrite(&aCount, 4, 1, theFile);
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		unsigned char anElement = anAtom->IsElement() ? 1 : 0;
		fwrite(&anElement, 1, 1, theFile);
		anAtom->Save(theFile);
		anAtom = mAtoms.Next();
	}
	fwrite(mCellState, sizeof(mCellState), 1, theFile);
	fwrite(&mCursorX, 4, 1, theFile);
	fwrite(&mCursorY, 4, 1, theFile);
	fwrite(&mCursorColor, 0x10, 1, theFile);
	fwrite(&mNumColors, 4, 1, theFile);
	fputc(mCheckMolecules ? 1 : 0, theFile);
	fwrite(&mCombo, 4, 1, theFile);
	fputc(mNoMoveLeft ? 1 : 0, theFile);
	fputc(mGameOver ? 1 : 0, theFile);
	fwrite(&mGameOverTick, 4, 1, theFile);
	fputc(mFieldRevealed ? 1 : 0, theFile);
	fputc(mRevealSaid ? 1 : 0, theFile);
	fputc(mComboEnded ? 1 : 0, theFile);
	fwrite(&mGeiger1, 4, 1, theFile);
	fwrite(&mGeiger2, 4, 1, theFile);
	fwrite(&mBiohazards, 4, 1, theFile);
	fwrite(&mLevelUpdates, 4, 1, theFile);
	fwrite(&mLevelSeconds, 4, 1, theFile);
	fwrite(&mLevelAtoms, 4, 1, theFile);
	fwrite(&mLevelMolecules, 4, 1, theFile);
	fwrite(&mLevelBestCombo, 4, 1, theFile);
	fwrite(&mLevelBonusMolecules, 4, 1, theFile);
	fwrite(&mLevelBonusUpgrades, 4, 1, theFile);
	fwrite(&mRoundTimer, 4, 1, theFile);
	fwrite(&mRoundTime, 4, 1, theFile);
	fwrite(&mCountDownTick, 4, 1, theFile);
	fwrite(&mChromiumChance, 4, 1, theFile);
	fwrite(&mBiohazardChance, 4, 1, theFile);
	fwrite(&mAtomsPerRound, 4, 1, theFile);
	fputc(mLevelSetUp ? 1 : 0, theFile);
	fwrite(&mMeterFillStep, 4, 1, theFile);
	fwrite(&mRounds, 4, 1, theFile);
	fwrite(mHintShown, 0xff, 1, theFile);
	fputc(mElementReady ? 1 : 0, theFile);
	fputc(mReadyElement, theFile);
	fputc(mBonusLevel ? 1 : 0, theFile);
	fputc(mUnsolvable ? 1 : 0, theFile);
	fwrite(&mBonusTick, 4, 1, theFile);
	fwrite(&mArrowX, 4, 1, theFile);
	fwrite(&mArrowY, 4, 1, theFile);
	fwrite(&mMagnetTick, 4, 1, theFile);
	fwrite(&mRadiation[0][0], 4, 1, theFile);	// the original writes 8 bytes from +0x4d0, so these four go twice
	fwrite(mRadiation, 0x48, 1, theFile);
	fputc(mRadiationRound ? 1 : 0, theFile);
	fwrite(&mRadiationBlinkTimer, 4, 1, theFile);
	fputc(mRadiationBlink ? 1 : 0, theFile);
}

// 0x40cd10
void Grid::Load(FILE* theFile)
{
	fread(&mPulse, 4, 1, theFile);
	fread(&mPulseStep, 4, 1, theFile);
	fread(&mSelectGlow, 4, 1, theFile);
	int aCount = 0;
	fread(&aCount, 4, 1, theFile);
	for (int i = 0; i < aCount; i++)
	{
		unsigned char anElement = 0;
		fread(&anElement, 1, 1, theFile);
		Atom* anAtom;
		if (anElement == 0)
			anAtom = new Atom(this, 0, false);
		else
			anAtom = new ElementAtom(this, 0, true);
		anAtom->Load(theFile);
	}
	fread(mCellState, sizeof(mCellState), 1, theFile);
	for (int aCol = 0; aCol < COLS; aCol++)
		for (int aRow = 0; aRow < ROWS; aRow++)
			if (mCellState[aCol][aRow] < 0)
				mCellState[aCol][aRow] = 0;
	fread(&mCursorX, 4, 1, theFile);
	fread(&mCursorY, 4, 1, theFile);
	fread(&mCursorColor, 0x10, 1, theFile);
	fread(&mNumColors, 4, 1, theFile);
	mCheckMolecules = fgetc(theFile) != 0;
	fread(&mCombo, 4, 1, theFile);
	mNoMoveLeft = fgetc(theFile) != 0;
	mGameOver = fgetc(theFile) != 0;
	fread(&mGameOverTick, 4, 1, theFile);
	mFieldRevealed = fgetc(theFile) != 0;
	mRevealSaid = fgetc(theFile) != 0;
	mComboEnded = fgetc(theFile) != 0;
	fread(&mGeiger1, 4, 1, theFile);
	fread(&mGeiger2, 4, 1, theFile);
	fread(&mBiohazards, 4, 1, theFile);
	fread(&mLevelUpdates, 4, 1, theFile);
	fread(&mLevelSeconds, 4, 1, theFile);
	fread(&mLevelAtoms, 4, 1, theFile);
	fread(&mLevelMolecules, 4, 1, theFile);
	fread(&mLevelBestCombo, 4, 1, theFile);
	fread(&mLevelBonusMolecules, 4, 1, theFile);
	fread(&mLevelBonusUpgrades, 4, 1, theFile);
	fread(&mRoundTimer, 4, 1, theFile);
	fread(&mRoundTime, 4, 1, theFile);
	fread(&mCountDownTick, 4, 1, theFile);
	fread(&mChromiumChance, 4, 1, theFile);
	fread(&mBiohazardChance, 4, 1, theFile);
	fread(&mAtomsPerRound, 4, 1, theFile);
	mLevelSetUp = fgetc(theFile) != 0;
	fread(&mMeterFillStep, 4, 1, theFile);
	fread(&mRounds, 4, 1, theFile);
	fread(mHintShown, 0xff, 1, theFile);
	mElementReady = fgetc(theFile) != 0;
	mReadyElement = (unsigned char) fgetc(theFile);
	mBonusLevel = fgetc(theFile) != 0;
	mUnsolvable = fgetc(theFile) != 0;
	fread(&mBonusTick, 4, 1, theFile);
	fread(&mArrowX, 4, 1, theFile);
	fread(&mArrowY, 4, 1, theFile);
	fread(&mMagnetTick, 4, 1, theFile);
	fread(&mRadiation[0][0], 4, 1, theFile);	// (overwritten by the next read)
	fread(mRadiation, 0x48, 1, theFile);
	mRadiationRound = fgetc(theFile) != 0;
	fread(&mRadiationBlinkTimer, 4, 1, theFile);
	mRadiationBlink = fgetc(theFile) != 0;
	mApp->mBoard->mNeedleTarget = (float) (mCombo * 0xc);
	mSelected = NULL;
}
