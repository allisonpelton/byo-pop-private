// Sexy::LuaWidgetTpl<T> (LuaWidgetTpl<Widget>: vtable 0x7aa13c, constructor 0x40c380, methods 0x40c550-0x40d100,
// registered by 0x40d600; one instance per widget class, ButtonWidget's at 0x679750-0x68d188): a framework
// widget whose virtual functions are written in Lua. Each event (Update, Draw, the mouse and keys, and the
// listener callbacks of the widgets it owns) calls the method of that name of the widget's linked Lua object;
// the Lua object gets the framework's own behaviour back by calling the same method of its C++ object
// (self.mCObj:MouseDown(...)), which the template registers for Lua.
//
// The original's LuaWidgetTplBase is the listener part (ButtonListener, EditListener, ScrollListener,
// CheckboxListener, ListListener) that the factories of LuaApp hand to the widgets they make; here it is also the
// LuaLinkedClass, which the original has as a base of its own.
#pragma once

#include <string>

#include "ButtonListener.h"
#include "CheckboxListener.h"
#include "EditListener.h"
#include "Graphics.h"
#include "ListListener.h"
#include "LuaBind.h"
#include "LuaLinkedClass.h"
#include "ScrollListener.h"
#include "Widget.h"

namespace Sexy {

class LuaWidgetTplBase : public LuaLib::LuaLinkedClass,
                         public ButtonListener,
                         public EditListener,
                         public ScrollListener,
                         public CheckboxListener,
                         public ListListener {
public:
	// The listener callbacks call the Lua object's methods of the same names (0x40cb60-0x40d100)
	void ButtonPress(int theId, int theClickCount) override;
	void ButtonDepress(int theId) override;
	void ButtonDownTick(int theId) override;
	void ButtonMouseEnter(int theId) override;
	void ButtonMouseLeave(int theId) override;
	void ButtonMouseMove(int theId, int theX, int theY) override;
	void EditWidgetText(int theId, const SexyString& theString) override;
	bool AllowKey(int theId, KeyCode theKey) override;
	bool AllowChar(int theId, SexyChar theChar) override;
	bool AllowText(int theId, const SexyString& theText) override;
	void ScrollPosition(int theId, double thePosition) override;
	void CheckboxChecked(int theId, bool checked) override;
	void ListClicked(int theId, int theIdx, int theClickCount) override;
	void ListClosed(int theId) override;
	void ListHiliteChanged(int theId, int theOldIdx, int theNewIdx) override;

	// What a Lua object's listener methods get when they call their C++ object's (slots 84-98 of
	// LuaWidgetTpl<Widget>): nothing, and true for the Allow* questions.
	virtual void LuaButtonPress(int theId, int theClickCount) {}
	virtual void LuaButtonDepress(int theId) {}
	virtual void LuaCheckboxChecked(int theId, bool checked) {}
	virtual void LuaButtonMouseEnter(int theId) {}
	virtual void LuaButtonMouseLeave(int theId) {}
	virtual void LuaButtonMouseMove(int theId, int theX, int theY) {}
	virtual void LuaButtonDownTick(int theId) {}
	virtual void LuaScrollPosition(int theId, double thePosition) {}
	virtual void LuaEditWidgetText(int theId, const std::string& theString) {}
	virtual bool LuaAllowKey(int theId, int theKey) { return true; }
	virtual bool LuaAllowChar(int theId, const char* theChar) { return true; }
	virtual bool LuaAllowText(int theId, const std::string& theText) { return true; }
	virtual void LuaListClicked(int theId, int theIdx, int theClickCount) {}
	virtual void LuaListClosed(int theId) {}
	virtual void LuaListHiliteChanged(int theId, int theOldIdx, int theNewIdx) {}

	// The widget part (the template's T)
	virtual Widget* GetTplWidget() = 0;
};

// Pushes a Graphics as a sexy.Graphics userdata (0x411550)
void PushGraphics(Graphics* g);

template <class T>
class LuaWidgetTpl : public T, public LuaWidgetTplBase {
public:
	template <class... A>
	explicit LuaWidgetTpl(A&&... a) : T(std::forward<A>(a)...)
	{
	}

	~LuaWidgetTpl() override
	{
		// 0x40c480: the children go with it, then the Lua object (LuaWidgetTplBase's destructor)
		T::RemoveAllWidgets(true, false);
	}

	const char* LuaGetRegisteredName() override { return "LuaWidgetTpl<_Base>"; }
	Widget* GetTplWidget() override { return this; }

	// ---- the widget's events, written in Lua ----
	void Update() override { CallLinkedMethod("Update", 0); }
	void Draw(Graphics* g) override
	{
		mGraphics = g;
		PushGraphics(g);
		CallLinkedMethod("Draw", 1);
		mGraphics = nullptr;
	}
	void DrawOverlay(Graphics* g) override
	{
		mGraphics = g;
		PushGraphics(g);
		CallLinkedMethod("DrawOverlay", 1);
		mGraphics = nullptr;
	}
	bool IsPointVisible(int x, int y) override
	{
		lua_State* L = LuaLib::gLuaSupport->L;
		lua_pushint(L, x);
		lua_pushint(L, y);
		return CallLinkedMethod("IsPointVisible", 2).Bool();
	}
	void MouseEnter() override { CallLinkedMethod("MouseEnter", 0); }
	void MouseLeave() override { CallLinkedMethod("MouseLeave", 0); }
	void MouseMove(int x, int y) override { PushInts(x, y), CallLinkedMethod("MouseMove", 2); }
	void MouseDrag(int x, int y) override { PushInts(x, y), CallLinkedMethod("MouseDrag", 2); }
	void MouseDown(int x, int y, int theClickCount) override { T::MouseDown(x, y, theClickCount); }
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override
	{
		PushInts(x, y, theBtnNum, theClickCount);
		CallLinkedMethod("MouseDown", 4);
	}
	void MouseUp(int x, int y) override { T::MouseUp(x, y); }
	void MouseUp(int x, int y, int theLastDownButtonId) override { Widget::MouseUp(x, y, theLastDownButtonId); }  // no base overrides it (ScrollbarWidget hides it)
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override
	{
		PushInts(x, y, theBtnNum, theClickCount);
		CallLinkedMethod("MouseUp", 4);
	}
	void MouseWheel(int theDelta) override { PushInts(theDelta), CallLinkedMethod("MouseWheel", 1); }
	void KeyChar(SexyChar theChar) override
	{
		char s[2] = {(char)theChar, 0};
		lua_pushstring(LuaLib::gLuaSupport->L, s);
		CallLinkedMethod("KeyChar", 1);
	}
	void KeyDown(KeyCode theKey) override { PushInts((int)theKey), CallLinkedMethod("KeyDown", 1); }
	void KeyUp(KeyCode theKey) override { PushInts((int)theKey), CallLinkedMethod("KeyUp", 1); }
	void GotFocus() override { CallLinkedMethod("GotFocus", 0); }
	void LostFocus() override { CallLinkedMethod("LostFocus", 0); }
	bool WantsFocus() override { return CallLinkedMethod("WantsFocus", 0).Bool(); }

	// ---- the framework's behaviour, for the Lua object (new virtuals 70-83) ----
	virtual void LuaUpdate() { T::Update(); }
	virtual void LuaDraw()
	{
		if (mGraphics != nullptr)
			T::Draw(mGraphics);
	}
	virtual void LuaDrawOverlay() {}
	virtual void LuaMouseEnter() { T::MouseEnter(); }
	virtual bool LuaIsPointVisible(int x, int y) { return T::IsPointVisible(x, y); }
	virtual void LuaMouseLeave() { T::MouseLeave(); }
	virtual void LuaMouseMove(int x, int y) { T::MouseMove(x, y); }
	virtual void LuaMouseDrag(int x, int y) { T::MouseDrag(x, y); }
	virtual void LuaMouseDown(int x, int y, int theBtnNum, int theClickCount) { T::MouseDown(x, y, theBtnNum, theClickCount); }
	virtual void LuaMouseUp(int x, int y, int theBtnNum, int theClickCount) { T::MouseUp(x, y, theBtnNum, theClickCount); }
	virtual void LuaMouseWheel(int theDelta) { T::MouseWheel(theDelta); }
	virtual void LuaKeyChar(const char* theChar) { T::KeyChar((SexyChar)(theChar != nullptr ? theChar[0] : 0)); }
	virtual void LuaKeyDown(int theKey) { T::KeyDown((KeyCode)theKey); }
	virtual void LuaKeyUp(int theKey) { T::KeyUp((KeyCode)theKey); }
	virtual void LuaGotFocus() { T::GotFocus(); }
	virtual void LuaLostFocus() { T::LostFocus(); }
	virtual bool LuaWantsFocus() { return T::WantsFocus(); }

	// The 40 methods every Lua widget class has (0x40d600); `name' is the Lua class ("LuaWidgetTpl<Widget>")
	static void RegisterLua(const char* name);

	Graphics* mGraphics = nullptr;  // the Graphics of the Draw in progress (+0xc8)

private:
	template <class... I>
	static void PushInts(I... v)
	{
		lua_State* L = LuaLib::gLuaSupport->L;
		(lua_pushint(L, v), ...);
	}

	static void MarkDirty_(LuaWidgetTpl* w) { w->MarkDirty(); }
	static void Resize_(LuaWidgetTpl* w, int x, int y, int width, int height) { w->Resize(x, y, width, height); }
	static void ShowFinger_(LuaWidgetTpl* w, bool on) { w->ShowFinger(on); }
	static void DeferOverlay_(LuaWidgetTpl* w, int thePriority) { w->DeferOverlay(thePriority); }
	// pushes the widget as a sexy.Widget (0x40ded0)
	static LuaLib::LuaMultiRet GetWidget_(LuaWidgetTpl* w)
	{
		LuaLib::gLuaSupport->PushObject(LuaLib::ClassOf<Widget>::info, (Widget*)w);
		return {1};
	}
	// the listener part, for LuaApp's factories (0x40df70; a pointer there, a light userdata here)
	static LuaLib::LuaMultiRet GetLuaWidgetTplBase_(LuaWidgetTpl* w)
	{
		lua_pushlightuserdata(LuaLib::gLuaSupport->L, (LuaWidgetTplBase*)w);
		return {1};
	}
	// {mX, mY, mWidth, mHeight} (0x40dfb0, 0x40e190)
	static LuaLib::LuaMultiRet PushRect(int x, int y, int width, int height)
	{
		lua_State* L = LuaLib::gLuaSupport->L;
		lua_newtable(L);
		const char* keys[4] = {"mHeight", "mWidth", "mY", "mX"};  // in the order the original sets them
		int vals[4] = {height, width, y, x};
		for (int i = 0; i < 4; i++) {
			lua_pushstring(L, keys[i]);
			lua_pushint(L, vals[i]);
			lua_settable(L, -3);
		}
		return {1};
	}
	static LuaLib::LuaMultiRet GetRect_(LuaWidgetTpl* w)
	{
		Rect r = w->GetRect();
		return PushRect(r.mX, r.mY, r.mWidth, r.mHeight);
	}
	static LuaLib::LuaMultiRet GetAbsoluteRect_(LuaWidgetTpl* w)
	{
		Point p = w->GetAbsPos();
		return PushRect(p.mX, p.mY, w->mWidth, w->mHeight);
	}
};

template <class T>
void LuaWidgetTpl<T>::RegisterLua(const char* name)
{
	using W = LuaWidgetTpl<T>;
	using B = LuaWidgetTplBase;
	LuaLib::Class<W>(name, false)
		.template Base<Widget>()
		.template Function<&W::MarkDirty_>("MarkDirty")
		.template Function<&W::Resize_>("Resize")
		.template Function<&W::ShowFinger_>("ShowFinger")
		.template Function<&W::DeferOverlay_>("DeferOverlay")
		.template Method<&W::LuaDrawOverlay>("DrawOverlay")
		.template Method<&W::LuaDraw>("Draw")
		.template Method<&W::LuaUpdate>("Update")
		.template Method<&W::LuaIsPointVisible>("IsPointVisible")
		.template Method<&W::LuaMouseEnter>("MouseEnter")
		.template Method<&W::LuaMouseLeave>("MouseLeave")
		.template Method<&W::LuaMouseMove>("MouseMove")
		.template Method<&W::LuaMouseDrag>("MouseDrag")
		.template Method<&W::LuaMouseDown>("MouseDown")
		.template Method<&W::LuaMouseUp>("MouseUp")
		.template Method<&W::LuaMouseWheel>("MouseWheel")
		.template Method<&W::LuaKeyChar>("KeyChar")
		.template Method<&W::LuaKeyDown>("KeyDown")
		.template Method<&W::LuaKeyUp>("KeyUp")
		.template Method<&W::LuaGotFocus>("GotFocus")
		.template Method<&W::LuaLostFocus>("LostFocus")
		.template Method<&W::LuaWantsFocus>("WantsFocus")
		.template Method<&B::LuaButtonPress>("ButtonPress")
		.template Method<&B::LuaButtonDepress>("ButtonDepress")
		.template Method<&B::LuaCheckboxChecked>("CheckboxChecked")
		.template Method<&B::LuaButtonMouseEnter>("ButtonMouseEnter")
		.template Method<&B::LuaButtonMouseLeave>("ButtonMouseLeave")
		.template Method<&B::LuaButtonMouseMove>("ButtonMouseMove")
		.template Method<&B::LuaButtonDownTick>("ButtonDownTick")
		.template Method<&B::LuaScrollPosition>("ScrollPosition")
		.template Method<&B::LuaEditWidgetText>("EditWidgetText")
		.template Method<&B::LuaAllowKey>("AllowKey")
		.template Method<&B::LuaAllowChar>("AllowChar")
		.template Method<&B::LuaAllowText>("AllowText")
		.template Method<&B::LuaListClicked>("ListClicked")
		.template Method<&B::LuaListClosed>("ListClosed")
		.template Method<&B::LuaListHiliteChanged>("ListHiliteChanged")
		.template Function<&W::GetWidget_>("GetWidget")
		.template Function<&W::GetLuaWidgetTplBase_>("GetLuaWidgetTplBase")
		.template Function<&W::GetRect_>("GetRect")
		.template Function<&W::GetAbsoluteRect_>("GetAbsoluteRect");
}

}  // namespace Sexy
