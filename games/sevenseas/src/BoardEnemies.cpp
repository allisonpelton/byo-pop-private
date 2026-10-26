// Board.cpp, part 4: the enemy ships (WinSS.exe 0x404bb0 their plans, 0x405700 their movement) and the small
// timers (0x405ca0-0x405f10, 0x40a050).

#include "Board.h"
#include "SevenSeasApp.h"
#include "Compat2004.h"

#include <cstdlib>

using namespace Sexy;

// the heading for a step of (dx, dy), as in Board::UpdateMove (0 when there is none)
static int StepHeading(int theDX, int theDY)
{
	int aHeading = 0;
	if (theDY < 0)
	{
		if (theDX < 0)
			aHeading = 0xe;
		else
			aHeading = (theDX < 1) - 1 & 2;
	}
	else if (theDY < 1)
	{
		if (theDX < 0)
			aHeading = 0xc;
		else if (0 < theDX)
			aHeading = 4;
	}
	else if (theDX < 0)
		aHeading = 10;
	else
		aHeading = ((theDX < 1) - 1 & -2) + 8;
	return aHeading;
}

// the original steps back to the node before the current one before it removes the current ship, and goes on from
// there; returns true when the ship is the first of the list (then the loop goes on from the new first)
static bool StepBack(std::list<Ship*>& theShips, std::list<Ship*>::iterator& theItr)
{
	if (theItr == theShips.begin())
		return true;
	--theItr;
	return false;
}

// 0x404bb0: every ship chooses its next cell and the heading to it
void Board::PlanEnemyMoves()
{
	bool aAllMonsters = true;
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
	{
		Ship* aShip = *anItr;
		aShip->mNextX = aShip->mX;
		aShip->mNextY = aShip->mY;
		if (aShip->mType != 2)
			aAllMonsters = false;
	}
	if (!aAllMonsters)
	{
		for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
		{
			Ship* aShip = *anItr;
			int aDX = 0;
			int aDY = 0;
			int aHeading = 0;
			bool aSetHeading = true;     // false: the heading was chosen directly
			bool aSkipHeading = false;   // the movement is none and the heading stays 0 (the original's default case)
			switch (aShip->mType)
			{
			case 0:
				if (mPlayerX < aShip->mX)
					aDX = -1;
				if (aShip->mX < mPlayerX)
					aDX = aDX + 1;
				if (mPlayerY < aShip->mY)
					aDY = -1;
				if (aShip->mY < mPlayerY)
					aDY = aDY + 1;
				break;
			case 1:
				if (!mApp->mTutorial)
				{
					int aX = 0;
					int aDistRow = 0xe8;
					do
					{
						int aY = 0;
						do
						{
							if ((mTerrain[aX][aY] == 0) && !mVisited[aX][aY] && (ShipAt(aX, aY, aShip) == NULL))
								(&mDist[0][0])[aDistRow - 0xe8 + aY] = 99;
							else
								(&mDist[0][0])[aDistRow - 0xe8 + aY] = 100;
							aY++;
						}
						while (aY < 0xb);
						aX++;
						aDistRow = aDistRow + 0xb;
					}
					while (aDistRow < 0x161);
					mDist[0][0] = 100;
					mDist[0][10] = 100;
					mDist[10][0] = 100;
					mDist[10][10] = 100;
					mDist[mPlayerX][mPlayerY] = 0;
					Flood(mPlayerX, mPlayerY);
					int aBest = 0x65;
					int aTX = -1;
					do
					{
						int aTY = -1;
						do
						{
							int aNX = aShip->mX + aTX;
							int aNY = aShip->mY + aTY;
							if (((aTX != 0) || (aTY != 0)) && (-1 < aNX) && (aNX < 0xb) && (-1 < aNY) && (aNY < 0xb))
							{
								int aValue = mDist[aNX][aNY];
								if (aValue < aBest)
								{
									aBest = aValue;
									aDX = aTX;
									aDY = aTY;
								}
								else if (aValue == aBest)
								{
									if (CrtRand() % 2 == 0)
									{
										aBest = aValue;
										aDX = aTX;
										aDY = aTY;
									}
								}
							}
							aTY++;
						}
						while (aTY < 2);
						aTX++;
					}
					while (aTX < 2);
					if (aBest < 99)
						break;
					if (mPlayerX < aShip->mX)
						aDX = -1;
					if (aShip->mX < mPlayerX)
						aDX = 1;
					if (mPlayerY < aShip->mY)
						aDY = -1;
					if (aShip->mY < mPlayerY)
						aDY = 1;
					break;
				}
				// the tutorial: the red pirate follows the script
				switch (mTutorialStep)
				{
				case 2:
					aDY = 1;
					aHeading = 8;
					aSetHeading = false;
					break;
				case 3:
				case 5:
				case 6:
				case 7:
				case 8:
					aHeading = 6;
					aDX = 1;
					aDY = 1;
					break;
				case 9:
					aDY = 1;
					aDX = -1;
					break;
				case 10:
					aDX = -1;
					break;
				default:
					aSkipHeading = true;
					break;
				}
				break;
			case 2:
				if (CrtRand() % 2 == 0)
				{
					int aX = 0;
					int aDistRow = 0xe8;
					do
					{
						int aY = 0;
						do
						{
							if ((mTerrain[aX][aY] == 0) && !mVisited[aX][aY])
								(&mDist[0][0])[aDistRow - 0xe8 + aY] = 99;
							else
								(&mDist[0][0])[aDistRow - 0xe8 + aY] = 100;
							aY++;
						}
						while (aY < 0xb);
						aX++;
						aDistRow = aDistRow + 0xb;
					}
					while (aDistRow < 0x161);
					mDist[0][0] = 100;
					mDist[0][10] = 100;
					mDist[10][0] = 100;
					mDist[10][10] = 100;
					mDist[mPlayerX][mPlayerY] = 0;
					Flood(mPlayerX, mPlayerY);
					int aBest = 0x65;
					int aTX = -1;
					do
					{
						int aTY = -1;
						do
						{
							int aNX = aShip->mX + aTX;
							int aNY = aShip->mY + aTY;
							if ((-1 < aNX) && (aNX < 0xb) && (-1 < aNY) && (aNY < 0xb))
							{
								int aValue = mDist[aNX][aNY];
								if (aValue < aBest)
								{
									aBest = aValue;
									aDX = aTX;
									aDY = aTY;
								}
								else if (aValue == aBest)
								{
									if (CrtRand() % 2 == 0)
									{
										aBest = aValue;
										aDX = aTX;
										aDY = aTY;
									}
								}
							}
							aTY++;
						}
						while (aTY < 2);
						aTX++;
					}
					while (aTX < 2);
				}
				else
				{
					bool aFound = false;
					int aCol = -1;
					do
					{
						int aNX = aCol + aShip->mX;
						int aNY = aShip->mY - 1;
						int aCount = 3;
						do
						{
							if ((-1 < aNX) && (aNX < 0xb) && (-1 < aNY) && (aNY < 0xb) &&
								(((aNX != 0) && (aNX != 10)) || ((aNY != 0) && (aNY != 10))) &&
								(mTerrain[aNX][aNY] == 0) && !mVisited[aNX][aNY])
								aFound = true;
							aNY++;
							aCount--;
						}
						while (aCount != 0);
						aCol++;
					}
					while (aCol < 2);
					if (!aFound)
					{
						aDX = 0;
						aDY = 0;
						aSkipHeading = true;
						break;
					}
					int aNX;
					int aNY;
					do
					{
						do
						{
							aDX = abs(CrtRand() % 3) - 1;
							aDY = abs(CrtRand() % 3) - 1;
							aNX = aShip->mX + aDX;
							aNY = aShip->mY + aDY;
						}
						while (aNX < 0);
					}
					while ((10 < aNX) || (aNY < 0) || (10 < aNY) || (((aNX == 0) || (aNX == 10)) && ((aNY == 0) || (aNY == 10))) ||
						(mTerrain[aNX][aNY] != 0) || mVisited[aNX][aNY]);
				}
				break;
			case 3:
			{
				int aX = 0;
				int aDistRow = 0xe8;
				do
				{
					int aY = 0;
					do
					{
						if ((mTerrain[aX][aY] == 0) && !mVisited[aX][aY] && (ShipAt(aX, aY, aShip) == NULL))
							(&mDist[0][0])[aDistRow - 0xe8 + aY] = 99;
						else
							(&mDist[0][0])[aDistRow - 0xe8 + aY] = 100;
						aY++;
					}
					while (aY < 0xb);
					aX++;
					aDistRow = aDistRow + 0xb;
				}
				while (aDistRow < 0x161);
				mDist[0][0] = 100;
				mDist[0][10] = 100;
				mDist[10][0] = 100;
				mDist[10][10] = 100;
				mDist[mPlayerX][mPlayerY] = 0;
				Flood(mPlayerX, mPlayerY);
				int aBest = 0x65;
				int aTX = -1;
				do
				{
					int aTY = -1;
					do
					{
						int aNX = aShip->mX + aTX;
						int aNY = aShip->mY + aTY;
						if (((aTX != 0) || (aTY != 0)) && (-1 < aNX) && (aNX < 0xb) && (-1 < aNY) && (aNY < 0xb))
						{
							int aValue = mDist[aNX][aNY];
							if (aValue < aBest)
							{
								aBest = aValue;
								aDX = aTX;
								aDY = aTY;
							}
							else if (aValue == aBest)
							{
								if (CrtRand() % 2 == 0)
								{
									aBest = aValue;
									aDX = aTX;
									aDY = aTY;
								}
							}
						}
						aTY++;
					}
					while (aTY < 2);
					aTX++;
				}
				while (aTX < 2);
				if (98 < aBest)
				{
					if (mPlayerX < aShip->mX)
						aDX = -1;
					if (aShip->mX < mPlayerX)
						aDX = 1;
					if (mPlayerY < aShip->mY)
						aDY = -1;
					if (aShip->mY < mPlayerY)
						aDY = 1;
				}
				break;
			}
			case 4:
			{
				int aX = 0;
				int aDistRow = 0xe8;
				do
				{
					int aY = 0;
					do
					{
						if ((mTerrain[aX][aY] == 0) && !mVisited[aX][aY] && (ShipAt(aX, aY, aShip) == NULL))
							(&mDist[0][0])[aDistRow - 0xe8 + aY] = 99;
						else
							(&mDist[0][0])[aDistRow - 0xe8 + aY] = 100;
						aY++;
					}
					while (aY < 0xb);
					aX++;
					aDistRow = aDistRow + 0xb;
				}
				while (aDistRow < 0x161);
				mDist[0][0] = 100;
				mDist[0][10] = 100;
				mDist[10][0] = 100;
				mDist[10][10] = 100;
				mDist[mPlayerX][mPlayerY] = 0;
				Flood(mPlayerX, mPlayerY);
				int aBest = 0x65;
				int aTX = -1;
				// the ghost keeps out of the player's broadside while it can
				do
				{
					int aTY = -1;
					do
					{
						int aNX = aShip->mX + aTX;
						int aNY = aShip->mY + aTY;
						if (((aTX != 0) || (aTY != 0)) && (-1 < aNX) && (aNX < 0xb) && (-1 < aNY) && (aNY < 0xb) &&
							!CanFireAt(mPlayerX, mPlayerY, mHeading, aNX, aNY, 3))
						{
							int aValue = mDist[aNX][aNY];
							if (aValue < aBest)
							{
								aBest = aValue;
								aDX = aTX;
								aDY = aTY;
							}
							else if (aValue == aBest)
							{
								if (CrtRand() % 2 == 0)
								{
									aBest = mDist[aNX][aNY];
									aDX = aTX;
									aDY = aTY;
								}
							}
						}
						aTY++;
					}
					while (aTY < 2);
					aTX++;
				}
				while (aTX < 2);
				int aX2 = -1;
				do
				{
					int aY2 = -1;
					do
					{
						int aNX = aShip->mX + aX2;
						int aNY = aShip->mY + aY2;
						if (((aX2 != 0) || (aY2 != 0)) && (-1 < aNX) && (aNX < 0xb) && (-1 < aNY) && (aNY < 0xb) &&
							(mDist[aNX][aNY] < aBest))
						{
							aBest = mDist[aNX][aNY];
							aDY = aY2;
							aDX = aX2;
						}
						aY2++;
					}
					while (aY2 < 2);
					aX2++;
				}
				while (aX2 < 2);
				if (98 < aBest)
				{
					if (mPlayerX < aShip->mX)
						aDX = -1;
					if (aShip->mX < mPlayerX)
						aDX = 1;
					if (mPlayerY < aShip->mY)
						aDY = -1;
					if (aShip->mY < mPlayerY)
						aDY = 1;
				}
				break;
			}
			default:
				aSkipHeading = true;
				break;
			}
			if (!aSkipHeading && aSetHeading && ((aDX != 0) || (aDY != 0)))
				aHeading = StepHeading(aDX, aDY);
			aShip->mWantHeading = aHeading;
			aShip->mNextX = aShip->mX + aDX;
			aShip->mNextY = aShip->mY + aDY;
		}
		mState = STATE_ENEMY_TURN;
		mDirty = true;
		return;
	}
	mLevelComplete = true;
}

// 0x405700: the enemy ships turn (state 9), then sail to their next cells (state 10) and meet what is there
void Board::UpdateEnemies()
{
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
	{
		Ship* aShip = *anItr;
		aShip->mAnim = aShip->mAnim + 1;
		if (mUpdateCnt % 4 == 0)
		{
			for (int i = 0; i < 9; i++)
			{
				if (-1 < aShip->mTrail[i])
					aShip->mTrail[i] = aShip->mTrail[i] - 1;
			}
		}
	}

	bool aWait = false;
	if (mState == STATE_ENEMY_TURN)
	{
		bool aTurning = false;
		for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
		{
			Ship* aShip = *anItr;
			if (aShip->mWantHeading != aShip->mHeading)
				aTurning = true;
			aShip->mMoveProgress = 0;
		}
		if (aTurning)
			aWait = true;
		else
		{
			mApp->PlaySample(SOUND_MOVEEVIL);
			mState = STATE_ENEMY_MOVE;
			mDirty = true;
			aWait = true;
		}
	}
	else if (mState == STATE_ENEMY_MOVE)
	{
		bool aMoving = false;
		int aCount = 0;
		bool aMonster = false;
		for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
		{
			Ship* aShip = *anItr;
			if ((aShip->mNextX != aShip->mX) || (aShip->mNextY != aShip->mY))
				aMoving = true;
			if (aShip->mType == 2)
				aMonster = true;
			aCount++;
		}
		if (!aMoving)
		{
			if ((mShips.size() == 0) || ((aCount == 1) && aMonster && !mLevelOver && !mApp->mTutorial))
			{
				mApp->LevelComplete();
				ClearCellFlash();
			}
			else
				EnemyShots();
		}
		aWait = true;
	}
	(void) aWait;

	std::list<Ship*>::iterator anItr = mShips.begin();
	while (anItr != mShips.end())
	{
		Ship* aShip = *anItr;
		bool aAtFront = false;
		if ((aShip->mHeading == aShip->mWantHeading) || (mState != STATE_ENEMY_TURN) || AnyBlockingAnim())
		{
			if (mState == STATE_ENEMY_MOVE)
			{
				if (aShip->mMoveProgress < 0x2a)
				{
					if ((aShip->mMoveProgress % 6 == 0) && (aShip->mType != 4))
					{
						int aX;
						int aY;
						if ((aShip->mHeading == 4) || (aShip->mHeading == 0xc))
						{
							aY = CellToPixel(aShip->mY) + 5 + (aShip->mNextY - aShip->mY) * (aShip->mMoveProgress - 8);
							aX = CellToPixel(aShip->mX) + (aShip->mNextX - aShip->mX) * (aShip->mMoveProgress - 8);
							SpawnSmoke(aX, aY);
							aY = CellToPixel(aShip->mY) + 5 + (aShip->mNextY - aShip->mY) * (aShip->mMoveProgress - 9);
							aX = CellToPixel(aShip->mX) + (aShip->mNextX - aShip->mX) * (aShip->mMoveProgress - 9);
						}
						else
						{
							aY = CellToPixel(aShip->mY) + (aShip->mNextY - aShip->mY) * (aShip->mMoveProgress - 8);
							aX = CellToPixel(aShip->mX) + (aShip->mNextX - aShip->mX) * (aShip->mMoveProgress - 8);
							SpawnSmoke(aX, aY);
							aY = CellToPixel(aShip->mY) + (aShip->mNextY - aShip->mY) * (aShip->mMoveProgress - 9);
							aX = CellToPixel(aShip->mX) + (aShip->mNextX - aShip->mX) * (aShip->mMoveProgress - 9);
						}
						SpawnSmoke(aX, aY);
					}
					aShip->mMoveProgress = aShip->mMoveProgress + 2;
				}
				else
				{
					int aNX = aShip->mNextX;
					if ((aNX == mPlayerX) && (aShip->mNextY == mPlayerY) && !mVisited[mPlayerX][mPlayerY])
					{
						// the ship sails into the player's cell
						aShip->mY = aShip->mNextY;
						aShip->mX = aNX;
						if (aShip->mType == 2)
						{
							mApp->PlaySample(SOUND_MONSTER, Pan(aNX));
							LoseLife();
						}
						else
						{
							mVisited[aShip->mNextX][aShip->mNextY] = true;
							aAtFront = StepBack(mShips, anItr);
							SpawnExplosion(aShip->mNextX, aShip->mNextY, true);
							AddPoints(0x14, aShip->mNextX, aShip->mNextY);
							SetScoreCheck(1);
							RemoveShip(aShip);
							mKills++;
							mKillsTotal++;
							LoseLife();
						}
					}
					else
					{
						int aNY = aShip->mNextY;
						if ((mTerrain[aNX][aNY] == 0) && !mVisited[aNX][aNY])
						{
							Ship* anOther = ShipAt(aNX, aNY, aShip);
							if (anOther == NULL)
							{
								aShip->mX = aShip->mNextX;
								aShip->mY = aShip->mNextY;
							}
							else
							{
								if (ShipAt(aNX, aNY, aShip)->mType == 2)
								{
									mApp->PlaySample(SOUND_MONSTER, Pan(aShip->mNextX));
								}
								else
								{
									AddPoints(0x14, aShip->mNextX, aShip->mNextY);
									SetScoreCheck(1);
									RemoveShip(ShipAt(aShip->mNextX, aShip->mNextY, aShip));
									mVisited[aShip->mNextX][aShip->mNextY] = true;
									mKills++;
									mKillsTotal++;
								}
								aAtFront = StepBack(mShips, anItr);
								SpawnExplosion(aShip->mNextX, aShip->mNextY, false);
								AddPoints(0x14, aShip->mNextX, aShip->mNextY);
								SetScoreCheck(1);
								RemoveShip(aShip);
								mKills++;
								mKillsTotal++;
							}
						}
						else if (aShip->mType != 2)
						{
							aAtFront = StepBack(mShips, anItr);
							SpawnExplosion(aNX, aNY, false);
							AddPoints(0x14, aShip->mNextX, aShip->mNextY);
							SetScoreCheck(3);
							RemoveShip(aShip);
							mKills++;
							mKillsTotal++;
						}
						else if (mVisited[aNX][aNY])
						{
							mApp->PlaySample(SOUND_MONSTER, Pan(aNX));
							mVisited[aShip->mNextX][aShip->mNextY] = false;
						}
					}
				}
			}
		}
		else
		{
			int aHeading = aShip->mHeading;
			int aWant = aShip->mWantHeading;
			if (aHeading < aWant)
			{
				if (aWant - aHeading < 8)
					aHeading = aHeading + 1;
				else
					aHeading = aHeading - 1;
			}
			else if (7 < aHeading - aWant)
				aHeading = aHeading + 1;
			else
				aHeading = aHeading - 1;
			aShip->mHeading = aHeading;
			if (0xf < aHeading)
				aShip->mHeading = 0;
			if (aShip->mHeading < 0)
				aShip->mHeading = 0xf;
		}
		if (aAtFront)
			anItr = mShips.begin();
		else
			++anItr;
	}
}

// 0x40a050: whether a cell lies on one of the two broadside lines (three cells long at the most) of a ship
bool Board::CanFireAt(int theX, int theY, int theHeading, int theTX, int theTY, int theRange)
{
	int aAX = 0;
	int aAY = 0;
	int aBX = 0;
	int aBY = 0;
	int aRangeA = 0;
	int aRangeB = 0;
	int aStep = 1;
	if ((theHeading == 0) || (theHeading == 8))
	{
		aAX = -1;
		aAY = 0;
		aBX = 1;
		aBY = 0;
	}
	else if ((theHeading == 2) || (theHeading == 10))
	{
		aAX = -1;
		aAY = -1;
		aBX = 1;
		aBY = 1;
	}
	else if ((theHeading == 4) || (theHeading == 0xc))
	{
		aAX = 0;
		aAY = -1;
		aBX = 0;
		aBY = 1;
	}
	else if ((theHeading == 6) || (theHeading == 0xe))
	{
		aAX = -1;
		aAY = 1;
		aBX = 1;
		aBY = -1;
	}
	if (0 < theRange)
	{
		int aScanAY = theY + aAY;
		int aScanBY = theY;
		int aScanBX = theX + aBX;
		int aScanAX = theX + aAX;
		do
		{
			aScanBY = aBY + aScanBY;
			if ((aRangeA == aStep - 1) && (-1 < aScanAX) && (aScanAX < 0xb) && (-1 < aScanAY) && (aScanAY < 0xb) &&
				(mTerrain[aScanAX][aScanAY] == 0))
			{
				aRangeA = aStep;
				if ((aScanAX == theTX) && (aScanAY == theTY))
					return true;
			}
			if ((aRangeB == aStep - 1) && (-1 < aScanBX) && (aScanBX < 0xb) && (-1 < aScanBY) && (aScanBY < 0xb) &&
				(mTerrain[aScanBX][aScanBY] == 0))
			{
				aRangeB = aStep;
				if ((aScanBX == theTX) && (aScanBY == theTY))
					return true;
			}
			aScanAX = aScanAX + aAX;
			aScanAY = aScanAY + aAY;
			aScanBX = aScanBX + aBX;
			aStep = aStep + 1;
		}
		while (aStep <= theRange);
	}
	return false;
}

// 0x405ca0: the whirlpools turn and appear
void Board::UpdatePorts()
{
	if (mUpdateCnt % 4 == 0)
	{
		for (std::list<Port*>::iterator anItr = mPorts.begin(); anItr != mPorts.end(); ++anItr)
		{
			Port* aPort = *anItr;
			if ((-1 < aPort->mAppear) || (mState == STATE_IDLE))
				aPort->mAppear = aPort->mAppear + 1;
			aPort->mFrame = (aPort->mFrame + 1) % 16;
		}
	}
}

// 0x405d00: now and then a whirlpool opens or closes (not under the ship, not on a wreck)
void Board::ToggleWhirls()
{
	for (std::list<Port*>::iterator anItr = mPorts.begin(); anItr != mPorts.end(); ++anItr)
	{
		Port* aPort = *anItr;
		if ((CrtRand() % 3 == 0) && ((aPort->mX != mPlayerX) || (aPort->mY != mPlayerY)) && !mVisited[aPort->mX][aPort->mY])
		{
			aPort->mActive = !aPort->mActive;
			if (aPort->mActive)
			{
				aPort->mFrame = 0;
				aPort->mAppear = 0;
			}
			else
				aPort->mAppear = -1;
			ClearUndo(3);
		}
	}
}

// 0x405d90: the points that rise from the cells where something was sunk
void Board::UpdateScoreTexts()
{
	if (mUpdateCnt % 3 == 0)
	{
		if (mDirty)
		{
			mDirty = false;
			int aX = 0;
			do
			{
				int aY = 0;
				do
				{
					if (0 < mPoints[aX][aY])
					{
						ScoreText* aText = new ScoreText();
						aText->mX = CellToPixel(aX);
						aText->mY = CellToPixel(aY);
						aText->mAlpha = 0x140;
						aText->mValue = mPoints[aX][aY];
						mScoreTexts.push_back(aText);
						mPoints[aX][aY] = 0;
					}
					aY++;
				}
				while (aY < 0xb);
				aX++;
			}
			while (aX < 0xb);
		}
		for (std::list<ScoreText*>::iterator anItr = mScoreTexts.begin(); anItr != mScoreTexts.end(); ++anItr)
		{
			(*anItr)->mAlpha = (*anItr)->mAlpha - 10;
			(*anItr)->mY = (*anItr)->mY - 1;
		}
	}
}

// 0x405ec0
void Board::UpdateCellFlash()
{
	int* aCell = &mCellFlash[0][0];
	int aRow = 0xb;
	do
	{
		int aCol = 0xb;
		do
		{
			if (0 < *aCell)
				*aCell = *aCell + 1;
			aCell++;
			aCol--;
		}
		while (aCol != 0);
		aRow--;
	}
	while (aRow != 0);
}

// 0x405f10: the two layers of water drift with a speed that wanders between 0.1 and 0.125
void Board::UpdateWaves()
{
	int aRand = CrtRand();
	if (aRand % 3 == 0)
		mWaveSpeedA = mWaveSpeedA + 0.04;
	else if (aRand % 3 == 1)
		mWaveSpeedA = mWaveSpeedA - 0.04;
	aRand = CrtRand();
	if (aRand % 3 == 0)
		mWaveSpeedB = mWaveSpeedB + 0.01;
	else if (aRand % 3 == 1)
		mWaveSpeedB = mWaveSpeedB - 0.01;
	if ((0.125 < mWaveSpeedA) || (0.1 <= mWaveSpeedA))
	{
		if (mWaveSpeedA > 0.125)
			mWaveSpeedA = 0.125;
	}
	else
		mWaveSpeedA = 0.1;
	if ((0.125 < mWaveSpeedB) || (0.1 <= mWaveSpeedB))
	{
		if (mWaveSpeedB > 0.125)
			mWaveSpeedB = 0.125;
	}
	else
		mWaveSpeedB = 0.1;
	mWaveA = mWaveA + mWaveSpeedA;
	if (mWaveA < 0.0)
		mWaveA = mWaveA + 500.0;
	if (500.0 <= mWaveA)
		mWaveA = mWaveA - 500.0;
	mWaveB = mWaveB + mWaveSpeedB;
	if (mWaveB < 0.0)
		mWaveB = mWaveB + 500.0;
	if (500.0 <= mWaveB)
		mWaveB = mWaveB - 500.0;
}
