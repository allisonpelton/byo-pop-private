// Shapes: a shape's records are sorted into the edges that bound each fill style and the chains of edges each
// line style draws, and a morph shape's two record lists are blended into one. See SwfTypes.h.
#include <math.h>
#include <algorithm>

#include "SwfTypes.h"

using namespace Sexy;

// ---- matrices and color transforms ----

SwfMatrix SwfMatrix::Concat(const SwfMatrix& theOuter, const SwfMatrix& theInner)
{
	const SwfMatrix& o = theOuter;
	const SwfMatrix& i = theInner;
	SwfMatrix r;
	r.a = o.a * i.a + o.c * i.b;
	r.b = o.b * i.a + o.d * i.b;
	r.c = o.a * i.c + o.c * i.d;
	r.d = o.b * i.c + o.d * i.d;
	r.tx = o.a * i.tx + o.c * i.ty + o.tx;
	r.ty = o.b * i.tx + o.d * i.ty + o.ty;
	return r;
}

bool SwfMatrix::Invert(SwfMatrix& theOut) const
{
	float aDet = a * d - b * c;
	if (fabsf(aDet) < 1e-20f)
		return false;
	float anInv = 1.0f / aDet;
	theOut.a = d * anInv;
	theOut.b = -b * anInv;
	theOut.c = -c * anInv;
	theOut.d = a * anInv;
	theOut.tx = -(theOut.a * tx + theOut.c * ty);
	theOut.ty = -(theOut.b * tx + theOut.d * ty);
	return true;
}

bool SwfCXForm::IsIdentity() const
{
	for (int i = 0; i < 4; i++) {
		if (mMul[i] != 1 || mAdd[i] != 0)
			return false;
	}
	return true;
}

SwfCXForm SwfCXForm::Concat(const SwfCXForm& theOuter, const SwfCXForm& theInner)
{
	SwfCXForm r;
	for (int i = 0; i < 4; i++) {
		r.mMul[i] = theOuter.mMul[i] * theInner.mMul[i];
		r.mAdd[i] = theInner.mAdd[i] * theOuter.mMul[i] + theOuter.mAdd[i];
	}
	return r;
}

// ---- compiling a shape ----

namespace {

// The layer of theShape for a style, made on first use. theLayerOf maps a style to its place in the layers.
template <class L>
L& LayerFor(std::vector<L>& theLayers, std::vector<int>& theLayerOf, int theStyle)
{
	int& aSlot = theLayerOf[theStyle];
	if (aSlot < 0) {
		aSlot = (int)theLayers.size();
		theLayers.push_back(L());
		theLayers.back().mStyle = theStyle;
	}
	return theLayers[aSlot];
}

SwfSeg Reversed(const SwfSeg& theSeg)
{
	SwfSeg r = theSeg;
	r.mX0 = theSeg.mX1;
	r.mY0 = theSeg.mY1;
	r.mX1 = theSeg.mX0;
	r.mY1 = theSeg.mY0;
	return r;
}

struct Bounds {
	float mMinX = 1e30f, mMinY = 1e30f, mMaxX = -1e30f, mMaxY = -1e30f;

	void Add(float theX, float theY, float theMargin)
	{
		mMinX = std::min(mMinX, theX - theMargin);
		mMinY = std::min(mMinY, theY - theMargin);
		mMaxX = std::max(mMaxX, theX + theMargin);
		mMaxY = std::max(mMaxY, theY + theMargin);
	}
	void Add(const SwfSeg& theSeg, float theMargin)
	{
		Add(theSeg.mX0, theSeg.mY0, theMargin);
		Add(theSeg.mX1, theSeg.mY1, theMargin);
		if (theSeg.mCurve)
			Add(theSeg.mCX, theSeg.mCY, theMargin);
	}
};

}  // namespace

void Sexy::SwfCompileShape(const SwfRawShape& theRaw, SwfShape& theShape)
{
	theShape.mFills = theRaw.mFills;
	theShape.mLines = theRaw.mLines;
	theShape.mFillLayers.clear();
	theShape.mLineLayers.clear();

	std::vector<int> aFillLayerOf(theRaw.mFills.size(), -1);
	std::vector<int> aLineLayerOf(theRaw.mLines.size(), -1);
	int aFillBase = 0, aLineBase = 0;
	int aFill0 = 0, aFill1 = 0, aLine = 0;  // the current styles, 1-based in the shape's lists, 0 for none

	for (size_t i = 0; i < theRaw.mRecs.size(); i++) {
		const SwfRec& aRec = theRaw.mRecs[i];
		if (aRec.mType == SwfRec::kStyleChange) {
			if (aRec.mNewStyles) {
				aFillBase = aRec.mFillBase;
				aLineBase = aRec.mLineBase;
				aFill0 = aFill1 = aLine = 0;
			}
			if (aRec.mHasFill0)
				aFill0 = aRec.mFill0 ? aFillBase + aRec.mFill0 : 0;
			if (aRec.mHasFill1)
				aFill1 = aRec.mFill1 ? aFillBase + aRec.mFill1 : 0;
			if (aRec.mHasLine)
				aLine = aRec.mLine ? aLineBase + aRec.mLine : 0;
			continue;
		}

		SwfSeg aSeg;
		aSeg.mX0 = aRec.mX0;
		aSeg.mY0 = aRec.mY0;
		aSeg.mCX = aRec.mCX;
		aSeg.mCY = aRec.mCY;
		aSeg.mX1 = aRec.mX;
		aSeg.mY1 = aRec.mY;
		aSeg.mCurve = aRec.mType == SwfRec::kCurve;
		if (aSeg.mX0 == aSeg.mX1 && aSeg.mY0 == aSeg.mY1 && !aSeg.mCurve)
			continue;

		// An edge with the same fill on both sides bounds nothing
		if (aFill0 != aFill1) {
			if (aFill0 > 0 && aFill0 <= (int)aFillLayerOf.size())
				LayerFor(theShape.mFillLayers, aFillLayerOf, aFill0 - 1).mSegs.push_back(aSeg);
			if (aFill1 > 0 && aFill1 <= (int)aFillLayerOf.size())
				LayerFor(theShape.mFillLayers, aFillLayerOf, aFill1 - 1).mSegs.push_back(Reversed(aSeg));
		}
		if (aLine > 0 && aLine <= (int)aLineLayerOf.size()) {
			SwfLineLayer& aLayer = LayerFor(theShape.mLineLayers, aLineLayerOf, aLine - 1);
			// An edge that starts where the one before it ended continues its chain
			if (!aLayer.mSegs.empty()) {
				const SwfSeg& aLast = aLayer.mSegs.back();
				if (aLast.mX1 != aSeg.mX0 || aLast.mY1 != aSeg.mY0)
					aLayer.mPathEnds.push_back((int)aLayer.mSegs.size());
			}
			aLayer.mSegs.push_back(aSeg);
		}
	}

	std::sort(theShape.mFillLayers.begin(), theShape.mFillLayers.end(),
	          [](const SwfFillLayer& l, const SwfFillLayer& r) { return l.mStyle < r.mStyle; });
	std::sort(theShape.mLineLayers.begin(), theShape.mLineLayers.end(),
	          [](const SwfLineLayer& l, const SwfLineLayer& r) { return l.mStyle < r.mStyle; });

	Bounds aBounds;
	for (size_t i = 0; i < theShape.mFillLayers.size(); i++) {
		const std::vector<SwfSeg>& aSegs = theShape.mFillLayers[i].mSegs;
		for (size_t j = 0; j < aSegs.size(); j++)
			aBounds.Add(aSegs[j], 0);
	}
	for (size_t i = 0; i < theShape.mLineLayers.size(); i++) {
		SwfLineLayer& aLayer = theShape.mLineLayers[i];
		if (!aLayer.mSegs.empty())
			aLayer.mPathEnds.push_back((int)aLayer.mSegs.size());
		float aMargin = theShape.mLines[aLayer.mStyle].mWidth * 0.5f;
		for (size_t j = 0; j < aLayer.mSegs.size(); j++)
			aBounds.Add(aLayer.mSegs[j], aMargin);
	}
	theShape.mEmpty = aBounds.mMinX > aBounds.mMaxX;
	if (!theShape.mEmpty) {
		theShape.mMinX = aBounds.mMinX;
		theShape.mMinY = aBounds.mMinY;
		theShape.mMaxX = aBounds.mMaxX;
		theShape.mMaxY = aBounds.mMaxY;
	}
}

// ---- morphing ----

namespace {

float Lerp(float theFrom, float theTo, float theT)
{
	return theFrom + (theTo - theFrom) * theT;
}

uint8_t LerpByte(uint8_t theFrom, uint8_t theTo, float theT)
{
	float v = theFrom + (theTo - theFrom) * theT + 0.5f;
	return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

SwfColor LerpColor(const SwfColor& theFrom, const SwfColor& theTo, float theT)
{
	SwfColor c;
	c.r = LerpByte(theFrom.r, theTo.r, theT);
	c.g = LerpByte(theFrom.g, theTo.g, theT);
	c.b = LerpByte(theFrom.b, theTo.b, theT);
	c.a = LerpByte(theFrom.a, theTo.a, theT);
	return c;
}

SwfMatrix LerpMatrix(const SwfMatrix& theFrom, const SwfMatrix& theTo, float theT)
{
	SwfMatrix m;
	m.a = Lerp(theFrom.a, theTo.a, theT);
	m.b = Lerp(theFrom.b, theTo.b, theT);
	m.c = Lerp(theFrom.c, theTo.c, theT);
	m.d = Lerp(theFrom.d, theTo.d, theT);
	m.tx = Lerp(theFrom.tx, theTo.tx, theT);
	m.ty = Lerp(theFrom.ty, theTo.ty, theT);
	return m;
}

SwfFillStyle LerpFill(const SwfFillStyle& theFrom, const SwfFillStyle& theTo, float theT)
{
	SwfFillStyle f = theFrom;
	f.mColor = LerpColor(theFrom.mColor, theTo.mColor, theT);
	if (theFrom.mType == theTo.mType && theFrom.mStops.size() == theTo.mStops.size()) {
		f.mMatrix = LerpMatrix(theFrom.mMatrix, theTo.mMatrix, theT);
		for (size_t i = 0; i < f.mStops.size(); i++) {
			f.mStops[i].mRatio = LerpByte(theFrom.mStops[i].mRatio, theTo.mStops[i].mRatio, theT);
			f.mStops[i].mColor = LerpColor(theFrom.mStops[i].mColor, theTo.mStops[i].mColor, theT);
		}
	}
	return f;
}

// A straight edge as the curve that runs along it
void MakeCurve(SwfRec& theRec)
{
	theRec.mType = SwfRec::kCurve;
	theRec.mCX = (theRec.mX0 + theRec.mX) * 0.5f;
	theRec.mCY = (theRec.mY0 + theRec.mY) * 0.5f;
}

}  // namespace

void Sexy::SwfBuildMorph(const SwfMorph& theMorph, int theRatio, SwfRawShape& theOut)
{
	float t = theRatio / 65535.0f;
	const SwfRawShape& aStart = theMorph.mStart;
	const SwfRawShape& anEnd = theMorph.mEnd;

	theOut.mFills.clear();
	for (size_t i = 0; i < aStart.mFills.size(); i++)
		theOut.mFills.push_back(i < anEnd.mFills.size() ? LerpFill(aStart.mFills[i], anEnd.mFills[i], t) : aStart.mFills[i]);
	theOut.mLines.clear();
	for (size_t i = 0; i < aStart.mLines.size(); i++) {
		SwfLineStyle aLine = aStart.mLines[i];
		if (i < anEnd.mLines.size()) {
			aLine.mWidth = Lerp(aStart.mLines[i].mWidth, anEnd.mLines[i].mWidth, t);
			aLine.mColor = LerpColor(aStart.mLines[i].mColor, anEnd.mLines[i].mColor, t);
		}
		theOut.mLines.push_back(aLine);
	}

	theOut.mRecs.clear();
	for (size_t i = 0; i < aStart.mRecs.size(); i++) {
		SwfRec r = aStart.mRecs[i];
		if (i >= anEnd.mRecs.size()) {
			theOut.mRecs.push_back(r);
			continue;
		}
		SwfRec e = anEnd.mRecs[i];
		if (r.mType == SwfRec::kStyleChange) {
			if (e.mType == SwfRec::kStyleChange && e.mHasMove) {
				r.mX = Lerp(r.mX, e.mX, t);
				r.mY = Lerp(r.mY, e.mY, t);
			}
		}
		else if (e.mType != SwfRec::kStyleChange) {
			// A straight edge against a curve becomes a curve
			if (r.mType != e.mType) {
				if (r.mType == SwfRec::kLine)
					MakeCurve(r);
				else
					MakeCurve(e);
			}
			r.mX0 = Lerp(r.mX0, e.mX0, t);
			r.mY0 = Lerp(r.mY0, e.mY0, t);
			r.mX = Lerp(r.mX, e.mX, t);
			r.mY = Lerp(r.mY, e.mY, t);
			if (r.mType == SwfRec::kCurve) {
				r.mCX = Lerp(r.mCX, e.mCX, t);
				r.mCY = Lerp(r.mCY, e.mCY, t);
			}
		}
		theOut.mRecs.push_back(r);
	}
}
