// Brick.cpp: see Brick.h. The shape arithmetic follows the original's x87 code as Line.cpp explains: a statement with several
// operations keeps the doubles of its steps and is rounded to a float once, and the cosines and sines are rounded to floats
// where the original stored them. The pictures (CreateImage) and the drawing are not translated yet.

#include "Brick.h"

#include "DataSync.h"
#include "MemoryImage.h"
#include "NotTranslated.h"
#include "Res.h"
#include "ThunderMath.h"

#include <cmath>

using namespace Sexy;

static const int NUM_KINDS = 10;				// the pictures a brick keeps: one for each of the kinds of peg, hit or not, and two more
static const float BAR_MAX_LENGTH = 600.0f;		// the longest a bar may be (Brick.cpp: the original's literal 600)
static const float SNAP_LIMIT = 0.01f;			// the ends of a straight side closer than this in x or in y are put on one line
static const float DEFAULT_RADIUS = 35.0f;		// 0x420c0000
static const float DEFAULT_ANGLE_SPAN = 30.0f;	// 0x41f00000
static const float DEFAULT_THICKNESS = 20.0f;	// 0x41a00000
static const float DEFAULT_ROTATION = -90.0f;	// 0xc2b40000
static const int DEFAULT_SUBDIVISIONS = 2;

// The point at theAngle (radians, y pointing down) on a circle round the origin
static SexyVector2 CirclePoint(float theAngle, float theRadius)
{
	float aCos = (float)cos((double)theAngle);
	float aSin = (float)sin((double)theAngle);
	return SexyVector2(aCos * theRadius, aSin * -theRadius);
}

// 0x481bd0
Brick::Brick()
{
	mRadius = DEFAULT_RADIUS;
	mAngleSpan = DEFAULT_ANGLE_SPAN;
	mLeftAngle = 0;
	mRightAngle = 0;
	mThickness = DEFAULT_THICKNESS;
	mRotation = DEFAULT_ROTATION;
	mOffsetX = 0;
	mOffsetY = 0;
	mSubdivisions = DEFAULT_SUBDIVISIONS;
	mType = BRICK_RING;
	for (int i = 0; i < 4; i++)
		mCorners[i] = 0;
	mTextureFlip = false;
	mAnchoredStart = false;
	mAnchoredEnd = false;
	for (int i = 0; i < NUM_KINDS; i++)
		mBrickImages[i] = nullptr;
	mShadowImage = nullptr;
}

// 0x483af0 (the body is 0x481d10). The original leaves the shadow's picture behind; here it goes with the others.
Brick::~Brick()
{
	for (int i = 0; i < NUM_KINDS; i++)
		delete mBrickImages[i];
	delete mShadowImage;
}

// 0x477b80
Image* Brick::GetTextureImage() const
{
	return mPegInfo != nullptr ? IMAGE_BRICK : nullptr;
}

// 0x477ba0
void Brick::DeleteImage()
{
	PhysObj::DeleteImage();
	for (int i = 0; i < NUM_KINDS; i++)
	{
		delete mBrickImages[i];
		mBrickImages[i] = nullptr;
		mBlendedImages[i].DeleteImages();
	}
}

// 0x488030
void Brick::SyncState(DataSync& theSync)
{
	SyncCommon(theSync, gSyncMode == SYNC_EDITOR);
	bool aReading = theSync.IsReading();
	if (aReading)
	{
		mType = BRICK_RING;
		mThickness = DEFAULT_THICKNESS;
		mSubdivisions = DEFAULT_SUBDIVISIONS;
		mOffsetY = 0;
		mOffsetX = 0;
	}

	bool aHasType = mType != BRICK_RING;
	bool aHasSubdivisions = mSubdivisions != DEFAULT_SUBDIVISIONS && mType != BRICK_BAR;
	bool aHasAngleSpan = mType != BRICK_BAR;
	bool aHasLeftAngle = mLeftAngle != 0;
	bool aHasRightAngle = mRightAngle != 0;
	bool aHasThickness = mThickness != DEFAULT_THICKNESS;
	bool aHasOffsetX = mOffsetX != 0;
	bool aHasOffsetY = mOffsetY != 0;

	theSync.SyncBoolBit(mAnchoredStart);
	theSync.SyncBoolBit(mAnchoredEnd);
	theSync.SyncBoolBit(aHasType);
	theSync.SyncBoolBit(aHasSubdivisions);
	theSync.SyncBoolBit(aHasAngleSpan);
	theSync.SyncBoolBit(aHasLeftAngle);
	theSync.SyncBoolBit(aHasRightAngle);
	theSync.SyncBoolBit(aHasThickness);
	theSync.EndBit();
	theSync.SyncBoolBit(aHasOffsetX);
	theSync.SyncBoolBit(aHasOffsetY);
	theSync.SyncBoolBit(mTextureFlip);
	theSync.EndBit();

	if (aHasOffsetX)
		theSync.SyncFloat(mOffsetX);
	if (aHasOffsetY)
		theSync.SyncFloat(mOffsetY);
	if (aHasType)
		theSync.SyncByte(mType);
	if (aHasSubdivisions)
		theSync.SyncByte(mSubdivisions);
	if (aHasLeftAngle)
		theSync.SyncFloat(mLeftAngle);
	if (aHasRightAngle)
	{
		// the original writes (and reads) the right angle twice
		theSync.SyncFloat(mRightAngle);
		theSync.SyncFloat(mRightAngle);
	}
	if (aHasAngleSpan)
		theSync.SyncFloat(mAngleSpan);
	if (aHasThickness)
		theSync.SyncFloat(mThickness);
	theSync.SyncFloat(mRadius);
	theSync.SyncFloat(mRotation);
	for (int i = 0; i < 4; i++)
		theSync.SyncByte(mCorners[i]);

	if (aReading && gSyncMode != SYNC_EDITOR)
		InitFromParams();
}

// 0x485670. The inner side of the brick is made first, from its end at the high angle to the one at the low angle, and then the
// outer side from the low angle to the high one; the list ends with the first point again. A curved side has mSubdivisions
// points between its ends; a straight side has only its two. At the end every point is moved so the shape's middle (plus the
// offset) is the origin.
void Brick::InitFromParams()
{
	for (int i = 0; i < NUM_KINDS; i++)
	{
		delete mBrickImages[i];
		mBrickImages[i] = nullptr;
		mBlendedImages[i].DeleteImages();
	}

	bool aInnerCurved = !(mType == BRICK_STRAIGHT_INNER || mType == BRICK_SLANT_INNER);
	bool aOuterCurved = !(mType == BRICK_STRAIGHT_OUTER || mType == BRICK_SLANT_OUTER);
	if (mType == BRICK_BAR)
	{
		if (BAR_MAX_LENGTH < mRadius)
			mRadius = BAR_MAX_LENGTH;
		aInnerCurved = false;
		aOuterCurved = false;
	}

	int aSubdivisions = mSubdivisions < 1 ? 0 : mSubdivisions;
	int aCount = 5;
	if (aInnerCurved)
		aCount += aSubdivisions;
	if (aOuterCurved)
		aCount += aSubdivisions;
	float aInnerRadius = mRadius;
	float aOuterRadius = (float)((double)mThickness + mRadius);
	float aRotation = (float)(((double)mRotation * PI_AS_FLOAT) / 180.0);
	float aLowDegrees = (float)((double)mRotation - (double)mAngleSpan * 0.5);
	float aLowAngle = (float)(((double)aLowDegrees * PI_AS_FLOAT) / 180.0);
	float aHighDegrees = (float)((double)mRotation + (double)mAngleSpan * 0.5);
	float aHighAngle = (float)(((double)PI_AS_FLOAT * aHighDegrees) / 180.0);
	float aPartCount = (float)(aSubdivisions + 1); // the curve is cut into this many parts

	mPoints.resize(aCount, SexyVector2(0, 0));

	SexyVector2 aEndA(0, 0); // the ends of a straight side
	SexyVector2 aEndB(0, 0);
	SexyVector2 aInnerMiddle;
	SexyVector2 aOuterMiddle;
	int aNext; // where the next point goes

	// the inner side
	if (aInnerCurved)
	{
		mCorners[2] = 0;
		mPoints[0] = CirclePoint(aHighAngle, aInnerRadius);
		aNext = 1;
		for (int i = 0; i < aSubdivisions; i++)
		{
			float aPart = (float)(i + 1) / aPartCount;
			float aAngle = (float)((double)aPart * aLowAngle + (1.0 - aPart) * aHighAngle);
			mPoints[aNext++] = CirclePoint(aAngle, aInnerRadius);
		}
		mCorners[0] = aNext;
		mPoints[aNext++] = CirclePoint(aLowAngle, aInnerRadius);

		const SexyVector2& aMiddleA = mPoints[(aSubdivisions + 2) / 2];
		const SexyVector2& aMiddleB = mPoints[(aSubdivisions + 1) / 2];
		aInnerMiddle.x = (aMiddleA.x + aMiddleB.x) * 0.5f;
		aInnerMiddle.y = 0.5f * (aMiddleB.y + aMiddleA.y);
	}
	else
	{
		if (mType == BRICK_SLANT_INNER)
		{
			float aRotationCos = (float)cos((double)aRotation);
			float aRotationNegSin = -(float)sin((double)aRotation);
			float aShiftX = aRotationCos * mThickness;
			float aShiftY = mThickness * aRotationNegSin;
			SexyVector2 aPointA = CirclePoint(aLowAngle, aOuterRadius);
			aEndA.x = aPointA.x - aShiftX;
			aEndA.y = aPointA.y - aShiftY;
			SexyVector2 aPointB = CirclePoint(aHighAngle, aOuterRadius);
			aEndB.x = aPointB.x - aShiftX;
			aEndB.y = aPointB.y - aShiftY;
		}
		else if (mType == BRICK_BAR)
		{
			float aCos = (float)cos((double)aRotation);
			float aCenterX = aCos * mRadius;
			float aSin = (float)sin((double)aRotation);
			float aCenterY = -mRadius * aSin;
			float aHalfX = (aSin * mRadius) * 0.5f;
			float aHalfY = 0.5f * (aCos * mRadius);
			aEndA.x = aHalfX + aCenterX;
			aEndA.y = aHalfY + aCenterY;
			aEndB.x = aCenterX - aHalfX;
			aEndB.y = aCenterY - aHalfY;
		}
		else
		{
			aEndA = CirclePoint(aLowAngle, aInnerRadius);
			aEndB = CirclePoint(aHighAngle, aInnerRadius);
		}

		if (SNAP_LIMIT <= fabsf(aEndA.x - aEndB.x))
		{
			if (fabsf(aEndA.y - aEndB.y) < SNAP_LIMIT)
				aEndB.y = aEndA.y;
		}
		else
			aEndB.x = aEndA.x;

		mCorners[2] = 0;
		mPoints[0] = aEndB;
		mCorners[0] = 1;
		mPoints[1] = aEndA;
		aInnerMiddle.x = (aEndB.x + aEndA.x) * 0.5f;
		aInnerMiddle.y = 0.5f * (aEndB.y + aEndA.y);
		aNext = 2;
	}

	// the outer side
	if (aOuterCurved)
	{
		int aFirst = aNext;
		mCorners[1] = aNext;
		mPoints[aNext++] = CirclePoint(aLowAngle, aOuterRadius);
		for (int i = 0; i < aSubdivisions; i++)
		{
			float aPart = (float)(i + 1) / aPartCount;
			float aAngle = (float)((double)aPart * aHighAngle + (1.0 - aPart) * aLowAngle);
			mPoints[aNext++] = CirclePoint(aAngle, aOuterRadius);
		}
		mCorners[3] = aNext;
		mPoints[aNext++] = CirclePoint(aHighAngle, aOuterRadius);

		const SexyVector2& aMiddleA = mPoints[(aSubdivisions + 2) / 2 + aFirst];
		const SexyVector2& aMiddleB = mPoints[(aSubdivisions + 1) / 2 + aFirst];
		aOuterMiddle.x = (aMiddleA.x + aMiddleB.x) * 0.5f;
		aOuterMiddle.y = (aMiddleB.y + aMiddleA.y) * 0.5f;
	}
	else
	{
		if (mType == BRICK_SLANT_OUTER)
		{
			float aRotationCos = (float)cos((double)aRotation);
			float aRotationNegSin = -(float)sin((double)aRotation);
			float aShiftX = aRotationCos * mThickness;
			float aShiftY = mThickness * aRotationNegSin;
			SexyVector2 aPointA = CirclePoint(aLowAngle, aInnerRadius);
			aEndA.x = aPointA.x + aShiftX;
			aEndA.y = aPointA.y + aShiftY;
			SexyVector2 aPointB = CirclePoint(aHighAngle, aInnerRadius);
			aEndB.x = aPointB.x + aShiftX;
			aEndB.y = aPointB.y + aShiftY;
		}
		else if (mType == BRICK_BAR)
		{
			// the bar's ends are the inner ones (made above) moved out by the thickness at the cut angles
			float aLeftAngle = (float)(((double)mLeftAngle * PI_AS_FLOAT) / 180.0) + aRotation;
			float aRightAngle = aRotation + (float)(((double)PI_AS_FLOAT * mRightAngle) / 180.0);
			SexyVector2 aShiftA = CirclePoint(aLeftAngle, mThickness);
			aEndA.x = aShiftA.x + aEndA.x;
			aEndA.y = aShiftA.y + aEndA.y;
			SexyVector2 aShiftB = CirclePoint(aRightAngle, mThickness);
			aEndB.x = aShiftB.x + aEndB.x;
			aEndB.y = aShiftB.y + aEndB.y;
		}
		else
		{
			aEndA = CirclePoint(aLowAngle, aOuterRadius);
			aEndB = CirclePoint(aHighAngle, aOuterRadius);
		}

		if (SNAP_LIMIT <= fabsf(aEndA.x - aEndB.x))
		{
			if (fabsf(aEndA.y - aEndB.y) < SNAP_LIMIT)
				aEndB.y = aEndA.y;
		}
		else
			aEndB.x = aEndA.x;

		mCorners[1] = aNext;
		mPoints[aNext++] = aEndA;
		mCorners[3] = aNext;
		mPoints[aNext++] = aEndB;
		aOuterMiddle.x = (aEndB.x + aEndA.x) * 0.5f;
		aOuterMiddle.y = 0.5f * (aEndB.y + aEndA.y);
	}

	mPoints[aNext] = mPoints[0]; // the list ends where it starts

	float aSumX = aOuterMiddle.x + aInnerMiddle.x;
	float aSumY = aOuterMiddle.y + aInnerMiddle.y;
	float aShiftX = mOffsetX + aSumX * 0.5f;
	float aShiftY = mOffsetY + 0.5f * aSumY;
	for (size_t i = 0; i < mPoints.size(); i++)
	{
		mPoints[i].x = mPoints[i].x - aShiftX;
		mPoints[i].y = mPoints[i].y - aShiftY;
	}
	InitFromPoints();

	// The original makes the pictures here (CreateImage 0x4827d0) unless the points are being read for a list of the level's
	// editor or the level is being loaded without pictures. They are made when first drawn instead.
}

// 0x483b20 (0x4827d0): a brick that is not a peg gets its picture by name; a peg's is built from the triangles of its shape for
// its kind (1 to 4) and whether it is hit (CreateImage(int) 0x481db0, which is part of the drawing)
void Brick::CreateImage(bool theRemake)
{
	if (mPegInfo == nullptr)
	{
		LoadImage();
		return;
	}
	NOT_TRANSLATED_DRAWING("Brick::CreateImage(int) for a peg");
}

// 0x483b30
void Brick::DrawShadow(Graphics* g)
{
	NOT_TRANSLATED("Brick::DrawShadow");
}

// 0x483d70
void Brick::Draw(Graphics* g)
{
	NOT_TRANSLATED("Brick::Draw");
}
