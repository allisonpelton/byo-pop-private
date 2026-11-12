// The tooltip (0x437b20-0x438100).

#include "Tooltip.h"

#include "ImageRecord.h"
#include "SceneTag.h"
#include "TextObject.h"

#include <cstring>

Tooltip::Tooltip()
{
	mType = ObjectTypeTooltip;
}

void Tooltip::Load(const char* theText, SceneTag* theTag)
{
	SceneObject::Load(theText, theTag);
	strcpy(mText, theTag->GetString("text"));
	strcpy(mFontName, theTag->GetString("font"));
	mLineWidth = theTag->GetInt("line_width");
	mJustification = JustifyLeft;
	if (theTag->Has("justification")) {
		if (strcmp(theTag->GetString("justification"), "left") == 0)
			mJustification = JustifyLeft;
		if (strcmp(theTag->GetString("justification"), "center") == 0)
			mJustification = JustifyCenter;
		if (strcmp(theTag->GetString("justification"), "right") == 0)
			mJustification = JustifyRight;
	}
}

// The text is cut into lines (CutTextLine) of at most the line width; each line is a text
// object, as far down as the font is tall below the one before, the first one a line below the top of the box's picture
void Tooltip::OnObjectClosed()
{
	// the font's line height is the bottom of its letter A
	FontRecord* aFont = AcquireFont(mFontName, nullptr, 0);
	int aLineHeight = (int)aFont->mGlyphs['A'].mBottom;
	float aLineY = 0.0f;
	if (mImage != nullptr)
		aLineY = (0.0f - mImage->mFrameHeight) / 2.0f + (float)aLineHeight;
	int aTextLength = (int)strlen(mText);
	int aLineStart = 0;
	while (aLineStart < aTextLength) {
		char aLine[sizeof(mText)];
		aLineStart = CutTextLine(mText, aLineStart, mLineWidth, aLine);
		TextObject* aLineText = new TextObject();
		NewChild(aLineText);
		aLineText->SetText(aLine, mFontName, -1.0f, 0, mJustification);
		aLineText->AddStage(1000.0f, 1, 1, 1, nullptr);
		if (mImage == nullptr) {
			aLineText->SetAttribute(AttributeX, 0.0f, 0.0f);
		} else {
			float aLineX = (0.0f - mImage->mFrameWidth) / 2.0f + 16.0f;
			aLineText->SetAttribute(AttributeX, aLineX, aLineX);
		}
		aLineText->SetAttribute(AttributeY, aLineY, aLineY);
		aLineY = (float)aLineHeight + aLineY;
	}
}

// It fades in while its parent is hot and out, to nothing (it is hidden below a tenth), when it is not
bool Tooltip::Update()
{
	if (!mParent->IsHot()) {
		mLocal[AttributeAlpha] = mLocal[AttributeAlpha] * 0.98f;
		if (mLocal[AttributeAlpha] < 0.1f) {
			mVisible = 0;
			mStages.mCursor->mVisible = false;
		}
	} else {
		mLocal[AttributeAlpha] = 0.05f > mLocal[AttributeAlpha] ? 0.05f : mLocal[AttributeAlpha];
		float aBrighter = mLocal[AttributeAlpha] * 1.1f;
		mLocal[AttributeAlpha] = aBrighter > 1.0f ? 1.0f : aBrighter;
		mVisible = 1;
		mStages.mCursor->mVisible = true;
	}
	return SceneObject::Update();
}
