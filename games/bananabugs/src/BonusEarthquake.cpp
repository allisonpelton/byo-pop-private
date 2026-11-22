#include "BonusEarthquake.h"

#include "Anim.h"
#include "BackDrop.h"
#include "Board.h"
#include "BugObj.h"
#include "GameApp.h"
#include "GameWidget.h"
#include "LevelObj.h"
#include "PipeObj.h"
#include "Player.h"
#include "Res.h"
#include "ScoreBoard.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

// The key event of a rock's last key (the base's events 1 and 2 are not used by the quake)
const int QUAKE_EVENT_ROCK_LANDED = 4;

const int QUAKE_NO_COUNTDOWN = -99;
const int QUAKE_FRAMES = 0x25d;            // 605: how long it shakes
const int QUAKE_AFTERSHOCK_EVERY = 200;    // frames between the bugs that die
const int QUAKE_AFTERSHOCKS = 3;           // at most
const int QUAKE_ROCKS_LEFT_AT_END = 25;    // what the rocks that are left are counted from when End is called
const int QUAKE_Z = 20;                    // the quake's own object (a picture of nothing) and its size
const float QUAKE_SCALE = 2.0f;
const float QUAKE_SLOW_FRAMES = 1;         // updates the bugs skip between two they run: every other frame

// The shake: the score board moves up to 4 pixels each way on one of the frames, the backdrop up to 2 on the others
const int QUAKE_BOARD_RANGE_X = 5;         // the draw is of 5 values, each twice over, 4 down
const int QUAKE_BOARD_RANGE_Y = 3;
const int QUAKE_BOARD_BIAS = 4;
const int QUAKE_BACKDROP_RANGE = 5;
const int QUAKE_BACKDROP_BIAS = 2;

// The rocks fall in from above the screen (design pixels): the first from 650 above, each next 128 higher. They start
// at a place 1920 wide to either side of the centre-left and come down by their orbit's radius (8 to 15 a frame)
const int QUAKE_ROCK_START_Y = -650;
const int QUAKE_ROCK_STEP_Y = -128;
const int QUAKE_ROCK_X_RANGE = 0x781;      // the first start: 1921 places from 960 left of the centre
const int QUAKE_ROCK_X_BIAS = 0x3c0;
const int QUAKE_ROCK_RESTART_X_RANGE = 0xa3d; // a rock that landed starts again from 2621 places, 1920 left of the centre
const int QUAKE_ROCK_RESTART_X_BIAS = 0x780;
const float QUAKE_ROCK_RESTART_Y = -650.0f;
const float QUAKE_ROCK_ANGLE = 4.71238899f; // 0x4096cbe4: straight down (the orbit's angle, three quarters round)
const int QUAKE_ROCK_FIRST_RADIUS = 8;
const int QUAKE_ROCK_STEPS = 8;            // the rocks' depth, size and radius repeat every 8
const int QUAKE_ROCK_CELS = 0x33;          // a rock's picture: one of the first 16 cels of the pipes' picture, or one of 12 to 15
const int QUAKE_ROCK_CEL_LIMIT = 0xf;
const int QUAKE_ROCK_LAST_CELS = 4;
const int QUAKE_ROCK_LAST_CELS_FIRST = 0xc;
const int QUAKE_ROCK_FALL_FRAMES = 300;
const float QUAKE_ROCK_FADE_START = 0.2f;
const float QUAKE_ROCK_LAND_ALPHA = 0.1f;
const float QUAKE_ROCK_LAND_RADIUS = 16.0f;
const float QUAKE_ROCK_END_Y = 1000.0f;    // the fall's key ends when the rock's value passes this

// 0x40b7c0
BonusEarthquake::BonusEarthquake()
{
	for (int i = 0; i < QUAKE_ROCKS; i++)
		mRocks[i] = nullptr;
	mEnded = false;
	mCountdown = 0;
	for (int i = 0; i < 3; i++)
		mUnused344[i] = 0;
	mRocksLeft = 0;
	mSavedX = 0.0f;
	mSavedY = 0.0f;
	mAftershocks = 0;
}

// 0x40b7d0
void BonusEarthquake::Setup(int theKind)
{
	Bonus::Setup(theKind);
	mCountdown = QUAKE_NO_COUNTDOWN;
	mEnded = false;
	mRocksLeft = 0;
	mAftershocks = 0;
	BuildRocks();
}

// 0x40b810: each frame the score board or the backdrop is shaken (in turn, by the countdown's being even or odd); every
// 200 frames of the countdown, up to three times, a bug that does not fly dies with its column; at the end both are
// put back and the bonus ends
bool BonusEarthquake::Update()
{
	if (!Bonus::Update())
		return false;
	if (mCountdown == QUAKE_NO_COUNTDOWN)
		return true;
	ObjBase* aBackDrop = sApp->mGameWidget->mBoard->mBackDrop;
	ScoreBoard* aScoreBoard = Player::sCurrent->mScoreBoard;
	if (mCountdown % 2 == 0) {
		int aShakeX = (int)(GameRandomNext() % QUAKE_BOARD_RANGE_X) * 2 - QUAKE_BOARD_BIAS;
		aScoreBoard->SetState(STATE_X, (float)((double)aShakeX + (double)mSavedX));
		int aShakeY = (int)(GameRandomNext() % QUAKE_BOARD_RANGE_Y) * 2 - QUAKE_BOARD_BIAS;
		aScoreBoard->SetState(STATE_Y, (float)((double)aShakeY + (double)mSavedY));
	} else {
		aBackDrop->mState[STATE_X] = (float)((int)(GameRandomNext() % QUAKE_BACKDROP_RANGE) - QUAKE_BACKDROP_BIAS);
		aBackDrop->mState[STATE_Y] = (float)((int)(GameRandomNext() % QUAKE_BACKDROP_RANGE) - QUAKE_BACKDROP_BIAS);
	}
	if (mCountdown > 0) {
		mCountdown--;
		if (mCountdown % QUAKE_AFTERSHOCK_EVERY == 0 && mAftershocks < QUAKE_AFTERSHOCKS) {
			mAftershocks++;
			BugObj* aBug = BugObj::PickPeaTarget();
			if (aBug != nullptr && !aBug->mAirborne)
				aBug->Zap(true, true, false);
		}
	} else if (mCountdown == 0) {
		mCountdown = QUAKE_NO_COUNTDOWN;
		aBackDrop->mState[STATE_X] = 0.0f;
		aBackDrop->mState[STATE_Y] = 0.0f;
		aScoreBoard->SetState(STATE_X, mSavedX);
		aScoreBoard->SetState(STATE_Y, mSavedY);
		End();
	}
	return true;
}

// 0x40ba10: a rock has landed (its last key, event 4). While the quake runs it starts again from above the screen at a
// random place along it; once the quake has ended it is put away, and the bonus when only one is left
void BonusEarthquake::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	if (theObj->mCurrentKey->mEventId != QUAKE_EVENT_ROCK_LANDED) {
		Bonus::Notify(theObj);
		return;
	}
	if (!mEnded) {
		theObj->mState[STATE_X] = (float)((int)(GameRandomNext() % QUAKE_ROCK_RESTART_X_RANGE) - QUAKE_ROCK_RESTART_X_BIAS);
		theObj->mState[STATE_Y] = QUAKE_ROCK_RESTART_Y;
	} else {
		Deactivate(theObj);
		mRocksLeft--;
		if (mRocksLeft < 2)
			Reset();
	}
}

// 0x40baa0: put away. The sound stops and the slow-down ends. (The original does not put the backdrop and the score
// board back here: only the countdown's own end does, so a quake put away in the middle leaves them shaken)
void BonusEarthquake::Reset()
{
	Bonus::Reset();
	sApp->mSoundQueue->StopLoop(SOUND_EARTHQUAKE);
	mCountdown = QUAKE_NO_COUNTDOWN;
	mEnded = false;
	mRocksLeft = 0;
	mAftershocks = 0;
	for (int i = 0; i < QUAKE_ROCKS; i++)
		Deactivate(mRocks[i]);
	BugObj::SetSlowFrames(0);
}

// 0x40bb10: the quake is over (the countdown is out, or the bonus is cancelled): a bonus that is not running is put away at
// once; otherwise the sound stops and the bugs run at their speed again, and the rocks fall away
void BonusEarthquake::End()
{
	mCountdown = QUAKE_NO_COUNTDOWN;
	mRocksLeft = QUAKE_ROCKS_LEFT_AT_END;
	if (!IsActive()) {
		Reset();
		return;
	}
	sApp->mSoundQueue->StopLoop(SOUND_EARTHQUAKE);
	if (!mEnded) {
		mEnded = true;
		BugObj::SetSlowFrames(0);
	}
}

// 0x40bb70: the quake begins: its sound loops, the score board's place is kept, the quake's own object is put on the
// level's object layer (hidden), the rocks start to fall and the bugs are slowed to every other frame
void BonusEarthquake::Start()
{
	strcpy(mName, "EARTHQUAKE");
	sApp->mSoundQueue->Request(SOUND_EARTHQUAKE, 0, true);
	ObjBase* aScoreBoard = Player::sCurrent->mScoreBoard;
	mSavedX = aScoreBoard->mState[STATE_X];
	mSavedY = aScoreBoard->mState[STATE_Y];
	mCountdown = QUAKE_FRAMES;
	mRocksLeft = QUAKE_NO_COUNTDOWN;
	mAftershocks = 0;
	mEnded = false;
	Bonus::Start();
	SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
	SetHidden(true);
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, QUAKE_Z);
	SetStateInt(STATE_ALPHA, 1);
	SetState(STATE_SCALE_X, QUAKE_SCALE);
	SetState(STATE_SCALE_Y, QUAKE_SCALE);
	SetStateInt(STATE_FRAME, 0);
	for (int i = 0; i < QUAKE_ROCKS; i++) {
		mRocks[i]->StartTrack(0);
		Activate(mRocks[i], false);
	}
	BugObj::SetSlowFrames((int)QUAKE_SLOW_FRAMES);
}

// 0x40bcc0: the 25 rocks, children of the root, each with its own random place along the top, depth, size, picture and
// speed (the random draws in the original's order). A rock's one track never ends: it falls (the orbit's radius, 8 to
// 15, shrinking: it lands in 300 frames at most, or when it is below 1000), fades a little, and its last key's event 4
// tells the quake it has landed (the key's next is the first, so the track starts over)
void BonusEarthquake::BuildRocks()
{
	int aStartY = QUAKE_ROCK_START_Y;
	for (int i = 0; i < QUAKE_ROCKS; i++) {
		ObjBase* aRock = new ObjBase();
		mRocks[i] = aRock;
		aRock->SetParent(ObjBase::sRoot, nullptr);
		aRock->SetStateInt(STATE_ALPHA, 1);
		PipeObj::sPipesAnim->AddRef();
		aRock->mAnim = PipeObj::sPipesAnim;
		int aStep = i & (QUAKE_ROCK_STEPS - 1);
		aRock->SetStateInt(STATE_X, (int)(GameRandomNext() % QUAKE_ROCK_X_RANGE) - QUAKE_ROCK_X_BIAS);
		aRock->SetStateInt(STATE_Y, aStartY);
		aRock->SetStateInt(STATE_Z, aStep * 20 + 1);
		float aScale = (float)((double)(aStep / 2) * 0.5);
		aRock->SetState(STATE_SCALE_X, aScale);
		aRock->SetState(STATE_SCALE_Y, aScale);
		aRock->SetState(STATE_ROTATION, QUAKE_ROCK_ANGLE);
		aRock->SetState(STATE_ORBIT_ANGLE, QUAKE_ROCK_ANGLE);
		aRock->SetStateInt(STATE_ORBIT_RADIUS, aStep + QUAKE_ROCK_FIRST_RADIUS);
		aRock->SetEventTarget(this);
		unsigned int aCel = GameRandomNext() % QUAKE_ROCK_CELS;
		if (aCel > QUAKE_ROCK_CEL_LIMIT)
			aCel = (GameRandomNext() & (QUAKE_ROCK_LAST_CELS - 1)) + QUAKE_ROCK_LAST_CELS_FIRST;
		aRock->SetStateInt(STATE_FRAME, (int)aCel);

		AnimKey* aFallKey = new AnimKey();
		aFallKey->SetDuration(QUAKE_ROCK_FALL_FRAMES);
		aFallKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
		aFallKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
		aFallKey->AddValue(STATE_ALPHA, QUAKE_ROCK_FADE_START, MOD_TOWARD_NEXT);
		aFallKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
		aFallKey->mEndCondition = KEY_END_ABOVE;
		aFallKey->mEndChannel = STATE_Y;
		aFallKey->mEndValue = QUAKE_ROCK_END_Y;
		AnimKey* aLandedKey = new AnimKey();
		aLandedKey->mEventId = QUAKE_EVENT_ROCK_LANDED;
		aLandedKey->SetDuration(1);
		aLandedKey->AddValue(STATE_ALPHA, QUAKE_ROCK_LAND_ALPHA, MOD_TOWARD_NEXT);
		aLandedKey->AddValue(STATE_ORBIT_RADIUS, QUAKE_ROCK_LAND_RADIUS, MOD_TOWARD_NEXT);
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aFallKey, nullptr);
		aTrack->AddKey(aLandedKey, aFallKey);
		aRock->AddTrack(aTrack, nullptr);
		Deactivate(aRock);
		aStartY += QUAKE_ROCK_STEP_Y;
	}
}
