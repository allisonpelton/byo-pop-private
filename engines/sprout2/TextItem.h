#ifndef __SPROUT2_TEXTITEM_H__
#define __SPROUT2_TEXTITEM_H__

// A component that draws text in its font (TextItem, 0xf0 bytes, vtable 0x55c32c (feedingfrenzy2); its constructor
// 0x431330 (feedingfrenzy2)): lines broken at each "\n" (a backslash and an n in the text itself) and, with a wrap
// width, at the last space before a line grows to it. The lines are placed by the alignment around the position,
// the first moved right by its indent and each later one by the other indent more.

#include "Component.h"

#include <string>
#include <vector>

namespace engine
{

// What a text item draws besides its text (+0xcc)
enum TextItemFlags
{
	TEXT_ITEM_UNDERLINE = 1,   // a line along each line's foot
	TEXT_ITEM_STRIKE = 2,      // a line through each line's middle
	TEXT_ITEM_SHADOW = 4       // the text again first, darkened, moved by the shadow's offset and faded by its opacity
};

class TextItem : public Component
{
public:
	// 0x431330 (feedingfrenzy2): visible, empty, left and top aligned, a half-opaque shadow 2 pixels down and right
	TextItem();

	// 0x68 (0x52300c (feedingfrenzy2))
	std::string getClassName() override { return "TextItem"; }
	// 0xa0 (0x4471b0 (feedingfrenzy2)): the shadow and the lines, when there is text
	void paint(DrawContext* theContext) override;
	// 0xa8 (0x447910 (feedingfrenzy2)): the widest line and the lines' height, placed by the alignment around the
	// position (the scale not applied)
	void computeBounds() override;

	// 0xf0 (0x44df00 (feedingfrenzy2)): the lines drawn in the context's state, which is left moved to the last one
	virtual void paintLines(DrawContext* theContext);

	// 0x44b4f0 (feedingfrenzy2): the text, broken into lines in the item's font
	void setText(const std::string& theText);
	const std::string& getText() const { return mText; }
	// Set where the original writes the fields (the screen's text element, the bubble glyph)
	void setAlignment(int theAlignH, int theAlignV)
	{
		setAlignH(theAlignH);
		setAlignV(theAlignV);
	}
	void setAlignH(int theAlignH)
	{
		mAlignH = theAlignH;
		addState(STATE_BOUNDS_DIRTY);
	}
	void setAlignV(int theAlignV)
	{
		mAlignV = theAlignV;
		addState(STATE_BOUNDS_DIRTY);
	}
	void addFlags(int theFlags) { mFlags |= (unsigned char)theFlags; }
	void setWrapWidth(int theWidth) { mWrapWidth = theWidth; }
	void setShadow(float theOpacity, const Vector2f& theOffset)
	{
		mShadowOpacity = theOpacity;
		mShadowX = theOffset.x;
		mShadowY = theOffset.y;
	}

private:
	std::string mText;                 // +0x98
	std::vector<std::string> mLines;   // +0xb4
	int mAlignH;                       // +0xc4: TextAlignH
	int mAlignV;                       // +0xc8: TextAlignV
	unsigned char mFlags;              // +0xcc: TextItemFlags
	float mFirstIndent;                // +0xd0
	float mIndent;                     // +0xd4: added for each line after the first
	int mWrapWidth;                    // +0xd8: 0 or less for no wrapping
	float mShadowOpacity;              // +0xdc
	float mShadowX;                    // +0xe0
	float mShadowY;                    // +0xe4
};

} // namespace engine

#endif
