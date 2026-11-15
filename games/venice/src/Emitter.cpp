// The emitter (0x421ca0-0x422700).

#include "Emitter.h"

#include "ImageRecord.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"

Emitter::Emitter()
{
	mType = ObjectTypeEmitter;
}

Emitter::~Emitter()
{
	mEmitterStages.Clear();
}

void Emitter::Load(const char* theText, SceneTag* theTag)
{
	mFrequency = (float)theTag->GetFloat("frequency");
	mRandom = (float)theTag->GetFloat("random");
	mMinX = (float)theTag->GetFloat("min_x");
	mMaxX = (float)theTag->GetFloat("max_x");
	mMinY = (float)theTag->GetFloat("min_y");
	mMaxY = (float)theTag->GetFloat("max_y");
	mKeepLocal = theTag->GetInt("keep_local");
	mParticlesPerEmit = theTag->GetInt("num_particles_per_emit");
	mBlitStyle = theTag->Has("blit_style") ? 0 : 1;
	SceneObject::Load(theText, theTag);
}

EmitterStage* Emitter::AddEmitterStage()
{
	EmitterStage* aStage = new EmitterStage();
	mEmitterStages.Append(aStage);
	return aStage;
}

void Emitter::LoadStageTag(const SceneTag* theTag)
{
	EmitterStage* aStage = AddEmitterStage();
	aStage->mMinDuration = (float)theTag->GetFloat("min_duration");
	aStage->mMaxDuration = (float)theTag->GetFloat("max_duration");
	aStage->mMinStartAlpha = (float)theTag->GetFloat("min_start_alpha");
	aStage->mMaxStartAlpha = (float)theTag->GetFloat("max_start_alpha");
	aStage->mMinEndAlpha = (float)theTag->GetFloat("min_end_alpha");
	aStage->mMaxEndAlpha = (float)theTag->GetFloat("max_end_alpha");
	aStage->mMinStartScale = (float)theTag->GetFloat("min_start_scale");
	aStage->mMaxStartScale = (float)theTag->GetFloat("max_start_scale");
	aStage->mMinEndScale = (float)theTag->GetFloat("min_end_scale");
	aStage->mMaxEndScale = (float)theTag->GetFloat("max_end_scale");
	aStage->mMinStartRotation = (float)theTag->GetFloat("min_start_rotation");
	aStage->mMaxStartRotation = (float)theTag->GetFloat("max_start_rotation");
	aStage->mMinEndRotation = (float)theTag->GetFloat("min_end_rotation");
	aStage->mMaxEndRotation = (float)theTag->GetFloat("max_end_rotation");
	aStage->mMinXSpeed = (float)theTag->GetFloat("min_x_speed");
	aStage->mMaxXSpeed = (float)theTag->GetFloat("max_x_speed");
	aStage->mMinYSpeed = (float)theTag->GetFloat("min_y_speed");
	aStage->mMaxYSpeed = (float)theTag->GetFloat("max_y_speed");
	aStage->mDecay = (float)theTag->GetFloat("decay");
	aStage->mGravity = (float)theTag->GetFloat("gravity");
	aStage->mImage = AcquireImageRecord(theTag->GetString("image"), 0, 0, 0, 0);
}

bool Emitter::Update()
{
	bool aResult = SceneObject::Update();
	bool aVisible = mParent == nullptr || mParent->mVisible != 0;
	if (!aVisible || mStages.mCursor == nullptr || !mStages.mCursor->mVisible)
		return aResult;
	int aDue;
	if (mRandom == 0.0f) {
		int aFrequency = (int)mFrequency;
		aDue = aFrequency == 0 ? 1 : (int)mTickCount % aFrequency; // the original divides by zero here
	} else
		aDue = RandomBelow((uint32_t)(int)mFrequency);
	if (aDue != 0 || mEmitterStages.mHead == nullptr)
		return aResult;
	for (int aCount = 0; aCount < mParticlesPerEmit; aCount++) {
		SceneObject* aParticle = NewChild(nullptr);
		aParticle->mLocal[AttributeX] = (float)RandomFloat(mMinX, mMaxX);
		aParticle->mLocal[AttributeY] = (float)RandomFloat(mMinY, mMaxY);
		aParticle->mBlitStyle = mBlitStyle;
		uint32_t aFrame = RandomBelow((uint32_t)mEmitterStages.mHead->mImage->mFrameCount);
		for (EmitterStage* aStage = mEmitterStages.mHead; aStage != nullptr; aStage = aStage->mNext) {
			aParticle->SetImageRecord(aStage->mImage);
			aParticle->AddStage((float)RandomFloat(aStage->mMinDuration, aStage->mMaxDuration), 0, 0, 1, nullptr);
			float anEndAlpha = (float)RandomFloat(aStage->mMinEndAlpha, aStage->mMaxEndAlpha);
			float aStartAlpha = (float)RandomFloat(aStage->mMinStartAlpha, aStage->mMaxStartAlpha);
			aParticle->SetAttribute(AttributeAlpha, aStartAlpha, anEndAlpha);
			float anEndXScale = (float)RandomFloat(aStage->mMinEndScale, aStage->mMaxEndScale);
			float aStartXScale = (float)RandomFloat(aStage->mMinStartScale, aStage->mMaxStartScale);
			aParticle->SetAttribute(AttributeXScale, aStartXScale, anEndXScale);
			float anEndYScale = (float)RandomFloat(aStage->mMinEndScale, aStage->mMaxEndScale);
			float aStartYScale = (float)RandomFloat(aStage->mMinStartScale, aStage->mMaxStartScale);
			aParticle->SetAttribute(AttributeYScale, aStartYScale, anEndYScale);
			float anEndRotation = (float)RandomFloat(aStage->mMinEndRotation, aStage->mMaxEndRotation);
			float aStartRotation = (float)RandomFloat(aStage->mMinStartRotation, aStage->mMaxStartRotation);
			aParticle->SetAttribute(AttributeRotation, aStartRotation, anEndRotation);
			aParticle->mLocal[AttributeXScale] = 0;
			aParticle->mLocal[AttributeYScale] = 0;
			SceneStage* aCurrentStage = mStages.mCursor;
			if (aCurrentStage->mAttributeMask != 0) {
				aParticle->SetAttribute(AttributeRed, aCurrentStage->mEnd[AttributeRed], aCurrentStage->mEnd[AttributeRed]);
				aParticle->SetAttribute(AttributeGreen, aCurrentStage->mEnd[AttributeGreen],
					aCurrentStage->mEnd[AttributeGreen]);
				aParticle->SetAttribute(AttributeBlue, aCurrentStage->mEnd[AttributeBlue], aCurrentStage->mEnd[AttributeBlue]);
			}
			aParticle->SetAttribute(AttributeFrameNumber, (float)(int)aFrame, (float)(int)aFrame);
			if (aStage == mEmitterStages.mHead) {
				float aXSpeed = (float)RandomFloat(aStage->mMinXSpeed, aStage->mMaxXSpeed);
				aParticle->AddFilter(FilterVelocity, AttributeX, aXSpeed, 0, aStage->mDecay);
				float aYSpeed = (float)RandomFloat(aStage->mMinYSpeed, aStage->mMaxYSpeed);
				aParticle->AddFilter(FilterVelocity, AttributeY, aYSpeed, aStage->mGravity, aStage->mDecay);
				aParticle->AddFilter(aStage->mFilterType, AttributeAll, 1.0f, 0, 0);
			} else
				aParticle->mStages.mTail->mCarriesFilters = 1;
			if (mKeepLocal != 0)
				aParticle->UpdateWorldValues();
		}
		if (mKeepLocal == 0)
			gSparkleLayer->AdoptNewestChild(this, false);
	}
	return aResult;
}
