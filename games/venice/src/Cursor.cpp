// The cursor object (0x4219d0-0x421c50).

#include "Cursor.h"

#include "ImageRecord.h"
#include "Level.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"

namespace
{
const float CursorZOrder = 11.0f;

bool gSystemCursorHidden = false; // 0x5ba444
} // namespace

Cursor::Cursor()
{
	mZOrder = CursorZOrder;
	HideSystemCursor();
	mTopLeft = 0;
	mType = ObjectTypeCursor;
}

void Cursor::Load(const char* theText, SceneTag* theTag)
{
	mTopLeft = theTag->GetInt("top_left");
	SceneObject::Load(theText, theTag);
}

bool Cursor::Update()
{
	bool aResult = SceneObject::Update();
	mWorld[AttributeX] = gActiveLevel->mMouseX;
	mWorld[AttributeY] = gActiveLevel->mMouseY;
	mLocal[AttributeX] = GetLocalX(mWorld[AttributeX], mWorld[AttributeY]);
	mLocal[AttributeY] = GetLocalY(mWorld[AttributeX], mWorld[AttributeY]);
	return aResult;
}

void Cursor::DrawSelf()
{
	bool aMouseInside = IsMouseInWindow();
	if (mParent->mNext == nullptr && aMouseInside && gHasFocus) {
		if (!gSystemCursorHidden) {
			gSystemCursorHidden = true;
			HideSystemCursor();
		}
		float aHalfWidth = mImage == nullptr ? 0.0f : ((float)mTopLeft * mImage->mFrameWidth) / 2.0f;
		mWorld[AttributeX] = (float)gMouseX + aHalfWidth;
		float aHalfHeight = mImage == nullptr ? 0.0f : ((float)mTopLeft * mImage->mFrameHeight) / 2.0f;
		mWorld[AttributeY] = (float)gMouseY + aHalfHeight;
		SceneObject::DrawSelf();
	} else if (!aMouseInside) {
		ShowSystemCursor();
		gSystemCursorHidden = false;
	}
}
