#include "BugScore.h"

#include "BBFont.h"
#include "GameApp.h"

using namespace Sexy;

// 0x426990: in the game's font, above the bugs, waiting; "0000" only sizes the text, which holds four characters
BugScore::BugScore()
{
	SetFont(gApp->mFont);
	SetStateInt(STATE_Z, 210);
	Create("0000", false, 0);
	BuildTrack();
	Deactivate(this);
}

// 0x426a60: the text at the place, in yellow, from nothing. The original activates it hidden, as it does; the track's
// keys show it.
void BugScore::Show(const char* theText, float theX, float theY)
{
	SetState(STATE_X, theX);
	SetState(STATE_Y, theY);
	SetStateInt(STATE_RED, 1);
	SetStateInt(STATE_GREEN, 1);
	SetStateInt(STATE_BLUE, 0);
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	Activate(this, true);
	SetText(theText);
	StartTrack(mTrackIndex);
}

// 0x426b00: popping up for 20 frames with a wobble in its size, then 150 frames on a small circle as it fades from 0.8,
// then gone
void BugScore::BuildTrack()
{
	mPopKey = new AnimKey();
	mPopKey->SetDuration(20);
	mPopKey->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
	mPopKey->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	mPopKey->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	mPopKey->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	mPopKey->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	mPopKey->mEasing = EASE_IN_CUBE;

	mCircleKey = new AnimKey();
	mCircleKey->SetDuration(150);
	mCircleKey->AddValue(STATE_ALPHA, 0.8f, MOD_TOWARD_NEXT);
	mCircleKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	mCircleKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	mCircleKey->AddValue(STATE_ORBIT_RADIUS, 1.0f, MOD_HOLD);
	mCircleKey->AddValue(STATE_ORBIT_ANGLE, 1.5708f, MOD_HOLD);
	mCircleKey->AddValue(STATE_SCALE_X, 0.8f, MOD_HOLD);
	mCircleKey->AddValue(STATE_SCALE_Y, 0.8f, MOD_HOLD);

	mGoneKey = new AnimKey();
	mGoneKey->SetDuration(1);
	mGoneKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	mGoneKey->AddValue(STATE_ORBIT_RADIUS, 1.0f, MOD_HOLD);

	mTrack = new AnimTrack();
	mTrack->AddKey(mPopKey, nullptr);
	mTrack->AddKey(mCircleKey, nullptr);
	mTrack->AddKey(mGoneKey, nullptr);
	mTrackIndex = AddTrack(mTrack, nullptr);
}
