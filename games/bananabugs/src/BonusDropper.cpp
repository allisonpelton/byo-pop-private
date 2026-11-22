#include "BonusDropper.h"

#include "GameApp.h"
#include "LevelObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The depth a bonus that is put down is drawn at (design pixels' own units, 0x4151999a)
const float DROPPED_Z = 13.1f;

// The angle of the orbit for each way the bonus was turned (Bonus::mRotation 1 to 4), in radians (the same four as
// BonusBomb's)
const float DROPPED_ANGLE_UP = 0.0f;
const float DROPPED_ANGLE_RIGHT = 4.71238899f; // 0x4096cbe4
const float DROPPED_ANGLE_DOWN = 3.14159274f;  // 0x40490fdb
const float DROPPED_ANGLE_LEFT = 1.57079637f;  // 0x3fc90fdb

// 0x40db20: held only (state 2): it goes to the level's object layer without moving on the screen, takes the angle of the
// way it was turned and starts its drop track. (The bomb's Place, 0x4082c0, is the same without the depth.)
bool BonusDropper::Place()
{
	if (mState != BONUS_HELD)
		return false;
	sApp->mSoundQueue->Request(SOUND_COCOLETGO, 0, false);
	Player::Drop();
	SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
	SetState(STATE_Z, DROPPED_Z);
	float aY = (float)(((double)mDrawn[STATE_Y] - (double)mParent->mDrawn[STATE_Y]) * (double)sApp->mWindowToDesignY);
	float aX = (float)(((double)mDrawn[STATE_X] - (double)mParent->mDrawn[STATE_X]) * (double)sApp->mWindowToDesignX);
	SetState(STATE_X, aX);
	SetState(STATE_Y, aY);
	float anAngle;
	if (mRotation == 1)
		anAngle = DROPPED_ANGLE_UP;
	else if (mRotation == 2)
		anAngle = DROPPED_ANGLE_RIGHT;
	else if (mRotation == 3)
		anAngle = DROPPED_ANGLE_DOWN;
	else
		anAngle = DROPPED_ANGLE_LEFT;
	SetState(STATE_ORBIT_ANGLE, anAngle);
	ComputeDrawn();
	StartTrack(mDropTrack);
	return true;
}
