#include "ObjString.h"

#include "Anim.h"
#include "Color.h"
#include "GameApp.h"
#include "Graphics.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// A glyph's width in the font's cels (the original indexes by the signed character; no text reaches above 127)
static int GlyphCelWidth(Anim* theFont, char theChar)
{
	return theFont->mCels[(unsigned char)theChar].mWidth;
}

static int GlyphCelHeight(Anim* theFont, char theChar)
{
	return theFont->mCels[(unsigned char)theChar].mHeight;
}

// 0x46e910
ObjString::ObjString()
{
	mText = nullptr;
	mLength = 0;
	mBlink = 0;
	mHidden = true;
	mType = OBJ_TYPE_STRING;
	mCursor = -1;
	// the original leaves these unset until Create
	mGlyphs = nullptr;
	mGlyphX = nullptr;
	mLeftAligned = false;
	mDrawnY = 0.0f;
}

// 0x46e970. The original frees only the text; the port frees the glyph and position lists too. The glyphs are
// children, deleted with this object.
ObjString::~ObjString()
{
	if (mText != nullptr) {
		free(mText);
		mText = nullptr;
	}
	free(mGlyphs);
	free(mGlyphX);
}

// 0x46e9a0
bool ObjString::Update()
{
	if (!ObjBase::Update())
		return false;
	mDrawnY = mDrawn[STATE_Y];
	return true;
}

// 0x46e9c0: only the text cursor, a gold bar before the glyph at the cursor (after the last glyph at the end), shown
// half the time. The glyphs are placed again each time it draws.
void ObjString::Draw(Graphics* g)
{
	if (mCursor < 0 || mText == nullptr)
		return;
	if (mBlink < 16) {
		SetText(mText);
		int aLength = (int)strlen(mText);
		if (aLength == 0) {
			float aX = (float)mGlyphX[0];
			float aHeight = (float)(GlyphCelHeight(mAnim, mText[0]) - 4) * mDrawn[STATE_SCALE_Y];
			g->SetColor(Color(200, 160, 0, 255));
			g->FillRect((int)aX, (int)(mDrawnY - aHeight * 0.5) + 1, 3, (int)aHeight - 2);
		} else if (mCursor < aLength) {
			float aX = (float)mGlyphX[mCursor];
			char aChar = mText[mCursor];
			float aWidth = (float)(GlyphCelWidth(mAnim, aChar) + 2) * mDrawn[STATE_SCALE_X];
			float aHeight = (float)(GlyphCelHeight(mAnim, aChar) - 4) * mDrawn[STATE_SCALE_Y];
			g->SetColor(Color(200, 160, 0, 255));
			int aY = (int)(mDrawnY - aHeight * 0.5) + 1;
			g->FillRect((int)(aX - aWidth * 0.5 + 1.0), aY, 3, (int)aHeight - 2);
		} else {
			float aX = (float)mGlyphX[mCursor - 1];
			char aChar = mText[mCursor - 1];
			float aWidth = (float)(GlyphCelWidth(mAnim, aChar) + 2) * mDrawn[STATE_SCALE_X];
			float aHeight = (float)(GlyphCelHeight(mAnim, aChar) - 4) * mDrawn[STATE_SCALE_Y];
			g->SetColor(Color(200, 160, 0, 255));
			int aBarHeight = (int)(aHeight - 2.0);
			int aY = (int)(mDrawnY - aHeight * 0.5 + 1.0);
			g->FillRect((int)(aWidth * 0.5 + aX), aY, 3, aBarHeight);
		}
	}
	mBlink = (mBlink + 1) % 32;
}

// 0x46ec50
int ObjString::GetTextLength()
{
	if (mText == nullptr)
		return 0;
	return (int)strlen(mText);
}

// 0x46ec70: the text's width in theObj's font, in window pixels
float ObjString::TextWidth(const char* theText, ObjBase* theObj)
{
	int aLength = (int)strlen(theText);
	float aScale = sApp->mDesignScaleY;
	float aWidth = 0.0f;
	int i = 0;
	// four at a time, as the original's loop
	if (3 < aLength) {
		do {
			aWidth = (float)((double)GlyphCelWidth(theObj->mAnim, theText[i + 3]) * aScale +
				(double)GlyphCelWidth(theObj->mAnim, theText[i + 2]) * aScale +
				(double)GlyphCelWidth(theObj->mAnim, theText[i + 1]) * aScale +
				(double)GlyphCelWidth(theObj->mAnim, theText[i]) * aScale + aWidth);
			i += 4;
		} while (i < aLength - 3);
	}
	for (; i < aLength; i++)
		aWidth = (float)((double)GlyphCelWidth(theObj->mAnim, theText[i]) * aScale + aWidth);
	return aWidth;
}

// 0x46ed60: a glyph object for each character, copying this object's tracks and font, until the glyphs' widths reach
// theMaxWidth (no limit at 0 or below). The string then holds no more characters than theText has.
void ObjString::Create(const char* theText, bool leftAligned, int theMaxWidth)
{
	if (mAnim == nullptr)
		return;
	ComputeDrawn();
	mDrawnY = mDrawn[STATE_Y];
	mLeftAligned = leftAligned;
	int aLength = (int)strlen(theText);
	mLength = aLength;
	// the original allocates again on each call; the port frees what an earlier call made
	free(mText);
	free(mGlyphs);
	free(mGlyphX);
	mText = (char*)malloc(aLength + 1);
	memset(mText, 0, aLength + 1);
	mGlyphs = (ObjBase**)calloc(aLength + 1, sizeof(ObjBase*));
	mGlyphX = (int*)calloc(aLength + 2, sizeof(int));
	mGlyphX[0] = (int)mDrawn[STATE_X];
	mGlyphX[1] = (int)mDrawn[STATE_X];
	strcpy(mText, theText);

	float aScale = sApp->mDesignScaleY;
	float aX;
	if (!mLeftAligned) {
		aX = (float)(-(double)TextWidth(mText, this) * 0.5);
	} else {
		float aHalf = (float)((double)GlyphCelWidth(mAnim, mText[0]) * aScale * 0.5);
		aX = (float)((double)aHalf * 0.5);
	}
	float aLastHalf = 0.0f;
	int aWidthSoFar = 0;
	for (int i = 0; i < aLength; i++) {
		char aChar = mText[i];
		float aHalf = (float)((double)GlyphCelWidth(mAnim, aChar) * aScale * 0.5);
		double aStep = (double)aLastHalf + aHalf;
		aX = (float)((double)aX + aStep);
		aWidthSoFar += (int)aStep;
		ObjBase* aGlyph = new ObjBase();
		aGlyph->CopyFrom(this);
		mGlyphs[i] = aGlyph;
		aGlyph->SetState(STATE_X, aX);
		aGlyph->SetStateInt(STATE_Z, 1);
		aGlyph->SetParent(this, nullptr);
		if (aChar > ' ') {
			aGlyph->mType = OBJ_TYPE_STRING_GLYPH;
			aGlyph->SetState(STATE_FRAME, (float)aChar);
			aGlyph->SetStateInt(STATE_SCALE_X, 1);
			aGlyph->SetStateInt(STATE_SCALE_Y, 1);
			aGlyph->SetStateInt(STATE_RED, 1);
			aGlyph->SetStateInt(STATE_GREEN, 1);
			aGlyph->SetStateInt(STATE_BLUE, 1);
			aGlyph->SetStateInt(STATE_ALPHA, 1);
			aGlyph->Activate(aGlyph, false);
		} else {
			aGlyph->Deactivate(aGlyph);
		}
		if (mCursor >= 0)
			aGlyph->ComputeDrawn();
		mGlyphX[i] = (int)aGlyph->mDrawn[STATE_X];
		aLastHalf = aHalf;
		if (theMaxWidth > 0 && aWidthSoFar >= theMaxWidth)
			break;
	}
	// the original adds the width of the cel numbered by the length, not the last character's
	if (aLength > 0)
		mGlyphX[aLength] += mAnim->mCels[aLength - 1].mWidth;
	mCurrentKey = nullptr;
	Activate(this, true);
}

// 0x46f050: the glyphs show theText, placed again from the drawn position; glyphs past its end are taken out. Left
// alignment measures from the old text's first character. A null text empties the string.
void ObjString::SetText(const char* theText)
{
	ComputeDrawn();
	float aWidth;
	if (theText == nullptr) {
		aWidth = 0.0f;
		if (mCursor > 0)
			mCursor = 0;
		memset(mText, 0, mLength + 1);
	} else {
		aWidth = TextWidth(theText, this);
	}
	mDrawnY = mDrawn[STATE_Y];
	mGlyphX[0] = (int)mDrawn[STATE_X];
	mGlyphX[1] = (int)mDrawn[STATE_X];
	float aScale = sApp->mDesignScaleY;
	float aX;
	if (!mLeftAligned) {
		aX = (float)(-(double)aWidth * 0.5);
	} else {
		float aHalf = (float)((double)GlyphCelWidth(mAnim, mText[0]) * aScale * 0.5);
		aX = (float)((double)aHalf * 0.5);
	}
	float aLastHalf = 0.0f;
	unsigned int i = 0;
	for (ObjBase* aGlyph = mFirstChild[LIST_OWNED]; aGlyph != nullptr; aGlyph = aGlyph->mNextSibling[LIST_OWNED], i++) {
		if (theText == nullptr || strlen(theText) <= i) {
			aGlyph->Deactivate(aGlyph);
			continue;
		}
		char aChar = theText[i];
		float aHalf = (float)((double)GlyphCelWidth(mAnim, aChar) * aScale * 0.5);
		aX = (float)((double)aLastHalf + aHalf + aX);
		aGlyph->SetState(STATE_X, aX);
		if (aChar > ' ') {
			aGlyph->Activate(aGlyph, false);
			aGlyph->SetState(STATE_FRAME, (float)aChar);
			aGlyph->ComputeDrawn();
		} else {
			aGlyph->Deactivate(aGlyph);
		}
		if (mCursor >= 0)
			aGlyph->ComputeDrawn();
		mGlyphX[i] = (int)aGlyph->mDrawn[STATE_X];
		mGlyphX[i + 1] = (int)aHalf + mGlyphX[i];
		aLastHalf = aHalf;
	}
	if (mText != nullptr && theText != nullptr && mText != theText)
		strcpy(mText, theText);
}

// 0x46f270: no further than the text's end
void ObjString::SetCursor(int theIndex)
{
	mCursor = theIndex;
	if (theIndex < 0)
		return;
	if (mText == nullptr) {
		mCursor = 0;
		ComputeDrawn();
		mGlyphX[0] = (int)mDrawn[STATE_X];
		return;
	}
	if ((int)strlen(mText) < theIndex)
		mCursor = (int)strlen(mText);
}

// 0x46f2e0
void ObjString::MoveCursor(int theDelta)
{
	mCursor += theDelta;
	if (mCursor < 0)
		mCursor = 0;
	SetCursor(mCursor);
}

// 0x46f300: the key typed (a letter, a digit or a space) goes in at the cursor; the last character drops off a full
// string
void ObjString::InsertTypedKey()
{
	int aKey = sKeyHeld;
	if (aKey != ' ' && isalnum(aKey) == 0)
		return;
	if (mText == nullptr)
		return;
	for (int i = mLength - 1; mCursor <= i; i--)
		mText[i + 1] = mText[i];
	mText[mCursor] = (char)aKey;
	mText[mLength] = '\0';
	SetText(mText);
	MoveCursor(1);
}

// 0x46f390: the Delete key
void ObjString::DeleteAtCursor()
{
	if (mText == nullptr || mLength <= 0)
		return;
	int i = mCursor;
	if (i >= (int)strlen(mText))
		return;
	for (; i < mLength - 1; i++)
		mText[i] = mText[i + 1];
	mText[mLength - 1] = '\0';
	SetText(mText);
	MoveCursor(0);
}

// 0x46f420: the Backspace key
void ObjString::DeleteBeforeCursor()
{
	if (mText == nullptr || mCursor <= 0)
		return;
	int i = mCursor - 1;
	if (i < 0 || (int)strlen(mText) < i)
		return;
	for (; i < mLength - 1; i++)
		mText[i] = mText[i + 1];
	mText[mLength - 1] = '\0';
	SetText(mText);
	MoveCursor(-1);
}

// 0x46f4b0: the spaces and control characters at both ends go; the cursor goes to the start if any did
void ObjString::Trim()
{
	char aTrimmed[128];
	memset(aTrimmed, 0, sizeof(aTrimmed));
	int aLength = (int)strlen(mText);
	if (aLength <= 0)
		return;
	int aStart = 0;
	for (int i = 0; i < aLength; i++) {
		aStart = i;
		if (' ' < mText[i])
			break;
		aStart = 0;
	}
	if (aStart < aLength)
		memcpy(aTrimmed, mText + aStart, aLength - aStart);
	while (0 < aLength - 1 && aTrimmed[aLength - 1] < '!') {
		aTrimmed[aLength - 1] = '\0';
		aLength--;
	}
	if (strlen(aTrimmed) != strlen(mText))
		SetCursor(0);
	SetText(aTrimmed);
}
