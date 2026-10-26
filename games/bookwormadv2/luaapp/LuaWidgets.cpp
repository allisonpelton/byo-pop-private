#include "LuaWidgets.h"

#include "BuiltinFont.h"
#include "Graphics.h"

using namespace Sexy;
using namespace LuaLib;

LuaWidgetTplBase* Sexy::LuaCheckListener(lua_State* L, int idx)
{
	if (lua_type(L, idx) != LUA_TUSERDATA) {
		luaL_error(L, "cobj expected, got %s", lua_typename(L, lua_type(L, idx)));
		return nullptr;
	}
	// obj:GetLuaWidgetTplBase()
	lua_pushvalue(L, idx);
	lua_pushstring(L, "GetLuaWidgetTplBase");
	lua_gettable(L, -2);
	lua_insert(L, -2);
	lua_call(L, 1, 1);
	LuaWidgetTplBase* base = (LuaWidgetTplBase*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return base;
}

// ---- LuaWidget, LuaTextWidget ----

void LuaWidget::RegisterLua()
{
	LuaWidgetTpl<Widget>::RegisterLua("LuaWidgetTpl<Widget>");
	Class<LuaWidget>("LuaWidget", false).Base<LuaWidgetTpl<Widget>>().Method<&LuaWidget::RegisterObject>("RegisterObject");
}

void LuaTextWidget::Draw(Graphics* g)
{
	LuaWidget::Draw(g);
	if (mLabel.empty())
		return;
	if (mFont == nullptr)
		mFont = new BuiltinFont(false);  // the original's SysFont "SansSerif", 10
	g->SetColor(mColors.empty() ? Color(255, 255, 255) : mColors[0]);
	g->SetFont(mFont);
	g->WriteWordWrapped(Rect(0, 0, mWidth, mHeight), mLabel, mLineSpacing, mJustification, nullptr, -1);
}

void LuaTextWidget::SetFont(LuaFont* theFont)
{
	delete mFont;
	mFont = theFont != nullptr ? theFont->mFont->Duplicate() : nullptr;  // the original crashed on nil
}

void LuaTextWidget::RegisterLua()
{
	Class<LuaTextWidget>("LuaTextWidget", false)
		.Base<LuaWidget>()
		.Method<&LuaTextWidget::GetLabel>("GetLabel")
		.Method<&LuaTextWidget::SetLabel>("SetLabel")
		.Method<&LuaTextWidget::SetFont>("SetFont")
		.Method<&LuaTextWidget::GetLineSpacing>("GetLineSpacing")
		.Method<&LuaTextWidget::SetLineSpacing>("SetLineSpacing")
		.Method<&LuaTextWidget::GetJustification>("GetJustification")
		.Method<&LuaTextWidget::SetJustification>("SetJustification");
}

// ---- LuaButtonWidget, LuaHyperlinkWidget ----

// ButtonWidget::SetFont keeps a copy of the font (and needs one)
static void SetButtonFont(ButtonWidget* w, LuaFont* theFont)
{
	if (theFont != nullptr) {
		w->SetFont(theFont->mFont);
	} else {
		delete w->mFont;
		w->mFont = nullptr;
	}
}

void LuaButtonWidget::SetFont(LuaFont* theFont)
{
	SetButtonFont(this, theFont);
}

void LuaButtonWidget::RegisterLua()
{
	LuaWidgetTpl<ButtonWidget>::RegisterLua("LuaWidgetTpl<ButtonWidget>");
	using C = LuaButtonWidget;
	Class<C>("LuaButtonWidget", false)
		.Base<LuaWidgetTpl<ButtonWidget>>()
		.Method<&C::SetLabel>("SetLabel")
		.Method<&C::RegisterObject>("RegisterObject")
		.Method<&C::SetFont>("SetFont")
		.Method<&C::SetNormalRect>("SetNormalRect")
		.Method<&C::SetOverRect>("SetOverRect")
		.Method<&C::SetDownRect>("SetDownRect")
		.Method<&C::SetDisabledRect>("SetDisabledRect")
		.Method<&C::SetButtonImage>("SetButtonImage")
		.Method<&C::SetOverImage>("SetOverImage")
		.Method<&C::SetDownImage>("SetDownImage")
		.Method<&C::GetId>("GetId")
		.Method<&C::SetId>("SetId")
		.Method<&C::GetFrameNoDraw>("GetFrameNoDraw")
		.Method<&C::SetFrameNoDraw>("SetFrameNoDraw")
		.Method<&C::GetInverted>("GetInverted")
		.Method<&C::SetInverted>("SetInverted")
		.Method<&C::GetLabelJustify>("GetLabelJustify")
		.Method<&C::SetLabelJustify>("SetLabelJustify");
}

void LuaHyperlinkWidget::SetColor(int theIdx, const Color& theColor)
{
	if (theIdx == 0)
		mColor = theColor;
	if (theIdx == 1)
		mOverColor = theColor;
}

void LuaHyperlinkWidget::SetFont(LuaFont* theFont)
{
	SetButtonFont(this, theFont);
}

void LuaHyperlinkWidget::RegisterLua()
{
	LuaWidgetTpl<HyperlinkWidget>::RegisterLua("LuaWidgetTpl<HyperlinkWidget>");
	using C = LuaHyperlinkWidget;
	Class<C>("LuaHyperlinkWidget", false)
		.Base<LuaWidgetTpl<HyperlinkWidget>>()
		.Method<&C::SetLabel>("SetLabel")
		.Method<&C::RegisterObject>("RegisterObject")
		.Method<&C::GetId>("GetId")
		.Method<&C::SetId>("SetId")
		.Method<&C::GetUnderlineSize>("GetUnderlineSize")
		.Method<&C::SetUnderlineSize>("SetUnderlineSize")
		.Method<&C::GetUnderlineOffset>("GetUnderlineOffset")
		.Method<&C::SetUnderlineOffset>("SetUnderlineOffset")
		.Method<&C::SetFont>("SetFont");
}

// ---- LuaEditWidget ----

void LuaEditWidget::SetText(const char* theText, LuaValue theLeftPos)
{
	EditWidget::SetText(theText, lua_toboolean(gLuaSupport->L, theLeftPos.idx) != 0);
}

void LuaEditWidget::RegisterLua()
{
	LuaWidgetTpl<EditWidget>::RegisterLua("LuaWidgetTpl<EditWidget>");
	using C = LuaEditWidget;
	Class<C>("LuaEditWidget", false)
		.Base<LuaWidgetTpl<EditWidget>>()
		.Method<&EditWidget::ClearWidthCheckFonts>("ClearWidthCheckFonts")
		.Method<&C::RegisterObject>("RegisterObject")
		.Method<&C::SetFont>("SetFont")
		.Method<&C::SetText>("SetText")
		.Method<&C::GetText>("GetText")
		.Method<&C::SetPasswordChar>("SetPasswordChar")
		.Method<&C::GetId>("GetId")
		.Method<&C::SetId>("SetId")
		.Method<&C::GetMaxChars>("GetMaxChars")
		.Method<&C::SetMaxChars>("SetMaxChars")
		.Method<&C::GetMaxPixels>("GetMaxPixels")
		.Method<&C::SetMaxPixels>("SetMaxPixels")
		.Method<&C::GetDrawSelOverride>("GetDrawSelOverride")
		.Method<&C::SetDrawSelOverride>("SetDrawSelOverride");
}

// ---- LuaListWidget ----

void LuaListWidget::RegisterLua()
{
	LuaWidgetTpl<ListWidget>::RegisterLua("LuaWidgetTpl<ListWidget>");
	using C = LuaListWidget;
	Class<C>("LuaListWidget", false)
		.Base<LuaWidgetTpl<ListWidget>>()
		.Method<&ListWidget::GetStringAt>("GetStringAt")
		.Method<&ListWidget::AddLine>("AddLine")
		.Method<&ListWidget::SetLine>("SetLine")
		.Method<&ListWidget::GetLineCount>("GetLineCount")
		.Method<&ListWidget::GetLineIdx>("GetLineIdx")
		.Method<&ListWidget::Sort>("Sort")
		.Method<&ListWidget::RemoveLine>("RemoveLine")
		.Method<&ListWidget::RemoveAll>("RemoveAll")
		.Method<&ListWidget::GetOptimalWidth>("GetOptimalWidth")
		.Method<&ListWidget::GetOptimalHeight>("GetOptimalHeight")
		.Method<&C::RegisterObject>("RegisterObject")
		.Method<&C::SetFont>("SetFont")
		.Method<&C::SetStrColor>("SetStrColor")
		.Method<&C::SetIdxColor>("SetIdxColor")
		.Method<&C::GetId>("GetId")
		.Method<&C::SetId>("SetId")
		.Method<&C::GetJustify>("GetJustify")
		.Method<&C::SetJustify>("SetJustify")
		.Method<&C::GetHiliteIdx>("GetHiliteIdx")
		.Method<&C::SetHiliteIdx>("SetHiliteIdx")
		.Method<&C::GetSelectIdx>("GetSelectIdx")
		.Method<&C::SetSelectIdx>("SetSelectIdx")
		.Method<&C::GetDrawOutline>("GetDrawOutline")
		.Method<&C::SetDrawOutline>("SetDrawOutline")
		.Method<&C::GetItemHeight>("GetItemHeight")
		.Method<&C::SetItemHeight>("SetItemHeight")
		.Method<&C::GetDrawSelectWhenHilited>("GetDrawSelectWhenHilited")
		.Method<&C::SetDrawSelectWhenHilited>("SetDrawSelectWhenHilited")
		.Method<&C::GetDoFingerWhenHilited>("GetDoFingerWhenHilited")
		.Method<&C::SetDoFingerWhenHilited>("SetDoFingerWhenHilited");
}

// ---- LuaScrollbarWidget ----

void LuaScrollbarWidget::RegisterLua()
{
	LuaWidgetTpl<ScrollbarWidget>::RegisterLua("LuaWidgetTpl<ScrollbarWidget>");
	using C = LuaScrollbarWidget;
	Class<C>("LuaScrollbarWidget", false)
		.Base<LuaWidgetTpl<ScrollbarWidget>>()
		.Method<&ScrollbarWidget::ResizeScrollbar>("ResizeScrollbar")
		.Method<&ScrollbarWidget::SetMaxValue>("SetMaxValue")
		.Method<&ScrollbarWidget::SetPageSize>("SetPageSize")
		.Method<&ScrollbarWidget::SetValue>("SetValue")
		.Method<&ScrollbarWidget::SetHorizontal>("SetHorizontal")
		.Method<&C::RegisterObject>("RegisterObject")
		.Method<&C::LuaScrollbarButtonPress>("ButtonPress")
		.Method<&C::LuaButtonDepress>("ButtonDepress")
		.Method<&C::LuaButtonDownTick>("ButtonDownTick")
		.Method<&C::GetMaxValue>("GetMaxValue")
		.Method<&C::GetPageSize>("GetPageSize")
		.Method<&C::GetValue>("GetValue")
		.Method<&C::GetHorizontal>("GetHorizontal")
		.Method<&C::GetId>("GetId")
		.Method<&C::SetId>("SetId");
}

// ---- LuaCheckboxWidget ----

void LuaCheckboxWidget::RegisterLua()
{
	LuaWidgetTpl<Checkbox>::RegisterLua("LuaWidgetTpl<Checkbox>");
	using C = LuaCheckboxWidget;
	Class<C>("LuaCheckboxWidget", false)
		.Base<LuaWidgetTpl<Checkbox>>()
		.Method<&C::RegisterObject>("RegisterObject")
		.Method<&C::GetId>("GetId")
		.Method<&C::SetId>("SetId")
		.Method<&C::GetChecked>("GetChecked")
		.Method<&C::SetChecked>("SetChecked");
}

// ---- LuaDialog ----

int LuaDialog::WaitForResult(LuaValue theAutoKill)
{
	return Dialog::WaitForResult(lua_toboolean(gLuaSupport->L, theAutoKill.idx) != 0);
}

void LuaDialog::RegisterLua()
{
	LuaWidgetTpl<Dialog>::RegisterLua("LuaWidgetTpl<Dialog>");
	using C = LuaDialog;
	Class<C>("LuaDialog", false)
		.Base<LuaWidgetTpl<Dialog>>()
		.Method<&Dialog::GetPreferredHeight>("GetPreferredHeight")
		.Method<&Dialog::IsModal>("IsModal")
		.Method<&C::RegisterObject>("RegisterObject")
		.Method<&C::SetButtonFont>("SetButtonFont")
		.Method<&C::SetHeaderFont>("SetHeaderFont")
		.Method<&C::SetLinesFont>("SetLinesFont")
		.Method<&C::WaitForResult>("WaitForResult")
		.Method<&C::LuaButtonPress>("ButtonPress")
		.Method<&C::LuaButtonDepress>("ButtonDepress")
		.Method<&C::GetId>("GetId")
		.Method<&C::SetId>("SetId");
}

void Sexy::RegisterLuaWidgets()
{
	LuaWidget::RegisterLua();
	LuaTextWidget::RegisterLua();
	LuaButtonWidget::RegisterLua();
	LuaHyperlinkWidget::RegisterLua();
	LuaEditWidget::RegisterLua();
	LuaListWidget::RegisterLua();
	LuaScrollbarWidget::RegisterLua();
	LuaCheckboxWidget::RegisterLua();
	LuaDialog::RegisterLua();
}
