// VizMagicSnakes (Visualizers3D.h). As in Visualizers3D3.cpp the original's float code runs on the x87 stack (rounded to
// float only where it stores, and to 53 bits, the CRT's default precision), so expressions are written in double where
// the original's are; a constant printed short in the export is a double, and a long one (6.666666507720947) is a float
// the compiler widened (read from the exe).
#include "Visualizers3D.h"

#include "Camera3D.h"
#include "Compat2004.h"
#include "DDImage.h"
#include "Graphics.h"
#include "Res.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdint>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------------------
// The class's own random generator and the small vector code that its snakes share

// 0x68b3c1 (the seed at 0x85a514, set to 0xbeef0b01 at its first use): the C library's LCG (times 0x343fd, plus 0x269ec3,
// bits 16 to 30). The class uses this and nothing else (neither the game's Rand nor rand).
static uint32_t sSnakeSeed = 0xbeef0b01;

// 0x68b3e1: the next draw, 0 to 1
static float SnakeRandom()
{
	sSnakeSeed = sSnakeSeed * 0x343fd + 0x269ec3;
	int aBits = (int)(sSnakeSeed >> 0x10 & 0x7fff);
	return (float)((double)aBits / 32767.0);
}

// 0x4c0800: a vector's length, the squares summed on the stack and rounded to a float once
static float LengthX87(const Vec3& v)
{
	float aSquare = (float)(((double)v.y * (double)v.y + (double)v.x * (double)v.x) + (double)v.z * (double)v.z);
	return (float)std::sqrt((double)aSquare);
}

// 0x4c0840: a vector made unit, as its x87 code does it; left as it is when it has no length
static Vec3 UnitX87(const Vec3& v)
{
	float aLength = LengthX87(v);
	if (aLength == 0.0f)
		return v;
	float anInverse = (float)(1.0 / (double)aLength);
	return Vec3{v.x * anInverse, v.y * anInverse, v.z * anInverse};
}

// 0x68b421: three draws (x, y, z), each made -1 to 1, and the vector made unit
static Vec3 RandomDirection()
{
	Vec3 aVector;
	aVector.x = (float)(2.0 * (double)SnakeRandom() - 1.0);
	aVector.y = (float)(2.0 * (double)SnakeRandom() - 1.0);
	aVector.z = (float)(2.0 * (double)SnakeRandom() - 1.0);
	return UnitX87(aVector);
}

// 0x68b2a8: theTo = theFrom + theTo, each part added on the x87 stack and rounded to a float
static void AddTo(Vec3& theTo, const Vec3& theFrom)
{
	theTo.x = (float)((double)theFrom.x + (double)theTo.x);
	theTo.y = (float)((double)theFrom.y + (double)theTo.y);
	theTo.z = (float)((double)theFrom.z + (double)theTo.z);
}

// 0x673bb6: a less b
static Vec3 Difference(const Vec3& a, const Vec3& b)
{
	return Vec3{(float)((double)a.x - (double)b.x), (float)((double)a.y - (double)b.y), (float)((double)a.z - (double)b.z)};
}

// ---------------------------------------------------------------------------------------------------------------------
// Snake

// 0x68b535: the place is a draw of 3 (made unit) times 25, the heading another times 50
void VizMagicSnakes::Snake::Init(int theIndex)
{
	mIndex = theIndex;
	mOdd = theIndex & 1;
	Vec3 aPlace = RandomDirection();
	mPos = Vec3{(float)((double)aPlace.x * 25.0), (float)((double)aPlace.y * 25.0), (float)((double)aPlace.z * 25.0)};
	Vec3 aHeading = RandomDirection();
	mVel = Vec3{(float)((double)aHeading.x * 50.0), (float)((double)aHeading.y * 50.0), (float)((double)aHeading.z * 50.0)};
}

// 0x68b6e2: the snake's place turned to look along its heading (a heading all but nothing is made (1, 0, 0) first, and
// stays so), with y up
Transform3D VizMagicSnakes::Snake::MakeTransform()
{
	Transform3D aTransform;  // 0x4bd4c0
	if (LengthX87(mVel) < 0.0010000000474974513f)
		mVel = Vec3{1.0f, 0.0f, 0.0f};
	Vec3 anUp{0.0f, 1.0f, 0.0f};
	Vec3 aTarget{(float)((double)mVel.x + (double)mPos.x), (float)((double)mVel.y + (double)mPos.y),
		(float)((double)mVel.z + (double)mPos.z)};  // 0x4c0760
	aTransform.mPos = mPos;  // 0x67411e
	LookFrom(aTransform, aTarget, anUp);
	return aTransform;
}

// 0x68bc6c: the mean of the others' headings made unit (this snake's own heading when there are no others)
Vec3 VizMagicSnakes::Snake::MeanVelocity(const std::vector<Snake>& theSnakes) const
{
	Vec3 aSum;
	int aCount = 0;
	for (size_t i = 0; i < theSnakes.size(); i++) {
		if (&theSnakes[i] != this) {
			AddTo(aSum, theSnakes[i].mVel);
			aCount++;
		}
	}
	if (aCount == 0)
		return mVel;
	float anInverse = (float)(1.0 / (double)(float)aCount);  // 0x68b2f0
	aSum = Vec3{aSum.x * anInverse, aSum.y * anInverse, aSum.z * anInverse};
	return UnitX87(aSum);
}

// 0x68bcfb: from each other snake nearer than 20 a push away (10 times 20 over the distance), from each one farther than
// 50 a pull towards it (10 times the distance over 50), summed
Vec3 VizMagicSnakes::Snake::Repulsion(const std::vector<Snake>& theSnakes) const
{
	Vec3 aSum;
	for (size_t i = 0; i < theSnakes.size(); i++) {
		const Snake& anOther = theSnakes[i];
		if (&anOther == this)
			continue;
		float aDistance = LengthX87(Difference(anOther.mPos, mPos));
		if (aDistance < 20.0f) {
			float aScale = (float)((20.0 / (double)aDistance) * 10.0);
			Vec3 aAway = Difference(mPos, anOther.mPos);
			AddTo(aSum, Vec3{aAway.x * aScale, aAway.y * aScale, aAway.z * aScale});
		} else if (50.0 < (double)aDistance) {
			float aScale = (float)(((double)aDistance / 50.0) * 10.0);
			Vec3 aToward = Difference(anOther.mPos, mPos);
			AddTo(aSum, Vec3{aToward.x * aScale, aToward.y * aScale, aToward.z * aScale});
		}
	}
	return aSum;
}

// 0x68b467: nothing within 20 of the middle, beyond it a pull back to it (10 times the distance over 20)
Vec3 VizMagicSnakes::Snake::Wall() const
{
	Vec3 aResult;
	float aDistance = LengthX87(mPos);
	if (20.0f < aDistance) {
		float aScale = (float)(((double)aDistance / 20.0) * 10.0);
		Vec3 aBack{-mPos.x, -mPos.y, -mPos.z};
		AddTo(aResult, Vec3{aBack.x * aScale, aBack.y * aScale, aBack.z * aScale});
	}
	return aResult;
}

// 0x68be10: the size swells and shrinks with the update count (and the snake's number); the push is the mean heading of
// the others, the others' push and pull, and the wall's pull times 20, held to a hundredth of a quarter of the size; the
// heading takes the push and is held to three quarters of a quarter of the size; the place takes the heading
void VizMagicSnakes::Snake::Update(int theCount, std::vector<Snake>& theSnakes)
{
	float aBase = (float)((double)mIndex * 6.666666507720947);
	float anAngle = (float)((((double)aBase + (double)theCount * 0.25) * 3.1415927410125732) * 0.019999999552965164);
	mSize = (float)((double)sinf(anAngle) * 0.75 + 1.0);
	mPush = MeanVelocity(theSnakes);
	AddTo(mPush, Repulsion(theSnakes));
	Vec3 aWall = Wall();
	AddTo(mPush, Vec3{(float)((double)aWall.x * 20.0), (float)((double)aWall.y * 20.0), (float)((double)aWall.z * 20.0)});
	float aQuarter = (float)((double)mSize * 0.25);
	float aMaxPush = (float)((double)aQuarter * 0.009999999776482582);
	if (aMaxPush < LengthX87(mPush)) {
		Vec3 aUnit = UnitX87(mPush);
		mPush = Vec3{aUnit.x * aMaxPush, aUnit.y * aMaxPush, aUnit.z * aMaxPush};
	}
	AddTo(mVel, mPush);
	float aMaxVel = (float)((double)aQuarter * 0.75);
	if (aMaxVel < LengthX87(mVel)) {
		Vec3 aUnit = UnitX87(mVel);
		mVel = Vec3{aUnit.x * aMaxVel, aUnit.y * aMaxVel, aUnit.z * aMaxVel};
	}
	AddTo(mPos, mVel);
}

// ---------------------------------------------------------------------------------------------------------------------
// Segment and Trail

// 0x68b74a: the snake's transform (0x68b6e2) scaled by 1.5 over its size, and a turn of a sixteenth of a radian for each
// update (the other way for an odd snake)
void VizMagicSnakes::Segment::Set(Snake& theSnake, int theCount)
{
	mTransform = theSnake.MakeTransform();  // 0x673c10
	float aScale = (float)((1.0 / (double)theSnake.mSize) * 1.5);
	mTransform.mScale = Vec3{aScale * mTransform.mScale.x, aScale * mTransform.mScale.y, aScale * mTransform.mScale.z};  // 0x68b341
	mAngle = (float)((double)theCount * 0.0625);
	if (theSnake.mOdd != 0)
		mAngle = -mAngle;
}

// 0x68b7c7: theFraction (0 for the newest segment, up to 1) scales the triangle and fades it, from a hue by the fraction
// (an odd snake's from 224 down to 64, the others' from 128 up to 224); the triangle (0, 1, 0), (1, -1, 0), (-1, -1, 0)
// is turned about z by the angle and put in the snake's place
void VizMagicSnakes::Segment::Build(const Snake& theSnake, float theFraction)
{
	mFraction = theFraction;
	Transform3D aLocal = mTransform;  // 0x4bd500
	aLocal.mScale = Vec3{theFraction * aLocal.mScale.x, theFraction * aLocal.mScale.y, theFraction * aLocal.mScale.z};  // 0x68b341
	int anAlpha = Ftol((1.0 - (double)theFraction) * 255.0);  // 0x77e016 cuts toward zero
	float aHue;
	if (theSnake.mOdd != 0)
		aHue = (float)((double)theFraction * -160.0 + 224.0);
	else
		aHue = (float)((double)theFraction * 96.0 + 128.0);
	unsigned int aColor = ((unsigned int)gApp->HSLToRGB(Ftol((double)aHue), 0xff, 0x80) & 0xffffff) | ((unsigned int)anAlpha << 24);  // 0x4a7220
	aLocal.mRot.RotateZ(mAngle);  // 0x673fd0
	const Vec3 aCorners[3] = {{0.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {-1.0f, -1.0f, 0.0f}};
	int aCorner = 3;
	Vertex* aVertex = mVertices;
	const Vec3* aPoint = aCorners;
	do {
		Vec3 aPlace = TransformPoint(aLocal, *aPoint);  // 0x68b6b6
		aVertex->x = aPlace.x;
		aVertex->y = aPlace.y;
		aVertex->z = aPlace.z;
		aVertex->mColor = aColor;
		aVertex->u = 0.0f;
		aVertex->v = 0.0f;
		aVertex++;
		aPoint++;
		aCorner--;
	} while (aCorner != 0);
}

// 0x68bacc: on every third update (the snake's number and the count equal mod 3) a segment at the snake: a new one up to 61,
// after that the oldest, moved to be the newest
void VizMagicSnakes::Trail::Add(int theCount)
{
	if (mSnake != nullptr && theCount % 3 == mSnake->mIndex % 3) {
		if (mSegments.size() > 0x3c)
			mSegments.splice(mSegments.end(), mSegments, mSegments.begin());  // 0x68b9af
		else
			mSegments.emplace_back();
		mSegments.back().Set(*mSnake, theCount);
	}
}

// 0x68c04d: with two or more segments, each is built (the newest first, a fraction of its place over the count) and then
// each pair, the newer's triangle against the older's, makes three quads of six vertices (0x90 bytes) at theVertices: for
// each edge j, the older's vertex j (u 0, v 1), the newer's vertex j (0, 0), the newer's vertex j + 1 (1, 0), the first
// again, the third again, the older's vertex j + 1 (1, 1)
void VizMagicSnakes::Trail::BuildStrip(Vertex*& theVertices)
{
	int aCount = (int)mSegments.size();
	if (aCount < 2)
		return;
	int anIndex = 0;
	for (std::list<Segment>::reverse_iterator i = mSegments.rbegin(); i != mSegments.rend(); ++i) {
		i->Build(*mSnake, (float)((double)anIndex / (double)aCount));
		anIndex++;
	}
	Vertex* aWrite = theVertices;
	std::list<Segment>::reverse_iterator aNewer = mSegments.rbegin();
	anIndex = 0;
	if (0 < aCount - 1) {
		do {
			std::list<Segment>::reverse_iterator anOlder = aNewer;
			++anOlder;
			int aEdge = 0;
			do {
				aEdge++;
				int aNext = aEdge % 3;
				Vertex* aQuad = aWrite;
				aQuad[0] = anOlder->mVertices[aEdge - 1];
				aQuad[0].u = 0.0f;
				aQuad[0].v = 1.0f;
				aQuad[1] = aNewer->mVertices[aEdge - 1];
				aQuad[1].u = 0.0f;
				aQuad[1].v = 0.0f;
				aQuad[2] = aNewer->mVertices[aNext];
				aQuad[2].u = 1.0f;
				aQuad[2].v = 0.0f;
				aQuad[3] = aQuad[0];
				aQuad[4] = aQuad[2];
				aQuad[5] = anOlder->mVertices[aNext];
				aQuad[5].u = 1.0f;
				aQuad[5].v = 1.0f;
				aWrite += 6;
			} while (aEdge < 3);
			anIndex++;
			aNewer = anOlder;
		} while (anIndex < aCount - 1);
	}
	theVertices = aWrite;
}

// ---------------------------------------------------------------------------------------------------------------------
// VizMagicSnakes

// 0x68c2dd: IMAGE_MAGICSNAKE_FRAGMENT and IMAGE_MAGICSNAKE_BG, and the manager's fifteen snakes (0x68c217, each by 0x68c1b7:
// the snake, then its trail); drawn anywhere on the screen, not only round the board (bit 0 of +0x4 cleared)
VizMagicSnakes::VizMagicSnakes(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mFragment = LoadVizImage(IMAGE_MAGICSNAKE_FRAGMENT_ID);
	mBackground = LoadVizImage(IMAGE_MAGICSNAKE_BG_ID);
	mSnakes.reserve(0xf);
	mTrails.reserve(0xf);
	int anIndex = 0;
	do {
		mSnakes.emplace_back();
		mSnakes.back().Init(anIndex);
		mTrails.emplace_back();
		mTrails.back().mSnake = &mSnakes.back();
		anIndex++;
	} while (anIndex < 0xf);
	mFlags4 &= ~1;
}

// 0x68c6ff (0x68c65e): the manager goes with the members, then IMAGE_MAGICSNAKE_FRAGMENT and IMAGE_MAGICSNAKE_BG
VizMagicSnakes::~VizMagicSnakes()
{
	mTrails.clear();
	mSnakes.clear();
	DeleteVizImage(IMAGE_MAGICSNAKE_FRAGMENT_ID);
	DeleteVizImage(IMAGE_MAGICSNAKE_BG_ID);
}

// 0x68c36b (Init, 0x68b5e4, does nothing): the count, then the manager's update (0x68c22e): each snake in turn (each sees
// the others as they stand, the earlier ones already moved), then each trail
void VizMagicSnakes::Update()
{
	mCount++;
	for (size_t i = 0; i < mSnakes.size(); i++)
		mSnakes[i].Update(mCount, mSnakes);
	for (size_t i = 0; i < mTrails.size(); i++)
		mTrails[i].Add(mCount);
}

// 0x68c37f: the background, then every trail's ribbon in one DrawPrimitiveEx (triangle list, IMAGE_MAGICSNAKE_FRAGMENT
// wrapped), under a perspective camera at (0, 0, 30) looking at the middle, no depth test or write, culling on
void VizMagicSnakes::Draw(Graphics* g)
{
	g->PushState();                // 0x41fd70
	g->SetColorizeImages(true);    // 0x41ff40
	g->SetColor(Color(0xffffff));  // 0x4016b0, 0x41fef0
	DrawImage(g, mBackground);     // 0x67fdb6
	{
		PerspectiveCamera aCamera(90.0f, 1.3333333730697632f, 1.0f, 10000.0f);  // 0x67417d
		WorldGraphicsListener aListener(&aCamera, true);                       // 0x4bd5d0
		aListener.SetCulling(true);                                            // 0x4bd6c0
		aListener.SetDepth(false, false);                                      // 0x4bd680
		{
			// 0x674143: the camera's transform put at the place and turned to look at the target (0x67411e)
			Transform3D aPlace = aCamera.mTransform;
			aPlace.mPos = Vec3{0.0f, 0.0f, 30.0f};
			if (LookFrom(aPlace, Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}))
				aCamera.mTransform = aPlace;
		}
		// 0x68b5c1: each trail of two or more segments gives 6 triangles (18 vertices) for each pair
		int aTriangles = 0;
		int aVertexCount = 0;
		for (size_t i = 0; i < mTrails.size(); i++) {
			int aSegments = (int)mTrails[i].mSegments.size();
			if (1 < aSegments) {
				int aPairs = aSegments - 1;
				aTriangles += aPairs * 6;
				aVertexCount += aPairs * 0x12;
			}
		}
		std::vector<Vertex> aVertices((size_t)aVertexCount);
		gD3DInterface->SetTexture(0, mFragment);  // slot 0xd4
		gD3DInterface->SetTextureWrap(0, true);   // slot 0xcc
		aListener.mWorld = Transform3D();         // 0x4bd4c0, 0x673c10
		Vertex* aWrite = aVertices.data();
		for (size_t i = 0; i < mTrails.size(); i++)
			mTrails[i].BuildStrip(aWrite);
		PrimitiveListener aPrimitiveListener(&aListener);
		gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLELIST, aVertices.data(), aTriangles, Color(0xff, 0xff, 0xff, 0xff), 1,
			0.0f, 0.0f, true, &aPrimitiveListener);  // slot 0x7c
		gD3DInterface->SetTexture(0, nullptr);
		gD3DInterface->SetTextureWrap(0, false);
	}
	g->PopState();  // 0x41fe00
}
