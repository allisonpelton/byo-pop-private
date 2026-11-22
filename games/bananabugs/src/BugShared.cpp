#include "BugShared.h"

#include "BugImg.h"
#include "PipeObj.h"

using namespace Sexy;

// The fade track: the bug shrinks away over this many frames
static const int FADE_FRAMES = 45;

// 0x41c640 (slot 5; one function for the kinds that name it): the base's update
bool Sexy::BugSharedUpdate(BugObj* theBug)
{
	return theBug->BugObj::Update();
}

// 0x41c620 (slot 15): the bug is updated, then its body
void Sexy::BugSharedStepSegment(BugObj* theBug)
{
	theBug->Update();
	// port (crash): the original does not look whether the bug has a body
	if (theBug->mBody != nullptr)
		theBug->mBody->Update();
}

// 0x415810 (slot 17): leaves play, and the fade track starts
void Sexy::BugSharedVanish(BugObj* theBug)
{
	theBug->BugObj::Vanish();
	theBug->StartTrack(theBug->mFadeTrack);
}

// 0x417aa0 (slot 18): the bug and its column are held and the body is told what for: freezing is a dance,
// dazing shows the stars, shock the ring. The original returns false always, whatever the base said
bool Sexy::BugSharedSetCommand(BugObj* theBug, int theCommand, int theFrames)
{
	if (theBug->BugObj::SetCommand(theCommand, theFrames) && theBug->mBody != nullptr) {
		switch (theBug->mCommand) {
		case COMMAND_FREEZE:
			theBug->mBody->Slot13(BUG_CMD_FREEZE_ON, theFrames, 0);
			break;
		case COMMAND_DAZE:
			theBug->mBody->Slot13(BUG_CMD_STARS_ON, theFrames, 0);
			break;
		case COMMAND_SHOCK:
			theBug->mBody->Slot13(BUG_CMD_RING_ON, theFrames, 0);
			break;
		}
	}
	return false;
}

// 0x41e1f0 (slot 19): the hold ends and the body is told to stop what it was doing. The original returns 0 always,
// whatever command was waiting
int Sexy::BugSharedTakeCommand(BugObj* theBug)
{
	if (theBug->mAlive) {
		int aCommand = theBug->BugObj::TakeCommand();
		if (aCommand != 0 && theBug->mBody != nullptr) {
			switch (aCommand) {
			case COMMAND_FREEZE:
				theBug->mBody->Slot13(BUG_CMD_FREEZE_OFF, 0, 0);
				break;
			case COMMAND_DAZE:
				theBug->mBody->Slot13(BUG_CMD_STARS_OFF, 0, 0);
				break;
			case COMMAND_SHOCK:
				theBug->mBody->Slot13(BUG_CMD_RING_OFF, 0, 0);
				break;
			}
		}
	}
	return 0;
}

// 0x415b60 (slot 22): the fade track: 45 frames shrinking and fading away, then a key that ends with the event (the
// original's code has the 5 in it; the kinds that name it all pass theirs)
void Sexy::BugSharedBuildFadeTrack(BugObj* theBug, int theEndEvent)
{
	AnimKey* aShrink = new AnimKey();
	aShrink->SetDuration(FADE_FRAMES);
	aShrink->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = theEndEvent;
	aEnd->SetDuration(1);
	aEnd->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aShrink, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	theBug->mFadeTrack = theBug->AddTrack(aTrack, nullptr);
}

// 0x4219c0 (slot 5; the bodies of the skeeter and the bee): ObjBase's update
bool Sexy::BugSharedImgUpdate(BugImg* theBody)
{
	return theBody->ObjBase::Update();
}

// 0x4237d0 (slot 4; the same bodies): the end of a key with event 2 (the dance's) starts theTrack (the hover)
void Sexy::BugSharedImgNotify(BugImg* theBody, ObjBase* theObj, int theTrack)
{
	if (theObj->mCurrentKey != nullptr && theObj->mCurrentKey->mEventId == 2)
		theBody->StartTrack(theTrack);
}

// The post's link in a heading (PipeObj keeps them in the order up, right, down, left; the original indexes them at +0x290)
PipeObj* Sexy::BugPostLink(PipeObj* thePost, int theHeading)
{
	switch (theHeading) {
	case HEADING_UP:
		return thePost->mUpLink;
	case HEADING_RIGHT:
		return thePost->mRightLink;
	case HEADING_DOWN:
		return thePost->mDownLink;
	case HEADING_LEFT:
		return thePost->mLeftLink;
	}
	// port (crash): the original reads the memory beside the links for a heading outside 0 to 3, which none ever is
	return nullptr;
}

// 0x41d3b0 (EAX the heading): the angle a heading faces, in radians: up is a quarter turn, left a half turn, down a
// quarter turn the other way, right none
float Sexy::BugHeadingAngle(int theHeading)
{
	switch (theHeading) {
	case HEADING_UP:
		return 1.5707963705062866f;
	case HEADING_DOWN:
		return -1.5707963705062866f;
	case HEADING_LEFT:
		return 3.1415927410125732f;
	}
	return 0.0f;
}

// 0x417b90 (EAX the heading, EDX the next heading, theBase pushed): theBase turned a quarter turn to the next heading
// from the one it faces: a quarter turn less when the next is the one after the heading, a quarter turn more when it
// is the one before, and as it was when it is the same or straight back
float Sexy::BugTurnAngle(int theHeading, int theNextHeading, float theBase)
{
	int aTurn = 0;
	if ((theHeading + 1) % HEADING_COUNT == theNextHeading)
		aTurn = -1;
	else {
		int aBefore = theHeading - 1;
		if (aBefore < 0)
			aBefore = HEADING_COUNT - 1;
		if (aBefore == theNextHeading)
			aTurn = 1;
	}
	return (float)((double)(aTurn * 90) * PI_F / 180.0 + (double)theBase);
}
