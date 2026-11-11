// The bonus molecules (see BonusMolecules.h).

#include "BonusMolecules.h"
#include "AtomicaApp.h"
#include "Atom.h"
#include "Atomicus.h"
#include "Board.h"
#include "Effects.h"
#include "GameText.h"
#include "Grid.h"
#include "Graphics.h"
#include "ImageFont.h"

#include <cmath>

using namespace Sexy;

// the thirty shapes (Atomica.exe 0x414830)
static const BonusPattern sPatterns[30] = {
#include "BonusPatterns.inc"
};

// ---------------------------------------------------------------------------------------------------------
// The effect: the atoms of the shape spiral together and an element atom is born (0x415d20, 0x415dc0, 0x416330, 0x416480)

namespace
{

class BonusEffect : public Effect
{
public:
	struct Piece
	{
		Atom*	mAtom;
		float	mAngle;			// +0x94 degrees round the middle
		float	mRadius;		// +0x98
		float	mFrame;			// +0xa4 the atom's shrinking
		int		mX, mY;			// +0x18 +0x1c
	};
	int					mCenterX;	// +0x18 where the atoms meet (on the screen)
	int					mCenterY;	// +0x1c
	int					mTick;		// +0x20 counts down from 255, then up
	bool				mWaiting;	// +0x3c
	int					mElement;	// +0x40 the type of the element atom
	std::vector<Piece>	mPieces;	// +0x24

	// 0x415d20: waits for the shape's atoms; the swoosh plays
	BonusEffect(AtomicaApp* theApp) :
		Effect(theApp),
		mCenterX(0),
		mCenterY(0),
		mTick(0),
		mWaiting(true),
		mElement(0)
	{
		mApp->mSndWhoosh->Play();
	}

	// 0x415c90: the atoms still held are freed
	virtual ~BonusEffect()
	{
		for (size_t i = 0; i < mPieces.size(); i++)
			delete mPieces[i].mAtom;
	}

	// 0x416480: the atoms leave the field and spiral round the middle of the shape
	void Fill(int theX, int theY, std::vector<Atom*>& theAtoms)
	{
		Grid* aGrid = mApp->mBoard->mGrid;
		mPieces.clear();
		mCenterX = theX + aGrid->mX;
		mCenterY = theY + aGrid->mY;
		for (size_t i = 0; i < theAtoms.size(); i++)
		{
			Atom* anAtom = theAtoms[i];
			for (int j = 0; j < 8; j++)
			{
				Sparkle* aSparkle = new Sparkle(mApp, anAtom->mCol, anAtom->mRow, j % 4);
				aGrid->mSparkles.Add(aSparkle);
				aSparkle->mAlpha = 0xaf;
			}
			anAtom->RemoveFromCell();
			aGrid->mAtoms.Remove(anAtom);
			Piece aPiece;
			aPiece.mAtom = anAtom;
			aPiece.mX = anAtom->mX + aGrid->mX;
			aPiece.mY = anAtom->mY + aGrid->mY;
			aPiece.mAngle = (float) CompassAngle(aPiece.mX, aPiece.mY, mCenterX, mCenterY);
			int aDY = mCenterY - aPiece.mY;
			int aDX = mCenterX - aPiece.mX;
			aPiece.mRadius = sqrtf((float) (aDY * aDY + aDX * aDX));
			aPiece.mFrame = 0;
			mPieces.push_back(aPiece);
		}
		mTick = 0xff;
	}

	// 0x415dc0: three steps an update
	virtual bool Update()
	{
		Grid* aGrid = mApp->mBoard->mGrid;
		for (int aStep = 0; aStep < 3; aStep++)
		{
			if (mWaiting)
			{
				mTick--;
				if (mTick < 0)
				{
					mWaiting = false;
					mApp->mSndWhoosh->Play();
					// a ring of sparks
					int anAngle = 0;
					for (int i = 0; i < 0xb4; i++)
					{
						Spark* aSpark = new Spark(mApp);
						aSpark->mX = (float) mCenterX;
						aSpark->mY = (float) mCenterY;
						anAngle += 3;
						if (anAngle > 0x167)
							anAngle -= 0x165;
						float aSpeed = (float) (mApp->mRand.Get(6, false) + 5) * 0.25f;
						aSpark->mVX = SinDeg(anAngle) * aSpeed;
						aSpark->mVY = NegCosDeg(anAngle) * aSpeed;
						aSpark->mColor = Color(255, 255, 255, 255);
						mApp->mBoard->mEffects.Add(aSpark);
					}
					int aCell[2] = {0, 0};
					Grid::PixelToCell(mCenterX - aGrid->mX, mCenterY - aGrid->mY, aCell);
					if ((aCell[0] >= 0) && (aCell[0] < Grid::COLS) && (aCell[1] >= 0) && (aCell[1] < Grid::ROWS) &&
						(aGrid->mCells[aCell[0]][aCell[1]] != NULL))
						aGrid->mCells[aCell[0]][aCell[1]]->FlyAway(mApp->mBoard->LastTarget());
					ElementAtom* anElement = new ElementAtom(aGrid, mElement);
					anElement->mKind = KIND_PLAIN;
					anElement->PlaceAt(aCell[0], aCell[1], true);
					anElement->mSpawn = 0;
					anElement->mPreStarted = true;
					mApp->mBoard->FloatScore(aGrid->mX + anElement->mX, aGrid->mY + anElement->mY, (anElement->mLevel + 1) * 100,
						Color(255, 255, 255, 255));
					for (int i = 0; i < 0x20; i++)
					{
						Sparkle* aSparkle = new Sparkle(mApp, anElement->mCol, anElement->mRow, i % 4);
						aGrid->mSparkles.Add(aSparkle);
						aSparkle->mAlpha = 0xaf;
					}
				}
				bool aEvery = true;
				for (size_t i = 0; i < mPieces.size(); i++)
				{
					Piece& aPiece = mPieces[i];
					int aTurn = (int) aPiece.mAngle + 5;
					if (aTurn > 0x167)
						aTurn -= 0x163;
					if (aTurn < 0)
						aTurn += 0x5a;
					aPiece.mAngle = (float) aTurn;
					aPiece.mRadius = aPiece.mRadius * 0.99f;
					if (aPiece.mRadius < 0.0f)
						aPiece.mRadius = 0;
					aPiece.mFrame += 0.2f;
					if (24.0f < aPiece.mFrame)
						aPiece.mFrame = 24.0f;
					aPiece.mX = (int) (SinDeg(aTurn) * aPiece.mRadius) + mCenterX;
					aPiece.mY = (int) (NegCosDeg(aTurn) * aPiece.mRadius) + mCenterY;
					if (aEvery)
					{
						Spark* aSpark = new Spark(mApp);
						aSpark->mX = (float) (mApp->mRand.Get(8, true) + aPiece.mX);
						aSpark->mY = (float) (mApp->mRand.Get(8, true) + aPiece.mY);
						int aDir = mApp->mRand.Get(0x168, false);
						aSpark->mVX = SinDeg(aDir) * 0.25f;
						aSpark->mVY = NegCosDeg(aDir) * 0.25f;
						Color aColor = aPiece.mAtom->mColor;
						aSpark->mColor = Color(aColor.mRed < 0x96 ? 0x96 : aColor.mRed, aColor.mGreen < 0x96 ? 0x96 : aColor.mGreen,
							aColor.mBlue < 0x96 ? 0x96 : aColor.mBlue, aColor.mAlpha);
						mApp->mBoard->mEffects.Add(aSpark);
					}
					aEvery = !aEvery;
				}
			}
			else
			{
				mTick++;
				if (mTick > 254)
					return false;
			}
		}
		return true;
	}

	// 0x416330
	virtual void Draw(Graphics* g)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff - mTick));
		mApp->mStarBurst->Draw(g, mCenterX - (mApp->mStarBurst->mWidth >> 1), mCenterY - (mApp->mStarBurst->mHeight >> 1));
		int anAlpha = mTick * 2;
		if (0xfe < anAlpha)
			anAlpha = 0xff;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		for (size_t i = 0; i < mPieces.size(); i++)
		{
			Piece& aPiece = mPieces[i];
			Sprite* aSprite = mApp->mAtomShrink[aPiece.mAtom->mType][(int) aPiece.mFrame];
			aSprite->Draw(g, aPiece.mX - (aSprite->mWidth >> 1), aPiece.mY - (aSprite->mHeight >> 1));
		}
		g->SetColorizeImages(false);
	}
};

}

// ---------------------------------------------------------------------------------------------------------
// BonusMolecules

// 0x414830 / 0x415ae0 (the thirty pattern objects the original makes are the table above; 0x415c50 starts their numbering at 0, which the
// table's order gives)
BonusMolecules::BonusMolecules(AtomicaApp* theApp) :
	mApp(theApp),
	mRound(0),
	mTarget(3),
	mCount(0),
	mPattern(NULL),
	mTimer(0),
	mElement(0),
	mDone(false),
	mBlinkOn(true),
	mBlink(100)
{
	mX = -mApp->mBonusFrame->mWidth;
	for (int i = 0; i < 25; i++)
		mCurrent[i] = 0;
	for (int i = 0; i < 30; i++)
		mLists[i / 10].push_back(&sPatterns[i]);
}

// 0x415be0: a shape the level can make: every colour in it is among the level's, and every element atom is on the field
bool BonusMolecules::Feasible(const BonusPattern* thePattern)
{
	Grid* aGrid = mApp->mBoard->mGrid;
	for (int i = 0; i < 25; i++)
	{
		int aCell = thePattern->mCells[i];
		if (aCell != 0)
		{
			if (aCell < 0xc)
			{
				if (aGrid->mNumColors < aCell)
					return false;
			}
			else if (!aGrid->HasType(aCell - 1))
				return false;
		}
	}
	return true;
}

// 0x4146a0: when the shape asked for can no longer be made, the count of molecules starts again
void BonusMolecules::Revalidate()
{
	if ((mPattern != NULL) && !Feasible(mPattern))
		mRound = 0;
}

// 0x415420 / 0x414640: the shape to ask for next: the upgrade shapes first, if the field has the atoms they need
void BonusMolecules::PickPattern()
{
	for (int i = 0; i < 25; i++)
		mCurrent[i] = 0;
	mDone = true;
	Grid* aGrid = mApp->mBoard->mGrid;
	if (!aGrid->mHintShown[7] && !mApp->mHintsShown[7] && (mApp->mMode == 0))
	{
		// until the box that explains the bonus molecules (hint 7) has been shown, the first shape is asked for
		mPattern = mLists[0][0];
		for (int aCol = 0; aCol < 5; aCol++)
			for (int aRow = 0; aRow < 5; aRow++)
				mCurrent[aCol * 5 + aRow] = mPattern->mCells[aRow * 5 + aCol];
		mElement = (int) (mPattern - sPatterns);
		return;
	}
	for (int aTier = 2; aTier >= 0; aTier--)
	{
		int aCount = (int) mLists[aTier].size();
		int aStart = mApp->mRand.Get(aCount, false);
		for (int i = 0; i < aCount; i++)
		{
			const BonusPattern* aPattern = mLists[aTier][aStart];
			mPattern = aPattern;
			if (Feasible(aPattern))
			{
				for (int aCol = 0; aCol < 5; aCol++)
					for (int aRow = 0; aRow < 5; aRow++)
						mCurrent[aCol * 5 + aRow] = aPattern->mCells[aRow * 5 + aCol];
				mElement = (int) (aPattern - sPatterns);
				return;
			}
			aStart++;
			if (aCount <= aStart)
				aStart = 0;
		}
	}
	mPattern = NULL;
	mRound = 0;
}

// 0x4153f0: a molecule was made
void BonusMolecules::Upgrade()
{
	mRound++;
	mCount++;
	if (3 < mCount)
		mDone = true;
	Revalidate();
}

// 0x415540: the shape is looked for in every position on the field once enough molecules have been made
void BonusMolecules::Check(Grid* theGrid)
{
	if ((mTarget > mRound) || (mPattern == NULL))
		return;
	for (int aRow0 = -5; aRow0 < 0xd; aRow0++)
	{
		for (int aCol0 = -5; aCol0 < 0xe; aCol0++)
		{
			std::vector<Atom*> aAtoms;
			int aSumX = 0;
			int aSumY = 0;
			bool aMatch = true;
			for (int aRow = aRow0; aRow < aRow0 + 5; aRow++)
			{
				for (int aCol = aCol0; aCol < aCol0 + 5; aCol++)
				{
					int aWanted = mCurrent[(aCol - aCol0) * 5 + (aRow - aRow0)];
					if (aWanted - 1 != -1)
					{
						int aType;
						Atom* anAtom = NULL;
						if ((aCol < 0) || (aRow < 0) || (aCol > 8) || (aRow > 7))
							aType = -1;
						else
						{
							anAtom = theGrid->mCells[aCol][aRow];
							if ((anAtom == NULL) || anAtom->mPre)
								aType = -100;
							else
								aType = anAtom->mType;
						}
						if (aType == aWanted - 1)
						{
							aAtoms.push_back(anAtom);
							aSumX += anAtom->mX;
							aSumY += anAtom->mY;
						}
						else
							aMatch = false;
					}
				}
			}
			if (aMatch && !aAtoms.empty())
			{
				BonusEffect* anEffect = new BonusEffect(mApp);
				anEffect->Fill(aSumX / (int) aAtoms.size(), aSumY / (int) aAtoms.size(), aAtoms);
				mApp->mBoard->mEffects.Add(anEffect);
				anEffect->mElement = mPattern->mElement - 1;
				mRound = 0;
			}
		}
	}
}

// 0x415760: the sign slides in when enough molecules have been made and out when not
void BonusMolecules::Update()
{
	mBlink--;
	if (mBlink < 0)
	{
		mBlinkOn = !mBlinkOn;
		mBlink = (mBlinkOn ? 100 : 0) + 0x32;
	}
	int aX = mX;
	if (mRound < mTarget)
	{
		mPattern = NULL;
		if (-mApp->mBonusFrame->mWidth < aX)
			mX = aX - 2;
		if (mX == -2)
			mApp->mSndCapClick->Play();
	}
	else
	{
		if (aX < 0)
			mX = aX + 2;
		if (mX == 2 - mApp->mBonusFrame->mWidth)
		{
			mApp->mSndCapClick->Play();
			mApp->mBoard->mAtomicus->Say(4);
			PickPattern();
			mTimer = 6000;
		}
	}
}

// 0x415830: the sign with the shape in small atoms
void BonusMolecules::Draw(Graphics* g)
{
	if (!mDone && mBlinkOn && (mApp->mPauseCount == 0))
		mApp->mMakeGroups->Draw(g, 0, 0x6a);
	mApp->mBonusFrame->Draw(g, mX, 0x6a);
	g->SetColorizeImages(false);
	g->SetFont(mApp->mMonitorFontSmall);
	for (int aCol = 0; aCol < 5; aCol++)
	{
		for (int aRow = 0; aRow < 5; aRow++)
		{
			int aCell = mCurrent[aCol * 5 + aRow];
			if (aCell != 0)
			{
				Sprite* aSprite = mApp->mAtomShrink[aCell - 1][12];
				int aX = mX + 0x1e + aCol * 20;
				int aY = aRow * 20 + 0x88;
				aSprite->Draw(g, aX - (aSprite->mWidth >> 1), aY - (aSprite->mHeight >> 1));
				if (mApp->mShowSymbols && (aCell - 1 < 0xc))
				{
					g->SetColor(Color(255, 255, 255, 0x96));
					const std::string& aSymbol = AtomSymbolText(aCell - 1);
					g->DrawString(aSymbol, aX - mApp->mMonitorFontSmall->StringWidth(aSymbol) / 2, aY);
				}
			}
		}
	}
}

// 0x4146d0
void BonusMolecules::Save(FILE* theFile)
{
	fwrite(&mX, 4, 1, theFile);
	fwrite(&mRound, 4, 1, theFile);
	fwrite(&mElement, 4, 1, theFile);
	fwrite(&mTimer, 4, 1, theFile);
	fwrite(mCurrent, 100, 1, theFile);
	unsigned char aHasShape = (mPattern != NULL) ? 1 : 0;
	fwrite(&aHasShape, 1, 1, theFile);
}

// 0x414750: the shape asked for is found again by its index (0 to 29, the order the original makes the thirty patterns in, which
// is the order of the table; the file holds the index, as the original's does: 0x415aa0 numbers them, 0x414640 copies it)
void BonusMolecules::Load(FILE* theFile)
{
	fread(&mX, 4, 1, theFile);
	fread(&mRound, 4, 1, theFile);
	fread(&mElement, 4, 1, theFile);
	fread(&mTimer, 4, 1, theFile);
	fread(mCurrent, 100, 1, theFile);
	unsigned char aHasShape = 0;
	fread(&aHasShape, 1, 1, theFile);
	if (aHasShape == 0)
	{
		mPattern = NULL;
		mDone = true;
		return;
	}
	bool aFound = false;
	for (int i = 0; (i < 3) && !aFound; i++)
		for (size_t j = 0; (j < mLists[i].size()) && !aFound; j++)
			if ((int) (mLists[i][j] - sPatterns) == mElement)
			{
				mPattern = mLists[i][j];
				aFound = true;
			}
	mDone = true;
}
