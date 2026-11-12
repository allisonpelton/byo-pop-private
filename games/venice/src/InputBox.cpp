// The name box (0x42cdc0-0x42dc60).

#include "InputBox.h"

#include "GameText.h"
#include "Level.h"
#include "TextObject.h"
#include "VeniceDraw.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"
#include "VeniceMath.h"

#include <cstdio>
#include <cstring>

InputBox::InputBox()
{
	mType = ObjectTypeInputBox;
}

// 0x42cec0, with the setup 0x42cf70 that it calls
void InputBox::Load(const char* theText, SceneTag* theTag)
{
	int aHeight = theTag->GetInt("height");
	int aWidth = theTag->GetInt("width");
	const char* aFontName = theTag->GetString("font");
	const char* anInitialText = TranslateText(theTag->GetString("initial_text"));
	// the caption is "none" in every data file, so that the box is the plain one
	mNoLines = 0;
	ClearKeyRing();
	if (aWidth == 0 || aHeight == 0) {
		mWidth = 100;
		mHeight = 16;
	} else {
		mWidth = aWidth;
		mHeight = aHeight;
	}
	if (aFontName[0] == '\0')
		strcpy(mFontName, "gfx\\sysfont.png");
	else
		strcpy(mFontName, aFontName);
	mLocal[AttributeY] = -5.0f;
	if (anInitialText != nullptr && strlen(anInitialText) != 0)
		strcpy(mText, anInitialText);
	mNoLines = theTag->GetInt("no_lines");
	mBoxOffset = theTag->GetInt("box_offset");
	SceneObject::Load(theText, theTag);
}

// 0x42d1b0: the latest key goes into the text (a backspace takes the last letter off), unless the letter would not fit
bool InputBox::Update()
{
	int aKey = PopKeyRing() & 0xff;
	FontRecord* aFont = AcquireFont(mFontName, nullptr, 0);
	if (aKey != 0) {
		if (IsTypingKey(aKey)) {
			aKey = KeyToCharacter(aKey);
			if (IsKeyDown(KeyCodeShift))
				aKey = ShiftedCharacter(aKey);
		} else if (aKey == 1 || aKey == 2 || aKey == 0x27 || aKey == 0x26 || aKey == 0x28)
			aKey = 0;
		else if (aKey == KeyCodeBackspace)
			aKey = KeyCodeBackspace;
		else if (aKey == 0x22)
			aKey = 0x60;
		else
			aKey = 0;
	}
	if (aKey == KeyCodeBackspace) {
		size_t aLength = strlen(mText);
		if (aLength != 0)
			mText[aLength - 1] = '\0';
	} else if (aKey != 0 && aFont->mGlyphs[aKey].mBottom != 0.0f) {
		char aKeyString[4] = {(char)aKey, 0};
		if (!mTyped && strlen(mText) == 0) {
			aKeyString[0] = (char)toupper((unsigned char)aKeyString[0]);
			mTyped = true;
		}
		char aNewText[64];
		snprintf(aNewText, sizeof(aNewText), "%s%s", mText, aKeyString);
		if (((double)mWidth + (double)mWidth) - 4.0 > aFont->MeasureWidth(aNewText, 1.0f, 0.0f))
			strcpy(mText, aNewText);
	}
	SceneObject::Update();
	return true;
}

// 0x42d520: not drawn while the screen it is on is scaled (as it opens and closes)
void InputBox::Draw()
{
	if (VeniceAbs(mParent->mWorld[AttributeXScale] - 1.0f) > (double)0.01f)
		return;
	if (mNoLines == 0) {
		int aBottom = mBoxOffset + (int)GetWorldY(mLocal[AttributeX], mLocal[AttributeY]) - 2 + mHeight;
		int aRight = (int)GetWorldX(mLocal[AttributeX], mLocal[AttributeY]) + 2 + mWidth;
		int aTop = (int)GetWorldY(mLocal[AttributeX], mLocal[AttributeY]) - mHeight;
		int aLeft = (int)GetWorldX(mLocal[AttributeX], (float)mBoxOffset + mLocal[AttributeY]) - (mWidth + 2);
		DrawBox(aLeft, aTop, aRight, aBottom, 0.0f, 0.0f, 0.0f, 0.3f, 1.0f, 1.0f, 1.0f, 0.5f);
	}
	char aShown[300];
	// the bar blinks: shown for the second half of every 200 updates
	if ((int)gActiveLevel->mTickCount % 200 < 101)
		snprintf(aShown, sizeof(aShown), "%s", mText);
	else
		snprintf(aShown, sizeof(aShown), "%s|", mText);
	DrawTextOnObject(this, aShown, mFontName, 0.0f - ((float)mWidth - 2.0f), 1.0f, 0.0f, JustifyLeft, 1.0f, 1.0f);
}
