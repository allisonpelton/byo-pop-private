// The widgets scripts make with LuaApp's CreateLua* functions (0x65f7f0-0x65fe60), each a LuaWidgetTpl of a
// framework widget with a few setters and getters for its fields (0x677700-0x679550). A widget's listener is
// the Lua object passed to its factory (its LuaWidgetTplBase part), so the widget's button, edit, list, scroll
// or checkbox events go to that object's Lua methods. RegisterObject links the widget to a Lua object of its
// own class (classlink.lua's CreateLinkedObject).
#pragma once

#include <string>

#include "ButtonWidget.h"
#include "Checkbox.h"
#include "Dialog.h"
#include "EditWidget.h"
#include "HyperlinkWidget.h"
#include "ListWidget.h"
#include "LuaImage.h"
#include "LuaWidgetTpl.h"
#include "ScrollbarWidget.h"

namespace Sexy {

// The listener (a LuaWidgetTplBase) of the Lua object at idx: its GetLuaWidgetTplBase(); "cobj expected" for
// anything but an object (0x65c900)
LuaWidgetTplBase* LuaCheckListener(lua_State* L, int idx);

class LuaWidget : public LuaWidgetTpl<Widget> {
public:
	const char* LuaGetRegisteredName() override { return "LuaWidget"; }
	void RegisterObject() { CreateLinkedObject(this); }
	static void RegisterLua();
};

// A widget that writes a word-wrapped label over what its Lua Draw drew (0x677bb0, Draw 0x677e90)
class LuaTextWidget : public LuaWidget {
public:
	std::string mLabel;
	Font* mFont = nullptr;  // its own copy; a system font when none is set
	int mLineSpacing = -1;
	int mJustification = -1;

	LuaTextWidget() { mClip = false; }
	~LuaTextWidget() override { delete mFont; }

	const char* LuaGetRegisteredName() override { return "LuaTextWidget"; }
	void Draw(Graphics* g) override;

	const char* GetLabel() { return mLabel.c_str(); }
	void SetLabel(const std::string& theLabel) { mLabel = theLabel; }
	void SetFont(LuaFont* theFont);
	int GetLineSpacing() { return mLineSpacing; }
	void SetLineSpacing(int n) { mLineSpacing = n; }
	int GetJustification() { return mJustification; }
	void SetJustification(int n) { mJustification = n; }

	static void RegisterLua();
};

class LuaButtonWidget : public LuaWidgetTpl<ButtonWidget> {
public:
	LuaButtonWidget(int theId, LuaWidgetTplBase* theListener)
	    : LuaWidgetTpl<ButtonWidget>(theId, static_cast<ButtonListener*>(theListener))
	{
	}
	const char* LuaGetRegisteredName() override { return "LuaButtonWidget"; }
	void RegisterObject() { CreateLinkedObject(this); }

	void SetLabel(const char* theLabel) { mLabel = theLabel; }
	void SetFont(LuaFont* theFont);
	void SetNormalRect(int x, int y, int w, int h) { mNormalRect = Rect(x, y, w, h); }
	void SetOverRect(int x, int y, int w, int h) { mOverRect = Rect(x, y, w, h); }
	void SetDownRect(int x, int y, int w, int h) { mDownRect = Rect(x, y, w, h); }
	void SetDisabledRect(int x, int y, int w, int h) { mDisabledRect = Rect(x, y, w, h); }
	void SetButtonImage(LuaImage* theImage) { mButtonImage = ImageOf(theImage); }
	void SetOverImage(LuaImage* theImage) { mOverImage = ImageOf(theImage); }
	void SetDownImage(LuaImage* theImage) { mDownImage = ImageOf(theImage); }
	int GetId() { return mId; }
	void SetId(int theId) { mId = theId; }
	bool GetFrameNoDraw() { return mFrameNoDraw; }
	void SetFrameNoDraw(bool on) { mFrameNoDraw = on; }
	bool GetInverted() { return mInverted; }
	void SetInverted(bool on) { mInverted = on; }
	int GetLabelJustify() { return mLabelJustify; }
	void SetLabelJustify(int n) { mLabelJustify = n; }

	static void RegisterLua();
};

class LuaHyperlinkWidget : public LuaWidgetTpl<HyperlinkWidget> {
public:
	LuaHyperlinkWidget(int theId, LuaWidgetTplBase* theListener)
	    : LuaWidgetTpl<HyperlinkWidget>(theId, static_cast<ButtonListener*>(theListener))
	{
	}
	const char* LuaGetRegisteredName() override { return "LuaHyperlinkWidget"; }
	void RegisterObject() { CreateLinkedObject(this); }
	// colour 0 is the link's, 1 its colour under the mouse (0x6785d0)
	void SetColor(int theIdx, const Color& theColor) override;

	void SetLabel(const char* theLabel) { mLabel = theLabel; }
	void SetFont(LuaFont* theFont);
	int GetId() { return mId; }
	void SetId(int theId) { mId = theId; }
	int GetUnderlineSize() { return mUnderlineSize; }
	void SetUnderlineSize(int n) { mUnderlineSize = n; }
	int GetUnderlineOffset() { return mUnderlineOffset; }
	void SetUnderlineOffset(int n) { mUnderlineOffset = n; }

	static void RegisterLua();
};

class LuaEditWidget : public LuaWidgetTpl<EditWidget> {
public:
	LuaEditWidget(int theId, LuaWidgetTplBase* theListener)
	    : LuaWidgetTpl<EditWidget>(theId, static_cast<EditListener*>(theListener))
	{
	}
	const char* LuaGetRegisteredName() override { return "LuaEditWidget"; }
	void RegisterObject() { CreateLinkedObject(this); }

	void SetFont(LuaFont* theFont) { EditWidget::SetFont(FontOf(theFont), nullptr); }
	// SetText(text[, leftPosToZero])
	void SetText(const char* theText, LuaLib::LuaValue theLeftPos);
	const char* GetText() { return mString.c_str(); }
	void SetPasswordChar(const char* theChar) { mPasswordChar = theChar[0]; }
	int GetId() { return mId; }
	void SetId(int theId) { mId = theId; }
	int GetMaxChars() { return mMaxChars; }
	void SetMaxChars(int n) { mMaxChars = n; }
	int GetMaxPixels() { return mMaxPixels; }
	void SetMaxPixels(int n) { mMaxPixels = n; }
	bool GetDrawSelOverride() { return mDrawSelOverride; }
	void SetDrawSelOverride(bool on) { mDrawSelOverride = on; }

	static void RegisterLua();
};

class LuaListWidget : public LuaWidgetTpl<ListWidget> {
public:
	LuaListWidget(int theId, Font* theFont, LuaWidgetTplBase* theListener)
	    : LuaWidgetTpl<ListWidget>(theId, theFont, static_cast<ListListener*>(theListener))
	{
	}
	const char* LuaGetRegisteredName() override { return "LuaListWidget"; }
	void RegisterObject() { CreateLinkedObject(this); }
	// a Lua ScrollPosition goes to the list's own (0x678d20)
	void LuaScrollPosition(int theId, double thePosition) override { ListWidget::ScrollPosition(theId, thePosition); }

	// the list uses the Lua font's own (0x678c80)
	void SetFont(LuaFont* theFont) { mFont = FontOf(theFont); }
	void SetStrColor(const std::string& theLine, int theColor) { ListWidget::SetColor(theLine, Color(theColor)); }
	void SetIdxColor(int theIdx, int theColor) { ListWidget::SetColor(theIdx, Color(theColor)); }
	int GetId() { return mId; }
	void SetId(int theId) { mId = theId; }
	int GetJustify() { return mJustify; }
	void SetJustify(int n) { mJustify = n; }
	int GetHiliteIdx() { return mHiliteIdx; }
	void SetHiliteIdx(int n) { mHiliteIdx = n; }
	int GetSelectIdx() { return mSelectIdx; }
	void SetSelectIdx(int n) { mSelectIdx = n; }
	bool GetDrawOutline() { return mDrawOutline; }
	void SetDrawOutline(bool on) { mDrawOutline = on; }
	int GetItemHeight() { return mItemHeight; }
	void SetItemHeight(int n) { mItemHeight = n; }
	bool GetDrawSelectWhenHilited() { return mDrawSelectWhenHilited; }
	void SetDrawSelectWhenHilited(bool on) { mDrawSelectWhenHilited = on; }
	bool GetDoFingerWhenHilited() { return mDoFingerWhenHilited; }
	void SetDoFingerWhenHilited(bool on) { mDoFingerWhenHilited = on; }

	static void RegisterLua();
};

class LuaScrollbarWidget : public LuaWidgetTpl<ScrollbarWidget> {
public:
	LuaScrollbarWidget(int theId, LuaWidgetTplBase* theListener)
	    : LuaWidgetTpl<ScrollbarWidget>(theId, static_cast<ScrollListener*>(theListener))
	{
	}
	const char* LuaGetRegisteredName() override { return "LuaScrollbarWidget"; }
	void RegisterObject() { CreateLinkedObject(this); }
	// Lua's button methods on a scrollbar are its own arrows' (0x678f90-0x678fb0)
	virtual void LuaScrollbarButtonPress(int theId) { ScrollbarWidget::ButtonPress(theId); }
	void LuaButtonDepress(int theId) override {}
	void LuaButtonDownTick(int theId) override { ScrollbarWidget::ButtonDownTick(theId); }

	double GetMaxValue() { return mMaxValue; }
	double GetPageSize() { return mPageSize; }
	double GetValue() { return mValue; }
	bool GetHorizontal() { return mHorizontal; }
	int GetId() { return mId; }
	void SetId(int theId) { mId = theId; }

	static void RegisterLua();
};

class LuaCheckboxWidget : public LuaWidgetTpl<Checkbox> {
public:
	LuaCheckboxWidget(Image* theUnchecked, Image* theChecked, int theId, LuaWidgetTplBase* theListener)
	    : LuaWidgetTpl<Checkbox>(theUnchecked, theChecked, theId, static_cast<CheckboxListener*>(theListener))
	{
	}
	const char* LuaGetRegisteredName() override { return "LuaCheckboxWidget"; }
	void RegisterObject() { CreateLinkedObject(this); }

	int GetId() { return mId; }
	void SetId(int theId) { mId = theId; }
	bool GetChecked() { return mChecked; }
	void SetChecked(bool on) { mChecked = on; }

	static void RegisterLua();
};

class LuaDialog : public LuaWidgetTpl<Dialog> {
public:
	LuaDialog(Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
	          const std::string& theHeader, const std::string& theLines, const std::string& theFooter, int theButtonMode)
	    : LuaWidgetTpl<Dialog>(theComponentImage, theButtonComponentImage, theId, isModal, theHeader, theLines,
	                           theFooter, theButtonMode)
	{
	}
	const char* LuaGetRegisteredName() override { return "LuaDialog"; }
	void RegisterObject() { CreateLinkedObject(this); }
	// Lua's ButtonPress and ButtonDepress are the dialog's own, for its Yes and No buttons (0x679530, 0x679550)
	void LuaButtonPress(int theId, int theClickCount) override { Dialog::ButtonPress(theId); }
	void LuaButtonDepress(int theId) override { Dialog::ButtonDepress(theId); }

	void SetButtonFont(LuaFont* theFont) { Dialog::SetButtonFont(FontOf(theFont)); }
	void SetHeaderFont(LuaFont* theFont) { Dialog::SetHeaderFont(FontOf(theFont)); }
	void SetLinesFont(LuaFont* theFont) { Dialog::SetLinesFont(FontOf(theFont)); }
	// WaitForResult([autoKill]) (0x6794e0)
	int WaitForResult(LuaLib::LuaValue theAutoKill);
	int GetId() { return mId; }
	void SetId(int theId) { mId = theId; }

	static void RegisterLua();
};

// The classes above, in the order the original's constructors registered them
void RegisterLuaWidgets();

}  // namespace Sexy
