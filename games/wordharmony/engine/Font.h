#ifndef __WORDHARMONY_ENGINE_FONT_H__
#define __WORDHARMONY_ENGINE_FONT_H__

// A bitmap font (0x1c40 bytes; constructor 0x43d770, vtable 0x484d28): one image, and for each character its A, B and
// C widths and its rectangle in the image. A font is loaded from an XML file (res/fonts/*.xml in the archive): the
// <font> element names the image and gives the metrics, and each <glyph char= a= b= c=> adds a character; the glyphs
// lie in the image in rows, in the order of the file, gap pixels apart (0x43daa0).

#include "Graphics.h"
#include "Xml.h"
#include <string>

namespace engine
{

class ResourceManager;

class Font : public Object
{
public:
	Font(ResourceManager* theResources);  // 0x43d770
	~Font() override;                     // 0x43d680

	// 0x43d840: loads the font from a file of the archive (the application's slot 0x2c, the resource manager's openStream)
	void load(const char* theFileName);

	bool hasGlyph(char c) const;                // 0x43d530
	int getStringWidth(const char* theString);  // 0x43d550
	int getHeight() const { return mHeight; }   // 0x43d5c0
	int getSpaceWidth() const { return mSpaceWidth; }  // 0x43d5d0
	int getTopOffset() const { return -mTopOverlap; }  // 0x43d5e0
	int getLineHeight() const { return mHeight - mBottomOverlap - mTopOverlap; }  // 0x43d5f0
	int getLeadingWidth(char c) const;          // 0x43d610: A less the left overlap
	int getTrailingWidth(char c) const;         // 0x43d630: B plus C less the right overlap
	const IntRect* getGlyphRect(char c) const;  // 0x43d660
	Image* getImage() const { return mImage; }  // 0x43db70
	// 0x43daa0: a character's widths; the glyph is the next cell of the image
	void addGlyph(int theChar, int theA, int theB, int theC);

	ResourceManager* mResources;  // +0x0c
	Ref<Image> mImage;            // +0x10
	int mA[256];                  // +0x14
	int mB[256];                  // +0x414
	int mC[256];                  // +0x814
	IntRect mGlyphRects[256];     // +0xc14
	int mHeight;                  // +0x1c14
	int mSpaceWidth;              // +0x1c18
	int mGap;                     // +0x1c1c
	int mCursorX;                 // +0x1c20
	int mCursorY;                 // +0x1c24
	int mLeftOverlap;             // +0x1c28
	int mRightOverlap;            // +0x1c2c
	int mTopOverlap;              // +0x1c30
	int mBottomOverlap;           // +0x1c34

private:
	static int index(char c) { return (unsigned char)c; }
};

// The XML handler that fills a font (vtable 0x484d40; its element handler is 0x43db80)
class FontParser : public XmlHandler
{
public:
	Font* mFont;  // +0x08 of the handler

	FontParser(Font* theFont) : mFont(theFont) {}
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;
};

} // namespace engine

#endif
