// The atoms of Atomica Deluxe (see Atom.h).

#include "Atom.h"
#include "Atomicus.h"
#include "ImageFont.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Swirl.h"
#include "Grid.h"
#include "GameText.h"
#include "Graphics.h"
#include "Font.h"
#include "PathFinder.h"
#include "Effects.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace Sexy;

// the 360 entries of the original's sine and minus cosine tables (app+0x3260 and +0x3800)
float Sexy::SinDeg(int theAngle)
{
	return (float) sin((double) theAngle * 3.14159265358979323846 / 180.0);
}

float Sexy::NegCosDeg(int theAngle)
{
	return (float) -cos((double) theAngle * 3.14159265358979323846 / 180.0);
}

// 0x421b00
int Sexy::CompassAngle(int theX1, int theY1, int theX2, int theY2)
{
	float aDX = (float) (theX2 - theX1);
	float aDY = (float) (theY2 - theY1);
	if (aDY == 0.0f)
		return (aDX <= 0.0f) ? 0x10e : 0x5a;
	float anAngle = (float) fabs(atan((double) (aDX / aDY)) * 180.0 / 3.14159265358979323846);
	if ((aDX >= 0.0f) && (aDY < 0.0f))
		return (int) anAngle;
	if ((aDX < 0.0f) && (aDY < 0.0f))
		return (int) (360.0f - anAngle);
	if ((aDX >= 0.0f) && (aDY > 0.0f))
		return (int) (180.0f - anAngle);
	return (int) (anAngle + 180.0f);
}

// the pictures the original reads from its tables by index: 0 is the picture itself, the others its shrunk copies
static Sprite* PreAtomFrame(AtomicaApp* theApp, int theFrame)
{
	return theFrame == 0 ? theApp->mPreAtom : theApp->mPreAtomShrink[theFrame - 1];
}

Sprite* Sexy::ChromiumFrame(AtomicaApp* theApp, int theFrame)
{
	return theFrame == 0 ? theApp->mChromium : theApp->mChromiumShrink[theFrame - 1];
}

// the symbols of the colour-blind option: the first twelve types are named after elements (game text 0x4ae38c...)
static const unsigned int gSymbolText[12] =
{
	0x4ae3b8, 0x4ae3b4, 0x4ae3b0, 0x4ae3ac, 0x4ae3a8, 0x4ae3a4, 0x4ae3a0, 0x4ae39c, 0x4ae398, 0x4ae394, 0x4ae390, 0x4ae38c
};

const std::string& Sexy::AtomSymbolText(int theType)
{
	return GT(gSymbolText[theType]);
}

// ---------------------------------------------------------------------------------------------------------
// Atom

// 0x4015d0
Atom::Atom(Grid* theGrid, int theType, bool theNoSpecial) :
	mPre(true),
	mPreStarted(false),
	mDead(false),
	mApp(theGrid->mApp),
	mGrid(theGrid),
	mCol(0),
	mRow(0),
	mX(0),
	mY(0),
	mDirX(0),
	mDirY(0),
	mAimDist(0),
	mScale(0.01f),
	mAimAngle(0),
	mPlaced(false),
	mFollowing(false),
	mPathNode(NULL),
	mNextNode(NULL),
	mSpeed(0),
	mColor(0xff, 0xff, 0xff, 0xff)
{
	mWobble = mApp->mRand.Get(0x80, false);
	mWobbleStep = 2;
	mType = theType;
	mSpawn = 24.0f;
	mFlyX = mFlySize = mFlyTurn = mFlyGrow = mFlyPhase = 0;
	mAlpha = 0xff;
	mFlag_ac = false;
	mHidden = false;
	mF_b0 = 0;
	mBounce = 0;
	mBounceStep = 1.0f;
	mBounceTick = 0;
	mKind = KIND_PLAIN;
	mKindArg = 0;
	mSpin = 0;
	mFade = 0;
	mGlow = 255.0f;
	mGlowStep = -2.5f;
	mNear = false;
	mNearAlpha = 0xff;
	mFlag_e0 = false;
	mE4 = mE8 = mEC = 0;

	bool aRing[5];		// which of the five rings of electrons the atom has (ring 0 is bVar2 of the original)
	aRing[0] = true;
	aRing[1] = true;
	aRing[2] = false;
	aRing[3] = false;
	aRing[4] = false;
	switch (theType)
	{
	case 0:
		mColor = Color(0xff, 0, 0, 0xff);
		aRing[0] = true; aRing[1] = false; aRing[2] = false; aRing[3] = false; aRing[4] = false;
		break;
	case 1:
		mColor = Color(0, 0xff, 0, 0xff);
		aRing[0] = true; aRing[1] = true; aRing[2] = false; aRing[3] = false; aRing[4] = false;
		break;
	case 2:
		mColor = Color(0xff, 0xff, 0, 0xff);
		aRing[0] = false; aRing[1] = true; aRing[2] = false; aRing[3] = false; aRing[4] = false;
		break;
	case 3:
		mColor = Color(0x96, 0x96, 0xff, 0xff);
		aRing[0] = false; aRing[1] = true; aRing[2] = true; aRing[3] = false; aRing[4] = false;
		break;
	case 4:
		mColor = Color(0xff, 0, 0xff, 0xff);
		aRing[0] = true; aRing[1] = true; aRing[2] = true; aRing[3] = false; aRing[4] = false;
		break;
	case 5:
		mColor = Color(0xfc, 0x86, 0, 0xff);
		aRing[0] = true; aRing[1] = false; aRing[2] = true; aRing[3] = false; aRing[4] = true;
		break;
	case 6:
		mColor = Color(0xff, 0xbd, 0xbd, 0xff);
		aRing[0] = false; aRing[1] = false; aRing[2] = false; aRing[3] = true; aRing[4] = true;
		break;
	case 7:
		mColor = Color(0xe4, 0xeb, 0x8e, 0x70);
		aRing[0] = true; aRing[1] = true; aRing[2] = true; aRing[3] = true; aRing[4] = true;
		break;
	case 8:
		mColor = Color(0x14, 0x14, 0x14, 0x14);
		aRing[0] = true; aRing[1] = false; aRing[2] = true; aRing[3] = true; aRing[4] = true;
		break;
	case 9:
		mColor = Color(0xa0, 0xa0, 0xf1, 0xcf);
		aRing[0] = true; aRing[1] = false; aRing[2] = true; aRing[3] = false; aRing[4] = true;
		break;
	case 10:
		mColor = Color(0xa0, 0xf1, 0xa0, 0xa7);
		aRing[0] = true; aRing[1] = false; aRing[2] = false; aRing[3] = false; aRing[4] = true;
		break;
	case 11:
		mColor = Color(0xc0, 0xc0, 0xc0, 0xc0);
		aRing[0] = true; aRing[1] = true; aRing[2] = true; aRing[3] = false; aRing[4] = true;
		break;
	default:
		mColor = Color(0xff, 0xff, 0xff, 0xff);
		aRing[0] = true; aRing[1] = true; aRing[2] = false; aRing[3] = false; aRing[4] = false;
		break;
	}
	if (!theNoSpecial)
	{
		int aRoll = mApp->mRand.Get(100000, false);
		if ((float) aRoll > mGrid->mChromiumChance * 1000.0f)
		{
			aRoll = mApp->mRand.Get(100000, false);
			if ((float) aRoll <= mGrid->mBiohazardChance * 1000.0f)
			{
				mKind = KIND_BIOHAZARD;
				mKindArg = 5;
			}
		}
		else
		{
			mColor = Color(0xff, 0xff, 0xff, 0xff);
			aRing[0] = aRing[1] = aRing[2] = aRing[3] = aRing[4] = true;
			mType = 0xb;
			mKind = KIND_CHROMIUM;
		}
	}

	mNear = false;
	mNearAlpha = 0xff;
	mElectrons.Clear();
	for (int aRingNum = 0; aRingNum < 5; aRingNum++)
	{
		if (!aRing[aRingNum])
			continue;
		Electron* anElectron = new Electron();
		anElectron->mAngle = (float) mApp->mRand.Get(0x20, false);
		anElectron->mRing = aRingNum;
		switch (mApp->mRand.Get(4, false))
		{
		case 0: anElectron->mSpeed = 0.1f; break;
		case 1: anElectron->mSpeed = 0.15f; break;
		case 2: anElectron->mSpeed = 0.2f; break;
		default: anElectron->mSpeed = 0.25f; break;
		}
		anElectron->mSpeed = anElectron->mSpeed + anElectron->mSpeed;
		mElectrons.Add(anElectron);
	}
}

// 0x401060
Atom::~Atom()
{
	for (int aCol = 0; aCol < Grid::COLS; aCol++)
		for (int aRow = 0; aRow < Grid::ROWS; aRow++)
			if (mGrid->mCells[aCol][aRow] == this)
				mGrid->mCells[aCol][aRow] = NULL;
	if (mGrid->mTooltipAtom == this)
		mGrid->mTooltipAtom = NULL;
	if (mKind == KIND_BIOHAZARD)
	{
		mGrid->mI364--;
		if (mGrid->mI364 < 0)
			mGrid->mI364 = 0;
	}
	mElectrons.Reset();
	Electron* anElectron = mElectrons.Next();
	while (anElectron != NULL)
	{
		delete anElectron;
		anElectron = mElectrons.Next();
	}
	mElectrons.Clear();
	mPath.Reset();
	int* aNode = mPath.Next();
	while (aNode != NULL)
	{
		delete[] aNode;
		aNode = mPath.Next();
	}
	mPath.Clear();
}

// 0x4011f0
void Atom::RemoveFromCell()
{
	if (mPlaced)
	{
		mGrid->mCells[mCol][mRow] = NULL;
		if (mGrid->mSelected == this)
			mGrid->mSelected = NULL;
		mPlaced = false;
	}
}

// 0x401230
void Atom::SetPath(PtrList<int>* thePath)
{
	mGrid->mCells[mCol][mRow] = NULL;
	if (mGrid->mSelected == this)
		mGrid->mSelected = NULL;
	mPlaced = false;
	mPath.Clear();
	for (size_t i = 0; i < thePath->mItems.size(); i++)
		mPath.mItems.push_back(thePath->mItems[i]);
	mPath.Reset();
	mFollowing = true;
	mNextNode = NULL;
	mPathNode = NULL;
	thePath->Reset();
	int* aNode = thePath->Next();
	while (aNode != NULL)
	{
		mPathNode = aNode;
		aNode = thePath->Next();
	}
	mSpeed = thePath->Size();
}

// 0x401c60: the shadow of an atom the held atom is aimed at: the picture leans toward the cursor
void Atom::DrawAimLine(Graphics* g)
{
	if (mPlaced && (mGrid->mSelected != this) && !mPre)
	{
		mAimAngle = CompassAngle(mX + 0x14, mY + 0x14, mGrid->mCursorX, mGrid->mCursorY);
		int aDY = mGrid->mCursorY - (mY + 0x14);
		int aDX = mGrid->mCursorX - (mX + 0x14);
		mAimDist = (float) sqrt((double) (aDY * aDY + aDX * aDX));
		mDirX = -SinDeg(mAimAngle);
		mDirY = -NegCosDeg(mAimAngle);
		g->SetColorizeImages(true);
		int aAlpha = (int) ((float) mGrid->mSelectGlow * 0.5714285714285714);
		g->SetColor(Color(0, 0, 0, aAlpha));
		float aLean = mAimDist * 0.06666667f * ((float) mGrid->mSelectGlow * 0.003921569f);
		mApp->mAtomA[mType]->DrawCenteredF(g, (float) mX + aLean * mDirX, (float) mY + aLean * mDirY);
		g->SetColorizeImages(false);
	}
}

// 0x401dc0
bool Atom::PlaceAt(int theCol, int theRow, bool theAddToList)
{
	Atom* anOther = mGrid->mCells[theCol][theRow];
	if (anOther != NULL)
	{
		if (!anOther->mPre)
		{
			mDead = true;
			return false;
		}
		if (anOther != this)
			anOther->mDead = true;
	}
	if (theAddToList)
		mGrid->mAtoms.Remove(this);
	mPlaced = true;
	mCol = theCol;
	mRow = theRow;
	if (theAddToList)
		mGrid->mAtoms.Add(this);
	mGrid->mCells[theCol][theRow] = this;
	int aPixel[2];
	Grid::CellToPixel(mCol, mRow, aPixel);
	mX = aPixel[0];
	mY = aPixel[1];
	return true;
}

// 0x401e60: into an empty cell chosen by chance
bool Atom::PlaceRandom()
{
	PtrList<int> aCells;
	for (int aCol = 0; aCol < Grid::COLS; aCol++)
	{
		for (int aRow = 0; aRow < Grid::ROWS; aRow++)
		{
			if (mGrid->mCells[aCol][aRow] == NULL)
			{
				int* aCell = new int[2];
				aCell[0] = aCol;
				aCell[1] = aRow;
				aCells.Add(aCell);
			}
		}
	}
	if (aCells.Size() != 0)
	{
		int aPick = mApp->mRand.Get(aCells.Size(), false);
		int* aCell = aCells.Get(aPick);
		PlaceAt(aCell[0], aCell[1], true);
		aCells.Reset();
		int* anItem = aCells.Next();
		while (anItem != NULL)
		{
			delete[] anItem;
			anItem = aCells.Next();
		}
		return true;
	}
	return false;
}

// 0x401f90
void Atom::Update()
{
	if ((mKind == KIND_BIOHAZARD) && !mPre)
	{
		mSpin += 0.5f;
		if (360.0f <= mSpin)
			mSpin -= 360.0f;
		mGlow = mGlow + mGlowStep;
		if (255.0f < mGlow)
		{
			mGlow = 255.0f;
			mGlowStep = -fabsf(mGlowStep);
		}
		if (mGlow < 64.0f)
		{
			mGlow = 64.0f;
			mGlowStep = fabsf(mGlowStep);
		}
		if (mGlow == 255.0f)
		{
			mApp->mSndHeartbeat->Play();
			mAlpha = 0x80;
		}
		mAlpha += 2;
		if (mAlpha > 0xff)
			mAlpha = 0xff;
		int anAngle = mApp->mRand.Get(0x168, false);
		Particle* aParticle = new Particle(mApp, mApp->mSpark);
		aParticle->mAlpha = 0x78;
		aParticle->mFade = 8;
		aParticle->mVX = SinDeg(anAngle);
		aParticle->mVY = NegCosDeg(anAngle);
		aParticle->mX = (float) (mGrid->mX + mX);
		aParticle->mY = (float) (mGrid->mY + mY);
		aParticle->mX = aParticle->mVX * 25.0f + aParticle->mX;
		aParticle->mY = aParticle->mVY * 25.0f + aParticle->mY;
		aParticle->mVX = aParticle->mVX * 0.4f;
		aParticle->mVY = aParticle->mVY * 0.4f;
		mApp->mBoard->mEffects.Add(aParticle);
	}

	Grid* aGrid = mGrid;
	if ((aGrid->mHoverCol == mCol) && (aGrid->mHoverRow == mRow))
	{
		mBounceTick++;
		if (mBounceTick > 3)
		{
			mBounce = mBounce + mBounceStep;
			if (5.0f < mBounce)
				mBounceStep = -1.0f;
			if (mBounce < 0.0f)
				mBounceStep = 1.0f;
			mBounceTick = 0;
		}
	}
	else
	{
		mBounce = mBounce - 1.0f;
		if (mBounce < 0.0f)
			mBounce = 0.0f;
	}

	if (mPre)
	{
		if ((aGrid->mCellState[mCol][mRow] != 0xff) && mHidden)
			return;
		if (!mPreStarted)
		{
			if (0.0f < mSpawn)
				mSpawn = mSpawn - 0.5f;
			if ((mSpawn == 0.0f) && (mFlag_ac || aGrid->mNoMoveLeft))
				mPreStarted = true;
		}
		else if (mSpawn < 24.0f)
		{
			if (mSpawn == 0.0f)
				aGrid->mB334 = true;
			mSpawn = mSpawn + 0.5f;
			if ((24.0f <= mSpawn) && (mApp->mMode == 1) && (aGrid->mAtoms.Size() > 0))
			{
				for (int i = 0; i < aGrid->mAtoms.Size(); i++)
				{
					Atom* anOther = aGrid->mAtoms.Get(i);
					if (!anOther->mPre && anOther->mFollowing &&
						((anOther->mNextNode == NULL) || ((anOther->mNextNode[0] == mCol) && (anOther->mNextNode[1] == mRow))))
						mSpawn = mSpawn - 1.0f;
				}
			}
			if (24.0f <= mSpawn)
			{
				mPre = false;
				aGrid->mB32c = true;
				if ((mKind == KIND_BIOHAZARD) && (++aGrid->mI364 == 1))
				{
					int aMessage = mApp->mRand.Get(2, false);
					mApp->mBoard->mAtomicus->Say(aMessage + 1);
				}
			}
		}
		int aStep = mWobbleStep;
		mWobble += aStep;
		if (mWobble > 0x7f)
		{
			mWobble = 0x80;
			mWobbleStep = -abs(mWobbleStep);
		}
		if (mWobble < 1)
		{
			mWobble = 0;
			mWobbleStep = abs(mWobbleStep);
		}
	}
	else
	{
		mFade += 3;
		if (mFade > 0xff)
			mFade = 0xff;
	}

	mElectrons.Reset();
	Electron* anElectron = mElectrons.Next();
	while (anElectron != NULL)
	{
		anElectron->mAngle += anElectron->mSpeed;
		if (32.0f <= anElectron->mAngle)
			anElectron->mAngle -= 32.0f;
		anElectron = mElectrons.Next();
	}

	if (aGrid->mSelected == this)
	{
		mScale = mScale + 0.035f;
		if (1.5f < mScale)
			mScale = 1.5f;
	}
	else
	{
		mScale = mScale * 0.99f;
		if (mScale < 0.25f)
			mScale = 0.25f;
	}

	if (mFollowing)
	{
		if (mNextNode == NULL)
		{
			mNextNode = mPath.Next();
			if (mNextNode == NULL)
			{
				mFollowing = false;
				PlaceAt(mCol, mRow, true);
				aGrid->EndRound();
				aGrid->AfterMove(false);
				Particle* aStar = new Particle(mApp, mApp->mStarBurst);
				aStar->mAlpha = 0x96;
				aStar->mFade = 10;
				aStar->mX = (float) (aGrid->mX + mX);
				aStar->mY = (float) (aGrid->mY + mY);
				mApp->mBoard->mEffects.Add(aStar);
			}
		}
		for (int i = 0; i < mSpeed * 3; i++)
		{
			int* aNode = mNextNode;
			if (aNode != NULL)
			{
				int aTarget[2] = {0, 0};
				Grid::CellToPixel(aNode[0], aNode[1], aTarget);
				int anOldX = mX;
				if (aTarget[0] < anOldX)
					mX = anOldX - 1;
				bool aWasLeft = anOldX <= aTarget[0];
				int aNewX = mX;
				bool aStillLeft = aNewX < aTarget[0];
				if (aStillLeft)
					mX = aNewX + 1;
				int anOldY = mY;
				if (aTarget[1] < anOldY)
					mY = anOldY - 1;
				bool aWasAbove = anOldY <= aTarget[1];
				int aNewY = mY;
				bool aStillAbove = aNewY < aTarget[1];
				if (aStillAbove)
					mY = aNewY + 1;
				if (i % 0x14 == 0)
				{
					Particle* aTrail = new Particle(mApp, mApp->mAtomC[mType]);
					aTrail->mAlpha = 100;
					aTrail->mFade = 0x14;
					aTrail->mX = (float) (mGrid->mX + mX);
					aTrail->mY = (float) (mGrid->mY + mY);
					mApp->mBoard->mEffects.Add(aTrail);
				}
				if ((aGrid->mSelected == NULL) || (aGrid->mSelected == this))
				{
					aGrid->mCursorX = mX;
					aGrid->mCursorY = mY;
				}
				if (!aStillAbove && aWasAbove && !aStillLeft && aWasLeft)
				{
					mCol = mNextNode[0];
					mRow = mNextNode[1];
					mNextNode = NULL;
					mPathNode = NULL;
				}
			}
		}
	}

	mNear = false;
	if (mApp->mBoard->mI198 > 0)
		mNear = aGrid->mPathFinder->Contains(mCol, mRow);
	if (mNear)
	{
		mNearAlpha -= 2;
		if (mNearAlpha > 0x7e)
			return;
		mNearAlpha = 0x7f;
	}
	else
	{
		mNearAlpha += 2;
		if (mNearAlpha < 0x100)
			return;
		mNearAlpha = 0xff;
	}
}

// 0x4027a0: the atom leaves the field and flies away: a copy of the newest swirl is made for it and it circles in that
void Atom::FlyAway(Swirl* theTarget)
{
	if (theTarget == NULL)
	{
		// port: the board has no swirl yet (the original does not look): the atom just goes
		mGrid->mCellState[mCol][mRow] = -50;
		RemoveFromCell();
		mDead = true;
		return;
	}
	Swirl* aCopy = theTarget->Clone();
	aCopy->mClone = true;
	mApp->mBoard->mSwirls.Add(aCopy);
	mGrid->mCellState[mCol][mRow] = -50;
	RemoveFromCell();
	mX += mGrid->mX;
	mY += mGrid->mY;
	aCopy->Aim(mX, mY, &mFlyX, &mFlySize);
	mFlyGrow = (float) mApp->mRand.Get(0x14, false) * 0.01f;
	mF_b0 = (float) mApp->mRand.Get(0x32, false) * 0.01f + 1.5f;
	mFlyTurn = (float) mApp->mRand.Get(0x14, false) * 0.01f + 0.7f;
	mGrid->mAtoms.Remove(this);
	aCopy->mAtoms.Add(this);
}

// 0x4028c0: what an atom shows over its picture, by kind: the chromium atom the colours of its neighbours, the
// biohazard its ring and countdown
void Atom::DrawOverlay(Graphics* g, int theFrame, int theX, int theY)
{
	if (mKind == KIND_PLAIN)
		return;
	if (mKind == KIND_CHROMIUM)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, 0x96));
		ChromiumFrame(mApp, theFrame)->DrawCentered(g, theX, theY);
		if (mPlaced && theFrame == 0)
		{
			Atom* aNeighbor;
			if ((mCol > 0) && ((aNeighbor = mGrid->mCells[mCol - 1][mRow]) != NULL) && !aNeighbor->mPre)
			{
				g->SetColor(Color(aNeighbor->mColor.mRed, aNeighbor->mColor.mGreen, aNeighbor->mColor.mBlue, 0xe1));
				mApp->mAdjacent[3]->DrawCentered(g, theX, theY);
			}
			if ((mCol < 8) && ((aNeighbor = mGrid->mCells[mCol + 1][mRow]) != NULL) && !aNeighbor->mPre)
			{
				g->SetColor(Color(aNeighbor->mColor.mRed, aNeighbor->mColor.mGreen, aNeighbor->mColor.mBlue, 0xe1));
				mApp->mAdjacent[1]->DrawCentered(g, theX, theY);
			}
			if ((mRow > 0) && ((aNeighbor = mGrid->mCells[mCol][mRow - 1]) != NULL) && !aNeighbor->mPre)
			{
				g->SetColor(Color(aNeighbor->mColor.mRed, aNeighbor->mColor.mGreen, aNeighbor->mColor.mBlue, 0xe1));
				mApp->mAdjacent[0]->DrawCentered(g, theX, theY);
			}
			if ((mRow < 7) && ((aNeighbor = mGrid->mCells[mCol][mRow + 1]) != NULL) && !aNeighbor->mPre)
			{
				g->SetColor(Color(aNeighbor->mColor.mRed, aNeighbor->mColor.mGreen, aNeighbor->mColor.mBlue, 0xe1));
				mApp->mAdjacent[2]->DrawCentered(g, theX, theY);
			}
		}
	}
	else if (mKind == KIND_BIOHAZARD)
	{
		if (mPre)
			return;
		g->SetColorizeImages(true);
		int aLevel = (int) mGlow;
		if (aLevel > 0xff)
			aLevel = 0xff;
		if (mFade < aLevel)
			aLevel = mFade;
		g->SetColor(Color(mColor.mRed, mColor.mGreen, mColor.mBlue, aLevel));
		mApp->mBiohazard[6 - mKindArg]->DrawCentered(g, theX, theY);
		g->SetColor(Color(mColor.mRed, mColor.mGreen, mColor.mBlue, (int) mGlow > mFade ? mFade : (int) mGlow));
		Sprite* aRing = (mAlpha < 0xaa) ? mApp->mBadRing[1] : mApp->mBadRing[0];
		aRing->DrawCentered(g, theX, theY);
		int aCountdownAlpha = (int) mGlow;
		if (mFade < aCountdownAlpha)
			aCountdownAlpha = mFade;
		g->SetColor(Color(mColor.mRed, mColor.mGreen, mColor.mBlue, aCountdownAlpha));
		if (mApp->mShowSymbols)
			theX -= 12;
		mApp->mBioCountdown[mKindArg - 1]->DrawCentered(g, theX, theY);
	}
	g->SetColorizeImages(false);
}

// 0x402c60: a biohazard that has counted down spreads to a free cell beside it
void Atom::Infect()
{
	if (!mPre && (mKind == KIND_BIOHAZARD) && mPlaced && (--mKindArg < 1))
	{
		PtrList<int> aFree;
		static const int aDeltas[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
		for (int i = 0; i < 8; i++)
		{
			int aCol = mCol + aDeltas[i][0];
			int aRow = mRow + aDeltas[i][1];
			if ((aCol >= 0) && (aCol <= 8) && (aRow >= 0) && (aRow <= 7) && (mGrid->mCells[aCol][aRow] == NULL))
			{
				int* aCell = new int[2];
				aCell[0] = aCol;
				aCell[1] = aRow;
				aFree.Add(aCell);
			}
		}
		if (aFree.Size() > 0)
		{
			int* aCell = aFree.Get(mApp->mRand.Get(aFree.Size(), false));
			Atom* aSpawn = new Atom(mGrid, mApp->mRand.Get(mGrid->mNumColors, false), true);
			aSpawn->mKind = KIND_BIOHAZARD;
			aSpawn->mKindArg = 5;
			aSpawn->PlaceAt(aCell[0], aCell[1], true);
			aSpawn->mPreStarted = true;
			aSpawn->mSpawn = 0.0f;
			mApp->mBoard->mEffects.Add(new InfectBolt(mApp, mX + mGrid->mX, mY + mGrid->mY, aSpawn->mX + mGrid->mX,
				aSpawn->mY + mGrid->mY));
		}
		aFree.Reset();
		int* anItem = aFree.Next();
		while (anItem != NULL)
		{
			delete[] anItem;
			anItem = aFree.Next();
		}
		mKindArg = 5;
	}
}

// 0x4037f0
void Atom::Draw(Graphics* g)
{
	if (mPlaced && mHidden && (mGrid->mCellState[mCol][mRow] < 0x96))
		return;

	if (mPre)
	{
		if (mApp->mMode == 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(mColor);
			mColor.mAlpha = 0x80;
			g->SetColor(mColor);
			PreAtomFrame(mApp, (int) mSpawn)->DrawCentered(g, mX, mY);
			mColor.mAlpha = mWobble;
			g->SetColor(mColor);
			PreAtomFrame(mApp, (int) mSpawn)->DrawCentered(g, mX, mY);
			if (mPreStarted)
			{
				mColor.mAlpha = 0xff;
				g->SetColor(mColor);
				g->SetColorizeImages(false);
				int aFrame = (int) (24.0f - mSpawn);
				mApp->mAtomShrink[mType][aFrame]->DrawCentered(g, mX, mY);
				DrawOverlay(g, (int) (24.0f - mSpawn), mX, mY);
			}
		}
		else
		{
			g->SetColorizeImages(true);
			g->SetColor(mColor);
			mColor.mAlpha = 0x80;
			g->SetColor(mColor);
			if (!mPreStarted)
			{
				PreAtomFrame(mApp, (int) mSpawn)->DrawCentered(g, mX, mY);
				mColor.mAlpha = mWobble;
				g->SetColor(mColor);
				PreAtomFrame(mApp, (int) mSpawn)->DrawCentered(g, mX, mY);
			}
			else
			{
				mColor.mAlpha = 0x80;
				g->SetColor(mColor);
				PreAtomFrame(mApp, (int) mSpawn)->DrawCentered(g, mX, mY);
				mColor.mAlpha = mWobble;
				g->SetColor(mColor);
				PreAtomFrame(mApp, (int) mSpawn)->DrawCentered(g, mX, mY);
				mColor.mAlpha = 0xff;
				g->SetColor(mColor);
				g->SetColorizeImages(false);
				mApp->mAtomShrink[mType][(int) (24.0f - mSpawn)]->DrawCentered(g, mX, mY);
				DrawOverlay(g, (int) (24.0f - mSpawn), mX, mY);
			}
		}
		g->SetColorizeImages(false);
	}
	else
	{
		g->SetColorizeImages(false);
		if (mNearAlpha != 0xff)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, mNearAlpha));
		}
		Grid* aGrid = mGrid;
		if ((aGrid->mSelected == this) || !mPlaced)
		{
			mApp->mAtomC[mType]->DrawCentered(g, mX, mY);
			DrawOverlay(g, 0, mX, mY);
		}
		else
		{
			int aGlow = aGrid->mSelectGlow;
			if (aGlow == 0)
			{
				if (aGrid->mLevelOver || aGrid->mUnsolvable)
				{
					int aShake = aGrid->mOverTick / 0x19 + aGrid->mBonusTick / 0x19;
					if (aShake < 6)
					{
						if (aShake < 1)
							aShake = 1;
					}
					else
						aShake = 5;
					mWobble = mApp->mRand.Get(5, false);
					int aDY = mApp->mRand.Get(aShake, true) + mY;
					int aDX = mApp->mRand.Get(aShake, true);
					mApp->mAtomShrink[mType][mWobble]->DrawCentered(g, aDX + mX, aDY);
				}
				else
				{
					int aFrame = (int) mBounce;
					if (aFrame < 1)
						aFrame = 0;
					mApp->mAtomShrink[mType][aFrame]->DrawCentered(g, mX, mY);
					DrawOverlay(g, 0, mX, mY);
				}
			}
			else if (aGlow < 0xf0)
			{
				int aSpin = mAimAngle / 0x12;
				if ((aGrid->mLevelOver || aGrid->mUnsolvable))
				{
					int aShake = aGrid->mOverTick / 0x19 + aGrid->mBonusTick / 0x19;
					if (aShake < 6)
					{
						if (aShake < 1)
							aShake = 1;
					}
					else
						aShake = 5;
					int aDY = mApp->mRand.Get(aShake, true) + mY;
					int aDX = mApp->mRand.Get(aShake, true) + mX;
					mWobble = mApp->mRand.Get(5, false);
					if (mType < AtomicaApp::NUM_SPIN_TYPES)
						mApp->mAtomMask[mType][19 - aSpin]->DrawCentered(g, aDX, aDY);
					else
						mApp->mAtomShrink[mType][mWobble]->DrawCentered(g, aDX, aDY);
					Color aSaved = mColor;
					g->SetColorizeImages(true);
					g->SetColor(Color(0xff, 0xff, 0xff, aGrid->mSelectGlow));
					if (mType < AtomicaApp::NUM_SPIN_TYPES)
						mApp->mAtomMask[mType][19 - aSpin]->DrawCentered(g, aDX, aDY);
					DrawOverlay(g, 0, aDX, aDY);
					mColor = aSaved;
				}
				else
				{
					if (aGlow < 0x4c)
					{
						g->SetColorizeImages(false);
						if (mType < AtomicaApp::NUM_SPIN_TYPES)
							mApp->mAtomSpin[mType][19 - aSpin]->DrawCentered(g, mX, mY);
						else
							mApp->mAtomShrink[mType][0]->DrawCentered(g, mX, mY);
						DrawOverlay(g, 0, mX, mY);
					}
					else
					{
						mApp->mAtomShrink[mType][0]->DrawCentered(g, mX, mY);
						g->SetColorizeImages(true);
						g->SetColor(Color(0xff, 0xff, 0xff, aGlow));
						if (mType < AtomicaApp::NUM_SPIN_TYPES)
							mApp->mAtomMask[mType][19 - aSpin]->DrawCentered(g, mX, mY);
						DrawOverlay(g, 0, mX, mY);
					}
					if (aGlow != 0xff)
						g->SetColorizeImages(false);
				}
			}
			else
			{
				int aSpin = mAimAngle / 0x12;
				if (mType < AtomicaApp::NUM_SPIN_TYPES)
					mApp->mAtomMask[mType][19 - aSpin]->DrawCentered(g, mX, mY);
				else
					mApp->mAtomShrink[mType][0]->DrawCentered(g, mX, mY);
			}
		}
		if (aGrid->mSelected == this)
		{
			g->SetColorizeImages(true);
			mElectrons.Reset();
			Electron* anElectron = mElectrons.Next();
			while (anElectron != NULL)
			{
				g->SetColor(Color(0, 0, 0, 0x40));
				mApp->mBonusLevels[anElectron->mRing * 0x20 + (int) anElectron->mAngle]->DrawCentered(g, mX + 1, mY + 1);
				g->SetColor(mColor);
				mApp->mBonusLevels[anElectron->mRing * 0x20 + (int) anElectron->mAngle]->DrawCentered(g, mX, mY);
				g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
				mApp->mBonusNucleus[anElectron->mRing * 0x20 + (int) anElectron->mAngle]->DrawCentered(g, mX, mY);
				anElectron = mElectrons.Next();
			}
			g->SetColorizeImages(false);
		}
	}
	g->SetColorizeImages(false);
	if (mApp->mShowSymbols && (mType < 0xc))
	{
		g->SetFont(mApp->mMonitorFontMedium);
		g->SetColor(Color(0, 0, 0, 0xff));
		const std::string& aSymbol = GT(gSymbolText[mType]);
		g->DrawString(aSymbol, mX - mApp->mMonitorFontMedium->StringWidth(aSymbol) / 2 + 2, mY + 7);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
		g->DrawString(aSymbol, mX - mApp->mMonitorFontMedium->StringWidth(aSymbol) / 2, mY + 5);
	}
}

// 0x4012c0 / 0x403100: the fields of a saved game
void Atom::Save(FILE* f)
{
	mDead = false;
	fwrite(&mPre, 1, 1, f);
	fwrite(&mPreStarted, 1, 1, f);
	fwrite(&mDead, 1, 1, f);
	fwrite(&mCol, 4, 2, f);
	fwrite(&mX, 4, 2, f);
	fwrite(&mPlaced, 1, 1, f);
	fwrite(&mFollowing, 1, 1, f);
	fwrite(&mColor, 4, 4, f);
	fwrite(&mWobble, 4, 1, f);
	fwrite(&mWobbleStep, 4, 1, f);
	fwrite(&mType, 4, 1, f);
	fwrite(&mSpawn, 4, 1, f);
	fwrite(&mAlpha, 4, 1, f);
	fwrite(&mFlag_ac, 1, 1, f);
	fwrite(&mHidden, 1, 1, f);
	fwrite(&mKind, 4, 1, f);
	fwrite(&mKindArg, 4, 1, f);
	fwrite(&mSpin, 4, 1, f);
	fwrite(&mFade, 4, 1, f);
	fwrite(&mGlow, 4, 1, f);
	fwrite(&mGlowStep, 4, 1, f);
	fwrite(&mNear, 1, 1, f);
	fwrite(&mNearAlpha, 4, 1, f);
	fwrite(&mFlag_e0, 1, 1, f);
	fwrite(&mE4, 4, 1, f);
	fwrite(&mE8, 4, 2, f);
}

void Atom::Load(FILE* f)
{
	fread(&mPre, 1, 1, f);
	fread(&mPreStarted, 1, 1, f);
	fread(&mDead, 1, 1, f);
	fread(&mCol, 4, 2, f);
	fread(&mX, 4, 2, f);
	fread(&mPlaced, 1, 1, f);
	fread(&mFollowing, 1, 1, f);
	fread(&mColor, 4, 4, f);
	fread(&mWobble, 4, 1, f);
	fread(&mWobbleStep, 4, 1, f);
	fread(&mType, 4, 1, f);
	fread(&mSpawn, 4, 1, f);
	fread(&mAlpha, 4, 1, f);
	fread(&mFlag_ac, 1, 1, f);
	fread(&mHidden, 1, 1, f);
	fread(&mKind, 4, 1, f);
	fread(&mKindArg, 4, 1, f);
	fread(&mSpin, 4, 1, f);
	fread(&mFade, 4, 1, f);
	fread(&mGlow, 4, 1, f);
	fread(&mGlowStep, 4, 1, f);
	fread(&mNear, 1, 1, f);
	fread(&mNearAlpha, 4, 1, f);
	fread(&mFlag_e0, 1, 1, f);
	fread(&mE4, 4, 1, f);
	fread(&mE8, 4, 2, f);
	mDead = false;
	if (mPlaced)
		PlaceAt(mCol, mRow, true);
}
