// PhysObj.cpp: see PhysObj.h.

#include "PhysObj.h"

#include "DataSync.h"
#include "Image.h"
#include "ImageMgr.h"
#include "ThunderballApp.h"

#include <cmath>

using namespace Sexy;

int Sexy::gSyncMode = 0;
bool Sexy::gSyncNoImages = false;

static int gPhysObjCount = 0; // 0x650a68

static const char* const LEVEL_IMAGE_FOLDER = "images/levels/"; // 0x5f1bfc: a picture named without a folder is in here
static const int FLAG_VERSION_DRAW_SORT = 15;					 // the files from this version on have the fourth run of flags

// 0x480660
PhysObj::PhysObj()
{
	mKind = KIND_PLAIN;
	mCollidable = true;
	mVisible = true;
	mMoving = true;
	mBackground = false;
	mBaseObject = false;
	mBallStopReset = false;
	mImage = nullptr;
	mOwnsImage = false;
	mUnknownD4 = 0;
	mUnknownD8 = 0;
	mSavedValueDC = 0;
	mSavedFlagE0 = false;
	mFlagE1 = false;
	gPhysObjCount++;
	mSerialNumber = gPhysObjCount;
	mSubId = 0;
	mDrawFloatingCopy = false;
	mUnknown30 = false;
	Reset();
}

// 0x47dfb0 (0x47d230)
PhysObj::~PhysObj()
{
	DeleteImage();
}

// 0x47f740
void PhysObj::Reset()
{
	mId.clear();
	mLogic.clear();
	mBounciness = 0;
	mMoverTime = 0;
	mRolliness = 0;
	mSolidColor = 0;
	mMaxBounceVelocity = 0;
	mInactiveTimer = 0;
	mMovingTimer = 0;
	mSound = 0;
	mFlipperFlags = 0;
	mOutlineColor = 0xffffff;
	mPegInfo = nullptr;
	mMover = nullptr;
	DeleteImage();
	mImage = nullptr;
	mImageDX = 0;
	mImageDY = 0;
	mOwnsImage = false;
	mImageName.clear();
	mImageRotation = 0;
	mForeground = false;
	mForeground2 = false;
	mDrawSorted = false;
	mDrawFloating = false;
	mDrawFloatingCopy = false;
}

// 0x4769f0
void PhysObj::SetActive(bool theActive)
{
	mCollidable = theActive;
	mVisible = theActive;
}

// 0x4769c0
void PhysObj::DeleteImage()
{
	if (mOwnsImage && mImage != nullptr)
		delete mImage;
	mImage = nullptr;
}

// 0x476a00
void PhysObj::GetCenter(float* theCenter) const
{
	theCenter[0] = (mRight + mLeft) * 0.5f;
	theCenter[1] = (mBottom + mTop) * 0.5f;
}

// 0x47d350
float PhysObj::GetXPos() const
{
	return (mRight + mLeft) * 0.5f;
}

// 0x47d370
float PhysObj::GetYPos() const
{
	return (mBottom + mTop) * 0.5f;
}

// 0x476b00
void PhysObj::Translate(float theDeltaX, float theDeltaY)
{
	mLeft = theDeltaX + mLeft;
	mRight = theDeltaX + mRight;
	mTop = theDeltaY + mTop;
	mBottom = theDeltaY + mBottom;
}

// 0x479890: what kind of piece it is, as the bits the logic and the collision code ask for
unsigned int PhysObj::GetTypeFlags() const
{
	unsigned int aFlags;
	if (mPegInfo == nullptr || mPegInfo->mKind == 0)
		aFlags = 0x1000;
	else
		aFlags = 1;
	if (mKind == KIND_PEG || mKind == KIND_BALL_3)
		aFlags |= 2;
	else
		aFlags |= 0x2000;
	if (GetClass() == CLASS_BRICK)
		aFlags |= 4;
	else
		aFlags |= 0x4000;
	if (!mForeground)
		aFlags |= 8;
	else
		aFlags |= 0x8000;
	if (!mBaseObject)
		aFlags |= 0x10000;
	else
		aFlags |= 0x10;
	if (!mFlagE1)
		aFlags |= 0x20000;
	if (mPegInfo != nullptr)
	{
		if (!mPegInfo->mVariable)
			aFlags |= 0x20;
		if (mPegInfo->mCrumble)
			aFlags |= 0x40;
	}
	if (mMover != nullptr)
		aFlags |= 0x80;
	return aFlags;
}

// 0x479a30
void PhysObj::Update()
{
	if (mInactiveTimer != 0)
	{
		mInactiveTimer--;
		if (mInactiveTimer == 0)
			SetActive(false);
	}
	if (mPegInfo != nullptr)
		mPegInfo->Update(this);
	if (mMover != nullptr)
	{
		float aOldX = mMover->mX;
		float aOldY = mMover->mY;
		if (mMoving)
			mMoverTime++;
		if (mMoving || mMover->mFollowed != nullptr)
			mMover->CalcPos(mMoverTime, 0);
		float aNewX = mMover->mX;
		float aNewY = mMover->mY;
		SetPos(aNewX, aNewY);
		SetVelocity(aNewX - aOldX, aNewY - aOldY);
	}
	if (mMovingTimer != 0)
	{
		mMovingTimer--;
		if (mMovingTimer == 0)
			mMoving = false;
	}
}

// 0x479b00: where the piece is drawn between two updates in slow motion
void PhysObj::UpdateSlowMo(float theSubFrame)
{
	if (mMover != nullptr && mMoving)
	{
		float aX = mMover->mX;
		float aY = mMover->mY;
		mMover->CalcPos(mMoverTime - 1, theSubFrame);
		SetPos(mMover->mX, mMover->mY);
		mMover->mX = aX;
		mMover->mY = aY;
	}
}

// 0x47f830
bool PhysObj::LoadImage()
{
	if (mImageName.empty())
		return false;
	DeleteImage();

	// The original loads a picture whose name has "shadow" in it with its mask-only colour set to black (a global at 0x64bef4
	// that the image loader reads, white otherwise). The platform's ImageLoader has no such colour yet: see GAME_NOTES.
	ImageMgr* anImageMgr = &gThunderballApp->mImageMgr;
	if (mImageName.find('/') == std::string::npos)
		mImage = anImageMgr->GetImage(std::string(LEVEL_IMAGE_FOLDER) + mImageName, nullptr, true);
	else
		mImage = anImageMgr->GetImage(mImageName, nullptr, true);
	return mImage != nullptr;
}

// 0x480750
void PhysObj::SyncState(DataSync& theSync)
{
	bool aReading = theSync.IsReading();
	if (aReading)
		Reset();

	bool aHasRolliness = mRolliness != 0;
	bool aHasBounciness = mBounciness != 0;
	bool aHasPegInfo = mPegInfo != nullptr;
	bool aHasImageDX = mImageDX != 0;
	bool aHasMover = mMover != nullptr;
	bool aHasMoverTime = mMoverTime != 0;
	bool aHasSolidColor = mSolidColor != 0;
	bool aHasOutlineColor = mOutlineColor != 0xffffff;
	bool aHasImageName = !mImageName.empty();
	bool aHasImageDY = mImageDY != 0;
	bool aHasImageRotation = mImageRotation != 0;
	bool aHasInactiveTimer = mInactiveTimer != 0;
	bool aHasMaxBounceVelocity = mMaxBounceVelocity != 0;
	bool aHasId = !mId.empty();
	bool aHasMovingTimer = mMovingTimer != 0;
	bool aHasSound = mSound != 0;
	bool aHasLogic = !mLogic.empty();
	bool aHasSubId = mSubId != 0;
	bool aHasFlipperFlags = mFlipperFlags != 0;

	theSync.SyncBoolBit(aHasRolliness);
	theSync.SyncBoolBit(aHasBounciness);
	theSync.SyncBoolBit(aHasPegInfo);
	theSync.SyncBoolBit(aHasMover);
	theSync.SyncBoolBit(aHasMoverTime);
	theSync.SyncBoolBit(mCollidable);
	theSync.SyncBoolBit(mVisible);
	theSync.SyncBoolBit(mMoving);
	theSync.EndBit();
	theSync.SyncBoolBit(aHasSolidColor);
	theSync.SyncBoolBit(aHasOutlineColor);
	theSync.SyncBoolBit(aHasImageName);
	theSync.SyncBoolBit(aHasImageDX);
	theSync.SyncBoolBit(aHasImageDY);
	theSync.SyncBoolBit(aHasImageRotation);
	theSync.SyncBoolBit(mBackground);
	theSync.SyncBoolBit(mBaseObject);
	theSync.EndBit();
	theSync.SyncBoolBit(aHasInactiveTimer);
	theSync.SyncBoolBit(aHasId);
	theSync.SyncBoolBit(aHasMovingTimer);
	theSync.SyncBoolBit(aHasSound);
	theSync.SyncBoolBit(mBallStopReset);
	theSync.SyncBoolBit(aHasLogic);
	theSync.SyncBoolBit(mForeground);
	theSync.SyncBoolBit(aHasMaxBounceVelocity);
	theSync.EndBit();
	if (theSync.mVersion >= FLAG_VERSION_DRAW_SORT)
	{
		theSync.SyncBoolBit(mDrawSorted);
		theSync.SyncBoolBit(mForeground2);
		theSync.SyncBoolBit(aHasSubId);
		theSync.SyncBoolBit(aHasFlipperFlags);
		theSync.SyncBoolBit(mDrawFloating);
		theSync.SyncBoolBit(mUnknown30);
		theSync.EndBit();
	}

	if (aHasRolliness)
		theSync.SyncFloat(mRolliness);
	if (aHasBounciness)
		theSync.SyncFloat(mBounciness);
	if (aHasMoverTime)
		theSync.SyncLong(mMoverTime);
	if (aHasSolidColor)
		theSync.SyncLong(mSolidColor);
	if (aHasOutlineColor)
		theSync.SyncLong(mOutlineColor);
	if (aHasImageName)
		theSync.SyncString(mImageName);
	if (aHasImageDX)
		theSync.SyncFloat(mImageDX);
	if (aHasImageDY)
		theSync.SyncFloat(mImageDY);
	if (aHasImageRotation)
		theSync.SyncFloat(mImageRotation);
	if (aHasInactiveTimer)
		theSync.SyncLong(mInactiveTimer);
	if (aHasId)
		theSync.SyncString(mId);
	if (aHasMovingTimer)
		theSync.SyncLong(mMovingTimer);
	if (aHasSound)
		theSync.SyncByte(mSound);
	if (aHasLogic)
		theSync.SyncString(mLogic);
	if (aHasMaxBounceVelocity)
		theSync.SyncFloat(mMaxBounceVelocity);
	if (aHasSubId)
		theSync.SyncLong(mSubId);
	if (aHasFlipperFlags)
		theSync.SyncByte(mFlipperFlags);

	if (aHasPegInfo)
	{
		if (aReading)
			mPegInfo = new PegInfo();
		mPegInfo->SyncState(theSync);
	}
	if (aHasMover)
		theSync.SyncSharedObject(mMover);

	if (gSyncMode == SYNC_STATE)
	{
		bool aHasSavedValue = mSavedValueDC != 0;
		theSync.SyncBoolBit(mSavedFlagE0);
		theSync.SyncBoolBit(aHasSavedValue);
		theSync.EndBit();
		if (aHasSavedValue)
			theSync.SyncLong(mSavedValueDC);
	}

	if (aReading)
	{
		mDrawFloatingCopy = mDrawFloating;
		if (mMover != nullptr)
		{
			int aShape = (int)std::fabs((float)mMover->mType);
			if (aShape == Mover::SHAPE_SPIN || aShape == Mover::SHAPE_ROCK || aShape == Mover::SHAPE_CIRCLE_TURNING)
				mDrawFloatingCopy = true; // a piece that turns is drawn without the floor's shadow
		}
		if (!gSyncNoImages)
			LoadImage();
	}
}
