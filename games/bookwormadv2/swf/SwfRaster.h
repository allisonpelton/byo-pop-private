// SwfRaster: draws shapes and texts into a 32-bit pixel buffer, anti-aliased as Flash's high quality is, from the
// edges of SwfShape (SwfTypes.h). Nothing here knows about timelines.
//
// A shape is drawn a row of pixels at a time. For each fill style, the edges that bound it (directed so that the
// fill lies on their left) are cut into the row and the exact area each covers of each pixel is gathered in a row
// of cells (the signed area method of font rasterizers: the running sum of the cells is the coverage). The
// styles of one shape are the parts of a partition, so a pixel where two of them meet takes both of their
// fractions at once instead of laying one over the other; that keeps the seams between neighboring fills from
// showing the background. Line styles are stroked into outlines (round joins and caps) and drawn over the fills.
//
// Speed: the colors of a gradient (256 of them, for each gradient and color transform) and the outline of a stroked
// line (for each scale) are worked out once and kept; the cells are a row's worth, found in order by a bit for each;
// and a rectangle (SetDamage) limits what is drawn, so that SwfMovie can draw only what has changed.
//
// A clip layer (a placement with a clip depth) is a SwfMask, a coverage the size of the target, into which its
// objects are drawn instead of the target; the objects it clips are then drawn with the mask set, which scales
// their coverage.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "SwfTypes.h"

namespace Sexy {

class SwfMask {
public:
	std::vector<uint8_t> mData;  // the target's width times its height
	int mX0 = 0, mY0 = 0, mX1 = 0, mY1 = 0;  // the rectangle that holds every non-zero value

	bool IsEmpty() const { return mX1 <= mX0 || mY1 <= mY0; }
};

// A rectangle of pixels, x0 and y0 inclusive, x1 and y1 exclusive
struct SwfRect {
	int mX0 = 0, mY0 = 0, mX1 = 0, mY1 = 0;

	bool IsEmpty() const { return mX1 <= mX0 || mY1 <= mY0; }
	void Union(const SwfRect& theOther);
	bool Intersects(const SwfRect& theOther) const;
	void Intersect(const SwfRect& theOther);
};

class SwfRaster {
public:
	SwfRaster();

	// The pixels to draw into (0xAARRGGBB, theStride pixels to a row), which stay the target until the next call.
	// Frees the masks of the frame before, and lets everything be drawn.
	void SetTarget(uint32_t* theBits, int theWidth, int theHeight, int theStride);
	// Limits the drawing, of shapes and of fills, to a rectangle of the target
	void SetDamage(const SwfRect& theRect);
	void Fill(uint32_t theColor);

	// A cleared mask of the target's size, which stays valid until the next SetTarget
	SwfMask* AllocMask();
	// The mask that is the product of two (null stands for a mask that covers everything)
	SwfMask* Intersect(const SwfMask* theFirst, const SwfMask* theSecond);
	// While a mask is the target, shapes add their coverage to it instead of drawing
	void SetMaskTarget(SwfMask* theMask) { mMaskTarget = theMask; }
	bool InMaskMode() const { return mMaskTarget != nullptr; }
	// Scales the coverage of the shapes that follow (null for none)
	void SetClip(const SwfMask* theMask) { mClip = theMask; }

	// theCacheable: the shape stays where it is in memory, unchanged, for as long as the raster lives, so that what is
	// worked out for it (the outline of its lines, which SwfLineLayer keeps) may be kept; the colors of a gradient
	// are kept by what they are, whatever shape they come from.
	void DrawShape(const SwfShape& theShape, const SwfMatrix& theMatrix, const SwfCXForm& theCX, bool theCacheable);
	void DrawText(const SwfText& theText, const SwfData& theMovie, const SwfMatrix& theMatrix, const SwfCXForm& theCX);

	// The pixels that drawing would reach, within a pixel or so: false when it has none
	static bool TransformBounds(float theMinX, float theMinY, float theMaxX, float theMaxY, const SwfMatrix& theMatrix, SwfRect& theRect);
	static bool ShapeBounds(const SwfShape& theShape, const SwfMatrix& theMatrix, SwfRect& theRect);
	static bool TextBounds(const SwfText& theText, const SwfData& theMovie, const SwfMatrix& theMatrix, SwfRect& theRect);

private:
	struct Edge {
		float mX0, mY0, mY1, mDxDy;  // mY0 < mY1; the x at mY0 and the change of x for each y
		int mDir;                    // 1 when the edge went down the page, -1 when up
	};
	struct Paint {
		int mKind = 0;  // 0 solid, 1 linear gradient, 2 radial gradient
		int mR = 0, mG = 0, mB = 0, mA = 255;
		uint32_t mArgb = 0;
		float mPR = 0, mPG = 0, mPB = 0, mPA = 0;  // the solid color times its alpha, for the partial pixels
		float mIA = 0, mIB = 0, mIC = 0, mID = 0, mITX = 0, mITY = 0;  // pixel center to gradient square
		const uint32_t* mLut = nullptr;  // 256 colors, 0xAARRGGBB, not premultiplied
		bool mOpaqueLut = false;
	};
	struct Layer {
		std::vector<Edge> mEdges;
		size_t mNext = 0;
		std::vector<int> mActive;
		Paint mPaint;
		uint32_t mLut[256];  // the colors of a gradient that is not cached
		bool mPartition = true;
		float mMinY = 0, mMaxY = 0;
		int mRow0 = 0, mRow1 = 0;  // the rows the layer has edges in
	};
	struct Pt {
		float x, y;
	};
	struct GlyphJob {
		const SwfShape* mShape;
		SwfMatrix mMatrix;
		int mRun;
	};
	struct AccPx {
		float r, g, b, a;
	};
	struct LutEntry {
		uint64_t mHash = 0;  // of the gradient's colors (0 for an entry not in use)
		SwfCXForm mCX;
		bool mOpaque = false;
		unsigned mJob = 0;  // the last job that used it: the entries of the job under way are not given up
		uint32_t mLut[256];
	};

	static void BuildGlyphs(const SwfText& theText, const SwfData& theMovie, const SwfMatrix& theMatrix, std::vector<GlyphJob>& theGlyphs,
	                        float* theBounds);
	bool BeginJob(float theMinX, float theMinY, float theMaxX, float theMaxY);
	Layer* NewLayer(bool thePartition);
	bool SetFillPaint(Layer& theLayer, const SwfFillStyle& theStyle, const SwfMatrix& theMatrix, const SwfCXForm& theCX,
	                  bool theCacheable);
	bool SetSolidPaint(Layer& theLayer, const SwfColor& theColor, const SwfCXForm& theCX);
	void AddSegs(Layer& theLayer, const SwfSeg* theSegs, size_t theCount, const SwfMatrix& theMatrix);
	void AddEdge(Layer& theLayer, float theX0, float theY0, float theX1, float theY1);
	void AddPolygons(Layer& theLayer, const float* thePoints, const int* theEnds, size_t theCount, const SwfMatrix& theMatrix);
	void AddStroke(Layer& theLayer, const SwfLineLayer& theLine, float theWidth, const SwfMatrix& theMatrix, bool theCacheable);
	void StrokeChain(bool theClosed, float theHalf, float theArcStep);
	void AppendPolygon();
	void AddJoin(const Pt& theP, const Pt& thePrev, const Pt& theNext, float theHalf, float theArcStep);
	void Rasterize();

	void ProcessRow(Layer& theLayer, int theY);
	void AccumEdge(const Edge& theEdge, int theY);
	void AccumSpan(float theXA, float theXB, float theArea);
	void AccumInside(float theXA, float theXB, float theArea);
	void AddCell(int theX, float theValue);
	void EmitRun(Layer& theLayer, int theY, int theStart, int theEnd, float theCoverage);
	void EmitPartial(Layer& theLayer, int theY, int theX, float theCoverage);
	void EmitGradient(Layer& theLayer, uint32_t* theRow, int theY, int theX0, int theX1);
	void PutPixel(Layer& theLayer, uint32_t* theRow, int theX, int theR, int theG, int theB, int theA, float theCoverage);
	void FlushRow(int theY);

	uint32_t* mBits = nullptr;
	int mWidth = 0, mHeight = 0, mStride = 0;
	SwfRect mDamage;                                         // all that may be drawn
	int mClipX0 = 0, mClipY0 = 0, mClipX1 = 0, mClipY1 = 0;  // the rectangle of the shape being drawn
	const SwfMask* mClip = nullptr;
	SwfMask* mMaskTarget = nullptr;

	std::vector<std::unique_ptr<Layer> > mLayers;
	int mNumLayers = 0;
	std::vector<float> mCells;  // the area cells of the row being gathered, all zero between rows
	std::vector<uint64_t> mTouchBits;  // a bit for each cell that was added to
	int mTouchMin = 0, mTouchMax = -1;  // the words of mTouchBits that hold set bits
	std::vector<AccPx> mAcc;  // the partial pixels of the row, premultiplied by their weights
	std::vector<int> mAccTouched;
	std::vector<std::unique_ptr<SwfMask> > mMasks;
	int mMasksUsed = 0;
	std::vector<LutEntry> mLutCache;  // a few ways to each of a number of sets
	unsigned mJob = 0;

	// scratch of the stroker and of DrawText
	std::vector<Pt> mChain, mDirs, mLeft, mRight, mPoly;
	std::vector<float> mStrokePoints;  // the polygons made so far, as in SwfLineLayer::mCachePoints
	std::vector<int> mStrokeEnds;
	std::vector<GlyphJob> mGlyphs;
};

}  // namespace Sexy
