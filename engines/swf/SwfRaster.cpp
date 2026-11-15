#include "SwfRaster.h"

#include <limits.h>
#include <math.h>
#include <string.h>
#include <algorithm>

#ifdef _MSC_VER
#include <intrin.h>
#endif

using namespace Sexy;

namespace {

const float kMinCoverage = 1.0f / 512.0f;  // less than this covers nothing
const float kFullCoverage = 0.998f;        // more than this covers all
const float kFlatness = 0.15f;              // how far a flattened curve may stray from the curve, in pixels
const float kPi = 3.14159265358979f;
const int kLutCacheSets = 128;
const int kLutCacheWays = 4;

// theDest moved theWeight/256 of the way to the color (theWeight 0 to 256); the pixel stays opaque
inline uint32_t Blend(uint32_t theDest, int theR, int theG, int theB, int theWeight)
{
	int r = (theDest >> 16) & 255;
	int g = (theDest >> 8) & 255;
	int b = theDest & 255;
	r += ((theR - r) * theWeight) >> 8;
	g += ((theG - g) * theWeight) >> 8;
	b += ((theB - b) * theWeight) >> 8;
	return 0xFF000000u | (r << 16) | (g << 8) | b;
}

inline int ClampByte(float theValue)
{
	int v = (int)(theValue + 0.5f);
	return v < 0 ? 0 : (v > 255 ? 255 : v);
}

// The number of zero bits below the lowest set bit
inline int LowestBit(uint64_t theBits)
{
#ifdef _MSC_VER
	unsigned long anIndex;
	_BitScanForward64(&anIndex, theBits);
	return (int)anIndex;
#else
	return __builtin_ctzll(theBits);
#endif
}

// Edges by the row they start in; a few edges are not worth a call to sort
template <class T>
void SortEdges(std::vector<T>& theEdges)
{
	size_t n = theEdges.size();
	if (n > 32) {
		std::sort(theEdges.begin(), theEdges.end(), [](const T& l, const T& r) { return l.mY0 < r.mY0; });
		return;
	}
	for (size_t i = 1; i < n; i++) {
		T e = theEdges[i];
		size_t j = i;
		while (j > 0 && theEdges[j - 1].mY0 > e.mY0) {
			theEdges[j] = theEdges[j - 1];
			j--;
		}
		theEdges[j] = e;
	}
}

}  // namespace

void SwfRect::Union(const SwfRect& theOther)
{
	if (theOther.IsEmpty())
		return;
	if (IsEmpty()) {
		*this = theOther;
		return;
	}
	mX0 = std::min(mX0, theOther.mX0);
	mY0 = std::min(mY0, theOther.mY0);
	mX1 = std::max(mX1, theOther.mX1);
	mY1 = std::max(mY1, theOther.mY1);
}

bool SwfRect::Intersects(const SwfRect& theOther) const
{
	return mX0 < theOther.mX1 && theOther.mX0 < mX1 && mY0 < theOther.mY1 && theOther.mY0 < mY1 && !IsEmpty() && !theOther.IsEmpty();
}

void SwfRect::Intersect(const SwfRect& theOther)
{
	mX0 = std::max(mX0, theOther.mX0);
	mY0 = std::max(mY0, theOther.mY0);
	mX1 = std::min(mX1, theOther.mX1);
	mY1 = std::min(mY1, theOther.mY1);
}

SwfRaster::SwfRaster() {}

void SwfRaster::SetTarget(uint32_t* theBits, int theWidth, int theHeight, int theStride)
{
	if (theWidth != mWidth || theHeight != mHeight) {
		mMasks.clear();
		mCells.assign(theWidth + 3, 0.0f);
		mTouchBits.assign((theWidth + 3 + 63) / 64 + 1, 0);
		AccPx aZero = {0, 0, 0, 0};
		mAcc.assign(theWidth, aZero);
		mAccTouched.clear();
	}
	mBits = theBits;
	mWidth = theWidth;
	mHeight = theHeight;
	mStride = theStride;
	mMasksUsed = 0;
	mClip = nullptr;
	mMaskTarget = nullptr;
	mDamage.mX0 = mDamage.mY0 = 0;
	mDamage.mX1 = theWidth;
	mDamage.mY1 = theHeight;
}

void SwfRaster::SetDamage(const SwfRect& theRect)
{
	mDamage = theRect;
	SwfRect aTarget;
	aTarget.mX1 = mWidth;
	aTarget.mY1 = mHeight;
	mDamage.Intersect(aTarget);
}

void SwfRaster::Fill(uint32_t theColor)
{
	if (mDamage.IsEmpty())
		return;
	uint32_t aPixel = 0xFF000000u | (theColor & 0xFFFFFF);
	for (int y = mDamage.mY0; y < mDamage.mY1; y++)
		std::fill(mBits + (size_t)y * mStride + mDamage.mX0, mBits + (size_t)y * mStride + mDamage.mX1, aPixel);
}

// ---- masks ----

SwfMask* SwfRaster::AllocMask()
{
	if (mMasksUsed == (int)mMasks.size()) {
		mMasks.push_back(std::unique_ptr<SwfMask>(new SwfMask()));
		mMasks.back()->mData.assign((size_t)mWidth * mHeight, 0);
	}
	SwfMask* aMask = mMasks[mMasksUsed++].get();
	// A mask that was used before is zero but for the rectangle it names
	if (!aMask->IsEmpty()) {
		for (int y = aMask->mY0; y < aMask->mY1; y++)
			memset(&aMask->mData[(size_t)y * mWidth + aMask->mX0], 0, aMask->mX1 - aMask->mX0);
	}
	aMask->mX0 = aMask->mY0 = aMask->mX1 = aMask->mY1 = 0;
	return aMask;
}

SwfMask* SwfRaster::Intersect(const SwfMask* theFirst, const SwfMask* theSecond)
{
	SwfMask* aMask = AllocMask();
	if (theFirst == nullptr || theSecond == nullptr) {
		const SwfMask* anOnly = theFirst ? theFirst : theSecond;
		if (anOnly == nullptr)
			return aMask;
		theFirst = theSecond = anOnly;
	}
	int x0 = std::max(theFirst->mX0, theSecond->mX0);
	int y0 = std::max(theFirst->mY0, theSecond->mY0);
	int x1 = std::min(theFirst->mX1, theSecond->mX1);
	int y1 = std::min(theFirst->mY1, theSecond->mY1);
	if (x1 <= x0 || y1 <= y0)
		return aMask;
	for (int y = y0; y < y1; y++) {
		const uint8_t* a = &theFirst->mData[(size_t)y * mWidth];
		const uint8_t* b = &theSecond->mData[(size_t)y * mWidth];
		uint8_t* o = &aMask->mData[(size_t)y * mWidth];
		for (int x = x0; x < x1; x++)
			o[x] = (uint8_t)((a[x] * b[x] * 257 + 32768) >> 16);
	}
	aMask->mX0 = x0;
	aMask->mY0 = y0;
	aMask->mX1 = x1;
	aMask->mY1 = y1;
	return aMask;
}

// ---- setting up a job ----

// The rectangle (in pixels) that the next shape can reach: the shape's own bounds within what may be drawn, and the
// mask's within that. False when nothing would show.
bool SwfRaster::BeginJob(float theMinX, float theMinY, float theMaxX, float theMaxY)
{
	mNumLayers = 0;
	mJob++;
	if (!(theMaxX > (float)mDamage.mX0 && theMaxY > (float)mDamage.mY0 && theMinX < (float)mDamage.mX1 && theMinY < (float)mDamage.mY1))
		return false;
	mClipX0 = (int)floorf(std::max(theMinX, (float)mDamage.mX0));
	mClipY0 = (int)floorf(std::max(theMinY, (float)mDamage.mY0));
	mClipX1 = (int)ceilf(std::min(theMaxX, (float)mDamage.mX1));
	mClipY1 = (int)ceilf(std::min(theMaxY, (float)mDamage.mY1));
	if (mMaskTarget == nullptr && mClip != nullptr) {
		if (mClip->IsEmpty())
			return false;
		mClipX0 = std::max(mClipX0, mClip->mX0);
		mClipY0 = std::max(mClipY0, mClip->mY0);
		mClipX1 = std::min(mClipX1, mClip->mX1);
		mClipY1 = std::min(mClipY1, mClip->mY1);
	}
	return mClipX1 > mClipX0 && mClipY1 > mClipY0;
}

SwfRaster::Layer* SwfRaster::NewLayer(bool thePartition)
{
	if (mNumLayers == (int)mLayers.size())
		mLayers.push_back(std::unique_ptr<Layer>(new Layer()));
	Layer* aLayer = mLayers[mNumLayers++].get();
	aLayer->mEdges.clear();
	aLayer->mNext = 0;
	aLayer->mActive.clear();
	aLayer->mPartition = thePartition;
	aLayer->mMinY = 1e30f;
	aLayer->mMaxY = -1e30f;
	return aLayer;
}

bool SwfRaster::SetSolidPaint(Layer& theLayer, const SwfColor& theColor, const SwfCXForm& theCX)
{
	Paint& p = theLayer.mPaint;
	p.mKind = 0;
	if (mMaskTarget != nullptr) {
		p.mR = p.mG = p.mB = p.mA = 255;
		p.mPR = p.mPG = p.mPB = 255.0f;
		p.mPA = 1.0f;
		p.mArgb = 0xFFFFFFFFu;
		return true;
	}
	p.mR = ClampByte(theColor.r * theCX.mMul[0] + theCX.mAdd[0]);
	p.mG = ClampByte(theColor.g * theCX.mMul[1] + theCX.mAdd[1]);
	p.mB = ClampByte(theColor.b * theCX.mMul[2] + theCX.mAdd[2]);
	p.mA = ClampByte(theColor.a * theCX.mMul[3] + theCX.mAdd[3]);
	p.mArgb = 0xFF000000u | (p.mR << 16) | (p.mG << 8) | p.mB;
	p.mPA = p.mA * (1.0f / 255.0f);
	p.mPR = p.mR * p.mPA;
	p.mPG = p.mG * p.mPA;
	p.mPB = p.mB * p.mPA;
	return p.mA > 0;
}

// The 256 colors of a gradient with a color transform applied to its stops; false when none of it shows
static bool BuildLut(const SwfFillStyle& theStyle, const SwfCXForm& theCX, uint32_t* theLut, bool& theOpaque)
{
	size_t aCount = std::min<size_t>(theStyle.mStops.size(), 16);
	float aColors[16][4];
	bool anyVisible = false;
	theOpaque = true;
	for (size_t i = 0; i < aCount; i++) {
		const SwfColor& c = theStyle.mStops[i].mColor;
		aColors[i][0] = std::min(255.0f, std::max(0.0f, c.r * theCX.mMul[0] + theCX.mAdd[0]));
		aColors[i][1] = std::min(255.0f, std::max(0.0f, c.g * theCX.mMul[1] + theCX.mAdd[1]));
		aColors[i][2] = std::min(255.0f, std::max(0.0f, c.b * theCX.mMul[2] + theCX.mAdd[2]));
		aColors[i][3] = std::min(255.0f, std::max(0.0f, c.a * theCX.mMul[3] + theCX.mAdd[3]));
		anyVisible = anyVisible || aColors[i][3] > 0;
	}
	if (!anyVisible)
		return false;
	size_t aSeg = 0;
	for (int i = 0; i < 256; i++) {
		while (aSeg + 1 < aCount && theStyle.mStops[aSeg + 1].mRatio <= i)
			aSeg++;
		float c[4];
		if (i <= theStyle.mStops[0].mRatio || aSeg + 1 >= aCount) {
			size_t aFrom = (i <= theStyle.mStops[0].mRatio) ? 0 : aCount - 1;
			for (int k = 0; k < 4; k++)
				c[k] = aColors[aFrom][k];
		}
		else {
			float aR0 = theStyle.mStops[aSeg].mRatio, aR1 = theStyle.mStops[aSeg + 1].mRatio;
			float t = aR1 > aR0 ? (i - aR0) / (aR1 - aR0) : 1.0f;
			for (int k = 0; k < 4; k++)
				c[k] = aColors[aSeg][k] + (aColors[aSeg + 1][k] - aColors[aSeg][k]) * t;
		}
		int a = ClampByte(c[3]);
		theOpaque = theOpaque && a == 255;
		theLut[i] = ((uint32_t)a << 24) | ((uint32_t)ClampByte(c[0]) << 16) | ((uint32_t)ClampByte(c[1]) << 8) | (uint32_t)ClampByte(c[2]);
	}
	return true;
}

bool SwfRaster::SetFillPaint(Layer& theLayer, const SwfFillStyle& theStyle, const SwfMatrix& theMatrix, const SwfCXForm& theCX,
                             bool theCacheable)
{
	if (theStyle.mType == 0 || mMaskTarget != nullptr)
		return SetSolidPaint(theLayer, theStyle.mColor, theCX);
	if (theStyle.mType >= 0x40) {
		// a bitmap: the fill's matrix takes its pixels to the shape, so the inverse of that and the shape's own takes a
		// pixel of the target to the bitmap's
		if (theStyle.mBitmap == nullptr)
			return false;
		SwfMatrix aBitmapToDevice = SwfMatrix::Concat(theMatrix, theStyle.mMatrix);
		SwfMatrix aBack;
		if (!aBitmapToDevice.Invert(aBack))
			return false;
		Paint& p = theLayer.mPaint;
		p.mKind = 3;
		p.mIA = aBack.a;
		p.mIB = aBack.b;
		p.mIC = aBack.c;
		p.mID = aBack.d;
		p.mITX = aBack.tx;
		p.mITY = aBack.ty;
		p.mBitmap = theStyle.mBitmap.get();
		p.mRepeat = (theStyle.mType & 1) == 0;
		p.mSmooth = (theStyle.mType & 2) == 0;
		p.mHasCX = !theCX.IsIdentity();
		for (int i = 0; i < 4; i++) {
			p.mCXMul[i] = theCX.mMul[i];
			p.mCXAdd[i] = theCX.mAdd[i];
		}
		return true;
	}
	if (theStyle.mStops.empty())
		return false;

	Paint& p = theLayer.mPaint;
	SwfMatrix aGradToDevice = SwfMatrix::Concat(theMatrix, theStyle.mMatrix);
	SwfMatrix anInverse;
	if (!aGradToDevice.Invert(anInverse))
		return false;
	p.mKind = theStyle.mType == 0x10 ? 1 : 2;
	p.mIA = anInverse.a;
	p.mIB = anInverse.b;
	p.mIC = anInverse.c;
	p.mID = anInverse.d;
	p.mITX = anInverse.tx;
	p.mITY = anInverse.ty;

	// What the gradient looks like depends on its stops and the color transform alone, and a file repeats the same
	// gradients in many places, so the colors are kept by what they are, not where they came from
	if (theCacheable && theStyle.mStopsHash != 0) {
		if (mLutCache.empty())
			mLutCache.resize(kLutCacheSets * kLutCacheWays);
		uint64_t aHash = theStyle.mStopsHash;
		LutEntry* aSet = &mLutCache[((aHash ^ (aHash >> 32)) % kLutCacheSets) * kLutCacheWays];
		LutEntry* anEntry = nullptr;
		LutEntry* aVictim = nullptr;
		for (int w = 0; w < kLutCacheWays && anEntry == nullptr; w++) {
			if (aSet[w].mHash == aHash && memcmp(&aSet[w].mCX, &theCX, sizeof(SwfCXForm)) == 0)
				anEntry = &aSet[w];
			else if (aSet[w].mJob != mJob && (aVictim == nullptr || aSet[w].mJob < aVictim->mJob))
				aVictim = &aSet[w];
		}
		if (anEntry == nullptr && aVictim != nullptr) {
			aVictim->mHash = 0;
			if (!BuildLut(theStyle, theCX, aVictim->mLut, aVictim->mOpaque))
				return false;
			aVictim->mHash = aHash;
			aVictim->mCX = theCX;
			anEntry = aVictim;
		}
		if (anEntry != nullptr) {
			anEntry->mJob = mJob;
			p.mLut = anEntry->mLut;
			p.mOpaqueLut = anEntry->mOpaque;
			return true;
		}
	}
	p.mLut = theLayer.mLut;
	return BuildLut(theStyle, theCX, theLayer.mLut, p.mOpaqueLut);
}

// ---- edges ----

void SwfRaster::AddEdge(Layer& theLayer, float theX0, float theY0, float theX1, float theY1)
{
	if (theY0 == theY1)
		return;
	int aDir = 1;
	if (theY0 > theY1) {
		std::swap(theX0, theX1);
		std::swap(theY0, theY1);
		aDir = -1;
	}
	// Above or below the rectangle, or right of it, an edge changes nothing that shows
	if (theY1 <= (float)mClipY0 || theY0 >= (float)mClipY1)
		return;
	if (theX0 >= (float)mClipX1 && theX1 >= (float)mClipX1)
		return;
	Edge e;
	e.mX0 = theX0;
	e.mY0 = theY0;
	e.mY1 = theY1;
	e.mDxDy = (theX1 - theX0) / (theY1 - theY0);
	e.mDir = aDir;
	theLayer.mEdges.push_back(e);
	theLayer.mMinY = std::min(theLayer.mMinY, theY0);
	theLayer.mMaxY = std::max(theLayer.mMaxY, theY1);
}

// The segments of a shape moved by theMatrix, with their curves flattened
void SwfRaster::AddSegs(Layer& theLayer, const SwfSeg* theSegs, size_t theCount, const SwfMatrix& m)
{
	for (size_t i = 0; i < theCount; i++) {
		const SwfSeg& s = theSegs[i];
		float x0, y0, x1, y1;
		m.Apply(s.mX0, s.mY0, x0, y0);
		m.Apply(s.mX1, s.mY1, x1, y1);
		if (!s.mCurve) {
			AddEdge(theLayer, x0, y0, x1, y1);
			continue;
		}
		float cx, cy;
		m.Apply(s.mCX, s.mCY, cx, cy);
		// A quadratic curve strays |p0 - 2c + p1| / (4 n * n) from its chord when cut into n pieces
		float dx = x0 - 2 * cx + x1, dy = y0 - 2 * cy + y1;
		float aLength = sqrtf(dx * dx + dy * dy);
		int n = (int)ceilf(sqrtf(aLength / (4 * kFlatness)));
		if (n < 1)
			n = 1;
		else if (n > 64)
			n = 64;
		float px = x0, py = y0;
		for (int k = 1; k <= n; k++) {
			float t = (float)k / n, u = 1 - t;
			float qx = u * u * x0 + 2 * u * t * cx + t * t * x1;
			float qy = u * u * y0 + 2 * u * t * cy + t * t * y1;
			if (k == n) {
				qx = x1;
				qy = y1;
			}
			AddEdge(theLayer, px, py, qx, qy);
			px = qx;
			py = qy;
		}
	}
}

// Polygons of points in a shape's space (x then y), the end of each in theEnds, with their edges moved by theMatrix
void SwfRaster::AddPolygons(Layer& theLayer, const float* thePoints, const int* theEnds, size_t theCount, const SwfMatrix& m)
{
	int aStart = 0;
	for (size_t p = 0; p < theCount; p++) {
		int anEnd = theEnds[p];
		if (anEnd - aStart >= 3) {
			float fx, fy;
			m.Apply(thePoints[aStart * 2], thePoints[aStart * 2 + 1], fx, fy);
			float px = fx, py = fy;
			for (int i = aStart + 1; i < anEnd; i++) {
				float x, y;
				m.Apply(thePoints[i * 2], thePoints[i * 2 + 1], x, y);
				AddEdge(theLayer, px, py, x, y);
				px = x;
				py = y;
			}
			AddEdge(theLayer, px, py, fx, fy);
		}
		aStart = anEnd;
	}
}

// ---- strokes ----

namespace {

inline void Rotate(float theX, float theY, float theAngle, float& theOutX, float& theOutY)
{
	float c = cosf(theAngle), s = sinf(theAngle);
	theOutX = theX * c - theY * s;
	theOutY = theX * s + theY * c;
}

}  // namespace

// A corner of the line at theP, where a segment going thePrev gives way to one going theNext (both unit vectors).
// Adds the corner to both sides of the outline: where the line turns slightly, one point on each side; where it
// turns sharply, the outer side goes round the corner in an arc and the inner side through the corner itself,
// which is the way to keep the inner side from folding the outline inside out.
void SwfRaster::AddJoin(const Pt& theP, const Pt& thePrev, const Pt& theNext, float theHalf, float theArcStep)
{
	float aCross = thePrev.x * theNext.y - thePrev.y * theNext.x;
	float aDot = thePrev.x * theNext.x + thePrev.y * theNext.y;
	Pt aPrevN = {-thePrev.y, thePrev.x};
	Pt aNextN = {-theNext.y, theNext.x};
	if (aDot > 0.98f) {
		float k = theHalf / (1 + aDot);
		float mx = (aPrevN.x + aNextN.x) * k, my = (aPrevN.y + aNextN.y) * k;
		mLeft.push_back({theP.x + mx, theP.y + my});
		mRight.push_back({theP.x - mx, theP.y - my});
		return;
	}
	float aTurn = atan2f(aCross, aDot);
	int aSteps = (int)ceilf(fabsf(aTurn) / theArcStep);
	if (aSteps < 1)
		aSteps = 1;
	if (aCross > 0) {
		mLeft.push_back({theP.x + theHalf * aPrevN.x, theP.y + theHalf * aPrevN.y});
		mLeft.push_back(theP);
		mLeft.push_back({theP.x + theHalf * aNextN.x, theP.y + theHalf * aNextN.y});
		for (int k = 0; k <= aSteps; k++) {
			float vx, vy;
			Rotate(-aPrevN.x, -aPrevN.y, aTurn * k / aSteps, vx, vy);
			mRight.push_back({theP.x + theHalf * vx, theP.y + theHalf * vy});
		}
	}
	else {
		mRight.push_back({theP.x - theHalf * aPrevN.x, theP.y - theHalf * aPrevN.y});
		mRight.push_back(theP);
		mRight.push_back({theP.x - theHalf * aNextN.x, theP.y - theHalf * aNextN.y});
		for (int k = 0; k <= aSteps; k++) {
			float vx, vy;
			Rotate(aPrevN.x, aPrevN.y, aTurn * k / aSteps, vx, vy);
			mLeft.push_back({theP.x + theHalf * vx, theP.y + theHalf * vy});
		}
	}
}

// Outlines the polyline in mChain (in shape space) and appends the outline's polygons. An open line gets one polygon,
// the left side out, a round cap, the right side back and a round cap; a closed one gets two, the left side and
// the right side backwards, which the non-zero rule turns into a ring.
// Adds the polygon in mPoly to the outline being made
void SwfRaster::AppendPolygon()
{
	for (size_t i = 0; i < mPoly.size(); i++) {
		mStrokePoints.push_back(mPoly[i].x);
		mStrokePoints.push_back(mPoly[i].y);
	}
	mStrokeEnds.push_back((int)(mStrokePoints.size() / 2));
}

void SwfRaster::StrokeChain(bool theClosed, float theHalf, float theArcStep)
{
	// Points that coincide have no direction
	size_t n = 0;
	for (size_t i = 0; i < mChain.size(); i++) {
		if (n > 0) {
			float dx = mChain[i].x - mChain[n - 1].x, dy = mChain[i].y - mChain[n - 1].y;
			if (dx * dx + dy * dy < 1e-6f)
				continue;
		}
		mChain[n++] = mChain[i];
	}
	mChain.resize(n);
	if (theClosed && n > 1) {
		float dx = mChain[0].x - mChain[n - 1].x, dy = mChain[0].y - mChain[n - 1].y;
		if (dx * dx + dy * dy < 1e-6f)
			mChain.resize(--n);
	}
	if (n == 0)
		return;
	mPoly.clear();
	if (n == 1) {
		// A dot: a round cap on a line of no length
		int aSteps = std::max(8, (int)ceilf(2 * kPi / theArcStep));
		for (int k = 0; k < aSteps; k++) {
			float a = 2 * kPi * k / aSteps;
			mPoly.push_back({mChain[0].x + theHalf * cosf(a), mChain[0].y + theHalf * sinf(a)});
		}
		AppendPolygon();
		return;
	}
	if (theClosed && n < 3)
		theClosed = false;

	size_t aSegs = theClosed ? n : n - 1;
	mDirs.resize(aSegs);
	for (size_t i = 0; i < aSegs; i++) {
		const Pt& a = mChain[i];
		const Pt& b = mChain[(i + 1) % n];
		float dx = b.x - a.x, dy = b.y - a.y;
		float aLength = sqrtf(dx * dx + dy * dy);
		mDirs[i] = {dx / aLength, dy / aLength};
	}

	mLeft.clear();
	mRight.clear();
	if (!theClosed) {
		mLeft.push_back({mChain[0].x - theHalf * mDirs[0].y, mChain[0].y + theHalf * mDirs[0].x});
		mRight.push_back({mChain[0].x + theHalf * mDirs[0].y, mChain[0].y - theHalf * mDirs[0].x});
		for (size_t i = 1; i + 1 < n; i++)
			AddJoin(mChain[i], mDirs[i - 1], mDirs[i], theHalf, theArcStep);
		const Pt& e = mChain[n - 1];
		const Pt& d = mDirs[n - 2];
		mLeft.push_back({e.x - theHalf * d.y, e.y + theHalf * d.x});
		mRight.push_back({e.x + theHalf * d.y, e.y - theHalf * d.x});

		mPoly = mLeft;
		int aCapSteps = std::max(2, (int)ceilf(kPi / theArcStep));
		for (int k = 1; k < aCapSteps; k++) {
			float vx, vy;
			Rotate(-d.y, d.x, -kPi * k / aCapSteps, vx, vy);
			mPoly.push_back({e.x + theHalf * vx, e.y + theHalf * vy});
		}
		for (size_t i = mRight.size(); i > 0; i--)
			mPoly.push_back(mRight[i - 1]);
		const Pt& s = mChain[0];
		const Pt& d0 = mDirs[0];
		for (int k = 1; k < aCapSteps; k++) {
			float vx, vy;
			Rotate(d0.y, -d0.x, -kPi * k / aCapSteps, vx, vy);
			mPoly.push_back({s.x + theHalf * vx, s.y + theHalf * vy});
		}
		AppendPolygon();
	}
	else {
		for (size_t i = 0; i < n; i++)
			AddJoin(mChain[i], mDirs[(i + n - 1) % n], mDirs[i], theHalf, theArcStep);
		mPoly = mLeft;
		AppendPolygon();
		mPoly.assign(mRight.rbegin(), mRight.rend());
		AppendPolygon();
	}
}

void SwfRaster::AddStroke(Layer& theLayer, const SwfLineLayer& theLine, float theWidth, const SwfMatrix& m, bool theCacheable)
{
	float aScale = sqrtf(fabsf(m.a * m.d - m.b * m.c));
	if (aScale < 1e-9f)
		return;
	if (theCacheable && theLine.mCacheScale > 0 && fabsf(aScale - theLine.mCacheScale) <= theLine.mCacheScale * 1e-3f) {
		AddPolygons(theLayer, theLine.mCachePoints.data(), theLine.mCachePolyEnds.data(), theLine.mCachePolyEnds.size(), m);
		return;
	}
	mStrokePoints.clear();
	mStrokeEnds.clear();
	// A hairline is one pixel wide at any scale
	float aRadius = (theWidth > 0 ? theWidth * aScale : 1.0f) * 0.5f;
	float aHalf = aRadius / aScale;  // in the shape's own space
	float aArcStep = aRadius > kFlatness ? 2 * acosf(1 - kFlatness / aRadius) : 1.5f;
	if (aArcStep > 1.0f)
		aArcStep = 1.0f;

	int aStart = 0;
	for (size_t p = 0; p < theLine.mPathEnds.size(); p++) {
		int anEnd = theLine.mPathEnds[p];
		mChain.clear();
		mChain.push_back({theLine.mSegs[aStart].mX0, theLine.mSegs[aStart].mY0});
		for (int i = aStart; i < anEnd; i++) {
			const SwfSeg& s = theLine.mSegs[i];
			if (s.mCurve) {
				float dx = s.mX0 - 2 * s.mCX + s.mX1, dy = s.mY0 - 2 * s.mCY + s.mY1;
				float aLength = sqrtf(dx * dx + dy * dy) * aScale;
				int n = (int)ceilf(sqrtf(aLength / (4 * kFlatness)));
				n = std::min(std::max(n, 1), 64);
				for (int k = 1; k < n; k++) {
					float t = (float)k / n, u = 1 - t;
					mChain.push_back({u * u * s.mX0 + 2 * u * t * s.mCX + t * t * s.mX1,
					                  u * u * s.mY0 + 2 * u * t * s.mCY + t * t * s.mY1});
				}
			}
			mChain.push_back({s.mX1, s.mY1});
		}
		aStart = anEnd;
		bool aClosed = mChain.size() > 2 && mChain.front().x == mChain.back().x && mChain.front().y == mChain.back().y;
		StrokeChain(aClosed, aHalf, aArcStep);
	}
	if (theCacheable) {
		theLine.mCacheScale = aScale;
		theLine.mCachePoints = mStrokePoints;
		theLine.mCachePolyEnds = mStrokeEnds;
	}
	AddPolygons(theLayer, mStrokePoints.data(), mStrokeEnds.data(), mStrokeEnds.size(), m);
}


// ---- drawing ----

bool SwfRaster::TransformBounds(float theMinX, float theMinY, float theMaxX, float theMaxY, const SwfMatrix& m, SwfRect& theRect)
{
	// A pixel wider all round for the edges' anti-aliasing, and for a hairline's width
	float xs[4], ys[4];
	m.Apply(theMinX, theMinY, xs[0], ys[0]);
	m.Apply(theMaxX, theMinY, xs[1], ys[1]);
	m.Apply(theMinX, theMaxY, xs[2], ys[2]);
	m.Apply(theMaxX, theMaxY, xs[3], ys[3]);
	float aMinX = std::min(std::min(xs[0], xs[1]), std::min(xs[2], xs[3])) - 1.5f;
	float aMaxX = std::max(std::max(xs[0], xs[1]), std::max(xs[2], xs[3])) + 1.5f;
	float aMinY = std::min(std::min(ys[0], ys[1]), std::min(ys[2], ys[3])) - 1.5f;
	float aMaxY = std::max(std::max(ys[0], ys[1]), std::max(ys[2], ys[3])) + 1.5f;
	const float kLimit = 100000.0f;
	theRect.mX0 = (int)floorf(std::max(aMinX, -kLimit));
	theRect.mY0 = (int)floorf(std::max(aMinY, -kLimit));
	theRect.mX1 = (int)ceilf(std::min(aMaxX, kLimit));
	theRect.mY1 = (int)ceilf(std::min(aMaxY, kLimit));
	return !theRect.IsEmpty();
}

bool SwfRaster::ShapeBounds(const SwfShape& theShape, const SwfMatrix& m, SwfRect& theRect)
{
	if (theShape.mEmpty)
		return false;
	return TransformBounds(theShape.mMinX, theShape.mMinY, theShape.mMaxX, theShape.mMaxY, m, theRect);
}

void SwfRaster::DrawShape(const SwfShape& theShape, const SwfMatrix& m, const SwfCXForm& theCX, bool theCacheable)
{
	SwfRect aBounds;
	if (!ShapeBounds(theShape, m, aBounds))
		return;
	if (!BeginJob((float)aBounds.mX0, (float)aBounds.mY0, (float)aBounds.mX1, (float)aBounds.mY1))
		return;

	for (size_t i = 0; i < theShape.mFillLayers.size(); i++) {
		const SwfFillLayer& aFill = theShape.mFillLayers[i];
		Layer* aLayer = NewLayer(true);
		if (!SetFillPaint(*aLayer, theShape.mFills[aFill.mStyle], m, theCX, theCacheable)) {
			mNumLayers--;
			continue;
		}
		AddSegs(*aLayer, aFill.mSegs.data(), aFill.mSegs.size(), m);
	}
	for (size_t i = 0; i < theShape.mLineLayers.size(); i++) {
		const SwfLineLayer& aLine = theShape.mLineLayers[i];
		const SwfLineStyle& aStyle = theShape.mLines[aLine.mStyle];
		Layer* aLayer = NewLayer(false);
		if (!SetSolidPaint(*aLayer, aStyle.mColor, theCX)) {
			mNumLayers--;
			continue;
		}
		AddStroke(*aLayer, aLine, aStyle.mWidth, m, theCacheable);
	}
	Rasterize();
}

// The glyphs of a text with the matrix of each: the text's, then the pen's position and the scale from the font's em
// square. theBounds gets the pixels they cover (min x, min y, max x, max y); the result is empty when nothing shows.
void SwfRaster::BuildGlyphs(const SwfText& theText, const SwfData& theMovie, const SwfMatrix& theMatrix, std::vector<GlyphJob>& theGlyphs,
                            float* theBounds)
{
	SwfMatrix aText = SwfMatrix::Concat(theMatrix, theText.mMatrix);
	theGlyphs.clear();
	theBounds[0] = theBounds[1] = 1e30f;
	theBounds[2] = theBounds[3] = -1e30f;
	for (size_t r = 0; r < theText.mRuns.size(); r++) {
		const SwfTextRun& aRun = theText.mRuns[r];
		const SwfCharacter* aFont = theMovie.Find(aRun.mFont);
		if (aFont == nullptr || aFont->mType != SwfCharacter::kFont)
			continue;
		float aScale = aRun.mHeight / 1024.0f;
		for (size_t g = 0; g < aRun.mGlyphs.size(); g++) {
			int anIndex = aRun.mGlyphs[g].mIndex;
			if (anIndex < 0 || anIndex >= (int)aFont->mFont.mGlyphs.size() || aFont->mFont.mGlyphs[anIndex].mEmpty)
				continue;
			const SwfShape& aShape = aFont->mFont.mGlyphs[anIndex];
			GlyphJob aGlyph;
			aGlyph.mShape = &aShape;
			aGlyph.mRun = (int)r;
			SwfMatrix& m = aGlyph.mMatrix;
			m.a = aText.a * aScale;
			m.b = aText.b * aScale;
			m.c = aText.c * aScale;
			m.d = aText.d * aScale;
			m.tx = aText.a * aRun.mGlyphs[g].mX + aText.c * aRun.mGlyphs[g].mY + aText.tx;
			m.ty = aText.b * aRun.mGlyphs[g].mX + aText.d * aRun.mGlyphs[g].mY + aText.ty;
			float xs[4], ys[4];
			m.Apply(aShape.mMinX, aShape.mMinY, xs[0], ys[0]);
			m.Apply(aShape.mMaxX, aShape.mMinY, xs[1], ys[1]);
			m.Apply(aShape.mMinX, aShape.mMaxY, xs[2], ys[2]);
			m.Apply(aShape.mMaxX, aShape.mMaxY, xs[3], ys[3]);
			for (int k = 0; k < 4; k++) {
				theBounds[0] = std::min(theBounds[0], xs[k]);
				theBounds[2] = std::max(theBounds[2], xs[k]);
				theBounds[1] = std::min(theBounds[1], ys[k]);
				theBounds[3] = std::max(theBounds[3], ys[k]);
			}
			theGlyphs.push_back(aGlyph);
		}
	}
}

bool SwfRaster::TextBounds(const SwfText& theText, const SwfData& theMovie, const SwfMatrix& theMatrix, SwfRect& theRect)
{
	std::vector<GlyphJob> aGlyphs;
	float b[4];
	BuildGlyphs(theText, theMovie, theMatrix, aGlyphs, b);
	if (aGlyphs.empty())
		return false;
	const float kLimit = 100000.0f;
	theRect.mX0 = (int)floorf(std::max(b[0] - 1.5f, -kLimit));
	theRect.mY0 = (int)floorf(std::max(b[1] - 1.5f, -kLimit));
	theRect.mX1 = (int)ceilf(std::min(b[2] + 1.5f, kLimit));
	theRect.mY1 = (int)ceilf(std::min(b[3] + 1.5f, kLimit));
	return !theRect.IsEmpty();
}

void SwfRaster::DrawText(const SwfText& theText, const SwfData& theMovie, const SwfMatrix& theMatrix, const SwfCXForm& theCX)
{
	std::vector<GlyphJob>& aGlyphs = mGlyphs;
	float b[4];
	BuildGlyphs(theText, theMovie, theMatrix, aGlyphs, b);
	if (aGlyphs.empty() || !BeginJob(b[0] - 1.5f, b[1] - 1.5f, b[2] + 1.5f, b[3] + 1.5f))
		return;

	// One layer for each run (they differ in color); glyphs seldom overlap, so none of them is a partition
	int aRunOfLayer = -1;
	Layer* aLayer = nullptr;
	bool aVisible = false;
	for (size_t i = 0; i < aGlyphs.size(); i++) {
		if (aGlyphs[i].mRun != aRunOfLayer) {
			aRunOfLayer = aGlyphs[i].mRun;
			aLayer = NewLayer(false);
			aVisible = SetSolidPaint(*aLayer, theText.mRuns[aRunOfLayer].mColor, theCX);
		}
		if (!aVisible)
			continue;
		const SwfShape& aShape = *aGlyphs[i].mShape;
		for (size_t l = 0; l < aShape.mFillLayers.size(); l++)
			AddSegs(*aLayer, aShape.mFillLayers[l].mSegs.data(), aShape.mFillLayers[l].mSegs.size(), aGlyphs[i].mMatrix);
	}
	Rasterize();
}

// ---- the rows ----

void SwfRaster::Rasterize()
{
	int aMinY = mClipY1, aMaxY = mClipY0;
	for (int i = 0; i < mNumLayers; i++) {
		Layer& aLayer = *mLayers[i];
		if (aLayer.mEdges.empty()) {
			aLayer.mRow0 = aLayer.mRow1 = 0;
			continue;
		}
		aLayer.mRow0 = std::max(mClipY0, (int)floorf(aLayer.mMinY));
		aLayer.mRow1 = std::min(mClipY1, (int)ceilf(aLayer.mMaxY));
		aMinY = std::min(aMinY, aLayer.mRow0);
		aMaxY = std::max(aMaxY, aLayer.mRow1);
		SortEdges(aLayer.mEdges);
	}
	for (int y = aMinY; y < aMaxY; y++) {
		for (int i = 0; i < mNumLayers; i++) {
			Layer& aLayer = *mLayers[i];
			if (aLayer.mPartition && y >= aLayer.mRow0 && y < aLayer.mRow1)
				ProcessRow(aLayer, y);
		}
		if (!mAccTouched.empty())
			FlushRow(y);
		for (int i = 0; i < mNumLayers; i++) {
			Layer& aLayer = *mLayers[i];
			if (!aLayer.mPartition && y >= aLayer.mRow0 && y < aLayer.mRow1)
				ProcessRow(aLayer, y);
		}
	}
	mNumLayers = 0;
}

void SwfRaster::AddCell(int theX, float theValue)
{
	mCells[theX] += theValue;
	mTouchBits[theX >> 6] |= (uint64_t)1 << (theX & 63);
}

// Adds a stretch of an edge within one row, from theXA to theXB (relative to the rectangle's left, theArea tall and
// signed by the edge's direction), to the cells. An edge left of the rectangle still has to count, as a vertical
// one at its border; right of it, it changes nothing.
void SwfRaster::AccumSpan(float theXA, float theXB, float theArea)
{
	const float W = (float)(mClipX1 - mClipX0);
	if (theXA >= 0 && theXB >= 0 && theXA <= W && theXB <= W) {
		AccumInside(theXA, theXB, theArea);
		return;
	}
	float aLeft = std::min(theXA, theXB), aRight = std::max(theXA, theXB);
	if (aRight <= 0) {
		AddCell(0, theArea);
		mTouchMin = 0;
		mTouchMax = std::max(mTouchMax, 0);
		return;
	}
	if (aLeft >= W)
		return;
	// It crosses a border: cut it at x = 0 and x = W
	float dx = theXB - theXA;
	float t0 = (0 - theXA) / dx, t1 = (W - theXA) / dx;
	if (t0 > t1)
		std::swap(t0, t1);
	t0 = std::min(1.0f, std::max(0.0f, t0));
	t1 = std::min(1.0f, std::max(0.0f, t1));
	if (t0 > 0 && theXA < 0) {
		AddCell(0, theArea * t0);
		mTouchMin = 0;
		mTouchMax = std::max(mTouchMax, 0);
	}
	if (t1 > t0) {
		float a = std::min(W, std::max(0.0f, theXA + dx * t0));
		float b = std::min(W, std::max(0.0f, theXA + dx * t1));
		AccumInside(a, b, theArea * (t1 - t0));
	}
	if (t1 < 1 && theXB < 0) {
		AddCell(0, theArea * (1 - t1));
		mTouchMin = 0;
		mTouchMax = std::max(mTouchMax, 0);
	}
}

// The cells for a stretch within the rectangle. The value of a cell is the change that its pixel makes to the
// running coverage; a vertical edge puts its height in the cell it passes through, shared between that pixel
// and the next by how far across the pixel it is, and a slanted one spreads its area over the pixels it crosses.
void SwfRaster::AccumInside(float theXA, float theXB, float d)
{
	float x0 = theXA < theXB ? theXA : theXB;
	float x1 = theXA < theXB ? theXB : theXA;
	int x0i = (int)x0;
	int x1i = (int)ceilf(x1);
	int aWord0 = x0i >> 6, aWord1 = (x1i + 1) >> 6;
	if (aWord0 < mTouchMin)
		mTouchMin = aWord0;
	if (aWord1 > mTouchMax)
		mTouchMax = aWord1;
	if (x1i <= x0i + 1) {
		float xmf = 0.5f * (theXA + theXB) - (float)x0i;
		AddCell(x0i, d - d * xmf);
		AddCell(x0i + 1, d * xmf);
		return;
	}
	float s = 1.0f / (x1 - x0);
	float x0f = x0 - (float)x0i;
	float a0 = 0.5f * s * (1 - x0f) * (1 - x0f);
	float x1f = x1 - (float)x1i + 1;
	float am = 0.5f * s * x1f * x1f;
	AddCell(x0i, d * a0);
	if (x1i == x0i + 2)
		AddCell(x0i + 1, d * (1 - a0 - am));
	else {
		float a1 = s * (1.5f - x0f);
		AddCell(x0i + 1, d * (a1 - a0));
		for (int xi = x0i + 2; xi < x1i - 1; xi++)
			AddCell(xi, d * s);
		float a2 = a1 + (float)(x1i - x0i - 3) * s;
		AddCell(x1i - 1, d * (1 - a2 - am));
	}
	AddCell(x1i, d * am);
}

void SwfRaster::AccumEdge(const Edge& e, int theY)
{
	float aTop = e.mY0 > (float)theY ? e.mY0 : (float)theY;
	float aBottom = e.mY1 < (float)(theY + 1) ? e.mY1 : (float)(theY + 1);
	float dy = aBottom - aTop;
	if (dy <= 0)
		return;
	float xa = e.mX0 + (aTop - e.mY0) * e.mDxDy - (float)mClipX0;
	float xb = xa + dy * e.mDxDy;
	AccumSpan(xa, xb, dy * (float)e.mDir);
}

// Gathers one row of a layer and paints its coverage: the cells that its edges touched are the places where the
// coverage changes, and between them it is the same.
void SwfRaster::ProcessRow(Layer& theLayer, int theY)
{
	while (theLayer.mNext < theLayer.mEdges.size() && theLayer.mEdges[theLayer.mNext].mY0 < (float)(theY + 1)) {
		if (theLayer.mEdges[theLayer.mNext].mY1 > (float)theY)
			theLayer.mActive.push_back((int)theLayer.mNext);
		theLayer.mNext++;
	}
	if (theLayer.mActive.empty())
		return;

	mTouchMin = INT_MAX;
	mTouchMax = -1;
	for (size_t i = 0; i < theLayer.mActive.size();) {
		const Edge& e = theLayer.mEdges[theLayer.mActive[i]];
		AccumEdge(e, theY);
		if (e.mY1 <= (float)(theY + 1)) {
			theLayer.mActive[i] = theLayer.mActive.back();
			theLayer.mActive.pop_back();
		}
		else
			i++;
	}
	if (mTouchMax < 0)
		return;

	// The touched cells in order, from the bits that mark them
	const int W = mClipX1 - mClipX0;
	float aSum = 0;
	int aPrevEnd = 0;
	for (int w = mTouchMin; w <= mTouchMax; w++) {
		uint64_t aBits = mTouchBits[w];
		if (aBits == 0)
			continue;
		mTouchBits[w] = 0;
		do {
			int x = (w << 6) + LowestBit(aBits);
			aBits &= aBits - 1;
			if (x >= W) {
				mCells[x] = 0;
				continue;
			}
			if (x > aPrevEnd) {
				float c = fabsf(aSum);
				if (c > kMinCoverage)
					EmitRun(theLayer, theY, aPrevEnd, x, c > 1 ? 1.0f : c);
			}
			aSum += mCells[x];
			mCells[x] = 0;
			float c = fabsf(aSum);
			if (c > kMinCoverage)
				EmitPartial(theLayer, theY, x, c > 1 ? 1.0f : c);
			aPrevEnd = x + 1;
		} while (aBits != 0);
	}
	if (aPrevEnd < W) {
		float c = fabsf(aSum);
		if (c > kMinCoverage)
			EmitRun(theLayer, theY, aPrevEnd, W, c > 1 ? 1.0f : c);
	}
}

// Paints one pixel of a layer at theCoverage (0 to 1, with the clip mask already counted in). A pixel that the
// layer covers all of takes the color; a part of one is held back, if the layer is a part of a partition, until
// every layer of the shape has had its say (FlushRow).
void SwfRaster::PutPixel(Layer& theLayer, uint32_t* theRow, int theX, int theR, int theG, int theB, int theA, float theCoverage)
{
	if (theCoverage >= kFullCoverage) {
		theRow[theX] = theA >= 255 ? (0xFF000000u | (theR << 16) | (theG << 8) | theB)
		                           : Blend(theRow[theX], theR, theG, theB, theA + (theA >> 7));
		return;
	}
	float w = theCoverage * (theA * (1.0f / 255.0f));
	if (theLayer.mPartition) {
		AccPx& a = mAcc[theX];
		if (a.a == 0)
			mAccTouched.push_back(theX);
		a.r += theR * w;
		a.g += theG * w;
		a.b += theB * w;
		a.a += w;
	}
	else
		theRow[theX] = Blend(theRow[theX], theR, theG, theB, (int)(w * 256.0f + 0.5f));
}

// A single pixel, the most common run: the edges of a shape are made of them
void SwfRaster::EmitPartial(Layer& theLayer, int theY, int theX, float theCoverage)
{
	const Paint& p = theLayer.mPaint;
	if (theCoverage < kFullCoverage && p.mKind == 0 && mClip == nullptr && mMaskTarget == nullptr) {
		int x = mClipX0 + theX;
		if (theLayer.mPartition) {
			AccPx& a = mAcc[x];
			if (a.a == 0)
				mAccTouched.push_back(x);
			a.r += p.mPR * theCoverage;
			a.g += p.mPG * theCoverage;
			a.b += p.mPB * theCoverage;
			a.a += p.mPA * theCoverage;
		}
		else {
			uint32_t* aPixel = mBits + (size_t)theY * mStride + x;
			*aPixel = Blend(*aPixel, p.mR, p.mG, p.mB, (int)(theCoverage * p.mPA * 256.0f + 0.5f));
		}
		return;
	}
	EmitRun(theLayer, theY, theX, theX + 1, theCoverage);
}

// The color of a bitmap fill at a pixel center of the target (straight, not multiplied by its alpha): the nearest pixel
// of the bitmap or, smoothed, the four around the point in the proportions that it lies between them; beyond the edges
// the bitmap repeats, or its edge pixels go on
void SwfRaster::SampleBitmap(const Paint& thePaint, float theX, float theY, int& theR, int& theG, int& theB, int& theA)
{
	const SwfBitmap& aBitmap = *thePaint.mBitmap;
	const int W = aBitmap.mWidth, H = aBitmap.mHeight;
	float u = thePaint.mIA * theX + thePaint.mIC * theY + thePaint.mITX;
	float v = thePaint.mIB * theX + thePaint.mID * theY + thePaint.mITY;
	u = std::min(1e7f, std::max(-1e7f, u));
	v = std::min(1e7f, std::max(-1e7f, v));
	float aR, aG, aB, aA;  // multiplied by the alpha
	if (!thePaint.mSmooth) {
		int x = (int)floorf(u), y = (int)floorf(v);
		if (thePaint.mRepeat) {
			x %= W;
			y %= H;
			x += x < 0 ? W : 0;
			y += y < 0 ? H : 0;
		}
		else {
			x = x < 0 ? 0 : (x >= W ? W - 1 : x);
			y = y < 0 ? 0 : (y >= H ? H - 1 : y);
		}
		uint32_t c = aBitmap.mPixels[(size_t)y * W + x];
		aA = (float)(c >> 24);
		aR = (float)((c >> 16) & 255);
		aG = (float)((c >> 8) & 255);
		aB = (float)(c & 255);
	}
	else {
		float fx = u - 0.5f, fy = v - 0.5f;
		float aFloorX = floorf(fx), aFloorY = floorf(fy);
		float tx = fx - aFloorX, ty = fy - aFloorY;
		int x[2] = {(int)aFloorX, (int)aFloorX + 1}, y[2] = {(int)aFloorY, (int)aFloorY + 1};
		for (int i = 0; i < 2; i++) {
			if (thePaint.mRepeat) {
				x[i] %= W;
				y[i] %= H;
				x[i] += x[i] < 0 ? W : 0;
				y[i] += y[i] < 0 ? H : 0;
			}
			else {
				x[i] = x[i] < 0 ? 0 : (x[i] >= W ? W - 1 : x[i]);
				y[i] = y[i] < 0 ? 0 : (y[i] >= H ? H - 1 : y[i]);
			}
		}
		const float aWeights[4] = {(1 - tx) * (1 - ty), tx * (1 - ty), (1 - tx) * ty, tx * ty};
		const uint32_t aTexels[4] = {aBitmap.mPixels[(size_t)y[0] * W + x[0]], aBitmap.mPixels[(size_t)y[0] * W + x[1]],
		                             aBitmap.mPixels[(size_t)y[1] * W + x[0]], aBitmap.mPixels[(size_t)y[1] * W + x[1]]};
		aR = aG = aB = aA = 0;
		for (int i = 0; i < 4; i++) {
			aA += aWeights[i] * (float)(aTexels[i] >> 24);
			aR += aWeights[i] * (float)((aTexels[i] >> 16) & 255);
			aG += aWeights[i] * (float)((aTexels[i] >> 8) & 255);
			aB += aWeights[i] * (float)(aTexels[i] & 255);
		}
	}
	if (aA < 0.5f) {
		theR = theG = theB = theA = 0;
		return;
	}
	if (aA < 254.5f) {
		float anUnmultiply = 255.0f / aA;
		aR = std::min(255.0f, aR * anUnmultiply);
		aG = std::min(255.0f, aG * anUnmultiply);
		aB = std::min(255.0f, aB * anUnmultiply);
	}
	if (thePaint.mHasCX) {
		theR = ClampByte(aR * thePaint.mCXMul[0] + thePaint.mCXAdd[0]);
		theG = ClampByte(aG * thePaint.mCXMul[1] + thePaint.mCXAdd[1]);
		theB = ClampByte(aB * thePaint.mCXMul[2] + thePaint.mCXAdd[2]);
		theA = ClampByte(aA * thePaint.mCXMul[3] + thePaint.mCXAdd[3]);
	}
	else {
		theR = (int)(aR + 0.5f);
		theG = (int)(aG + 0.5f);
		theB = (int)(aB + 0.5f);
		theA = (int)(aA + 0.5f);
	}
}

// A stretch of a gradient or bitmap that is all covered and not clipped
void SwfRaster::EmitGradient(Layer& theLayer, uint32_t* theRow, int theY, int theX0, int theX1)
{
	const Paint& p = theLayer.mPaint;
	float X = (float)theX0 + 0.5f, Y = (float)theY + 0.5f;
	if (p.mKind == 3) {
		for (int x = theX0; x < theX1; x++) {
			int r, g, b, a;
			SampleBitmap(p, (float)x + 0.5f, Y, r, g, b, a);
			if (a >= 255)
				theRow[x] = 0xFF000000u | (r << 16) | (g << 8) | b;
			else if (a != 0)
				theRow[x] = Blend(theRow[x], r, g, b, a + (a >> 7));
		}
	}
	else if (p.mKind == 1) {
		float u = (p.mIA * X + p.mIC * Y + p.mITX + 16384.0f) * (1.0f / 128.0f);
		float du = p.mIA * (1.0f / 128.0f);
		for (int x = theX0; x < theX1; x++, u += du) {
			uint32_t c = p.mLut[u <= 0 ? 0 : (u >= 255 ? 255 : (int)u)];
			int a = c >> 24;
			if (a == 255)
				theRow[x] = c;
			else if (a != 0)
				theRow[x] = Blend(theRow[x], (c >> 16) & 255, (c >> 8) & 255, c & 255, a + (a >> 7));
		}
	}
	else {
		float gx = p.mIA * X + p.mIC * Y + p.mITX;
		float gy = p.mIB * X + p.mID * Y + p.mITY;
		for (int x = theX0; x < theX1; x++, gx += p.mIA, gy += p.mIB) {
			float u = sqrtf(gx * gx + gy * gy) * (1.0f / 64.0f);
			uint32_t c = p.mLut[u >= 255 ? 255 : (int)u];
			int a = c >> 24;
			if (a == 255)
				theRow[x] = c;
			else if (a != 0)
				theRow[x] = Blend(theRow[x], (c >> 16) & 255, (c >> 8) & 255, c & 255, a + (a >> 7));
		}
	}
}

void SwfRaster::EmitRun(Layer& theLayer, int theY, int theStart, int theEnd, float theCoverage)
{
	int x0 = mClipX0 + theStart, x1 = mClipX0 + theEnd;

	if (mMaskTarget != nullptr) {
		uint8_t* aRow = &mMaskTarget->mData[(size_t)theY * mWidth];
		int c = (int)(theCoverage * 255.0f + 0.5f);
		for (int x = x0; x < x1; x++) {
			int v = aRow[x] + c;
			aRow[x] = (uint8_t)(v > 255 ? 255 : v);
		}
		if (mMaskTarget->IsEmpty()) {
			mMaskTarget->mX0 = x0;
			mMaskTarget->mX1 = x1;
			mMaskTarget->mY0 = theY;
			mMaskTarget->mY1 = theY + 1;
		}
		else {
			mMaskTarget->mX0 = std::min(mMaskTarget->mX0, x0);
			mMaskTarget->mX1 = std::max(mMaskTarget->mX1, x1);
			mMaskTarget->mY0 = std::min(mMaskTarget->mY0, theY);
			mMaskTarget->mY1 = std::max(mMaskTarget->mY1, theY + 1);
		}
		return;
	}

	uint32_t* aRow = mBits + (size_t)theY * mStride;
	const Paint& p = theLayer.mPaint;
	const uint8_t* aMask = mClip ? &mClip->mData[(size_t)theY * mWidth] : nullptr;

	if (aMask == nullptr) {
		if (p.mKind == 0) {
			if (theCoverage >= kFullCoverage) {
				if (p.mA >= 255) {
					std::fill(aRow + x0, aRow + x1, p.mArgb);
					return;
				}
				int w = p.mA + (p.mA >> 7);
				for (int x = x0; x < x1; x++)
					aRow[x] = Blend(aRow[x], p.mR, p.mG, p.mB, w);
				return;
			}
			for (int x = x0; x < x1; x++)
				PutPixel(theLayer, aRow, x, p.mR, p.mG, p.mB, p.mA, theCoverage);
			return;
		}
		if (theCoverage >= kFullCoverage) {
			EmitGradient(theLayer, aRow, theY, x0, x1);
			return;
		}
	}

	float Y = (float)theY + 0.5f;
	for (int x = x0; x < x1; x++) {
		float c = theCoverage;
		if (aMask != nullptr) {
			int m = aMask[x];
			if (m == 0)
				continue;
			c *= m * (1.0f / 255.0f);
		}
		int r, g, b, a;
		if (p.mKind == 0) {
			r = p.mR;
			g = p.mG;
			b = p.mB;
			a = p.mA;
		}
		else if (p.mKind == 3) {
			SampleBitmap(p, (float)x + 0.5f, Y, r, g, b, a);
			if (a == 0)
				continue;
		}
		else {
			float X = (float)x + 0.5f;
			int anIndex;
			if (p.mKind == 1) {
				float u = (p.mIA * X + p.mIC * Y + p.mITX + 16384.0f) * (1.0f / 128.0f);
				anIndex = u <= 0 ? 0 : (u >= 255 ? 255 : (int)u);
			}
			else {
				float gx = p.mIA * X + p.mIC * Y + p.mITX;
				float gy = p.mIB * X + p.mID * Y + p.mITY;
				float u = sqrtf(gx * gx + gy * gy) * (1.0f / 64.0f);
				anIndex = u >= 255 ? 255 : (int)u;
			}
			uint32_t v = p.mLut[anIndex];
			a = v >> 24;
			r = (v >> 16) & 255;
			g = (v >> 8) & 255;
			b = v & 255;
			if (a == 0)
				continue;
		}
		PutPixel(theLayer, aRow, x, r, g, b, a, c);
	}
}

// Lays the partial pixels that the partition's layers gathered over the row: each layer took its fraction of the
// pixel, and the rest of the pixel keeps what was there.
void SwfRaster::FlushRow(int theY)
{
	uint32_t* aRow = mBits + (size_t)theY * mStride;
	for (size_t i = 0; i < mAccTouched.size(); i++) {
		int x = mAccTouched[i];
		AccPx a = mAcc[x];
		mAcc[x].r = mAcc[x].g = mAcc[x].b = mAcc[x].a = 0;
		if (a.a > 1) {
			float s = 1.0f / a.a;
			a.r *= s;
			a.g *= s;
			a.b *= s;
			a.a = 1;
		}
		uint32_t d = aRow[x];
		float k = 1 - a.a;
		int nr = (int)(a.r + ((d >> 16) & 255) * k + 0.5f);
		int ng = (int)(a.g + ((d >> 8) & 255) * k + 0.5f);
		int nb = (int)(a.b + (d & 255) * k + 0.5f);
		aRow[x] = 0xFF000000u | ((nr > 255 ? 255 : nr) << 16) | ((ng > 255 ? 255 : ng) << 8) | (nb > 255 ? 255 : nb);
	}
	mAccTouched.clear();
}
