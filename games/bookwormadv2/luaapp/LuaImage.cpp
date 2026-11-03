#include "LuaImage.h"

#include <string.h>

#include "BuiltinFont.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "SexyAppBase.h"

using namespace Sexy;
using namespace LuaLib;

LuaMultiRet LuaImage::GetCelRect(int theCol, int theRow)
{
	Rect r = Image()->GetCelRect(theCol, theRow);
	lua_State* L = gLuaSupport->L;
	lua_pushint(L, r.mX);
	lua_pushint(L, r.mY);
	lua_pushint(L, r.mWidth);
	lua_pushint(L, r.mHeight);
	return {4};
}

int LuaImage::GetPixel(int x, int y)
{
	MemoryImage* img = Image();
	if (x < 0 || x >= img->mWidth || y < 0 || y >= img->mHeight)
		return 0;
	return (int)img->GetBits()[img->mWidth * y + x];
}

void LuaImage::PutPixel(int x, int y, int theColor)
{
	MemoryImage* img = Image();
	if (x < 0 || x >= img->mWidth || y < 0 || y >= img->mHeight)
		return;
	img->GetBits()[img->mWidth * y + x] = (ulong)theColor;
	img->BitsChanged();
}

LuaMultiRet LuaImage::GetGraphics()
{
	gLuaSupport->PushObject(ClassOf<Graphics>::info, new Graphics((Sexy::Image*)mImage), true);
	return {1};
}

void LuaImage::RegisterLua()
{
	Class<LuaImage>("LuaImage")
		.Method<&LuaImage::GetWidth>("GetWidth")
		.Method<&LuaImage::GetHeight>("GetHeight")
		.Method<&LuaImage::SetD3DFlags>("SetD3DFlags")
		.Method<&LuaImage::GetNumCols>("GetNumCols")
		.Method<&LuaImage::GetNumRows>("GetNumRows")
		.Method<&LuaImage::SetNumCols>("SetNumCols")
		.Method<&LuaImage::SetNumRows>("SetNumRows")
		.Method<&LuaImage::GetCelWidth>("GetCelWidth")
		.Method<&LuaImage::GetCelHeight>("GetCelHeight")
		.Method<&LuaImage::GetCelRect>("GetCelRect")
		.Method<&LuaImage::GetNumCels>("GetNumCels")
		.Method<&LuaImage::Create>("Create")
		.Method<&LuaImage::SetImageMode>("SetImageMode")
		.Method<&LuaImage::GetPixel>("GetPixel")
		.Method<&LuaImage::PutPixel>("PutPixel")
		.Method<&LuaImage::Palletize>("Palletize")
		.Method<&LuaImage::Clear>("Clear")
		.Method<&LuaImage::GetGraphics>("GetGraphics");
}

// ---- LuaFont ----

static bool StartsWithNoCase(const std::string& s, const char* prefix)
{
	size_t n = strlen(prefix);
	return s.size() >= n && _strnicmp(s.c_str(), prefix, n) == 0;
}

LuaFont* LuaFont::Load(const std::string& theName)
{
	Font* font = nullptr;
	bool isSys = false;
	if (theName.compare(0, 5, "!sys:") == 0) {
		// "!sys:Face,size[,bold][,underline][,italic]": a system font. The port has no system fonts and draws
		// these with the framework's own font, bold when asked.
		std::string spec = theName.substr(5);
		bool bold = false;
		size_t pos = spec.find(',');
		while (pos != std::string::npos) {
			size_t next = spec.find(',', pos + 1);
			std::string opt = spec.substr(pos + 1, next == std::string::npos ? std::string::npos : next - pos - 1);
			if (StartsWithNoCase(opt, "bold") && opt.size() == 4)
				bold = true;
			pos = next;
		}
		font = new BuiltinFont(bold);
		isSys = true;
	} else {
		ImageFont* f = new ImageFont(gSexyAppBase, theName);
		if (f->mFontData == nullptr || !f->mFontData->mInitialized) {
			delete f;
			return nullptr;
		}
		font = f;
	}
	LuaFont* lf = new LuaFont();
	lf->mFont = font;
	lf->mName = theName;
	lf->mIsSysFont = isSys;
	return lf;
}

void LuaFont::RegisterLua()
{
	Class<LuaFont>("LuaFont")
		.Method<&LuaFont::StringWidth>("StringWidth")
		.Method<&LuaFont::GetAscent>("GetAscent")
		.Method<&LuaFont::GetHeight>("GetHeight")
		.Method<&LuaFont::GetLineSpacing>("GetLineSpacing")
		.Method<&LuaFont::GetLineSpacingOffset>("GetLineSpacingOffset");
}
