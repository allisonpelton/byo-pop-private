// Reads a .swf file into SwfData (SwfTypes.h), from the public SWF file format specification (version 10). Only
// what the game's cutscenes use is read: DefineShape, 2 and 3, DefineMorphShape, DefineSprite, DefineText and
// 2, DefineFont2, the bitmaps (DefineBits with JPEGTables, DefineBitsJPEG2 and 3, DefineBitsLossless and 2),
// PlaceObject2, RemoveObject2, ShowFrame, FrameLabel, SetBackgroundColor, DoAction (Play, Stop and GetURL) and Protect. Any other tag, and any other action or feature of the ones read, is skipped and counted in
// SwfData::mUnsupported, so that a test run reports it.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <stb_image.h>

#include "SwfTypes.h"

using namespace Sexy;

namespace {

// Reads the file's bytes and bit fields; reading past the end sets mOk to false and reads zeros
class Reader {
public:
	const uint8_t* mData;
	size_t mSize;
	size_t mPos = 0;
	int mBitsLeft = 0;
	uint8_t mCur = 0;
	bool mOk = true;

	Reader(const uint8_t* theData, size_t theSize) : mData(theData), mSize(theSize) {}

	size_t Remaining() const { return mPos < mSize ? mSize - mPos : 0; }
	bool AtEnd() const { return mPos >= mSize; }

	void Align() { mBitsLeft = 0; }
	uint32_t Bits(int theCount)
	{
		uint32_t v = 0;
		for (int i = 0; i < theCount; i++) {
			if (mBitsLeft == 0) {
				if (mPos >= mSize) {
					mOk = false;
					return 0;
				}
				mCur = mData[mPos++];
				mBitsLeft = 8;
			}
			v = (v << 1) | ((mCur >> --mBitsLeft) & 1);
		}
		return v;
	}
	int32_t SBits(int theCount)
	{
		if (theCount == 0)
			return 0;
		uint32_t v = Bits(theCount);
		if (v & (1u << (theCount - 1)))
			v |= ~0u << theCount;
		return (int32_t)v;
	}
	// A fixed point number of theCount bits with 16 fraction bits
	float FixedBits(int theCount) { return SBits(theCount) / 65536.0f; }

	uint8_t U8()
	{
		Align();
		if (mPos >= mSize) {
			mOk = false;
			return 0;
		}
		return mData[mPos++];
	}
	uint16_t U16()
	{
		uint16_t v = U8();
		return (uint16_t)(v | (U8() << 8));
	}
	int16_t S16() { return (int16_t)U16(); }
	uint32_t U32()
	{
		uint32_t v = U16();
		return v | ((uint32_t)U16() << 16);
	}
	std::string CString()
	{
		Align();
		std::string s;
		while (mPos < mSize && mData[mPos] != 0)
			s += (char)mData[mPos++];
		if (mPos < mSize)
			mPos++;
		else
			mOk = false;
		return s;
	}
	void Skip(size_t theCount)
	{
		Align();
		if (theCount > Remaining()) {
			mPos = mSize;
			mOk = false;
		}
		else
			mPos += theCount;
	}
};

// RECT, as twips: xmin, xmax, ymin, ymax
void ReadRect(Reader& r, float* theOut)
{
	r.Align();
	int aBits = (int)r.Bits(5);
	float aMinX = (float)r.SBits(aBits);
	float aMaxX = (float)r.SBits(aBits);
	float aMinY = (float)r.SBits(aBits);
	float aMaxY = (float)r.SBits(aBits);
	r.Align();
	theOut[0] = aMinX;
	theOut[1] = aMaxX;
	theOut[2] = aMinY;
	theOut[3] = aMaxY;
}

SwfMatrix ReadMatrix(Reader& r)
{
	SwfMatrix m;
	r.Align();
	if (r.Bits(1)) {
		int aBits = (int)r.Bits(5);
		m.a = r.FixedBits(aBits);
		m.d = r.FixedBits(aBits);
	}
	if (r.Bits(1)) {
		int aBits = (int)r.Bits(5);
		m.b = r.FixedBits(aBits);
		m.c = r.FixedBits(aBits);
	}
	int aBits = (int)r.Bits(5);
	m.tx = (float)r.SBits(aBits);
	m.ty = (float)r.SBits(aBits);
	r.Align();
	return m;
}

// CXFORMWITHALPHA (the file's multiply terms are 8.8 fixed point, the add terms whole)
SwfCXForm ReadCXForm(Reader& r)
{
	SwfCXForm cx;
	r.Align();
	bool aHasAdd = r.Bits(1) != 0;
	bool aHasMul = r.Bits(1) != 0;
	int aBits = (int)r.Bits(4);
	if (aHasMul) {
		for (int i = 0; i < 4; i++)
			cx.mMul[i] = r.SBits(aBits) / 256.0f;
	}
	if (aHasAdd) {
		for (int i = 0; i < 4; i++)
			cx.mAdd[i] = (float)r.SBits(aBits);
	}
	r.Align();
	return cx;
}

SwfColor ReadColor(Reader& r, bool theAlpha)
{
	SwfColor c;
	c.r = r.U8();
	c.g = r.U8();
	c.b = r.U8();
	c.a = theAlpha ? r.U8() : 255;
	return c;
}

// ---- bitmaps ----

// A bitmap from thePixels, 0xAARRGGBB, whose colors are multiplied by their alpha here unless theMultiplied
std::shared_ptr<const SwfBitmap> MakeBitmap(int theWidth, int theHeight, std::vector<uint32_t>& thePixels, bool theMultiplied)
{
	if (theWidth <= 0 || theHeight <= 0 || thePixels.size() != (size_t)theWidth * (size_t)theHeight)
		return nullptr;
	if (!theMultiplied) {
		for (size_t i = 0; i < thePixels.size(); i++) {
			uint32_t c = thePixels[i];
			uint32_t a = c >> 24;
			if (a == 255)
				continue;
			uint32_t r = (((c >> 16) & 255) * a + 127) / 255, g = (((c >> 8) & 255) * a + 127) / 255, b = ((c & 255) * a + 127) / 255;
			thePixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}
	std::shared_ptr<SwfBitmap> aBitmap = std::make_shared<SwfBitmap>();
	aBitmap->mWidth = theWidth;
	aBitmap->mHeight = theHeight;
	aBitmap->mPixels.swap(thePixels);
	return aBitmap;
}

// A bitmap from an image file's bytes (JPEG, or the PNG and GIF that later versions allow), with theAlpha (one byte
// for each pixel, or null) as its alpha
std::shared_ptr<const SwfBitmap> DecodeImage(const uint8_t* theData, size_t theSize, const uint8_t* theAlpha, size_t theAlphaSize)
{
	int w = 0, h = 0, aComps = 0;
	uint8_t* aRGBA = stbi_load_from_memory(theData, (int)theSize, &w, &h, &aComps, 4);
	if (aRGBA == nullptr)
		return nullptr;
	std::vector<uint32_t> aPixels((size_t)w * (size_t)h);
	bool anAlphaOfItsOwn = aComps == 4 || theAlpha == nullptr;
	for (size_t i = 0; i < aPixels.size(); i++) {
		uint32_t a = anAlphaOfItsOwn ? aRGBA[i * 4 + 3] : (i < theAlphaSize ? theAlpha[i] : 255);
		aPixels[i] = (a << 24) | ((uint32_t)aRGBA[i * 4] << 16) | ((uint32_t)aRGBA[i * 4 + 1] << 8) | aRGBA[i * 4 + 2];
	}
	stbi_image_free(aRGBA);
	return MakeBitmap(w, h, aPixels, false);
}

// The JPEG data of a bitmap tag: an erroneous leading end and start of image marker pair that old files have is dropped
void SkipJpegPrefix(const uint8_t*& theData, size_t& theSize)
{
	if (theSize >= 4 && theData[0] == 0xff && theData[1] == 0xd9 && theData[2] == 0xff && theData[3] == 0xd8) {
		theData += 4;
		theSize -= 4;
	}
}

// DefineBitsLossless (theVersion 1, RGB) and DefineBitsLossless2 (2, with alpha): a zlib stream of a color table and
// indices (format 3), of 15-bit (4) or 32-bit (5) pixels, each row padded to a whole number of 32-bit words
std::shared_ptr<const SwfBitmap> DecodeLossless(Reader& r, int theVersion)
{
	int aFormat = r.U8();
	int w = r.U16();
	int h = r.U16();
	int aColors = aFormat == 3 ? r.U8() + 1 : 0;
	if (!r.mOk || w <= 0 || h <= 0 || r.Remaining() == 0)
		return nullptr;
	int aLength = 0;
	uint8_t* aRaw = (uint8_t*)stbi_zlib_decode_malloc((const char*)r.mData + r.mPos, (int)r.Remaining(), &aLength);
	if (aRaw == nullptr)
		return nullptr;
	std::vector<uint32_t> aPixels((size_t)w * (size_t)h, 0);
	size_t aSize = (size_t)aLength;
	bool anOk = true;
	if (aFormat == 3) {
		size_t anEntry = theVersion == 2 ? 4 : 3;
		size_t aTable = (size_t)aColors * anEntry;
		size_t aStride = ((size_t)w + 3) & ~(size_t)3;
		anOk = aSize >= aTable + aStride * (size_t)h;
		for (int y = 0; anOk && y < h; y++)
			for (int x = 0; x < w; x++) {
				int anIndex = aRaw[aTable + aStride * y + x];
				const uint8_t* c = aRaw + (anIndex < aColors ? anIndex : 0) * anEntry;
				aPixels[(size_t)y * w + x] = ((theVersion == 2 ? (uint32_t)c[3] : 255u) << 24) | ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | c[2];
			}
	}
	else if (aFormat == 4) {
		size_t aStride = ((size_t)w * 2 + 3) & ~(size_t)3;
		anOk = aSize >= aStride * (size_t)h;
		for (int y = 0; anOk && y < h; y++)
			for (int x = 0; x < w; x++) {
				unsigned v = ((unsigned)aRaw[aStride * y + x * 2] << 8) | aRaw[aStride * y + x * 2 + 1];
				unsigned r5 = (v >> 10) & 31, g5 = (v >> 5) & 31, b5 = v & 31;
				aPixels[(size_t)y * w + x] = 0xff000000u | (((r5 << 3) | (r5 >> 2)) << 16) | (((g5 << 3) | (g5 >> 2)) << 8) | ((b5 << 3) | (b5 >> 2));
			}
	}
	else if (aFormat == 5) {
		anOk = aSize >= (size_t)w * (size_t)h * 4;
		for (size_t i = 0; anOk && i < aPixels.size(); i++) {
			const uint8_t* c = aRaw + i * 4;
			// the first byte is the alpha of version 2 (the colors are then already multiplied by it), padding in version 1
			aPixels[i] = ((theVersion == 2 ? (uint32_t)c[0] : 255u) << 24) | ((uint32_t)c[1] << 16) | ((uint32_t)c[2] << 8) | c[3];
		}
	}
	else
		anOk = false;
	free(aRaw);
	if (!anOk)
		return nullptr;
	// the colors of a table are not multiplied by their alpha, the pixels of format 5 are
	return MakeBitmap(w, h, aPixels, aFormat == 5 && theVersion == 2);
}

// The bitmap tags: 6 DefineBits, 8 JPEGTables, 20 DefineBitsLossless, 21 DefineBitsJPEG2, 35 DefineBitsJPEG3,
// 36 DefineBitsLossless2
void ReadBitmapTag(Reader& r, SwfData& theMovie, int theTag)
{
	if (theTag == 8) {
		theMovie.mJpegTables.assign(r.mData + r.mPos, r.mData + r.mSize);
		return;
	}
	int anId = r.U16();
	std::shared_ptr<const SwfBitmap> aBitmap;
	const uint8_t* aData = r.mData + r.mPos;
	size_t aSize = r.Remaining();
	if (theTag == 20 || theTag == 36)
		aBitmap = DecodeLossless(r, theTag == 36 ? 2 : 1);
	else if (theTag == 35) {
		size_t anAlphaOffset = r.U32();
		aData = r.mData + r.mPos;
		aSize = r.Remaining();
		if (anAlphaOffset > aSize)
			anAlphaOffset = aSize;
		std::vector<uint8_t> anAlpha;
		if (anAlphaOffset < aSize) {
			int aLength = 0;
			char* anInflated = stbi_zlib_decode_malloc((const char*)aData + anAlphaOffset, (int)(aSize - anAlphaOffset), &aLength);
			if (anInflated != nullptr) {
				anAlpha.assign((const uint8_t*)anInflated, (const uint8_t*)anInflated + aLength);
				free(anInflated);
			}
		}
		size_t aJpegSize = anAlphaOffset;
		SkipJpegPrefix(aData, aJpegSize);
		aBitmap = DecodeImage(aData, aJpegSize, anAlpha.empty() ? nullptr : anAlpha.data(), anAlpha.size());
	}
	else if (theTag == 21) {
		SkipJpegPrefix(aData, aSize);
		aBitmap = DecodeImage(aData, aSize, nullptr, 0);
	}
	else {
		// DefineBits: the JPEG tables (without their end marker) and then the data (without its start marker)
		std::vector<uint8_t> aJoined;
		if (theMovie.mJpegTables.size() >= 4 && aSize >= 2 && aData[0] == 0xff && aData[1] == 0xd8) {
			size_t aTables = theMovie.mJpegTables.size();
			if (theMovie.mJpegTables[aTables - 2] == 0xff && theMovie.mJpegTables[aTables - 1] == 0xd9)
				aTables -= 2;
			aJoined.assign(theMovie.mJpegTables.begin(), theMovie.mJpegTables.begin() + aTables);
			aJoined.insert(aJoined.end(), aData + 2, aData + aSize);
			aData = aJoined.data();
			aSize = aJoined.size();
		}
		aBitmap = DecodeImage(aData, aSize, nullptr, 0);
	}
	if (aBitmap == nullptr)
		theMovie.mUnsupported["a bitmap that did not decode"]++;
	else
		theMovie.mBitmaps[anId] = aBitmap;
}

// A hash of what a gradient's colors are (its type and stops)
uint64_t HashStops(const SwfFillStyle& theStyle)
{
	uint64_t h = 1469598103934665603ULL ^ (uint64_t)theStyle.mType;
	for (size_t i = 0; i < theStyle.mStops.size(); i++) {
		const SwfGradStop& s = theStyle.mStops[i];
		uint32_t v = ((uint32_t)s.mRatio << 24) ^ ((uint32_t)s.mColor.r << 16) ^ ((uint32_t)s.mColor.g << 8) ^ s.mColor.b ^ ((uint32_t)s.mColor.a * 2654435761u);
		h = (h ^ v) * 0x100000001b3ULL;
		h ^= h >> 29;
	}
	return h;
}

// theShapeVersion: 1 for DefineShape, 2 for DefineShape2, 3 for DefineShape3 (and the morph shape and text, which have RGBA)
void ReadFillStyles(Reader& r, int theShapeVersion, bool theAlpha, std::vector<SwfFillStyle>& theOut, SwfData& theMovie)
{
	int aCount = r.U8();
	if (aCount == 0xff && theShapeVersion >= 2)
		aCount = r.U16();
	for (int i = 0; i < aCount && r.mOk; i++) {
		SwfFillStyle f;
		f.mType = r.U8();
		if (f.mType == 0x00)
			f.mColor = ReadColor(r, theAlpha);
		else if (f.mType == 0x10 || f.mType == 0x12 || f.mType == 0x13) {
			if (f.mType == 0x13)
				theMovie.mUnsupported["focal gradient fill (drawn as radial)"]++;
			f.mMatrix = ReadMatrix(r);
			if (f.mType == 0x13)
				f.mType = 0x12;
			int aStops = r.U8() & 0x0f;
			for (int j = 0; j < aStops; j++) {
				SwfGradStop aStop;
				aStop.mRatio = r.U8();
				aStop.mColor = ReadColor(r, theAlpha);
				f.mStops.push_back(aStop);
			}
			f.mStopsHash = HashStops(f);
		}
		else if (f.mType >= 0x40 && f.mType <= 0x43) {
			int aBitmapId = r.U16();
			f.mMatrix = ReadMatrix(r);
			std::unordered_map<int, std::shared_ptr<const SwfBitmap> >::const_iterator anItr = theMovie.mBitmaps.find(aBitmapId);
			if (anItr != theMovie.mBitmaps.end())
				f.mBitmap = anItr->second;
			else {
				// 65535 is the id of no bitmap, which shows nothing; any other id that has no bitmap (it did not decode) is a gray fill
				f.mType = 0;
				f.mColor.r = f.mColor.g = f.mColor.b = 128;
				if (aBitmapId == 0xffff)
					f.mColor.a = 0;
				else
					theMovie.mUnsupported["bitmap fill without its bitmap " + std::to_string(aBitmapId)]++;
			}
		}
		else {
			theMovie.mUnsupported["fill style of an unknown type"]++;
			f.mType = 0;
			f.mColor.r = f.mColor.g = f.mColor.b = 128;
		}
		theOut.push_back(f);
	}
}

void ReadLineStyles(Reader& r, int theShapeVersion, bool theAlpha, std::vector<SwfLineStyle>& theOut)
{
	int aCount = r.U8();
	if (aCount == 0xff && theShapeVersion >= 2)
		aCount = r.U16();
	for (int i = 0; i < aCount && r.mOk; i++) {
		SwfLineStyle l;
		l.mWidth = (float)r.U16();
		l.mColor = ReadColor(r, theAlpha);
		theOut.push_back(l);
	}
}

// The records of a SHAPE or SHAPEWITHSTYLE, after the style arrays (when it has any) and the two bit counts. A
// style change may bring new style arrays, which go on the end of theRaw's lists.
void ReadShapeRecords(Reader& r, SwfRawShape& theRaw, int theShapeVersion, bool theAlpha, int theFillBits, int theLineBits,
                      SwfData& theMovie)
{
	float aPenX = 0, aPenY = 0;
	while (r.mOk) {
		if (r.Bits(1) == 0) {
			uint32_t aFlags = r.Bits(5);
			if (aFlags == 0)
				break;
			SwfRec aRec;
			aRec.mType = SwfRec::kStyleChange;
			if (aFlags & 1) {
				int aBits = (int)r.Bits(5);
				aPenX = (float)r.SBits(aBits);
				aPenY = (float)r.SBits(aBits);
				aRec.mHasMove = true;
				aRec.mX = aPenX;
				aRec.mY = aPenY;
			}
			if (aFlags & 2) {
				aRec.mHasFill0 = true;
				aRec.mFill0 = (int)r.Bits(theFillBits);
			}
			if (aFlags & 4) {
				aRec.mHasFill1 = true;
				aRec.mFill1 = (int)r.Bits(theFillBits);
			}
			if (aFlags & 8) {
				aRec.mHasLine = true;
				aRec.mLine = (int)r.Bits(theLineBits);
			}
			if (aFlags & 16) {
				r.Align();
				aRec.mNewStyles = true;
				aRec.mFillBase = (int)theRaw.mFills.size();
				aRec.mLineBase = (int)theRaw.mLines.size();
				ReadFillStyles(r, theShapeVersion, theAlpha, theRaw.mFills, theMovie);
				ReadLineStyles(r, theShapeVersion, theAlpha, theRaw.mLines);
				theFillBits = (int)r.Bits(4);
				theLineBits = (int)r.Bits(4);
			}
			theRaw.mRecs.push_back(aRec);
		}
		else {
			SwfRec aRec;
			aRec.mX0 = aPenX;
			aRec.mY0 = aPenY;
			bool aStraight = r.Bits(1) != 0;
			int aBits = (int)r.Bits(4) + 2;
			if (aStraight) {
				float aDX = 0, aDY = 0;
				if (r.Bits(1)) {
					aDX = (float)r.SBits(aBits);
					aDY = (float)r.SBits(aBits);
				}
				else if (r.Bits(1))
					aDY = (float)r.SBits(aBits);
				else
					aDX = (float)r.SBits(aBits);
				aRec.mType = SwfRec::kLine;
				aPenX += aDX;
				aPenY += aDY;
			}
			else {
				float aCDX = (float)r.SBits(aBits);
				float aCDY = (float)r.SBits(aBits);
				float anADX = (float)r.SBits(aBits);
				float anADY = (float)r.SBits(aBits);
				aRec.mType = SwfRec::kCurve;
				aRec.mCX = aPenX + aCDX;
				aRec.mCY = aPenY + aCDY;
				aPenX = aRec.mCX + anADX;
				aPenY = aRec.mCY + anADY;
			}
			aRec.mX = aPenX;
			aRec.mY = aPenY;
			theRaw.mRecs.push_back(aRec);
		}
	}
	r.Align();
}

// ---- characters ----

SwfCharacter& NewChar(SwfData& theMovie, int theId, SwfCharacter::Type theType)
{
	std::unique_ptr<SwfCharacter>& aSlot = theMovie.mChars[theId];
	aSlot.reset(new SwfCharacter());
	aSlot->mType = theType;
	return *aSlot;
}

void ReadShapeTag(Reader& r, SwfData& theMovie, int theShapeVersion)
{
	int anId = r.U16();
	float aBounds[4];
	ReadRect(r, aBounds);
	SwfRawShape aRaw;
	bool anAlpha = theShapeVersion >= 3;
	ReadFillStyles(r, theShapeVersion, anAlpha, aRaw.mFills, theMovie);
	ReadLineStyles(r, theShapeVersion, anAlpha, aRaw.mLines);
	int aFillBits = (int)r.Bits(4);
	int aLineBits = (int)r.Bits(4);
	ReadShapeRecords(r, aRaw, theShapeVersion, anAlpha, aFillBits, aLineBits, theMovie);
	SwfCharacter& aChar = NewChar(theMovie, anId, SwfCharacter::kShape);
	SwfCompileShape(aRaw, aChar.mShape);
}

void ReadMorphFillStyles(Reader& r, SwfRawShape& theStart, SwfRawShape& theEnd, SwfData& theMovie)
{
	int aCount = r.U8();
	if (aCount == 0xff)
		aCount = r.U16();
	for (int i = 0; i < aCount && r.mOk; i++) {
		SwfFillStyle s, e;
		s.mType = e.mType = r.U8();
		if (s.mType == 0x00) {
			s.mColor = ReadColor(r, true);
			e.mColor = ReadColor(r, true);
		}
		else if (s.mType == 0x10 || s.mType == 0x12 || s.mType == 0x13) {
			if (s.mType == 0x13) {
				theMovie.mUnsupported["focal gradient fill (drawn as radial)"]++;
				s.mType = e.mType = 0x12;
			}
			s.mMatrix = ReadMatrix(r);
			e.mMatrix = ReadMatrix(r);
			int aStops = r.U8() & 0x0f;
			for (int j = 0; j < aStops; j++) {
				SwfGradStop aStart, anEnd;
				aStart.mRatio = r.U8();
				aStart.mColor = ReadColor(r, true);
				anEnd.mRatio = r.U8();
				anEnd.mColor = ReadColor(r, true);
				s.mStops.push_back(aStart);
				e.mStops.push_back(anEnd);
			}
		}
		else if (s.mType >= 0x40 && s.mType <= 0x43) {
			int aBitmapId = r.U16();
			s.mMatrix = ReadMatrix(r);
			e.mMatrix = ReadMatrix(r);
			std::unordered_map<int, std::shared_ptr<const SwfBitmap> >::const_iterator anItr = theMovie.mBitmaps.find(aBitmapId);
			if (anItr != theMovie.mBitmaps.end())
				s.mBitmap = e.mBitmap = anItr->second;
			else {
				s.mType = e.mType = 0;
				s.mColor.r = s.mColor.g = s.mColor.b = 128;
				if (aBitmapId == 0xffff)
					s.mColor.a = 0;
				else
					theMovie.mUnsupported["bitmap fill without its bitmap " + std::to_string(aBitmapId)]++;
				e.mColor = s.mColor;
			}
		}
		else {
			theMovie.mUnsupported["fill style of an unknown type"]++;
			s.mType = e.mType = 0;
			s.mColor.r = s.mColor.g = s.mColor.b = 128;
			e.mColor = s.mColor;
		}
		theStart.mFills.push_back(s);
		theEnd.mFills.push_back(e);
	}
}

void ReadMorphShapeTag(Reader& r, SwfData& theMovie)
{
	int anId = r.U16();
	float aBounds[4];
	ReadRect(r, aBounds);
	ReadRect(r, aBounds);
	uint32_t anOffset = r.U32();
	(void)anOffset;
	SwfCharacter& aChar = NewChar(theMovie, anId, SwfCharacter::kMorph);
	SwfMorph& aMorph = aChar.mMorph;
	ReadMorphFillStyles(r, aMorph.mStart, aMorph.mEnd, theMovie);
	int aLineCount = r.U8();
	if (aLineCount == 0xff)
		aLineCount = r.U16();
	for (int i = 0; i < aLineCount && r.mOk; i++) {
		SwfLineStyle s, e;
		s.mWidth = (float)r.U16();
		e.mWidth = (float)r.U16();
		s.mColor = ReadColor(r, true);
		e.mColor = ReadColor(r, true);
		aMorph.mStart.mLines.push_back(s);
		aMorph.mEnd.mLines.push_back(e);
	}
	r.Align();
	int aFillBits = (int)r.Bits(4);
	int aLineBits = (int)r.Bits(4);
	ReadShapeRecords(r, aMorph.mStart, 2, true, aFillBits, aLineBits, theMovie);
	aFillBits = (int)r.Bits(4);
	aLineBits = (int)r.Bits(4);
	ReadShapeRecords(r, aMorph.mEnd, 2, true, aFillBits, aLineBits, theMovie);

	// Where the shape can be at any ratio: a point between its two ends is inside the box of the points of both
	float aMargin = 0;
	for (size_t i = 0; i < aMorph.mStart.mLines.size(); i++)
		aMargin = std::max(aMargin, std::max(aMorph.mStart.mLines[i].mWidth, aMorph.mEnd.mLines.size() > i ? aMorph.mEnd.mLines[i].mWidth : 0.0f) * 0.5f);
	const SwfRawShape* aShapes[2] = {&aMorph.mStart, &aMorph.mEnd};
	for (int s = 0; s < 2; s++) {
		for (size_t i = 0; i < aShapes[s]->mRecs.size(); i++) {
			const SwfRec& e = *(&aShapes[s]->mRecs[i]);
			float aPoints[6] = {e.mX, e.mY, e.mX0, e.mY0, e.mCX, e.mCY};
			int aCount = e.mType == SwfRec::kStyleChange ? (e.mHasMove ? 1 : 0) : (e.mType == SwfRec::kLine ? 2 : 3);
			for (int k = 0; k < aCount; k++) {
				float x = aPoints[k * 2], y = aPoints[k * 2 + 1];
				if (aMorph.mEmpty) {
					aMorph.mMinX = aMorph.mMaxX = x;
					aMorph.mMinY = aMorph.mMaxY = y;
					aMorph.mEmpty = false;
				}
				aMorph.mMinX = std::min(aMorph.mMinX, x);
				aMorph.mMaxX = std::max(aMorph.mMaxX, x);
				aMorph.mMinY = std::min(aMorph.mMinY, y);
				aMorph.mMaxY = std::max(aMorph.mMaxY, y);
			}
		}
	}
	if (!aMorph.mEmpty) {
		aMorph.mMinX -= aMargin;
		aMorph.mMinY -= aMargin;
		aMorph.mMaxX += aMargin;
		aMorph.mMaxY += aMargin;
	}
}

void ReadFont2Tag(Reader& r, SwfData& theMovie)
{
	int anId = r.U16();
	uint8_t aFlags = r.U8();
	r.U8();  // language code
	int aNameLen = r.U8();
	r.Skip(aNameLen);
	int aGlyphs = r.U16();
	bool aWideOffsets = (aFlags & 0x08) != 0;
	size_t aTableStart = r.mPos;
	std::vector<uint32_t> anOffsets;
	for (int i = 0; i < aGlyphs && r.mOk; i++)
		anOffsets.push_back(aWideOffsets ? r.U32() : r.U16());
	if (aWideOffsets)
		r.U32();  // code table offset
	else
		r.U16();

	SwfCharacter& aChar = NewChar(theMovie, anId, SwfCharacter::kFont);
	aChar.mFont.mGlyphs.resize(anOffsets.size());
	for (size_t i = 0; i < anOffsets.size() && r.mOk; i++) {
		size_t aStart = aTableStart + anOffsets[i];
		if (aStart >= r.mSize)
			continue;
		Reader g(r.mData + aStart, r.mSize - aStart);
		int aFillBits = (int)g.Bits(4);
		int aLineBits = (int)g.Bits(4);
		SwfRawShape aRaw;
		SwfFillStyle aFill;
		aFill.mColor.r = aFill.mColor.g = aFill.mColor.b = 255;
		aRaw.mFills.push_back(aFill);
		ReadShapeRecords(g, aRaw, 1, false, aFillBits, aLineBits, theMovie);
		SwfCompileShape(aRaw, aChar.mFont.mGlyphs[i]);
	}
	// The character codes and the layout (advances, bounds, kerning) are not needed: a text names its glyphs by
	// index and gives their advances itself
}

void ReadTextTag(Reader& r, SwfData& theMovie, bool theAlpha)
{
	int anId = r.U16();
	float aBounds[4];
	ReadRect(r, aBounds);
	SwfCharacter& aChar = NewChar(theMovie, anId, SwfCharacter::kText);
	SwfText& aText = aChar.mText;
	aText.mMatrix = ReadMatrix(r);
	int aGlyphBits = r.U8();
	int anAdvanceBits = r.U8();

	int aFont = 0;
	SwfColor aColor;
	float aHeight = 0, aPenX = 0, aPenY = 0;
	while (r.mOk) {
		uint8_t aHeader = r.U8();
		if (aHeader == 0)
			break;
		if (aHeader & 0x08)
			aFont = r.U16();
		if (aHeader & 0x04)
			aColor = ReadColor(r, theAlpha);
		if (aHeader & 0x01)
			aPenX = (float)r.S16();
		if (aHeader & 0x02)
			aPenY = (float)r.S16();
		if (aHeader & 0x08)
			aHeight = (float)r.U16();
		int aCount = r.U8();
		SwfTextRun aRun;
		aRun.mFont = aFont;
		aRun.mColor = aColor;
		aRun.mHeight = aHeight;
		for (int i = 0; i < aCount && r.mOk; i++) {
			SwfTextGlyph aGlyph;
			aGlyph.mIndex = (int)r.Bits(aGlyphBits);
			float anAdvance = (float)r.SBits(anAdvanceBits);
			aGlyph.mX = aPenX;
			aGlyph.mY = aPenY;
			aRun.mGlyphs.push_back(aGlyph);
			aPenX += anAdvance;
		}
		aText.mRuns.push_back(aRun);
	}
}

// ---- timelines ----

const char* TagName(int theTag)
{
	switch (theTag) {
	case 4: return "PlaceObject";
	case 5: return "RemoveObject";
	case 6: return "DefineBits";
	case 7: return "DefineButton";
	case 8: return "JPEGTables";
	case 10: return "DefineFont";
	case 13: return "DefineFontInfo";
	case 14: return "DefineSound";
	case 15: return "StartSound";
	case 17: return "DefineButtonSound";
	case 18: return "SoundStreamHead";
	case 19: return "SoundStreamBlock";
	case 20: return "DefineBitsLossless";
	case 21: return "DefineBitsJPEG2";
	case 34: return "DefineButton2";
	case 35: return "DefineBitsJPEG3";
	case 36: return "DefineBitsLossless2";
	case 37: return "DefineEditText";
	case 45: return "SoundStreamHead2";
	case 56: return "ExportAssets";
	case 57: return "ImportAssets";
	case 59: return "DoInitAction";
	case 60: return "DefineVideoStream";
	case 61: return "VideoFrame";
	case 62: return "DefineFontInfo2";
	case 69: return "FileAttributes";
	case 70: return "PlaceObject3";
	case 75: return "DefineFont3";
	case 83: return "DefineShape4";
	case 84: return "DefineMorphShape2";
	}
	return "";
}

void ReadPlaceObject2(Reader& r, SwfData& theMovie, SwfFrame& theFrame)
{
	SwfPlaceCmd c;
	uint8_t aFlags = r.U8();
	c.mDepth = r.U16();
	c.mMove = (aFlags & 0x01) != 0;
	if (aFlags & 0x02) {
		c.mHasChar = true;
		c.mCharId = r.U16();
	}
	if (aFlags & 0x04) {
		c.mHasMatrix = true;
		c.mMatrix = ReadMatrix(r);
	}
	if (aFlags & 0x08) {
		c.mHasCX = true;
		c.mCX = ReadCXForm(r);
	}
	if (aFlags & 0x10) {
		c.mHasRatio = true;
		c.mRatio = r.U16();
	}
	if (aFlags & 0x20)
		r.CString();
	if (aFlags & 0x40) {
		c.mHasClip = true;
		c.mClipDepth = r.U16();
	}
	if (aFlags & 0x80)
		theMovie.mUnsupported["PlaceObject2 clip actions"]++;
	if (r.mOk)
		theFrame.mCmds.push_back(c);
}

void ReadDoAction(Reader& r, SwfData& theMovie, SwfFrame& theFrame)
{
	while (r.mOk && !r.AtEnd()) {
		uint8_t aCode = r.U8();
		if (aCode == 0)
			break;
		size_t aLength = 0;
		if (aCode >= 0x80)
			aLength = r.U16();
		if (aLength > r.Remaining()) {
			r.mOk = false;
			break;
		}
		size_t anEnd = r.mPos + aLength;
		if (aCode == 0x06 || aCode == 0x07) {
			SwfAction a;
			a.mType = aCode == 0x06 ? SwfAction::kPlay : SwfAction::kStop;
			theFrame.mActions.push_back(a);
		}
		else if (aCode == 0x83) {
			SwfAction a;
			a.mType = SwfAction::kGetURL;
			a.mURL = r.CString();
			a.mTarget = r.CString();
			theFrame.mActions.push_back(a);
		}
		else {
			char aName[32];
			snprintf(aName, sizeof(aName), "action 0x%02x", aCode);
			theMovie.mUnsupported[aName]++;
		}
		r.mPos = anEnd;
		r.mBitsLeft = 0;
		r.mOk = r.mOk && r.mPos <= r.mSize;
	}
}

// The tags of the movie (root, true) or of a sprite (the rest of its DefineSprite tag), up to End
bool ReadTags(Reader& r, SwfData& theMovie, SwfTimelineDef& theTimeline, bool theRoot, std::string& theError)
{
	SwfFrame aFrame;
	while (r.Remaining() >= 2) {
		uint16_t aCodeAndLength = r.U16();
		int aTag = aCodeAndLength >> 6;
		size_t aLength = aCodeAndLength & 0x3f;
		if (aLength == 0x3f)
			aLength = r.U32();
		if (aLength > r.Remaining()) {
			theError = "a tag runs past the end of the file";
			return false;
		}
		Reader aBody(r.mData + r.mPos, aLength);
		r.mPos += aLength;
		if (aTag == 0)
			break;

		switch (aTag) {
		case 1:  // ShowFrame
			theTimeline.mFrames.push_back(aFrame);
			aFrame = SwfFrame();
			break;
		case 2: ReadShapeTag(aBody, theMovie, 1); break;
		case 22: ReadShapeTag(aBody, theMovie, 2); break;
		case 32: ReadShapeTag(aBody, theMovie, 3); break;
		case 46: ReadMorphShapeTag(aBody, theMovie); break;
		case 11: ReadTextTag(aBody, theMovie, false); break;
		case 33: ReadTextTag(aBody, theMovie, true); break;
		case 48: ReadFont2Tag(aBody, theMovie); break;
		case 39: {  // DefineSprite: its id, its frame count, then its own tags
			int anId = aBody.U16();
			aBody.U16();
			SwfCharacter& aChar = NewChar(theMovie, anId, SwfCharacter::kSprite);
			if (!ReadTags(aBody, theMovie, aChar.mSprite, false, theError))
				return false;
			break;
		}
		case 26: ReadPlaceObject2(aBody, theMovie, aFrame); break;
		case 28: {
			SwfPlaceCmd c;
			c.mRemove = true;
			c.mDepth = aBody.U16();
			aFrame.mCmds.push_back(c);
			break;
		}
		case 6: case 8: case 20: case 21: case 35: case 36: ReadBitmapTag(aBody, theMovie, aTag); break;
		case 12: ReadDoAction(aBody, theMovie, aFrame); break;
		case 43: aFrame.mLabel = aBody.CString(); break;
		case 9:
			if (theRoot)
				theMovie.mBackground = ReadColor(aBody, false);
			break;
		case 24:  // Protect
			break;
		case 88:  // DefineFontName: the name and copyright of a font, which nothing here shows
			break;
		default: {
			char aName[64];
			const char* aKnown = TagName(aTag);
			if (*aKnown)
				snprintf(aName, sizeof(aName), "tag %d %s", aTag, aKnown);
			else
				snprintf(aName, sizeof(aName), "tag %d", aTag);
			theMovie.mUnsupported[aName]++;
			break;
		}
		}
		if (!aBody.mOk) {
			char aMessage[64];
			snprintf(aMessage, sizeof(aMessage), "tag %d is cut short", aTag);
			theError = aMessage;
			return false;
		}
	}
	return true;
}

}  // namespace

bool Sexy::SwfParse(const uint8_t* theData, size_t theSize, SwfData& theMovie, std::string& theError)
{
	if (theSize < 12 || theData[1] != 'W' || theData[2] != 'S' || (theData[0] != 'F' && theData[0] != 'C' && theData[0] != 'Z')) {
		theError = "not a SWF file";
		return false;
	}
	if (theData[0] == 'Z') {
		theError = "LZMA compressed SWF files are not supported";
		return false;
	}
	theMovie.mVersion = theData[3];

	// A compressed file is a zlib stream after the first eight bytes
	std::vector<uint8_t> aBody;
	if (theData[0] == 'C') {
		int aLength = 0;
		char* anInflated = stbi_zlib_decode_malloc((const char*)theData + 8, (int)(theSize - 8), &aLength);
		if (anInflated == nullptr) {
			theError = "the compressed data is damaged";
			return false;
		}
		aBody.assign((const uint8_t*)anInflated, (const uint8_t*)anInflated + aLength);
		free(anInflated);
	}
	else
		aBody.assign(theData + 8, theData + theSize);

	Reader r(aBody.data(), aBody.size());
	float aFrame[4];
	ReadRect(r, aFrame);
	theMovie.mXMin = aFrame[0];
	theMovie.mXMax = aFrame[1];
	theMovie.mYMin = aFrame[2];
	theMovie.mYMax = aFrame[3];
	theMovie.mFrameRate = r.U16() / 256.0f;
	r.U16();  // frame count; the ShowFrame tags are what counts
	if (!r.mOk || theMovie.mXMax <= theMovie.mXMin || theMovie.mYMax <= theMovie.mYMin) {
		theError = "the header is damaged";
		return false;
	}
	return ReadTags(r, theMovie, theMovie.mRoot, true, theError);
}
