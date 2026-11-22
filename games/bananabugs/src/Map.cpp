#include "Map.h"

#include "Anim.h"
#include "GameApp.h"
#include "SoundQueue.h"

#include <cstdio>

using namespace Sexy;

Anim* Map::sPlayAnim = nullptr;
Anim* Map::sMapAnim = nullptr;
Anim* Map::sDotAnim = nullptr;
Anim* Map::sStampAnim = nullptr;
Anim* Map::sIconAnims[MAP_LEVELS];

// The key events of the map's own tracks and of the stamps
enum MapEvent
{
	MAP_EVENT_OPENING = 1,   // the map unfolds: its sound
	MAP_EVENT_STAMPED = 2,   // an X has landed: the next one, and the trail to it
	MAP_EVENT_CLOSED = 4,
};

// The map's tracks and the play button's
enum MapTrack
{
	MAP_TRACK_OPEN = 0,
	MAP_TRACK_CLOSE = 1,
	PLAY_TRACK_HOVER = 1,
	PLAY_TRACK_LEAVE = 2,
};

// Where each level's icon sits on the map, in design pixels from the corner
static const float LEVEL_ICON_PLACES[MAP_LEVELS][2] = {
	{985.0f, 902.0f}, {1167.0f, 769.0f}, {977.0f, 582.0f}, {1247.0f, 512.0f}, {1381.0f, 656.0f},
	{1515.0f, 427.0f}, {1177.0f, 330.0f}, {931.0f, 370.0f}, {673.0f, 328.0f}, {467.0f, 282.0f},
	{517.0f, 504.0f}, {773.0f, 525.0f}, {683.0f, 704.0f}, {435.0f, 696.0f},
};

// 0x4449c0: made hidden; Start shows it
Map::Map()
{
	mDone = false;
	mClicked = false;
	// the original leaves these unset until Start
	mStampedLevel = 0;
	mFromLevel = -1;
	mToLevel = -1;
	mWalkAngle = 0.0f;
	mWalkStepX = 0.0f;
	mWalkStepY = 0.0f;
	mWalkSteps = 0.0f;
	mTrailCount = 0;
	mWalkCount = 0;
	mWalkX = 0.0f;
	mWalkY = 0.0f;
	mWalkToX = 0.0f;
	mWalkToY = 0.0f;
	mDotCount = 0;
	mWalkDelay = 0;
	BuildMap();
	BuildIcons();
	BuildDots();
	BuildStamps();
	BuildPlayButton();
	Deactivate(this);
}

// 0x444a60
Map::~Map()
{
	SetHoverMinZ(0.0f);
}

// 0x444a80: the levels before theFromLevel brighter, those after theToLevel dimmer; the stamps start at the first
// level, and the footprints from theFromLevel's icon (or beside theToLevel's, for none) to theToLevel's
void Map::Start(int theFromLevel, int theToLevel)
{
	mDotCount = 0;
	mWalkDelay = 50;
	mTrailCount = 0;
	mWalkCount = 0;
	mStampedLevel = 0;
	mFromLevel = theFromLevel;
	mToLevel = theToLevel;
	ObjBase* aTo = mIcons[theToLevel];
	if (theFromLevel >= 0) {
		ObjBase* aFrom = mIcons[theFromLevel];
		SetWalk(aFrom->mState[STATE_X], aFrom->mState[STATE_Y], aTo->mState[STATE_X], aTo->mState[STATE_Y]);
	} else {
		SetWalk(aTo->mState[STATE_X], aTo->mState[STATE_Y], (float)(aTo->mState[STATE_X] + 23.0),
			(float)(aTo->mState[STATE_Y] + 23.0));
	}
	for (int i = 0; i < MAP_LEVELS; i++) {
		if (i < mFromLevel)
			mIcons[i]->SetState(STATE_ALPHA, 0.8f);
		else if (mToLevel < i)
			mIcons[i]->SetState(STATE_ALPHA, 0.4f);
		else
			mIcons[i]->SetStateInt(STATE_ALPHA, 1);
	}
	if (mToLevel > 0) {
		ObjBase* aStamp = mStamps[mStampedLevel];
		aStamp->SetState(STATE_X, mIcons[mStampedLevel]->mState[STATE_X]);
		aStamp->SetState(STATE_Y, mIcons[mStampedLevel]->mState[STATE_Y]);
		Activate(aStamp, false);
	}
	StartTrack(MAP_TRACK_OPEN);
	Activate(this, false);
}

// 0x444c20
void Map::Notify(ObjBase* theObj)
{
	AnimKey* aKey = theObj->mCurrentKey;
	if (aKey == nullptr)
		return;
	if (aKey->mEventId == MAP_EVENT_OPENING) {
		sApp->mSoundQueue->Request(0x5b, 0, false);
		sMouseHeld = false;
	} else if (aKey->mEventId == MAP_EVENT_STAMPED) {
		mStampedLevel++;
		if (mStampedLevel <= mFromLevel) {
			ObjBase* aStamp = mStamps[mStampedLevel];
			aStamp->SetState(STATE_X, mIcons[mStampedLevel]->mState[STATE_X]);
			aStamp->SetState(STATE_Y, mIcons[mStampedLevel]->mState[STATE_Y]);
			Activate(aStamp, false);
			AddTrail(mStampedLevel - 1, mStampedLevel);
			sApp->mSoundQueue->Request(0x5c, 0, false);
		}
	} else if (aKey->mEventId == MAP_EVENT_CLOSED) {
		mDone = true;
	}
}

// 0x444d20: after a wait the footprints walk a pixel a frame, a print every 32 (the last ones lit, as dots); the play
// button lights under the mouse
bool Map::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mDone)
		return true;
	if (mToLevel >= 0 && 0.0f < mWalkSteps) {
		if (mWalkDelay < 1) {
			if (mWalkCount % 32 == 0) {
				if (32.0f <= mWalkSteps) {
					AddDot(mWalkX, mWalkY, 4.0f, -0.02f, 0.5f, mWalkAngle, 1.0f);
					sApp->mSoundQueue->Request(3, 0, false);
				} else {
					AddDot(mWalkX, mWalkY, 4.0f, 0.0f, 0.8f, 0.0f, 2.0f);
					sApp->mSoundQueue->Request(6, 0, false);
				}
			}
			mWalkCount++;
			mWalkX = mWalkStepX + mWalkX;
			mWalkY = mWalkStepY + mWalkY;
			mWalkSteps = (float)(mWalkSteps - 1.0);
		} else {
			mWalkDelay--;
		}
	}
	ObjBase* aButton = mPlayButton;
	if (aButton != nullptr) {
		if (aButton == sHover) {
			if (aButton->mHoverState == 0) {
				aButton->StartTrack(PLAY_TRACK_HOVER);
				mPlayButton->mHoverState = 1;
				sApp->mSoundQueue->Request(3, 0, false);
				return true;
			}
		} else if (aButton->mHoverState == 1) {
			aButton->StartTrack(PLAY_TRACK_LEAVE);
			mPlayButton->mHoverState = 0;
		}
	}
	return true;
}

// 0x444f10: the play button's click (the map listens for it) folds the map away
bool Map::MouseUp(ObjBase* theObj)
{
	(void)theObj;
	if (mClicked)
		return false;
	sApp->mSoundQueue->Request(0x3a, 0, false);
	mClicked = true;
	StartTrack(MAP_TRACK_CLOSE);
	return true;
}

// 0x444f50
void Map::SetWalk(float theFromX, float theFromY, float theToX, float theToY)
{
	float aDeltaX = theToX - theFromX;
	mWalkAngle = FloatAtan2(theFromY - theToY, aDeltaX);
	float aDeltaY = theToY - theFromY;
	float anAbsX = FloatAbs(aDeltaX);
	float anAbsY = FloatAbs(aDeltaY);
	mWalkSteps = anAbsY < anAbsX ? anAbsX : anAbsY;
	mWalkStepX = aDeltaX / mWalkSteps;
	mWalkStepY = aDeltaY / mWalkSteps;
	mWalkX = theFromX;
	mWalkY = theFromY;
	mWalkToX = theToX;
	mWalkToY = theToY;
}

// 0x445030: a dot every 32 pixels from one icon to the next, the count running on from the last trail
void Map::AddTrail(int theFromLevel, int theToLevel)
{
	ObjBase* aFrom = mIcons[theFromLevel];
	float aX = aFrom->mState[STATE_X];
	ObjBase* aTo = mIcons[theToLevel];
	float aY = aFrom->mState[STATE_Y];
	float aToY = aTo->mState[STATE_Y];
	float aDeltaX = aTo->mState[STATE_X] - aX;
	float anAngle = FloatAtan2(aY - aToY, aDeltaX);
	float aDeltaY = aToY - aY;
	float anAbsX = FloatAbs(aDeltaX);
	float anAbsY = FloatAbs(aDeltaY);
	float aSteps = anAbsY < anAbsX ? anAbsX : anAbsY;
	float aStepX = aDeltaX / aSteps;
	float aStepY = aDeltaY / aSteps;
	if (!(0.0f < aSteps))
		return;
	do {
		aSteps = (float)(aSteps - 1.0);
		if (mTrailCount % 32 == 0)
			AddDot(aX, aY, 1.0f, 0.0f, 0.3f, anAngle, 0.0f);
		mTrailCount++;
		aX = aStepX + aX;
		aY = aStepY + aY;
	} while (0.0f <= aSteps);
}

// 0x4451c0: the next of the 128 dots, if any is left; the footprint cel pulses, the others pop in
void Map::AddDot(float theX, float theY, float theZ, float theScale, float theAlpha, float theRotation, float theCel)
{
	if (mDotCount >= MAP_MAX_DOTS)
		return;
	ObjBase* aDot = mDots[mDotCount];
	mDotCount++;
	aDot->SetState(STATE_X, theX);
	aDot->SetState(STATE_Y, theY);
	aDot->SetState(STATE_Z, theZ);
	aDot->SetState(STATE_ALPHA, theAlpha);
	aDot->SetState(STATE_SCALE_X, theScale);
	aDot->SetState(STATE_SCALE_Y, theScale);
	aDot->SetState(STATE_FRAME, theCel);
	aDot->SetState(STATE_ROTATION, theRotation);
	aDot->StartTrack(theCel == 2.0f ? 1 : 0);
	Activate(aDot, false);
}

// 0x4452b0: the map unfolds (squashed flat, then opening over 30 frames) and holds; its close track folds it
void Map::BuildMap()
{
	SetHoverMinZ(300.0f);
	sMapAnim->AddRef();
	mAnim = sMapAnim;
	SetMouseable(32.0f, 32.0f);
	SetStateInt(STATE_Z, 300);
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);

	AnimKey* aFlat = new AnimKey();
	aFlat->SetDuration(2);
	aFlat->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFlat->AddValue(STATE_SCALE_Y, 0.9f, MOD_TOWARD_NEXT);
	aFlat->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* anOpening = new AnimKey();
	anOpening->mEventId = MAP_EVENT_OPENING;
	anOpening->SetDuration(30);
	anOpening->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	anOpening->AddValue(STATE_SCALE_Y, 0.9f, MOD_TOWARD_NEXT);
	anOpening->AddValue(STATE_ALPHA, 0.4f, MOD_TOWARD_NEXT);
	AnimKey* anOpen = new AnimKey();
	anOpen->SetDuration(40);
	anOpen->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	anOpen->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	anOpen->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimTrack* anOpenTrack = new AnimTrack();
	anOpenTrack->AddKey(aFlat, nullptr);
	anOpenTrack->AddKey(anOpening, nullptr);
	anOpenTrack->AddKey(anOpen, anOpen);
	AddTrack(anOpenTrack, nullptr);

	AnimKey* aClosing = new AnimKey();
	aClosing->SetDuration(20);
	aClosing->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aClosing->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aClosing->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aClosed = new AnimKey();
	aClosed->mEventId = MAP_EVENT_CLOSED;
	aClosed->SetDuration(40);
	aClosed->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aClosed->AddValue(STATE_SCALE_Y, 0.9f, MOD_HOLD);
	aClosed->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	AnimTrack* aCloseTrack = new AnimTrack();
	aCloseTrack->AddKey(aClosing, nullptr);
	aCloseTrack->AddKey(aClosed, nullptr);
	AddTrack(aCloseTrack, nullptr);
}

// 0x4456b0
void Map::BuildIcons()
{
	for (int i = 0; i < MAP_LEVELS; i++) {
		ObjBase* anIcon = new ObjBase();
		mIcons[i] = anIcon;
		anIcon->SetParent(this, nullptr);
		sIconAnims[i]->AddRef();
		anIcon->mAnim = sIconAnims[i];
		anIcon->SetDesignX(LEVEL_ICON_PLACES[i][0]);
		anIcon->SetDesignY(LEVEL_ICON_PLACES[i][1]);
		anIcon->SetState(STATE_Z, 1.1f);
		anIcon->SetStateInt(STATE_ALPHA, 1);
		anIcon->SetStateInt(STATE_SCALE_X, 1);
		anIcon->SetStateInt(STATE_SCALE_Y, 1);
		Activate(anIcon, false);
	}
}

// 0x4458c0: each dot pops in and holds (track 0) or pulses (track 1)
void Map::BuildDots()
{
	for (int i = 0; i < MAP_MAX_DOTS; i++) {
		ObjBase* aDot = new ObjBase();
		mDots[i] = aDot;
		aDot->SetParent(this, nullptr);
		sDotAnim->AddRef();
		aDot->mAnim = sDotAnim;
		aDot->SetStateInt(STATE_Z, 2);
		aDot->SetStateInt(STATE_ALPHA, 0);
		aDot->SetStateInt(STATE_SCALE_X, 0);
		aDot->SetStateInt(STATE_SCALE_Y, 0);

		AnimKey* aPop = new AnimKey();
		aPop->SetDuration(5);
		aPop->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
		aPop->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
		aPop->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
		AnimKey* aHold = new AnimKey();
		aHold->SetDuration(100);
		aHold->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
		aHold->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
		aHold->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		AnimTrack* aPopTrack = new AnimTrack();
		aPopTrack->AddKey(aPop, nullptr);
		aPopTrack->AddKey(aHold, aHold);
		aDot->AddTrack(aPopTrack, nullptr);

		AnimKey* aSwell = new AnimKey();
		aSwell->SetDuration(40);
		aSwell->AddValue(STATE_SCALE_X, 0.7f, MOD_TOWARD_NEXT);
		aSwell->AddModifier(STATE_SCALE_X, false, 1.0f, MOD_SINE, 1.0f, 1.8f, 0.0f, nullptr);
		aSwell->AddValue(STATE_SCALE_Y, 0.7f, MOD_TOWARD_NEXT);
		aSwell->AddModifier(STATE_SCALE_Y, false, 1.0f, MOD_SINE, 1.0f, 1.8f, 0.0f, nullptr);
		aSwell->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aFade = new AnimKey();
		aFade->SetDuration(30);
		aFade->AddValue(STATE_SCALE_X, 0.7f, MOD_TOWARD_NEXT);
		aFade->AddValue(STATE_SCALE_Y, 0.7f, MOD_TOWARD_NEXT);
		aFade->AddValue(STATE_ALPHA, -0.7f, MOD_TOWARD_NEXT);
		AnimTrack* aPulseTrack = new AnimTrack();
		aPulseTrack->AddKey(aSwell, nullptr);
		aPulseTrack->AddKey(aFade, aSwell);
		aDot->AddTrack(aPulseTrack, nullptr);
		Deactivate(aDot);
	}
}

// 0x445ca0: each X lands from three times its size after 40 frames, and tells the map as it does
void Map::BuildStamps()
{
	for (int i = 0; i < MAP_LEVELS; i++) {
		ObjBase* aStamp = new ObjBase();
		mStamps[i] = aStamp;
		aStamp->SetParent(this, nullptr);
		sStampAnim->AddRef();
		aStamp->mAnim = sStampAnim;
		aStamp->SetStateInt(STATE_Z, 3);
		aStamp->SetStateInt(STATE_ALPHA, 0);
		aStamp->SetStateInt(STATE_SCALE_X, 0);
		aStamp->SetStateInt(STATE_SCALE_Y, 0);
		aStamp->SetEventTarget(this);

		AnimKey* aDrop = new AnimKey();
		aDrop->SetDuration(10);
		aDrop->AddValue(STATE_SCALE_X, 3.0f, MOD_TOWARD_NEXT);
		aDrop->AddValue(STATE_SCALE_Y, 3.0f, MOD_TOWARD_NEXT);
		aDrop->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aLanding = new AnimKey();
		aLanding->SetDuration(5);
		aLanding->AddValue(STATE_SCALE_X, 0.9f, MOD_HOLD);
		aLanding->AddValue(STATE_SCALE_Y, 0.9f, MOD_HOLD);
		aLanding->AddValue(STATE_ALPHA, 0.4f, MOD_HOLD);
		AnimKey* aLanded = new AnimKey();
		aLanded->mEventId = MAP_EVENT_STAMPED;
		aLanded->SetDuration(1);
		aLanded->AddValue(STATE_SCALE_X, 1.1f, MOD_HOLD);
		aLanded->AddValue(STATE_SCALE_Y, 1.1f, MOD_HOLD);
		aLanded->AddValue(STATE_ALPHA, 0.6f, MOD_HOLD);
		AnimKey* aHold = new AnimKey();
		aHold->SetDuration(50);
		aHold->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
		aHold->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
		aHold->AddValue(STATE_ALPHA, 0.6f, MOD_HOLD);
		AnimTrack* aTrack = new AnimTrack();
		aTrack->SetDelay(40, false);
		aTrack->AddKey(aDrop, nullptr);
		aTrack->AddKey(aLanding, nullptr);
		aTrack->AddKey(aLanded, nullptr);
		aTrack->AddKey(aHold, aHold);
		aStamp->AddTrack(aTrack, nullptr);
		aStamp->StartTrack(0);
		Deactivate(aStamp);
	}
}

// 0x446050: the play button, its picture taken without a reference; it grows a little under the mouse
void Map::BuildPlayButton()
{
	ObjBase* aButton = new ObjBase();
	mPlayButton = aButton;
	aButton->mAnim = sPlayAnim;
	aButton->mType = OBJ_TYPE_MAP_PLAY;
	aButton->SetParent(this, nullptr);
	aButton->SetStateInt(STATE_Y, 516);
	aButton->SetStateInt(STATE_Z, 1);
	aButton->SetMouseable(32.0f, 32.0f);
	aButton->SetMouseListener(this);
	Activate(aButton, false);

	AnimKey* aRest = new AnimKey();
	aRest->mEventId = 0;
	aRest->SetDuration(50);
	aRest->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aRest->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aRestTrack = new AnimTrack();
	aRestTrack->AddKey(aRest, aRest);
	aButton->AddTrack(aRestTrack, nullptr);

	AnimKey* aGrowing = new AnimKey();
	aGrowing->SetDuration(5);
	aGrowing->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrowing->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aGrown = new AnimKey();
	aGrown->SetDuration(2);
	aGrown->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aGrown->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	AnimTrack* aHoverTrack = new AnimTrack();
	aHoverTrack->AddKey(aGrowing, nullptr);
	aHoverTrack->AddKey(aGrown, aGrown);
	aButton->AddTrack(aHoverTrack, nullptr);

	AnimKey* aShrinking = new AnimKey();
	aShrinking->SetDuration(5);
	aShrinking->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aShrinking->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	AnimKey* aShrunk = new AnimKey();
	aShrunk->SetDuration(2);
	aShrunk->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aShrunk->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aLeaveTrack = new AnimTrack();
	aLeaveTrack->AddKey(aShrinking, nullptr);
	aLeaveTrack->AddKey(aShrunk, aShrunk);
	aButton->AddTrack(aLeaveTrack, nullptr);
}

// 0x4463c0
void Sexy::PreloadMap()
{
	Map::sPlayAnim = Anim::Get("images/map/map_play", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	Map::sDotAnim = Anim::Get("images/map/map_dot", ANIM_SCALE_SMOOTH, false, 0x23, 0x23, 3, false);
	Map::sMapAnim = Anim::Get("images/map/map", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	Map::sStampAnim = Anim::Get("images/map/map_X", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	char aName[32];
	int i = 0;
	do {
		int aNumber = i + 1;
		sprintf(aName, "images/map/map_icon%02d", aNumber);
		Map::sIconAnims[i] = Anim::Get(aName, ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
		i = aNumber;
	} while (i < MAP_LEVELS);
}
