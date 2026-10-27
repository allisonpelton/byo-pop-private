// What a .swf file holds, as SwfMovie keeps it in memory: the shapes, morph shapes, fonts, texts and sprites it
// defines (its characters), and the timeline of the movie itself. Nothing here is specific to one game, and the
// parts of the format that Bookworm Adventures' four cutscenes do not use are not read (SwfParser.cpp counts the
// tags and actions it skips).
//
// Units: shapes, matrices and rectangles are in twips (a twentieth of a pixel at the movie's own size), as the file
// has them, until SwfMovie scales the root transform to the size it draws at.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Sexy {

// A 2x3 matrix as the file's MATRIX record has it: x' = a*x + c*y + tx, y' = b*x + d*y + ty
// (ScaleX, RotateSkew0, RotateSkew1, ScaleY, TranslateX, TranslateY)
struct SwfMatrix {
	float a = 1, b = 0, c = 0, d = 1, tx = 0, ty = 0;

	void Apply(float theX, float theY, float& theOutX, float& theOutY) const
	{
		theOutX = a * theX + c * theY + tx;
		theOutY = b * theX + d * theY + ty;
	}
	// theOuter * theInner: the inner matrix applies first
	static SwfMatrix Concat(const SwfMatrix& theOuter, const SwfMatrix& theInner);
	// False when the matrix has no inverse
	bool Invert(SwfMatrix& theOut) const;
};

// A color transform: every channel (red, green, blue, alpha) is multiplied and then added to, in 0 to 255 units
struct SwfCXForm {
	float mMul[4] = {1, 1, 1, 1};
	float mAdd[4] = {0, 0, 0, 0};

	bool IsIdentity() const;
	// theOuter applied after theInner
	static SwfCXForm Concat(const SwfCXForm& theOuter, const SwfCXForm& theInner);
};

struct SwfColor {
	uint8_t r = 0, g = 0, b = 0, a = 255;
};

struct SwfGradStop {
	uint8_t mRatio = 0;  // 0 to 255 along the gradient
	SwfColor mColor;
};

// A bitmap of a movie (DefineBits, DefineBitsJPEG2 and 3, DefineBitsLossless and 2), decoded: its pixels are 0xAARRGGBB
// with the color already multiplied by the alpha
struct SwfBitmap {
	int mWidth = 0, mHeight = 0;
	std::vector<uint32_t> mPixels;
};

// A fill style: solid (0x00), linear gradient (0x10), radial gradient (0x12) or a bitmap (0x40 repeating, 0x41 clipped,
// 0x42 repeating and not smoothed, 0x43 clipped and not smoothed). The gradient matrix maps the gradient square,
// -16384 to 16384 in both directions, onto the shape; a bitmap's matrix maps the bitmap's pixels (twips of the shape's
// space, so a scale of 20 is a pixel for a pixel) and a clipped bitmap shows its edge pixels beyond it.
struct SwfFillStyle {
	int mType = 0;
	SwfColor mColor;
	SwfMatrix mMatrix;
	std::shared_ptr<const SwfBitmap> mBitmap;  // the bitmap of a bitmap fill
	std::vector<SwfGradStop> mStops;
	uint64_t mStopsHash = 0;  // of a gradient's type and stops, which tells the same colors apart wherever they are
};

struct SwfLineStyle {
	float mWidth = 0;  // twips; 0 is a hairline, one pixel wide at any scale
	SwfColor mColor;
};

// A record of a shape as the file has it, with the positions made absolute: a style change (which may move the
// pen and select styles), a straight edge or a quadratic curve. Morph shapes pair two of these lists.
struct SwfRec {
	enum { kStyleChange, kLine, kCurve };
	uint8_t mType = kStyleChange;
	float mX0 = 0, mY0 = 0;  // an edge's start
	float mCX = 0, mCY = 0;  // a curve's control point
	float mX = 0, mY = 0;    // an edge's end, or a style change's move-to
	bool mHasMove = false, mHasFill0 = false, mHasFill1 = false, mHasLine = false, mNewStyles = false;
	int mFill0 = 0, mFill1 = 0, mLine = 0;  // 1-based in the current style group, 0 for none
	int mFillBase = 0, mLineBase = 0;       // a new group's first style in SwfRawShape's lists
};

struct SwfRawShape {
	std::vector<SwfFillStyle> mFills;  // every group's styles, one after the other
	std::vector<SwfLineStyle> mLines;
	std::vector<SwfRec> mRecs;
};

// One edge of a compiled shape: a line from (mX0, mY0) to (mX1, mY1), or a quadratic curve through the control
// point (mCX, mCY)
struct SwfSeg {
	float mX0, mY0, mCX, mCY, mX1, mY1;
	bool mCurve;
};

// The edges that bound one fill style, each directed so that the fill lies on its left
struct SwfFillLayer {
	int mStyle = 0;  // index into SwfShape::mFills
	std::vector<SwfSeg> mSegs;
};

// The edges drawn with one line style: chains of connected edges, mPathEnds holding where each chain ends
struct SwfLineLayer {
	int mStyle = 0;  // index into SwfShape::mLines
	std::vector<SwfSeg> mSegs;
	std::vector<int> mPathEnds;
	// The outline of the lines in the shape's own space, as the drawing last made it, for the scale it was made at:
	// polygons whose points (x then y) are in mCachePoints, mCachePolyEnds saying where each ends. It saves working it
	// out again while the shape is drawn at the same scale.
	mutable float mCacheScale = 0;
	mutable std::vector<float> mCachePoints;
	mutable std::vector<int> mCachePolyEnds;
};

// A shape ready to draw: its fill styles with the edges around each, then its line styles with theirs
struct SwfShape {
	std::vector<SwfFillStyle> mFills;
	std::vector<SwfLineStyle> mLines;
	std::vector<SwfFillLayer> mFillLayers;
	std::vector<SwfLineLayer> mLineLayers;
	float mMinX = 0, mMinY = 0, mMaxX = 0, mMaxY = 0;  // of the fills and, widened by half the line width, the lines
	bool mEmpty = true;
};

// A morph shape: the shape at ratio 0 and at ratio 65535, whose records and styles correspond one to one
struct SwfMorph {
	SwfRawShape mStart;
	SwfRawShape mEnd;  // its mFills and mLines hold the end styles, and its mRecs the end edges
	// A rectangle that holds the shape at every ratio (the hull of the points of both ends, widened for the lines)
	float mMinX = 0, mMinY = 0, mMaxX = 0, mMaxY = 0;
	bool mEmpty = true;
};

struct SwfFont {
	std::vector<SwfShape> mGlyphs;  // in an em square of 1024 units, one fill
};

struct SwfTextGlyph {
	int mIndex;
	float mX, mY;  // the pen, in twips of the text's own space
};

struct SwfTextRun {
	int mFont = 0;
	SwfColor mColor;
	float mHeight = 0;  // twips
	std::vector<SwfTextGlyph> mGlyphs;
};

struct SwfText {
	SwfMatrix mMatrix;
	std::vector<SwfTextRun> mRuns;
};

// A PlaceObject2 or RemoveObject2 of one frame
struct SwfPlaceCmd {
	int mDepth = 0;
	bool mRemove = false;
	bool mMove = false, mHasChar = false, mHasMatrix = false, mHasCX = false, mHasRatio = false, mHasClip = false;
	int mCharId = 0;
	SwfMatrix mMatrix;
	SwfCXForm mCX;
	int mRatio = 0;
	int mClipDepth = 0;
};

// What a DoAction can do here: Play, Stop or GetURL
struct SwfAction {
	enum { kPlay, kStop, kGetURL };
	int mType = kPlay;
	std::string mURL, mTarget;
};

struct SwfFrame {
	std::vector<SwfPlaceCmd> mCmds;
	std::vector<SwfAction> mActions;
	std::string mLabel;
};

// The frames of the movie, or of a sprite
struct SwfTimelineDef {
	std::vector<SwfFrame> mFrames;
};

struct SwfCharacter {
	enum Type { kNone, kShape, kMorph, kSprite, kText, kFont };
	Type mType = kNone;
	SwfShape mShape;
	SwfMorph mMorph;
	SwfTimelineDef mSprite;
	SwfText mText;
	SwfFont mFont;
};

// A parsed movie
struct SwfData {
	int mVersion = 0;
	float mXMin = 0, mYMin = 0, mXMax = 0, mYMax = 0;  // the frame rectangle, twips
	float mFrameRate = 12;
	SwfColor mBackground;
	SwfTimelineDef mRoot;
	std::unordered_map<int, std::unique_ptr<SwfCharacter>> mChars;
	std::unordered_map<int, std::shared_ptr<const SwfBitmap>> mBitmaps;  // by the id of the tag that defined them
	std::vector<uint8_t> mJpegTables;  // the JPEGTables tag, which the DefineBits tags' JPEG data lacks
	std::map<std::string, int> mUnsupported;  // tags, actions and features skipped, each with how often

	const SwfCharacter* Find(int theId) const
	{
		std::unordered_map<int, std::unique_ptr<SwfCharacter>>::const_iterator anItr = mChars.find(theId);
		return anItr == mChars.end() ? nullptr : anItr->second.get();
	}
};

// Builds theShape from the records (SwfShape.cpp)
void SwfCompileShape(const SwfRawShape& theRaw, SwfShape& theShape);
// The raw shape of a morph shape at theRatio (0 to 65535)
void SwfBuildMorph(const SwfMorph& theMorph, int theRatio, SwfRawShape& theOut);
// Parses a whole file, already inflated; false (and theError) when it is not a SWF this player can read
bool SwfParse(const uint8_t* theData, size_t theSize, SwfData& theMovie, std::string& theError);

}  // namespace Sexy
