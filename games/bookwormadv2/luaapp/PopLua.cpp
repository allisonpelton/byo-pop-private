#include "PopLua.h"

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <vector>

#include "Color.h"
#include "Common.h"
#include "Font.h"
#include "Graphics.h"
#include "LuaBind.h"
#include "LuaImage.h"
#include "SexyAppBase.h"
#include "TriVertex.h"
#include "Widget.h"
#include "WidgetManager.h"
#include "lauxlib.h"
#include "platform/VFS.h"

using namespace Sexy;
using namespace LuaLib;

// ---- argument helpers ----

// An argument read as the raw functions read them: ints, numbers (truncated) and number strings, else 0
static int ToInt(lua_State* L, int idx)
{
	return idx <= lua_gettop(L) ? (int)lua_toint(L, idx) : 0;
}

static float ToFloat(lua_State* L, int idx)
{
	return idx <= lua_gettop(L) ? (float)lua_tonumber(L, idx) : 0.0f;
}

// A colour 0xAARRGGBB (numbers above 0x7fffffff keep their low 32 bits, as the original's conversion did)
static uint32_t ToColorBits(lua_State* L, int idx)
{
	if (lua_type(L, idx) == LUA_TINT)
		return (uint32_t)lua_toint(L, idx);
	return (uint32_t)(int64_t)lua_tonumber(L, idx);
}

// 0x6154c0: a table's int fields r, g, b and a (a missing or non-number one is 0: common.lua's Color(r, g, b, a) makes
// such a table), else the packed number
static Color ToColor(lua_State* L, int idx)
{
	if (lua_type(L, idx) == LUA_TTABLE) {
		if (idx < 0)
			idx = lua_gettop(L) + idx + 1;
		int aField[4];
		static const char* const aKeys[4] = {"r", "g", "b", "a"};
		for (int i = 0; i < 4; i++) {
			lua_pushstring(L, aKeys[i]);
			lua_gettable(L, idx);
			aField[i] = lua_isnumber(L, -1) ? (int)lua_toint(L, -1) : 0;
			lua_pop(L, 1);
		}
		return Color(aField[0], aField[1], aField[2], aField[3]);
	}
	uint32_t c = ToColorBits(L, idx);
	return Color((c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, (c >> 24) & 0xff);
}

// The int field `key' of the table at idx; an argument error unless it is a number
static int FieldInt(lua_State* L, int idx, const char* key)
{
	lua_pushstring(L, key);
	lua_gettable(L, idx);
	if (!lua_isnumber(L, -1))
		luaL_typerror(L, -1, "number");
	int v = (int)lua_toint(L, -1);
	lua_pop(L, 1);
	return v;
}

static Rect TableRect(lua_State* L, int idx)
{
	if (idx < 0)
		idx = lua_gettop(L) + idx + 1;
	Rect r;
	r.mX = FieldInt(L, idx, "mX");
	r.mY = FieldInt(L, idx, "mY");
	r.mWidth = FieldInt(L, idx, "mWidth");
	r.mHeight = FieldInt(L, idx, "mHeight");
	return r;
}

Rect Sexy::LuaCheckRect(lua_State* L, int idx)
{
	if (lua_istable(L, idx))
		return TableRect(L, idx);
	return Rect(ToInt(L, idx), ToInt(L, idx + 1), ToInt(L, idx + 2), ToInt(L, idx + 3));
}

static Image* CheckImage(lua_State* L, int idx)
{
	LuaImage* img = (LuaImage*)gLuaSupport->CheckObject(idx, ClassOf<LuaImage>::info);
	if (img == nullptr)
		luaL_typerror(L, idx, "LuaImage");
	return ImageOf(img);
}

static std::string CheckString(lua_State* L, int idx)
{
	return std::string(luaL_checkstring(L, idx), lua_strlen(L, idx));
}

// ---- sexy.Graphics (0x669100-0x66ce40) ----

static int G_GetColorizedAlpha(Graphics* g, lua_State* L)
{
	lua_pushint(L, g->mColor.mAlpha);
	return 1;
}

// PolyFill(points, n[, convex]) with points a table of n {mX, mY} from index 0, or
// PolyFill(x1, y1, ..., xn, yn, n[, convex])
static void ReadPolygon(lua_State* L, std::vector<Point>& pts, bool& convex)
{
	int n = lua_gettop(L);
	convex = false;
	if (lua_istable(L, 1) && n < 4) {
		int count = ToInt(L, 2);
		if (count < 0)
			count = 0;
		pts.assign(count, Point(0, 0));
		if (n == 3)
			convex = lua_toboolean(L, 3) != 0;
		lua_pushnil(L);
		while (lua_next(L, 1) != 0) {
			int i = (int)lua_toint(L, -2);
			int x = 0, y = 0;
			if (lua_istable(L, -1)) {
				lua_pushstring(L, "mX");
				lua_gettable(L, -2);
				x = (int)lua_toint(L, -1);
				lua_pop(L, 1);
				lua_pushstring(L, "mY");
				lua_gettable(L, -2);
				y = (int)lua_toint(L, -1);
				lua_pop(L, 1);
			}
			if (i >= 0 && i < count)  // the original writes past its array
				pts[i] = Point(x, y);
			lua_pop(L, 1);
		}
		return;
	}
	if (n >= 1 && lua_type(L, n) == LUA_TBOOLEAN) {
		convex = lua_toboolean(L, n) != 0;
		n--;
	}
	int count = n >= 1 ? ToInt(L, n) : 0;
	for (int i = 0; i < count && 2 * i + 2 <= n - 1; i++)
		pts.push_back(Point(ToInt(L, 2 * i + 1), ToInt(L, 2 * i + 2)));
}

static int G_PolyFill(Graphics* g, lua_State* L)
{
	std::vector<Point> pts;
	bool convex;
	ReadPolygon(L, pts, convex);
	if (!pts.empty())
		g->PolyFill(pts.data(), (int)pts.size(), convex);
	return 0;
}

static int G_PolyFillAA(Graphics* g, lua_State* L)
{
	std::vector<Point> pts;
	bool convex;
	ReadPolygon(L, pts, convex);
	if (!pts.empty())
		g->PolyFillAA(pts.data(), (int)pts.size(), convex);
	return 0;
}

static int G_ClearRect(Graphics* g, lua_State* L)
{
	g->ClearRect(LuaCheckRect(L, 1));
	return 0;
}

static int G_ClipRect(Graphics* g, lua_State* L)
{
	g->ClipRect(LuaCheckRect(L, 1));
	return 0;
}

// The original's SetClipRect also intersects (it calls ClipRect, 0x4f5bd0)
static int G_SetClipRect(Graphics* g, lua_State* L)
{
	g->ClipRect(LuaCheckRect(L, 1));
	return 0;
}

static int G_FillRect(Graphics* g, lua_State* L)
{
	g->FillRect(LuaCheckRect(L, 1));
	return 0;
}

static int G_DrawRect(Graphics* g, lua_State* L)
{
	g->DrawRect(LuaCheckRect(L, 1));
	return 0;
}

// SetColor(0xAARRGGBB), SetColor(r, g, b) or SetColor(r, g, b, a)
static int G_SetColor(Graphics* g, lua_State* L)
{
	int n = lua_gettop(L);
	if (n == 1)
		g->SetColor(ToColor(L, 1));
	else if (n == 3)
		g->SetColor(Color(ToInt(L, 1), ToInt(L, 2), ToInt(L, 3)));
	else
		g->SetColor(Color(ToInt(L, 1), ToInt(L, 2), ToInt(L, 3), ToInt(L, 4)));
	return 0;
}

static int G_SetColorI(Graphics* g, lua_State* L)
{
	g->SetColor(Color((int)ToColorBits(L, 1)));
	return 0;
}

// DrawImage(img, destRect[, srcRect]), DrawImage(img, x, y, srcRect) or DrawImage(img, x, y)
static int G_DrawImage(Graphics* g, lua_State* L)
{
	Image* img = CheckImage(L, 1);
	if (lua_istable(L, 2)) {
		Rect dest = TableRect(L, 2);
		Rect src(0, 0, img->GetWidth(), img->GetHeight());
		if (lua_gettop(L) != 2) {
			if (!lua_istable(L, 3))
				luaL_typerror(L, 3, "table");
			src = TableRect(L, 3);
		}
		g->DrawImage(img, dest, src);
	} else if (lua_istable(L, 4)) {
		Rect src = TableRect(L, 4);
		g->DrawImage(img, (int)luaL_checkint(L, 2), (int)luaL_checkint(L, 3), src);
	} else {
		g->DrawImage(img, (int)luaL_checkint(L, 2), (int)luaL_checkint(L, 3));
	}
	return 0;
}

// DrawImageF(img, x, y[, srcRect])
static int G_DrawImageF(Graphics* g, lua_State* L)
{
	Image* img = CheckImage(L, 1);
	Rect src(0, 0, img->GetWidth(), img->GetHeight());
	if (lua_gettop(L) == 4)
		src = TableRect(L, 4);
	g->DrawImageF(img, (float)lua_tonumber(L, 2), (float)lua_tonumber(L, 3), src);
	return 0;
}

// DrawImageCel(img, x, y, cel), (img, x, y, col, row) or (img, x, y, col, row, width, height) stretched
static int G_DrawImageCel(Graphics* g, lua_State* L)
{
	Image* img = CheckImage(L, 1);
	int n = lua_gettop(L);
	if (n < 5) {
		g->DrawImageCel(img, (int)luaL_checknumber(L, 2), (int)luaL_checknumber(L, 3), (int)luaL_checknumber(L, 4));
	} else if (n == 5) {
		g->DrawImageCel(img, (int)luaL_checknumber(L, 2), (int)luaL_checknumber(L, 3), (int)luaL_checknumber(L, 4),
		                (int)luaL_checknumber(L, 5));
	} else if (n == 7) {
		int col = (int)luaL_checknumber(L, 4);
		int row = (int)luaL_checknumber(L, 5);
		int cw = img->GetCelWidth();
		int ch = img->GetCelHeight();
		Rect src(cw * col, ch * row, cw, ch);
		Rect dest((int)luaL_checkint(L, 2), (int)luaL_checkint(L, 3), (int)luaL_checkint(L, 6), (int)luaL_checkint(L, 7));
		g->DrawImage(img, dest, src);
	}
	return 0;
}

// DrawImageRotated(img, x, y, rot[, useFloat][, cx, cy][, srcRect | cel | col, row]) and DrawImageRotatedF
// (without useFloat). The centre defaults to the middle of a cel, the source to the first cel; a single cel
// number counts along the longer side of the strip.
static int DrawRotated(Graphics* g, lua_State* L, bool allowFloatFlag)
{
	Image* img = CheckImage(L, 1);
	int n = lua_gettop(L);
	float x = ToFloat(L, 2);
	float y = ToFloat(L, 3);
	float rot = ToFloat(L, 4);
	bool useFloat = !allowFloatFlag;
	int i = 5;
	if (allowFloatFlag && n >= 5 && lua_type(L, 5) == LUA_TBOOLEAN) {
		useFloat = lua_toboolean(L, 5) != 0;
		i = 6;
	}
	int cw = img->GetCelWidth();
	int ch = img->GetCelHeight();
	float cx = n < i ? cw * 0.5f : (float)lua_tonumber(L, i);
	float cy = n < i + 1 ? ch * 0.5f : (float)lua_tonumber(L, i + 1);
	Rect src(0, 0, cw, ch);
	int s = i + 2;
	if (n >= s) {
		if (lua_istable(L, s)) {
			src = LuaCheckRect(L, s);
		} else {
			int a = (int)lua_tonumber(L, s);
			int col = a, row = 0;
			if (n >= s + 1)
				row = (int)lua_tonumber(L, s + 1);
			else if (img->mNumCols < img->mNumRows) {
				col = 0;
				row = a;
			}
			src = Rect(col * cw, row * ch, cw, ch);
		}
	}
	if (useFloat)
		g->DrawImageRotatedF(img, x, y, rot, cx, cy, &src);
	else
		g->DrawImageRotated(img, (int)x, (int)y, rot, (int)cx, (int)cy, &src);
	return 0;
}

static int G_DrawImageRotated(Graphics* g, lua_State* L)
{
	return DrawRotated(g, L, true);
}

static int G_DrawImageRotatedF(Graphics* g, lua_State* L)
{
	return DrawRotated(g, L, false);
}

// DrawImageBox(destRect | x, y, w, h[, srcRect], img)
static int G_DrawImageBox(Graphics* g, lua_State* L)
{
	Rect dest;
	int i;
	if (lua_istable(L, 1)) {
		dest = TableRect(L, 1);
		i = 2;
	} else {
		dest = Rect((int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2), (int)lua_tonumber(L, 3), (int)lua_tonumber(L, 4));
		i = 5;
	}
	bool haveSrc = lua_istable(L, i);
	Rect src;
	if (haveSrc) {
		src = TableRect(L, i);
		i++;
	}
	Image* img = CheckImage(L, i);
	if (!haveSrc)
		src = Rect(0, 0, img->GetWidth(), img->GetHeight());
	g->DrawImageBox(src, dest, img);
	return 0;
}

// DrawImageMirror(img, x, y[, srcRect])
static int G_DrawImageMirror(Graphics* g, lua_State* L)
{
	Image* img = CheckImage(L, 1);
	if (lua_istable(L, 4)) {
		Rect src = TableRect(L, 4);
		g->DrawImageMirror(img, (int)luaL_checkint(L, 2), (int)luaL_checkint(L, 3), src, true);
	} else {
		g->DrawImageMirror(img, (int)luaL_checknumber(L, 2), (int)luaL_checknumber(L, 3), true);
	}
	return 0;
}

// DrawImageTiled(img, x, y, width, height): the image's left and right thirds as caps, its middle third
// stretched between them
static int G_DrawImageTiled(Graphics* g, lua_State* L)
{
	if (lua_gettop(L) != 5)
		luaL_error(L, "5 parameters expected to DrawImageTiled");
	Image* img = CheckImage(L, 1);
	int x = ToInt(L, 2);
	int y = ToInt(L, 3);
	int w = ToInt(L, 4);
	int h = ToInt(L, 5);
	int iw = img->GetWidth();
	int ih = img->GetHeight();
	if (w < iw)
		luaL_error(L, "Desired width (%d) < image width (%d): must be at least same or greater.", w, iw);
	int third = iw / 3;
	int mid = w - third * 2;
	g->DrawImage(img, Rect(x, y, third, h), Rect(0, 0, third, ih));
	g->DrawImage(img, Rect(x + third, y, mid, h), Rect(third, 0, third, ih));
	g->DrawImage(img, Rect(x + third + mid, y, third, h), Rect(third * 2, 0, third, ih));
	return 0;
}

static int G_SetFont(Graphics* g, lua_State* L)
{
	LuaFont* f = (LuaFont*)gLuaSupport->CheckObject(1, ClassOf<LuaFont>::info);
	if (f == nullptr)
		luaL_typerror(L, 1, "LuaFont");
	g->SetFont(FontOf(f));
	return 0;
}

static int G_GetFontHeight(Graphics* g, lua_State* L)
{
	Font* f = g->GetFont();
	lua_pushint(L, f != nullptr ? f->GetHeight() : 0);
	return 1;
}

static int G_DrawString(Graphics* g, lua_State* L)
{
	std::string s = CheckString(L, 1);
	int x = (int)luaL_checkint(L, 2);
	int y = (int)luaL_checkint(L, 3);
	g->DrawString(s, x, y);
	return 0;
}

// WriteWordWrapped(text, rect | x, y, w, h, lineSpacing, justification[, centreVertically[, maxChars
// [, lastWidth]]]): the height written, the widest line and the last line's width
static int G_WriteWordWrapped(Graphics* g, lua_State* L)
{
	std::string s = CheckString(L, 1);
	int n = lua_gettop(L);
	Rect r;
	int spacing, just, maxChars = -1, lastWidth = 0;
	bool vcenter = false;
	if (lua_istable(L, 2)) {
		lua_pushstring(L, "mX");
		lua_gettable(L, 2);
		lua_pushstring(L, "mY");
		lua_gettable(L, 2);
		lua_pushstring(L, "mWidth");
		lua_gettable(L, 2);
		lua_pushstring(L, "mHeight");
		lua_gettable(L, 2);
		r = Rect(ToInt(L, -4), ToInt(L, -3), ToInt(L, -2), ToInt(L, -1));
		lua_pop(L, 4);
		spacing = (int)luaL_checkint(L, 3);
		just = (int)luaL_checkint(L, 4);
		if (n > 4)
			vcenter = lua_toboolean(L, 5) != 0;
		if (n > 5)
			maxChars = (int)luaL_checkint(L, 6);
		if (n >= 7)
			lastWidth = (int)luaL_checkint(L, 7);
	} else {
		r = Rect((int)luaL_checkint(L, 2), (int)luaL_checkint(L, 3), (int)luaL_checkint(L, 4), (int)luaL_checkint(L, 5));
		spacing = (int)luaL_checkint(L, 6);
		just = (int)luaL_checkint(L, 7);
		if (n > 7)
			vcenter = lua_toboolean(L, 8) != 0;
		if (n > 8)
			maxChars = (int)luaL_checkint(L, 9);
		if (n >= 10)
			lastWidth = (int)luaL_checkint(L, 10);
	}
	if (vcenter) {
		int height = g->GetWordWrappedHeight(r.mWidth, s, spacing, nullptr);
		r.mY += r.mHeight / 2 - height / 2;
	}
	int maxWidth = 0;
	int result = g->WriteWordWrapped(r, s, spacing, just, &maxWidth, maxChars, &lastWidth);
	lua_pushint(L, result);
	lua_pushint(L, maxWidth);
	lua_pushint(L, lastWidth);
	return 3;
}

static int G_StringWidth(Graphics* g, lua_State* L)
{
	lua_pushint(L, g->StringWidth(CheckString(L, 1)));
	return 1;
}

// GetWordWrappedHeight(width, text[, lineSpacing]): the height and the widest line
static int G_GetWordWrappedHeight(Graphics* g, lua_State* L)
{
	int width = (int)luaL_checkint(L, 1);
	std::string s = CheckString(L, 2);
	int spacing = lua_gettop(L) >= 3 ? (int)luaL_checkint(L, 3) : -1;
	int maxWidth = -1;
	int h = g->GetWordWrappedHeight(width, s, spacing, &maxWidth);
	lua_pushint(L, h);
	lua_pushint(L, maxWidth);
	return 2;
}

// DrawTriangleTex(img, triangles[, r, g, b, a]): triangles from index 0, each three vertices {x, y, u, v
// [, color]}; at most 1024. No script of Bookworm Adventures uses it.
static int G_DrawTriangleTex(Graphics* g, lua_State* L)
{
	Image* img = CheckImage(L, 1);
	if (!lua_istable(L, 2))
		luaL_typerror(L, 2, "table");
	uint32_t color = 0;
	if (lua_gettop(L) > 2)
		color = ((uint32_t)ToInt(L, 6) & 0xff) << 24 | ((uint32_t)ToInt(L, 3) & 0xff) << 16 |
		        ((uint32_t)ToInt(L, 4) & 0xff) << 8 | ((uint32_t)ToInt(L, 5) & 0xff);
	std::vector<TriVertex> verts;
	for (int t = 0; t < 1024; t++) {
		lua_pushint(L, t);
		lua_gettable(L, 2);
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			break;
		}
		for (int j = 0; j < 3; j++) {
			lua_pushint(L, j);
			lua_gettable(L, -2);
			TriVertex v;
			if (lua_istable(L, -1)) {
				const char* keys[4] = {"x", "y", "u", "v"};
				float f[4];
				for (int k = 0; k < 4; k++) {
					lua_pushstring(L, keys[k]);
					lua_gettable(L, -2);
					f[k] = (float)lua_tonumber(L, -1);
					lua_pop(L, 1);
				}
				v = TriVertex(f[0], f[1], f[2], f[3], color);
				lua_pushstring(L, "color");
				lua_gettable(L, -2);
				if (lua_isnumber(L, -1))
					v.color = ToColorBits(L, -1);
				lua_pop(L, 1);
			}
			verts.push_back(v);
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
	if (!verts.empty())
		g->DrawTrianglesTex(img, (const TriVertex(*)[3])verts.data(), (int)verts.size() / 3);
	return 0;
}

// ---- sexy.Widget (0x66e770-0x66ee30) ----

static int W_SetColors(Widget* w, lua_State* L)
{
	if (!lua_istable(L, 1))
		luaL_typerror(L, 1, "table");
	int n = luaL_getn(L, 1);
	for (int i = 0; i < n; i++) {
		lua_rawgeti(L, 1, i);
		w->SetColor(i, ToColor(L, -1));
		lua_pop(L, 1);
	}
	return 0;
}

static int W_WriteCenteredLine(Widget* w, lua_State* L)
{
	std::string s = CheckString(L, 3);
	Graphics* g = (Graphics*)gLuaSupport->CheckObject(1, ClassOf<Graphics>::info);
	int offset = (int)luaL_checkint(L, 2);
	w->WriteCenteredLine(g, offset, s);
	return 0;
}

static int W_SetTabNext(Widget* w, lua_State* L)
{
	w->mTabNext = (Widget*)gLuaSupport->CheckObject(1, ClassOf<Widget>::info);
	return 0;
}

static int W_SetTabPrev(Widget* w, lua_State* L)
{
	w->mTabPrev = (Widget*)gLuaSupport->CheckObject(1, ClassOf<Widget>::info);
	return 0;
}

template <class C, bool C::*F>
static int GetBool(C* o, lua_State* L)
{
	lua_pushboolean(L, o->*F);
	return 1;
}

template <class C, bool C::*F>
static int SetBool(C* o, lua_State* L)
{
	o->*F = lua_toboolean(L, 1) != 0;
	return 0;
}

template <class C, int C::*F>
static int GetInt(C* o, lua_State* L)
{
	lua_pushint(L, o->*F);
	return 1;
}

template <class C, int C::*F>
static int SetInt(C* o, lua_State* L)
{
	o->*F = (int)luaL_checkint(L, 1);
	return 0;
}

// ---- sexy.WidgetContainer (0x66e490-0x66f130) ----

static int WC_GetParent(WidgetContainer* w, lua_State* L)
{
	gLuaSupport->PushObject(ClassOf<WidgetContainer>::info, w->mParent);
	return 1;
}

static int WC_GetAbsPos(WidgetContainer* w, lua_State* L)
{
	Point p = w->GetAbsPos();
	lua_pushint(L, p.mX);
	lua_pushint(L, p.mY);
	return 2;
}

static int WC_GetSize(WidgetContainer* w, lua_State* L)
{
	lua_pushint(L, w->mWidth);
	lua_pushint(L, w->mHeight);
	return 2;
}

static int WC_AddWidget(WidgetContainer* w, lua_State* L)
{
	w->AddWidget((Widget*)gLuaSupport->CheckObject(1, ClassOf<Widget>::info));
	return 0;
}

static int WC_RemoveWidget(WidgetContainer* w, lua_State* L)
{
	w->RemoveWidget((Widget*)gLuaSupport->CheckObject(1, ClassOf<Widget>::info));
	return 0;
}

static int WC_Draw(WidgetContainer* w, lua_State* L)
{
	w->Draw((Graphics*)gLuaSupport->CheckObject(1, ClassOf<Graphics>::info));
	return 0;
}

// ---- sexy.WidgetManager (0x66f1e0-0x66f4d0) ----

static int WM_SetFocus(WidgetManager* wm, lua_State* L)
{
	wm->SetFocus((Widget*)gLuaSupport->CheckObject(1, ClassOf<Widget>::info));
	return 0;
}

static int WM_GetAnyWidgetAt(WidgetManager* wm, lua_State* L)
{
	int wx, wy;
	Widget* w = wm->GetAnyWidgetAt((int)luaL_checkint(L, 1), (int)luaL_checkint(L, 2), &wx, &wy);
	gLuaSupport->PushObject(ClassOf<Widget>::info, w);
	return 1;
}

static int WM_GetWidgetAt(WidgetManager* wm, lua_State* L)
{
	int wx, wy;
	Widget* w = wm->GetWidgetAt((int)luaL_checkint(L, 1), (int)luaL_checkint(L, 2), &wx, &wy);
	gLuaSupport->PushObject(ClassOf<Widget>::info, w);
	return 1;
}

static int WM_IsKeyDown(WidgetManager* wm, lua_State* L)
{
	int k = (int)luaL_checkint(L, 1) & 0xff;
	lua_pushboolean(L, k < (int)sizeof(wm->mKeyDown) && wm->mKeyDown[k]);
	return 1;
}

static int WM_GetLastMousePos(WidgetManager* wm, lua_State* L)
{
	lua_pushint(L, wm->mLastMouseX);
	lua_pushint(L, wm->mLastMouseY);
	return 2;
}

// ---- free functions (0x66d430-0x66df40) ----

static int F_Rand(lua_State* L)
{
	lua_pushint(L, Sexy::Rand());
	return 1;
}

// sexy.PlaySample(id[, pan])
static int F_PlaySample(lua_State* L)
{
	if (lua_gettop(L) == 1)
		gSexyAppBase->PlaySample((int)luaL_checkint(L, 1));
	else
		gSexyAppBase->PlaySample((int)luaL_checkint(L, 1), (int)luaL_checknumber(L, 2));
	return 0;
}

static int F_StringToInt(lua_State* L)
{
	std::string s = CheckString(L, 1);
	int v = 0;
	if (!Sexy::StringToInt(s, &v)) {
		std::string msg = s + " does not have a valid integer conversion.";
		lua_pushstring(L, msg.c_str());
		lua_error(L);
	}
	lua_pushint(L, v);
	return 1;
}

// PushXMLFileData(file) (0x66dbd0) pushed an XML file as nested tables, and StringWrapAnalyze(font, text,
// width[, ...]) (0x66df40) measured how a text wraps. No script of Bookworm Adventures calls them.
static int F_NotPorted(lua_State* L)
{
	return luaL_error(L, "PushXMLFileData and StringWrapAnalyze are not in the port");
}

// ---- the scripts' files ----

// port: the original read the scripts (and io's files) from the current folder, the game's. The port reads
// them through the VFS (the game folder and its mods, any case), a file the scripts wrote from the user
// folder first, and writes io's files there.
static std::string UserFile(const char* thePath)
{
	const std::string& aUserDir = Platform::VFS::Get().GetUserDir();
	return Platform::VFS::FindCaseInsensitive(aUserDir, Platform::VFS::Normalize(thePath));
}

static int ReadScriptFile(const char* thePath, char** theData, size_t* theSize)
{
	std::vector<uint8_t> aData;
	std::string aUserFile = UserFile(thePath);
	bool aRead = false;
	if (!aUserFile.empty()) {
		if (FILE* f = fopen(aUserFile.c_str(), "rb")) {
			uint8_t aBuf[4096];
			size_t n;
			while ((n = fread(aBuf, 1, sizeof aBuf, f)) > 0)
				aData.insert(aData.end(), aBuf, aBuf + n);
			fclose(f);
			aRead = true;
		}
	}
	if (!aRead && !Platform::VFS::Get().ReadAll(Platform::VFS::Normalize(thePath), aData))
		return 0;
	*theData = (char*)malloc(aData.empty() ? 1 : aData.size());
	if (!aData.empty())
		memcpy(*theData, aData.data(), aData.size());
	*theSize = aData.size();
	return 1;
}

static int ScriptFileExists(const char* thePath)
{
	return !UserFile(thePath).empty() || Platform::VFS::Get().Exists(Platform::VFS::Normalize(thePath));
}

static int WriteScriptFile(const char* thePath, const char* theData, size_t theSize)
{
	std::string aPath = Platform::VFS::Get().GetUserDir() + "/" + Platform::VFS::Normalize(thePath);
	size_t aSlash = aPath.find_last_of('/');
	if (aSlash != std::string::npos)
		Platform::MakeDirs(aPath.substr(0, aSlash));
	FILE* f = fopen(aPath.c_str(), "wb");
	if (f == NULL)
		return 0;
	bool ok = fwrite(theData, 1, theSize, f) == theSize;
	fclose(f);
	return ok ? 1 : 0;
}

// ---- registering (0x66f510) ----

void PopLua::OpenPopLua()
{
	poplua_setreader(ReadScriptFile);
	poplua_setexists(ScriptFileExists);
	poplua_setwriter(WriteScriptFile);
	Open();

	RegisterFunction("Rand", F_Rand);
	RegisterFunction("sexy.PlaySample", F_PlaySample);
	RegisterFunction("PushXMLFileData", F_NotPorted);
	RegisterFunction("StringToInt", F_StringToInt);
	RegisterFunction("StringWrapAnalyze", F_NotPorted);

	Class<Graphics>("sexy.Graphics")
		.Method<&Graphics::SetDrawMode>("SetDrawMode")
		.Method<&Graphics::GetDrawMode>("GetDrawMode")
		.Method<&Graphics::PushState>("PushState")
		.Method<&Graphics::PopState>("PopState")
		.Method<&Graphics::ClearClipRect>("ClearClipRect")
		.Method<&Graphics::Translate>("Translate")
		.Method<&Graphics::TranslateF>("TranslateF")
		.Method<&Graphics::SetFastStretch>("SetFastStretch")
		.Method<&Graphics::GetFastStretch>("GetFastStretch")
		.Method<&Graphics::SetLinearBlend>("SetLinearBlend")
		.Method<&Graphics::GetLinearBlend>("GetLinearBlend")
		.Method<&Graphics::SetColorizeImages>("SetColorizeImages")
		.Method<&Graphics::GetColorizeImages>("GetColorizeImages")
		.Method<&Graphics::DrawLine>("DrawLine")
		.Raw<G_GetColorizedAlpha>("GetColorizedAlpha")
		.Raw<G_PolyFill>("PolyFill")
		.Raw<G_ClearRect>("ClearRect")
		.Raw<G_ClipRect>("ClipRect")
		.Raw<G_SetClipRect>("SetClipRect")
		.Raw<G_FillRect>("FillRect")
		.Raw<G_DrawRect>("DrawRect")
		.Raw<G_SetColor>("SetColor")
		.Raw<G_SetColorI>("SetColorI")
		.Raw<G_DrawImage>("DrawImage")
		.Raw<G_DrawImageF>("DrawImageF")
		.Raw<G_DrawImageCel>("DrawImageCel")
		.Raw<G_DrawImageRotated>("DrawImageRotated")
		.Raw<G_DrawImageRotatedF>("DrawImageRotatedF")
		.Raw<G_DrawImageBox>("DrawImageBox")
		.Method<&Graphics::SetScale>("SetScale")
		.Raw<G_DrawImageMirror>("DrawImageMirror")
		.Raw<G_PolyFillAA>("PolyFillAA")
		.Raw<G_DrawImageTiled>("DrawImageTiled")
		.Raw<G_SetFont>("SetFont")
		.Raw<G_GetFontHeight>("GetFontHeight")
		.Raw<G_DrawString>("DrawString")
		.Raw<G_WriteWordWrapped>("WriteWordWrapped")
		.Raw<G_StringWidth>("StringWidth")
		.Raw<G_GetWordWrappedHeight>("GetWordWrappedHeight")
		.Raw<G_DrawTriangleTex>("DrawTriangleTex");

	Class<WidgetContainer>("sexy.WidgetContainer", false)
		.Method<static_cast<void (WidgetContainer::*)()>(&WidgetContainer::MarkDirty)>("MarkDirty")
		.Method<&WidgetContainer::MarkAllDirty>("MarkAllDirty")
		.Raw<WC_GetParent>("GetParent")
		.Raw<WC_GetAbsPos>("GetAbsPos")
		.Raw<WC_GetSize>("GetSize")
		.Raw<WC_AddWidget>("AddWidget")
		.Raw<WC_RemoveWidget>("RemoveWidget")
		.Raw<WC_Draw>("Draw")
		.Raw<GetInt<WidgetContainer, &WidgetContainer::mX>>("GetX")
		.Raw<SetInt<WidgetContainer, &WidgetContainer::mX>>("SetX")
		.Raw<GetInt<WidgetContainer, &WidgetContainer::mY>>("GetY")
		.Raw<SetInt<WidgetContainer, &WidgetContainer::mY>>("SetY")
		.Raw<GetInt<WidgetContainer, &WidgetContainer::mWidth>>("GetWidth")
		.Raw<SetInt<WidgetContainer, &WidgetContainer::mWidth>>("SetWidth")
		.Raw<GetInt<WidgetContainer, &WidgetContainer::mHeight>>("GetHeight")
		.Raw<SetInt<WidgetContainer, &WidgetContainer::mHeight>>("SetHeight")
		.Raw<GetBool<WidgetContainer, &WidgetContainer::mHasAlpha>>("GetHasAlpha")
		.Raw<SetBool<WidgetContainer, &WidgetContainer::mHasAlpha>>("SetHasAlpha")
		.Raw<GetBool<WidgetContainer, &WidgetContainer::mClip>>("GetClip")
		.Raw<SetBool<WidgetContainer, &WidgetContainer::mClip>>("SetClip");

	Class<WidgetManager>("sexy.WidgetManager", false)
		.Base<WidgetContainer>()
		.Method<&WidgetManager::IsLeftButtonDown>("IsLeftButtonDown")
		.Method<&WidgetManager::IsMiddleButtonDown>("IsMiddleButtonDown")
		.Method<&WidgetManager::IsRightButtonDown>("IsRightButtonDown")
		.Raw<WM_SetFocus>("SetFocus")
		.Raw<WM_GetAnyWidgetAt>("GetAnyWidgetAt")
		.Raw<WM_GetWidgetAt>("GetWidgetAt")
		.Raw<WM_IsKeyDown>("IsKeyDown")
		.Raw<WM_GetLastMousePos>("GetLastMousePos");

	Class<Widget>("sexy.Widget", false)
		.Base<WidgetContainer>()
		.Raw<W_SetColors>("SetColors")
		.Raw<W_WriteCenteredLine>("WriteCenteredLine")
		.Raw<W_SetTabNext>("SetTabNext")
		.Raw<W_SetTabPrev>("SetTabPrev")
		.Method<&Widget::SetDisabled>("SetDisabled")
		.Method<&Widget::ShowFinger>("ShowFinger")
		.Raw<GetBool<Widget, &Widget::mVisible>>("GetVisible")
		.Raw<SetBool<Widget, &Widget::mVisible>>("SetVisible")
		.Raw<GetBool<Widget, &Widget::mMouseVisible>>("GetMouseVisible")
		.Raw<SetBool<Widget, &Widget::mMouseVisible>>("SetMouseVisible")
		.Raw<GetBool<Widget, &Widget::mHasFocus>>("GetHasFocus")
		.Raw<SetBool<Widget, &Widget::mHasFocus>>("SetHasFocus")
		.Raw<GetBool<Widget, &Widget::mIsDown>>("GetIsDown")
		.Raw<SetBool<Widget, &Widget::mIsDown>>("SetIsDown")
		.Raw<GetBool<Widget, &Widget::mIsOver>>("GetIsOver")
		.Raw<SetBool<Widget, &Widget::mIsOver>>("SetIsOver")
		.Raw<GetBool<Widget, &Widget::mDoFinger>>("GetDoFinger")
		.Raw<SetBool<Widget, &Widget::mDoFinger>>("SetDoFinger");
}
