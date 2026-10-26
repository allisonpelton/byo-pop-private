// Sexy::LuaImage (constructor 0x667920, 0x80 bytes; methods 0x667bb0-0x6686f0) and Sexy::LuaFont (0x6687a0,
// 0x4c bytes; loaded by 0x668990, methods 0x669010-0x6690e0): the image and font handles scripts get from
// LuaApp's GetImage/TryGetImage and GetFont/TryGetFont. Lua owns them; an image handle holds a reference to the
// shared image, a font handle its own copy of the font.
#pragma once

#include <string>

#include "Font.h"
#include "LuaBind.h"
#include "MemoryImage.h"
#include "SharedImage.h"

namespace Sexy {

class Graphics;

class LuaImage {
public:
	SharedImageRef mImage;  // +0x28
	std::string mPath;      // the file it was loaded from, and its variant
	std::string mVariant;

	LuaImage() {}
	explicit LuaImage(const SharedImageRef& theImage) : mImage(theImage) {}

	MemoryImage* Image() { return (MemoryImage*)mImage; }

	int GetWidth() { return Image()->GetWidth(); }
	int GetHeight() { return Image()->GetHeight(); }
	void SetD3DFlags(int theFlags) { Image()->mD3DFlags = theFlags; }
	int GetNumCols() { return Image()->mNumCols; }
	int GetNumRows() { return Image()->mNumRows; }
	int GetNumCels() { return Image()->mNumCols * Image()->mNumRows; }
	void SetNumCols(int n) { Image()->mNumCols = n; }
	void SetNumRows(int n) { Image()->mNumRows = n; }
	int GetCelWidth() { return Image()->GetCelWidth(); }
	int GetCelHeight() { return Image()->GetCelHeight(); }
	// x, y, width, height
	LuaLib::LuaMultiRet GetCelRect(int theCol, int theRow);
	void Create(int theWidth, int theHeight) { Image()->Create(theWidth, theHeight); }
	void SetImageMode(bool hasTrans, bool hasAlpha) { Image()->SetImageMode(hasTrans, hasAlpha); }
	// 0 outside the image
	int GetPixel(int x, int y);
	void PutPixel(int x, int y, int theColor);
	bool Palletize() { return Image()->Palletize(); }
	void Clear() { Image()->Clear(); }
	// A Graphics that draws into the image (Lua owns it)
	LuaLib::LuaMultiRet GetGraphics();

	static void RegisterLua();
};

class LuaFont {
public:
	Font* mFont = nullptr;  // +0x44, its own copy (or a resource's, when borrowed)
	std::string mName;
	bool mBorrowed = false;   // +0x48: the font belongs to someone else (a resource's: 0x670080)
	bool mIsSysFont = false;  // +0x49: made from "name,size[,bold,...]" rather than a resource

	LuaFont() {}
	LuaFont(const std::string& theName, Font* theFont) : mFont(theFont), mName(theName), mBorrowed(true) {}
	~LuaFont()
	{
		if (!mBorrowed)
			delete mFont;
	}

	int StringWidth(const char* theString) { return mFont->StringWidth(theString); }
	int GetAscent() { return mFont->mAscent; }
	int GetHeight() { return mFont->mHeight; }
	int GetLineSpacing() { return mFont->GetLineSpacing(); }
	int GetLineSpacingOffset() { return mFont->GetLineSpacingOffset(); }

	// The font of a resource id, or a system font "Name,size[,bold][,italic][,underline]" (0x668990); NULL when
	// there is none
	static LuaFont* Load(const std::string& theName);

	static void RegisterLua();
};

// The Font of a LuaFont argument (NULL for nil)
inline Font* FontOf(LuaFont* f)
{
	return f != nullptr ? f->mFont : nullptr;
}
inline Image* ImageOf(LuaImage* i)
{
	return i != nullptr ? (Image*)i->mImage : nullptr;
}

}  // namespace Sexy
