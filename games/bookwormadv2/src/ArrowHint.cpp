#include "ArrowHint.h"

#include <string>
#include <utility>

#include "Board.h"
#include "CurtainTransition.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "LuaBind.h"
#include "LuaImage.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;
using namespace LuaLib;

namespace {

// The screen the hints are kept on (the original wrote 800 and 600 where it needed them)
const int kScreenWidth = 800;
const int kScreenHeight = 600;

// 0x40b2a0: the click counts of the left button (a click and a double click)
bool IsLeftClick(int theClickCount)
{
	return theClickCount == 1 || theClickCount == 2;
}

// What the bubble's lines show of a text: its characters outside ^colour^ tags
int VisibleChars(const std::string& theText)
{
	bool aInTag = false;
	int aCount = 0;
	for (size_t i = 0; i < theText.size(); i++) {
		if (theText[i] == '^')
			aInTag = !aInTag;
		else if (!aInTag)
			aCount++;
	}
	return aCount;
}

std::string StripColorTags(const std::string& theText)
{
	bool aInTag = false;
	std::string aResult;
	for (size_t i = 0; i < theText.size(); i++) {
		if (theText[i] == '^')
			aInTag = !aInTag;
		else if (!aInTag)
			aResult += theText[i];
	}
	return aResult;
}

// 0x6a7810 (AddText's call): cuts a text at its '#' marks into pieces, the pieces between a pair of marks being
// flagged. The piece after an odd last mark is dropped (and the result is false).
bool SplitHighlights(const std::string& theText, std::vector<std::pair<std::string, bool> >& thePieces)
{
	size_t aStart = 0;
	bool aInHighlight = false;
	for (size_t i = 0; i < theText.size(); i++) {
		if (theText[i] != '#')
			continue;
		std::string aPiece = theText.substr(aStart, i - aStart);
		aStart = i + 1;
		if (!aPiece.empty())
			thePieces.push_back(std::make_pair(aPiece, aInHighlight));
		aInHighlight = !aInHighlight;
	}
	if (aInHighlight)
		return false;
	std::string aTail = theText.substr(aStart);
	if (!aTail.empty())
		thePieces.push_back(std::make_pair(aTail, false));
	return true;
}

// The picture of the arrow that points the way theDir says, and its glow (null for no such way)
Image* ArrowImage(int theDir)
{
	switch (theDir) {
	case ArrowHint::DIR_LEFT: return IMAGE_HINT_ARROW_LEFT;
	case ArrowHint::DIR_RIGHT: return IMAGE_HINT_ARROW_RIGHT;
	case ArrowHint::DIR_UP: return IMAGE_HINT_ARROW_UP;
	case ArrowHint::DIR_DOWN: return IMAGE_HINT_ARROW_DOWN;
	}
	return nullptr;
}

Image* ArrowGlowImage(int theDir)
{
	switch (theDir) {
	case ArrowHint::DIR_LEFT: return IMAGE_HINT_ARROW_LEFTX;
	case ArrowHint::DIR_RIGHT: return IMAGE_HINT_ARROW_RIGHTX;
	case ArrowHint::DIR_UP: return IMAGE_HINT_ARROW_UPX;
	case ArrowHint::DIR_DOWN: return IMAGE_HINT_ARROW_DOWNX;
	}
	return nullptr;
}

}  // namespace

// ======== ArrowHint ========

// The statics start at zero, except the second value's step (0x8186b4)
int ArrowHint::sPulse = 0;
int ArrowHint::sPulseDelta = 0;
int ArrowHint::sPulseMin = 0;
int ArrowHint::sPulseMax = 0;
int ArrowHint::sPulseUpdate = 0;
int ArrowHint::sFlash = 0;
int ArrowHint::sFlashDelta = 6;

ArrowHint::ArrowHint(ArrowHintManager* theManager, int theId) : mManager(theManager), mId(theId)
{
	RegisterLua();
	// the constants the scripts' ArrowHint gets when it is linked
	mEnums["TimeExpired"] = CALLBACK_TIME_EXPIRED;
	mEnums["ClickedHint"] = CALLBACK_CLICKED_HINT;
	mEnums["ClickedHotSpot"] = CALLBACK_CLICKED_HOT_SPOT;
	mEnums["ClickedScreen"] = CALLBACK_CLICKED_SCREEN;
	mEnums["DirLeft"] = DIR_LEFT;
	mEnums["DirRight"] = DIR_RIGHT;
	mEnums["DirUp"] = DIR_UP;
	mEnums["DirDown"] = DIR_DOWN;
	SetState(STATE_SHOWING);
}

ArrowHint::~ArrowHint()
{
	// the rectangles it blocked are free again
	if (mBlockMouse && mManager->mMouseWidget != nullptr) {
		for (size_t i = 0; i < mBlockedRects.size(); i++)
			mManager->mMouseWidget->RemoveRect(mBlockedRects[i]);
	}
}

// The original registered the class in the constructor (0x402b90), every time
void ArrowHint::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Class<ArrowHint>("ArrowHint", false)
		.Method<&ArrowHint::AddText>("AddText")
		.Method<&ArrowHint::SetFont>("SetFont")
		.Method<&ArrowHint::SetCallback>("SetCallback")
		.Method<&ArrowHint::SetPulseValues>("SetPulseValues")
		.Method<&ArrowHint::SetDir>("SetDir")
		.Method<&ArrowHint::SetHotSpot>("SetHotSpot")
		.Method<&ArrowHint::AddOverSpot>("AddOverSpot")
		.Method<&ArrowHint::SetExpirationTime>("SetExpirationTime")
		.Method<&ArrowHint::SetShortText>("SetShortText")
		.Method<&ArrowHint::GetId>("GetId")
		.Method<&ArrowHint::ClearText>("ClearText")
		.Method<&ArrowHint::DrawWhenDialogs>("DrawWhenDialogs")
		.Method<&ArrowHint::BlockMouseEvents>("BlockMouseEvents")
		.Method<&ArrowHint::SetXY>("SetXY");
}

// ---- the state machine ----

// 0x6acbb0: the state, with its entry function (0x4035b0, none, 0x4035d0, 0x403590)
void ArrowHint::SetState(int theState)
{
	mState = theState;
	EnterState(theState);
}

void ArrowHint::EnterState(int theState)
{
	switch (theState) {
	case STATE_FADE_IN:  // 0x4035b0
		mActive = true;
		mPulseCount = 0;
		mAlpha = 0;
		mAlphaDelta = 10;
		break;
	case STATE_SHOWING:  // 0x4035d0
		mActive = true;
		mExpiration = 0;
		break;
	case STATE_FADE_OUT:  // 0x403590
		mActive = true;
		mFadeDelta = -8;
		mFadeAlpha = 255;
		break;
	}
}

// 0x6acc40, with the machine's transitions (0 to 1, 1 to 2, 1 to 3 and 2 to 3)
bool ArrowHint::ChangeState(int theState)
{
	bool anAllowed = (mState == STATE_FADE_IN && theState == STATE_PULSE_IN) ||
	                 (mState == STATE_PULSE_IN && (theState == STATE_SHOWING || theState == STATE_FADE_OUT)) ||
	                 (mState == STATE_SHOWING && theState == STATE_FADE_OUT);
	if (!anAllowed)
		return false;
	SetState(theState);
	return true;
}

// The current state's update function; true when the hint is finished
bool ArrowHint::UpdateState()
{
	switch (mState) {
	case STATE_FADE_IN:  // 0x403430
		mAlpha += mAlphaDelta;
		if (mAlpha >= 128)
			ChangeState(STATE_PULSE_IN);
		return false;
	case STATE_PULSE_IN:  // 0x403460: the bubble runs between 128 and 255 twice
		mAlpha += mAlphaDelta;
		if (mAlphaDelta > 0) {
			if (mAlpha >= 255) {
				mPulseCount++;
				mAlpha = 255;
				if (mPulseCount >= 2) {
					ChangeState(STATE_SHOWING);
					return false;
				}
				mAlphaDelta = -mAlphaDelta;
			}
		} else if (mAlphaDelta < 0) {
			if (mAlpha <= 128) {
				mPulseCount++;
				mAlphaDelta = -mAlphaDelta;
			}
		}
		return false;
	case STATE_SHOWING: {  // 0x4034d0: the shared pulse moves once an update, whichever hint asks first
		int anUpdate = gApp->mUpdateCount;
		if (sPulseUpdate >= anUpdate)
			return false;
		sPulse += sPulseDelta;
		if (sPulseDelta < 0 ? sPulse <= sPulseMin : (sPulseDelta > 0 && sPulse >= sPulseMax)) {
			sPulse = sPulseDelta < 0 ? sPulseMin : sPulseMax;
			sPulseDelta = -sPulseDelta;
		}
		sFlash += sFlashDelta;
		if (sFlashDelta < 0) {
			if (sFlash <= 0) {
				sFlashDelta = -sFlashDelta;
				sPulseUpdate = anUpdate;
				sFlash = 0;
				return false;
			}
		} else if (sFlashDelta > 0) {
			if (sFlash >= 255) {
				sFlash = 255;
				sFlashDelta = -sFlashDelta;
			}
		}
		sPulseUpdate = anUpdate;
		return false;
	}
	case STATE_FADE_OUT:  // 0x4033f0: fades out where there is alpha blending, else it is gone at once
		mFadeAlpha += mFadeDelta;
		if (mFadeAlpha > 0 && gApp->Is3DAccelerated())
			return false;
		mFadeAlpha = 0;
		return true;
	}
	return false;
}

// ---- the manager's calls ----

bool ArrowHint::Update()
{
	if (UpdateState())
		return true;
	if (mExpiration > 0) {
		mExpiration--;
		if (mExpiration < 1) {
			if (!mCallback.empty()) {
				// callback(TimeExpired, id, hint)
				lua_State* L = gLuaSupport->L;
				lua_pushint(L, CALLBACK_TIME_EXPIRED);
				lua_pushint(L, mId);
				gLuaSupport->PushObject(ClassOf<ArrowHint>::info, this);
				CallLua(mCallback, 3);
			}
			ChangeState(STATE_FADE_OUT);
		}
	}
	return false;
}

void ArrowHint::CallCallback(int theReason, bool theLeft, int theSpot)
{
	lua_State* L = gLuaSupport->L;
	lua_pushint(L, theReason);
	lua_pushint(L, mId);
	gLuaSupport->PushObject(ClassOf<ArrowHint>::info, this);
	lua_pushboolean(L, theLeft);
	lua_pushint(L, theSpot);
	CallLua(mCallback, 5);
}

bool ArrowHint::Click(int x, int y, bool theLeft)
{
	if (mState != STATE_SHOWING)
		return false;
	for (size_t i = 0; i < mHotSpots.size(); i++) {
		const Rect& aSpot = mHotSpots[i];
		// the lines' bubble is a hot spot only while the lines are up
		if (!mShowLong && aSpot == mRectLong)
			continue;
		if (!aSpot.Contains(x, y))
			continue;
		if (!mCallback.empty()) {
			int aReason = CALLBACK_CLICKED_HOT_SPOT;
			if (aSpot == mRectLong || aSpot == mRectShort)
				aReason = CALLBACK_CLICKED_HINT;
			else if (aSpot.mX == 0 && aSpot.mY == 0 && aSpot.mWidth == kScreenWidth && aSpot.mHeight == kScreenHeight)
				aReason = CALLBACK_CLICKED_SCREEN;
			CallCallback(aReason, theLeft, (int)i);
		}
		ChangeState(STATE_FADE_OUT);
		return true;
	}
	return false;
}

bool ArrowHint::UpdateHover(int x, int y)
{
	if (mState == STATE_FADE_OUT)
		return mShowLong;
	const Rect& aRect = mShowLong ? mRectLong : mRectShort;
	if (!aRect.Contains(x, y) && !InOverSpot(x, y)) {
		mShowLong = false;
		return false;
	}
	mShowLong = true;
	return true;
}

bool ArrowHint::HitTest(int x, int y)
{
	Rect aRect = mShowLong ? mRectLong : mRectShort;
	// the arrow's side. port: as the original, the bubble moves left and up by the arrow's size (instead of
	// growing by it) at the left and the top
	switch (mDir) {
	case DIR_LEFT: aRect.mX -= IMAGE_HINT_ARROW_LEFT->GetCelWidth(); break;
	case DIR_RIGHT: aRect.mWidth += IMAGE_HINT_ARROW_RIGHT->GetCelWidth(); break;
	case DIR_UP: aRect.mY -= IMAGE_HINT_ARROW_UP->GetCelHeight(); break;
	case DIR_DOWN: aRect.mHeight += IMAGE_HINT_ARROW_DOWN->GetCelHeight(); break;
	}
	if (aRect.Contains(x, y))
		return true;
	return InOverSpot(x, y);
}

bool ArrowHint::InOverSpot(int x, int y)
{
	for (size_t i = 0; i < mOverSpots.size(); i++) {
		if (mOverSpots[i].Contains(x, y))
			return true;
	}
	return false;
}

Rect ArrowHint::GetArrowRect()
{
	Image* anArrow = ArrowImage(mDir);
	return Rect(mX, mY, anArrow->GetCelWidth(), anArrow->GetCelHeight());
}

// 0x4036c0: the bubble (a box of pictures) with a glow over it, its title picture, the short text in the pulse's
// red with the hint to mouse over, or the lines; then the arrow with its glow
void ArrowHint::Draw(Graphics* g)
{
	// port: the original drew with whatever font it had, and no font crashed it
	if (mFont == nullptr)
		return;
	bool aHasLines = !mLines.empty();
	bool aLinesUp = (mShowLong || mShortText.empty()) && aHasLines;
	const Rect* aRect = (mShowLong && aHasLines) ? &mRectLong : &mRectShort;
	g->PushState();
	int anAlpha = mAlpha != 255 ? mAlpha : mFadeAlpha;
	if (anAlpha != 255) {
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->SetColorizeImages(true);
	}
	g->DrawImageBox(*aRect, IMAGE_HINT_ARROW_BOX);
	g->PushState();
	g->SetColorizeImages(true);
	g->SetColor(Color(sPulse, sPulse, sPulse, anAlpha));
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->DrawImageBox(Rect(aRect->mX - 5, aRect->mY - 5, aRect->mWidth + 10, aRect->mHeight + 10),
	                IMAGE_HINT_ARROW_GLOW_ADD);
	g->PopState();
	g->DrawImage(IMAGE_HINT_ARROW_TITLE, (aRect->mWidth - IMAGE_HINT_ARROW_TITLE->mWidth) / 2 + aRect->mX,
	             aRect->mY);
	int aPad = IMAGE_HINT_ARROW_BOX->mWidth / 3;
	int aY = mFont->GetHeight() + IMAGE_HINT_ARROW_TITLE->mHeight;
	g->PopState();
	g->SetFont(mFont);
	if (aLinesUp) {
		g->SetColor(Color(255, 255, 255, anAlpha));
		for (size_t i = 0; i < mLines.size(); i++) {
			g->WriteString(mLines[i], aRect->mX + aPad, aRect->mY + aY, mRectLong.mWidth - aPad * 2, 0, true, 0, -1,
			               -1);
			aY += mFont->GetHeight();
		}
	} else {
		g->SetColor(Color(sPulse, 0, 0, anAlpha));
		g->WriteString(mShortText, aRect->mX + aPad, aRect->mY + aY, mRectShort.mWidth - aPad * 2, 0, true, 0, -1, -1);
		if (aHasLines) {
			g->SetColor(Color(255, 255, 255, anAlpha));
			g->WriteString(GT(0x7a98f8), aRect->mX + aPad, aRect->mY + aY + mFont->GetHeight(),
			               mRectShort.mWidth - aPad * 2, 0, true, 0, -1, -1);
		}
	}
	// the arrow, and its glow over it. port: no arrow for no direction (the original used whatever was in a
	// register)
	Image* anArrow = ArrowImage(mDir);
	Image* aGlow = ArrowGlowImage(mDir);
	if (anArrow == nullptr)
		return;
	g->PushState();
	if (anAlpha != 255) {
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->SetColorizeImages(true);
	}
	g->DrawImage(anArrow, mX, mY);
	g->PopState();
	g->PushState();
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	if (anAlpha != 255)
		g->SetColor(Color(sPulse, sPulse, sPulse, anAlpha));
	else
		g->SetColor(Color(sPulse, sPulse, sPulse));
	g->DrawImage(aGlow, mX, mY);
	g->PopState();
}

// ---- the scripts' methods ----

// 0x403c60: a line of text (^colour^ tags for the text between '#' marks), then both bubbles are fitted
void ArrowHint::AddText(std::string theText)
{
	std::vector<std::pair<std::string, bool> > aPieces;
	SplitHighlights(theText, aPieces);
	std::string aColored;
	for (size_t i = 0; i < aPieces.size(); i++) {
		aColored += aPieces[i].second ? "^FF0000^" : "^FFFFFF^";
		aColored += aPieces[i].first;
	}
	mLines.push_back(aColored);
	// the width of the line with the most characters shown
	int aMostChars = 0;
	std::string aLongest;
	for (size_t i = 0; i < mLines.size(); i++) {
		int aChars = VisibleChars(mLines[i]);
		if (aChars > aMostChars) {
			aMostChars = aChars;
			aLongest = mLines[i];
		}
	}
	std::string aPlain = StripColorTags(aLongest);
	int aPad = IMAGE_HINT_ARROW_BOX->mWidth / 3;
	// port: the original used the font without looking whether the script had set one
	mRectLong.mWidth = (mFont != nullptr ? mFont->StringWidth(aPlain) : 0) + aPad * 2;
	if (mRectLong.mWidth < IMAGE_HINT_ARROW_TITLE->mWidth)
		mRectLong.mWidth = IMAGE_HINT_ARROW_TITLE->mWidth;
	mRectLong.mHeight =
		(mFont != nullptr ? mFont->GetHeight() : 0) * ((int)mLines.size() + 1) + IMAGE_HINT_ARROW_TITLE->mHeight;
	FitToScreen();
}

// 0x4033b0
void ArrowHint::SetFont(LuaFont* theFont)
{
	mLuaFont = theFont;
	mFont = FontOf(theFont);
}

void ArrowHint::SetPulseValues(int theDelta, int theMin, int theMax, int theStart)
{
	sPulse = theStart;
	sPulseDelta = theDelta;
	sPulseMin = theMin;
	sPulseMax = theMax;
}

void ArrowHint::SetDir(int theDir)
{
	mDir = theDir;
	FitToScreen();
}

void ArrowHint::SetHotSpot(int x, int y, int w, int h)
{
	if (x == -1 && y == -1 && w == -1 && h == -1) {
		// both bubbles
		mHotSpots.push_back(mRectShort);
		x = mRectLong.mX;
		y = mRectLong.mY;
		w = mRectLong.mWidth;
		h = mRectLong.mHeight;
	} else if (x == 0 && y == 0 && w == 0 && h == 0) {
		// the whole screen
		w = kScreenWidth;
		h = kScreenHeight;
	}
	mHotSpots.push_back(Rect(x, y, w, h));
}

void ArrowHint::AddOverSpot(int x, int y, int w, int h)
{
	mOverSpots.push_back(Rect(x, y, w, h));
}

// 0x403b80: the short text, and a bubble wide enough for it or for the hint to mouse over
void ArrowHint::SetShortText(std::string theText)
{
	mShortText = theText;
	// port: the original used the font without looking whether the script had set one
	int aHintWidth = mFont != nullptr ? mFont->StringWidth(GT(0x7a98f8)) : 0;
	int aTextWidth = mFont != nullptr ? mFont->StringWidth(theText) : 0;
	if (aHintWidth <= aTextWidth)
		aHintWidth = aTextWidth;
	mRectShort.mWidth = aHintWidth + (IMAGE_HINT_ARROW_BOX->mWidth / 3) * 2;
	mRectShort.mHeight = (mFont != nullptr ? mFont->GetHeight() : 0) * 3 + IMAGE_HINT_ARROW_TITLE->mHeight;
}

void ArrowHint::ClearText()
{
	mLines.clear();
	mShortText.clear();
}

// 0x404c00: the mouse widget blocks the clicks on the short bubble, the arrow, the lines' bubble and the over spots
void ArrowHint::BlockMouseEvents()
{
	MouseWidget* aMouseWidget = mManager->mMouseWidget;
	mBlockMouse = true;
	Rect aRects[3] = {mRectShort, GetArrowRect(), mRectLong};
	for (int i = 0; i < 3; i++) {
		if (aMouseWidget->AddRect(aRects[i]))
			mBlockedRects.push_back(aRects[i]);
	}
	for (size_t i = 0; i < mOverSpots.size(); i++) {
		if (aMouseWidget->AddRect(mOverSpots[i]))
			mBlockedRects.push_back(mOverSpots[i]);
	}
}

// 0x404580
void ArrowHint::SetXY(int x, int y, bool theAnchorToTip)
{
	if (!theAnchorToTip) {
		mRectLong.mX = x;
		mRectLong.mY = y;
		FitToScreen();
		return;
	}
	if (mShortText.empty())
		mRectShort = mRectLong;
	// the arrow's picture sits at the tip, and the bubbles beside it (centred on it), overlapping it a little
	switch (mDir) {
	case DIR_LEFT: {
		Image* anArrow = IMAGE_HINT_ARROW_LEFT;
		mX = x;
		mY = y - anArrow->mHeight / 2;
		int aBubbleX = anArrow->mWidth + x - 5;
		mRectShort.mX = aBubbleX;
		mRectShort.mY = mY - (mRectShort.mHeight - anArrow->mHeight) / 2;
		mRectLong.mX = aBubbleX;
		mRectLong.mY = mY - (mRectLong.mHeight - anArrow->mHeight) / 2;
		break;
	}
	case DIR_RIGHT: {
		Image* anArrow = IMAGE_HINT_ARROW_RIGHT;
		mX = x - anArrow->mWidth;
		mY = y - anArrow->mHeight / 2;
		mRectShort.mX = mX - mRectShort.mWidth + 5;
		mRectShort.mY = mY - (mRectShort.mHeight - anArrow->mHeight) / 2;
		mRectLong.mX = mX - mRectLong.mWidth + 5;
		mRectLong.mY = mY - (mRectLong.mHeight - anArrow->mHeight) / 2;
		break;
	}
	case DIR_UP: {
		Image* anArrow = IMAGE_HINT_ARROW_UP;
		mX = x - anArrow->mWidth / 2;
		mY = y;
		mRectShort.mX = mX - (mRectShort.mWidth - anArrow->mWidth) / 2;
		mRectShort.mY = anArrow->mHeight + y - 4;
		mRectLong.mX = mX - (mRectLong.mWidth - anArrow->mWidth) / 2;
		mRectLong.mY = anArrow->mHeight + y - 4;
		break;
	}
	case DIR_DOWN: {
		Image* anArrow = IMAGE_HINT_ARROW_DOWN;
		mX = x - anArrow->mWidth / 2;
		mY = y;
		mRectShort.mX = mX - (mRectShort.mWidth - anArrow->mWidth) / 2;
		mRectShort.mY = y - mRectShort.mHeight + 5;
		mRectLong.mX = mX - (mRectLong.mWidth - anArrow->mWidth) / 2;
		mRectLong.mY = y - mRectLong.mHeight + 5;
		break;
	}
	default:
		// port: the original stored whatever was in its registers; there is no arrow to place the bubbles by
		return;
	}
	FitToScreen();
}

// 0x4047d0: each bubble (with the arrow's room at its side, for a right or down arrow) kept on the screen; the
// lines' bubble is at least as big as the short one
void ArrowHint::FitToScreen()
{
	Rect* aRects[2] = {&mRectLong, &mRectShort};
	for (int i = 0; i < 2; i++) {
		Rect* aRect = aRects[i];
		int x = aRect->mX;
		int y = aRect->mY;
		int aWidth = aRect->mWidth + (mDir == DIR_RIGHT ? IMAGE_HINT_ARROW_RIGHT->mWidth : 0);
		int aHeight = aRect->mHeight + (mDir == DIR_DOWN ? IMAGE_HINT_ARROW_DOWN->mHeight : 0);
		if (x + aWidth > kScreenWidth)
			x = kScreenWidth - aWidth;
		else if (x - (mDir == DIR_LEFT ? IMAGE_HINT_ARROW_LEFT->mWidth : 0) < 0)
			x = 0;
		if (y + aHeight > kScreenHeight)
			y = kScreenHeight - aHeight;
		else if (y - (mDir == DIR_UP ? IMAGE_HINT_ARROW_UP->mHeight : 0) < 0)
			y = 0;
		aRect->mX = x;
		aRect->mY = y;
	}
	if (mRectLong.mWidth < mRectShort.mWidth) {
		mRectLong.mX = mRectShort.mX;
		mRectLong.mWidth = mRectShort.mWidth;
	}
	if (mRectLong.mHeight < mRectShort.mHeight) {
		mRectLong.mY = mRectShort.mY;
		mRectLong.mHeight = mRectShort.mHeight;
	}
}

// ======== MouseWidget ========

MouseWidget::MouseWidget(ArrowHintManager* theManager) : mManager(theManager)
{
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_UPDATE | WIDGETFLAGS_DRAW;
	mZOrder = 100;
}

bool MouseWidget::AddRect(const Rect& theRect)
{
	for (size_t i = 0; i < mRects.size(); i++) {
		if (mRects[i] == theRect)
			return false;
	}
	mRects.push_back(theRect);
	return true;
}

void MouseWidget::RemoveRect(const Rect& theRect)
{
	for (size_t i = 0; i < mRects.size(); i++) {
		if (mRects[i] == theRect) {
			mRects.erase(mRects.begin() + i);
			return;
		}
	}
}

bool MouseWidget::IntersectsRects(const Rect& theRect) const
{
	for (size_t i = 0; i < mRects.size(); i++) {
		if (theRect.Intersects(mRects[i]))
			return true;
	}
	return false;
}

// The clicks are the manager's, unless the board is paused or nothing is blocked
void MouseWidget::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	if ((gApp->mBoard == nullptr || gApp->mBoard->mPauseCount < 1) && !mRects.empty())
		mManager->Click(x, y, theClickCount);
}

void MouseWidget::MouseMove(int x, int y)
{
	if ((gApp->mBoard == nullptr || gApp->mBoard->mPauseCount < 1) && !mRects.empty())
		mManager->MouseMove(x, y);
}

// Only the blocked rectangles are the widget's (the cursor is the pointer over them): nothing while a dialog is
// up, or the board is paused
bool MouseWidget::IsPointVisible(int x, int y)
{
	if (mRects.empty() || gApp->GetDialogCount() > 0)
		return false;
	if (gApp->mBoard != nullptr && gApp->mBoard->mPauseCount > 0)
		return false;
	for (size_t i = 0; i < mRects.size(); i++) {
		if (mRects[i].Contains(x, y)) {
			gApp->SetCursor(CURSOR_POINTER);
			return true;
		}
	}
	return false;
}

// ======== ArrowHintManager ========

bool ArrowHintManager::sWalking = false;

ArrowHintManager::ArrowHintManager(GameApp* theApp) : mApp(theApp)
{
	RegisterLua();
	mHasTransparencies = true;
	Widget::SetDisabled(true);
	Widget::SetVisible(false);
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	mMouseWidget = new MouseWidget(this);
	mMouseWidget->Resize(0, 0, kScreenWidth, kScreenHeight);
}

ArrowHintManager::~ArrowHintManager()
{
	for (size_t i = 0; i < mHints.size(); i++)
		delete mHints[i];
	mHints.clear();
	delete mMouseWidget;
	mMouseWidget = nullptr;
}

// The original registered the class in the constructor (0x40b2c0), every time
void ArrowHintManager::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Class<ArrowHintManager>("ArrowHintManager", false)
		.Base<LuaWidget>()
		.Method<&ArrowHintManager::AddArrowHint>("AddArrowHint")
		.Method<&ArrowHintManager::DeleteArrowHint>("DeleteArrowHint")
		.Method<&ArrowHintManager::GetArrowHint>("GetArrowHint")
		.Method<&ArrowHintManager::GetArrowHintById>("GetArrowHintById")
		.Method<&ArrowHintManager::DeleteAllArrowHints>("DeleteAllArrowHints")
		.Method<&ArrowHintManager::DontDrawArrowHints>("DontDrawArrowHints")
		.Method<&ArrowHintManager::DisableArrowHints>("DisableArrowHints")
		.Method<&ArrowHintManager::SetDrawPriority>("SetDrawPriority");
}

// 0x40b630 and 0x40b600: the mouse widget goes in and out of the widget manager with the manager
void ArrowHintManager::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMouseWidget);
}

void ArrowHintManager::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMouseWidget);
}

// The hints added during a walk join the list; with none the widget sleeps (invisible, disabled)
void ArrowHintManager::EndWalk()
{
	for (size_t i = 0; i < mPending.size(); i++)
		mHints.push_back(mPending[i]);
	mPending.clear();
	if (mHints.empty()) {
		SetDisabled(true);
		SetVisible(false);
	}
}

void ArrowHintManager::Update()
{
	if (mApp->mBoard != nullptr && mApp->mBoard->mPauseCount > 0)
		return;
	if (!mVisible || mDisabled || mHintsDisabled)
		return;
	sWalking = true;
	for (size_t i = 0; i < mHints.size(); i++) {
		if (mHints[i]->Update()) {
			delete mHints[i];
			mHints.erase(mHints.begin() + i);
			i--;
		}
	}
	sWalking = false;
	EndWalk();
}

void ArrowHintManager::Click(int x, int y, int theClickCount)
{
	if (mApp->mBoard != nullptr && mApp->mBoard->mPauseCount > 0)
		return;
	if (mHintsDisabled || mApp->mCredits != nullptr)
		return;
	sWalking = true;
	for (size_t i = 0; i < mHints.size(); i++)
		mHints[i]->Click(x, y, IsLeftClick(theClickCount));
	sWalking = false;
	EndWalk();
}

// Asks for the overlay draw: over the conversation panel it does not (the panel's callers draw the hints), over a
// paused board it does at 9999, over a dialog before everything
void ArrowHintManager::Draw(Graphics* g)
{
	if (mHints.empty())
		return;
	Board* aBoard = mApp->mBoard;
	if (aBoard != nullptr && aBoard->mDrawArrowHintsAboveOverlay)
		return;
	if (mApp->ShowingConversationPanel())
		return;
	if (aBoard != nullptr && aBoard->mPauseCount > 0) {
		DeferOverlay(9999);
		return;
	}
	if (mApp->GetDialogCount() == 0)
		DeferOverlay(mDrawPriority);
	else
		DeferOverlay(-1);
}

void ArrowHintManager::DrawOverlay(Graphics* g)
{
	// not while the curtain moves
	CurtainTransition* aCurtain = mApp->mCurtain;
	if (aCurtain != nullptr && !(aCurtain->mDone && aCurtain->mDrawn))
		return;
	if (mDontDraw || mApp->mCredits != nullptr)
		return;
	for (size_t i = 0; i < mHints.size(); i++) {
		ArrowHint* aHint = mHints[i];
		if ((aHint->mDrawWhenDialogs || mApp->GetDialogCount() == 0) && aHint->mActive)
			aHint->Draw(g);
	}
}

// port: mLastEventUpdate (the widget's own update count, stored) is compared with the app's, as in the original;
// the widget's count stands still while it sleeps
void ArrowHintManager::MouseMove(int x, int y)
{
	if (mLastEventUpdate < mApp->mUpdateCount && !mHintsDisabled) {
		mLastEventUpdate = mUpdateCnt;
		for (size_t i = 0; i < mHints.size(); i++)
			mHints[i]->UpdateHover(x, y);
	}
}

void ArrowHintManager::MouseDown(int x, int y, int theClickCount)
{
	if (mLastEventUpdate < mApp->mUpdateCount) {
		mLastEventUpdate = mUpdateCnt;
		Click(x, y, theClickCount);
	}
}

ArrowHint* ArrowHintManager::AddArrowHint(int theId, bool)
{
	ArrowHint* anExisting = nullptr;
	for (size_t i = 0; i < mHints.size(); i++) {
		if (mHints[i]->mId == theId) {
			anExisting = mHints[i];
			break;
		}
	}
	for (size_t i = 0; i < mPending.size(); i++) {
		if (mPending[i]->mId == theId) {
			anExisting = mPending[i];
			break;
		}
	}
	// (during a walk a hint with a number in use is made all the same)
	if (anExisting != nullptr && !sWalking)
		return nullptr;
	ArrowHint* aHint = new ArrowHint(this, theId);
	aHint->CreateLinkedObject(aHint);
	if (!sWalking)
		mHints.push_back(aHint);
	else
		mPending.push_back(aHint);
	SetVisible(true);
	SetDisabled(false);
	return aHint;
}

void ArrowHintManager::DeleteArrowHint(int theId)
{
	if (sWalking) {
		mApp->MsgBox(GT(0x7a9c0c), GT(0x7a9c04), 0);
	} else {
		for (size_t i = 0; i < mHints.size(); i++) {
			if (mHints[i]->mId == theId) {
				delete mHints[i];
				mHints.erase(mHints.begin() + i);
				break;
			}
		}
	}
	if (mHints.empty()) {
		SetDisabled(true);
		SetVisible(false);
		mDrawPriority = 9998;
	}
}

int ArrowHintManager::GetArrowHint(int x, int y)
{
	for (size_t i = 0; i < mHints.size(); i++) {
		if (mHints[i]->HitTest(x, y))
			return mHints[i]->mId;
	}
	return -32767;
}

ArrowHint* ArrowHintManager::GetArrowHintById(int theId)
{
	for (size_t i = 0; i < mHints.size(); i++) {
		if (mHints[i]->mId == theId)
			return mHints[i];
	}
	return nullptr;
}

// During a walk (a hint's callback calling this) the original says so and forgets the hints without deleting them
void ArrowHintManager::DeleteAllArrowHints()
{
	if (sWalking) {
		mApp->MsgBox(GT(0x7a9c0c), GT(0x7a9c04), 0);
	} else {
		for (size_t i = 0; i < mHints.size(); i++)
			delete mHints[i];
	}
	mHints.clear();
	SetDisabled(true);
	SetVisible(false);
	mDrawPriority = 9998;
	mMouseWidget->mRects.clear();
}
