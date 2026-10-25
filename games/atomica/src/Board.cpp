// The screen of a game (see Board.h).

#include "Board.h"
#include "AtomicaApp.h"
#include "Atomicus.h"
#include "BonusMolecules.h"
#include "Grid.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// Meter

// 0x405a80
Meter::Meter()
{
	Init();
	mMax = 8.0f;
}

// 0x4056b0
void Meter::Reset()
{
	mValue = 0;
	mShown = 0;
	mFull = false;
}

// 0x4056c0: (the original lets two debug globals change the start level; both are never set)
void Meter::Init()
{
	mLevel = 1;
	mMoleculesMade = 1;
	Reset();
	mTick = 0;
	mCountdown = 0x14;
	mOverTick = 0;
	mFrozen = false;
}

// 0x405700: the two bars of the level meter, at both ends of the machine's top
void Meter::Draw(Graphics* g)
{
	float aWidth1 = mShown / mMax * 112.0f;
	g->SetColor(Color(100, 255, 100, 255));
	int aWidth = (int) aWidth1;
	g->FillRect(0xd9, 0x12, aWidth, 8);
	g->SetColor(Color(150, 255, 150, 255));
	g->FillRect(0xd9, 0x1a, aWidth, 6);
	float aWidth2 = mShown / mMax * 118.0f;
	g->SetColor(Color(100, 255, 100, 255));
	int aWidthRight = (int) aWidth2;
	int aX = (int) (570.0f - aWidth2);
	g->FillRect(aX, 0x12, aWidthRight, 8);
	g->SetColor(Color(150, 255, 150, 255));
	g->FillRect(aX, 0x1a, aWidthRight, 6);
}

// 0x405820
void Meter::Update()
{
	if ((mValue <= mShown) || mFrozen)
		mShown = mValue;
	else
		mShown = mShown + 0.05f;
	mCountdown--;
	if (mCountdown < 0)
	{
		mCountdown = 0x14;
		mTick++;
	}
	if (mFull)
	{
		float aValue = mValue * 0.97f;
		mOverTick++;
		mTick = -10;
		mValue = aValue;
		mShown = aValue;
		if (aValue < 0.0f)
			mValue = 0;
	}
}

// 0x4058a0: a hit takes the meter down
void Meter::Hit(int theAmount)
{
	float aValue = mValue - (float) theAmount;
	mValue = aValue;
	float aShown = mShown - (float) theAmount;
	mShown = aShown;
	if (aShown < 0.0f)
		mShown = 0;
	if (aValue < 0.0f)
		mValue = 0;
}

// 0x4058f0: molecules fill the meter (and in the action mode, where it is frozen at the top, they empty it)
void Meter::Add(int theAmount, int theCombo)
{
	if (mFrozen)
	{
		float aValue = (mValue - (float) (theAmount * 10)) - (float) (theCombo * 10);
		mValue = aValue;
		float aShown = (mShown - (float) (theAmount * 10)) - (float) (theCombo * 10);
		mShown = aShown;
		if (aShown < 0.0f)
			mShown = 0;
		if (aValue < 0.0f)
			mValue = 0;
	}
	else
	{
		float aValue = (float) theAmount + mValue;
		mValue = aValue;
		if (mMax < aValue)
		{
			mValue = mMax;
			mFull = true;
			mOverTick = 0;
		}
	}
}

// 0x405990
void Meter::Save(FILE* f)
{
	fwrite(&mLevel, 4, 1, f);
	fwrite(&mValue, 4, 1, f);
	fwrite(&mMax, 4, 1, f);
	fwrite(&mTick, 4, 1, f);
	fwrite(&mCountdown, 4, 1, f);
	unsigned char aFull = mFull ? 1 : 0;
	fwrite(&aFull, 1, 1, f);
	fwrite(&mOverTick, 4, 1, f);
}

// 0x405a00
void Meter::Load(FILE* f)
{
	fread(&mLevel, 4, 1, f);
	fread(&mValue, 4, 1, f);
	fread(&mMax, 4, 1, f);
	fread(&mTick, 4, 1, f);
	fread(&mCountdown, 4, 1, f);
	unsigned char aFull = 0;
	fread(&aFull, 1, 1, f);
	mFull = aFull != 0;
	fread(&mOverTick, 4, 1, f);
	mShown = mValue;
}

// ---------------------------------------------------------------------------------------------------------
// Board

// 0x432c20
Board::Board(AtomicaApp* theApp) :
	mApp(theApp),
	mGrid(NULL),
	mFlash(0, 0, 0, 255),
	mBombTint(255, 255, 255, 255),
	mTint(255, 255, 255, 255)
{
	mMouseX = 0;
	mMouseY = 0;
	mTooltip[0] = 0;
	mI16c = 0x163;
	mI170 = 0;
	mI174 = 0;
	mI1c0 = 0;
	mStat1c4 = mStat1c8 = mStat1cc = mStat1d0 = mStat1d4 = mStat1d8 = mStat1dc = 0;
	mTintOn = false;
	mGeigerGlow = 0;
	mGeigerGlowStep = 1.0f;
	mNumColorsStart = 3;
	mBombOn = false;
	mBonus = new BonusMolecules(mApp);
	mProgress = 0;
	mBombPhase = 0;
	mBombFrame = 0;
	mPulse = 0;
	mPulseStep = 0.5f;
	mI90 = 0;
	mF8c = 50.0f;
	mF0ac = 0;
	mNeedle = 0;
	mNeedleSpeed = 3.0f;
	mNeedleTick = 0;
	mNeedleBlink = false;
	mMonitorSlide = 0xfa;
	mI148 = mI14c = 0;
	mF150 = 0;
	mB164 = false;
	mMonitorSlide = 0xfa;

	mButtonOptions = new ButtonWidget(0, this);
	mButtons.Add(mButtonOptions);
	mButtonOptions->mBtnNoDraw = true;
	mButtonOptions->mDoFinger = true;
	mButtonOptions->Resize(0x16, 0x1c0, 100, 0x20);

	mButtonMenu = new ButtonWidget(2, this);
	mButtons.Add(mButtonMenu);
	mButtonMenu->mBtnNoDraw = true;
	mButtonMenu->mDoFinger = true;
	mButtonMenu->Resize(0x16, 0xfe, 100, 0x20);

	mButtonBomb = new ButtonWidget(1, this);
	mButtons.Add(mButtonBomb);
	mButtonBomb->mBtnNoDraw = true;
	mButtonBomb->mDoFinger = true;
	mButtonBomb->Resize(0x164, 0x1ab, 0x44, 0x2e);

	mAtomicus = new Atomicus(mApp);
	mI184 = 0;
	mI180 = 0;
	mI194 = 0xff;
	mI198 = 0;
	mI188 = 0;
	mI18c = 0;
	mI190 = 0;
	mScore = 0;
	mGeigerGlow = 0;
	SetTint(0xff, 0xff, 0xff);
}

// 0x4319c0
Board::~Board()
{
	if (mGrid != NULL)
	{
		mApp->SafeDeleteWidget(mGrid);
		mGrid = NULL;
	}
	mButtons.Reset();
	Widget* aButton = mButtons.Next();
	while (aButton != NULL)
	{
		mApp->SafeDeleteWidget(aButton);
		aButton = mButtons.Next();
	}
	mButtons.Clear();
	mEffects.Reset();
	Effect* anEffect = mEffects.Next();
	while (anEffect != NULL)
	{
		delete anEffect;
		anEffect = mEffects.Next();
	}
	mEffects.Clear();
	delete mAtomicus;
	mAtomicus = NULL;
	delete mBonus;
	mBonus = NULL;
	if (mApp->mBoard == this)
		mApp->mBoard = NULL;
}

// 0x431b50: a game starts
void Board::Start()
{
	if (mGrid != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mGrid);
		mApp->SafeDeleteWidget(mGrid);
	}
	mScore = 0;
	mGrid = new Grid(mApp);
	mGrid->Start();
	mApp->mWidgetManager->AddWidget(mGrid);
	mApp->mWidgetManager->BringToFront(mGrid);
	mApp->mWidgetManager->BringToFront(mButtonBomb);
	mMeter.Init();
	mNumColorsStart = 3;
	mI148 = 0;
	mI14c = 0;
	mBombOn = false;
	mProgress = 0;
	mBombPhase = 0;
	mBombFrame = 0;
	mPulse = 0;
	mPulseStep = 0.5f;
	mF8c = 50.0f;
	mFlash = Color(0, 0, 0, 255);
	mF0ac = 0;
	mNeedle = 0;
	mNeedleSpeed = 3.0f;
	mNeedleTick = 0;
	mNeedleBlink = false;
	mApp->mWidgetManager->SetFocus(mGrid);
	mAtomicus->Say(6);
}

// 0x431d20
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mButtons.Reset();
	Widget* aButton = mButtons.Next();
	while (aButton != NULL)
	{
		theWidgetManager->AddWidget(aButton);
		aButton = mButtons.Next();
	}
}

// 0x431d60
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	mButtons.Reset();
	Widget* aButton = mButtons.Next();
	while (aButton != NULL)
	{
		theWidgetManager->RemoveWidget(aButton);
		aButton = mButtons.Next();
	}
	if (mGrid != NULL)
		theWidgetManager->RemoveWidget(mGrid);
}

// 0x431db0: the mouse moves over the board: the field is told, and a text may appear for the thing under it
void Board::MouseMove(int x, int y)
{
	if (mGrid != NULL)
	{
		mGrid->MouseMove(x - mGrid->mX, y - mGrid->mY);
		mGrid->mI368 = 0;
		mGrid->mTooltipAtom = NULL;
	}
	mMouseX = x;
	mMouseY = y;
	mTooltip[0] = 0;
	if ((0x165 < x) && (10 < y) && (x < 0x1a3) && (y < 0x38))
	{
		// the geiger counter: the name of the combo
		static const char* const aNames[] = {"No Combo!", "No Combo!", "Combo!", "Super Combo!", "Mega Combo!",
			"Ultra Combo!", "Hyper Combo!", "Giga Combo!"};
		unsigned int aCombo = (unsigned int) mGrid->mCombo;
		strcpy(mTooltip, aCombo < 8 ? aNames[aCombo] : "Max Combo!");
		return;
	}
	if ((0xd7 < x) && (0xf < y) && (x < 0x23c) && (y < 0x23) && (mApp->mMode == 0) && (mMeter.mLevel % 5 != 0))
		snprintf(mTooltip, sizeof(mTooltip), GTC(0x4b2fa4), (int) mMeter.mValue, (int) mMeter.mMax);
}

// 0x432000: a click on the board opens the help box of what is under it (port: not ported; the boxes only explain)
void Board::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	(void) x;
	(void) y;
	(void) theBtnNum;
	(void) theClickCount;
}

// 0x432210
void Board::AddProgress(int theAmount)
{
	mProgress = (float) theAmount + mProgress;
	if (48.0f < mProgress)
		mProgress = 48.0f;
}

// 0x4322c0: the bomb button: the bomb goes off when there is something to blast
void Board::CheckBomb()
{
	if (!mBombOn)
	{
		int aCount = mGrid->CountAtoms(true);
		if ((aCount != 0) && (mI174 < 1))
		{
			mAtomicus->Say(9);
			mBombOn = true;
			mI148 = 0x4b;
			if ((float) aCount <= mProgress)
				mF150 = (mProgress * 1.5f) / (float) aCount;
			else
				mF150 = 1.0f;
			mI14c = 2;
			mF150 = mF150 * 0.75f;
		}
	}
}

// 0x432360: a step of the bomb: it bursts atoms until its charge is used up
void Board::StepBomb()
{
	mI148--;
	if (mI148 < 0)
	{
		mB164 = true;
		mI148 = 10;
		int i = 0;
		if (0 < mI14c)
		{
			while (true)
			{
				if (1 < mGrid->CountAtoms(true))
				{
					PtrList<Atom> aCandidates;
					mGrid->mAtoms.Reset();
					Atom* anAtom = mGrid->mAtoms.Next();
					while (anAtom != NULL)
					{
						if (!anAtom->mPre && !anAtom->IsElement())
							aCandidates.Add(anAtom);
						anAtom = mGrid->mAtoms.Next();
					}
					if (0 < aCandidates.Size())
					{
						int aPick = mApp->mRand.Get(aCandidates.Size(), false);
						mGrid->Burst(aCandidates.Get(aPick));
					}
					aCandidates.Clear();
				}
				mProgress = mProgress - mF150;
				if (mProgress <= 0.0f)
					break;
				i++;
				if (mI14c <= i)
					return;
			}
			mProgress = 0;
			mBombOn = false;
			if (mGrid->CountAtoms(true) < 1)
				mGrid->AfterMove(false);
		}
	}
}

// 0x4324f0: a score floats up from a place and is added
void Board::FloatScore(int theX, int theY, int thePoints, const Color& theColor)
{
	int aPoints = thePoints;
	if (mApp->mExpertMode == 1)
		aPoints *= 2;
	char aText[20];
	snprintf(aText, sizeof(aText), "+%d", aPoints);
	FloatyText* aFloaty = new FloatyText(mApp, theX, theY, aText, mApp->mFloatyFont);
	aFloaty->mColor = Color(theColor.mRed, theColor.mGreen, theColor.mBlue, 255);
	aFloaty->mLife = 0x15e;
	aFloaty->mVY = -0.25f;
	mEffects.Add(aFloaty);
	mScore += aPoints;
}

// 0x4325c0: the water element: the field stays frozen for some moves
void Board::StartFreeze(int theMoves)
{
	mI188 = 0;
	mI184 = theMoves;
	mI18c = 0;
	if (mI180 < 1)
		mI180 = 0;
}

// 0x432610
void Board::StepFreeze()
{
	mI180++;
	if (mI180 == 0x80)
		mApp->mSndFreeze->Play();
	if (0x9b < mI180)
		mI180 = 0x9b;
	mI18c--;
	if (mI18c < 1)
		mI18c = 10;
}

// 0x432690
void Board::CountDown()
{
	mI184--;
	if (mI184 < 0)
		mI184 = 0;
	mI198--;
	if (mI198 < 0)
		mI198 = 0;
}

// 0x4326c0: the air element: for some moves the atoms are no obstacle
void Board::StartGhost(int theMoves)
{
	mI194 = 0xff;
	mI198 = theMoves;
}

// 0x4328a0
void Board::SetTint(int theR, int theG, int theB)
{
	if ((theR == 0xff) && (theG == 0xff) && (theB == 0xff))
	{
		mTintOn = false;
		return;
	}
	mTint.mGreen = theG;
	mTintOn = true;
	mTint.mRed = theR;
	mTint.mBlue = theB;
	mTint.mAlpha = 0xff;
	mTintF[0] = (float) theR * 0.003921569f;
	mTintF[1] = (float) theG * 0.003921569f;
	mTintF[2] = (float) theB * 0.003921569f;
}

// 0x432b80: the next level: the field's cells come back one by one
void Board::NextLevel()
{
	mMeter.mLevel++;
	mMeter.mMoleculesMade++;
	mI16c = 400;
	for (int aCol = 0; aCol < Grid::COLS; aCol++)
		for (int aRow = 0; aRow < Grid::ROWS; aRow++)
			mGrid->mCellState[aCol][aRow] = -100000;
}

// 0x432930
void Board::Save(FILE* f)
{
	fwrite(&mF8c, 4, 1, f);
	fwrite(&mBombPhase, 4, 1, f);
	fwrite(&mBombFrame, 4, 1, f);
	fwrite(&mProgress, 4, 1, f);
	fwrite(&mPulse, 4, 1, f);
	fwrite(&mPulseStep, 4, 1, f);
	fwrite(&mNumColorsStart, 4, 1, f);
	fwrite(&mMonitorSlide, 4, 1, f);
	fwrite(&mI16c, 4, 1, f);
	fwrite(&mI170, 4, 1, f);
	fwrite(&mI174, 4, 1, f);
	fwrite(&mGeigerGlow, 4, 1, f);
	fwrite(&mGeigerGlowStep, 4, 1, f);
	fwrite(&mI180, 4, 1, f);
	fwrite(&mI184, 4, 1, f);
	fwrite(&mI18c, 4, 1, f);
	fwrite(&mI190, 4, 1, f);
	fwrite(&mI194, 4, 1, f);
	fwrite(&mI198, 4, 1, f);
	fwrite(&mTint, 0x10, 1, f);
	unsigned char aTintOn = mTintOn ? 1 : 0;
	fwrite(&aTintOn, 1, 1, f);
	fwrite(mTintF, 4, 3, f);
	fwrite(&mScore, 4, 1, f);
	fwrite(&mI1c0, 4, 1, f);
	fwrite(&mStat1c4, 4, 7, f);
	mMeter.Save(f);
	mBonus->Save(f);
	mGrid->Save(f);
	fwrite(&mMeter.mMoleculesMade, 4, 1, f);
}

// 0x433ca0
void Board::Load(FILE* f)
{
	mAtomicus->mMuted = true;
	fread(&mF8c, 4, 1, f);
	fread(&mBombPhase, 4, 1, f);
	fread(&mBombFrame, 4, 1, f);
	fread(&mProgress, 4, 1, f);
	fread(&mPulse, 4, 1, f);
	fread(&mPulseStep, 4, 1, f);
	fread(&mNumColorsStart, 4, 1, f);
	fread(&mMonitorSlide, 4, 1, f);
	fread(&mI16c, 4, 1, f);
	fread(&mI170, 4, 1, f);
	fread(&mI174, 4, 1, f);
	fread(&mGeigerGlow, 4, 1, f);
	fread(&mGeigerGlowStep, 4, 1, f);
	fread(&mI180, 4, 1, f);
	fread(&mI184, 4, 1, f);
	fread(&mI18c, 4, 1, f);
	fread(&mI190, 4, 1, f);
	fread(&mI194, 4, 1, f);
	fread(&mI198, 4, 1, f);
	fread(&mTint, 0x10, 1, f);
	unsigned char aTintOn = 0;
	fread(&aTintOn, 1, 1, f);
	mTintOn = aTintOn != 0;
	fread(mTintF, 4, 3, f);
	fread(&mScore, 4, 1, f);
	fread(&mI1c0, 4, 1, f);
	fread(&mStat1c4, 4, 7, f);
	mMeter.Load(f);
	mBonus->Load(f);
	mGrid->Load(f);
	fread(&mMeter.mMoleculesMade, 4, 1, f);
	mAtomicus->mMuted = false;
}

// 0x433c40
void Board::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
		mApp->ShowQuit();
		break;
	case 1:
		CheckBomb();
		break;
	case 2:
		mApp->ShowOptions();
		break;
	case 3:
		mApp->ShowConfirm();
		break;
	}
}
