// The field of Atomica Deluxe, part 2: the molecules, the move that follows each of the player's, and the magnet
// (see Grid.h).

#include "Grid.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Atomicus.h"
#include "BonusMolecules.h"
#include "Effects.h"
#include "GameText.h"
#include "Graphics.h"

using namespace Sexy;

extern bool gGridKeepTooltip;

// 0x412280: every block of one type that is two or three atoms wide and two or three high becomes a molecule: the
// atoms are taken (Collect), the combo grows (+2 for a block of six, +1 for one of four), and the meter and the
// score float up. Returns whether a molecule was made.
bool Grid::FindMolecules()
{
	gGridKeepTooltip = true;
	bool aMade = false;
	MouseMove(mM31c, mM320);
	gGridKeepTooltip = false;
	mApp->mBoard->mBonus->Check(this);

	int aCol = 0;
	int aNextCol;
	do
	{
		aNextCol = aCol + 1;
		int aRow = 0;
		int aNextRow;
		do
		{
			// three wide, two high
			if ((aCol < 7) && (TypesInRect(aCol, aRow, aCol + 2, aRow + 1) == 1))
			{
				mCombo += 2;
				mApp->mSndCreateMolecule->Play();
				mB335 = true;
				int aCount = Collect(aCol, aRow, aCol + 2, aRow + 1);
				mStat37c += aCount;
				if (!mRadiationRound || mB525)
				{
					if (!mB4c0)
						aCount = 2;
					mApp->mBoard->mMeter.Add(aCount, mCombo);
				}
				mStat380++;
				mGroupX += mX;
				mGroupY += mY;
				aMade = true;
				int aCombo = mCombo - 1;
				mApp->mBoard->FloatScore(mGroupX, mGroupY, ((aCombo < 1 ? 0 : aCombo) * 5 + 5) * 0x10,
					Color(mGroupColor.mRed, mGroupColor.mGreen, mGroupColor.mBlue, 255));
			}
			// two wide, three high
			if ((aRow < 6) && (TypesInRect(aCol, aRow, aNextCol, aRow + 2) == 1))
			{
				mCombo += 2;
				mApp->mSndCreateMolecule->Play();
				mB335 = true;
				int aCount = Collect(aCol, aRow, aNextCol, aRow + 2);
				mStat37c += aCount;
				if (!mRadiationRound || mB525)
				{
					if (!mB4c0)
						aCount = 2;
					mApp->mBoard->mMeter.Add(aCount, mCombo);
				}
				mStat380++;
				mGroupX += mX;
				mGroupY += mY;
				aMade = true;
				int aCombo = mCombo - 1;
				mApp->mBoard->FloatScore(mGroupX, mGroupY, ((aCombo < 1 ? 0 : aCombo) * 5 + 5) * 0x10,
					Color(mGroupColor.mRed, mGroupColor.mGreen, mGroupColor.mBlue, 255));
			}
			// two by two
			aNextRow = aRow + 1;
			if (TypesInRect(aCol, aRow, aNextCol, aNextRow) == 1)
			{
				mCombo += 1;
				mApp->mSndCreateMolecule->Play();
				mB335 = true;
				int aCount = Collect(aCol, aRow, aNextCol, aNextRow);
				mStat37c += aCount;
				if (!mRadiationRound || mB525)
				{
					if (!mB4c0)
						aCount = 1;
					mApp->mBoard->mMeter.Add(aCount, mCombo);
				}
				mGroupX += mX;
				mStat380++;
				mGroupY += mY;
				aMade = true;
				int aCombo = mCombo - 1;
				mApp->mBoard->FloatScore(mGroupX, mGroupY, ((aCombo < 1 ? 0 : aCombo) * 5 + 5) * 0x10,
					Color(mGroupColor.mRed, mGroupColor.mGreen, mGroupColor.mBlue, 255));
			}
			aRow = aNextRow;
		} while (aNextRow < 7);
		aCol = aNextCol;
	} while (aNextCol < 8);

	if (!aMade)
		mB358 = true;
	else
	{
		mB358 = false;
		mF390 = mF390 + 100.0f;
	}
	if (mApp->mMode == 1)
		mB358 = false;
	if (mStat384 < mCombo)
		mStat384 = mCombo;
	if (aMade)
	{
		if (1 < mCombo)
			mApp->mBoard->AddProgress(mCombo - 1);
		mApp->mBoard->mAtomicus->Comment(mCombo);
		if (7 < mCombo)
			mCombo = 7;
		mApp->mBoard->mF0ac = (float) (mCombo * 0xc);
		mApp->mBoard->mBonus->Upgrade();
	}
	if (mApp->mBoard->mMeter.mFull)
		mStat378 = mStat374 / 100;
	return aMade;
}

// 0x4127c0: what follows each move of the player (and, in the action mode, each tick of the clock): the molecules
// are looked for, and if there are none the biohazard atoms spread and a round of new atoms appears
void Grid::AfterMove(bool theTimerTick)
{
	if (mApp->mMode == 0)
		mApp->mBoard->CountDown();
	bool aMade = FindMolecules();
	if (aMade)
	{
		if ((mApp->mMode == 0) && (mApp->mBoard->mMeter.mLevel == 1) && (mRounds < 2))
			return;
		if (mApp->mMode == 1)
			return;
		if (mApp->mBoard->mMeter.mLevel % 5 == 0)
			return;
		if (CountAtoms(true) != 0)
			return;
		// a field cleared of its atoms: the next round forms at once
		mNoMoveLeft = false;
		mAtoms.Reset();
		Atom* anAtom = mAtoms.Next();
		while (anAtom != NULL)
		{
			if (anAtom->mPre)
			{
				anAtom->mPreStarted = true;
				anAtom->mSpawn = 0;
				anAtom->Infect();
			}
			anAtom = mAtoms.Next();
		}
		return;
	}

	if ((mApp->mMode == 1) && !theTimerTick)
	{
		mCombo = 0;
		mApp->mBoard->mF0ac = 0;
	}
	bool aNewRound = true;
	bool aSpread = true;
	if (0 < mApp->mBoard->mI184)
	{
		aNewRound = false;
		aSpread = false;
		if (CountAtoms(true) == 0)
			aNewRound = true;
	}
	if ((mApp->mMode == 0) && (mApp->mBoard->mMeter.mLevel == 1) && (mRounds < 2))
		aNewRound = false;
	if ((mApp->mMode == 1) && !theTimerTick)
		return;
	if (aNewRound)
	{
		// the pre-atoms of the last round settle
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
		// and the next round's appear
		int aPlaced = 0;
		int aCount = mAtomsPerRound;
		int i = 0;
		bool aOK = false;
		if (0 < aCount)
		{
			do
			{
				Atom* aNewAtom = new Atom(this, mApp->mRand.Get(mNumColors, false), false);
				if (!aNewAtom->PlaceRandom())
					aNewAtom->mDead = true;
				else
					aPlaced++;
				aCount = mAtomsPerRound;
				i++;
			} while (i < aCount);
			if (1 < aPlaced)
			{
				mNoMoveLeft = false;
				aOK = true;
			}
		}
		if (!aOK && !mNoMoveLeft)
			mNoMoveLeft = true;
	}
	if (((mApp->mMode != 1) || theTimerTick) && aSpread)
	{
		mAtoms.Reset();
		Atom* anAtom = mAtoms.Next();
		while (anAtom != NULL)
		{
			anAtom->Infect();
			anAtom = mAtoms.Next();
		}
	}
}

// 0x412a80: the atoms slide one cell along the magnet's arrow, as far as they can; when any moved, the molecules are
// looked for again
void Grid::Magnetise()
{
	PtrList<Atom> aMoved;
	bool aAnyMoved = false;
	bool aPassMoved;
	do
	{
		aPassMoved = false;
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			for (int aRow = 0; aRow < ROWS; aRow++)
			{
				Atom* anAtom = mCells[aCol][aRow];
				if ((anAtom != NULL) && !anAtom->mPre && mAtoms.Contains(anAtom) && !aMoved.Contains(anAtom))
				{
					int aNewCol = mArrowX + aCol;
					int aNewRow = mArrowY + aRow;
					if ((aNewCol > -1) && (aNewRow > -1) && (aNewCol < 9) && (aNewRow < 8) && (mCells[aNewCol][aNewRow] == NULL))
					{
						anAtom->RemoveFromCell();
						anAtom->PlaceAt(aNewCol, aNewRow, true);
						int i = 0;
						int anAlpha = 0xff;
						do
						{
							Particle* aTrail = new Particle(mApp, mApp->mAtomC[anAtom->mType]);
							aTrail->mAlpha = anAlpha;
							aTrail->mFade = 0x14;
							aTrail->mX = (float) (anAtom->mX + mArrowX * i * -10 + mX);
							aTrail->mY = (float) (anAtom->mY + mArrowY * i * -10 + mY);
							mApp->mBoard->mEffects.Add(aTrail);
							anAlpha -= 0x28;
							i++;
						} while (0x37 < anAlpha);
						aMoved.Add(anAtom);
						aPassMoved = true;
						aAnyMoved = true;
					}
				}
			}
		}
	} while (aPassMoved);
	if (aAnyMoved)
	{
		mApp->mSndMagnet->Play();
		bool aOldFlag = mB358;
		FindMolecules();
		if (mB358)
			mB358 = aOldFlag;
	}
}

// 0x40ef10: the atoms of a block leave their cells and form a molecule (a new group, which shakes and then flies
// away); the sparks run around the cells. Leaves the molecule's middle and average colour in the grid and returns
// the number of atoms.
int Grid::Collect(int theCol1, int theRow1, int theCol2, int theRow2)
{
	int aN = 0;
	mB525 = false;
	bool aAllRadiated = true;
	mGroupX = 0;
	mGroupY = 0;
	mGroupColor.mRed = 0;
	mGroupColor.mGreen = 0;
	mGroupColor.mBlue = 0;
	int aCount = 0;
	PtrList<Atom>* aGroup = new PtrList<Atom>();
	for (int aRow = theRow1; aRow <= theRow2; aRow++)
	{
		if (aRow > -1)
		{
			if (aRow > 7)
				break;
			for (int aCol = theCol1; aCol <= theCol2; aCol++)
			{
				if (aCol > -1)
				{
					if (aCol > 8)
						break;
					Atom* anAtom = mCells[aCol][aRow];
					if (anAtom != NULL)
					{
						if (!mRadiation[aCol][aRow])
							aAllRadiated = false;
						int aPass = 2;
						do
						{
							for (int aCorner = 0; aCorner < 4; aCorner++)
							{
								Sparkle* aSparkle = new Sparkle(mApp, aCol, aRow, aCorner);
								mSparkles.Add(aSparkle);
								aSparkle->mAlpha = 0xff;
								aSparkle->mFadeStep = 3;
							}
							aPass--;
						} while (aPass != 0);
						mGroupColor.mRed += anAtom->mColor.mRed;
						mGroupColor.mGreen += anAtom->mColor.mGreen;
						mGroupColor.mBlue += anAtom->mColor.mBlue;
						anAtom->RemoveFromCell();
						mAtoms.Remove(anAtom);
						aGroup->Add(anAtom);
						anAtom->mE4 = 0x32;
						aCount++;
						int aPixel[2] = {0, 0};
						CellToPixel(aCol, aRow, aPixel);
						aN++;
						mGroupX += aPixel[0];
						mGroupY += aPixel[1];
					}
				}
			}
		}
	}
	mGroups.Add(aGroup);
	if (0 < aN)
	{
		mGroupX /= aN;
		mGroupY /= aN;
		mGroupColor.mRed /= aN;
		mGroupColor.mGreen /= aN;
		mB525 = aAllRadiated;
		mGroupColor.mBlue /= aN;
	}
	return aCount;
}

// 0x40ea60: a biohazard atom the molecule it was in has destroyed: it shatters
void Grid::Obliterate(Atom* theAtom)
{
	theAtom->RemoveFromCell();
	mAtoms.Remove(theAtom);
	int aPixel[2] = {0, 0};
	CellToPixel(theAtom->mCol, theAtom->mRow, aPixel);
	aPixel[0] += mX;
	aPixel[1] += mY;
	NukeFlash* aFlash = new NukeFlash(mApp);
	aFlash->mX = (float) aPixel[0];
	aFlash->mY = (float) aPixel[1];
	mApp->mBoard->mEffects.Add(aFlash);
	mApp->mBoard->mEffects.Add(new Shatter(mApp, mApp->mAtomC[theAtom->mType], aPixel[0], aPixel[1]));
	delete theAtom;
	mApp->mSndNukeBlast->Play();
}

// 0x40eba0: lightning between the atom of a cell and each neighbour of the same type
void Grid::SpawnBonds(int theCol, int theRow)
{
	Atom* anAtom = mCells[theCol][theRow];
	if (anAtom->mPre)
		return;
	Atom* aNeighbour;
	if ((theCol < 8) && ((aNeighbour = mCells[theCol + 1][theRow]) != NULL) && (anAtom->mType == aNeighbour->mType) && !aNeighbour->mPre)
		mApp->mBoard->mEffects.Add(new Bolt(mApp, this, theCol, theRow, theCol + 1, theRow, anAtom->mColor, true));
	if (0 < theCol)
	{
		aNeighbour = mCells[theCol - 1][theRow];
		if ((aNeighbour != NULL) && (anAtom->mType == aNeighbour->mType) && !aNeighbour->mPre)
			mApp->mBoard->mEffects.Add(new Bolt(mApp, this, theCol, theRow, theCol - 1, theRow, anAtom->mColor, true));
	}
	if (theRow < 7)
	{
		aNeighbour = mCells[theCol][theRow + 1];
		if ((aNeighbour != NULL) && (anAtom->mType == aNeighbour->mType) && !aNeighbour->mPre)
			mApp->mBoard->mEffects.Add(new Bolt(mApp, this, theCol, theRow, theCol, theRow + 1, anAtom->mColor, true));
	}
	if (0 < theRow)
	{
		aNeighbour = mCells[theCol][theRow - 1];
		if ((aNeighbour != NULL) && (anAtom->mType == aNeighbour->mType) && !aNeighbour->mPre)
			mApp->mBoard->mEffects.Add(new Bolt(mApp, this, theCol, theRow, theCol, theRow - 1, anAtom->mColor, true));
	}
}

// 0x40f5d0: an atom bursts (the atomic bomb's, in every mode: StepBomb is the only caller)
void Grid::Burst(Atom* theAtom)
{
	theAtom->RemoveFromCell();
	mAtoms.Remove(theAtom);
	int aPixel[2] = {0, 0};
	CellToPixel(theAtom->mCol, theAtom->mRow, aPixel);
	aPixel[0] += mX;
	aPixel[1] += mY;
	NukeFlash* aFlash = new NukeFlash(mApp);
	aFlash->mX = (float) aPixel[0];
	aFlash->mY = (float) aPixel[1];
	mApp->mBoard->mEffects.Add(aFlash);
	Explosion* anExplosion = new Explosion(mApp);
	anExplosion->mX = (float) aPixel[0];
	anExplosion->mY = (float) aPixel[1];
	mApp->mBoard->mEffects.Add(anExplosion);
	delete theAtom;
	if (mB4c0)
		mApp->mBoard->mMeter.Add(1, 0);
	mApp->mSndNukeBlast->Play();
}

// 0x40e250: on every fifth level of the strategy mode, when no colour has more than three atoms left, no molecule can
// be made any more, and the field says so
void Grid::CheckUnsolvable()
{
	if (mApp->mMode == 1)
		return;
	if (mApp->mBoard->mMeter.mFull)
		return;
	int aLevel = mApp->mBoard->mMeter.mLevel;
	if ((aLevel % 5 == 0) && (0 < aLevel))
	{
		bool aNone = true;
		int aType = 0;
		if (0 < mNumColors)
		{
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
				return;
		}
		if (!mUnsolvable)
		{
			mUnsolvable = true;
			mBonusTick = 0;
		}
	}
}

// 0x40f2e0: the bond between two atoms of a molecule (shaking before it flies away): the lightning of the Bolt
// effect between their places. The original gives its bolt its own jitter; this one draws as a Bolt does.
void Grid::DrawBond(Graphics* g, Atom* theA, Atom* theB, bool thePlaySound)
{
	Bolt aBolt(mApp, this, theA->mX, theA->mY, theB->mX, theB->mY, theA->mColor, thePlaySound);
	aBolt.DrawPixels(g, theA->mX, theA->mY, theB->mX, theB->mY);
}
