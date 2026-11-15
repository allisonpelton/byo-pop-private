// The cupid (0x403520-0x404040).

#include "Cupid.h"

#include "Jewel.h"
#include "Level.h"
#include "Profiles.h"
#include "SceneLoader.h"
#include "Shooter.h"
#include "Slot.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstring>

namespace
{
// the flap of the wings: a picture frame that goes there and back, and a sound every 320 ms
void AddFlap(Cupid* theCupid)
{
	theCupid->AddFilter(FilterPingPongAnim, AttributeFrameNumber, 4.0f, 10.0f, 1.0f);
	theCupid->AddEvent(TriggerInterval, ActionPlaySound, "320", nullptr, "audio\\cupid_flap.ogg", nullptr, nullptr);
}
}

Cupid::Cupid()
{
	mType = ObjectTypeCupid;
	mZOrder = 3.0f;
	SetImage("gfx\\cupid.png", 0, 0, 0, 0);

	// it flies in from the left, to a place above the shooter
	AddStage(1204.0f, 0, 0, 1, "entering");
	SetAttribute(AttributeX, -450.0f, -360.0f);
	SetAttribute(AttributeY, gShooter->mStages.mCursor->mEnd[AttributeY] - 270.0f,
		gShooter->mStages.mCursor->mEnd[AttributeY] - 170.0f);
	AddFilter(FilterSuperCos, AttributeAll, 1.0f, 1.0f, 0);
	AddFlap(this);

	// it hovers while it waits for a jewel
	AddStage(3204.0f, 1, 1, 1, "waiting");
	SetAttribute(AttributeX, AttributeUnsetValue, AttributeUnsetValue);
	SetAttribute(AttributeY, AttributeUnsetValue, AttributeUnsetValue);
	AddFilter(FilterSined, AttributeX, 5.0f, 2.0f, 0);
	AddFilter(FilterSined, AttributeY, 10.0f, 1.0f, 0);
	AddFlap(this);

	// it carries the jewel to its slot (the stage's end place follows the slot)
	AddStage(1204.0f, 0, 0, 1, "searching");
	SetAttribute(AttributeX, AttributeUnsetValue, gShooter->mLocal[AttributeX]);
	SetAttribute(AttributeY, AttributeUnsetValue, gShooter->mLocal[AttributeY]);
	AddFilter(FilterSuperCos, AttributeAll, 1.0f, 1.0f, 0);
	AddEvent(TriggerEnd, ActionCustom, nullptr, nullptr, nullptr, nullptr, nullptr);
	AddFlap(this);

	// and flies off to the right
	AddStage(1204.0f, 0, 0, 1, "leaving");
	SetAttribute(AttributeX, AttributeUnsetValue, 450.0f);
	SetAttribute(AttributeY, AttributeUnsetValue, -350.0f);
	AddFilter(FilterSuperCos, AttributeAll, 1.0f, 1.0f, 0);
	AddFlap(this);
}

bool Cupid::Update()
{
	if (strstr(mStages.mCursor->mName, "enter") != nullptr)
		CatchJewel();
	UpdateWaiting();
	UpdateSearching();
	FaceTarget();
	return SceneObject::Update();
}

// The cupid faces the way it flies, and the jewel it carries moves to the side that is the front
void Cupid::FaceTarget()
{
	SceneStage* aStage = mStages.mCursor;
	if (5.0 <= VeniceAbs(aStage->mStart[AttributeX] - aStage->mEnd[AttributeX])) {
		if (aStage->mEnd[AttributeX] <= aStage->mStart[AttributeX]) {
			if (aStage->mEnd[AttributeX] < aStage->mStart[AttributeX]) {
				mFlipX = true;
				if (mChildren.mHead != nullptr) {
					mChildren.mHead->mStages.mCursor->mStart[AttributeX] = -11.0f;
					mChildren.mHead->mStages.mCursor->mEnd[AttributeX] = -11.0f;
				}
			}
		} else {
			mFlipX = false;
			if (mChildren.mHead != nullptr) {
				mChildren.mHead->mStages.mCursor->mStart[AttributeX] = 11.0f;
				mChildren.mHead->mStages.mCursor->mEnd[AttributeX] = 11.0f;
			}
		}
	}
}

// After the first five updates of its flight in (or of its wait) the cupid catches the first jewel that was shot and is
// within 30 of it, if a plate still has a slot to fill, and holds it in its arms; it goes to look for a slot of its colour
void Cupid::CatchJewel()
{
	if (5.0f <= mStages.mCursor->mFrame) {
		Jewel* aJewel = nullptr;
		do {
			aJewel = static_cast<Jewel*>(gPlateLayer->FindChildByType(ObjectTypeJewel, aJewel));
			if (aJewel == nullptr)
				return;
		} while (aJewel->mShot == 0 || aJewel->CircleOverlapAt(mWorld[AttributeX], mWorld[AttributeY], 30.0f) == 0);
		aJewel->mIsDead = true;
		if (AnyPlateIncomplete()) {
			Jewel* aCarried = new Jewel(aJewel->mKind);
			NewChild(aCarried);
			aCarried->mStages.Clear();
			aCarried->AddStage(1000.0f, 1, 1, 1, nullptr);
			aCarried->SetAttribute(AttributeX, 11.0f, 11.0f);
			aCarried->SetAttribute(AttributeY, 25.0f, 25.0f);
			aCarried->mComboShot = aJewel->mComboShot;
			aCarried->mShot = 0;
			FindTarget(aJewel->mKind);
			LoadAndPlaySound("audio\\cupid_catch.ogg", -1);
			MakeSparkles(20, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}

// While it waits the cupid says what it is for (the first time ever), looks for a jewel to catch, and leaves after 2000 updates
// or when the level no longer scrolls
void Cupid::UpdateWaiting()
{
	if (strstr(mStages.mCursor->mName, "wait") != nullptr) {
		if (CurrentProfile().mCupidMessageSeen == 0) {
			CurrentProfile().mCupidMessageSeen = 1;
			PushSceneByName("cupid_message");
		}
		if (GetRoot()->mNext == nullptr && (mWaitTicks = mWaitTicks + 1, 2000 < mWaitTicks ||
			strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") == nullptr)) {
			GoToStageByName("leaving");
		} else {
			mFlipX = false;
			CatchJewel();
		}
	}
}

// The highest open slot (below the top of the screen) of the colour. The cupid flies to it, or, with no slot to take the jewel
// to, drops the jewel it holds and goes back to waiting
Slot* Cupid::FindTarget(int theKind)
{
	Slot* aTarget = nullptr;
	SceneObject* aPlate = nullptr;
	while ((aPlate = gPlateLayer->FindChildByType(ObjectTypePlate, aPlate)) != nullptr) {
		Slot* aSlot = nullptr;
		while ((aSlot = static_cast<Slot*>(aPlate->FindChildByType(ObjectTypeSlot, aSlot))) != nullptr) {
			if (0.0f < aSlot->mWorld[AttributeY] && aSlot->mFilled == 0 && (aSlot->mKind == theKind || theKind == WildJewelKind) &&
				(aTarget == nullptr || aSlot->mWorld[AttributeY] < aTarget->mWorld[AttributeY]))
				aTarget = aSlot;
		}
	}
	if (aTarget == nullptr) {
		mChildren.Clear();
		GoToStageByName("entering");
		mStages.mCursor->mStart[AttributeX] = mLocal[AttributeX];
		mStages.mCursor->mStart[AttributeY] = mLocal[AttributeY];
	} else {
		GoToStageByName("searching");
		mStages.mCursor->mStart[AttributeX] = mLocal[AttributeX];
		mStages.mCursor->mStart[AttributeY] = mLocal[AttributeY];
		mWaitTicks = 0;
	}
	mTarget = aTarget;
	return aTarget;
}

// The flight to the slot ends at the slot (it is moved to the side that the cupid's face is turned to); a slot that was filled
// in the meantime sends the cupid for another one
void Cupid::UpdateSearching()
{
	if (strstr(mStages.mCursor->mName, "search") != nullptr) {
		if (mTarget->mFilled == 0) {
			if (!mFlipX)
				mStages.mCursor->mEnd[AttributeX] = (mTarget->mWorld[AttributeX] - gScreenWidth / 2.0f) - 11.0f;
			else
				mStages.mCursor->mEnd[AttributeX] = (mTarget->mWorld[AttributeX] - gScreenWidth / 2.0f) + 11.0f;
			mStages.mCursor->mEnd[AttributeY] = mTarget->mWorld[AttributeY] - gScreenHeight / 2.0f;
		} else {
			Jewel* aCarried = static_cast<Jewel*>(FindChildByType(ObjectTypeJewel, nullptr));
			FindTarget(aCarried->mKind);
			mStages.mCursor->mFrame = 0;
		}
	}
}

// The cupid reaches the slot: the jewel it carries falls onto the plates from there, and after three jewels the cupid flies away
void Cupid::OnCustomEvent()
{
	SceneObject* aCarried = mChildren.mHead;
	mChildren.mCursor = aCarried;
	UpdateWorldValues();
	aCarried->Update();
	gPlateLayer->AdoptNewestChild(this, true);
	aCarried->mStages.mCursor->AddFilter(FilterVelocity, AttributeY, 0.01f, 0.02f, 0, false);
	gActiveLevel->mShotsFired--;
	mCatches++;
	if (mCatches < 3) {
		GoToStageByName("entering");
		mStages.mCursor->mStart[AttributeX] = mLocal[AttributeX];
		mStages.mCursor->mStart[AttributeY] = mLocal[AttributeY];
	}
}
