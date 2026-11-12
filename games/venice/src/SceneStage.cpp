// Stages, filters and events of the scene engine (0x429d50-0x42b4b0 and 0x422c50-0x422e20).

#include "SceneObject.h"

#include "GameText.h"
#include "SceneNames.h"
#include "VeniceMath.h"

#include <cstdlib>
#include <cstring>

// port: the original multiplies by the degrees-to-radians constant in line, in the filters (0x42a1c0)
static float ToRadians(double theDegrees) { return (float)(theDegrees * (double)DegreesToRadians); }

// 0x42a1c0: the attribute filters. The ones on the attribute "all" change the stage's progress; the others add to the
// owner's value of their attribute.
float SceneFilter::Apply(float* theProgress)
{
	SceneObject* anOwner = mStage->mOwner;
	float aTime = *theProgress;
	if (mDuration != 0.0f) {
		mTickCount += 1.0f;
		aTime = mTickCount / mDuration;
	}
	float* aLocal = anOwner->mLocal;
	switch (mType) {
	case FilterSuperCos:
		for (int i = 0; (float)i < mAmplitude; i++)
			*theProgress = (float)((1.0 - VeniceCos(aTime * 180.0f * DegreesToRadians)) / 2.0);
		break;
	case FilterSquared:
		*theProgress = aTime * aTime;
		break;
	case FilterCubed:
		*theProgress = aTime * aTime * aTime;
		break;
	case FilterSined:
		aLocal[mAttribute] = (float)(VeniceSin(ToRadians(((double)aTime * 360.0 * mPeriod + mOffset))) * mAmplitude +
			aLocal[mAttribute]);
		break;
	case FilterCosined:
		aLocal[mAttribute] = (float)(VeniceCos(ToRadians(((double)aTime * 360.0 * mPeriod + mOffset))) * mAmplitude +
			aLocal[mAttribute]);
		break;
	case FilterDecaySined:
		aLocal[mAttribute] = (float)(VeniceSin((aTime * 360.0f * mPeriod + mOffset) * DegreesToRadians) *
			((1.0 - (double)aTime) * (double)mAmplitude) + (double)aLocal[mAttribute]);
		break;
	case FilterDecayCosined:
		aLocal[mAttribute] = (float)(VeniceCos((aTime * 360.0f * mPeriod + mOffset) * DegreesToRadians) *
			((1.0 - (double)aTime) * (double)mAmplitude) + (double)aLocal[mAttribute]);
		break;
	case FilterLoopAnim:
		if ((int)mStage->mFrame % (int)mPeriod == 0) {
			aLocal[mAttribute] = aLocal[mAttribute] + 1.0f;
			if (aLocal[mAttribute] > mAmplitude)
				aLocal[mAttribute] = 0.0f;
		} else if (mStage->mFrame == 0.0f)
			aLocal[mAttribute] = 0.0f;
		break;
	case FilterPingPongAnim:
		if ((int)mStage->mFrame % (int)mPeriod == 0) {
			aLocal[mAttribute] = aLocal[mAttribute] + mOffset;
			if (aLocal[mAttribute] > mAmplitude || aLocal[mAttribute] < 0.0f) {
				mOffset = 0.0f - mOffset;
				aLocal[mAttribute] = 2.0f * mOffset + aLocal[mAttribute];
			}
		} else if (mStage->mFrame == 0.0f)
			aLocal[mAttribute] = 0.0f;
		break;
	case FilterHotSpot:
		aLocal[mAttribute] = anOwner->IsHot() ? 1.0f : 0.0f;
		break;
	case FilterVelocity:
	case FilterVelocityLooped:
		aLocal[mAttribute] = aLocal[mAttribute] + mAmplitude;
		mAmplitude = mAmplitude + mPeriod;
		if (mOffset != 0.0f) {
			mAmplitude = mAmplitude * mOffset;
			if (mType == FilterVelocity && VeniceAbs(mAmplitude) < 5e-05)
				return FilterFinishedValue;
		}
		break;
	}
	if (mDuration == 0.0f || mTickCount < mDuration)
		return *theProgress;
	if (mResetsAttribute) {
		mStage->mStart[mAttribute] = anOwner->mLocal[mAttribute];
		mStage->mEnd[mAttribute] = anOwner->mLocal[mAttribute];
	}
	return FilterFinishedValue;
}

// 0x42a070
SceneFilter* SceneStage::AddFilter(int theType, int theAttribute, float theAmplitude, float thePeriod, float theOffset,
	bool theOnce)
{
	SceneFilter* aFilter = new SceneFilter();
	mFilters.Append(aFilter);
	aFilter->mType = theType;
	aFilter->mAmplitude = theAmplitude;
	aFilter->mPeriod = thePeriod;
	aFilter->mOffset = theOffset;
	aFilter->mStage = this;
	aFilter->mAttribute = theAttribute;
	aFilter->mOnce = theOnce;
	return mFilters.mCursor;
}

// 0x42a840
SceneFilter* SceneStage::FindFilter(int theType, int theAttribute)
{
	SceneFilter* aFilter = mFilters.mHead;
	while (aFilter != nullptr && (aFilter->mType != theType || aFilter->mAttribute != theAttribute))
		aFilter = aFilter->mNext;
	return aFilter;
}

// 0x42a7f0
SceneEvent* SceneStage::FindEvent(int theTrigger, int theAction)
{
	SceneEvent* anEvent = mEvents.mHead;
	while (anEvent != nullptr && (anEvent->mAction != theAction || anEvent->mTrigger != theTrigger))
		anEvent = anEvent->mNext;
	return anEvent;
}

// 0x42a170: the trigger and action are given as numbers
SceneEvent* SceneStage::AddEvent(int theTrigger, int theAction, const char* theParm1, const char* theParm2,
	const char* theParm3, const char* theParm4, const char* theAltKey)
{
	SceneEvent* anEvent = new SceneEvent();
	mEvents.Append(anEvent);
	anEvent->mStage = this;
	anEvent->Set(theTrigger, theAction, theParm1, theParm2, theParm3, theParm4, theAltKey);
	return anEvent;
}

// 0x42a120: the trigger and action are given by name
SceneEvent* SceneStage::AddEventFromNames(const char* theTrigger, const char* theAction, const char* theParm1,
	const char* theParm2, const char* theParm3, const char* theParm4, const char* theAltKey)
{
	SceneEvent* anEvent = new SceneEvent();
	mEvents.Append(anEvent);
	anEvent->mStage = this;
	int anAction = ActionFromName(theAction);
	int aTrigger = TriggerFromName(theTrigger);
	anEvent->Set(aTrigger, anAction, theParm1, theParm2, theParm3, theParm4, theAltKey);
	return anEvent;
}

// 0x422c50
void SceneEvent::Set(int theTrigger, int theAction, const char* theParm1, const char* theParm2, const char* theParm3,
	const char* theParm4, const char* theAltKey)
{
	mTrigger = theTrigger;
	mAction = theAction;
	mAltKey = 0;
	strcpy(mParm1, theParm1 == nullptr ? "" : TranslateText(theParm1));
	strcpy(mParm2, theParm2 == nullptr ? "" : TranslateText(theParm2));
	strcpy(mParm3, theParm3 == nullptr ? "" : TranslateText(theParm3));
	strcpy(mParm4, theParm4 == nullptr ? "" : TranslateText(theParm4));
	if (theAltKey != nullptr && strlen(theAltKey) != 0) {
		if (strcmp(theAltKey, "return") == 0)
			mAltKey = 0x0d;
		else if (strcmp(theAltKey, "escape") == 0)
			mAltKey = 0x1b;
		else if (strcmp(theAltKey, "space") == 0)
			mAltKey = 0x20;
		else
			mAltKey = *theAltKey;
	}
}

// 0x429f10: one update of the stage; the filters of "all" change the progress, the attributes the stage animates move
// between their start and end values by it, the filters of single attributes add to the result, and the events fire.
bool SceneStage::Update()
{
	mProgress = mFrame / mLength;
	ApplyFilters(&mProgress);
	for (int i = 0; i < AttributeCount; i++) {
		if ((mAttributeMask & (1u << i)) != 0)
			mOwner->mLocal[i] = (mEnd[i] - mStart[i]) * mProgress + mStart[i];
	}
	ApplyAttributeFilters(&mProgress);
	bool aRunning = mProgress < 1.0f;
	if (!CheckEvents())
		return aRunning;
	mOwner->mVisible = mVisible;
	mFrame = mFrame + 1.0f;
	return mProgress < 1.0f;
}

// 0x42a890
void SceneStage::ApplyFilters(float* theProgress)
{
	bool aMore = mFilters.Begin();
	while (aMore) {
		if (mFilters.mCursor->mAttribute == AttributeAll)
			mFilters.mCursor->Apply(theProgress);
		aMore = mFilters.Next();
	}
}

// 0x42a8f0: a finished filter is taken out of the stage
void SceneStage::ApplyAttributeFilters(float* theProgress)
{
	SceneFilter* aFilter = mFilters.mHead;
	while (aFilter != nullptr) {
		if ((unsigned int)aFilter->mAttribute < AttributeAll) {
			float aResult = aFilter->Apply(theProgress);
			if (aResult == FilterFinishedValue || (mFrame == mLength && aFilter->mOnce)) {
				SceneFilter* aNext = aFilter->mNext;
				mFilters.mCursor = aFilter;
				mFilters.RemoveCursor();
				aFilter = aNext;
				continue;
			}
		}
		aFilter = aFilter->mNext;
	}
}

// port: lets CheckEvents know that an event freed the stage
SceneStage::~SceneStage()
{
	if (mFreedDuringEvents != nullptr)
		*mFreedDuringEvents = true;
}

// 0x42a980
// port: the original goes on reading the stage after an event has freed it (the firework's last event rebuilds its stages); the
// freed memory still held the stage's last values there, so the loop ended after that event and the update returned whether
// the stage's progress was under 1. The port stops in the same place without touching the freed stage.
bool SceneStage::CheckEvents()
{
	bool aFreed = false;
	mFreedDuringEvents = &aFreed;
	bool aMore = mEvents.Begin();
	while (aMore) {
		mEvents.mCursor->Check();
		if (aFreed)
			return false;
		aMore = mEvents.Next();
	}
	mFreedDuringEvents = nullptr;
	return true;
}

// 0x422e20
void SceneEvent::Check()
{
	bool aHolds = false;
	switch (mTrigger) {
	case TriggerClick:
		aHolds = ClickHandled(false);
		break;
	case TriggerClickAnywhere:
		if (mStage->mOwner->GetRoot()->mNext == nullptr)
			aHolds = ClickHandled(true);
		break;
	case TriggerHot:
		aHolds = mStage->mOwner->IsHot();
		break;
	case TriggerNotHot:
		aHolds = !mStage->mOwner->IsHot();
		break;
	case TriggerStart:
		aHolds = mStage->mFrame == 1.0f;
		break;
	case TriggerEnd:
		aHolds = mStage->mLength <= mStage->mFrame;
		break;
	case TriggerLessThan:
		aHolds = LessThanHolds();
		break;
	case TriggerInterval: {
		int aFrame = (int)mStage->mFrame;
		int aMilliseconds = atol(mParm1);
		int anInterval = (aMilliseconds + (aMilliseconds >> 31 & 3)) >> 2;
		if (aFrame % anInterval == 0)
			aHolds = true;
		break;
	}
	}
	if (aHolds)
		Fire();
}
