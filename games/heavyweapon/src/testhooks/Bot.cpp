// port: the port's own bot (Bot.h). It aims at the boss while one can be hurt, otherwise at the live enemy nearest the
// tank across the field, and holds the button while it has a target. The tank then drives toward that enemy, since it
// drives where its gun points.

#include "Bot.h"
#include "Armory.h"
#include "Board.h"
#include "Craft.h"
#include "Enemy.h"
#include "HeavyWeaponApp.h"
#include "PowerUpDrops.h"
#include "Projectile.h"

#include <cmath>
#include <cstdlib>

namespace Sexy
{

namespace
{
	const int FIELD_HALF_WIDTH = 320;	// enemies' x is from the field's centre
	const int FIELD_WIDTH = 640;
	const int AIM_TOP = 10;				// the highest the pointer aims
	const int AIM_ABOVE_TANK = 40;		// the lowest it aims, above the tank's gun
	const int IDLE_AIM_Y = 200;			// with no target, the pointer above the tank, so it stops
	const double SHOT_SPEED = 5.0;		// the gun's bullets, in pixels an update (Board::AddGunShot)
	const double MUZZLE_ABOVE_TANK = 48.0;	// the turret's pivot (12) and the barrel (36) above the tank's y
	const int LEAD_PASSES = 3;			// the aim point refined this many times for the target's travel
	const double TANK_TOP = 30.0;		// a falling shot hits the tank this far above its y
	const double DANGER_HALF_WIDTH = 56.0;	// a shot landing this near the tank's x is dodged
	const double CAUTION_HALF_WIDTH = 120.0;	// and the tank never drives toward one landing this near
	const int DANGER_TIME = 200;		// and only one that lands within this many updates
	const int DODGE_DISTANCE = 220;		// the pointer this far to the side, low, so the tank drives off fast
	const int DODGE_ABOVE_TANK = 60;
	const int FIELD_EDGE = 280;			// a dodge never heads past this x (from the centre); it turns back instead
	// a bomb's fall (EnemyProjectiles.cpp, FallBomb): it gains 0.05 an update up to 2, and its sideways speed fades
	const double BOMB_GRAVITY = 0.05;
	const double BOMB_MAX_FALL = 2.0;
	const double BOMB_DRAG = 0.995;

	// When and where a shot reaches theTop: a bullet keeps its sideways speed and gains theGravity an update; a bomb
	// (isBomb) falls as a bomb does. False when it does not land within DANGER_TIME.
	bool Landing(double theX, double theY, double theVelX, double theVelY, double theGravity, bool isBomb, double theTop,
		int* theTime, double* theLandX)
	{
		double aX = theX;
		double aY = theY;
		double aVelX = theVelX;
		double aVelY = theVelY;
		for (int aTime = 1; aTime <= DANGER_TIME; aTime++)
		{
			aX += aVelX;
			aY += aVelY;
			if (isBomb)
			{
				aVelX *= BOMB_DRAG;
				if (aVelY < BOMB_MAX_FALL)
					aVelY += BOMB_GRAVITY;
			}
			else
				aVelY += theGravity;
			if (aY >= theTop)
			{
				*theTime = aTime;
				*theLandX = aX;
				return true;
			}
		}
		return false;
	}

	// The x of the soonest enemy shot or falling projectile that would land within theHalfWidth of the tank, or false
	// when none would
	bool FindThreat(Board* theBoard, double theHalfWidth, double* theLandX)
	{
		double aTop = (double)theBoard->mTankY - TANK_TOP;
		int aSoonest = DANGER_TIME + 1;
		for (ObjectList<Bullet>::Node* aNode = theBoard->mBullets.mHead->mNext; aNode != NULL; aNode = aNode->mNext)
		{
			Bullet* aBullet = aNode->mItem;
			int aTime;
			double aLandX;
			if (aBullet->mIsEnemy && aBullet->mY < aTop &&
				Landing(aBullet->mX, aBullet->mY, aBullet->mVelX, aBullet->mVelY, aBullet->mGravity, false, aTop, &aTime,
					&aLandX) &&
				fabs(aLandX - theBoard->mTankX) < theHalfWidth && aTime < aSoonest)
			{
				aSoonest = aTime;
				*theLandX = aLandX;
			}
		}
		for (ObjectList<Projectile>::Node* aNode = theBoard->mProjectiles.mHead->mNext; aNode != NULL; aNode = aNode->mNext)
		{
			Projectile* aProjectile = aNode->mItem;
			int aTime;
			double aLandX;
			if (!aProjectile->mIsInBackground && aProjectile->mY < aTop &&
				Landing(aProjectile->mX, aProjectile->mY, aProjectile->mVelX, aProjectile->mVelY, 0.0, true, aTop, &aTime,
					&aLandX) &&
				fabs(aLandX - theBoard->mTankX) < theHalfWidth && aTime < aSoonest)
			{
				aSoonest = aTime;
				*theLandX = aLandX;
			}
		}
		// a satellite charging or burning its beam, which kills a tank under it
		for (ObjectList<Enemy>::Node* aNode = theBoard->mEnemies.mHead->mNext; aNode != NULL; aNode = aNode->mNext)
		{
			Satellite* aSatellite = dynamic_cast<Satellite*>(aNode->mItem);
			if (aSatellite == NULL || aSatellite->mBeamTime <= 0 || fabs(aSatellite->mX - theBoard->mTankX) >= theHalfWidth)
				continue;
			int aTime = aSatellite->mBeamTime > Satellite::BEAM_BURN_START ?
				aSatellite->mBeamTime - Satellite::BEAM_BURN_START : 0;
			if (aTime < aSoonest)
			{
				aSoonest = aTime;
				*theLandX = aSatellite->mX;
			}
		}
		return aSoonest <= DANGER_TIME;
	}
}

bool ChooseBotMove(Board* theBoard, BotMove* theMove)
{
	if (theBoard == NULL || theBoard->mPaused || theBoard->mResultsScreen != NULL)
		return false;

	// while no tank is on the field (the wait before one, a tank lost) and while the boss dies, nothing is aimed at
	bool canAim = theBoard->mTankDeadTime == 0 && theBoard->mBossDeath == NULL;
	Enemy* aTarget = NULL;
	int aTargetIndex = -1;
	if (canAim && theBoard->mBoss != NULL && theBoard->mBoss->mIsAlive && !theBoard->mBossUntouchable)
	{
		aTarget = theBoard->mBoss;
		int anIndex = 0;
		for (ObjectList<Enemy>::Node* aNode = theBoard->mEnemies.mHead->mNext; aNode != NULL; aNode = aNode->mNext, anIndex++)
		{
			if (aNode->mItem == aTarget)
				aTargetIndex = anIndex;
		}
	}
	else if (canAim)
	{
		double aBestDistance = 0.0;
		int anIndex = 0;
		for (ObjectList<Enemy>::Node* aNode = theBoard->mEnemies.mHead->mNext; aNode != NULL; aNode = aNode->mNext, anIndex++)
		{
			Enemy* anEnemy = aNode->mItem;
			if (!anEnemy->mIsAlive || anEnemy->mIsFriendly || anEnemy == theBoard->mBoss)
				continue;
			if (anEnemy->mX < -FIELD_HALF_WIDTH || anEnemy->mX > FIELD_HALF_WIDTH || anEnemy->mY < 0.0)
				continue;
			double aDistance = fabs(anEnemy->mX - theBoard->mTankX);
			if (aTarget == NULL || aDistance < aBestDistance)
			{
				aTarget = anEnemy;
				aTargetIndex = anIndex;
				aBestDistance = aDistance;
			}
		}
	}

	int aLowest = theBoard->mTankY - AIM_ABOVE_TANK;
	if (aTarget == NULL)
	{
		theMove->mX = (int)theBoard->mTankX + FIELD_HALF_WIDTH;
		theMove->mY = IDLE_AIM_Y;
		theMove->mFire = false;
		theMove->mTarget = -1;
	}
	else
	{
		// where the target will be when a bullet reaches it, at its present speed
		double anAimX = aTarget->mX;
		double anAimY = aTarget->mY;
		for (int aPass = 0; aPass < LEAD_PASSES; aPass++)
		{
			double aDeltaX = anAimX - theBoard->mTankX;
			double aDeltaY = anAimY - ((double)theBoard->mTankY - MUZZLE_ABOVE_TANK);
			double aTime = sqrt(aDeltaX * aDeltaX + aDeltaY * aDeltaY) / SHOT_SPEED;
			anAimX = aTarget->mX + aTarget->mVelX * aTime;
			anAimY = aTarget->mY + aTarget->mVelY * aTime;
		}
		theMove->mX = (int)anAimX + FIELD_HALF_WIDTH;
		theMove->mY = (int)anAimY;
		theMove->mFire = true;
		theMove->mTarget = aTargetIndex;
	}
	// a shot about to land on the tank: the pointer goes low to the side away from it, so the tank drives clear; the
	// button stays as it was
	double aLandX;
	if (canAim && FindThreat(theBoard, DANGER_HALF_WIDTH, &aLandX))
	{
		int aDirection = (aLandX > theBoard->mTankX) ? -1 : 1;
		if (theBoard->mTankX + aDirection * DODGE_DISTANCE > FIELD_EDGE ||
			theBoard->mTankX + aDirection * DODGE_DISTANCE < -FIELD_EDGE)
			aDirection = -aDirection;
		theMove->mX = (int)theBoard->mTankX + aDirection * DODGE_DISTANCE + FIELD_HALF_WIDTH;
		theMove->mY = theBoard->mTankY - DODGE_ABOVE_TANK;
	}
	else if (canAim && FindThreat(theBoard, CAUTION_HALF_WIDTH, &aLandX) &&
		((theMove->mX - FIELD_HALF_WIDTH > theBoard->mTankX) == (aLandX > theBoard->mTankX)))
	{
		// one is coming down on the side the tank would drive to: the pointer straight above the tank, so it stops
		theMove->mX = (int)theBoard->mTankX + FIELD_HALF_WIDTH;
		theMove->mY = IDLE_AIM_Y;
	}
	if (theMove->mX < 0)
		theMove->mX = 0;
	if (theMove->mX > FIELD_WIDTH - 1)
		theMove->mX = FIELD_WIDTH - 1;
	if (theMove->mY < AIM_TOP)
		theMove->mY = AIM_TOP;
	if (theMove->mY > aLowest)
		theMove->mY = aLowest;
	return true;
}

int ChooseArmoryUpgrade(Armory* theArmory)
{
	if (theArmory->mPointsTotal - theArmory->mPointsSpent <= 0)
		return -1;
	int aBest = -1;
	for (int i = 0; i < theArmory->mNumPanels; i++)
	{
		int aLevel = theArmory->mApp->mWeapons[i];
		if (aLevel >= PowerUpDrops::MAX_WEAPON_LEVEL)
			continue;
		if (aBest < 0 || aLevel < theArmory->mApp->mWeapons[aBest])
			aBest = i;
	}
	return aBest;
}

}
