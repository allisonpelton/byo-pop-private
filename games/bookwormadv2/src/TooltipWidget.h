// Sexy::TooltipWidget (0x134 bytes, constructor 0x48bb20, vtable 0x6ffb9c; code 0x48b7e0-V1:0x4a4130): a box of text
// lines that shows near the mouse. Something (the app's UpdateTooltipHover, a button's mouse enter) shows it with
// Show; it appears after mDisplayDelay updates and goes away after mDisplayTime (0 or less: when it is hidden).
// The app keeps every tooltip (GameApp::MakeTooltip, RemoveTooltip, UpdateTooltipHover; TooltipWidget.cpp), and the
// scripts reach them through the app and through the 21 methods below. A LuaWidget, but nothing links it to a Lua
// object, so its events (Update, Draw, the mouse) never go to Lua.
#pragma once

#include <string>
#include <vector>

#include "Color.h"
#include "LuaWidgets.h"
#include "Rect.h"

namespace Sexy {

class Font;

class TooltipWidget : public LuaWidget {
public:
	int mDisplayDelay = 100;           // +0xcc: updates between Show and the box appearing
	int mDisplayTime = 200;            // +0xd0: updates the box stays up (0 or less: until Hide)
	int mUpdates = 0;                  // +0xd4: counts the updates (nothing reads it)
	int mWaitCount = 0;                // +0xd8: updates since Show (waiting to appear), then since it appeared
	bool mArmed = false;               // +0xdc: shown (Show) and not hidden since
	bool mShowing = false;             // +0xdd: the box is up
	bool mAllowOverlayDisplay = false; // +0xde: also drawn while an overlay or the conversation panel is up
	Font* mFont = nullptr;             // +0xe0: the lines' font (not owned)
	std::vector<std::string> mLines;   // +0xe4: the lines (one AddText each)
	Color mBorderColor;                // +0xf4: (0, 0, 0) until set
	Color mTooltipColor;               // +0x104: the box (40, 65, 45) until set
	Color mTextColor;                  // +0x114: (215, 255, 10) until set
	Rect mActivationRegion;            // +0x124: where the mouse shows the tooltip (GameApp::UpdateTooltipHover)

	TooltipWidget();  // 0x48bb20

	const char* LuaGetRegisteredName() override { return "TooltipWidget"; }  // 0x48b7e0
	// The original registered the class in the constructor, every time
	static void RegisterLua();

	// ---- Widget ----
	void Update() override;           // 0x48bf30
	void Draw(Graphics* g) override;  // 0x48bfb0
	// vf 0xc8 and vf 0xcc, which the app and the board call. The framework's mouse-enter and mouse-leave are the
	// tooltip's Show and Hide: nothing calls them as the mouse events of the widget (it is not mouse-visible)
	void MouseEnter() override;  // 0x48c170
	void MouseLeave() override;  // 0x48c1f0

	// Starts the countdown to appearing (Show) / hides it and stops the countdown (Hide: also a Lua method). Show
	// does nothing when the player's profile has DisableTips set.
	void Show() { MouseEnter(); }
	void Hide() { MouseLeave(); }  // 0x48c1f0

	// ---- the scripts' methods (Lua names in bindings.txt; slot N = vtable offset N * 4) ----
	void Disable();                                // slot 99, 0x48c500: invisible, disabled, hidden
	void Enable();                                 // slot 100, 0x48c520: visible and enabled, when it has lines
	virtual int CalcWidth();                       // slot 101, 0x48c2e0: the widest line + 8, 0 without a font
	virtual int CalcHeight();                      // slot 102, 0x48c390: lines * (ascent + 2) + 8, 0 without a font
	void SetAllowOverlayDisplay(bool on) { mAllowOverlayDisplay = on; }  // slot 103, 0x48b7f0
	void AddText(std::string theText);             // slot 104, 0x48c240: a line, then the box is fitted to the lines
	void ClearText();                              // slot 105, 0x48b800 (the box keeps its size)
	void SetFont(Font* theFont);                   // slot 106, 0x48c210: the box is fitted to the lines
	void SetXY(int x, int y);                      // slot 107, 0x48c410: kept on the screen
	void SetX(int x) { SetXY(x, mY); }             // slot 108, 0x48b850
	void SetY(int y) { SetXY(mX, y); }             // slot 109, 0x48b870
	int GetX() { return mX; }                      // slot 110, 0x40da50
	int GetY() { return mY; }                      // slot 111, 0x48b890
	void SetActivationRegion(int x, int y, int w, int h);  // slots 112 and 113, 0x48b8d0, 0x48b8a0
	void SlideActivationRegion(int dx) { mActivationRegion.mX += dx; }  // slot 114, 0x48b900
	void SetBorderColor(int r, int g, int b, int a);  // slot 115, 0x48b940
	void SetBorderColor(const Color& theColor) { mBorderColor = theColor; }  // slot 116, 0x48b910
	void SetTooltipColor(int r, int g, int b, int a);  // slot 117, 0x48b9d0
	void SetTooltipColor(const Color& theColor) { mTooltipColor = theColor; }  // slot 118, 0x48b9a0
	void SetTextColor(int r, int g, int b, int a);  // slot 119, 0x48ba60
	void SetTextColor(const Color& theColor) { mTextColor = theColor; }  // slot 120, 0x48ba30
	void SetDisplayDelay(int theUpdates) { mDisplayDelay = theUpdates; }  // slot 121, 0x48bac0
	void SetDisplayTime(int theUpdates) { mDisplayTime = theUpdates; }  // slot 122, 0x61bbc0
	const Rect& GetActivationRegion() const { return mActivationRegion; }  // slot 123, 0x48bad0 (the Lua method is raw, V1:0x4a26a0)
	Rect GetTooltipRect() { return Rect(mX, mY, mWidth, mHeight); }  // slot 124, 0x48c150: the box
	int NumTextLines() { return (int)mLines.size(); }  // slot 125, 0x48bae0
	void SetPriority(int thePriority) { mPriority = thePriority; }  // V1:0x4a1ce0
};

}  // namespace Sexy
