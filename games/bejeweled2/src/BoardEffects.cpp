// BoardEffects.cpp: the explosions and sparks of the Board (Sexy::Board): the 3x3 blast of a flame or bomb gem
// (ExplodeAt), the small pop of a cleared gem (SmallExplodeAt), the score text (DoPoints), the particles and the
// animations (UpdateEffects, AddAlphaSparkle, AddLightning) and the dropping of new gems after a clear (DoneClearing).
// Every Rand() draw is kept in the original's order; the particles use the C library's rand() for their jitter, which
// the port takes from the framework's emulation of it (CrtRand).

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "Res.h"
#include "Points.h"
#include "Compat2004.h"
#include "Font.h"
#include "MemoryImage.h"
#include <math.h>
#include <algorithm>
#include <stdlib.h>

using namespace Sexy;

// the colours of the gems (the original's statics at 0x6797d0..0x6799cf; the tables are made at start-up). The four tables are
// one run of memory in the original, in this order, and the gem types past 7 (the rock, the bomb and the hyper cube) index past
// the end of a table into the next one (a lightning of type 9 takes gGemColors[1], a score of a bomb gem takes
// gOrigGemColors[0]); the port keeps the run as one block and the tables as views of it
static Color gColorBlock[32] =
{
	// gElectColors, 0x6797d0: the colours of the lightning
	Color(255, 255, 64), Color(200, 200, 200), Color(64, 128, 255), Color(255, 100, 100),
	Color(255, 64, 255), Color(255, 128, 64), Color(64, 255, 64), Color(64, 64, 64),
	// gGemColors, 0x679850: copied from gOrigGemColors by the Board's constructor (zero until then)
	Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0),
	Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0),
	// gPointColors, 0x6798d0: the colours of the score numbers
	Color(255, 255, 64), Color(255, 255, 255), Color(64, 128, 255), Color(255, 153, 153),
	Color(255, 64, 255), Color(255, 181, 145), Color(64, 255, 64), Color(64, 64, 64),
	// gOrigGemColors, 0x679950
	Color(255, 255, 0), Color(255, 255, 255), Color(0, 128, 255), Color(255, 100, 100),
	Color(255, 0, 255), Color(255, 128, 0), Color(0, 255, 0), Color(0, 0, 0)
};
extern Color* const gElectColors = gColorBlock;
extern Color* const gGemColors = gColorBlock + 8;
extern Color* const gPointColors = gColorBlock + 16;
extern Color* const gOrigGemColors = gColorBlock + 24;

// what ExplodeAtHelper collects for ExplodeAt (the original's statics at 0x6710c0, 0x6712c0, 0x6753c8, 0x6795c8,
// 0x6797cc and 0x6713c0): the gems the blast destroys and the squares it burns
static int gShardCount;                 // 0x6797cc
static int gShardTypes[64];             // 0x6712c0
static int gShardPoints[64][2];         // 0x6710c0
static int gShardExplodeCenter[64][2];  // 0x6753c8
static int gExplodeCount;               // 0x6713c0
static int gExplodePoints[64][2];       // 0x6795c8

// how many particles each destroyed gem and square sends, by their number (0x62a4e8 and 0x62a490)
static const float gShardIncs[16] =
{
	1.0f, 0.98f, 0.95f, 0.93f, 0.9f, 0.88f, 0.84f, 0.8f, 0.75f, 0.7f, 0.65f, 0.6f, 0.55f, 0.5f, 0.4f, 0.35f
};
static const float gExplodeIncs[16] =
{
	1.0f, 0.9f, 0.85f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.28f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f
};

// 0x594b7e
Effect* Board::AllocEffect()
{
	return new Effect();
}

// 0x5a6236: floating score text at (theX, theY); puzzle boards show none (except the secret ones with no moves left)
void Board::DoPoints(int thePoints, int theX, int theY, const Color& theColor)
{
	if (!mIsPuzzle || (mIsSecret && m1918 < 1))
	{
		int aSize = FTol(std::min((double)thePoints / mLevelValue, 80.0));
		int aLife = aSize * 2 + 70;
		if (aLife > 180)
			aLife = 180;
		Points* aPoints = new Points(mApp, FONT_DIGIT, StrFormat("%d", thePoints), theX, theY, aLife, 0, theColor);
		aPoints->mTargetScale = (float)((double)aSize * 0.006 + 0.5);
		aPoints->mRise = 0.018f;    // 0x3c9374bc
		float anAlpha = (float)((double)aSize * 0.0015 + 0.86);
		aPoints->mFade = anAlpha;
		if (anAlpha > 0.962)
			aPoints->mFade = 0.9616f;   // 0x3f7645a2
		mPoints.push_back(aPoints);
		mScore += thePoints;
	}
}

// 0x5a63a3: the blast of the gem at (theCol, theRow) takes in the 3x3 squares around it; the gems it destroys and the
// squares it burns are collected for ExplodeAt. A flame or bomb gem in the blast explodes in its turn (after 10 updates)
void Board::ExplodeAtHelper(int theCol, int theRow)
{
	mColumnExplodeTick[theCol] = mUpdateCnt;
	int aX = GetColX(theCol) + 42;
	int aY = GetRowY(theRow) + 42;
	Piece* aCenterPiece = mBoard[theRow][theCol];
	if (aCenterPiece != NULL)
	{
		aX = FTol(aCenterPiece->mX + 42.0);
		aY = FTol(42.0 + aCenterPiece->mY);
	}

	ShockWave aWave;
	aWave.mX = (float)(GetColX(theCol) + 42);
	aWave.mY = (float)(GetRowY(theRow) + 42);
	aWave.mProgress = 0;
	aWave.mRadius = 45.0f;
	aWave.mWidth = 20.0f;
	aWave.mGrowStep = 0.1f;
	aWave.mFadeStep = 0.05f;
	aWave.mGrowing = true;      // the original never sets it (its stack slot decides: GAME_NOTES, Oddities)
	mShockWaves.push_back(aWave);

	int aExplode = gExplodeCount++;
	gExplodePoints[aExplode][0] = aX;
	gExplodePoints[aExplode][1] = aY;

	int anOffset[3] = {0, -1, 1};
	for (int i = 0; i < 3; i++)
	{
		unsigned int aRow = anOffset[i] + theRow;
		for (int j = 0; j < 3; j++)
		{
			unsigned int aCol = anOffset[j] + theCol;
			if (aCol < 8 && aRow < 8)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece != NULL && aPiece->m78 == 0)
				{
					int aType = aPiece->mType;
					bool aKeep = false;
					if (aType == 9)
					{
						// a hyper cube in the blast: a ring of sparks and a bonus
						Color aWhite(255, 255, 255);
						DoPoints(FTol(mLevelValue * 75.0), FTol(42.0 + aPiece->mX),
							FTol(aPiece->mY + 42.0 - 8.0), aWhite);
						for (int k = 0; k < 0x78; k++)
						{
							double aD = (double)k;
							int aStep = FTol(aD * 0.1);
							int aRing = FTol((double)aStep + 5.4);
							int aRnd = Rand() % 100;
							double anAngle = aD * 0.106 + (double)aRnd / 400.0;
							double aRingD = (double)aRing;
							int aSX = FTol(cos(anAngle) * aRingD) + 0x2a + FTol(aPiece->mX);
							int aSY = FTol(sin(anAngle) * aRingD) + 0x2a + FTol(aPiece->mY);
							Shard* aShard = new Shard();
							aShard->mKind = 2;
							aShard->mX = (float)aSX;
							aShard->mY = (float)aSY;
							double aVX = cos(anAngle) * aRingD;
							int aRnd2 = Rand() % 100;
							aShard->mVX = (float)(((double)(float)aRnd2 / 200.0 + 0.2) * (double)(float)aVX);
							double aVY = sin(anAngle) * aRingD;
							int aRnd3 = Rand() % 100;
							aShard->mVY = (float)(((double)(float)aRnd3 / 200.0 + 0.2) * (double)(float)aVY);
							aShard->mFrameDelay = Rand() % 4 + 3;
							aShard->mHue = Rand() % 256;
							aShard->mCycleColor = true;
							mShards.push_back(aShard);
							if (mLastExplodeSoundTick != mUpdateCnt)
							{
								mApp->PlaySample(SOUND_HYPERGEM_DESTROYED);
								mLastExplodeSoundTick = mUpdateCnt;
							}
						}
					}
					else if (j != 0 || i != 0)
					{
						int n = gShardCount;
						gShardTypes[n] = aType;
						gShardPoints[n][0] = FTol(aPiece->mX) + 0x2a;
						gShardPoints[n][1] = FTol(aPiece->mY) + 0x2a;
						gShardExplodeCenter[n][0] = aX;
						gShardExplodeCenter[n][1] = aY;
						gShardCount = n + 1;
						if (aType == 8 || aPiece->m70)
							aKeep = true;
					}

					if (aKeep)
					{
						// the chain: the next flame or bomb gem explodes after 10 updates
						int aBonus = mApp->GetInteger("ExplosionChainBonus", 10) * mChainCount;
						aBonus += mApp->GetInteger("ExplosionChainBonusAdd", 40);
						aBonus = FTol((double)aBonus * mLevelValue);
						DoPoints(aBonus, FTol(42.0 + aPiece->mX), FTol(aPiece->mY + 42.0 - 8.0), gPointColors[aType]);
						mChainCount++;
						aPiece->m78 = 10;
					}
					else
					{
						if (mSelectedPiece == aPiece)
							mSelectedPiece = NULL;
						mBoard[aRow][aCol] = NULL;
						delete aPiece;
					}
				}
			}
		}
	}
}

// 0x5a68b0
void Board::ExplodeAt(int theCol, int theRow)
{
	mExplodeHappened = true;
	gExplodeCount = 0;
	gShardCount = 0;
	ExplodeAtHelper(theCol, theRow);

	float aShardInc = gShardIncs[std::min(gShardCount, 15)];
	float anAcc = 0;
	for (int aShard = 0; aShard < gShardCount; aShard++)
	{
		for (int k = 0; k < 15; k++)
		{
			anAcc += aShardInc;
			if (anAcc >= 1.0f)
			{
				anAcc -= 1.0f;
				double aD = (double)k;
				int aRadius = FTol(aD * 2.6);
				int aRnd = Rand() % 100;
				double anAngle = aD * 0.906 + (double)aRnd / 400.0;
				double aRadiusD = (double)aRadius;
				int aPX = FTol(cos(anAngle) * aRadiusD) + gShardPoints[aShard][0];
				int aPY = FTol(sin(anAngle) * aRadiusD) + gShardPoints[aShard][1];
				int aDX = aPX - gShardExplodeCenter[aShard][0];
				int aDY = aPY - gShardExplodeCenter[aShard][1];
				double aDir = atan2((double)aDY, (double)aDX);
				Shard* s = new Shard();
				int aType = gShardTypes[aShard];
				if (aType == 7)
					s->mKind = 1;
				else
					s->mColor = gGemColors[aType];
				s->mX = (float)(aPX - 15);
				s->mY = (float)(aPY - 15);
				double aJitterX = ((double)(CrtRand() % 100) / 100.0 - 0.5) * 3.1;
				s->mVX = (float)(cos(aDir) * 4.0 + aJitterX);
				double aJitterY = ((double)(CrtRand() % 100) / 100.0 - 1.0) * 3.1;
				s->mVY = (float)(sin(aDir) * 4.0 + aJitterY);
				s->mFrame = Rand() % 0x28;
				s->mFrameDelay = Rand() % 4 + 1;
				mShards.push_back(s);
			}
		}
	}

	float anExplodeInc = gExplodeIncs[std::min(gExplodeCount, 15)];
	float anEffectAcc = 0;
	for (int anExplode = 0; anExplode < gExplodeCount; anExplode++)
	{
		for (int k = 0; k < 0x12; k++)
		{
			anAcc += aShardInc;
			if (anAcc >= 1.0f)
			{
				anAcc -= 1.0f;
				double aD = (double)k;
				int aStep = FTol(aD * 0.2);
				int aRadius = FTol((double)aStep + 5.4);
				int aRnd = Rand() % 100;
				double anAngle = aD * 0.906 + (double)aRnd / 400.0;
				double aRadiusD = (double)aRadius;
				int aPX = FTol(cos(anAngle) * aRadiusD) + gExplodePoints[anExplode][0];
				int aPY = FTol(sin(anAngle) * aRadiusD) + gExplodePoints[anExplode][1];
				int aDX = aPX - gShardExplodeCenter[anExplode][0];
				int aDY = aPY - gShardExplodeCenter[anExplode][1];
				double aDir = atan2((double)aDY, (double)aDX);
				Shard* s = new Shard();
				s->mKind = 2;
				s->mX = (float)aPX;
				s->mY = (float)aPY;
				s->mVX = (float)(cos(aDir) * aRadiusD * (double)0.351f);
				s->mVY = (float)(sin(aDir) * aRadiusD * (double)0.351f);
				s->mFrameDelay = Rand() % 4 + 3;
				mShards.push_back(s);
			}
		}

		for (int k = 0; k < 0x50; k++)
		{
			anEffectAcc += anExplodeInc;
			if (anEffectAcc >= 1.0f)
			{
				anEffectAcc -= 1.0f;
				Effect* e = AllocEffect();
				int aRadius = FTol((double)(k + 0x78) / 120.0) * k;
				int aRnd = Rand() % 100;
				e->mKind = 0;
				double anAngle = (double)k * 0.503 + (double)aRnd / 800.0;
				double aRadiusD = (double)aRadius;
				e->mX = FTol(cos(anAngle) * aRadiusD) + gExplodePoints[anExplode][0];
				e->mY = FTol(sin(anAngle) * aRadiusD) + gExplodePoints[anExplode][1];
				e->mDelay = aRadius / 10;
				mEffects.push_back(e);
			}
		}
	}

	if (mChainCount > 0)
	{
		float aChain = (float)mChainCount;
		double aPitch = (double)aChain * 0.2 + 1.2;
		if (2.0 <= aPitch)
			aPitch = 2.0;
		mShakeTime = (float)aPitch;
		double aVolume = (double)aChain * 1.2 + 1.25;
		if (6.0 <= aVolume)
			aVolume = 6.0;
		mShakeSize = (float)aVolume;
	}
	ExplodingFinished();
}

// 0x5a6e5f: the pop of a cleared gem
void Board::SmallExplodeAt(int theCol, int theRow)
{
	mColumnExplodeTick[theCol] = mUpdateCnt;
	int aX = GetColX(theCol) + 42;
	int aY = GetRowY(theRow) + 42;

	ShockWave aWave;
	aWave.mX = (float)(GetColX(theCol) + 42);
	aWave.mY = (float)(GetRowY(theRow) + 42);
	aWave.mProgress = 0;
	aWave.mRadius = 45.0f;
	aWave.mWidth = 20.0f;
	aWave.mGrowStep = 0.1f;
	aWave.mFadeStep = 0.05f;
	aWave.mGrowing = true;
	mShockWaves.push_back(aWave);

	for (int k = 0; k < 10; k++)
	{
		Effect* e = AllocEffect();
		int aRadius = FTol((double)(k + 0x78) / 120.0) * k;
		int aRnd = Rand() % 100;
		e->mKind = 0;
		double anAngle = (double)k * 0.503 + (double)aRnd / 800.0;
		double aRadiusD = (double)aRadius;
		e->mX = FTol(cos(anAngle) * aRadiusD) + aX;
		e->mY = FTol(sin(anAngle) * aRadiusD) + aY;
		e->mDelay = aRadius / 10;
		mEffects.push_back(e);
	}

	Piece* aPiece = mBoard[theRow][theCol];
	if (aPiece->mType != 9)
	{
		for (int k = 0; k < 0x12; k++)
		{
			double aD = (double)k;
			int aRadius = FTol(aD * 2.6);
			int aRnd = Rand() % 100;
			double anAngle = aD * 0.906 + (double)aRnd / 400.0;
			int aBaseX = FTol(aPiece->mX);
			int aBaseY = FTol(aPiece->mY) + 0x2a;
			double aRadiusD = (double)aRadius;
			int aSX = FTol(cos(anAngle) * aRadiusD) + aBaseX + 0x2a;
			int aSY = FTol(sin(anAngle) * aRadiusD) + aBaseY;
			int aDX = aSX - aX;
			int aDY = aSY - aY;
			double aDir = atan2((double)aDY, (double)aDX);
			Shard* s = new Shard();
			if (aPiece->mType == 7)
				s->mKind = 1;
			else
				s->mColor = gGemColors[aPiece->mType];
			s->mX = (float)(aSX - 15);
			s->mY = (float)(aSY - 15);
			double aJitterX = ((double)(CrtRand() % 100) / 100.0 - 0.5) * 1.9;
			s->mVX = (float)(cos(aDir) * 2.5 + aJitterX);
			double aJitterY = ((double)(CrtRand() % 100) / 100.0 - 1.0) * 1.9;
			s->mVY = (float)(sin(aDir) * 2.5 + aJitterY);
			s->mFrame = Rand() % 0x28;
			s->mFrameDelay = Rand() % 4 + 1;
			mShards.push_back(s);
		}
	}
	if (mSelectedPiece == aPiece)
		mSelectedPiece = NULL;
	mBoard[theRow][theCol] = NULL;
	delete aPiece;
}

// 0x5a71f0: the explosions are over: the gems fall to fill the holes, and new ones drop in from above (the classic
// game checks that the new gems leave a move)
void Board::DoneClearing()
{
	for (int aRow = 7; aRow >= 0; aRow--)
		for (int aCol = 0; aCol < 8; aCol++)
			if (mBoard[aRow][aCol] != NULL && mBoard[aRow][aCol]->m78 > 0)
				return;

	std::vector<Piece*> aNewPieces;
	bool aChanged;
	do
	{
		aChanged = false;
		for (int aCol = 0; aCol < 8; aCol++)
		{
			bool aHole = false;
			int aLowestY = GetRowY(0);
			double aLastVY = 0;
			double anInitialVY = 1;
			for (int aRow = 7; aRow >= 0; aRow--)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece == NULL)
				{
					if (aHole)
						mBoard[aRow + 1][aCol] = NULL;
					aHole = true;
				}
				else
				{
					if (!mFlipped)
					{
						if (aPiece->mY < (float)aLowestY)
							aLowestY = FTol(aPiece->mY);
					}
					else if ((float)aLowestY < aPiece->mY)
						aLowestY = FTol(aPiece->mY);

					if (aHole)
					{
						aChanged = true;
						if (aPiece->mVY == 0.0f)
						{
							if (!mFlipped)
								aPiece->mVY = (float)((double)aPiece->mVY + anInitialVY);
							else
								aPiece->mVY = (float)((double)aPiece->mVY - anInitialVY);
							aPiece->m5C = mUpdateCnt;
						}
						aPiece->mRow++;
						aLastVY = (double)aPiece->mVY;
						mBoard[aRow + 1][aCol] = aPiece;
					}
				}
			}

			if (aHole)
			{
				if (!mIsPuzzle)
				{
					aChanged = true;
					Piece* aNew = new Piece();
					if (!mFlipped)
						aNew->mVY = (float)(aLastVY - 0.55);
					else
						aNew->mVY = (float)(aLastVY + 0.35);
					aNew->m5C = mUpdateCnt;
					aNew->mCol = aCol;
					aNew->mRow = 0;
					aNew->mX = (float)GetColX(aCol);
					if (!mFlipped)
					{
						float aY = (float)(aLowestY - 0x66);
						aNew->mY = aY;
						if (-84.0f < aY)
							aNew->mY = -84.0f;
					}
					else
					{
						float aY = (float)(aLowestY + 0x66);
						aNew->mY = aY;
						if (aY < 768.0f)
							aNew->mY = 768.0f;
					}
					aNewPieces.push_back(aNew);
					mBoard[0][aCol] = aNew;
				}
				else
					mBoard[0][aCol] = NULL;
			}
		}
	} while (aChanged);

	if (!aNewPieces.empty())
	{
		bool aIsEndless = mIsEndless;
		int aCount = 0;
		bool aHasMove = true;
		if (aIsEndless || mIsAction || mLevel == 0)
			aHasMove = false;
		if (!mIsAction && !aIsEndless && mIsSecret)
			aHasMove = true;

		bool aAllSame;
		do
		{
			aAllSame = true;
			int aFirstType = 0;
			for (int i = 0; i < (int)aNewPieces.size(); i++)
			{
				Piece* aPiece = aNewPieces[i];
				aPiece->m58 = 0;
				aPiece->m70 = false;
				aPiece->mType = mRand.Next() % 7;
				if (mIsFinity)
				{
					double aRoll = (double)(mRand.Next() % 1000) / 1000.0;
					bool aBomb = mApp->GetDouble("BombChance") > aRoll;
					if (!aBomb)
					{
						aRoll = (double)(mRand.Next() % 1000) / 1000.0;
						bool aRock = mApp->GetDouble("RockChance") > aRoll;
						if (!aRock)
						{
							aRoll = (double)(mRand.Next() % 1000) / 1000.0;
							if (aRoll < mApp->GetDouble("PowerGemChance"))
								aPiece->m70 = true;
						}
						else
							aPiece->mType = 7;
					}
					else
					{
						aPiece->mType = 8;
						aPiece->m58 = mApp->GetInteger("BombCountdown");
					}
				}

				aCount++;
				if (aCount >= 500 && i == 0)
				{
					aPiece->m58 = 0;
					aPiece->mType = 8;
					aHasMove = true;
				}
				if (i == 0)
					aFirstType = aPiece->mType;
				else if (aPiece->mType != aFirstType)
					aAllSame = false;
			}
			aHasMove |= FindMove(NULL, 0, true, true);
		} while (!aHasMove);

		if (aNewPieces.size() > 1 && mIsFinity && aAllSame && aCount > 1)
			aNewPieces[1]->m70 = true;
	}

	MarkDirtyFull();
	mState = STATE_NEW;
	mCascadeCount++;
}

// 0x5a7712: the particles, the explosion animations and the ripples move on by one update
void Board::UpdateEffects()
{
	for (int i = 0; i < (int)mShards.size(); i++)
	{
		bool aRemove = false;
		Shard* s = mShards[i];
		if (s->mCycleColor)
		{
			s->mHue = (s->mHue + 8) & 0xff;
			s->mColor = Color(mApp->HSLToRGB(s->mHue, 0xff, 0x80));
		}
		if (0.0f < s->mTrailChance)
		{
			int aRnd = Rand() % 100000;
			if ((float)aRnd / 100000.0 < (double)s->mTrailChance)
			{
				Shard* aTrail = new Shard();
				aTrail->mKind = 2;
				aTrail->mX = s->mX;
				aTrail->mY = s->mY;
				aTrail->mVX = (float)((double)s->mVX * 0.35);
				aTrail->mVY = (float)((double)s->mVY * 0.35 + 0.25);
				aTrail->mFrameDelay = 5;
				mShards.push_back(aTrail);
			}
			s->mTrailChance = s->mTrailStep + s->mTrailChance;
		}

		s->mAge++;
		if (s->mKind != 5)
		{
			if (s->mKind == 3)
			{
				float aSwing = (float)sin((double)(s->mFrequency * (float)s->mAge));
				s->mX = s->mAmplitude * aSwing + s->mBaseX;
				int aReach = abs(FTol((double)s->mVY * 3.0 + (double)s->mY2));
				if (s->mLimit <= (float)aReach)
					s->mFading = true;
				if (!s->mFading)
				{
					float anAlpha = (float)((double)s->mAlpha + 0.1);
					if (1.0f <= anAlpha)
						anAlpha = 1.0f;
					s->mAlpha = anAlpha;
				}
				else
				{
					float anAlpha = (float)((double)s->mAlpha - 0.1);
					s->mAlpha = anAlpha;
					if (anAlpha <= 0.0f)
						aRemove = true;
				}
			}
			else
			{
				float aVY;
				if (s->mKind == 2)
				{
					s->mVX = (float)((double)s->mVX * 0.98);
					float aSlowed = (float)((double)s->mVY * 0.98);
					aVY = (float)((double)s->mWeight * 0.07 + (double)aSlowed);
				}
				else
					aVY = (float)((double)s->mWeight * 0.15 + (double)s->mVY);
				s->mVY = aVY;
			}
		}

		float aVY = s->mGravity + s->mVY;
		s->mVY = aVY;
		s->mX = s->mVX + s->mX;
		s->mY = aVY + s->mY;
		s->mY2 = aVY + s->mY2;
		MarkDirtyFull();
		if (s->mKind == 3)
		{
			float aDX = s->mX - s->mCenterX;
			float aDY = s->mY - s->mCenterY;
			float aCos = (float)cos((double)s->mAngle);
			float aSin = (float)sin((double)s->mAngle);
			s->mPosX = (aCos * aDX + s->mCenterX) - aSin * aDY;
			s->mPosY = aSin * aDX + aCos * aDY + s->mCenterY;
		}
		if (mUpdateCnt % s->mFrameDelay == 0)
			s->mFrame = (s->mFrame + 1) % 0x28;
		if (s->mKind == 2 && s->mFrame == 0xe)
			aRemove = true;
		if ((double)mHeight <= (double)mApp->ScaleNum(s->mY, 0.0f))
			aRemove = true;

		if (aRemove)
		{
			// the last one takes its place
			delete s;
			mShards[i] = mShards.back();
			mShards.pop_back();
			i--;
		}
	}

	for (int i = 0; i < (int)mEffects.size(); i++)
	{
		Effect* e = mEffects[i];
		if (e->mKind == 0)
		{
			if (e->mDelay < 1)
			{
				if ((mUpdateCnt & 1) == 0)
				{
					e->mFrame++;
					MarkDirtyFull();
					if (e->mFrame == 0x14)
					{
						delete e;
						mEffects[i] = mEffects.back();
						mEffects.pop_back();
						i--;
					}
				}
			}
			else
				e->mDelay--;
		}
	}

	for (int i = 0; i < (int)mShockWaves.size(); i++)
	{
		ShockWave& w = mShockWaves[i];
		if (!w.mGrowing)
		{
			w.mProgress = w.mProgress - w.mFadeStep;
			if (w.mProgress <= 0.0f)
			{
				mShockWaves.erase(mShockWaves.begin() + i);
				i--;
			}
		}
		else
		{
			w.mProgress = w.mGrowStep + w.mProgress;
			if (w.mProgress >= 1.0f)
			{
				w.mProgress = 1.0f;
				w.mGrowing = false;
			}
		}
	}
}

extern bool gTestNoSparkle;  // TestCommands.cpp: the test command nosparkle

// 0x5a7bfb: a spark on a solid pixel of the picture (up to 100 tries to find one)
void Board::AddAlphaSparkle(int theX, int theY, MemoryImage* theImage, int theWidth, int theHeight)
{
	if (gTestNoSparkle)
		return;
	if (theWidth == 0)
		theWidth = theImage->GetWidth();
	if (theHeight == 0)
		theHeight = theImage->GetHeight();
	ulong* aBits = theImage->GetBits();
	for (int aTry = 0; aTry < 100; aTry++)
	{
		int aX = Rand() % theWidth;
		int aY = Rand() % theHeight;
		if (0xa0000000 < (aBits[theImage->GetWidth() * aY + aX] & 0xff000000))
		{
			Shard* s = new Shard();
			s->mKind = 2;
			s->mX = (float)(aX + theX);
			s->mY = (float)(aY + theY);
			s->mVX = 0;
			s->mVY = (float)(fabsf(GetRandFloat()) * 0.5);
			s->mFrameDelay = Rand() % 4 + 3;
			s->mWeight = 0.1f;      // 0x3dcccccd
			mShards.push_back(s);
			break;
		}
	}
}

// 0x5a7d39: a flash of lightning from (theX1, theY1) to (theX2, theY2)
void Board::AddLightning(int theX1, int theY1, int theX2, int theY2)
{
	Lightning* aLightning = new Lightning();
	aLightning->mProgress = 0;
	float aDY = (float)(theY2 - theY1);
	float aDX = (float)(theX2 - theX1);
	float anAngle = (float)atan2((double)aDY, (double)aDX);
	float aLength = (float)sqrt((double)(aDY * aDY + aDX * aDX));
	float aSwing = (float)((double)anAngle - 1.570795);
	aLightning->mSwingX = (float)((double)(float)cos((double)aSwing) * (double)aLength * (double)0.4f);
	aLightning->mSwingY = (float)((double)(float)sin((double)aSwing) * (double)aLength * (double)0.4f);
	float aX1 = (float)theX1;
	float aX2 = (float)theX2;
	float aY1 = (float)theY1;
	for (int i = 0; i < 8; i++)
	{
		float aT = (float)((double)i / 7.0);
		float aX = (float)((double)aX1 * (1.0 - (double)aT) + (double)aX2 * (double)aT);
		float aY = (float)((double)(float)theY2 * (double)aT + (double)aY1 * (1.0 - (double)aT));
		aLightning->mPoint[i][0] = aX;
		aLightning->mPoint[i][1] = aY;
		aLightning->mPoint[i][2] = aX;
		aLightning->mPoint[i][3] = aY;
	}
	mLightning.push_back(aLightning);
}
