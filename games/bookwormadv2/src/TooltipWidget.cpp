// Sexy::TooltipWidget (0x48b7e0-V1:0x4a4130) and the app's tooltip methods (V1:0x44bf00-V1:0x44c590, GameApp members: they
// are the other half of how a tooltip shows).
#include "TooltipWidget.h"

#include "ArrowHint.h"
#include "BattleEngine.h"
#include "Board.h"
#include "ConversationPanel.h"
#include "Font.h"
#include "GameApp.h"
#include "Graphics.h"
#include "LuaBind.h"
#include "Profile.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;
using namespace LuaLib;

namespace {

// 0x445660: something is up that the tooltips keep out of: the board has overlays (Lua functions that draw over
// it) or the conversation panel is active
bool OverlayUp()
{
	if (gApp->mBoard != nullptr && gApp->mBoard->HasOverlays())
		return true;
	return gApp->mConversationPanel != nullptr && gApp->mConversationPanel->Active();
}

// V1:0x4a26a0, the raw Lua method: the activation region as four numbers (x, y, width, height)
int LuaGetActivationRegion(TooltipWidget* theTooltip, lua_State* L)
{
	const Rect& aRegion = theTooltip->mActivationRegion;
	lua_pushint(L, aRegion.mX);
	lua_pushint(L, aRegion.mY);
	lua_pushint(L, aRegion.mWidth);
	lua_pushint(L, aRegion.mHeight);
	return 4;
}

}  // namespace

TooltipWidget::TooltipWidget()
	: mBorderColor(0, 0, 0), mTooltipColor(40, 65, 45), mTextColor(215, 255, 10)
{
	RegisterLua();
	// (the original set its own counters, its position and its size to 0 here; they start so)
	mMouseVisible = false;
}

// The original registered the class in the constructor (0x48bb20), every time
void TooltipWidget::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	// port: the overloads of the colours' setters are told apart by their types (the bound ones take four ints)
	Class<TooltipWidget>("TooltipWidget", false)
		.Base<LuaWidget>()
		.Method<&TooltipWidget::AddText>("AddText")
		.Method<&TooltipWidget::ClearText>("ClearText")
		.Method<&TooltipWidget::SetX>("SetX")
		.Method<&TooltipWidget::SetY>("SetY")
		.Method<&TooltipWidget::GetX>("GetX")
		.Method<&TooltipWidget::GetY>("GetY")
		.Method<&TooltipWidget::Hide>("Hide")
		.Method<&TooltipWidget::SetXY>("SetXY")
		.Method<&TooltipWidget::Disable>("Disable")
		.Method<&TooltipWidget::Enable>("Enable")
		.Method<&TooltipWidget::SetPriority>("SetPriority")
		.Method<&TooltipWidget::SetActivationRegion>("SetActivationRegion")
		.Method<&TooltipWidget::SlideActivationRegion>("SlideActivationRegion")
		.Method<static_cast<void (TooltipWidget::*)(int, int, int, int)>(&TooltipWidget::SetBorderColor)>(
			"SetBorderColor")
		.Method<static_cast<void (TooltipWidget::*)(int, int, int, int)>(&TooltipWidget::SetTooltipColor)>(
			"SetTooltipColor")
		.Method<static_cast<void (TooltipWidget::*)(int, int, int, int)>(&TooltipWidget::SetTextColor)>(
			"SetTextColor")
		.Method<&TooltipWidget::SetDisplayDelay>("SetDisplayDelay")
		.Method<&TooltipWidget::SetDisplayTime>("SetDisplayTime")
		.Method<&TooltipWidget::NumTextLines>("NumTextLines")
		.Method<&TooltipWidget::SetAllowOverlayDisplay>("SetAllowOverlayDisplay")
		.Raw<&LuaGetActivationRegion>("GetActivationRegion");
}

// 0x48bf30: counts the delay up once it is shown (Show), then the time it stays
void TooltipWidget::Update()
{
	if (mDisabled)
		return;
	mUpdates++;
	if (mArmed)
		mWaitCount++;
	if (!mShowing) {
		if (mArmed && mDisplayDelay <= mWaitCount) {
			mWaitCount = 0;
			mShowing = true;
		}
	} else if (mDisplayTime > 0 && mDisplayTime <= mWaitCount) {
		mShowing = false;
		mArmed = false;
		mWaitCount = 0;
	}
}

// 0x48bfb0: a border, the box, and the lines in the font
void TooltipWidget::Draw(Graphics* g)
{
	if (OverlayUp() && !mAllowOverlayDisplay)
		return;
	if (!mShowing || !mVisible || mDisabled)
		return;
	if (gApp->mBoard != nullptr && gApp->mBoard->mPauseCount > 0)
		return;
	// not over the board's grid while the battle's script says the grid has an overlay (in the grid's pixels)
	if (gBattleEngine != nullptr && gBattleEngine->CallLinkedMethod("GridHasOverlay", 0).Bool() && mX >= 301 &&
		mX <= 501 && mY >= 309 && mY <= 509)
		return;
	g->SetFont(mFont);
	g->SetColor(mBorderColor);
	g->FillRect(Rect(0, 0, mWidth, mHeight));
	g->SetColor(mTooltipColor);
	g->FillRect(Rect(1, 1, mWidth - 2, mHeight - 2));
	g->SetColor(mTextColor);
	// port: the original dereferenced the font here without looking
	if (mFont == nullptr)
		return;
	for (size_t i = 0; i < mLines.size(); i++)
		g->DrawString(mLines[i], 3, (int)(mFont->GetAscent() * (i + 1) + 2 + i * 2));
}

// 0x48c170 (vf 0xc8): the countdown to appearing starts, unless the player turned the tips off
void TooltipWidget::MouseEnter()
{
	Profile* aProfile = gApp->mProfile;
	if (aProfile != nullptr && aProfile->KeyExists("DisableTips") && aProfile->GetBool("DisableTips"))
		return;
	if (!mArmed) {
		mArmed = true;
		mShowing = false;
	}
}

// 0x48c1f0 (vf 0xcc)
void TooltipWidget::MouseLeave()
{
	if (mArmed) {
		mWaitCount = 0;
		mArmed = false;
		mShowing = false;
	}
}

void TooltipWidget::Disable()
{
	Widget::SetVisible(false);
	Widget::SetDisabled(true);
	Hide();
}

void TooltipWidget::Enable()
{
	if (NumTextLines() > 0) {
		Widget::SetVisible(true);
		Widget::SetDisabled(false);
	}
}

int TooltipWidget::CalcWidth()
{
	if (mFont == nullptr)
		return 0;
	int aWidth = 0;
	for (size_t i = 0; i < mLines.size(); i++) {
		int aLineWidth = mFont->StringWidth(mLines[i]);
		if (aLineWidth > aWidth)
			aWidth = aLineWidth;
	}
	return aWidth + 8;
}

int TooltipWidget::CalcHeight()
{
	if (mFont == nullptr)
		return 0;
	int aLines = (int)mLines.size();
	return mFont->GetAscent() * aLines + 8 + aLines * 2;
}

void TooltipWidget::AddText(std::string theText)
{
	mLines.push_back(theText);
	mWidth = CalcWidth();
	mHeight = CalcHeight();
	SetXY(mX, mY);
}

void TooltipWidget::ClearText()
{
	mLines.clear();
}

void TooltipWidget::SetFont(Font* theFont)
{
	mFont = theFont;
	mWidth = CalcWidth();
	mHeight = CalcHeight();
}

// 0x48c410: the box is kept inside the screen (right and bottom edges first, then the left and top)
void TooltipWidget::SetXY(int x, int y)
{
	int aWidth = mWidth;
	if (x + aWidth > gApp->mWidth)
		x = gApp->mWidth - aWidth;
	else if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	else if (y + mHeight > gApp->mHeight)
		y = gApp->mHeight - mHeight;
	Resize(x, y, aWidth, mHeight);
}

void TooltipWidget::SetActivationRegion(int x, int y, int w, int h)
{
	mActivationRegion = Rect(x, y, w, h);
}

void TooltipWidget::SetBorderColor(int r, int g, int b, int a)
{
	SetBorderColor(Color(r, g, b, a));
}

void TooltipWidget::SetTooltipColor(int r, int g, int b, int a)
{
	SetTooltipColor(Color(r, g, b, a));
}

void TooltipWidget::SetTextColor(int r, int g, int b, int a)
{
	SetTextColor(Color(r, g, b, a));
}

// ---- tooltips (GameApp, TooltipWidget.cpp) ----

// V1:0x44bf00: a tooltip in front of everything, in the small font, black on pale yellow, appearing at once and
// staying until it is hidden. The scripts' MakeTooltip.
TooltipWidget* GameApp::MakeTooltip()
{
	TooltipWidget* aTooltip = new TooltipWidget();
	mWidgetManager->AddWidget(aTooltip);
	mWidgetManager->BringToFront(aTooltip);
	aTooltip->SetFont(FONT_SMALL);
	aTooltip->SetBorderColor(Color(0, 0, 0));
	aTooltip->SetTooltipColor(Color(255, 255, 225));
	aTooltip->SetTextColor(Color(0, 0, 0));
	aTooltip->SetDisplayDelay(0);
	aTooltip->SetDisplayTime(-1);
	mTooltips.push_back(aTooltip);
	return aTooltip;
}

// V1:0x44c080: takes the tooltip away and deletes it; false when it is not one of the app's. The scripts'
// RemoveTooltip.
bool GameApp::RemoveTooltip(TooltipWidget* theTooltip)
{
	std::vector<TooltipWidget*>::iterator anItr = mTooltips.begin();
	while (anItr != mTooltips.end() && *anItr != theTooltip)
		++anItr;
	if (anItr == mTooltips.end())
		return false;
	mDisabledTooltips.erase(theTooltip);
	mTooltips.erase(anItr);
	mWidgetManager->RemoveWidget(theTooltip);
	if (mHoverTooltip == theTooltip)
		mHoverTooltip = nullptr;
	delete theTooltip;
	return true;
}

// V1:0x44c1a0 (the book's returns from a mini-game and to the chapter select): every tooltip goes
void GameApp::RemoveAllTooltips()
{
	for (size_t i = 0; i < mTooltips.size(); i++) {
		if (mHoverTooltip == mTooltips[i])
			mHoverTooltip = nullptr;
		mWidgetManager->RemoveWidget(mTooltips[i]);
		delete mTooltips[i];
	}
	mTooltips.clear();
	// port: the original left the disabled ones' set pointing at the deleted tooltips
	mDisabledTooltips.clear();
}

// 0x441800: the mouse is at (x, y): the tooltip whose region it is in (and that is enabled) shows, the one that
// showed hides. A tooltip that touches one of the tutorial arrows' blocked rectangles stays out of it.
void GameApp::UpdateTooltipHover(int x, int y)
{
	bool aFound = false;
	for (size_t i = 0; i < mTooltips.size(); i++) {
		TooltipWidget* aTooltip = mTooltips[i];
		// port: the original asked the arrow hints' mouse widget without looking whether there is one
		if (mArrowHintManager != nullptr && mArrowHintManager->mMouseWidget->IntersectsRects(aTooltip->GetTooltipRect()))
			continue;
		const Rect& aRegion = aTooltip->GetActivationRegion();
		if (aRegion.Contains(x, y) && !aTooltip->mDisabled) {
			aFound = true;
			if (aTooltip == mHoverTooltip)
				return;
			if (mHoverTooltip != nullptr)
				mHoverTooltip->Hide();
			mHoverTooltip = aTooltip;
			aTooltip->Show();
		}
	}
	if (!aFound && mHoverTooltip != nullptr) {
		mHoverTooltip->Hide();
		mHoverTooltip = nullptr;
	}
}

// V1:0x44c570: the tooltip showing is hidden (it stays the one under the mouse)
void GameApp::TurnOffTooltips()
{
	if (mHoverTooltip != nullptr)
		mHoverTooltip->Hide();
}

// V1:0x44c590: false disables every enabled tooltip (the app remembers which) and hides the one showing; true
// enables again those it disabled. The scripts' EnableTooltips.
void GameApp::EnableTooltips(bool theEnable)
{
	if (!theEnable) {
		for (size_t i = 0; i < mTooltips.size(); i++) {
			if (!mTooltips[i]->mDisabled) {
				mTooltips[i]->Disable();
				mDisabledTooltips.insert(mTooltips[i]);
			}
		}
		if (mHoverTooltip != nullptr)
			mHoverTooltip->Hide();
	} else {
		for (std::set<TooltipWidget*>::iterator anItr = mDisabledTooltips.begin(); anItr != mDisabledTooltips.end();
			 ++anItr)
			(*anItr)->Enable();
		mDisabledTooltips.clear();
	}
}
