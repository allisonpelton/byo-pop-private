#include "MonkeyLives.h"

#include "Anim.h"
#include "Emitter.h"
#include "Pea.h"
#include "Player.h"

using namespace Sexy;

Anim* MonkeyLives::sDanceAnim[3] = { nullptr, nullptr, nullptr };
Anim* MonkeyLives::sShootAnim = nullptr;
Anim* MonkeyLives::sStandDownAnim = nullptr;

// The monkey's tracks (the order they are added in). The first three are the dances.
enum MonkeyTrack
{
	MONKEY_TRACK_DANCE_COUNT = 3, // the dances take tracks 0 to 2
	MONKEY_TRACK_SHOOT = 3,
	MONKEY_TRACK_STAND_DOWN = 4,
	MONKEY_TRACK_VANISH = 5,      // nothing starts it
	MONKEY_TRACK_REAPPEAR = 6,    // started by the vanishing track's last key
};

// The key events the monkey's tracks raise (AnimKey::mEventId)
enum MonkeyEvent
{
	MONKEY_EVENT_DONE = 1,        // a dance, the stepping aside or the reappearing has ended: pick what comes next
	MONKEY_EVENT_SHOT = 2,        // the blowgun fires: the pea leaves and the puff shows
	MONKEY_EVENT_SHOT_DONE = 3,   // the shooting has ended
	MONKEY_EVENT_VANISHED = 4,    // the vanishing track has ended
	MONKEY_EVENT_REAPPEAR = 5,    // the reappearing track's first key has ended: a burst of swells
};

// How close the player's monkey (design pixels) sends this one aside
const float MONKEY_NEAR_REACH = 124.0f;

// The puff of light is the particles picture's cel 2, set 28 up and left of the monkey's middle, at depth 5; the pea
// flies at depth 300
const int PUFF_FRAME = 2;
const int PUFF_OFFSET = -28;
const int PUFF_Z = 5;
const int PEA_Z = 300;

// How fast the dances' frame counters step (the cels a dance counts through are in BuildTracks)
const float DANCE_COUNT_RATE = 20.0f;

// The swells the monkey's burst is made of are put at the places of a grid of this step, over the picture's size
const int BURST_GRID_STEP = 32;

// A key of one of the monkey's tracks, showing one of its pictures
static AnimKey* MakeKey(Anim* thePicture, int theEventId, int theFrames)
{
	AnimKey* aKey = new AnimKey();
	aKey->mEventId = theEventId;
	thePicture->AddRef();
	aKey->mAnim = thePicture;
	aKey->SetDuration(theFrames);
	return aKey;
}

// 0x44f450: the pictures, the swells it bursts, then its tracks and its puff, and the pea it fires, under the root.
// It starts in one of the three dances. The original also names the object "MonkeyLives" in a field nothing reads.
MonkeyLives::MonkeyLives()
{
	sDanceAnim[0] = Anim::Get("images/monkey_lives1", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0x1c, false);
	sDanceAnim[1] = Anim::Get("images/monkey_lives2", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0x23, false);
	sDanceAnim[2] = Anim::Get("images/monkey_lives3", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0x32, false);
	sShootAnim = Anim::Get("images/monkey_lives4", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0x4b, false);
	sStandDownAnim = Anim::Get("images/monkey_lives5", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0x24, false);
	mEmitters = new EmitterSet(this);
	mEmitters->MakeSlowSwell(5.0f);
	SetStateInt(STATE_Z, 5);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_RED, 1);
	SetStateInt(STATE_GREEN, 1);
	SetStateInt(STATE_BLUE, 1);
	SetStateInt(STATE_ALPHA, 1);
	BuildTracks();
	BuildPuff();
	mPhase = MONKEY_PHASE_DANCE;
	mPea = new Pea();
	mPea->SetParent(nullptr, nullptr);
	mPea->SetStateInt(STATE_Z, PEA_Z);
	StartTrack((int)(GameRandomNext() % MONKEY_TRACK_DANCE_COUNT));
}

// 0x44f650: the pea goes with it
MonkeyLives::~MonkeyLives()
{
	if (mPea != nullptr)
		mPea->Delete();
}

// 0x44f6c0: a key of the monkey's tracks ended
void MonkeyLives::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case MONKEY_EVENT_DONE:
		// a dance, the stepping aside or the reappearing is over: shoot if asked to, step aside if sent, else dance
		theObj->SetStateInt(STATE_FRAME, 0);
		if (mPhase == MONKEY_PHASE_SHOOT) {
			theObj->StartTrack(MONKEY_TRACK_SHOOT);
		} else if (mPhase == MONKEY_PHASE_STAND_DOWN) {
			theObj->StartTrack(MONKEY_TRACK_STAND_DOWN);
			mPhase = MONKEY_PHASE_DANCE;
		} else {
			theObj->StartTrack((int)(GameRandomNext() % MONKEY_TRACK_DANCE_COUNT));
		}
		break;
	case MONKEY_EVENT_SHOT:
		// the pea leaves the monkey's place and the puff shows (and runs its own track)
		mPea->Fire(mScreenX, mScreenY);
		Activate(mPuff, false);
		mPuff->StartTrack(0);
		break;
	case MONKEY_EVENT_SHOT_DONE:
		// back to dancing; a monkey sent aside while it shot stays sent, and steps aside at the end of that dance
		theObj->SetStateInt(STATE_FRAME, 0);
		theObj->StartTrack((int)(GameRandomNext() % MONKEY_TRACK_DANCE_COUNT));
		if (mPhase != MONKEY_PHASE_STAND_DOWN)
			mPhase = MONKEY_PHASE_DANCE;
		break;
	case MONKEY_EVENT_VANISHED:
		theObj->StartTrack(mReappearTrack);
		break;
	case MONKEY_EVENT_REAPPEAR:
		BurstSmoke();
		break;
	default:
		break;
	}
}

// 0x44f800: a monkey whose frame is over and that the player's monkey is within 124 pixels of is sent aside
bool MonkeyLives::Update()
{
	if (!ObjBase::Update())
		return false;
	if (Player::IsNear(this, MONKEY_NEAR_REACH))
		mPhase = MONKEY_PHASE_STAND_DOWN;
	return true;
}

// 0x44f840: with its pea not in flight and nothing asked of it, it is to shoot (the original reads the pea's active
// flag, not the monkey's: the old stand-in read the monkey's own)
void MonkeyLives::StartShooting()
{
	if (!mPea->IsActive() && mPhase == MONKEY_PHASE_DANCE)
		mPhase = MONKEY_PHASE_SHOOT;
}

// 0x44f870
void MonkeyLives::StandDown()
{
	mPhase = MONKEY_PHASE_STAND_DOWN;
}

// 0x44f880: slow swells at the places of a grid (32 apart, counting from 32, over the dance picture's size) with the
// picture's middle taken off, and five more at the middle and 16 above, below, left and right. Only the track the
// original never starts reaches it.
void MonkeyLives::BurstSmoke()
{
	Anim* aPicture = sDanceAnim[0];
	float aHalfWidth = (float)((double)aPicture->GetSourceCelWidth() * 0.5);
	float aHalfHeight = (float)((double)aPicture->GetSourceCelHeight() * 0.5);
	int aHeightLimit = (int)aPicture->GetSourceCelHeight();
	int aWidthLimit = (int)aPicture->GetSourceCelWidth();
	for (int aRow = BURST_GRID_STEP; aRow <= aHeightLimit; aRow += BURST_GRID_STEP) {
		for (int aColumn = BURST_GRID_STEP; aColumn <= aWidthLimit; aColumn += BURST_GRID_STEP)
			mEmitters->BurstSlowSwell((float)aColumn - aHalfWidth, (float)aRow - aHalfHeight);
	}
	mEmitters->BurstSlowSwell(0.0f, 0.0f);
	mEmitters->BurstSlowSwell(0.0f, -16.0f);
	mEmitters->BurstSlowSwell(0.0f, 16.0f);
	mEmitters->BurstSlowSwell(-16.0f, 0.0f);
	mEmitters->BurstSlowSwell(16.0f, 0.0f);
}

// 0x44f9e0: the puff of light, a child of the monkey: the particles picture's cel 2 in additive light, 28 up and left
// of the monkey's middle, waiting until a shot. Its one track, 17 frames: it grows to 0.8 turning an eighth of a turn
// and a quarter of a turn more in the last frame (where the original sets a width of 0.1 and a height of 0).
void MonkeyLives::BuildPuff()
{
	mPuff = new ObjBase();
	mPuff->SetParent(this, nullptr);
	Emitter::sParticlesAnim->AddRef();
	mPuff->mAnim = Emitter::sParticlesAnim;
	mPuff->SetStateInt(STATE_FRAME, PUFF_FRAME);
	mPuff->SetStateInt(STATE_SCALE_X, 0);
	mPuff->SetStateInt(STATE_SCALE_Y, 0);
	mPuff->SetStateInt(STATE_X, PUFF_OFFSET);
	mPuff->SetStateInt(STATE_Y, PUFF_OFFSET);
	mPuff->SetStateInt(STATE_Z, PUFF_Z);
	mPuff->SetStateInt(STATE_ALPHA, 0);

	AnimKey* aGrow = new AnimKey();
	aGrow->SetDuration(8);
	aGrow->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aGrow->mDrawMode = 1;

	AnimKey* aFull = new AnimKey();
	aFull->SetDuration(8);
	aFull->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aFull->AddValue(STATE_SCALE_X, 0.8f, MOD_TOWARD_NEXT);
	aFull->AddValue(STATE_SCALE_Y, 0.8f, MOD_TOWARD_NEXT);
	aFull->AddValue(STATE_ROTATION, 0.7853982f, MOD_TOWARD_NEXT);
	aFull->mDrawMode = 1;

	AnimKey* aGone = new AnimKey();
	aGone->SetDuration(1);
	aGone->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_SCALE_X, 0.1f, MOD_HOLD);
	aGone->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_ROTATION, 1.5707964f, MOD_HOLD);
	aGone->mDrawMode = 1;

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aGrow, nullptr);
	aTrack->AddKey(aFull, nullptr);
	aTrack->AddKey(aGone, nullptr);
	mPuff->AddTrack(aTrack, nullptr);
	mPuff->Deactivate(mPuff);
}

// 0x44fd50: the monkey's seven tracks, each key showing one of the five pictures. The values are relative to the
// object's own, so an alpha of -1 is invisible and a frame of 0 is the first.
void MonkeyLives::BuildTracks()
{
	// the three dances: one cycle of the dance's cels (a wrapping counter) and the key ends, raising event 1 each time
	const int aDanceFrames[3] = { 0x91, 0xaf, 0xfa };
	const float aDanceCels[3] = { 28.0f, 35.0f, 50.0f };
	for (int i = 0; i < 3; i++) {
		AnimKey* aDance = MakeKey(sDanceAnim[i], MONKEY_EVENT_DONE, aDanceFrames[i]);
		aDance->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, DANCE_COUNT_RATE, aDanceCels[i], 0.0f, nullptr);
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aDance, aDance);
		AddTrack(aTrack, nullptr);
	}

	// the shot: cels 0 to 25 in 100 frames, 25 to 74 in 196 (the blowgun fires as these end), then the shooting ends
	AnimKey* aShootStart = MakeKey(sShootAnim, 0, 100);
	aShootStart->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aShootFire = MakeKey(sShootAnim, MONKEY_EVENT_SHOT, 0xc4);
	aShootFire->AddValue(STATE_FRAME, 25.0f, MOD_TOWARD_NEXT);
	AnimKey* aShootEnd = MakeKey(sShootAnim, MONKEY_EVENT_SHOT_DONE, 0);
	aShootEnd->AddValue(STATE_FRAME, 74.0f, MOD_TOWARD_NEXT);
	AnimTrack* aShootTrack = new AnimTrack();
	aShootTrack->AddKey(aShootStart, nullptr);
	aShootTrack->AddKey(aShootFire, nullptr);
	aShootTrack->AddKey(aShootEnd, nullptr);
	AddTrack(aShootTrack, nullptr);

	// stepping aside: cels 0 to 35 in 144 frames, then one more frame
	AnimKey* aStandStart = MakeKey(sStandDownAnim, 0, 0x90);
	aStandStart->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aStandEnd = MakeKey(sStandDownAnim, MONKEY_EVENT_DONE, 1);
	aStandEnd->AddValue(STATE_FRAME, 35.0f, MOD_TOWARD_NEXT);
	AnimTrack* aStandTrack = new AnimTrack();
	aStandTrack->AddKey(aStandStart, nullptr);
	aStandTrack->AddKey(aStandEnd, nullptr);
	AddTrack(aStandTrack, nullptr);

	// vanishing: it fades out in 5 frames, stays out 100 frames, then raises event 4 (the original never starts it)
	AnimKey* aFade = MakeKey(sDanceAnim[0], 0, 5);
	aFade->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aOut = MakeKey(sDanceAnim[0], 0, 100);
	aOut->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	AnimKey* aVanished = MakeKey(sDanceAnim[0], MONKEY_EVENT_VANISHED, 1);
	aVanished->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	AnimTrack* aVanishTrack = new AnimTrack();
	aVanishTrack->AddKey(aFade, nullptr);
	aVanishTrack->AddKey(aOut, nullptr);
	aVanishTrack->AddKey(aVanished, nullptr);
	mVanishTrack = AddTrack(aVanishTrack, nullptr);

	// reappearing: out for 200 frames, a burst of swells (event 5), then back in a frame, and event 1
	AnimKey* aWait = MakeKey(sDanceAnim[0], 0, 200);
	aWait->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aWait->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimKey* aBurst = MakeKey(sDanceAnim[0], MONKEY_EVENT_REAPPEAR, 2);
	aBurst->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aBurst->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimKey* aBack = MakeKey(sDanceAnim[0], MONKEY_EVENT_DONE, 1);
	aBack->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aBack->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimTrack* aReappearTrack = new AnimTrack();
	aReappearTrack->AddKey(aWait, nullptr);
	aReappearTrack->AddKey(aBurst, nullptr);
	aReappearTrack->AddKey(aBack, nullptr);
	mReappearTrack = AddTrack(aReappearTrack, nullptr);
}
