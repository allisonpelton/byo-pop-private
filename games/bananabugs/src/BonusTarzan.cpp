#include "BonusTarzan.h"

#include "Anim.h"
#include "BugObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusTarzan::sTarzanAnim = nullptr;

// The key event of a swing's last key (the base's events 1 and 2 are not used by the monkeys)
const int TARZAN_EVENT_SWUNG = 4;

const int TARZAN_Z = 20;                   // the bonus's own object (a picture of nothing) and the monkeys' depth
const int TARZAN_HAND_Z = 50;
const int TARZAN_SLOW_FRAMES = 2;          // updates the bugs skip between two they run
const int TARZAN_SEND_EVERY = 0x4b;        // frames between monkeys: 75
const int TARZAN_YELL_FRAMES = 0x50;       // a monkey yells this many frames after it is sent
const int TARZAN_NO_TIMER = -1;
const int TARZAN_ATTEMPTS = 3;             // a bug already sent to is picked again at most this many times
const float TARZAN_PICK_ABOVE = 300.0f;    // the bugs it picks are above this on the screen
const float TARZAN_GRAB_REACH = 64.0f;     // a bug this near a hand is picked up
const double TARZAN_TIP_BACK = 64.0;       // the hand is this far in from the picture's end; the monkey hangs from it
const int TARZAN_RANDOM_RANGE = 0xc9;      // a monkey with no bug to go to goes to 400 to 600 along the top
const int TARZAN_RANDOM_FIRST = 0x190;
const double TARZAN_RANDOM_TOP = 100.0;    // ... and its height is this less how far the picture's end reaches
const int TARZAN_SWING_FRAMES = 0x14a;     // 330
const float TARZAN_SWING_FAR = 6.63225174f;  // 0x40d43b68: the swing's two ends (radians), swapped for the monkeys that hang the other way
const float TARZAN_SWING_NEAR = 2.79252696f; // 0x4032b8c3
const int TARZAN_SOUNDS_FIRST = SOUND_TARZAN1;

// 0x411c60: the kind is given by Setup; the fields are as operator new left them in the original, the port clears them
BonusTarzan::BonusTarzan()
{
	for (int i = 0; i < TARZAN_MONKEYS; i++) {
		mMonkeys[i] = nullptr;
		mHands[i] = nullptr;
		mTargets[i] = nullptr;
		mCaught[i] = false;
		mTimers[i] = 0;
		mSounds[i] = 0;
	}
	mTargetCount = 0;
	mEnded = false;
	mMonkeysOut = 0;
	mDelay = 0;
	mSending = false;
}

// 0x411c70
void BonusTarzan::Setup(int theKind)
{
	Bonus::Setup(theKind);
	for (int i = 0; i < TARZAN_MONKEYS; i++) {
		mTimers[i] = TARZAN_NO_TIMER;
		mCaught[i] = false;
		mHands[i] = nullptr;
		mMonkeys[i] = nullptr;
	}
	mEnded = false;
	mMonkeysOut = 0;
	mTargetCount = 0;
	mSending = false;
	BuildMonkeys();
}

// 0x411cd0: each monkey that is swinging has its hand watched: a bug within reach that is not already on the hand is
// picked up (the bug dies and its picture is left on the hand). The yells play as their timers run out. Every 75 frames
// the next monkey is sent
bool BonusTarzan::Update()
{
	if (!Bonus::Update())
		return false;
	if (mMonkeys[0] == nullptr)
		return true;
	for (int i = 0; i < TARZAN_MONKEYS; i++) {
		if (mMonkeys[i]->IsActive() && !mCaught[i]) {
			ObjBase* aHand = mHands[i];
			BugObj* aBug = BugObj::FindNear(aHand->mScreenX, aHand->mScreenY, TARZAN_GRAB_REACH);
			if (aBug != nullptr && aBug->mParent != aHand) {
				mCaught[i] = true;
				aBug->LeaveCopy(aHand);
			}
		}
		if (mTimers[i] > 0) {
			mTimers[i]--;
			if (mTimers[i] == 0)
				sApp->mSoundQueue->Request(mSounds[i], 0, false);
		}
	}
	if (mSending) {
		if (mDelay < 1) {
			mDelay = TARZAN_SEND_EVERY;
			if (!SendMonkey()) {
				mSending = false;
				return true;
			}
		} else {
			mDelay--;
		}
	}
	return true;
}

// 0x411de0: a monkey's swing is over (event 4): it is put away (turned upright, first), and the bonus is put away when
// only one swing is left to end
void BonusTarzan::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	if (theObj->mCurrentKey->mEventId != TARZAN_EVENT_SWUNG) {
		Bonus::Notify(theObj);
		return;
	}
	theObj->SetStateInt(STATE_ROTATION, 0);
	Deactivate(theObj);
	mMonkeysOut--;
	if (mMonkeysOut <= 1)
		Reset();
}

// 0x411e40: put away: the bugs run at their speed again, and the monkeys are put away
void BonusTarzan::Reset()
{
	Bonus::Reset();
	BugObj::SetSlowFrames(0);
	for (int i = 0; i < TARZAN_MONKEYS; i++) {
		if (mMonkeys[i] != nullptr)
			Deactivate(mMonkeys[i]);
		mTimers[i] = TARZAN_NO_TIMER;
		mCaught[i] = false;
	}
	mTargetCount = 0;
	mMonkeysOut = 0;
	mSending = false;
	mEnded = false;
}

// 0x411eb0: the first End puts the bonus away at once when no monkey has been sent
void BonusTarzan::End()
{
	if (!mEnded) {
		mEnded = true;
		if (mMonkeysOut < 1)
			Reset();
	}
}

// 0x411ee0: the monkeys start to come, the bugs are slowed to a third (they skip two updates in three)
void BonusTarzan::Start()
{
	strcpy(mName, "MONKEY ATTACK");
	for (int i = 0; i < TARZAN_MONKEYS; i++)
		mTargets[i] = nullptr;
	mCaught[0] = false;
	mCaught[1] = false;
	mCaught[2] = false;
	mSending = true;
	mMonkeysOut = 0;
	mDelay = 0;
	mTargetCount = 0;
	Bonus::Start();
	SetHidden(true);
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, TARZAN_Z);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, 0);
	BugObj::SetSlowFrames(TARZAN_SLOW_FRAMES);
}

// 0x411fa0: sends the next monkey (false when three have been sent, or three bugs are being gone for, or there is no bug
// to pick from above): over a bug picked from those high on the screen that no monkey has gone for (it picks again up to
// three times, then goes to a random place along the top), the swing starts, and the monkey is shown
bool BonusTarzan::SendMonkey()
{
	if (mTargetCount >= TARZAN_MONKEYS)
		return false;
	if (mMonkeysOut >= TARZAN_MONKEYS)
		return false;
	int aTries = 0;
	BugObj* aBug;
	for (;;) {
		aBug = BugObj::PickAbove(TARZAN_PICK_ABOVE);
		if (aBug == nullptr)
			return false;
		int i = 0;
		if (mTargetCount > 0) {
			for (; i < mTargetCount; i++) {
				if (mTargets[i] == aBug)
					break;
			}
			if (i < mTargetCount) {
				aTries++;
				aBug = nullptr;
				if (aTries < TARZAN_ATTEMPTS)
					continue;
			}
		}
		break;
	}
	int aSent = mMonkeysOut;
	if (aSent % 3 == 0)
		mSounds[aSent] = TARZAN_SOUNDS_FIRST;
	else if (aSent % 3 == 1)
		mSounds[aSent] = TARZAN_SOUNDS_FIRST + 1;
	else
		mSounds[aSent] = TARZAN_SOUNDS_FIRST + 2;
	mTimers[aSent] = TARZAN_YELL_FRAMES;
	ObjBase* aMonkey = mMonkeys[aSent];
	double aReach = (double)sTarzanAnim->GetSourceCelWidth() - TARZAN_TIP_BACK;
	if (aBug == nullptr) {
		aMonkey->SetStateInt(STATE_X, (int)(GameRandomNext() % TARZAN_RANDOM_RANGE) + TARZAN_RANDOM_FIRST);
		aMonkey->SetState(STATE_Y, (float)(TARZAN_RANDOM_TOP - aReach));
	} else {
		aMonkey->SetState(STATE_X, aBug->mState[STATE_X]);
		aMonkey->SetState(STATE_Y, (float)((double)aBug->mState[STATE_Y] - aReach));
		mTargets[mTargetCount] = aBug;
		mTargetCount++;
	}
	aMonkey->StartTrack(0);
	Activate(aMonkey, false);
	aMonkey->ComputeDrawn();
	mMonkeysOut++;
	return true;
}

// 0x412140: the three monkeys, children of the level's object layer, and each one's hand. A monkey hangs from the top
// by its picture's width (half of it is its offset), faces up or down by its number, and swings by turning from one end
// of the arc to the other in 330 frames (the other way for every other monkey); its last key's event 4 says it is over
void BonusTarzan::BuildMonkeys()
{
	for (int i = 0; i < TARZAN_MONKEYS; i++) {
		ObjBase* aMonkey = new ObjBase();
		mMonkeys[i] = aMonkey;
		aMonkey->SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
		aMonkey->SetStateInt(STATE_ALPHA, 1);
		sTarzanAnim->AddRef();
		aMonkey->mAnim = sTarzanAnim;
		aMonkey->SetStateInt(STATE_X, 0);
		aMonkey->SetStateInt(STATE_Y, 0);
		aMonkey->SetStateInt(STATE_Z, i + TARZAN_Z);
		aMonkey->SetStateInt(STATE_SCALE_X, 1);
		bool anOdd = (i & 1) != 0;
		aMonkey->SetStateInt(STATE_SCALE_Y, anOdd ? -1 : 1);
		aMonkey->SetStateInt(STATE_ROTATION, 0);
		aMonkey->SetState(STATE_OFFSET_X, (float)((double)sTarzanAnim->GetSourceCelWidth() * 0.5));
		aMonkey->SetEventTarget(this);

		AnimKey* aSwingKey = new AnimKey();
		aSwingKey->SetDuration(TARZAN_SWING_FRAMES);
		aSwingKey->AddValue(STATE_ROTATION, anOdd ? TARZAN_SWING_NEAR : TARZAN_SWING_FAR, MOD_TOWARD_NEXT);
		aSwingKey->mEasing = EASE_COSINE_TOO;
		AnimKey* aSwungKey = new AnimKey();
		aSwungKey->SetDuration(1);
		aSwungKey->mEventId = TARZAN_EVENT_SWUNG;
		aSwungKey->AddValue(STATE_ROTATION, anOdd ? TARZAN_SWING_FAR : TARZAN_SWING_NEAR, MOD_HOLD);
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aSwingKey, nullptr);
		aTrack->AddKey(aSwungKey, nullptr);
		aMonkey->AddTrack(aTrack, nullptr);
		Deactivate(aMonkey);

		ObjBase* aHand = new ObjBase();
		mHands[i] = aHand;
		aHand->SetParent(aMonkey, nullptr);
		aHand->SetState(STATE_X, (float)((double)sTarzanAnim->GetSourceCelWidth() - TARZAN_TIP_BACK));
		aHand->SetStateInt(STATE_Y, 0);
		aHand->SetStateInt(STATE_Z, TARZAN_HAND_Z);
		Activate(aHand, false);
	}
}
