// The pigeon (0x404040-0x404530).

#include "Pigeon.h"

#include "Geometry.h"
#include "ImageRecord.h"
#include "SceneTag.h"
#include "Tuning.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"

#include <cmath>
#include <cstring>

Pigeon::Pigeon()
{
	mType = ObjectTypePigeon;
}

void Pigeon::Load(const char* theText, SceneTag* theTag)
{
	SetImage(theTag->GetString("flight_image"), 0, 0, 0, 0);
	mFlightImage = mImage;
	SceneObject::Load(theText, theTag);
}

// The click that bumps the tick counter is left out of every stage (the data file's pigeon scene has one, for the editor)
void Pigeon::OnObjectClosed()
{
	for (SceneStage* aStage = mStages.mHead; aStage != nullptr; aStage = aStage->mNext) {
		SceneEvent* anEvent;
		while ((anEvent = aStage->FindEvent(TriggerClickAnywhere, ActionBumpTickCounter)) != nullptr) {
			aStage->mEvents.mCursor = anEvent;
			aStage->mEvents.RemoveCursor();
			aStage->mEvents.mCursor = aStage->mEvents.mHead;
		}
	}
}

// The first update of the flight puts the wings on (and turns a bird that sits on the right round), and a bird that the level
// has scrolled below the fade point flies away
bool Pigeon::Update()
{
	if (mStages.mCursor->mFrame == 0.0f && strcmp(mStages.mCursor->mName, "fly_away") == 0) {
		SetImage(mFlightImage->mName, 0, 0, 0, 0);
		if (strstr(mName, "left") == nullptr)
			mFlipX = true;
	}
	if (gTuning.mBumperFadePoint < mWorld[AttributeY] && gShooter != nullptr &&
		strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") != nullptr && strstr(mStages.mCursor->mName, "fly") == nullptr)
		Scare(mWorld[AttributeX], mWorld[AttributeY]);
	return SceneObject::Update();
}

// The flight is a stage that ends off the screen on its side, as long as it takes a bird of this speed to get there, after a
// random pause (the stage before it)
void Pigeon::Scare(float theX, float theY)
{
	if (CircleOverlap(mWorld[AttributeX], mWorld[AttributeY], 60.0f, theX, theY, 60.0f) != 0.0f &&
		strcmp(mStages.mCursor->mName, "fly_away") != 0 && strcmp(mStages.mCursor->mName, "pre_fly_away") != 0) {
		GoToStageByName("fly_away");
		if (strstr(mName, "left") == nullptr) {
			mStages.mCursor->mEnd[AttributeX] = (float)RandomFloat(gScreenWidth / 2.0f, gScreenWidth / 2.0f + 40.0f);
			float aRiseOffset = (float)(((double)mLocal[AttributeY] - (double)mWorld[AttributeY]) - 20.0);
			mStages.mCursor->mEnd[AttributeY] = (float)(RandomFloat(-20.0f, 20.0f) + (double)aRiseOffset);
			double aDistance = VeniceSqrt((gScreenWidth - mWorld[AttributeX]) * (gScreenWidth - mWorld[AttributeX]) +
				mWorld[AttributeY] * mWorld[AttributeY]);
			mStages.mCursor->mLength = (float)(int)(aDistance * 0.3f);
		} else {
			mStages.mCursor->mEnd[AttributeX] =
				(float)RandomFloat((0.0f - gScreenWidth) / 2.0f - 40.0f, (0.0f - gScreenWidth) / 2.0f);
			float aRiseOffset = (float)(((double)mLocal[AttributeY] - (double)mWorld[AttributeY]) - 20.0);
			mStages.mCursor->mEnd[AttributeY] = (float)(RandomFloat(-20.0f, 20.0f) + (double)aRiseOffset);
			double aDistance = VeniceSqrt(mWorld[AttributeX] * mWorld[AttributeX] + mWorld[AttributeY] * mWorld[AttributeY]);
			mStages.mCursor->mLength = (float)(int)(aDistance * 0.3f);
		}
		GoToStageByName("pre_fly_away");
		mStages.mCursor->mLength = (float)RandomFloat(0.0f, 150.0f);
	}
}
