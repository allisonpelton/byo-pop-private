#include "Help.h"

#include "Anim.h"

using namespace Sexy;

// The help pictures' tracks, and the event the bottom one's leaving ends with
enum HelpTrack
{
	HELP_TRACK_IN = 0,
	HELP_TRACK_OUT = 1,
};
const int HELP_EVENT_GONE = 99;

// 0x434700: made hidden; Start builds and shows it
Help::Help()
{
	mClosing = false;
	mHoverMinZBefore = GetHoverMinZ();
	// the original leaves these unset until Start
	mTopLeft = nullptr;
	mTopRight = nullptr;
	mBottom = nullptr;
	Deactivate(this);
}

// 0x434750
Help::~Help()
{
	SetHoverMinZ(mHoverMinZBefore);
}

// 0x434770: above everything at depth 100, and the mouse goes to nothing below it
void Help::Start()
{
	SetStateInt(STATE_Z, 100);
	ComputeDrawn();
	mHoverMinZBefore = GetHoverMinZ();
	SetHoverMinZ(mDrawn[STATE_Z]);
	BuildBottom();
	BuildTopLeft();
	BuildTopRight();
	mTopRight->StartTrack(HELP_TRACK_IN);
	Activate(mTopRight, false);
	mTopLeft->StartTrack(HELP_TRACK_IN);
	Activate(mTopLeft, false);
	mBottom->StartTrack(HELP_TRACK_IN);
	Activate(mBottom, false);
	Activate(this, false);
	SetMouseable(32.0f, 32.0f);
	SetMouseListener(this);
}

// 0x434840
void Help::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr && theObj->mCurrentKey->mEventId == HELP_EVENT_GONE)
		Delete();
}

// 0x434860: the first click slides the pictures out
bool Help::MouseUp(ObjBase* theObj)
{
	(void)theObj;
	if (!mClosing) {
		mClosing = true;
		mTopRight->StartTrack(HELP_TRACK_OUT);
		mTopLeft->StartTrack(HELP_TRACK_OUT);
		mBottom->StartTrack(HELP_TRACK_OUT);
	}
	return true;
}

// 0x4348c0: in from 700 pixels above over 15 frames
void Help::BuildTopLeft()
{
	ObjBase* aPart = new ObjBase();
	mTopLeft = aPart;
	aPart->SetParent(this, nullptr);
	mTopLeft->mAnim = Anim::Get("images/help/help_topleft", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mTopLeft->SetStateInt(STATE_X, -280);
	mTopLeft->SetStateInt(STATE_Y, -400);
	mTopLeft->SetStateInt(STATE_Z, 1);
	mTopLeft->SetMouseListener(this);
	mTopLeft->SetMouseable(32.0f, 32.0f);

	AnimKey* aDropping = new AnimKey();
	aDropping->SetDuration(15);
	aDropping->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aDropping->AddValue(STATE_Y, -700.0f, MOD_TOWARD_NEXT);
	AnimKey* aDown = new AnimKey();
	aDown->SetDuration(1);
	aDown->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aDown->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	AnimTrack* anInTrack = new AnimTrack();
	anInTrack->AddKey(aDropping, nullptr);
	anInTrack->AddKey(aDown, aDown);
	mTopLeft->AddTrack(anInTrack, nullptr);

	AnimKey* aRising = new AnimKey();
	aRising->SetDuration(12);
	aRising->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aRising->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* anUp = new AnimKey();
	anUp->SetDuration(1);
	anUp->AddValue(STATE_X, 0.0f, MOD_HOLD);
	anUp->AddValue(STATE_Y, -700.0f, MOD_HOLD);
	AnimTrack* anOutTrack = new AnimTrack();
	anOutTrack->AddKey(aRising, nullptr);
	anOutTrack->AddKey(anUp, nullptr);
	mTopLeft->AddTrack(anOutTrack, nullptr);
}

// 0x434ba0: the same over 25 frames, each way 25 frames after the left one
void Help::BuildTopRight()
{
	ObjBase* aPart = new ObjBase();
	mTopRight = aPart;
	aPart->SetParent(this, nullptr);
	mTopRight->mAnim = Anim::Get("images/help/help_topright", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mTopRight->SetStateInt(STATE_X, 280);
	mTopRight->SetStateInt(STATE_Y, -400);
	mTopRight->SetStateInt(STATE_Z, 1);
	mTopRight->SetMouseListener(this);
	mTopRight->SetMouseable(32.0f, 32.0f);

	AnimKey* aDropping = new AnimKey();
	aDropping->SetDuration(25);
	aDropping->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aDropping->AddValue(STATE_Y, -700.0f, MOD_TOWARD_NEXT);
	AnimKey* aDown = new AnimKey();
	aDown->SetDuration(1);
	aDown->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aDown->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	AnimTrack* anInTrack = new AnimTrack();
	anInTrack->AddKey(aDropping, nullptr);
	anInTrack->AddKey(aDown, aDown);
	anInTrack->SetDelay(25, false);
	mTopRight->AddTrack(anInTrack, nullptr);

	AnimKey* aRising = new AnimKey();
	aRising->SetDuration(12);
	aRising->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aRising->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* anUp = new AnimKey();
	anUp->SetDuration(1);
	anUp->AddValue(STATE_X, 0.0f, MOD_HOLD);
	anUp->AddValue(STATE_Y, -700.0f, MOD_HOLD);
	AnimTrack* anOutTrack = new AnimTrack();
	anOutTrack->AddKey(aRising, nullptr);
	anOutTrack->AddKey(anUp, nullptr);
	anOutTrack->SetDelay(25, false);
	mTopRight->AddTrack(anOutTrack, nullptr);
}

// 0x434ea0: up from 700 pixels below, easing in, after 50 frames; its leaving ends the screen
void Help::BuildBottom()
{
	ObjBase* aPart = new ObjBase();
	mBottom = aPart;
	aPart->SetParent(this, nullptr);
	mBottom->mAnim = Anim::Get("images/help/help_bottom", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mBottom->SetStateInt(STATE_X, 0);
	mBottom->SetStateInt(STATE_Y, 270);
	mBottom->SetStateInt(STATE_Z, 1);
	mBottom->SetEventTarget(this);
	mBottom->SetMouseListener(this);
	mBottom->SetMouseable(32.0f, 32.0f);

	AnimKey* aRising = new AnimKey();
	aRising->SetDuration(25);
	aRising->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aRising->AddValue(STATE_Y, 700.0f, MOD_TOWARD_NEXT);
	aRising->mEasing = EASE_IN_SQUARE;
	AnimKey* anUp = new AnimKey();
	anUp->SetDuration(1);
	anUp->AddValue(STATE_X, 0.0f, MOD_HOLD);
	anUp->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	AnimTrack* anInTrack = new AnimTrack();
	anInTrack->AddKey(aRising, nullptr);
	anInTrack->AddKey(anUp, anUp);
	anInTrack->SetDelay(50, false);
	mBottom->AddTrack(anInTrack, nullptr);

	AnimKey* aFalling = new AnimKey();
	aFalling->SetDuration(12);
	aFalling->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aFalling->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	aGone->mEventId = HELP_EVENT_GONE;
	aGone->SetDuration(1);
	aGone->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_Y, 700.0f, MOD_HOLD);
	AnimTrack* anOutTrack = new AnimTrack();
	anOutTrack->AddKey(aFalling, nullptr);
	anOutTrack->AddKey(aGone, nullptr);
	anOutTrack->SetDelay(50, false);
	mBottom->AddTrack(anOutTrack, nullptr);
}
