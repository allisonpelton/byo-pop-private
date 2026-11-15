// The scene object (0x425ce0-0x42a070): attributes, stages, children, drawing and hit tests.

#include "SceneObject.h"

#include "GameText.h"
#include "Geometry.h"
#include "ImageRecord.h"
#include "Level.h"
#include "SceneNames.h"
#include "VeniceDraw.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstdlib>
#include <cstring>

// 0x425e70
SceneObject::SceneObject()
{
	Reset();
}

// 0x425ee0, and 0x422bf0 (the release of the picture, translated inline)
SceneObject::~SceneObject()
{
	mChildren.Clear();
	mStages.Clear();
	if (!gScenes.IsEmpty() && mImage != nullptr) {
		mImage->mReferenceCount--;
		if (mImage->mReferenceCount < 1)
			ReleaseImageRecord(mImage);
	}
}

// 0x425ce0
void SceneObject::Reset()
{
	mTickCount = 0;
	mZOrder = 0;
	mSourceLeft = mSourceTop = mSourceRight = mSourceBottom = 0;
	mParent = nullptr;
	mImage = nullptr;
	mParticleImage = nullptr;
	ResetAttributes(mWorld);
	ResetAttributes(mLocal);
	mName[0] = 0;
	mType = 0;
	mIsDead = false;
	mHasShadow = 0;
	mTilesDown = mTilesUp = mTilesRight = mTilesLeft = 0;
	mBilinear = 1;
	mSnap = 1;
	mFlipX = false;
	mFlipY = false;
	mClipLeft = mClipRight = mClipTop = mClipBottom = 0;
	mNoCulling = 0;
	mIsHot = false;
	mBlitStyle = 0;
	mFullscreenOnly = 0;
	mSelectable = 0; // a new object is not selectable until its data says so, so a slot made by code is dropped when a data slot loads after it
}

// 0x42aed0
void SceneObject::ResetAttributes(float* theAttributes)
{
	theAttributes[AttributeX] = 0;
	theAttributes[AttributeY] = 0;
	theAttributes[AttributeZ] = 0;
	theAttributes[AttributeRotation] = 0;
	theAttributes[AttributeXScale] = 1.0f;
	theAttributes[AttributeYScale] = 1.0f;
	theAttributes[AttributeRed] = 1.0f;
	theAttributes[AttributeGreen] = 1.0f;
	theAttributes[AttributeBlue] = 1.0f;
	theAttributes[AttributeAlpha] = 1.0f;
	theAttributes[AttributeFrameNumber] = 0;
}

// 0x426570: the attributes every object kind reads from its tag
void SceneObject::Load(const char*, SceneTag* theTag)
{
	if (theTag->Has("image"))
		SetImage(TranslateText(theTag->GetString("image")), 0, 0, 0, 0);
	if (theTag->Has("particle_image"))
		mParticleImage = AcquireImageRecord(theTag->GetString("particle_image"), 0, 0, 0, 0);
	if (theTag->Has("name"))
		strcpy(mName, theTag->GetString("name"));
	if (theTag->Has("z_order"))
		SetZOrder(ParseNumber(theTag->GetString("z_order")));
	if (theTag->Has("shadow"))
		mHasShadow = theTag->GetInt("shadow");
	if (!theTag->Has("bilinear")) {
		if (mParent != nullptr)
			mBilinear = mParent->mBilinear;
	} else
		mBilinear = theTag->GetInt("bilinear");
	if (theTag->Has("snap"))
		mSnap = theTag->GetInt("snap");
	if (theTag->Has("flip_x"))
		mFlipX = theTag->GetInt("flip_x") != 0;
	if (theTag->Has("flip_y"))
		mFlipY = theTag->GetInt("flip_y") != 0;
	if (theTag->Has("clip_left"))
		mClipLeft = theTag->GetInt("clip_left");
	if (theTag->Has("clip_right"))
		mClipRight = theTag->GetInt("clip_right");
	if (theTag->Has("clip_top"))
		mClipTop = theTag->GetInt("clip_top");
	if (theTag->Has("clip_bottom"))
		mClipBottom = theTag->GetInt("clip_bottom");
	if (theTag->Has("blit_style"))
		mBlitStyle = theTag->GetInt("blit_style");
	mFullscreenOnly = theTag->GetInt("fullscreen_only");
	mTilesLeft = theTag->GetInt("tiles_left");
	mTilesRight = theTag->GetInt("tiles_right");
	mTilesUp = theTag->GetInt("tiles_up");
	mTilesDown = theTag->GetInt("tiles_down");
	mSelectable = 1;
	if (theTag->Has("selectable"))
		mSelectable = theTag->GetInt("selectable");
}

// 0x4284b0
SceneStage* SceneObject::AddStage(float theMilliseconds, int theLoopStart, int theLoopEnd, int theVisible,
	const char* theName)
{
	SceneStage* aPreviousCurrent = mStages.mCursor;
	SceneStage* aStage = new SceneStage();
	mStages.Append(aStage);
	mStages.mCursor->mOwner = this;
	mStages.mCursor->mLength = theMilliseconds / 4.0f;
	mStages.mCursor->mLoopStart = theLoopStart;
	mStages.mCursor->mLoopEnd = theLoopEnd;
	mStages.mCursor->mVisible = theVisible != 0;
	if (theName != nullptr)
		strcpy(mStages.mCursor->mName, theName);
	SceneStage* aNewStage = mStages.mCursor;
	if (aPreviousCurrent != nullptr)
		mStages.mCursor = aPreviousCurrent;
	return aNewStage;
}

// 0x428920
SceneStage* SceneObject::GoToStage(SceneStage* theStage)
{
	theStage->mFrame = 0;
	mStages.mCursor = theStage;
	ApplyStageDefaults(theStage);
	theStage->mProgress = 0;
	return theStage;
}

// 0x428970
SceneStage* SceneObject::GoToStageByName(const char* theName)
{
	SceneStage* aStage = mStages.mHead;
	while (aStage != nullptr && strcmp(aStage->mName, theName) != 0)
		aStage = aStage->mNext;
	if (aStage != nullptr)
		GoToStage(aStage);
	return aStage;
}

// 0x428a90: a stage that starts takes the object's values for the attributes it does not give
void SceneObject::ApplyStageDefaults(SceneStage* theStage)
{
	for (int i = 0; i < AttributeCount; i++) {
		if (theStage->mStart[i] == AttributeContinueValue && theStage->mPrev != nullptr)
			theStage->mStart[i] = theStage->mPrev->mEnd[i];
		else if (theStage->mHasStartValue[i])
			theStage->mStart[i] = mLocal[i];
		if (theStage->mEnd[i] == AttributeUnsetValue)
			theStage->mEnd[i] = mLocal[i];
	}
}

// 0x4289d0: the stage after the current one, or the one the loop goes back to
SceneStage* SceneObject::NextStage()
{
	SceneStage* aCurrent = mStages.mCursor;
	SceneStage* aNext;
	if (mStages.mCursor->mLoopEnd == 0) {
		aNext = aCurrent->mNext;
		if (aNext != nullptr) {
			ApplyStageDefaults(aNext);
			if (aNext->mCarriesFilters != 0) {
				for (SceneFilter* aFilter = aCurrent->mFilters.mHead; aFilter != nullptr; aFilter = aFilter->mNext)
					aNext->AddFilter(aFilter->mType, aFilter->mAttribute, aFilter->mAmplitude, aFilter->mPeriod,
						aFilter->mOffset, aFilter->mOnce);
			}
		}
	} else
		aNext = LoopBackStage();
	return aNext;
}

// 0x428b60
SceneStage* SceneObject::LoopBackStage()
{
	SceneStage* aStage = mStages.mCursor;
	while (aStage != nullptr && aStage->mLoopStart == 0) {
		aStage->mFrame = 1.0f;
		aStage = aStage->mPrev;
	}
	if (aStage != nullptr)
		aStage->mFrame = 1.0f;
	return aStage;
}

// 0x428610
void SceneObject::SetAttribute(int theAttribute, float theStart, float theEnd)
{
	// the attributes, filters and events a stage tag is followed by belong to the newest stage, not the current one
	SceneStage* aStage = mStages.mTail;
	aStage->mStart[theAttribute] = theStart;
	aStage->mEnd[theAttribute] = theEnd;
	aStage->mAttributeMask |= 1u << theAttribute;
	if (theStart == AttributeUnsetValue)
		aStage->mHasStartValue[theAttribute] = true;
}

// 0x4286a0
void SceneObject::LoadAttributeTag(const char*, SceneTag* theTag)
{
	float anEnd = (float)theTag->GetFloat("end");
	float aStart = (float)theTag->GetFloat("start");
	int anAttribute = AttributeFromName(theTag->GetString("type"));
	SetAttribute(anAttribute, aStart, anEnd);
}

// 0x428700: a hot-spot filter with no values spans the image
SceneFilter* SceneObject::AddFilter(int theType, int theAttribute, float theAmplitude, float thePeriod, float theOffset)
{
	if (theAttribute == AttributeFrameNumber && theAmplitude == 0.0f && thePeriod == 0.0f) {
		theAmplitude = (float)(unsigned int)mImage->mFrameCount - 1.0f;
		thePeriod = mImage->mFrameDuration;
	}
	return mStages.mTail->AddFilter(theType, theAttribute, theAmplitude, thePeriod, theOffset, false);
}

// 0x428810
void SceneObject::LoadFilterTag(const char*, SceneTag* theTag)
{
	float anOffset = (float)theTag->GetFloat("offset");
	float aPeriod = (float)theTag->GetFloat("period");
	float anAmplitude = (float)theTag->GetFloat("amplitude");
	int anAttribute = AttributeFromName(theTag->GetString("attribute"));
	int aType = FilterTypeFromName(theTag->GetString("type"));
	AddFilter(aType, anAttribute, anAmplitude, aPeriod, anOffset);
}

// 0x428790
SceneEvent* SceneObject::AddEventFromNames(const char* theTrigger, const char* theAction, const char* theParm1,
	const char* theParm2, const char* theParm3, const char* theParm4, const char* theAltKey)
{
	return mStages.mTail->AddEventFromNames(theTrigger, theAction, theParm1, theParm2, theParm3, theParm4, theAltKey);
}

// 0x4287d0
SceneEvent* SceneObject::AddEvent(int theTrigger, int theAction, const char* theParm1, const char* theParm2,
	const char* theParm3, const char* theParm4, const char* theAltKey)
{
	return mStages.mTail->AddEvent(theTrigger, theAction, theParm1, theParm2, theParm3, theParm4, theAltKey);
}

// 0x4288a0
void SceneObject::LoadEventTag(const char*, SceneTag* theTag)
{
	const char* anAltKey = theTag->GetString("alt_key");
	const char* aParm4 = theTag->GetString("parm4");
	const char* aParm3 = theTag->GetString("parm3");
	const char* aParm2 = theTag->GetString("parm2");
	const char* aParm1 = theTag->GetString("parm1");
	const char* anAction = theTag->GetString("action");
	const char* aTrigger = theTag->GetString("type");
	AddEventFromNames(aTrigger, anAction, aParm1, aParm2, aParm3, aParm4, anAltKey);
}

// 0x428580 is the stage tag's reader, in the scene loader

// 0x4260a0
void SceneObject::AddToAttribute(int theAttribute, float theDelta)
{
	mWorld[theAttribute] = mWorld[theAttribute] + theDelta;
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext)
		aChild->AddToAttribute(theAttribute, theDelta);
}

// 0x426110
void SceneObject::Move(float theDeltaX, float theDeltaY)
{
	AddToAttribute(AttributeX, theDeltaX);
	AddToAttribute(AttributeY, theDeltaY);
	mLocal[AttributeX] = mLocal[AttributeX] + theDeltaX;
	mLocal[AttributeY] = mLocal[AttributeY] + theDeltaY;
	for (SceneStage* aStage = mStages.mHead; aStage != nullptr; aStage = aStage->mNext) {
		if (aStage->mStart[AttributeX] != AttributeUnsetValue)
			aStage->mStart[AttributeX] = aStage->mStart[AttributeX] + theDeltaX;
		if (aStage->mStart[AttributeY] != AttributeUnsetValue)
			aStage->mStart[AttributeY] = aStage->mStart[AttributeY] + theDeltaY;
		if (aStage->mEnd[AttributeX] != AttributeUnsetValue)
			aStage->mEnd[AttributeX] = aStage->mEnd[AttributeX] + theDeltaX;
		if (aStage->mEnd[AttributeY] != AttributeUnsetValue)
			aStage->mEnd[AttributeY] = aStage->mEnd[AttributeY] + theDeltaY;
	}
	if (mClipBottom != 0) {
		mClipLeft = (int)theDeltaX + mClipLeft;
		mClipRight = (int)theDeltaX + mClipRight;
		mClipTop = (int)theDeltaY + mClipTop;
		mClipBottom = (int)theDeltaY + mClipBottom;
	}
	SnapToGrid();
}

// 0x426370: positions go to multiples of ten
void SceneObject::SnapToGrid()
{
	if (mSnap == 0)
		return;
	float aSnappedX = (float)(((int)mLocal[AttributeX] / 10) * 10);
	float aSnappedY = (float)(((int)mLocal[AttributeY] / 10) * 10);
	float aDeltaX = aSnappedX - mLocal[AttributeX];
	float aDeltaY = aSnappedY - mLocal[AttributeY];
	mLocal[AttributeX] = aSnappedX;
	mLocal[AttributeY] = aSnappedY;
	UpdateWorldValues();
	for (SceneStage* aStage = mStages.mHead; aStage != nullptr; aStage = aStage->mNext) {
		if (aStage->mStart[AttributeX] != AttributeUnsetValue && aStage->mStart[AttributeX] != AttributeContinueValue)
			aStage->mStart[AttributeX] = aStage->mStart[AttributeX] + aDeltaX;
		if (aStage->mStart[AttributeY] != AttributeUnsetValue && aStage->mStart[AttributeY] != AttributeContinueValue)
			aStage->mStart[AttributeY] = aStage->mStart[AttributeY] + aDeltaY;
		if (aStage->mEnd[AttributeX] != AttributeUnsetValue && aStage->mEnd[AttributeX] != AttributeContinueValue)
			aStage->mEnd[AttributeX] = aStage->mEnd[AttributeX] + aDeltaX;
		if (aStage->mEnd[AttributeY] != AttributeUnsetValue && aStage->mEnd[AttributeY] != AttributeContinueValue)
			aStage->mEnd[AttributeY] = aStage->mEnd[AttributeY] + aDeltaY;
	}
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext)
		aChild->SnapToGrid();
}

// 0x429c90
void SceneObject::FlipHorizontally()
{
	mFlipX = true;
	for (SceneStage* aStage = mStages.mHead; aStage != nullptr; aStage = aStage->mNext) {
		if (aStage->mStart[AttributeX] != AttributeUnsetValue)
			aStage->mStart[AttributeX] = 0.0f - aStage->mStart[AttributeX];
		if (aStage->mEnd[AttributeX] != AttributeUnsetValue)
			aStage->mEnd[AttributeX] = 0.0f - aStage->mEnd[AttributeX];
	}
}

// 0x426dc0
bool SceneObject::Update()
{
	if (mIsDead)
		return false;
	if (mTickCount == 0.0f)
		OnFirstUpdate();
	ResetAttributes(mWorld);
	bool aStageRunning = mStages.mCursor != nullptr ? mStages.mCursor->Update() : false;
	UpdateWorldValues();
	if (mFullscreenOnly != 0)
		mVisible = IsWindowedOnScreen() ? 0 : (int)mStages.mCursor->mVisible;
	UpdateChildren();
	if (!aStageRunning && mStages.mCursor != nullptr && mStages.mCursor->mFrame != 0.0f)
		mStages.mCursor = NextStage();
	mTickCount = mTickCount + 1.0f;
	return mStages.mCursor != nullptr;
}

// 0x426f10
void SceneObject::UpdateChildren()
{
	SceneObject* aChild = mChildren.mHead;
	while (aChild != nullptr) {
		if (!aChild->Update()) {
			SceneObject* aNext = aChild->mNext;
			mChildren.mCursor = aChild;
			mChildren.RemoveCursor();
			aChild = aNext;
		} else
			aChild = aChild->mNext;
	}
}

// 0x426f60
void SceneObject::Draw()
{
	if (mVisible != 0 && mTickCount != 0.0f) {
		DrawSelf();
		DrawChildren();
	}
}

// 0x4277d0
void SceneObject::DrawChildren()
{
	bool aMore = mChildren.Begin();
	while (aMore) {
		mChildren.mCursor->Draw();
		aMore = mChildren.Next();
	}
}

// 0x4270f0
void SceneObject::DrawSelf()
{
	if (mImage == nullptr)
		return;
	if (mNoCulling == 0 && gRenderTarget == nullptr) {
		int aTilesDown = mTilesDown < 1 ? 1 : mTilesDown;
		int aTilesRight = mTilesRight < 1 ? 1 : mTilesRight;
		if (!RectsOverlap(mWorld[AttributeX], mWorld[AttributeY],
				(mSourceRight - mSourceLeft) * mWorld[AttributeXScale] * (float)aTilesRight,
				(mSourceBottom - mSourceTop) * mWorld[AttributeYScale] * (float)aTilesDown, gScreenWidth / 2.0f,
				gScreenHeight / 2.0f, gScreenWidth + 80.0f, gScreenHeight + 80.0f))
			return;
	}
	if (mImage->mFrameRects != nullptr) {
		int aFrame = (int)mWorld[AttributeFrameNumber];
		mSourceLeft = mImage->mFrameRects[aFrame].mLeft;
		mSourceRight = mImage->mFrameRects[aFrame].mRight;
		mSourceTop = mImage->mFrameRects[aFrame].mTop;
		mSourceBottom = mImage->mFrameRects[aFrame].mBottom;
	}
	float aX = mWorld[AttributeX];
	float aY = mWorld[AttributeY];
	if (mBilinear == 0) {
		aX = (float)(int)(aX + 0.5f);
		aY = (float)(int)(aY + 0.5f);
	}
	if (mClipBottom != 0 || mClipTop != 0 || mClipLeft != 0 || mClipRight != 0)
		SetDrawClip(mClipLeft, mClipTop, mClipRight, mClipBottom);
	if (mBlitStyle != 0)
		SetBlitStyleOnTarget(mBlitStyle);
	for (float aTileY = (float)-mTilesUp; aTileY <= (float)mTilesDown; aTileY = aTileY + 1.0f) {
		for (float aTileX = (float)-mTilesLeft; aTileX <= (float)mTilesRight; aTileX = aTileX + 1.0f) {
			if (mHasShadow != 0 && IsAccelerated()) {
				float aHeight = mSourceBottom - mSourceTop;
				float aWidth = mSourceRight - mSourceLeft;
				float aShadowY = (float)(((double)aTileY * mWorld[AttributeYScale]) * aHeight +
					((double)aY - (double)aHeight / 2.0));
				float aShadowX = (float)(((double)aTileX * mWorld[AttributeXScale]) * aWidth +
					((double)aX - (double)aWidth / 2.0));
				DrawImageRecord(mImage, mSourceLeft, mSourceTop, mSourceRight, mSourceBottom,
					(float)(VeniceSqrt(aWidth) / 2.0 + aShadowX), (float)(VeniceSqrt(aHeight) / 2.0 + aShadowY),
					mWorld[AttributeRotation], mWorld[AttributeXScale], mWorld[AttributeYScale], 0.0f, 0.0f, 0.0f,
					0.4f * mWorld[AttributeAlpha], mFlipX, mFlipY);
			}
			float aWidth = mSourceRight - mSourceLeft;
			float aHeight = mSourceBottom - mSourceTop;
			DrawImageRecord(mImage, mSourceLeft, mSourceTop, mSourceRight, mSourceBottom,
				(float)(((double)mWorld[AttributeXScale] * aTileX) * aWidth + ((double)aX - (double)aWidth / 2.0)) -
					1.0f,
				(float)(((double)mWorld[AttributeYScale] * aTileY) * aHeight +
					((double)aY - (double)aHeight / 2.0)),
				mWorld[AttributeRotation], mWorld[AttributeXScale], mWorld[AttributeYScale], mWorld[AttributeRed],
				mWorld[AttributeGreen], mWorld[AttributeBlue], mWorld[AttributeAlpha], mFlipX, mFlipY);
		}
	}
	if (mBlitStyle != 0)
		SetBlitStyleOnTarget(0);
	if (mClipBottom != 0)
		SetDrawClip(0, 0, (int)gScreenWidth, (int)gScreenHeight);
}

// 0x429d30
void SceneObject::Kill()
{
	mIsDead = true;
	mType = ObjectTypeBase;
}

// 0x427df0: the local values moved through the parent
void SceneObject::UpdateWorldValues()
{
	memcpy(mWorld, mLocal, sizeof(mWorld));
	if (mParent != nullptr) {
		mWorld[AttributeX] = GetWorldX(mLocal[AttributeX], mLocal[AttributeY]);
		mWorld[AttributeY] = GetWorldY(mLocal[AttributeX], mLocal[AttributeY]);
		mWorld[AttributeRotation] = GetWorldRotation(mLocal[AttributeRotation]);
		mWorld[AttributeXScale] = GetWorldXScale(mLocal[AttributeXScale]);
		mWorld[AttributeYScale] = GetWorldYScale(mLocal[AttributeYScale]);
		mWorld[AttributeAlpha] = GetWorldAlpha(mLocal[AttributeAlpha]);
		mWorld[AttributeRed] = mLocal[AttributeRed] * mParent->mWorld[AttributeRed];
		mWorld[AttributeGreen] = mLocal[AttributeGreen] * mParent->mWorld[AttributeGreen];
		mWorld[AttributeBlue] = mLocal[AttributeBlue] * mParent->mWorld[AttributeBlue];
	}
}

// 0x427950
float SceneObject::GetWorldX(float theX, float theY)
{
	if (mParent == nullptr)
		return 0.0f;
	if (mParent->mWorld[AttributeRotation] != 0.0f) {
		float anAngle = 0.0f - mParent->mWorld[AttributeRotation];
		double aCos = VeniceCos(anAngle * DegreesToRadians);
		double aSin = VeniceSin(anAngle * DegreesToRadians);
		theX = (float)((double)(float)(aCos * (double)theX) - aSin * (double)theY);
	}
	return (float)((double)(theX * mParent->mWorld[AttributeXScale]) + (double)mParent->mWorld[AttributeX]);
}

// 0x427a30
float SceneObject::GetWorldY(float theX, float theY)
{
	if (mParent == nullptr)
		return 0.0f;
	if (mParent->mWorld[AttributeRotation] != 0.0f) {
		float anAngle = 0.0f - mParent->mWorld[AttributeRotation];
		double aSin = VeniceSin(anAngle * DegreesToRadians);
		double aCos = VeniceCos(anAngle * DegreesToRadians);
		theY = (float)(aCos * (double)theY + (double)(float)(aSin * (double)theX));
	}
	return (float)((double)(theY * mParent->mWorld[AttributeYScale]) + (double)mParent->mWorld[AttributeY]);
}

// 0x427b10
float SceneObject::GetWorldRotation(float theRotation)
{
	if (mParent == nullptr)
		return 0.0f;
	if (mParent->mWorld[AttributeRotation] != 0.0f)
		theRotation = theRotation + mParent->mWorld[AttributeRotation];
	return theRotation;
}

// 0x427b70
float SceneObject::GetWorldAlpha(float theAlpha)
{
	return mParent == nullptr ? 0.0f : theAlpha * mParent->mWorld[AttributeAlpha];
}

// 0x427ba0
float SceneObject::GetWorldXScale(float theScale)
{
	return mParent == nullptr ? 0.0f : theScale * mParent->mWorld[AttributeXScale];
}

// 0x427bd0
float SceneObject::GetWorldYScale(float theScale)
{
	return mParent == nullptr ? 0.0f : theScale * mParent->mWorld[AttributeYScale];
}

// 0x427c00
float SceneObject::GetLocalX(float theWorldX, float)
{
	return mParent == nullptr ? 0.0f : theWorldX - mParent->mWorld[AttributeX];
}

// 0x427c30
float SceneObject::GetLocalY(float, float theWorldY)
{
	return mParent == nullptr ? 0.0f : theWorldY - mParent->mWorld[AttributeY];
}

// 0x427f50: the object's image, loaded or taken from the cache by name
void SceneObject::SetImage(const char* theName, int theFrameWidth, int theFrameHeight, int theFrameCount,
	int theFramesPerSecond)
{
	mImage = AcquireImageRecord(theName, theFrameWidth, theFrameHeight, theFrameCount, theFramesPerSecond);
	mSourceLeft = 0;
	mSourceRight = mImage->mWidth;
	mSourceTop = 0;
	mSourceBottom = mImage->mHeight;
}

// 0x428080
void SceneObject::SetImageRecord(ImageRecord* theImage)
{
	mImage = theImage;
	mSourceLeft = 0;
	mSourceRight = theImage->mWidth - 1.0f;
	mSourceTop = 0;
	mSourceBottom = theImage->mHeight - 1.0f;
	mImage->mReferenceCount++;
}

// 0x428120
SceneObject* SceneObject::NewChild(SceneObject* theChild)
{
	if (theChild == nullptr)
		theChild = new SceneObject();
	theChild->mParent = this;
	InsertChildByZOrder(theChild);
	return mChildren.mCursor;
}

// 0x428160
void SceneObject::InsertChildByZOrder(SceneObject* theChild)
{
	SceneObject* aSibling = mChildren.mHead;
	while (aSibling != nullptr && aSibling->mZOrder <= theChild->mZOrder)
		aSibling = aSibling->mNext;
	if (aSibling == nullptr)
		mChildren.Append(theChild);
	else {
		mChildren.mCursor = aSibling;
		mChildren.InsertBeforeCursor(theChild);
	}
}

// 0x4281e0
SceneObject* SceneObject::AddChild(SceneObject* theChild)
{
	if (theChild == nullptr)
		theChild = new SceneObject();
	theChild->mParent = this;
	mChildren.Append(theChild);
	return mChildren.mCursor;
}

// 0x429720
void SceneObject::SetZOrder(float theZOrder)
{
	mZOrder = theZOrder;
	if (mParent != nullptr) {
		mParent->mChildren.mCursor = this;
		mParent->mChildren.UnlinkCursor();
		mParent->InsertChildByZOrder(this);
	}
}

// 0x428220
SceneObject* SceneObject::GetRoot()
{
	SceneObject* anObject = this;
	while (anObject->mParent != nullptr)
		anObject = anObject->mParent;
	return anObject;
}

// 0x428250
SceneObject* SceneObject::FindChildByName(const char* theName, SceneObject* theAfter)
{
	SceneObject* aChild = theAfter == nullptr ? mChildren.mHead : theAfter->mNext;
	while (aChild != nullptr && strcmp(theName, aChild->mName) != 0)
		aChild = aChild->mNext;
	return aChild;
}

// 0x4282b0
SceneObject* SceneObject::FindChildByType(int theType, SceneObject* theAfter)
{
	SceneObject* aChild = theAfter == nullptr ? mChildren.mHead : theAfter->mNext;
	while (aChild != nullptr && aChild->mType != theType)
		aChild = aChild->mNext;
	return aChild;
}

// 0x428310: the nearest sibling of a type that is on the screen
SceneObject* SceneObject::FindNearestSibling(int theType)
{
	SceneObject* aNearest = nullptr;
	for (SceneObject* aSibling = mParent->mChildren.mHead; aSibling != nullptr; aSibling = aSibling->mNext) {
		if (aSibling->mType == theType && aSibling != this &&
			aSibling->HitTest(gScreenWidth / 2.0f, gScreenHeight / 2.0f, gScreenWidth, gScreenHeight, 0.0f)) {
			if (aNearest == nullptr)
				aNearest = aSibling;
			else if ((float)DistanceTo(aSibling) < DistanceTo(aNearest))
				aNearest = aSibling;
		}
	}
	return aNearest;
}

// 0x4283f0: the child nearest the top
SceneObject* SceneObject::FindLowestChild()
{
	SceneObject* aLowest = nullptr;
	float aLowestY = 9999999.0f;
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mLocal[AttributeY] <= aLowestY && aLowestY != aChild->mLocal[AttributeY]) {
			aLowestY = aChild->mLocal[AttributeY];
			aLowest = aChild;
		}
	}
	return aLowest;
}

// 0x428460
int SceneObject::CountChildrenOfType(int theType)
{
	int aCount = 0;
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mType == theType)
			aCount++;
	}
	return aCount;
}

// 0x429790
double SceneObject::DistanceTo(SceneObject* theObject)
{
	float aDeltaX = mWorld[AttributeX] - theObject->mWorld[AttributeX];
	float aDeltaY = mWorld[AttributeY] - theObject->mWorld[AttributeY];
	return VeniceSqrt(aDeltaX * aDeltaX + aDeltaY * aDeltaY);
}

// 0x429360: whether the mouse is over the object. A scene that is not the top one has no hot objects.
bool SceneObject::IsHot()
{
	SceneObject* aRoot = GetRoot();
	if (aRoot->mNext != nullptr)
		return false;
	Level* aLevel = static_cast<Level*>(aRoot);
	bool aHot = HitTest(aLevel->mMouseX, aLevel->mMouseY, 1.0f, 1.0f, 0.0f);
	if (aHot && !mIsHot && (mParent == nullptr || mParent->mType != ObjectTypeText)) {
		int aFrequency = (int)RandomBelow(8000) + 40100;
		PlaySoundByName("audio\\hot.ogg", aFrequency);
	}
	mIsHot = aHot;
	return aHot;
}

// 0x428bd0: whether the object overlaps a rectangle (centre, size and rotation) of the screen
bool SceneObject::HitTest(float theX, float theY, float theWidth, float theHeight, float theRotation)
{
	if (mLocal[AttributeRotation] == 0.0f && theRotation == 0.0f) {
		float aHeight = (mSourceBottom - mSourceTop) * mLocal[AttributeYScale];
		float aWidth = (mSourceRight - mSourceLeft) * mLocal[AttributeXScale];
		float aWorldY = GetWorldY(mLocal[AttributeX], mLocal[AttributeY]);
		float aWorldX = GetWorldX(mLocal[AttributeX], mLocal[AttributeY]);
		return RectsOverlap(aWorldX, aWorldY, aWidth, aHeight, theX, theY, theWidth, theHeight);
	}
	float aRotation = mLocal[AttributeRotation];
	float aHeight = (mSourceBottom - mSourceTop) * mLocal[AttributeYScale];
	float aWidth = (mSourceRight - mSourceLeft) * mLocal[AttributeXScale];
	float aWorldY = GetWorldY(mLocal[AttributeX], mLocal[AttributeY]);
	float aWorldX = GetWorldX(mLocal[AttributeX], mLocal[AttributeY]);
	return RotatedRectsOverlap(theX, theY, theWidth, theHeight, theRotation, aWorldX, aWorldY, aWidth, aHeight, aRotation);
}

// 0x428d70: whether the other object's picture, as it is placed, overlaps this object
bool SceneObject::Overlaps(SceneObject* theObject)
{
	return HitTest(theObject->mWorld[AttributeX], theObject->mWorld[AttributeY],
		(theObject->mSourceRight - theObject->mSourceLeft) * theObject->mLocal[AttributeXScale],
		(theObject->mSourceBottom - theObject->mSourceTop) * theObject->mLocal[AttributeYScale],
		theObject->mLocal[AttributeRotation]);
}

// 0x428e00: how much this object's circle (as wide as its picture's frame) overlaps a circle
int SceneObject::CircleOverlapAt(float theX, float theY, float theRadius)
{
	float aRadius = mImage->mFrameWidth;
	float aWorldY = GetWorldY(mLocal[AttributeX], mLocal[AttributeY]);
	float aWorldX = GetWorldX(mLocal[AttributeX], mLocal[AttributeY]);
	return (int)CircleOverlap(aWorldX, aWorldY, aRadius, theX, theY, theRadius);
}

// 0x427d50 and 0x427da0
float SceneObject::GetXVelocity()
{
	SceneFilter* aFilter = mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
	return aFilter == nullptr ? 0.0f : aFilter->mAmplitude;
}

float SceneObject::GetYVelocity()
{
	SceneFilter* aFilter = mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
	return aFilter == nullptr ? 0.0f : aFilter->mAmplitude;
}

// 0x429420: the newest child of theSource becomes this object's child; its position is kept
void SceneObject::AdoptNewestChild(SceneObject* theSource, bool theByZOrder)
{
	SceneObject* aChild = theSource->mChildren.mCursor;
	if (aChild == nullptr)
		return;
	aChild->UpdateWorldValues();
	theSource->mChildren.UnlinkCursor();
	if (!theByZOrder)
		AddChild(aChild);
	else
		InsertChildByZOrder(aChild);
	aChild->mParent = this;
	float aLocalX = aChild->GetLocalX(aChild->mWorld[AttributeX], aChild->mWorld[AttributeY]);
	float aLocalY = aChild->GetLocalY(aChild->mWorld[AttributeX], aChild->mWorld[AttributeY]);
	for (SceneStage* aStage = aChild->mStages.mHead; aStage != nullptr; aStage = aStage->mNext) {
		if (aStage->mStart[AttributeX] != 0.0f || aStage->mEnd[AttributeX] != 0.0f) {
			aStage->mStart[AttributeX] = (aLocalX - aChild->mLocal[AttributeX]) + aStage->mStart[AttributeX];
			aStage->mEnd[AttributeX] = (aLocalX - aChild->mLocal[AttributeX]) + aStage->mEnd[AttributeX];
		}
		if (aStage->mStart[AttributeY] != 0.0f || aStage->mEnd[AttributeY] != 0.0f) {
			aStage->mStart[AttributeY] = (aLocalY - aChild->mLocal[AttributeY]) + aStage->mStart[AttributeY];
			aStage->mEnd[AttributeY] = (aLocalY - aChild->mLocal[AttributeY]) + aStage->mEnd[AttributeY];
		}
	}
	aChild->mLocal[AttributeX] = aLocalX;
	aChild->mLocal[AttributeY] = aLocalY;
}

// 0x429870: sparkles that drift away from the object. The original takes a second argument that its code never reads
// (every caller passes 0); the port leaves it out.
void SceneObject::MakeSparkles(int theCount, float theRed, float theGreen, float theBlue, float theAlpha)
{
	ImageRecord* aSparkleImage = gActiveLevel->mSparkleImage;
	UpdateWorldValues();
	for (int i = 0; i < theCount; i++) {
		SceneObject* aSparkle = NewChild(nullptr);
		aSparkle->SetImageRecord(aSparkleImage);
		aSparkle->AddStage((float)RandomBetween(1000, 3000), 0, 0, 1, nullptr);
		aSparkle->SetAttribute(AttributeAlpha, (float)(RandomFloat(0.7f, 1.0f) * (double)theAlpha), 0.0f);
		if (theRed != 0.0f || theGreen != 0.0f || theBlue != 0.0f) {
			aSparkle->SetAttribute(AttributeRed, theRed, theRed);
			aSparkle->SetAttribute(AttributeGreen, theGreen, theGreen);
			aSparkle->SetAttribute(AttributeBlue, theBlue, theBlue);
		}
		aSparkle->SetAttribute(AttributeXScale, 1.0f, 0.5f);
		aSparkle->SetAttribute(AttributeYScale, 1.0f, 0.5f);
		// the original evaluates the end of the x movement before its start
		float aXEnd = (float)RandomFloat(-200.0f, 200.0f);
		float aXStart = (float)RandomFloat(-20.0f, 20.0f);
		aSparkle->SetAttribute(AttributeX, aXStart, aXEnd);
		aSparkle->AddFilter(FilterSined, AttributeX, (float)RandomFloat(0.0f, 20.0f), 3.0f, 0.0f);
		aSparkle->AddFilter(FilterVelocity, AttributeY, (float)RandomFloat(-1.0f, 0.0f), 0.004f, 0.9995f);
		aSparkle->AddFilter(FilterVelocity, AttributeRotation, (float)RandomFloat(-0.57f, 0.57f), 0.0f, 0.0f);
		float aWobblePeriod = (float)RandomFloat(1.0f, 4.0f);
		aSparkle->AddFilter(FilterSined, AttributeXScale, 0.4f, aWobblePeriod, 0.0f);
		aSparkle->AddFilter(FilterSined, AttributeYScale, 0.4f, aWobblePeriod, 0.0f);
		aSparkle->mZOrder = ParticleZOrder;
		aSparkle->mBlitStyle = 1;
		gSparkleLayer->AdoptNewestChild(this, false);
	}
}

// 0x429b00: the object's own particle image drifting away from it
void SceneObject::MakeParticles(int theCount)
{
	UpdateWorldValues();
	ImageRecord* aParticleImage = mParticleImage;
	if (aParticleImage == nullptr)
		return;
	for (int i = 0; i < theCount; i++) {
		SceneObject* aParticle = NewChild(nullptr);
		aParticle->SetImageRecord(aParticleImage);
		aParticle->AddStage((float)RandomBetween(1000, 3000), 0, 0, 1, nullptr);
		aParticle->SetAttribute(AttributeAlpha, 1.0f, 0.0f);
		aParticle->SetAttribute(AttributeXScale, 1.0f, 0.5f);
		aParticle->SetAttribute(AttributeYScale, 1.0f, 0.5f);
		float aXEnd = (float)RandomFloat(-200.0f, 200.0f);
		float aXStart = (float)RandomFloat(-20.0f, 20.0f);
		aParticle->SetAttribute(AttributeX, aXStart, aXEnd);
		aParticle->AddFilter(FilterVelocity, AttributeY, (float)(RandomFloat(-100.0f, 0.0f) / 100.0), 0.02f, 1.0f);
		aParticle->AddFilter(FilterVelocity, AttributeRotation, (float)RandomFloat(-0.57f, 0.57f), 0.0f, 0.0f);
		aParticle->mZOrder = ParticleZOrder;
		gSparkleLayer->AdoptNewestChild(this, false);
	}
}
