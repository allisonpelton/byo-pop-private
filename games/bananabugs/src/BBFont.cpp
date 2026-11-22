#include "BBFont.h"

#include "DDImage.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

#include <cstring>

using namespace Sexy;

// The character whose box the control characters and the space take
const int GLYPH_BOX_SOURCE = 'O';
const int GLYPH_BOX_COPIES = 0x21;

// 0x42b8f0
BBFont::BBFont()
{
	mImage = nullptr;
	mImageFont = nullptr;
	// mScale, mCelSize, mFitCapHeight and mGlyphs are left as operator new gave them; Load sets them before use
	mScale = 0.0f;
	mCelSize = 0;
	mFitCapHeight = false;
	memset(mGlyphs, 0, sizeof(mGlyphs));
}

// 0x42b910
BBFont::~BBFont()
{
	if (mImageFont != nullptr) {
		delete mImageFont;
		mImageFont = nullptr;
	}
}

// 0x42b980: the cels, at the window's scale. The glyphs are drawn smoothly, left-aligned in their cels, and each cel
// is then cut at its first column with no ink.
DDImage* BBFont::BuildCels(DDImage* theFontImage)
{
	Rect aSrcRect(0, 0, 0, 0);
	double aScale;
	if (mFitCapHeight)
		aScale = (43.0 / (double)(mGlyphs[0].mBottom - mGlyphs[0].mTop + 1)) * (double)gScreenScaleY;
	else
		aScale = (double)gScreenScaleY;
	mScale = (float)aScale;
	mCelSize = (int)(aScale * (double)(mGlyphs[0].mBottom - mGlyphs[0].mTop + 1) + 0.5);
	int aSheetSize = mCelSize << 4;
	MemoryImage* aSheet = new MemoryImage(sApp);
	aSheet->Create(aSheetSize, aSheetSize);
	{
		Graphics aSheetG(aSheet);
		aSheetG.SetFastStretch(false);
		for (unsigned int aChar = 0; (int)aChar < 0x100; aChar++) {
			int aCelSize = mCelSize;
			const GlyphBox& aBox = mGlyphs[aChar];
			aSrcRect = Rect(aBox.mLeft, aBox.mTop, aBox.mRight - aBox.mLeft + 1, aBox.mBottom - aBox.mTop + 1);
			Rect aDestRect(aCelSize * (int)(aChar % 16), aCelSize * (int)(aChar / 16),
				(int)((double)aSrcRect.mWidth * aScale), aCelSize);
			aSheetG.DrawImage(theFontImage, aDestRect, aSrcRect);
		}
		// The original makes a DDImage of the sheet's bits, commits them and makes its DirectDraw surface (0x47ffd0);
		// the framework's DDImage makes its texture when first drawn
		DDImage* aCels = new DDImage(sApp->mDDInterface);
		aCels->SetBits(aSheet->GetBits(), aSheetSize, aSheetSize, true);
		aCels->CommitBits();
		delete aSheet;
		ulong* aBits = aCels->GetBits();
		mCelWidth = (float)mCelSize;
		mCelsAcross = 16;
		mCelHeight = (float)mCelSize;
		mCelsDown = 16;
		mNumCels = 0x100;
		for (int aChar = 0; aChar < 0x100; aChar++) {
			AnimCel& aCel = mCels[aChar];
			aCel.mWidth = mCelSize;
			aCel.mHeight = mCelSize;
			int aX = (aChar % 16) * mCelSize;
			int aY = (aChar / 16) * mCelSize;
			aCel.mX = aX;
			aCel.mY = aY;
			aCel.mBottom = mCelSize + aY - 1;
			for (int aColumn = 0; aColumn < mCelSize; aColumn++) {
				bool anInked = false;
				for (int aRow = 0; aRow < mCelSize; aRow++) {
					if (aBits[aCels->GetWidth() * (aY + aRow) + aColumn + aX] != 0)
						anInked = true;
				}
				if (!anInked) {
					aCel.mWidth = aColumn + 1;
					aCel.mRight = aColumn;
					break;
				}
			}
		}
		return aCels;
	}
}

// 0x42bd10: the font file (a PopCap font descriptor), its picture's inked rows, and each glyph's inked columns
bool BBFont::Load(bool fitCapHeight, const char* theFileName)
{
	mFitCapHeight = fitCapHeight;
	strcpy(mName, theFileName);
	mImageFont = new ImageFont(sApp, std::string(mName));
	// the font's first active layer: its picture at the font's point size, and each glyph's rectangle in it
	ActiveFontLayer& aLayer = mImageFont->mActiveLayerList.front();
	DDImage* aFontImage = dynamic_cast<DDImage*>(aLayer.mScaledImage);
	ulong* aBits = aFontImage->GetBits();
	int aWidth = aFontImage->GetWidth();
	int aHeight = aFontImage->GetHeight();

	// The first row and the last with any ink (the last is looked for down to row 1); -1 when there is none
	int aTop = -1;
	for (int aRow = 0; aRow < aHeight; aRow++) {
		bool anInked = false;
		for (int aColumn = 0; aColumn < aWidth; aColumn++) {
			if (aBits[aRow * aWidth + aColumn] != 0) {
				anInked = true;
				break;
			}
		}
		if (anInked) {
			aTop = aRow;
			break;
		}
	}
	int aBottom = -1;
	for (int aRow = aHeight - 1; 0 < aRow; aRow--) {
		bool anInked = false;
		for (int aColumn = 0; aColumn < aWidth; aColumn++) {
			if (aBits[aRow * aWidth + aColumn] != 0) {
				anInked = true;
				break;
			}
		}
		if (anInked) {
			aBottom = aRow;
			break;
		}
	}

	// Each glyph's first and last inked column within its rectangle (which the scan takes one column too wide).
	// The two are kept from glyph to glyph, so a glyph with no ink takes the last inked glyph's columns.
	int anInkLeft = -1;
	int anInkRight = -1;
	for (int aChar = 0; aChar < 0x100; aChar++) {
		const Rect& aRect = aLayer.mScaledCharImageRects[aChar];
		int aLeft = aRect.mX;
		int aRight = aRect.mWidth + aLeft;
		GlyphBox& aBox = mGlyphs[aChar];
		aBox.mLeft = aLeft;
		aBox.mTop = aTop;
		aBox.mRight = aRight;
		aBox.mBottom = aBottom;
		for (int aColumn = aLeft; aColumn <= aBox.mRight; aColumn++) {
			bool anInked = false;
			for (int aRow = 0; aRow < aHeight; aRow++) {
				if (aBits[aRow * aWidth + aColumn] != 0) {
					anInked = true;
					break;
				}
			}
			if (anInked) {
				anInkLeft = aColumn;
				break;
			}
		}
		if (aLeft < aRight) {
			for (int aColumn = aRight; aBox.mLeft < aColumn; aColumn--) {
				bool anInked = false;
				for (int aRow = 0; aRow < aHeight; aRow++) {
					if (aBits[aRow * aWidth + aColumn] != 0) {
						anInked = true;
						break;
					}
				}
				if (anInked) {
					anInkRight = aColumn;
					break;
				}
			}
		}
		if (anInkLeft != -1 && anInkRight != -1) {
			aBox.mLeft = anInkLeft;
			aBox.mRight = anInkRight;
		}
	}
	for (int aChar = 0; aChar < GLYPH_BOX_COPIES; aChar++)
		mGlyphs[aChar] = mGlyphs[GLYPH_BOX_SOURCE];

	mImage = BuildCels(aFontImage);
	sAnims.push_back(this);
	return true;
}

// 0x42c0f0: the cels again, at the window's new scale
void BBFont::Reload()
{
	ActiveFontLayer& aLayer = mImageFont->mActiveLayerList.front();
	mImage = BuildCels(dynamic_cast<DDImage*>(aLayer.mScaledImage));
}

// 0x42c140: a font, or none before the app is set
BBFont* BBFont::Create(const char* theFileName, bool fitCapHeight)
{
	if (sApp == nullptr)
		return nullptr;
	BBFont* aFont = new BBFont();
	aFont->Load(fitCapHeight, theFileName);
	return aFont;
}
