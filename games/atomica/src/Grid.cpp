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
	mCursorColor(0, 0, 0, 255),
	mGroupColor(0, 0, 0, 255)
{
	mCursorX = 0;
	mCursorY = 0;
	mPickedCol = 0;
	mPickedRow = 0;
	mHoverCol = mHoverRow = 0;
	mMouseX = mMouseY = 0;
	mGroupX = mGroupY = 0;
	mGeiger1 = mGeiger2 = 0;
	mBiohazards = 0;
	mReadyElementX = mReadyElementY = 0;
	mArrowX = mArrowY = 0;
	mRevealSaid = false;
	mPausedByKey = false;
	mElementReady = false;
	mCheckMolecules = false;
	mAppearSoundDue = false;
	mWhooshDue = false;
	mPickedCol = -1000;			// 0xfffffc18
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
	mGameOver = false;
	mGameOverTick = 0;
	mSelectGlow = 0;
	mComboEnded = true;
	mLevelUpdates = mLevelAtoms = mLevelMolecules = mLevelBestCombo = mLevelBonusMolecules = mLevelBonusUpgrades = 0;
	mReadyElement = 0;
	mHoverTick = 0;
	mTooltipAtom = NULL;
	mBonusLevel = false;
	mUnsolvable = false;
	for (int i = 0; i < 0xff; i++)
		mHintShown[i] = false;
	mRadiationRound = false;
	mRadiationBlinkTimer = 0;
	mRadiationBlink = false;
	mAllRadiated = false;
	mPulse = 0;
	mPulseStep = 0;
	mRoundTimer = mRoundTime = 0;
	mCountDownTick = 0;
	mNumColors = 0;
	mChromiumChance = mBiohazardChance = 0;
	mAtomsPerRound = 0;
	mLevelSetUp = false;
	mMeterFillStep = 0;
	mRounds = 0;
	mMagnetTick = 0;
	mBonusTick = 0;
	mFieldRevealed = false;
	mTooltipAlpha = 0;
	mApp->mMusicInterface->PlayMusic(0, mApp->mMode == 0 ? 0xc : 0x2c, false);
}

// 0x40c340 (with 0x405fa0, the pause count's decrement: it stops at 0)
Grid::~Grid()
{
	if (mPausedByKey)
	{
		mApp->mPauseCount--;
		if (mApp->mPauseCount < 0)		// 0x405fa0: the count stops at 0
			mApp->mPauseCount = 0;
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
	mRevealSaid = false;
	mBiohazards = 0;
	Resize(0xa5, 0x28, 0x1c2, 400);
	mRoundTimer = mRoundTime;
	mPulse = 10.0f;
	mPulseStep = 0.15f;
	mRoundTime = 300.0f;
	mCountDownTick = 0;
	FlushStats();
}

// 0x40c540: every atom flies away (the level is over, or the bomb went off)
void Grid::FlyAwayAll()
{
	mUnsolvable = false;
	mCombo = 0;
	mApp->mBoard->mNeedleTarget = 0;
	mApp->mSndWhoosh->Play();
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		if (!mGameOver)
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
	mApp->mBoard->mBonus->mRound = 0;
}

// 0x40c8c0: the totals of the field go to the board's
void Grid::FlushStats()
{
	Board* aBoard = mApp->mBoard;
	aBoard->mStatUpdates += mLevelUpdates;
	aBoard->mStatAtoms += mLevelAtoms;
	aBoard->mStatMolecules += mLevelMolecules;
	aBoard->mStatBestCombo = aBoard->mStatBestCombo > mLevelBestCombo ? aBoard->mStatBestCombo : mLevelBestCombo;
	aBoard->mStatBonusMolecules += mLevelBonusMolecules;
	aBoard->mStatBonusUpgrades += mLevelBonusUpgrades;
	mLevelUpdates = 0;
	mLevelAtoms = 0;
	mLevelMolecules = 0;
	mLevelBestCombo = 0;
	mLevelBonusMolecules = 0;
	mLevelBonusUpgrades = 0;
}

// 0x40c880 (with 0x405fa0, as in the destructor above)
void Grid::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_SPACE)
	{
		if (mPausedByKey)
		{
			mPausedByKey = false;
			mApp->mPauseCount--;
			if (mApp->mPauseCount < 0)		// 0x405fa0: the count stops at 0
				mApp->mPauseCount = 0;
			return;
		}
		mPausedByKey = true;
		mApp->mPauseCount++;
	}
}

// 0x40c6a0: how many atoms are on the field (theNoElements: the element atoms are left out)
int Grid::CountAtoms(bool theNoElements)
{
	int aCount = 0;
	for (int aCol = 0; aCol < COLS; aCol++)
	{
		for (int aRow = 0; aRow < ROWS; aRow++)
		{
			Atom* anAtom = mCells[aCol][aRow];
			if ((anAtom != NULL) && !anAtom->mPre)
			{
				if (!theNoElements)
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
		mHoverTick = 0;
		mTooltipAtom = NULL;
	}
	if (!mGameOver)
	{
		mPathFinder->Reset();
		PixelToCell(x, y, &mHoverCol);
		mMouseX = x;
		mMouseY = y;
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
	mHoverTick = 0;
	mTooltipAtom = NULL;
	if (!mGameOver && !mIsOver)
		mSelected = NULL;
}

// 0x40e7f0: theClick is the 2003 framework's: 1 left, 2 double, -1 right
void Grid::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	int aClick = (theBtnNum == 1) ? -1 : (theClickCount > 1 ? 2 : 1);
	mHoverTick = 0;
	mTooltipAtom = NULL;
	if (mGameOver || (mApp->mPauseCount >= 1))
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
					mPickedCol = aCell[0];
					mPickedRow = aCell[1];
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
			if (mComboEnded)
			{
				mCombo = 0;
				mApp->mBoard->mNeedleTarget = 0;
				return;
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------
// Update

// 0x412cd0 / 0x40c230 / 0x40c240
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
			mGameOverTick = 0;
			aBoard->mMeter.Add(500, 0);
			mBonusLevel = false;
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
		mMagnetTick--;
		if (mMagnetTick < 0)
		{
			mMagnetTick = 5;
			Magnetise();
		}
	}
	if (mRadiationRound)
	{
		mRadiationBlinkTimer--;
		if (mRadiationBlinkTimer < 1)
		{
			mRadiationBlink = !mRadiationBlink;
			mRadiationBlinkTimer = 100;
		}
	}

	int aCountdown = mHoverTick;
	mLevelUpdates++;
	mHoverTick = aCountdown + 1;
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

	if (mGameOver)
	{
		mGameOverTick++;
		mHoverTick = 0;
		mTooltipAtom = NULL;
		mSelected = NULL;
		mCombo = 0;
		aBoard->mNeedleTarget = 0;
		if (mGameOverTick == 300)
		{
			mApp->mMusicInterface->PlayMusic(0, 0x42, false);
			FlyAwayAll();
			aBoard->mAtomicus->Say(0xf);		// (not after a game given up, in the original: AppGame.cpp, ShowGameOver)
		}
		if (mGameOverTick == 600)
			mApp->ShowGameOver();
	}

	if (mCombo < 2)
	{
		float aOldGeiger1 = mGeiger1;
		float aOldGeiger2 = mGeiger2;
		mGeiger1 = mGeiger1 - 0.01f;
		mGeiger2 = mGeiger2 - 0.01f;
		if (aOldGeiger1 - 0.01f < 0.0f)
			mGeiger1 = 0;
		if (aOldGeiger2 - 0.01f < 0.0f)
			mGeiger2 = 0;
	}
	else
	{
		float aGeiger2Target = 0.0f;
		if (3 < mCombo)
			aGeiger2Target = ((float) mCombo - 3.0f) * 0.25f;
		float aGeiger1Target = (float) (mCombo - 1) * 0.16666667f * 0.5f;
		aGeiger2Target = aGeiger2Target * 0.5f;
		if (aGeiger2Target < 0.0f)
			aGeiger2Target = 0.0f;
		if (mGeiger1 < aGeiger1Target)
			mGeiger1 = mGeiger1 + 0.1f;
		if (mGeiger2 < aGeiger2Target)
			mGeiger2 = mGeiger2 + 0.1f;
		if (aGeiger1Target < mGeiger1)
			mGeiger1 = aGeiger1Target;
		if (aGeiger2Target < mGeiger2)
			mGeiger2 = aGeiger2Target;
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

	if (mAppearSoundDue)
	{
		mApp->mSndAtomAppear->Play();
		mAppearSoundDue = false;
	}
	if (mWhooshDue)
	{
		mApp->mSndWhoosh->Play();
		mWhooshDue = false;
	}
	int aRoll = mApp->mRand.Get(0x32, false);
	if (mGameOver)
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
	if (mCheckMolecules)
	{
		FindMolecules();
		mCheckMolecules = false;
		if (mComboEnded)
		{
			mCombo = 0;
			aBoard->mNeedleTarget = 0;
		}
		int aCount = CountAtoms(false);
		if (aCount > 0x47)
		{
			mGameOver = true;
			mGameOverTick = 0;
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
		mCursorY = mMouseY + 0x19;
		mCursorX = mMouseX + 0x19;
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

	mFieldRevealed = true;
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
				if (!mGameOver && ((*aState = *aState + 1) > 0xff))
					*aState = 0xff;
				else if (*aState < 0x96)
					mFieldRevealed = false;
			}
			else if (*aState < 0x96)
				mFieldRevealed = false;
		}
	}
	if (mFieldRevealed && !mRevealSaid)
	{
		mRevealSaid = true;
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
			aMember->mShakeX = mApp->mRand.Get(aMember->mShakeTime / 5, true);
			aMember->mShakeY = mApp->mRand.Get(aMember->mShakeTime / 5, true);
			aMember->mShakeTime--;
			if (aMember->mShakeTime < 1)
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

	if ((mApp->mMode == 1) && (aBoard->mGetReadyAlpha < 6) && (aBoard->mGameOverAlpha < 6))
	{
		if (aBoard->mFreezeMoves < 1)
		{
			aBoard->mMeter.mValue = mMeterFillStep + aBoard->mMeter.mValue;
			if (aBoard->mMeter.mMax < aBoard->mMeter.mValue)
			{
				aBoard->mMeter.mValue = 0;
				aBoard->mAtomicus->Say(0x10);
				mNumColors++;
				if (mNumColors > 0xb)
					mNumColors = 0xb;
				mBiohazardChance = 0;
				mMeterFillStep = 0.05f;
				aBoard->mMeter.mLevel++;
				aBoard->mMeter.mMoleculesMade++;
				int aSavedProgress = (int) aBoard->mProgress;
				int aSavedBombTimer = aBoard->mBombTimer;
				int aSavedBombAtoms = aBoard->mBombAtomsPerStep;
				aBoard->mBombTimer = 0;
				aBoard->mBombAtomsPerStep = 1;
				aBoard->mBombTimer = 0;
				aBoard->StepBomb();
				aBoard->mBombTimer = 0;
				aBoard->StepBomb();
				aBoard->mBombTimer = 0;
				aBoard->StepBomb();
				aBoard->mBombTimer = aSavedBombTimer;
				aBoard->mProgress = (float) aSavedProgress;
				aBoard->mBombAtomsPerStep = aSavedBombAtoms;
			}
			mChromiumChance = mChromiumChance + 0.0001f;
			mBiohazardChance = mBiohazardChance + 0.0001f;
			if (3.0f < mChromiumChance)
				mChromiumChance = 3.0f;
			if (8.0f < mBiohazardChance)
				mBiohazardChance = 8.0f;
			mMeterFillStep = mMeterFillStep + 1e-06f;
			if (0.6f < mMeterFillStep)
				mMeterFillStep = 0.6f;
		}
		if (!aBoard->mMeter.mFull && mRevealSaid)
		{
			if (aBoard->mFreezeMoves < 1)
			{
				mRoundTimer = mRoundTimer - 1.25f;
				if ((0.5f < aBoard->mMeter.mValue / aBoard->mMeter.mMax) || (1 < aBoard->mMeter.mLevel))
				{
					int aCount = CountAtoms(false);
					mRoundTimer = mRoundTimer - (1.0f - (float) aCount * 0.013888889f);
				}
				if (mRoundTimer <= 0.0f)
				{
					mRoundTimer = mRoundTime;
					AfterMove(true);
				}
			}
			mCountDownTick++;
			if (300 < mCountDownTick)
			{
				mCountDownTick = 0;
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

// 0x425ce0: the loop over the bonds sets the group's cursor (aGroup->mCursor = i + 1)
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
			bool anElement = (anAtom != NULL) && anAtom->mIsElementAtom;
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
			int aX = aMember->mShakeX + aMember->mX;
			int aY = aMember->mY + aMember->mShakeY;
			mApp->mAtomC[aMember->mType]->DrawCentered(g, aX, aY);
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, 0x96));
			int aShake = aMember->mShakeTime < 3 ? 2 : aMember->mShakeTime;
			mApp->mAtomC[aMember->mType]->DrawCentered(g, mApp->mRand.Get(aShake / 3, true) + aMember->mX,
				mApp->mRand.Get(aShake / 3, true) + aMember->mY);
			g->SetColor(Color(0xff, 0xff, 0xff, 100));
			mApp->mAtomC[aMember->mType]->DrawCentered(g, mApp->mRand.Get(aShake / 2, true) + aMember->mX,
				mApp->mRand.Get(aShake / 2, true) + aMember->mY);
			g->SetColor(Color(0xff, 0xff, 0xff, (int) ((double) aMember->mShakeTime * (double) 0.02f * 255.0)));
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
