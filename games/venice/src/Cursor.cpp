// The cursor object (0x4219d0-0x421c50).

#include "Cursor.h"

#include "ImageRecord.h"
#include "Level.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"

#ifdef __EMSCRIPTEN__
#include "DDImage.h"
#include "MemoryImage.h"
#include "VeniceApp.h"
#include "WebCursor.h"

#include <algorithm>
#endif

namespace
{
const float CursorZOrder = 11.0f;

bool gSystemCursorHidden = false; // 0x5ba444

#ifdef __EMSCRIPTEN__
// port(site): the cursor's picture as the site's cursor set's pointer (framework/WebCursor.h, WebCursorSetKind; docs/WEB_PLAYER.md,
// "The site's cursor"; the user, 2026-11-19: "Venice is missing its custom cursor"). The game hides the system's cursor and draws
// its own as a scene object, so the framework holds no image of it. The first cursor object loaded reads its picture's first
// frame again from the file into an image of the site's own: the framework puts the mouse at an image's middle, so a picture
// drawn with its top left at the pointer (top_left) sits in the bottom right quarter of an image twice its size. What the game
// draws is unchanged; natively nothing here is built.
Sexy::MemoryImage* gSiteCursor = nullptr;

void GiveSiteCursor(const ImageRecord* theRecord, int theTopLeft)
{
	if (gSiteCursor != nullptr || theRecord == nullptr || theRecord->mName[0] == '\0')
		return;
	Sexy::DDImage* aPicture = gApp->GetImage(theRecord->mName, false);
	if (aPicture == nullptr)
		return;
	int aWidth = std::min((int)theRecord->mFrameWidth, aPicture->GetWidth());
	int aHeight = std::min((int)theRecord->mFrameHeight, aPicture->GetHeight());
	int aLeft = theTopLeft != 0 ? aWidth : 0;
	int aTop = theTopLeft != 0 ? aHeight : 0;
	if (aWidth > 0 && aHeight > 0) {
		gSiteCursor = new Sexy::MemoryImage(gApp);
		gSiteCursor->Create(aWidth + aLeft, aHeight + aTop);
		ulong* aTo = gSiteCursor->GetBits();
		const ulong* aFrom = aPicture->GetBits();
		std::fill(aTo, aTo + (aWidth + aLeft) * (aHeight + aTop), 0);
		for (int y = 0; y < aHeight; y++)
			std::copy(aFrom + y * aPicture->GetWidth(), aFrom + y * aPicture->GetWidth() + aWidth,
				aTo + (y + aTop) * (aWidth + aLeft) + aLeft);
		gSiteCursor->BitsChanged();
		Sexy::WebCursorSetKind(Sexy::CURSOR_POINTER, gSiteCursor);
	}
	delete aPicture;
}
#endif
} // namespace

#ifdef __EMSCRIPTEN__
void TakeSiteCursor()
{
	Sexy::WebCursorSetKind(Sexy::CURSOR_POINTER, nullptr);
	delete gSiteCursor;
	gSiteCursor = nullptr;
}
#endif

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
#ifdef __EMSCRIPTEN__
	GiveSiteCursor(mImage, mTopLeft);
#endif
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
