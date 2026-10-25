// The field of Atomica Deluxe, part 1: construction, input, update and drawing (see Grid.h).

#include "Grid.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Atomicus.h"
#include "BonusMolecules.h"
#include "Effects.h"
#include "PathFinder.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "SoundInstance.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

// DAT_004d40d0 and DAT_004d40d1 (set by the hint boxes)
bool gGridKeepTooltip = false;
bool gGridBoardClicksOff = false;

// ---------------------------------------------------------------------------------------------------------
// Construction

// 0x40e3e0
Grid::Grid(AtomicaApp* theApp) :
	mApp(theApp),
	mGroupColor(0, 0, 0, 255),
	mCursorColor(0, 0, 0, 255)
{
	mCursorX = 0;
	mCursorY = 0;
	mM30c = 0;
	mM310 = 0;
	mHoverCol = mHoverRow = 0;
	mM31c = mM320 = 0;
	mGroupX = mGroupY = 0;
	mGeiger1 = mGeiger2 = 0;
	mI364 = 0;
	mI4b8 = mI4bc = 0;
	mArrowX = mArrowY = 0;
	mB33d = false;
	mPausedByKey = false;
	mB4b3 = false;
	mB32c = false;
	mB334 = false;
	mB335 = false;
	mM30c = -1000;			// 0xfffffc18
	mSelected = NULL;
	mPathFinder = new PathFinder(this);
	for (int c = 0; c < COLS; c++)
		for (int r = 0; r < ROWS; r++)
		{
			mCells[c][r] = NULL;
			mCellState[c][r] = -1;
			mRadiation[c][r] = false;
		}
	mCombo = 0;
	mNoMoveLeft = false;
	mLevelOver = false;
	mOverTick = 0;
	mSelectGlow = 0;
	mB358 = true;
	mStat374 = mStat37c = mStat380 = mStat384 = mStat388 = mStat38c = 0;
	mB4b4 = 0;
	mI368 = 0;
	mTooltipAtom = NULL;
	mB4c0 = false;
	mUnsolvable = false;
	for (int i = 0; i < 0xff; i++)
		mHintShown[i] = false;
	mRadiationRound = false;
	mI520 = 0;
	mB524 = false;
	mB525 = false;
	mPulse = 0;
	mPulseStep = 0;
	mF390 = mF394 = 0;
	mI398 = 0;
	mNumColors = 0;
	mChromiumChance = mBiohazardChance = 0;
	mAtomsPerRound = 0;
	mB3a8 = false;
	mF3ac = 0;
	mRounds = 0;
	mI4d0 = 0;
	mBonusTick = 0;
	mB33c = false;
	mTooltipAlpha = 0;
	mApp->mMusicInterface->PlayMusic(0, mApp->mMode == 0 ? 0xc : 0x2c, false);
}

// 0x40c340
Grid::~Grid()
{
	if (mPausedByKey)
	{
		mApp->mPauseCount--;
		mPausedByKey = false;
	}
	if (mApp->mGeiger1Instance != NULL && mApp->mGeiger1Instance->IsPlaying())
	{
		mApp->mGeiger1Instance->Stop();
		mApp->mGeiger2Instance->Stop();
	}
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		delete anAtom;
		anAtom = mAtoms.Next();
	}
	mAtoms.Clear();
	mGroups.Reset();
	PtrList<Atom>* aGroup = mGroups.Next();
	while (aGroup != NULL)
	{
		aGroup->Reset();
		Atom* aMember = aGroup->Next();
		while (aMember != NULL)
		{
			delete aMember;
			aMember = aGroup->Next();
		}
		delete aGroup;
		aGroup = mGroups.Next();
	}
	mGroups.Clear();
	delete mPathFinder;
	mSparkles.Reset();
	Sparkle* aSparkle = mSparkles.Next();
	while (aSparkle != NULL)
	{
		delete aSparkle;
		aSparkle = mSparkles.Next();
	}
	mSparkles.Clear();
	mApp->mMusicInterface->StopMusic(0);
}

// 0x40c4c0: the middle of a cell
void Grid::CellToPixel(int theCol, int theRow, int* thePixel)
{
	thePixel[0] = theCol * 0x32 + 0x19;
	thePixel[1] = theRow * 0x32 + 0x19;
}

// 0x40c4f0
void Grid::PixelToCell(int theX, int theY, int* theCell)
{
	theCell[0] = theX / 0x32;
	theCell[1] = theY / 0x32;
}

// 0x40e670
void Grid::Start()
{
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		delete anAtom;
		anAtom = mAtoms.Next();
	}
	mAtoms.Clear();
	mB33d = false;
	mI364 = 0;
	Resize(0xa5, 0x28, 0x1c2, 400);
	mF390 = mF394;
	mPulse = 10.0f;
	mPulseStep = 0.15f;
	mF394 = 300.0f;
	mI398 = 0;
	FlushStats();
}

// 0x40c540: every atom flies away (the level is over, or the bomb went off)
void Grid::FlyAwayAll()
{
	mUnsolvable = false;
	mCombo = 0;
	mApp->mBoard->mF0ac = 0;
	mApp->mSndWhoosh->Play();
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		if (!mLevelOver)
		{
			for (int i = 0; i < 4; i++)
			{
				Sparkle* aSparkle = new Sparkle(mApp, anAtom->mCol, anAtom->mRow, i % 4);
				mSparkles.Add(aSparkle);
				aSparkle->mAlpha = 0xff;
				aSparkle->mRevealing = false;
			}
		}
		anAtom->FlyAway(mApp->mBoard->LastTarget());
		anAtom = mAtoms.Next();
	}
	for (int aCol = 0; aCol < COLS; aCol++)
		for (int aRow = 0; aRow < ROWS; aRow++)
			mCellState[aCol][aRow] = -1000000;
	mApp->mBoard->mBonus->mAlpha = 0;
}

// 0x40c8c0: the totals of the field go to the board's
void Grid::FlushStats()
{
	Board* aBoard = mApp->mBoard;
	aBoard->mStat1c4 += mStat374;
	aBoard->mStat1cc += mStat37c;
	aBoard->mStat1d0 += mStat380;
	aBoard->mStat1d4 = aBoard->mStat1d4 > mStat384 ? aBoard->mStat1d4 : mStat384;
	aBoard->mStat1d8 += mStat388;
	aBoard->mStat1dc += mStat38c;
	mStat374 = 0;
	mStat37c = 0;
	mStat380 = 0;
	mStat384 = 0;
	mStat388 = 0;
	mStat38c = 0;
}

// 0x40c880
void Grid::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_SPACE)
	{
		if (mPausedByKey)
		{
			mPausedByKey = false;
			mApp->mPauseCount--;
			return;
		}
		mPausedByKey = true;
		mApp->mPauseCount++;
	}
}

// 0x40c6a0: how many atoms are on the field
int Grid::CountAtoms(bool theOnlyFlying)
{
	int aCount = 0;
	for (int aCol = 0; aCol < COLS; aCol++)
	{
		for (int aRow = 0; aRow < ROWS; aRow++)
		{
			Atom* anAtom = mCells[aCol][aRow];
			if ((anAtom != NULL) && !anAtom->mPre)
			{
				if (!theOnlyFlying)
					aCount++;
				else if (!anAtom->IsElement())
					aCount++;
			}
		}
	}
	return aCount;
}

// 0x40c700: the number of different types in a rectangle of cells (an empty cell or a pre-atom counts as the type -1,
// chromium atoms count for none); 1 means the block is made of one type (the original keeps them in a list)
int Grid::TypesInRect(int theCol1, int theRow1, int theCol2, int theRow2)
{
	int aTypeStore[16];
	int aNumTypes = 0;
	auto aHas = [&](int theType) { for (int i = 0; i < aNumTypes; i++) if (aTypeStore[i] == theType) return true; return false; };
	for (int aRow = theRow1; aRow <= theRow2; aRow++)
	{
		if (aRow < 0)
			continue;
		if (aRow > 7)
			break;
		for (int aCol = theCol1; aCol <= theCol2; aCol++)
		{
			if (aCol < 0)
				continue;
			if (aCol > 8)
				break;
			Atom* anAtom = mCells[aCol][aRow];
			if ((anAtom == NULL) || anAtom->mPre)
			{
				if (!aHas(-1))
					aTypeStore[aNumTypes++] = -1;
			}
			else if ((anAtom->mKind != KIND_CHROMIUM) && !aHas(anAtom->mType))
				aTypeStore[aNumTypes++] = anAtom->mType;
		}
	}
	int aResult = aNumTypes;
	if ((aResult == 1) && (aTypeStore[0] == -1))
		aResult = 0;
	return aResult;
}

// 0x40c830
bool Grid::HasType(int theType)
{
	for (int aCol = 0; aCol < COLS; aCol++)
		for (int aRow = 0; aRow < ROWS; aRow++)
		{
			Atom* anAtom = mCells[aCol][aRow];
			if ((anAtom != NULL) && !anAtom->mPre && (anAtom->mType == theType))
				return true;
		}
	return false;
}

// ---------------------------------------------------------------------------------------------------------
// Input

// 0x40e710
void Grid::MouseMove(int x, int y)
{
	if (!gGridKeepTooltip)
	{
		mI368 = 0;
		mTooltipAtom = NULL;
	}
	if (!mLevelOver)
	{
		mPathFinder->Reset();
		PixelToCell(x, y, &mHoverCol);
		mM31c = x;
		mM320 = y;
		if ((mSelected != NULL) && mSelected->mPlaced)
		{
			int aCell[2] = {0, 0};
			PixelToCell(x, y, aCell);
			mPathFinder->Find(mSelected->mCol, mSelected->mRow, aCell[0], aCell[1]);
		}
	}
}

void Grid::MouseDrag(int x, int y)
{
	MouseMove(x, y);
}

// 0x40e7c0
void Grid::MouseUp(int x, int y, int theBtnNum)
{
	(void) x;
	(void) y;
	(void) theBtnNum;
	mI368 = 0;
	mTooltipAtom = NULL;
	if (!mLevelOver && !mIsOver)
		mSelected = NULL;
}

// 0x40e7f0: theClick is the 2003 framework's: 1 left, 2 double, -1 right
void Grid::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	int aClick = (theBtnNum == 1) ? -1 : (theClickCount > 1 ? 2 : 1);
	mI368 = 0;
	mTooltipAtom = NULL;
	if (mLevelOver || (mApp->mPauseCount >= 1))
		return;
	if ((aClick > 1) && (mSelected != NULL))
	{
		mSelected->OnPlaced();
		mSelected = NULL;
		return;
	}
	if (aClick < 0)
	{
		mSelected = NULL;
		mPathFinder->mPath.Clear();
		return;
	}
	if (!mApp->mBoard->mMeter.mFull && ((mSelected == NULL) || mSelected->mPlaced))
	{
		if ((mPathFinder->mPath.Size() < 1) || (mSelected == NULL))
		{
			mPathFinder->mPath.Clear();
			int aCell[2] = {0, 0};
			PixelToCell(x, y, aCell);
			Atom* aPrevious = mSelected;
			mSelected = (aCell[0] >= 0 && aCell[0] < COLS && aCell[1] >= 0 && aCell[1] < ROWS) ? mCells[aCell[0]][aCell[1]] : NULL;
			if (mSelected == aPrevious)
				mSelected = NULL;
			if (mSelected != NULL)
			{
				if (mSelected->mPre)
					mSelected = NULL;
				else
				{
					mAtoms.Remove(mSelected);
					mAtoms.Add(mSelected);
					int aPixel[2];
					CellToPixel(aCell[0], aCell[1], aPixel);
					mCursorX = aPixel[0];
					mCursorY = aPixel[1] + 0x19;
					mCursorX += 0x19;
					mSelectGlow = 0;
					mCursorColor = Color(mSelected->mColor.mRed, mSelected->mColor.mGreen, mSelected->mColor.mBlue, 255);
					mM30c = aCell[0];
					mM310 = aCell[1];
					mApp->mSndSelectAtom->Play();
				}
			}
			if ((mSelected == NULL) && !gGridBoardClicksOff)
				mApp->mBoard->MouseDown(mX + x, mY + y, aClick);
		}
		else
		{
			if (mSelected->mKind == KIND_BIOHAZARD)
				mApp->mSndMoveBioAtom->Play();
			else
				mApp->mSndMoveAtom->Play();
			mSelected->SetPath(&mPathFinder->mPath);
			mPathFinder->mPath.Clear();
			if (mB358)
			{
				mCombo = 0;
				mApp->mBoard->mF0ac = 0;
				return;
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------
// Update

// 0x412cd0
void Grid::Update()
{
	if (!mApp->mHasFocus || mApp->mMinimized || (mApp->mPauseCount > 0) || (mApp->mBoard == NULL))
		return;
	mApp->mWidgetManager->SetFocus(this);
	Board* aBoard = mApp->mBoard;

	if (mUnsolvable)
	{
		mBonusTick++;
		if (400 < mBonusTick)
		{
			mUnsolvable = false;
			mOverTick = 0;
			aBoard->mMeter.Add(500, 0);
			mB4c0 = false;
		}
	}
	else
	{
		mBonusTick -= 10;
		if (mBonusTick < 0)
			mBonusTick = 0;
	}

	if ((mArrowX != 0) || (mArrowY != 0))
	{
		mI4d0--;
		if (mI4d0 < 0)
		{
			mI4d0 = 5;
			Magnetise();
		}
	}
	if (mRadiationRound)
	{
		mI520--;
		if (mI520 < 1)
		{
			mB525 = mB524 ? false : true;
			mI520 = 100;
		}
	}

	int aCountdown = mI368;
	mStat374++;
	mI368 = aCountdown + 1;
	if ((aCountdown + 1 == 0x32) && (mHoverCol > -1) && (mHoverCol < 9) && (mHoverRow > -1) && (mHoverRow < 8) &&
		(mCells[mHoverCol][mHoverRow] != NULL) &&
		(mCells[mHoverCol][mHoverRow]->IsElement() || (mCells[mHoverCol][mHoverRow]->mKind != 0)) &&
		!mCells[mHoverCol][mHoverRow]->mPre)
	{
		mTooltipAtom = mCells[mHoverCol][mHoverRow];
		mTooltipAlpha = 0;
	}
	mTooltipAlpha += 3;
	if (mTooltipAlpha > 0xff)
		mTooltipAlpha = 0xff;

	if (mLevelOver)
	{
		mOverTick++;
		mI368 = 0;
		mTooltipAtom = NULL;
		mSelected = NULL;
		mCombo = 0;
		aBoard->mF0ac = 0;
		if (mOverTick == 300)
		{
			mApp->mMusicInterface->PlayMusic(0, 0x42, false);
			FlyAwayAll();
			if (!mApp->mB3ea0)
				aBoard->mAtomicus->Say(0xf);
		}
		if (mOverTick == 600)
			mApp->ShowGameOver();
	}

	if (mCombo < 2)
	{
		float aOld1 = mGeiger1;
		float aOld2 = mGeiger2;
		mGeiger1 = mGeiger1 - 0.01f;
		mGeiger2 = mGeiger2 - 0.01f;
		if (aOld1 - 0.01f < 0.0f)
			mGeiger1 = 0;
		if (aOld2 - 0.01f < 0.0f)
			mGeiger2 = 0;
	}
	else
	{
		float aTemp = 0.0f;
		if (3 < mCombo)
			aTemp = ((float) mCombo - 3.0f) * 0.25f;
		float aTarget1 = (float) (mCombo - 1) * 0.16666667f * 0.5f;
		aTemp = aTemp * 0.5f;
		if (aTemp < 0.0f)
			aTemp = 0.0f;
		if (mGeiger1 < aTarget1)
			mGeiger1 = mGeiger1 + 0.1f;
		if (mGeiger2 < aTemp)
			mGeiger2 = mGeiger2 + 0.1f;
		if (aTarget1 < mGeiger1)
			mGeiger1 = aTarget1;
		if (aTemp < mGeiger2)
			mGeiger2 = aTemp;
	}
	// the geiger counters grow with the combo
	mApp->mGeiger1Instance->SetVolume((double) mGeiger1);
	mApp->mGeiger2Instance->SetVolume((double) mGeiger2);
	if (mGeiger1 == 0.0f)
	{
		if (mApp->mGeiger1Instance->IsPlaying())
			mApp->mGeiger1Instance->Stop();
	}
	else if (!mApp->mGeiger1Instance->IsPlaying())
		mApp->mGeiger1Instance->Play(true, false);
	if (mGeiger2 == 0.0f)
	{
		if (mApp->mGeiger2Instance->IsPlaying())
			mApp->mGeiger2Instance->Stop();
	}
	else if (!mApp->mGeiger2Instance->IsPlaying())
		mApp->mGeiger2Instance->Play(true, false);

	if (mB334)
	{
		mApp->mSndAtomAppear->Play();
		mB334 = false;
	}
	if (mB335)
	{
		mApp->mSndWhoosh->Play();
		mB335 = false;
	}
	int aRoll = mApp->mRand.Get(0x32, false);
	if (mLevelOver)
		aRoll = mApp->mRand.Get(2, false);
	if (aRoll == 0)
	{
		int aCol = mApp->mRand.Get(9, false);
		int aRow = mApp->mRand.Get(8, false);
		if (mCells[aCol][aRow] != NULL)
			SpawnBonds(aCol, aRow);
	}

	mPulse = mPulse + mPulseStep;
	if (100.0f < mPulse)
		mPulseStep = -0.15f;
	if (mPulse < 50.0f)
		mPulseStep = 0.15f;

	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		if (!anAtom->mDead)
			anAtom->Update();
		if (anAtom->mDead)
		{
			mAtoms.Remove(anAtom);
			delete anAtom;
		}
		anAtom = mAtoms.Next();
	}
	if (mB32c)
	{
		FindMolecules();
		mB32c = false;
		if (mB358)
		{
			mCombo = 0;
			aBoard->mF0ac = 0;
		}
		int aCount = CountAtoms(false);
		if (aCount > 0x47)
		{
			mLevelOver = true;
			mOverTick = 0;
			FlushStats();
		}
	}
	mSparkles.Reset();
	Sparkle* aSparkle = mSparkles.Next();
	while (aSparkle != NULL)
	{
		if (!aSparkle->Update())
		{
			mSparkles.Remove(aSparkle);
			delete aSparkle;
		}
		aSparkle = mSparkles.Next();
	}

	if (mSelected == NULL)
	{
		mCursorY = mM320 + 0x19;
		mCursorX = mM31c + 0x19;
		if ((0x4b < mSelectGlow) && (mSelectGlow - 10 > 0x4a))
			mSelectGlow -= 10;
		else
			mSelectGlow = 0x4b;
	}
	else
	{
		if (mSelectGlow < 0xff)
		{
			mSelectGlow += 0x19;
			if (0xff < mSelectGlow)
				mSelectGlow = 0xff;
		}
		else
			mSelectGlow = 0xff - mApp->mRand.Get(10, false);
	}

	mB33c = true;
	for (int aCol = 0; aCol < COLS; aCol++)
	{
		for (int aRow = 0; aRow < ROWS; aRow++)
		{
			int* aState = &mCellState[aCol][aRow];
			bool aStep = false;
			if ((*aState < 0) && (-1000 < *aState))
			{
				*aState = 0;
				aStep = true;
			}
			else if (*aState < 0x100)
				aStep = *aState < 0xff;
			else
				*aState = 0xff;
			if (aStep)
			{
				if (!mLevelOver && ((*aState = *aState + 1) > 0xff))
					*aState = 0xff;
				else if (*aState < 0x96)
					mB33c = false;
			}
			else if (*aState < 0x96)
				mB33c = false;
		}
	}
	if (mB33c && !mB33d)
	{
		mB33d = true;
		aBoard->mAtomicus->Say(8);
	}

	// the molecules just made: their atoms shake, then fly away
	bool anyFlown = false;
	mGroups.Reset();
	PtrList<Atom>* aGroup = mGroups.Next();
	while (aGroup != NULL)
	{
		aGroup->Reset();
		Atom* aMember = aGroup->Next();
		while (aMember != NULL)
		{
			aMember->mE8 = mApp->mRand.Get(aMember->mE4 / 5, true);
			aMember->mEC = mApp->mRand.Get(aMember->mE4 / 5, true);
			aMember->mE4--;
			if (aMember->mE4 < 1)
			{
				aGroup->Remove(aMember);
				if (aMember->mKind == KIND_BIOHAZARD)
					Obliterate(aMember);
				else
					aMember->FlyAway(aBoard->LastTarget());
				anyFlown = true;
			}
			aMember = aGroup->Next();
		}
		if (aGroup->Size() == 0)
		{
			mGroups.Remove(aGroup);
			delete aGroup;
		}
		aGroup = mGroups.Next();
	}
	if (anyFlown)
		CheckUnsolvable();

	if ((mApp->mMode == 1) && (aBoard->mI16c < 6) && (aBoard->mI170 < 6))
	{
		if (aBoard->mI184 < 1)
		{
			aBoard->mMeter.mValue = mF3ac + aBoard->mMeter.mValue;
			if (aBoard->mMeter.mMax < aBoard->mMeter.mValue)
			{
				aBoard->mMeter.mValue = 0;
				aBoard->mAtomicus->Say(0x10);
				mNumColors++;
				if (mNumColors > 0xb)
					mNumColors = 0xb;
				mBiohazardChance = 0;
				mF3ac = 0.05f;
				aBoard->mMeter.mLevel++;
				aBoard->mMeter.mMoleculesMade++;
				int aSavedProgress = (int) aBoard->mProgress;
				int aSaved148 = aBoard->mI148;
				int aSaved14c = aBoard->mI14c;
				aBoard->mI148 = 0;
				aBoard->mI14c = 1;
				aBoard->mI148 = 0;
				aBoard->StepBomb();
				aBoard->mI148 = 0;
				aBoard->StepBomb();
				aBoard->mI148 = 0;
				aBoard->StepBomb();
				aBoard->mI148 = aSaved148;
				aBoard->mProgress = (float) aSavedProgress;
				aBoard->mI14c = aSaved14c;
			}
			mChromiumChance = mChromiumChance + 0.0001f;
			mBiohazardChance = mBiohazardChance + 0.0001f;
			if (3.0f < mChromiumChance)
				mChromiumChance = 3.0f;
			if (8.0f < mBiohazardChance)
				mBiohazardChance = 8.0f;
			mF3ac = mF3ac + 1e-06f;
			if (0.6f < mF3ac)
				mF3ac = 0.6f;
		}
		if (!aBoard->mMeter.mFull && mB33d)
		{
			if (aBoard->mI184 < 1)
			{
				mF390 = mF390 - 1.25f;
				if ((0.5f < aBoard->mMeter.mValue / aBoard->mMeter.mMax) || (1 < aBoard->mMeter.mLevel))
				{
					int aCount = CountAtoms(false);
					mF390 = mF390 - (1.0f - (float) aCount * 0.013888889f);
				}
				if (mF390 <= 0.0f)
				{
					mF390 = mF394;
					AfterMove(true);
				}
			}
			mI398++;
			if (300 < mI398)
			{
				mI398 = 0;
				aBoard->CountDown();
			}
		}
	}
	CheckHints();
}

// ---------------------------------------------------------------------------------------------------------
// Draw

// 0x413880: the board draws the field in the middle of its own picture (DrawField); as a widget it draws nothing
void Grid::Draw(Graphics* g)
{
	(void) g;
}

void Grid::DrawField(Graphics* g)
{
	bool aToggle = true;
	for (int aCol = 0, aX = 0; aCol < COLS; aCol++, aX += CELL)
	{
		bool aRowToggle = aToggle;
		for (int aRow = 0, aY = 0; aRow < ROWS; aRow++, aY += CELL)
		{
			int aState = mCellState[aCol][aRow];
			if (aState < 0)
				aState = 0;
			Atom* anAtom = mCells[aCol][aRow];
			bool anElement = (anAtom != NULL) && anAtom->mFlag_e0;
			bool aFill = false;
			Color aColor(0, 0, 0, 0);
			Board* aBoard = mApp->mBoard;
			if (mRadiationRound && mRadiation[aCol][aRow])
			{
				aColor = aRowToggle ? Color(200, 200, 0, aState / 2) : Color(200, 200, 0, aState);
				aFill = true;
			}
			else if (aRowToggle)
			{
				if (!anElement)
				{
					int aB = aBoard->mTint.mBlue < 0x100 ? aBoard->mTint.mBlue : 0xff;
					int aG = aBoard->mTint.mGreen < 0x65 ? aBoard->mTint.mGreen : 100;
					int aR = aBoard->mTint.mRed < 0x4c ? aBoard->mTint.mRed : 0x4b;
					if ((mHoverCol == aCol) && (mHoverRow == aRow))
						aColor = Color(aR, aG, aB, aState / 2);
					else
					{
						int aG2 = aBoard->mTint.mGreen < 0x4c ? aBoard->mTint.mGreen : 0x4b;
						aColor = Color(aR, aG2, aB, aState / 2);
					}
				}
				else
					aColor = Color(anAtom->mColor.mRed, anAtom->mColor.mGreen, anAtom->mColor.mBlue, aState / 2);
				aFill = true;
			}
			else if (anElement)
			{
				aColor = Color(anAtom->mColor.mRed, anAtom->mColor.mGreen, anAtom->mColor.mBlue, aState / 4);
				aFill = true;
			}
			if (aFill)
			{
				g->SetColor(aColor);
				g->FillRect(aX, aY, 0x32, 0x32);
			}
			g->SetColor(Color(0, 0, 0, aState));
			g->DrawLine(aX, aY - 1, aX + 0x32, aY - 1);
			g->DrawLine(aX, aY, aX + 0x32, aY);
			g->DrawLine(aX, aY, aX, aY + 0x32);
			g->DrawLine(aX, aY + 0x32, aX + 0x32, aY + 0x32);
			g->DrawLine(aX + 0x32, aY, aX + 0x32, aY + 0x32);
			aRowToggle = !aRowToggle;
		}
		aToggle = !aToggle;
	}

	// the sparks of the reveal
	mSparkles.Reset();
	Sparkle* aSparkle = mSparkles.Next();
	while (aSparkle != NULL)
	{
		aSparkle->Draw(g);
		aSparkle = mSparkles.Next();
	}
	if (0x80 < mSelectGlow)
	{
		g->SetColorizeImages(true);
		Color aColor(mCursorColor.mRed < 0x97 ? 0x96 : mCursorColor.mRed, mCursorColor.mGreen < 0x97 ? 0x96 : mCursorColor.mGreen,
			mCursorColor.mBlue < 0x97 ? 0x96 : mCursorColor.mBlue, mSelectGlow);
		g->SetColor(aColor);
		mApp->mStarBurst->Draw(g, (mCursorX - 0x19) - (mApp->mStarBurst->mWidth >> 1),
			(mCursorY - 0x19) - (mApp->mStarBurst->mHeight >> 1));
		g->SetColorizeImages(true);
	}

	if ((mApp->mMode != 1) || (mApp->mPauseCount < 1))
	{
		mAtoms.Reset();
		Atom* anAtom = mAtoms.Next();
		while (anAtom != NULL)
		{
			anAtom->DrawAimLine(g);
			anAtom = mAtoms.Next();
		}
		mAtoms.Reset();
		anAtom = mAtoms.Next();
		while (anAtom != NULL)
		{
			anAtom->Draw(g);
			anAtom = mAtoms.Next();
		}
		if (mApp->mPauseCount == 0)
		{
			mPathFinder->mPath.Reset();
			int* aNode = mPathFinder->mPath.Next();
			while (aNode != NULL)
			{
				int aPixel[2] = {0, 0};
				CellToPixel(aNode[0], aNode[1], aPixel);
				mPathFinder->DrawNode(aNode, aPixel[0], aPixel[1], g);
				aNode = mPathFinder->mPath.Next();
			}
		}
	}

	// the molecules just made: shaking, with the bonds between their atoms
	bool aFirst = true;
	mGroups.Reset();
	PtrList<Atom>* aGroup = mGroups.Next();
	while (aGroup != NULL)
	{
		aGroup->Reset();
		Atom* aMember = aGroup->Next();
		while (aMember != NULL)
		{
			int aX = aMember->mE8 + aMember->mX;
			int aY = aMember->mY + aMember->mEC;
			mApp->mAtomC[aMember->mType]->DrawCentered(g, aX, aY);
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, 0x96));
			int aShake = aMember->mE4 < 3 ? 2 : aMember->mE4;
			mApp->mAtomC[aMember->mType]->DrawCentered(g, mApp->mRand.Get(aShake / 3, true) + aMember->mX,
				mApp->mRand.Get(aShake / 3, true) + aMember->mY);
			g->SetColor(Color(0xff, 0xff, 0xff, 100));
			mApp->mAtomC[aMember->mType]->DrawCentered(g, mApp->mRand.Get(aShake / 2, true) + aMember->mX,
				mApp->mRand.Get(aShake / 2, true) + aMember->mY);
			g->SetColor(Color(0xff, 0xff, 0xff, aMember->mE4 > 0xff ? 0xff : aMember->mE4));
			mApp->mAtomC[11]->DrawCentered(g, aX, aY);
			if (aMember->mKind == KIND_BIOHAZARD)
			{
				g->SetColor(Color(0, 0, 0, 0x80));
				mApp->mBiohazard[5]->DrawCentered(g, aX, aY);
			}
			g->SetColorizeImages(false);
			aMember = aGroup->Next();
		}
		int aSize = aGroup->Size();
		for (int i = 0; i < aSize; i++)
		{
			Atom* aFrom = aGroup->Get(i);
			aGroup->Reset();
			aGroup->mCursor = i + 1;
			Atom* aTo = aGroup->Next();
			while (aTo != NULL)
			{
				if (((aTo->mX == aFrom->mX) && (abs(aTo->mY - aFrom->mY) < 0x3c)) ||
					((aTo->mY == aFrom->mY) && (abs(aTo->mX - aFrom->mX) < 0x3c)))
				{
					DrawBond(g, aTo, aFrom, aFirst);
					aFirst = false;
				}
				aTo = aGroup->Next();
			}
		}
		aGroup = mGroups.Next();
	}

	// "this radiation round can no longer be solved", with a shadow
	if (0 < mBonusTick)
	{
		static const unsigned int aTexts[4] = {0x4af850, 0x4af840, 0x4af834, 0x4af82c};
		g->SetFont(mApp->mAnnounceFont);
		int aAlpha = mBonusTick * 2;
		if (aAlpha > 0xfe)
			aAlpha = 0xff;
		g->SetColor(Color(0, 0, 0, aAlpha));
		g->Translate(4, 4);
		for (int i = 0; i < 4; i++)
		{
			const std::string& aText = GT(aTexts[i]);
			g->DrawString(aText, mWidth / 2 - mApp->mAnnounceFont->StringWidth(aText) / 2, mHeight / 2 + (i - 1) * 0x1e);
		}
		g->Translate(-4, -4);
		g->SetColor(Color(0xff, 200, 200, aAlpha));
		for (int i = 0; i < 4; i++)
		{
			const std::string& aText = GT(aTexts[i]);
			g->DrawString(aText, mWidth / 2 - mApp->mAnnounceFont->StringWidth(aText) / 2, mHeight / 2 + (i - 1) * 0x1e);
		}
	}
}
