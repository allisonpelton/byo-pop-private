#include "BonusHurricane.h"

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
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

// The key event of a piece's last key (the base's events 1 and 2 are not used by the hurricane)
const int HURRICANE_EVENT_LEAF_GONE = 4;

const int HURRICANE_NO_COUNTDOWN = -99;
const int HURRICANE_FRAMES = 0x259;        // 601: how long it blows
const int HURRICANE_BUG_EVERY = 0xfa;      // frames between the bugs that are blown away (250)
const int HURRICANE_BUGS = 3;              // at most
const int HURRICANE_LEAVES_LEFT_AT_END = 50; // what the pieces that are left are counted from when End is called
const int HURRICANE_Z = 20;                // the hurricane's own object (a picture of nothing) and its size
const float HURRICANE_SCALE = 2.0f;
const int HURRICANE_SLOW_FRAMES = 1;       // updates the bugs skip between two they run: every other frame

// The pieces blow in from the left of the screen: the first from 1050 left of and 650 above the origin, each next 32
// further left and 64 higher. A piece that has left the screen on the right starts again at 1050 left and, at random,
// from 600 above to 600 below
const int HURRICANE_START_X = -1050;
const int HURRICANE_START_Y = -650;
const int HURRICANE_STEP_X = -32;
const int HURRICANE_STEP_Y = -64;
const float HURRICANE_RESTART_X = -1050.0f;
const int HURRICANE_RESTART_Y_RANGE = 0x4b1;  // 1201 places from 600 above
const int HURRICANE_RESTART_Y_BIAS = 600;
const float HURRICANE_END_X = 1000.0f;     // the piece's blow ends when its value passes this

// A piece (the pipes' picture, one of the first 16 cels or one of 12 to 15): its depth, size, radius and spin repeat
// every 8 pieces
const int HURRICANE_STEPS = 8;
const int HURRICANE_CELS = 0x1f;
const int HURRICANE_CEL_LIMIT = 0xf;
const int HURRICANE_LAST_CELS = 4;
const int HURRICANE_LAST_CELS_FIRST = 0xc;
const int HURRICANE_FIRST_RADIUS = 0x19;   // the orbit's radius (a piece's speed a frame): 25 and 3 more for each step
const int HURRICANE_RADIUS_STEP = 3;
const int HURRICANE_BLOW_FRAMES = 999;
const float HURRICANE_SPIN_RATE = 1.0f;    // the rotation is wrapped by whole degrees every frame
const int HURRICANE_SPIN_RANGE = 4;        // and by 1 to 4 degrees, more for each step

// 0x40d090: the fields are as operator new left them in the original; the port clears them
BonusHurricane::BonusHurricane()
{
	for (int i = 0; i < HURRICANE_LEAVES; i++)
		mLeaves[i] = nullptr;
	mEnded = false;
	mCountdown = 0;
	for (int i = 0; i < 3; i++)
		mUnused3a8[i] = 0;
	mDarkened = false;
	mLeavesLeft = 0;
	mBugsBlown = 0;
}

// 0x40d0a0
void BonusHurricane::Setup(int theKind)
{
	Bonus::Setup(theKind);
	mDarkened = false;
	mCountdown = HURRICANE_NO_COUNTDOWN;
	mEnded = false;
	mLeavesLeft = 0;
	mBugsBlown = 0;
	BuildLeaves();
}

// 0x40d0e0: the countdown ends the bonus; every 250 frames of it, up to three times, a bug at random is blown away
bool BonusHurricane::Update()
{
	if (!Bonus::Update())
		return false;
	if (mCountdown != HURRICANE_NO_COUNTDOWN) {
		if (mCountdown > 0) {
			mCountdown--;
			if (mCountdown % HURRICANE_BUG_EVERY == 0 && mBugsBlown < HURRICANE_BUGS) {
				mBugsBlown++;
				BugObj* aBug = BugObj::PickPeaTarget();
				if (aBug != nullptr) {
					aBug->FlyOff();
					return true;
				}
			}
		} else if (mCountdown == 0) {
			mCountdown = HURRICANE_NO_COUNTDOWN;
			End();
		}
	}
	return true;
}

// 0x40d160: a piece has left the screen (its last key, event 4). While the hurricane blows it starts again from the left
// at a random height; once it has ended it is put away, and the bonus when only one is left
void BonusHurricane::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	if (theObj->mCurrentKey->mEventId != HURRICANE_EVENT_LEAF_GONE) {
		Bonus::Notify(theObj);
		return;
	}
	if (!mEnded) {
		theObj->mState[STATE_X] = HURRICANE_RESTART_X;
		theObj->mState[STATE_Y] = (float)((int)(GameRandomNext() % HURRICANE_RESTART_Y_RANGE) - HURRICANE_RESTART_Y_BIAS);
	} else {
		Deactivate(theObj);
		mLeavesLeft--;
		if (mLeavesLeft < 2)
			Reset();
	}
}

// 0x40d1f0: put away. The sound stops, the backdrop is put back and the slow-down ends
void BonusHurricane::Reset()
{
	sApp->mSoundQueue->StopLoop(SOUND_HURRICANE);
	Bonus::Reset();
	mCountdown = HURRICANE_NO_COUNTDOWN;
	mEnded = false;
	mLeavesLeft = 0;
	mBugsBlown = 0;
	if (mDarkened) {
		mDarkened = false;
		sApp->mGameWidget->mBoard->mBackDrop->Undarken();
	}
	for (int i = 0; i < HURRICANE_LEAVES; i++)
		Deactivate(mLeaves[i]);
	Deactivate(this);
	BugObj::SetSlowFrames(0);
}

// 0x40d290: the hurricane is over (the countdown is out, or the bonus is cancelled): a bonus that is not running is put
// away at once; otherwise the sound stops, the backdrop is put back and the bugs run at their speed again, and the pieces
// blow away
void BonusHurricane::End()
{
	if (!IsActive()) {
		Reset();
		return;
	}
	sApp->mSoundQueue->StopLoop(SOUND_HURRICANE);
	mCountdown = HURRICANE_NO_COUNTDOWN;
	mLeavesLeft = HURRICANE_LEAVES_LEFT_AT_END;
	if (!mEnded) {
		mEnded = true;
		if (mDarkened) {
			mDarkened = false;
			sApp->mGameWidget->mBoard->mBackDrop->Undarken();
		}
		BugObj::SetSlowFrames(0);
	}
}

// 0x412570
bool BonusHurricane::EndWhenBugsGone()
{
	return EndOnLastBug();
}

// 0x40d320: the hurricane begins: its sound loops, the hurricane's own object is put on the level's object layer
// (hidden), the backdrop is darkened, the pieces start to blow and the bugs are slowed to every other frame
void BonusHurricane::Start()
{
	strcpy(mName, "HURRICANE");
	sApp->mSoundQueue->Request(SOUND_HURRICANE, 0, true);
	Bonus::Start();
	mCountdown = HURRICANE_FRAMES;
	mLeavesLeft = HURRICANE_NO_COUNTDOWN;
	mBugsBlown = 0;
	mEnded = false;
	SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
	SetHidden(true);
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, HURRICANE_Z);
	SetStateInt(STATE_ALPHA, 1);
	SetState(STATE_SCALE_X, HURRICANE_SCALE);
	SetState(STATE_SCALE_Y, HURRICANE_SCALE);
	SetStateInt(STATE_FRAME, 0);
	sApp->mGameWidget->mBoard->mBackDrop->Darken();
	mDarkened = true;
	int aStartX = HURRICANE_START_X;
	int aStartY = HURRICANE_START_Y;
	for (int i = 0; i < HURRICANE_LEAVES; i++) {
		mLeaves[i]->StartTrack(0);
		mLeaves[i]->SetStateInt(STATE_X, aStartX);
		mLeaves[i]->SetStateInt(STATE_Y, aStartY);
		Activate(mLeaves[i], false);
		aStartX += HURRICANE_STEP_X;
		aStartY += HURRICANE_STEP_Y;
	}
	BugObj::SetSlowFrames(HURRICANE_SLOW_FRAMES);
}

// 0x40d490: the 50 pieces, children of the root, each with its own place along the left, depth, size, picture and speed
// (the random draws in the original's order). A piece's one track never ends: it blows along its orbit (the radius,
// 25 and up, is its speed a frame) while it spins by whole degrees, until its value passes 1000 to the right, and its last
// key's event 4 tells the hurricane it has gone (the key's next is the first, so the track starts over)
void BonusHurricane::BuildLeaves()
{
	int aStartX = HURRICANE_START_X;
	int aStartY = HURRICANE_START_Y;
	for (int i = 0; i < HURRICANE_LEAVES; i++) {
		ObjBase* aLeaf = new ObjBase();
		mLeaves[i] = aLeaf;
		aLeaf->SetParent(ObjBase::sRoot, nullptr);
		aLeaf->SetStateInt(STATE_ALPHA, 1);
		PipeObj::sPipesAnim->AddRef();
		aLeaf->mAnim = PipeObj::sPipesAnim;
		int aStep = i & (HURRICANE_STEPS - 1);
		aLeaf->SetStateInt(STATE_X, aStartX);
		aLeaf->SetStateInt(STATE_Y, aStartY);
		aLeaf->SetStateInt(STATE_Z, aStep * 20 + 1);
		float aScale = (float)((double)(aStep / 2) * 0.5);
		aLeaf->SetState(STATE_SCALE_X, aScale);
		aLeaf->SetState(STATE_SCALE_Y, aScale);
		aLeaf->SetState(STATE_ROTATION, 0.0f);
		aLeaf->SetState(STATE_ORBIT_ANGLE, 0.0f);
		aLeaf->SetStateInt(STATE_ORBIT_RADIUS, aStep * HURRICANE_RADIUS_STEP + HURRICANE_FIRST_RADIUS);
		unsigned int aCel = GameRandomNext() % HURRICANE_CELS;
		if ((int)aCel > HURRICANE_CEL_LIMIT)
			aCel = (GameRandomNext() & (HURRICANE_LAST_CELS - 1)) + HURRICANE_LAST_CELS_FIRST;
		aLeaf->SetStateInt(STATE_FRAME, (int)aCel);
		aLeaf->SetEventTarget(this);

		AnimKey* aBlowKey = new AnimKey();
		aBlowKey->SetDuration(HURRICANE_BLOW_FRAMES);
		aBlowKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
		aBlowKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
		aBlowKey->mEndCondition = KEY_END_ABOVE;
		aBlowKey->mEndChannel = STATE_X;
		aBlowKey->mEndValue = HURRICANE_END_X;
		unsigned int aSpin = GameRandomNext();
		aBlowKey->AddModifier(STATE_ROTATION, false, 0.0f, MOD_WRAP_DEGREES, HURRICANE_SPIN_RATE,
			(float)(int)((aSpin & (HURRICANE_SPIN_RANGE - 1)) + 1 + aStep), 0.0f, nullptr);
		AnimKey* aGoneKey = new AnimKey();
		aGoneKey->mEventId = HURRICANE_EVENT_LEAF_GONE;
		aGoneKey->SetDuration(1);
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aBlowKey, nullptr);
		aTrack->AddKey(aGoneKey, aBlowKey);
		aLeaf->AddTrack(aTrack, nullptr);
		Deactivate(aLeaf);
		aStartX += HURRICANE_STEP_X;
		aStartY += HURRICANE_STEP_Y;
	}
}
