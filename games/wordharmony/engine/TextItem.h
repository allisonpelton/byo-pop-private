#ifndef __WORDHARMONY_ENGINE_TEXTITEM_H__
#define __WORDHARMONY_ENGINE_TEXTITEM_H__

// A text node (0xc0 bytes, constructor 0x427680, vtable 0x4833f0): lines in the node's font, aligned around the
// position, underlined or struck through. A text breaks into lines at the two characters backslash and n.

#include "Node.h"
#include <string>
#include <vector>

namespace engine
{

enum
{
	TEXT_UNDERLINE = 1,
	TEXT_STRIKETHROUGH = 2
};

class TextItem : public Node
{
public:
	TextItem();            // 0x427680
	~TextItem() override;  // 0x4277d0

	void draw(Graphics* g) override;  // 0x6c: 0x427260
	void layout() override;           // 0x74: 0x4270b0

	void setHAlign(int theAlign);  // 0x427070: 0 left, 1 centre, 2 right
	void setVAlign(int theAlign);  // 0x427090: 0 top, 1 middle, 2 bottom
	// 0x427600: a font of the archive by its name
	void setFont(std::string theName);
	using Node::setFont;
	// 0x4278c0
	void setText(std::string theText);
	// 0x4279f0: a number with commas between the thousands
	void setNumber(int theNumber);
	// 0x427b60: seconds as hours:minutes:seconds
	void setTime(int theSeconds);

	std::string mText;                // +0x88
	std::vector<std::string> mLines;  // +0xa4
	int mHAlign;                      // +0xb4
	int mVAlign;                      // +0xb8
	unsigned int mStyle;              // +0xbc
};

} // namespace engine

#endif
