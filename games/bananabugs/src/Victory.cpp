#include "Victory.h"

#include "Anim.h"
#include "FireWorks.h"
#include "GameApp.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The keys' event ids (AnimKey::mEventId)
const int VICTORY_EVENT_FADE_START = 100;     // the scene's first track's second key; nothing answers it
const int VICTORY_EVENT_FADE_HOLD = 0x65;     // its third key; nothing answers it either
const int VICTORY_EVENT_HEART = 0x62;         // a heart's last key: it starts over from another place
const int VICTORY_EVENT_CONFETTI = 0x63;      // a piece of confetti's last key: it starts over at the top

// The scene's own tracks: the first plays as the scene shows, the second is the fade-away a press starts
enum VictoryTrack
{
	VICTORY_TRACK_SHOW = 0,
	VICTORY_TRACK_FADE = 1,
};

const int VICTORY_Z = 0x140;
const int VICTORY_START_DELAY = 300;           // frames before the fireworks start and a press counts
const int VICTORY_CONFETTI = 0x78;             // pieces of confetti
const int VICTORY_HEARTS = 4;                  // hearts in the stream
const int VICTORY_HEART_SPACING = 35;          // frames between one heart's start and the next's
const int VICTORY_FIREWORK_PERIOD = 30;        // frames between rockets
const int VICTORY_SOUND_PERIOD = 70;           // frames between the monkeys' calls
const int VICTORY_SOUND_DRAWS = 12;            // the call is drawn from this many; four of them are silent
const int VICTORY_FIREWORK_SPREAD = 0x4b1;     // a rocket's place across, from 600 left to 600 right of the middle
const int VICTORY_FIREWORK_MARGIN = 600;
const float VICTORY_FIREWORK_Y = 600.0f;

// The first track's turn, in radians (the scene spins in from this far round)
const float VICTORY_TURN = -11.170108f;

// The sound each draw gives (-1: none, for 5, 6, 10 and 11)
static const int VICTORY_CALLS[VICTORY_SOUND_DRAWS] = {
	SOUND_MONKEY_HEEEK, SOUND_MONKEY_JIM, SOUND_MONKEY_WHOHAH, SOUND_MONKEY_OHH1, SOUND_MONKEY_OHH1, -1, -1,
	SOUND_MONKEY_MIKE, SOUND_MONKEY_OHH2, SOUND_MONKEY_OHH2, -1, -1,
};

// 0x473460: the scene is a picture (the backdrop) at depth 320, with the first track's fade and everything else its
// children; it starts inactive (Show brings it in). The fireworks are twelve rockets at depth 1.
Victory::Victory()
{
	SetStateInt(STATE_Z, VICTORY_Z);
	mDelay = VICTORY_START_DELAY;
	mDone = false;
	mMusicChanged = false;
	// the mode is the register the call before left, the channel 2 (smooth)
	mAnim = Anim::Get("images/victory/win_backdrop", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	BuildFadeTracks();
	BuildMonkeys();
	BuildTrophy();
	BuildConfetti();
	BuildHearts();
	BuildSign();
	ObjBase::Activate(this, false);
	for (int i = 0; i < VICTORY_FIREWORKS; i++) {
		mFireWorks[i] = new FireWorks();
		mFireWorks[i]->SetParent(this, nullptr);
		mFireWorks[i]->SetStateInt(STATE_Z, 1);
	}
	ObjBase::Deactivate(this);
}

// 0x473590 (body 0x4735b0): the main menu's song comes back if Show changed it
Victory::~Victory()
{
	if (mMusicChanged)
		gApp->ChangeMusic(MUSIC_MAIN_MENU);
}

// 0x4738b0: the scene's own tracks, on the picture of the backdrop (all its children go with it). Track 0, which
// plays as the scene shows: the values are offsets from the object's own, so over 300 and then 55 frames the scene
// grows in from nothing (alpha and both scales from -1, the turn from -11 radians), then it holds. Track 1, the fade
// a press starts: 25 frames from the scene as it is to a key of no length that is clear, a tenth larger and not turned.
void Victory::BuildFadeTracks()
{
	AnimKey* aFirst = new AnimKey();
	aFirst->SetDuration(VICTORY_START_DELAY);
	aFirst->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aFirst->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aFirst->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	aFirst->AddValue(STATE_ROTATION, VICTORY_TURN, MOD_TOWARD_NEXT);

	AnimKey* aSecond = new AnimKey();
	aSecond->mEventId = VICTORY_EVENT_FADE_START;
	aSecond->SetDuration(0x37);
	aSecond->AddValue(STATE_ALPHA, -0.5f, MOD_TOWARD_NEXT);
	aSecond->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aSecond->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	aSecond->AddValue(STATE_ROTATION, VICTORY_TURN, MOD_TOWARD_NEXT);

	AnimKey* aThird = new AnimKey();
	aThird->mEventId = VICTORY_EVENT_FADE_HOLD;
	aThird->SetDuration(1);
	aThird->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aThird->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aThird->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aThird->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimKey* aFourth = new AnimKey();
	aFourth->SetDuration(10);
	aFourth->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aFourth->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aFourth->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aFourth->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimTrack* aShowTrack = new AnimTrack();
	aShowTrack->AddKey(aFirst, nullptr);
	aShowTrack->AddKey(aSecond, nullptr);
	aShowTrack->AddKey(aThird, nullptr);
	aShowTrack->AddKey(aFourth, aFourth);
	AddTrack(aShowTrack, nullptr);

	AnimKey* aFadeFrom = new AnimKey();
	aFadeFrom->SetDuration(0x19);
	aFadeFrom->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFadeFrom->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFadeFrom->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFadeFrom->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aFadeTo = new AnimKey();
	aFadeTo->SetDuration(0);
	aFadeTo->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aFadeTo->AddValue(STATE_SCALE_X, 1.1f, MOD_TOWARD_NEXT);
	aFadeTo->AddValue(STATE_SCALE_Y, 1.1f, MOD_TOWARD_NEXT);
	aFadeTo->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aFadeTrack = new AnimTrack();
	aFadeTrack->AddKey(aFadeFrom, nullptr);
	aFadeTrack->AddKey(aFadeTo, nullptr);
	AddTrack(aFadeTrack, nullptr);
}

// 0x473de0: three pictures of the monkeys' heads at the bottom (the first drawn smaller), each bobbing up and down
// 16 pixels on a cosine, a little slower than the one before
void Victory::BuildMonkeys()
{
	for (int i = 0; i < VICTORY_MONKEYS; i++) {
		ObjBase* aMonkey = new ObjBase();
		mMonkeys[i] = aMonkey;
		aMonkey->mAnim = Anim::Get("images/victory/win_monkeys", ANIM_SCALE_SMOOTH, false, 0x77f, 0x1db, 2, false);
		aMonkey->SetStateInt(STATE_Y, 0x1ae);
		aMonkey->SetStateInt(STATE_Z, i + 2);
		aMonkey->SetStateInt(STATE_ALPHA, 1);
		aMonkey->SetParent(this, nullptr);
		aMonkey->SetStateInt(STATE_FRAME, i & 1);
		if (i == 0) {
			aMonkey->SetState(STATE_SCALE_X, 0.8f);
			aMonkey->SetState(STATE_SCALE_Y, 0.8f);
		}

		int aBobFrames = i * 3 + 0x19;
		AnimKey* aUp = new AnimKey();
		aUp->SetDuration(aBobFrames);
		aUp->AddValue(STATE_Y, 16.0f, MOD_TOWARD_NEXT);
		aUp->mEasing = EASE_COSINE;
		AnimKey* aDown = new AnimKey();
		aDown->SetDuration(aBobFrames);
		aDown->AddValue(STATE_Y, -16.0f, MOD_TOWARD_NEXT);
		aDown->mEasing = EASE_COSINE;
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aUp, nullptr);
		aTrack->AddKey(aDown, aUp);
		aMonkey->AddTrack(aTrack, nullptr);
		aMonkey->StartTrack(0);
		Activate(aMonkey, false);
	}
}

// 0x473fe0: the trophy, placed from the corner at (1090, 211) at depth 5: its track drops it in from 400 above in 500
// frames, then bobs it up and down 16 pixels on a cosine (60 frames each way, the key of no rise first). Two
// coronas, children of it but activated by the scene, turn behind it for ever: the first by degrees wrapped, the
// second spinning, both lit additively.
void Victory::BuildTrophy()
{
	ObjBase* aTrophy = new ObjBase();
	mTrophy = aTrophy;
	aTrophy->mAnim = Anim::Get("images/victory/win_trophy", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	aTrophy->SetParent(this, nullptr);
	aTrophy->SetDesignX(1090.0f);
	aTrophy->SetDesignY(211.0f);
	aTrophy->SetStateInt(STATE_Z, 5);
	aTrophy->SetStateInt(STATE_RED, 1);
	aTrophy->SetStateInt(STATE_GREEN, 1);
	aTrophy->SetStateInt(STATE_BLUE, 1);
	aTrophy->SetStateInt(STATE_ALPHA, 1);

	AnimKey* aDrop = new AnimKey();
	aDrop->SetDuration(500);
	aDrop->AddValue(STATE_Y, -400.0f, MOD_TOWARD_NEXT);
	AnimKey* aRest = new AnimKey();
	aRest->SetDuration(0x3c);
	aRest->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aRest->mEasing = EASE_COSINE;
	AnimKey* aRise = new AnimKey();
	aRise->SetDuration(0x3c);
	aRise->AddValue(STATE_Y, -16.0f, MOD_TOWARD_NEXT);
	aRise->mEasing = EASE_COSINE;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aDrop, nullptr);
	aTrack->AddKey(aRest, nullptr);
	aTrack->AddKey(aRise, aRest);
	aTrophy->AddTrack(aTrack, nullptr);
	aTrophy->StartTrack(0);
	Activate(aTrophy, false);

	for (int i = 0; i < VICTORY_CORONAS; i++) {
		ObjBase* aCorona = new ObjBase();
		mCoronas[i] = aCorona;
		aCorona->SetParent(aTrophy, nullptr);
		aCorona->mAnim = Anim::Get("images/corona1", ANIM_SCALE_FAST, false, 0, 0, 1, false);
		aCorona->SetStateInt(STATE_X, -0x20);
		aCorona->SetStateInt(STATE_Y, 0);   // the original passes the register the call before left, the channel 0
		aCorona->SetState(STATE_SCALE_X, 2.5f);
		aCorona->SetState(STATE_SCALE_Y, 2.5f);
		aCorona->SetStateInt(STATE_Y, 0);
		aCorona->SetStateInt(STATE_Z, -1);
		aCorona->SetStateInt(STATE_RED, 1);
		aCorona->SetStateInt(STATE_GREEN, 1);
		aCorona->SetStateInt(STATE_BLUE, 1);
		aCorona->SetState(STATE_ALPHA, 0.9f);

		AnimKey* aTurn = new AnimKey();
		aTurn->SetDuration(0x168);
		aTurn->mDrawMode = 1;               // lit additively (AnimKey 0x4535c0)
		aTurn->AddModifier(STATE_ROTATION, false, 0.0f, i == 0 ? MOD_WRAP_DEGREES : MOD_SPIN, 1.0f, 1.0f, 0.0f, nullptr);
		AnimTrack* aCoronaTrack = new AnimTrack();
		aCoronaTrack->AddKey(aTurn, aTurn);
		aCorona->AddTrack(aCoronaTrack, nullptr);
		aCorona->StartTrack(0);
		Activate(aCorona, false);
	}
}

// 0x4743d0: 120 pieces of confetti in four sizes and four colours, each a cel of the picture, put at a random place
// along the top. Its track waits (i mod 40) * 5 frames, then falls from 700 above to the bottom, swaying 32 pixels
// from side to side, turning through its two cels, ending when it is lower than 1200; the last key, of no length,
// asks the scene (Notify) to start it over from the top at a new place across.
void Victory::BuildConfetti()
{
	for (int i = 0; i < VICTORY_CONFETTI; i++) {
		ObjBase* aPiece = new ObjBase();
		aPiece->mAnim = Anim::Get("images/victory/win_confetti", ANIM_SCALE_FAST, false, 0x27, 0x2f, 2, false);
		aPiece->SetParent(this, nullptr);
		aPiece->SetStateInt(STATE_X, (int)(GameRandomNext() % 0x3c - 0x10) * 0x3c);
		aPiece->SetStateInt(STATE_Y, 0);   // the original passes the register the call before left, the channel 0
		int aSize = i & 3;
		aPiece->SetStateInt(STATE_Z, aSize + 1);
		float aScale = (float)((double)aSize * 0.0625 + 0.5);
		aPiece->SetState(STATE_SCALE_X, aScale);
		aPiece->SetState(STATE_SCALE_Y, aScale);
		switch (aSize) {
		case 1:
			aPiece->SetStateInt(STATE_RED, 1);
			aPiece->SetStateInt(STATE_GREEN, 1);
			aPiece->SetStateInt(STATE_BLUE, 1);
			break;
		case 2:
			aPiece->SetStateInt(STATE_RED, 0);
			aPiece->SetStateInt(STATE_GREEN, 0);
			aPiece->SetStateInt(STATE_BLUE, 1);
			break;
		case 3:
			aPiece->SetStateInt(STATE_RED, 1);
			aPiece->SetStateInt(STATE_GREEN, 0);
			aPiece->SetStateInt(STATE_BLUE, 0);
			break;
		default:
			aPiece->SetStateInt(STATE_RED, 0);
			aPiece->SetStateInt(STATE_GREEN, 1);
			aPiece->SetStateInt(STATE_BLUE, 0);
			break;
		}
		aPiece->SetStateInt(STATE_ALPHA, 1);
		aPiece->SetEventTarget(this);

		AnimKey* aFall = new AnimKey();
		aFall->SetDuration(i + 0x10e);
		aFall->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aFall->AddModifier(STATE_X, false, 0.0f, MOD_SINE, 2.0f, 32.0f, 0.0f, nullptr);
		aFall->AddValue(STATE_Y, -700.0f, MOD_TOWARD_NEXT);
		aFall->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, (float)(aSize + 10), 2.0f, 0.0f, nullptr);
		aFall->mEndCondition = KEY_END_ABOVE;     // 0x4533b0: the key ends when the piece is lower than 1200
		aFall->mEndChannel = STATE_Y;
		aFall->mEndValue = 1200.0f;

		AnimKey* aAgain = new AnimKey();
		aAgain->SetDuration(0);
		aAgain->mEventId = VICTORY_EVENT_CONFETTI;
		aAgain->AddValue(STATE_X, 0.0f, MOD_HOLD);
		aAgain->AddValue(STATE_Y, 720.0f, MOD_HOLD);

		AnimTrack* aTrack = new AnimTrack();
		aTrack->SetDelay((i % 40) * 5, false);
		aTrack->AddKey(aFall, nullptr);
		aTrack->AddKey(aAgain, nullptr);
		aPiece->AddTrack(aTrack, nullptr);
		aPiece->StartTrack(0);
		Activate(aPiece, false);
	}
}

// 0x474730: a stream of four hearts, children of a holder placed from the corner at (114, 580) at depth 0.01 (itself
// an object with no picture). Each heart is put at a random place, 96 across and 32 high at most, at a depth that
// orders them, and invisible; its track waits 35 frames more than the heart before, swells it in (alpha to 1, 0.9 of
// its size) in 8 and 5 frames, holds 200 frames while it sinks to 0.8 and 0.8, and ends with a key that asks the scene
// (Notify) to move it and start it over, after it rose 128 and went clear.
void Victory::BuildHearts()
{
	ObjBase* aHolder = new ObjBase();
	aHolder->SetParent(this, nullptr);
	aHolder->SetDesignX(114.0f);
	aHolder->SetDesignY(580.0f);
	aHolder->SetState(STATE_Z, 0.01f);
	Activate(aHolder, false);

	int aDelay = 0;
	for (int i = 0; i < VICTORY_HEARTS; i++) {
		ObjBase* aHeart = new ObjBase();
		aHeart->mAnim = Anim::Get("images/victory/win_heart", ANIM_SCALE_FAST, false, 0, 0, 1, false);
		aHeart->SetParent(aHolder, nullptr);
		aHeart->SetStateInt(STATE_X, (int)(GameRandomNext() % 0x31) * 2 - 0x30);
		aHeart->SetStateInt(STATE_Y, (int)(GameRandomNext() % 0x11) * 2 - 0x10);
		aHeart->SetState(STATE_Z, (float)((double)i * 0.03125 + 0.5));
		aHeart->SetStateInt(STATE_ALPHA, 0);
		aHeart->SetStateInt(STATE_SCALE_X, 0);
		aHeart->SetStateInt(STATE_SCALE_Y, 0);
		aHeart->SetEventTarget(this);

		AnimKey* aStart = new AnimKey();
		aStart->SetDuration(8);
		aStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aStart->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aStart->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		aStart->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aStart->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);

		AnimKey* aShow = new AnimKey();
		aShow->SetDuration(5);
		aShow->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aShow->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aShow->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
		aShow->AddValue(STATE_SCALE_X, 0.9f, MOD_TOWARD_NEXT);
		aShow->AddValue(STATE_SCALE_Y, 0.9f, MOD_TOWARD_NEXT);

		AnimKey* aFloat = new AnimKey();
		aFloat->SetDuration(200);
		aFloat->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aFloat->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aFloat->AddValue(STATE_ALPHA, 0.8f, MOD_TOWARD_NEXT);
		aFloat->AddValue(STATE_SCALE_X, 0.8f, MOD_TOWARD_NEXT);
		aFloat->AddValue(STATE_SCALE_Y, 0.8f, MOD_TOWARD_NEXT);

		AnimKey* aEnd = new AnimKey();
		aEnd->SetDuration(1);
		aEnd->mEventId = VICTORY_EVENT_HEART;
		aEnd->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aEnd->AddValue(STATE_Y, -128.0f, MOD_TOWARD_NEXT);
		aEnd->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aEnd->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aEnd->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);

		AnimTrack* aTrack = new AnimTrack();
		aTrack->SetDelay(aDelay, false);
		aTrack->AddKey(aStart, nullptr);
		aTrack->AddKey(aShow, nullptr);
		aTrack->AddKey(aFloat, nullptr);
		aTrack->AddKey(aEnd, nullptr);
		aHeart->AddTrack(aTrack, nullptr);
		aHeart->StartTrack(0);
		Activate(aHeart, false);
		aDelay += VICTORY_HEART_SPACING;
	}
}

// 0x474cc0: the banner, a child of the engine's root (no parent), at 280 left and depth 420: its track waits at 700
// above for the scene, drops into place in 60 frames (a square ease), stays 400, then 22 more, and leaves again to
// 700 above in a key of one frame (a straight ease). It is made inactive: Show brings it in with the scene.
void Victory::BuildSign()
{
	ObjBase* aSign = new ObjBase();
	mSign = aSign;
	aSign->mAnim = Anim::Get("images/victory/win_sign", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	aSign->SetParent(nullptr, nullptr);
	aSign->SetStateInt(STATE_Y, -280);
	aSign->SetStateInt(STATE_Z, 420);

	AnimKey* aDrop = new AnimKey();
	aDrop->SetDuration(0x3c);
	aDrop->AddValue(STATE_Y, -700.0f, MOD_TOWARD_NEXT);
	aDrop->mEasing = EASE_IN_SQUARE;
	AnimKey* aStay = new AnimKey();
	aStay->SetDuration(400);
	aStay->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aStayMore = new AnimKey();
	aStayMore->SetDuration(0x16);
	aStayMore->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aLeave = new AnimKey();
	aLeave->SetDuration(1);
	aLeave->AddValue(STATE_Y, -700.0f, MOD_TOWARD_NEXT);
	aLeave->mEasing = EASE_LINEAR;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aDrop, nullptr);
	aTrack->AddKey(aStay, nullptr);
	aTrack->AddKey(aStayMore, nullptr);
	aTrack->AddKey(aLeave, nullptr);
	aSign->AddTrack(aTrack, nullptr);
	Deactivate(aSign);
	aSign->StartTrack(0);
}

// 0x473630 (the object whose key ended is theObj, the one asked to be told: the scene): a heart that has risen goes
// to a new place (up to 64 each way) and starts over; a piece of confetti that has fallen goes to a new place across
// the top and starts over, and is updated at once
void Victory::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anX;
	int aY;
	switch (theObj->mCurrentKey->mEventId) {
	case VICTORY_EVENT_HEART:
		aY = (int)(GameRandomNext() % 0x81) - 0x40;
		theObj->SetStateInt(STATE_Y, aY);
		anX = (int)(GameRandomNext() % 0x81) - 0x40;
		break;
	case VICTORY_EVENT_CONFETTI:
		theObj->SetStateInt(STATE_Y, 0);
		anX = (int)(GameRandomNext() % 0x781) - 0x3c0;
		break;
	default:
		return;
	}
	theObj->SetStateInt(STATE_X, anX);
	theObj->StartTrack(0);
	theObj->Update();
}

// 0x4736c0: for the first 300 frames nothing happens. Then, until the press: every 30 frames a rocket goes up at a
// random place along the bottom if one is free, every 70 a monkey calls (eight draws of twelve), and a press (the mouse
// held, as sMouseHeld) starts the fade-away and ends the scene.
bool Victory::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mDelay > 0) {
		mDelay--;
		return true;
	}
	if (mDone)
		return true;
	if (sFrame % VICTORY_FIREWORK_PERIOD == 0) {
		for (int i = 0; i < VICTORY_FIREWORKS; i++) {
			if (!mFireWorks[i]->IsActive()) {
				unsigned int aRandom = GameRandomNext();
				mFireWorks[i]->Launch((float)(int)(aRandom % VICTORY_FIREWORK_SPREAD - VICTORY_FIREWORK_MARGIN),
					VICTORY_FIREWORK_Y);
				break;
			}
		}
	}
	if (sFrame % VICTORY_SOUND_PERIOD == 0) {
		int aCall = VICTORY_CALLS[GameRandomNext() % VICTORY_SOUND_DRAWS];
		if (aCall >= 0)
			gApp->mSoundQueue->Request(aCall, 0, false);
	}
	if (sMouseHeld) {
		sMouseHeld = false;
		StartTrack(VICTORY_TRACK_FADE);
		mDone = true;
	}
	return true;
}

// 0x473840 (EBX the scene): the scene and its banner come in, the victory song plays, the monkey (the player's object)
// is let go, and a press that was already held does not count
void Victory::Show()
{
	Activate(this, false);
	Activate(mSign, false);
	gApp->ChangeMusic(MUSIC_VICTORY);
	mMusicChanged = true;
	Deactivate(Player::sCurrent);
	sMouseHeld = false;
}
