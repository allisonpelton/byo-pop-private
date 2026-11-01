// Board.cpp, part 3: the board's update (WinSS.exe 0x4060e0, 0x4035d0-0x405f10): the player's move, the
// cannonballs, the enemy ships and the small timers.

#include "Board.h"
#include "SevenSeasApp.h"
#include "Screens.h"
#include "Anims.h"
#include "Compat2004.h"
#include "KeyCodes.h"
#include "WidgetManager.h"

#include <cstdlib>

using namespace Sexy;

// the heading (0 up, 4 right, 8 down, 0xc left, 16 steps) for a move of (dx, dy); 0 when there is none. The
// original writes this out in each of the places that turn the ship.
static int HeadingFor(int theDX, int theDY)
{
	unsigned int aHeading = 0;
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
		aHeading = ((theDX < 1) - 1 & 0xfffffffe) + 8;
	return (int) aHeading;
}

// port: the original reads the keys' state (mKeyDown) every second update, so a press that ends before the next read is
// lost; its timer ran an update every 10 ms and a key is never down for less, but the port's frames can handle a key's press
// and release together after a hitch. A press is kept for the next read, which sees what the original would have seen
// had the press come a little earlier.
void Board::KeyDown(KeyCode theKey)
{
	Widget::KeyDown(theKey);
	if ((theKey >= 0) && (theKey < 0xFF))
		mKeyTap[theKey] = true;
}

void Board::GetKeys(bool* theKeys)
{
	for (int aKey = 0; aKey < 0xFF; aKey++)
		theKeys[aKey] = mWidgetManager->mKeyDown[aKey] || mKeyTap[aKey];
}

// 0x4060e0
void Board::Update()
{
	Widget::Update();
	UpdateWaves();
	UpdateAnims();
	UpdatePorts();
	UpdateScoreTexts();
	UpdateCellFlash();
	if (mState == STATE_LEVEL_END)
	{
		if (mUpdateCnt - mGameStartCnt <= 300)
		{
			mWidgetManager->BringToFront(mApp->mTalkWidget);
			mApp->mTalking = true;
		}
		else
		{
			mDirty = true;
			// the score check of the internet high scores is closed here (not ported)
			mApp->ShowHighScoreWidget();
		}
	}
	else
	{
		UpdateMove();
		UpdateShots();
		UpdateEnemies();
		if (mLevelComplete && !mApp->mTutorial)
		{
			mApp->LevelComplete();
			ClearCellFlash();
			mDirty = true;
		}
	}
	mHintTimer -= 2;
	if (mBonus)
	{
		if (mBonusState == 1)
		{
			if (mUpdateCnt % 10 == 0)
				SpawnSparkle(CellToPixel(mTreasureX + 1), CellToPixel(mTreasureY + 1));
			if (mUpdateCnt - mBonusTick == 0x32)
			{
				mTreasures++;
				AddPoints(mTreasures * 200, mTreasureX + 1, mTreasureY + 1);
				SetScoreCheck(8);
				mDirty = true;
			}
		}
		else if ((mBonusState == 2) && (mUpdateCnt - mBonusTick2 == 0x96))
		{
			mApp->SetMusicOrder(0x12, false);
			mApp->SetMusicOrder(0x12, false);
		}
	}
	if (mApp->mTalking)
	{
		TalkWidget* aTalk = mApp->mTalkWidget;
		if ((aTalk->mMode == 1) && (aTalk->mProgress == 1.0))
		{
			if (mState == STATE_LEVEL_END)
			{
				int aChoice = CrtRand() % 4;
				if (aChoice == 0)
				{
					aTalk->SetMouth(1);
					mApp->PlaySample(0xe);
				}
				else if (aChoice == 1)
				{
					aTalk->SetMouth(4);
					mApp->PlaySample(0x10);
				}
				else if (aChoice == 2)
				{
					aTalk->SetMouth(3);
					mApp->PlaySample(0x11);
				}
				else if (aChoice == 3)
				{
					aTalk->SetMouth(2);
					mApp->PlaySample(0x12);
				}
			}
			aTalk->mMode = 0;
		}
		if ((aTalk->mMode == -1) && (aTalk->mProgress == 0.0))
		{
			aTalk->mMode = 0;
			mApp->mTalking = false;
		}
	}
	if (700 < mUpdateCnt)
		mTutorialShown = false;
	if (mUpdateCnt % 2 == 0)
	{
		MarkDirty();
		// the poll of this update has seen the presses
		for (int aKey = 0; aKey < 0xFF; aKey++)
			mKeyTap[aKey] = false;
	}
}

// 0x4035d0: the player's ship (keys, turning, sailing, the whirlpool, the new position)
void Board::UpdateMove()
{
	if ((mState != STATE_WHIRL_IN) && (mState != STATE_WHIRL_OUT))
		mShipAnim++;
	if (0 < mLifeFlash)
		mLifeFlash--;
	if (mUpdateCnt % 2 != 0)
		return;
	if (mUpdateCnt % 4 == 0)
	{
		for (int i = 0; i < 9; i++)
		{
			if (-1 < mGunFlash[i])
				mGunFlash[i]--;
		}
	}
	if (!mLevelOver || !AllPortsDone())
	{
		if (mState == STATE_NO_LIVES_FLASH)
		{
			if (0x11 < mTimer)
			{
				mTimer = 0;
				mState = STATE_BLINK;
				mBlinkCount = 0;
				PlaceShip();
				mApp->PlaySample(SOUND_REAPPEAR, Pan(mPlayerX));
				mBlink = !mBlink;
			}
			else
				mTimer++;
		}
		else if (mState == STATE_BLINK)
		{
			mTimer++;
			if (mTimer % 0xf == 0)
			{
				mBlink = !mBlink;
				if (!mBlink)
				{
					mApp->PlaySample(SOUND_REAPPEAR, Pan(mPlayerX));
					mBlinkCount++;
				}
			}
			if (mBlinkCount == 2)
			{
				mBlink = false;
				mState = STATE_IDLE;
				mDirty = true;
			}
		}
	}
	else if (mLives < 1)
	{
		GameOver();
	}
	else
	{
		mLevelOver = false;
		bool aMonster = false;
		int aCount = 0;
		for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
		{
			if ((*anItr)->mType == 2)
				aMonster = true;
			aCount++;
		}
		if (((mShips.size() == 0) || ((aCount == 1) && aMonster)) && !mApp->mTutorial)
		{
			ClearCellFlash();
			mApp->LevelComplete();
		}
		else
		{
			mState = STATE_NO_LIVES_FLASH;
			mTimer = 0;
			mBlink = !mBlink;
		}
	}

	int aState = mState;
	if (((aState == STATE_IDLE) || (aState == STATE_BLINK)) && !mLevelOver && (mApp->mTrialWidget == NULL))
	{
		int aDX = 0;
		int aDY = 0;
		if ((mApp->GetDialogCount() == 0) && !mApp->mTutorial && !mBonus && (mApp->mMapWidget == NULL))
		{
			bool aKeys[0xFF];
			GetKeys(aKeys);
			if (aKeys[KEYCODE_LEFT] || aKeys[KEYCODE_NUMPAD1] || aKeys[KEYCODE_NUMPAD4] || aKeys[KEYCODE_NUMPAD7] ||
				aKeys[KEYCODE_END] || aKeys[KEYCODE_HOME])
				aDX = -1;
			if (aKeys[KEYCODE_RIGHT] || aKeys[KEYCODE_NUMPAD3] || aKeys[KEYCODE_NUMPAD6] || aKeys[KEYCODE_NUMPAD9] ||
				aKeys[KEYCODE_PRIOR] || aKeys[KEYCODE_NEXT])
				aDX = aDX + 1;
			if (aKeys[KEYCODE_UP] || aKeys[KEYCODE_NUMPAD7] || aKeys[KEYCODE_NUMPAD8] || aKeys[KEYCODE_NUMPAD9] ||
				aKeys[KEYCODE_HOME] || aKeys[KEYCODE_PRIOR])
				aDY = -1;
			if (aKeys[KEYCODE_DOWN] || aKeys[KEYCODE_NUMPAD1] || aKeys[KEYCODE_NUMPAD2] || aKeys[KEYCODE_NUMPAD3] ||
				aKeys[KEYCODE_END] || aKeys[KEYCODE_NEXT])
				aDY = aDY + 1;
			int aNewX = mPlayerX + aDX;
			if (((aDX != 0) || (aDY != 0)) && (-1 < aNewX) && (aNewX < 0xb))
			{
				int aNewY = mPlayerY + aDY;
				if ((-1 < aNewY) && (aNewY < 0xb) && CellFree(aNewX, aNewY))
				{
					int aHeading = HeadingFor(aDX, aDY);
					SaveUndo();
					mWantHeading = aHeading;
					mDestX = mPlayerX + aDX;
					mBlink = false;
					mDestY = aDY + mPlayerY;
					mState = STATE_TURN_MOVE;
					mDirty = true;
					mApp->PlaySample(SOUND_MOVEGOOD, Pan(mDestX));
					return;
				}
			}
		}
		bool aKeys[0xFF];
		GetKeys(aKeys);
		if (((mApp->GetDialogCount() != 0) || (mApp->mMapWidget != NULL) || mApp->mTutorial || mBonus ||
			!aKeys[KEYCODE_SPACE]) && !aKeys[KEYCODE_NUMPAD5] && !aKeys[KEYCODE_NUMPAD0] && !aKeys[KEYCODE_INSERT])
			return;
		SaveUndo();
		mBlink = false;
		FirePlayerShots();
		return;
	}

	int aHeading = mHeading;
	int aWant = mWantHeading;
	if (aHeading == aWant)
	{
		if (aState == STATE_TURN_MOVE)
		{
			mMoveProgress = 0;
			mState = STATE_SAIL;
			mDirty = true;
			return;
		}
	}
	else if (aState == STATE_TURN_MOVE)
	{
		mTutorialShown = false;
		if (aHeading < aWant)
		{
			if (7 < aWant - aHeading)
				aHeading = aHeading - 1;
			else
				aHeading = aHeading + 1;
		}
		else if (aHeading - aWant < 8)
			aHeading = aHeading - 1;
		else
			aHeading = aHeading + 1;
		mHeading = aHeading;
		if (0xf < aHeading)
			mHeading = 0;
		if (-1 < mHeading)
			return;
		mHeading = 0xf;
		return;
	}

	if (aState == STATE_SAIL)
	{
		if (mMoveProgress < 0x2a)
		{
			if (mMoveProgress % 6 == 0)
			{
				int aX;
				int aY;
				if ((aHeading == 4) || (aHeading == 0xc))
				{
					aY = CellToPixel(mPlayerY) + 5 + (mDestY - mPlayerY) * (mMoveProgress - 8);
					aX = CellToPixel(mPlayerX) + (mDestX - mPlayerX) * (mMoveProgress - 8);
					SpawnWake(aX, aY);
					aY = CellToPixel(mPlayerY) + 5 + (mDestY - mPlayerY) * (mMoveProgress - 9);
					aX = CellToPixel(mPlayerX) + (mDestX - mPlayerX) * (mMoveProgress - 9);
				}
				else
				{
					aY = CellToPixel(mPlayerY) + (mDestY - mPlayerY) * (mMoveProgress - 8);
					aX = CellToPixel(mPlayerX) + (mDestX - mPlayerX) * (mMoveProgress - 8);
					SpawnWake(aX, aY);
					aY = CellToPixel(mPlayerY) + (mDestY - mPlayerY) * (mMoveProgress - 9);
					aX = CellToPixel(mPlayerX) + (mDestX - mPlayerX) * (mMoveProgress - 9);
				}
				SpawnWake(aX, aY);
			}
			mMoveProgress = mMoveProgress + 3;
			return;
		}
		mPlayerX = mDestX;
		mPlayerY = mDestY;
		mMoveProgress = 0;
		for (std::list<Port*>::iterator anItr = mPorts.begin(); anItr != mPorts.end(); ++anItr)
		{
			Port* aPort = *anItr;
			if ((aPort->mX == mPlayerX) && (aPort->mY == mPlayerY) && aPort->mActive)
			{
				ClearUndo(1);
				mState = STATE_WHIRL_OUT;
				mDirty = true;
				mApp->PlaySample(SOUND_WHIRLOUT, Pan(mPlayerX));
				mShipAnim = 0;
			}
		}
		if (mState == STATE_WHIRL_OUT)
			return;
	}
	else if (aState == STATE_WHIRL_OUT)
	{
		if (aHeading == 0)
			mShipAnim++;
		else
			mHeading = (aHeading + 1) % 16;
		if (mShipAnim < 0x1e)
			return;
		PlaceShip();
		mHeading = 0;
		mShipAnim = 0x1e;
		mState = STATE_WHIRL_IN;
		mDirty = true;
		mApp->PlaySample(SOUND_WHIRLIN, Pan(mPlayerX));
		return;
	}
	else
	{
		if (aState != STATE_WHIRL_IN)
			return;
		if (0 < mShipAnim)
		{
			mShipAnim--;
			return;
		}
		if (aHeading != 4)
		{
			mHeading = aHeading - 1;
			if (-1 < aHeading - 1)
				return;
			mHeading = 0xf;
			return;
		}
	}
	PlanEnemyMoves();
}

// 0x403e80: the cannonballs fly (the player's in state 7, the enemies' in state 8)
void Board::UpdateShots()
{
	if (mState == STATE_PLAYER_SHOTS)
	{
		std::list<Shot*>::iterator anItr = mShots.begin();
		while (anItr != mShots.end())
		{
			Shot* aShot = *anItr;
			if (aShot->mOwner == 0)
			{
				bool aHitMonster = false;
				if ((aShot->mRange < 2) || (4 < aShot->mFrame))
				{
					if (aShot->mRange == 1)
						aShot->mFrame = aShot->mFrame - 1;
				}
				else
					aShot->mFrame = aShot->mFrame + 1;
				if ((aShot->mProgress == 0) && (ShipAt(aShot->mX, aShot->mY, NULL) != NULL))
				{
					Ship* aTarget = ShipAt(aShot->mX, aShot->mY, NULL);
					if (aTarget->mType == 2)
					{
						aHitMonster = true;
						mApp->PlaySample(SOUND_MONSTER, Pan(aShot->mX));
					}
					else
					{
						mVisited[aShot->mX][aShot->mY] = true;
						SpawnExplosion(aShot->mX, aShot->mY, true);
						int aType = aTarget->mType;
						RemoveShip(ShipAt(aShot->mX, aShot->mY, NULL));
						mKills++;
						mKillsTotal++;
						switch (aType)
						{
						case 0:
							AddPoints(0x1e, aShot->mX, aShot->mY);
							SetScoreCheck(4);
							break;
						case 1:
							AddPoints(0x28, aShot->mX, aShot->mY);
							SetScoreCheck(5);
							break;
						case 3:
							AddPoints(0x32, aShot->mX, aShot->mY);
							SetScoreCheck(6);
							break;
						case 4:
							AddPoints(0x3c, aShot->mX, aShot->mY);
							SetScoreCheck(7);
						}
					}
					aShot->mRange = 0;
				}
				int aProgress = aShot->mProgress;
				if (((aProgress < 0x2a) && (1 < aShot->mRange)) || ((aProgress < 0x14) && (aShot->mRange < 2)))
				{
					aShot->mProgress = aProgress + 2;
					++anItr;
					continue;
				}
				int aX = aShot->mX + aShot->mDX;
				aShot->mX = aX;
				int aDY = aShot->mDY;
				aShot->mProgress = 0;
				int aY = aShot->mY + aDY;
				aShot->mRange = aShot->mRange - 1;
				aShot->mY = aY;
				if (0 < aShot->mRange)
				{
					++anItr;
					continue;
				}
				if (!aHitMonster)
				{
					aX = aX - aShot->mDX;
					if (!mVisited[aX][aY - aDY])
						SpawnSplash(aX, aY - aDY);
				}
				delete aShot;
				anItr = mShots.erase(anItr);
			}
			else
				++anItr;
		}
		if (mShips.size() == 0)
		{
			if (mShots.size() != 0)
				return;
			if ((0 < mLives) && !mApp->mTutorial)
			{
				mApp->LevelComplete();
				ClearCellFlash();
				return;
			}
		}
		if ((mShots.size() == 0) && (0 < mLives))
		{
			PlanEnemyMoves();
			return;
		}
	}
	else if (mState == STATE_ENEMY_SHOTS)
	{
		std::list<Shot*>::iterator anItr = mShots.begin();
		while (anItr != mShots.end())
		{
			Shot* aShot = *anItr;
			bool aHitPlayer = false;
			if (aShot->mOwner == 1)
			{
				bool aHitMonster = false;
				if ((aShot->mRange < 2) || (4 < aShot->mFrame))
				{
					if (aShot->mRange == 1)
						aShot->mFrame = aShot->mFrame - 1;
				}
				else
					aShot->mFrame = aShot->mFrame + 1;
				if (aShot->mProgress == 0)
				{
					if (ShipAt(aShot->mX, aShot->mY, NULL) == NULL)
					{
						int aX = aShot->mX;
						if ((aX == mPlayerX) && (aShot->mY == mPlayerY))
						{
							mVisited[aX][aShot->mY] = true;
							SpawnExplosion(aShot->mX, aShot->mY, true);
							LoseLife();
							aShot->mRange = 0;
							aHitPlayer = true;
						}
					}
					else
					{
						Ship* aTarget = ShipAt(aShot->mX, aShot->mY, NULL);
						if (aTarget->mType == 2)
						{
							mApp->PlaySample(SOUND_MONSTER, Pan(aShot->mX));
							aHitMonster = true;
							aShot->mRange = 0;
						}
						else
						{
							mVisited[aShot->mX][aShot->mY] = true;
							SpawnExplosion(aShot->mX, aShot->mY, true);
							RemoveShip(ShipAt(aShot->mX, aShot->mY, NULL));
							aShot->mRange = 0;
						}
					}
				}
				int aProgress = aShot->mProgress;
				bool aKeep = false;
				if (((aProgress < 0x2a) && (1 < aShot->mRange)) || ((aProgress < 0x14) && (aShot->mRange < 2)))
				{
					aShot->mProgress = aProgress + 2;
					aKeep = !aHitPlayer;
				}
				else
				{
					aShot->mProgress = 0;
					int aX = aShot->mX + aShot->mDX;
					aShot->mX = aX;
					int aDY = aShot->mDY;
					int aY = aShot->mY + aDY;
					aShot->mRange = aShot->mRange - 1;
					aShot->mY = aY;
					if (0 < aShot->mRange)
						aKeep = !aHitPlayer;
					else if (!aHitMonster)
					{
						aX = aX - aShot->mDX;
						if (!mVisited[aX][aY - aDY])
							SpawnSplash(aX, aY - aDY);
					}
				}
				if (aKeep)
				{
					++anItr;
					continue;
				}
				delete aShot;
				anItr = mShots.erase(anItr);
			}
			else
				++anItr;
		}
		if (mShips.size() == 0)
		{
			if (mShots.size() != 0)
				return;
			if ((0 < mLives) && !mLevelOver && !mApp->mTutorial)
			{
				mApp->LevelComplete();
				ClearCellFlash();
				return;
			}
		}
		if ((mShots.size() == 0) && (0 < mLives))
		{
			mDirty = true;
			mState = STATE_IDLE;
			ClearCellFlash();
			if (mApp->mTutorial && ((mTutorialStep == 2) || (mTutorialStep == 10)))
				mCellFlash[mPlayerX][mPlayerY] = 1;
		}
	}
}

// 0x404430: the running animations step to their next frame and end
void Board::UpdateAnims()
{
	std::list<AnimInstance*>::iterator anItr = mAnims.begin();
	while (anItr != mAnims.end())
	{
		AnimInstance* anAnim = *anItr;
		anAnim->mDelay = anAnim->mDelay - 1;
		if (anAnim->mDelay < 1)
		{
			AnimDef* aDef = anAnim->mDef;
			int aNumFrames = (int) aDef->mFrames.size();
			anAnim->mFrameIdx = anAnim->mFrameIdx + 1;
			if (anAnim->mFrameIdx < aNumFrames)
				anAnim->mDelay = aDef->mFrames[anAnim->mFrameIdx].mDelay;
			else if (aDef->mLoop)
			{
				anAnim->mFrameIdx = 0;
				anAnim->mDelay = aDef->mFrames[0].mDelay;
			}
			else
			{
				delete anAnim;
				anItr = mAnims.erase(anItr);
				continue;
			}
		}
		++anItr;
	}
}

// 0x4044d0: the player's broadside, to both sides of the ship, as far as the water goes (three cells)
void Board::FirePlayerShots()
{
	int aHeading = mHeading;
	int aAX = 0;
	int aAY = 0;
	int aBX = 0;
	int aBY = 0;
	int aRangeA = 0;
	int aRangeB = 0;
	if ((aHeading == 0) || (aHeading == 8))
	{
		aAX = -1;
		aAY = 0;
		aBX = 1;
		aBY = 0;
	}
	else if ((aHeading == 2) || (aHeading == 10))
	{
		aAX = -1;
		aAY = -1;
		aBX = 1;
		aBY = 1;
	}
	else if ((aHeading == 4) || (aHeading == 0xc))
	{
		aAX = 0;
		aAY = -1;
		aBX = 0;
		aBY = 1;
	}
	else if ((aHeading == 6) || (aHeading == 0xe))
	{
		aAX = -1;
		aAY = 1;
		aBX = 1;
		aBY = -1;
	}
	int aScanBY = mPlayerY;
	int aScanAY = mPlayerY + aAY;
	int aScanBX = mPlayerX + aBX;
	int aScanAX = mPlayerX + aAX;
	int aStep = 1;
	do
	{
		aScanBY = aBY + aScanBY;
		if (((aRangeA == aStep - 1) && (-1 < aScanAX)) && (aScanAX < 0xb) && (-1 < aScanAY) && (aScanAY < 0xb) &&
			(mTerrain[aScanAX][aScanAY] == 0))
			aRangeA = aStep;
		if (((aRangeB == aStep - 1) && (-1 < aScanBX)) && (aScanBX < 0xb) && (-1 < aScanBY) && (aScanBY < 0xb) &&
			(mTerrain[aScanBX][aScanBY] == 0))
			aRangeB = aStep;
		aStep = aStep + 1;
		aScanAX = aScanAX + aAX;
		aScanAY = aScanAY + aAY;
		aScanBX = aScanBX + aBX;
	}
	while (aStep < 4);
	if (0 < aRangeA)
	{
		Shot* aShot = new Shot();
		aShot->mX = mPlayerX + aAX;
		aShot->mY = mPlayerY + aAY;
		aShot->mDX = aAX;
		aShot->mDY = aAY;
		aShot->mRange = aRangeA;
		aShot->mOwner = 0;
		aShot->mProgress = 0;
		if (aRangeA < 2)
			aShot->mFrame = 0;
		else
			aShot->mFrame = -7;
		mGunFlash[(aAY + aAX * 3) + 4] = 0xe;
		mShots.push_back(aShot);
	}
	if (0 < aRangeB)
	{
		Shot* aShot = new Shot();
		aShot->mX = mPlayerX + aBX;
		aShot->mY = mPlayerY + aBY;
		aShot->mDX = aBX;
		aShot->mDY = aBY;
		aShot->mRange = aRangeB;
		aShot->mOwner = 0;
		aShot->mProgress = 0;
		if (aRangeB < 2)
			aShot->mFrame = 0;
		else
			aShot->mFrame = -7;
		mGunFlash[(aBY + aBX * 3) + 4] = 0xe;
		mShots.push_back(aShot);
	}
	if (mShots.size() != 0)
		mApp->PlaySample(SOUND_BOOM, Pan(mPlayerX));
	mState = STATE_PLAYER_SHOTS;
	mDirty = true;
}

// 0x404820: the blue pirates fire, then the enemy cannonballs fly
void Board::EnemyShots()
{
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
	{
		if ((*anItr)->mType == 3)
			EnemyFire(*anItr);
	}
	mState = STATE_ENEMY_SHOTS;
	int aDifficulty = mApp->mDifficulty;
	if (((aDifficulty == 2) && (10 < mLevel)) || ((aDifficulty == 3) && !mApp->mTutorial))
		ToggleWhirls();
	mDirty = true;
}

// 0x404890: a blue pirate's broadside (the same as the player's, with the ship's own flash timers)
void Board::EnemyFire(Ship* theShip)
{
	int aHeading = theShip->mHeading;
	int aAX = 0;
	int aAY = 0;
	int aBX = 0;
	int aBY = 0;
	int aRangeA = 0;
	int aRangeB = 0;
	if ((aHeading == 0) || (aHeading == 8))
	{
		aAX = -1;
		aAY = 0;
		aBX = 1;
		aBY = 0;
	}
	else if ((aHeading == 2) || (aHeading == 10))
	{
		aAX = -1;
		aAY = -1;
		aBX = 1;
		aBY = 1;
	}
	else if ((aHeading == 4) || (aHeading == 0xc))
	{
		aAX = 0;
		aAY = -1;
		aBX = 0;
		aBY = 1;
	}
	else if ((aHeading == 6) || (aHeading == 0xe))
	{
		aAX = -1;
		aAY = 1;
		aBX = 1;
		aBY = -1;
	}
	int aScanAY = theShip->mY;
	int aScanBX = theShip->mX + aBX;
	int aScanAX = theShip->mX + aAX;
	int aScanBY = aScanAY;
	int aStep = 1;
	do
	{
		aScanBY = aBY + aScanBY;
		aScanAY = aScanAY + aAY;
		if (((aRangeA == aStep - 1) && (-1 < aScanAX)) && (aScanAX < 0xb) && (-1 < aScanAY) && (aScanAY < 0xb) &&
			(mTerrain[aScanAX][aScanAY] == 0))
			aRangeA = aStep;
		if (((aRangeB == aStep - 1) && (-1 < aScanBX)) && (aScanBX < 0xb) && (-1 < aScanBY) && (aScanBY < 0xb) &&
			(mTerrain[aScanBX][aScanBY] == 0))
			aRangeB = aStep;
		aStep = aStep + 1;
		aScanAX = aScanAX + aAX;
		aScanBX = aScanBX + aBX;
	}
	while (aStep < 4);
	if (0 < aRangeA)
	{
		Shot* aShot = new Shot();
		aShot->mX = theShip->mX + aAX;
		aShot->mY = theShip->mY + aAY;
		aShot->mDX = aAX;
		aShot->mDY = aAY;
		aShot->mRange = aRangeA;
		aShot->mOwner = 1;
		aShot->mProgress = 0;
		if (aRangeA < 2)
			aShot->mFrame = 0;
		else
			aShot->mFrame = -7;
		theShip->mTrail[(aAY + aAX * 3) + 4] = 0xe;
		mShots.push_back(aShot);
	}
	if (0 < aRangeB)
	{
		Shot* aShot = new Shot();
		aShot->mX = theShip->mX + aBX;
		aShot->mY = theShip->mY + aBY;
		aShot->mDX = aBX;
		aShot->mDY = aBY;
		aShot->mRange = aRangeB;
		aShot->mOwner = 1;
		aShot->mProgress = 0;
		if (aRangeB < 2)
			aShot->mFrame = 0;
		else
			aShot->mFrame = -7;
		theShip->mTrail[(aBY + aBX * 3) + 4] = 0xe;
		mShots.push_back(aShot);
	}
	if (mShots.size() != 0)
		mApp->PlaySample(SOUND_BOOM, Pan(theShip->mX));
}

// 0x405660: the distance map from a cell (a flood fill: the cells set to 100 are walls)
void Board::Flood(int theX, int theY)
{
	int aNext = mDist[theX][theY] + 1;
	int aNX = theX - 1;
	int aCount = 3;
	do
	{
		int aDY = -1;
		do
		{
			int aNY = theY + aDY;
			if (((-1 < aNX) && (aNX < 0xb)) && (-1 < aNY) && (aNY < 0xb))
			{
				int aValue = mDist[aNX][aNY];
				if ((aNext < aValue) && (aValue != 100))
				{
					mDist[aNX][aNY] = aNext;
					Flood(aNX, aNY);
				}
			}
			aDY = aDY + 1;
		}
		while (aDY < 2);
		aNX = aNX + 1;
		aCount--;
	}
	while (aCount != 0);
}
