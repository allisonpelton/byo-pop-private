#ifndef __SPROUT_TEXTITEM_H__
#define __SPROUT_TEXTITEM_H__

// A text node (0xc0 bytes, constructor 0x427680 (wordharmony), vtable 0x4833f0 (wordharmony); 0x160 bytes, constructor
// 0x46fc70 (pizzafrenzy), vtable 0x502168 (pizzafrenzy)): lines in the node's font, aligned around the position,
// underlined or struck through. A text breaks into lines at the two characters backslash and n. The signal build's
// also breaks a line that is wider than its wrap width at its last space, has a colour of its own and can have a
// shadow, and its lines can be moved sideways (a first line's offset and one for the next lines).

#include "Node.h"
#include <string>
#include <vector>

namespace engine
{

enum
{
	TEXT_UNDERLINE = 1,
	TEXT_STRIKETHROUGH = 2,
	TEXT_SHADOW = 4  // the signal build's
};

class TextItem : public Node
{
public:
	TextItem();            // 0x427680 (wordharmony), 0x46fc70 (pizzafrenzy)
	~TextItem() override;  // 0x4277d0 (wordharmony)

	void draw(Graphics* g) override;  // 0x6c: 0x427260 (wordharmony); 0x90: 0x46f5b0 (pizzafrenzy)
	void layout() override;           // 0x74: 0x4270b0 (wordharmony); 0x98: 0x46f670 (pizzafrenzy)

	void setHAlign(int theAlign);  // 0x427070 (wordharmony), 0x46f520 (pizzafrenzy): 0 left, 1 centre, 2 right
	void setVAlign(int theAlign);  // 0x427090 (wordharmony), 0x46f540 (pizzafrenzy): 0 top, 1 middle, 2 bottom
	// 0x427600 (wordharmony), 0x46f560 (pizzafrenzy): a font of the archive by its name
	void setFont(std::string theName);
	using Node::setFont;
	// 0x4278c0 (wordharmony), 0x46fee0 (pizzafrenzy)
	void setText(std::string theText);
	// The signal build's (Pizza Frenzy's)
	// 0x46f4d0 (pizzafrenzy): the shadow's opacity and offset
	void setShadow(float theOpacity, const Point& theOffset)
	{
		mShadowOpacity = theOpacity;
		mShadowOffset = theOffset;
	}
	// 0x46f500 (pizzafrenzy): the offsets added to the first line's x and to the next lines'
	void setLineOffsets(float theFirst, float theNext)
	{
		mLineOffset = theFirst;
		mNextLineOffset = theNext;
	}
	// 0x4279f0 (wordharmony): a number with commas between the thousands
	void setNumber(int theNumber);
	// 0x427b60 (wordharmony): seconds as hours:minutes:seconds
	void setTime(int theSeconds);

	std::string mText;                // +0x88; +0x108 (pizzafrenzy)
	std::vector<std::string> mLines;  // +0xa4; +0x124 (pizzafrenzy)
	int mHAlign;                      // +0xb4; +0x134 (pizzafrenzy)
	int mVAlign;                      // +0xb8; +0x138 (pizzafrenzy)
	unsigned int mStyle;              // +0xbc; +0x13c (pizzafrenzy)
	int mWordWrap;                    // +0x148 (pizzafrenzy): the width a line breaks at, 0 for no wrapping
	float mShadowOpacity;             // +0x14c (pizzafrenzy)
	Point mShadowOffset;              // +0x150 (pizzafrenzy)
	float mLineOffset;                // +0x140 (pizzafrenzy)
	float mNextLineOffset;            // +0x144 (pizzafrenzy)

private:
	void drawLines(Graphics* g);  // 0x108: 0x46f830 (pizzafrenzy)
};

} // namespace engine

#endif
