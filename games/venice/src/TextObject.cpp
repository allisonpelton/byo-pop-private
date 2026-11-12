// Fonts and text objects (0x4242f0-0x425ab0).

#include "TextObject.h"

#include "GameText.h"
#include "Level.h"
#include "VeniceDraw.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstring>

CursorList<FontRecord> gFonts; // 0x5ba448

// the side that the next glow of a hot text flies to (0x5b5170): it changes with every glow, so the two glows of one hover
// go to both sides
static float sGlowDirection = 1.0f;

// 0x4242f0
bool FontRecord::Load(const char* theName, const char* theCharacterSet, int theLetterHeight)
{
	strcpy(mName, theName);
	strcpy(mCharacterSet, theCharacterSet);
	mImage = AcquireImageRecord(theName, 0, 0, 0, 0);
	bool aLoaded = mImage != nullptr;
	if (aLoaded) {
		mImage->LoadPixels();
		FindGlyphs(theLetterHeight);
		memcpy(mKerned, mGlyphs, sizeof(mGlyphs));
	}
	return aLoaded;
}

// 0x4243a0: the picture is read a row of letters at a time (a last row that ends at the picture's bottom edge is not read);
// the space is as wide as the capital I
void FontRecord::FindGlyphs(int theLetterHeight)
{
	int aWidth = (int)mImage->mWidth;
	int aHeight = (int)mImage->mHeight;
	int aCharacterIndex = 0;
	for (int aTop = 0; aTop + theLetterHeight < aHeight; aTop += theLetterHeight) {
		const uint32_t* aRow = mImage->mPixels + (mImage->mStride / 4) * aTop;
		FindGlyphsInRow(&aCharacterIndex, aRow, aTop, aWidth, theLetterHeight);
	}
	mGlyphs[' '] = mGlyphs['I'];
}

// 0x4244a0: a letter is a run of columns that have a pixel that is not transparent
void FontRecord::FindGlyphsInRow(int* theCharacterIndex, const uint32_t* theRow, int theTop, int theWidth, int theHeight)
{
	int aColumn = 0;
	while (aColumn < theWidth) {
		while (aColumn < theWidth && !ColumnHasPixels(aColumn, theRow, theWidth, theHeight))
			aColumn++;
		if (aColumn >= theWidth)
			return;
		float aLeft = (float)aColumn;
		while (aColumn < theWidth && ColumnHasPixels(aColumn, theRow, theWidth, theHeight))
			aColumn++;
		GlyphRect& aGlyph = mGlyphs[(unsigned char)mCharacterSet[*theCharacterIndex]];
		aGlyph.mLeft = aLeft;
		aGlyph.mRight = ((float)aColumn - 1.0f) + 1.0f;
		aGlyph.mTop = (float)theTop;
		aGlyph.mBottom = ((float)theTop + (float)theHeight) - 1.0f;
		*theCharacterIndex = *theCharacterIndex + 1;
	}
}

// 0x424610
bool FontRecord::ColumnHasPixels(int theColumn, const uint32_t* theRow, int theWidth, int theHeight)
{
	for (int aRow = 0; aRow < theHeight; aRow++)
		if ((theRow[aRow * theWidth + theColumn] >> 24) != 0)
			return true;
	return false;
}

// 0x424660
float FontRecord::MeasureWidth(const char* theString, float theScale, float theSpacing) const
{
	float aWidth = 0.0f;
	for (size_t aLetter = 0; aLetter < strlen(theString); aLetter++) {
		const GlyphRect& aGlyph = mGlyphs[(unsigned char)theString[aLetter]];
		aWidth = (float)(((double)aGlyph.mRight - (double)aGlyph.mLeft) * (double)theScale + (double)aWidth);
		if (aLetter != strlen(theString) - 1)
			aWidth = aWidth + theSpacing;
	}
	return aWidth;
}

// 0x424710
FontRecord* AcquireFont(const char* theName, const char* theCharacterSet, int theLetterHeight)
{
	bool aMore = gFonts.Begin();
	while (aMore) {
		if (strcmp(theName, gFonts.mCursor->mName) == 0)
			return gFonts.mCursor;
		aMore = gFonts.Next();
	}
	gFonts.Append(new FontRecord());
	if (theCharacterSet == nullptr || theLetterHeight == 0 || !gFonts.mCursor->Load(theName, theCharacterSet, theLetterHeight))
		return nullptr;
	return gFonts.mCursor;
}

// 0x4247a0
void AddFontKerning(const char* theCharacter, float theLeft, float theRight)
{
	GlyphRect& aGlyph = gFonts.mTail->mKerned[(unsigned char)theCharacter[0]];
	aGlyph.mLeft = aGlyph.mLeft + theLeft;
	aGlyph.mRight = aGlyph.mRight + theRight;
}

// 0x424810
TextObject::TextObject()
{
	mZOrder = 10.0f;
	mType = ObjectTypeText;
	mIsHot = false;
	mText[0] = 0;
}

// 0x424860
void TextObject::Load(const char* theText, SceneTag* theTag)
{
	int aJustification = JustifyCenter;
	if (theTag->GetString("justification") != nullptr) {
		if (strcmp(theTag->GetString("justification"), "left") == 0)
			aJustification = JustifyLeft;
		else if (strcmp(theTag->GetString("justification"), "right") == 0)
			aJustification = JustifyRight;
	}
	if (theTag->Has("z_order")) {
		mZOrder = (float)theTag->GetFloat("z_order");
		SetZOrder(mZOrder);
	}
	int aStagger = theTag->GetInt("stagger_ms");
	float aSpacing = (float)theTag->GetFloat("spacing");
	SetText(theTag->GetString("string"), theTag->GetString("font"), aSpacing, aStagger, aJustification);
}

// 0x424960: a letter that follows the first starts a stagger later than the one before it, and is hidden until then
void TextObject::SetText(const char* theString, const char* theFontName, float theSpacing, int theStagger,
	int theJustification)
{
	mChildren.Clear();
	mStages.Clear();
	strcpy(mText, TranslateText(theString));
	mFont = AcquireFont(theFontName, nullptr, 0);
	mStagger = theStagger;
	mJustification = theJustification;
	mSpacing = theSpacing;
	mImage = mFont->mImage;
	mImage->mReferenceCount++;
	float aDelay = 0.0f;
	for (size_t aLetter = 0; aLetter < strlen(mText); aLetter++) {
		SceneObject* aChild = new SceneObject();
		aChild->mImage = mImage;
		mImage->mReferenceCount++;
		aChild->mZOrder = mZOrder;
		aChild->mBilinear = mBilinear;
		aChild->mSnap = 0;
		NewChild(aChild);
		const GlyphRect& aGlyph = mFont->mGlyphs[(unsigned char)mText[aLetter]];
		aChild->mSourceLeft = aGlyph.mLeft;
		aChild->mSourceTop = aGlyph.mTop;
		aChild->mSourceRight = aGlyph.mRight;
		aChild->mSourceBottom = aGlyph.mBottom;
		if (aDelay != 0.0f) {
			aChild->AddStage(aDelay, 0, 0, 1, nullptr);
			aChild->mStages.mTail->mVisible = false;
		}
		aDelay = (float)(unsigned int)mStagger + aDelay;
	}
}

// 0x424b60: the text's own stage lasts until the last letter's has ended; the stage of a space does not show
SceneStage* TextObject::AddStage(float theMilliseconds, int theLoopStart, int theLoopEnd, int theVisible,
	const char* theName)
{
	int aLetter = 0;
	SceneObject::AddStage((float)((int)strlen(mText) * mStagger) + theMilliseconds, theLoopStart, theLoopEnd, theVisible,
		theName);
	bool aMore = mChildren.Begin();
	while (aMore) {
		mChildren.mCursor->AddStage(theMilliseconds, theLoopStart, theLoopEnd, theVisible, theName);
		if (mText[aLetter] == ' ')
			mChildren.mCursor->mStages.mTail->mVisible = false;
		aLetter++;
		aMore = mChildren.Next();
	}
	return nullptr;
}

// 0x424c70
SceneFilter* TextObject::AddFilter(int theType, int theAttribute, float theAmplitude, float thePeriod, float theOffset)
{
	bool aMore = mChildren.Begin();
	while (aMore) {
		mChildren.mCursor->AddFilter(theType, theAttribute, theAmplitude, thePeriod, theOffset);
		aMore = mChildren.Next();
	}
	return nullptr;
}

// 0x424cd0: every letter gets the attribute; an x is moved to the letter's place in the string first
void TextObject::SetAttribute(int theAttribute, float theStart, float theEnd)
{
	bool aMore = mChildren.Begin();
	while (aMore) {
		float aStart = theStart;
		float aEnd = theEnd;
		if (theAttribute == AttributeX)
			GetLayoutOffsets(&aStart, &aEnd);
		mChildren.mCursor->SetAttribute(theAttribute, aStart, aEnd);
		aMore = mChildren.Next();
	}
}

// 0x424d50: moves an x (the value at the start and the value at the end of a stage, which scale differently when the first
// letter's stage scales) to where the letter at the cursor goes: the widths of the letters before it, less half the text's
// width for a centred text or all of it for a right-justified one
void TextObject::GetLayoutOffsets(float* theStartX, float* theEndX)
{
	SceneStage* aFirstStage = mChildren.mHead->mStages.mTail;
	float aStartScale = (aFirstStage->mAttributeMask & 1) == 0 ? 1.0f : aFirstStage->mStart[AttributeXScale];
	float aEndScale = (aFirstStage->mAttributeMask & 1) == 0 ? 1.0f : aFirstStage->mEnd[AttributeXScale];
	float aCursorStartX = 0.0f;
	float aCursorEndX = 0.0f;
	float aStartWidth = 0.0f;
	float aEndWidth = 0.0f;
	int aLetter = 0;
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		const GlyphRect& aGlyph = mFont->mKerned[(unsigned char)mText[aLetter]];
		float aStartHalf = (float)(((double)(aGlyph.mRight - aGlyph.mLeft) / 2.0) * (double)aStartScale);
		aStartWidth = aStartWidth + aStartHalf;
		float aEndHalf = (float)(((double)(aGlyph.mRight - aGlyph.mLeft) / 2.0) * (double)aEndScale);
		aEndWidth = aEndWidth + aEndHalf;
		if (aChild == mChildren.mCursor) {
			aCursorStartX = (((aStartWidth - aGlyph.mLeft) + aChild->mSourceLeft) - aGlyph.mRight) + aChild->mSourceRight;
			aCursorEndX = (((aEndWidth - aGlyph.mLeft) + aChild->mSourceLeft) - aGlyph.mRight) + aChild->mSourceRight;
		}
		aStartWidth = (float)((double)aStartScale * (double)mSpacing + (double)aStartWidth + (double)aStartHalf);
		aEndWidth = (float)((double)aEndScale * (double)mSpacing + (double)aEndWidth + (double)aEndHalf);
		aLetter++;
	}
	aStartWidth = (float)((double)aStartWidth - (double)aStartScale * (double)mSpacing);
	aEndWidth = (float)((double)aEndWidth - (double)aEndScale * (double)mSpacing);
	if (*theStartX != AttributeUnsetValue && *theStartX != AttributeContinueValue) {
		if (mJustification == JustifyLeft) {
			*theStartX = *theStartX + aCursorStartX;
		} else if (mJustification == JustifyRight) {
			*theStartX = *theStartX - aStartWidth;
			*theStartX = *theStartX + aCursorStartX;
		} else if (mJustification == JustifyCenter) {
			*theStartX = (float)((double)*theStartX - (double)aStartWidth / 2.0);
			*theStartX = *theStartX + aCursorStartX;
		}
	}
	if (*theEndX != AttributeUnsetValue && *theEndX != AttributeContinueValue) {
		if (mJustification == JustifyLeft) {
			*theEndX = *theEndX + aCursorEndX;
		} else if (mJustification == JustifyRight) {
			*theEndX = *theEndX - aEndWidth;
			*theEndX = *theEndX + aCursorEndX;
		} else if (mJustification == JustifyCenter) {
			*theEndX = (float)((double)*theEndX - (double)aEndWidth / 2.0);
			*theEndX = *theEndX + aCursorEndX;
		}
	}
}

// 0x425220: the rectangle that the letters cover
void TextObject::GetBounds(float* theWidth, float* theHeight, float* theCenterX, float* theCenterY)
{
	float aRight = -1e+08f;
	float aLeft = 1e+08f;
	float aBottom = -1e+08f;
	float aTop = 1e+08f;
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mType != ObjectTypeBase)
			continue;
		float aHalfWidth = (aChild->mSourceRight - aChild->mSourceLeft) / 2.0f;
		float aHalfHeight = (aChild->mSourceBottom - aChild->mSourceTop) / 2.0f;
		float aChildLeft = aChild->mLocal[AttributeX] - aHalfWidth;
		aLeft = aLeft <= aChildLeft ? aLeft : aChildLeft;
		float aChildRight = aChild->mLocal[AttributeX] + aHalfWidth;
		aRight = aChildRight <= aRight ? aRight : aChildRight;
		float aChildTop = aChild->mLocal[AttributeY] - aHalfHeight;
		aTop = aTop <= aChildTop ? aTop : aChildTop;
		float aChildBottom = aChild->mLocal[AttributeY] + aHalfHeight;
		aBottom = aChildBottom <= aBottom ? aBottom : aChildBottom;
	}
	if (theWidth != nullptr)
		*theWidth = aRight - aLeft;
	if (theHeight != nullptr)
		*theHeight = aBottom - aTop;
	if (theCenterX != nullptr)
		*theCenterX = (aLeft + aRight) / 2.0f;
	if (theCenterY != nullptr)
		*theCenterY = (aTop + aBottom) / 2.0f;
}

// 0x4254b0: the text is hot while one of its letters is. A pointer that comes onto it makes two glows (on an accelerated
// display only) and plays a sound at a random pitch.
bool TextObject::IsHot()
{
	UpdateWorldValues();
	SceneObject* aHotLetter = nullptr;
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->IsHot()) {
			aHotLetter = aChild;
			break;
		}
	}
	if (aHotLetter == nullptr) {
		mIsHot = false;
		return false;
	}
	if (!mIsHot) {
		for (int aGlow = 0; aGlow < 2 && IsAccelerated(); aGlow++) {
			SceneObject* aSprite = new SceneObject();
			aSprite->SetImageRecord(gActiveLevel->mLevelParticleImage);
			aSprite->mZOrder = (float)((double)mZOrder - 0.001);
			mParent->NewChild(aSprite);
			aSprite->AddStage(400.0f, 0, 0, 1, nullptr);
			aSprite->SetAttribute(AttributeAlpha, 0.3f, 0.0f);
			aSprite->SetAttribute(AttributeRed, 1.0f, 1.0f);
			aSprite->SetAttribute(AttributeGreen, 1.0f, 1.0f);
			aSprite->SetAttribute(AttributeBlue, 1.0f, 1.0f);
			sGlowDirection = 0.0f - sGlowDirection;
			float aWidth;
			float aHeight;
			float aCenterX;
			float aCenterY;
			GetBounds(&aWidth, &aHeight, &aCenterX, &aCenterY);
			aSprite->SetAttribute(AttributeX, aCenterX, (float)((double)sGlowDirection * (double)aWidth + (double)aCenterX));
			aSprite->SetAttribute(AttributeY, aCenterY, aCenterY);
			aSprite->AddFilter(FilterVelocity, AttributeXScale, 1.0f, 0.0f, 0.0f);
			aSprite->Update();
		}
		int aFrequency = (int)RandomBelow(8000) + 40100;
		PlaySoundByName("audio\\hot.ogg", aFrequency);
	}
	mIsHot = true;
	return true;
}

// 0x4256e0
SceneStage* TextObject::GoToStageByName(const char* theName)
{
	SceneStage* aStage = SceneObject::GoToStageByName(theName);
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext)
		aChild->GoToStageByName(theName);
	return aStage;
}

// 0x425730
void TextObject::MakeSparkles(int theCount, float theRed, float theGreen, float theBlue, float theAlpha)
{
	int aLength = (int)strlen(mText);
	UpdateWorldValues();
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext)
		aChild->MakeSparkles(theCount / aLength, theRed, theGreen, theBlue, theAlpha);
}

void DrawTextOnObject(SceneObject* theObject, const char* theString, const char* theFontName, float theX, float theY,
	float theSpacing, int theJustification, float theScale, float theAlpha)
{
	// the original's is a static of the function, made at the first call
	static TextObject* aText = new TextObject();
	aText->mBilinear = theObject->mBilinear;
	aText->mParent = theObject;
	aText->SetText(theString, theFontName, theSpacing, 0, theJustification);
	aText->AddStage(1000.0f, 1, 1, 1, nullptr);
	aText->SetAttribute(AttributeAlpha, theAlpha, theAlpha);
	aText->SetAttribute(AttributeXScale, theScale, theScale);
	aText->SetAttribute(AttributeYScale, theScale, theScale);
	aText->SetAttribute(AttributeX, theX, theX);
	aText->SetAttribute(AttributeY, theY, theY);
	aText->Update();
	aText->Draw();
}

int CutTextLine(char* theText, int theStart, int theWidth, char* theLine)
{
	int aTextLength = (int)strlen(theText);
	int aLimit = theStart + theWidth < aTextLength ? theStart + theWidth : aTextLength;
	int aLineEnd = aLimit;
	for (int aLetter = theStart; aLetter < aLimit; aLetter++) {
		if (theText[aLetter] == '^') {
			theText[aLetter] = ' ';
			aLineEnd = aLetter;
			break;
		}
	}
	if (aLineEnd < aTextLength) {
		while (theStart < aLineEnd && theText[aLineEnd] != ' ')
			aLineEnd--;
	}
	strncpy(theLine, theText + theStart, aLineEnd - theStart);
	theLine[aLineEnd - theStart] = '\0';
	return aLineEnd + 1;
}
