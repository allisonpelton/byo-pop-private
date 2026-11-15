#ifndef __SPROUT_LISTBOX_H__
#define __SPROUT_LISTBOX_H__

// The signal build's list of rows (Pizza Frenzy's; Iggle Pop's Mac build names it engine::ListBox: a component with
// slots; 0x1b0 bytes, constructor 0x491ea0 (pizzafrenzy), vtable 0x504738 (pizzafrenzy)): rows of text in a size, in
// colours, with an indent, that the mouse picks from (the sounds are for the mouse entering a row and pressing one).
// Only what the layout reader reaches is here: the constructor and the setters. Its drawing (0x90:
// 0x4919d0 (pizzafrenzy)), layout (0x98: 0x491720 (pizzafrenzy)), mouse events (0xa0: 0x491c00 (pizzafrenzy), 0xd0:
// 0x491930 (pizzafrenzy), 0xe4: 0x491750 (pizzafrenzy)), its list model and its signal (+0x118, which the constructor
// makes) are not translated, nothing reaches them yet, so a list box draws nothing here.

#include "Color4.h"
#include "Node.h"
#include "Signal.h"

namespace engine
{

class Sound;

class ListBox : public Node, public HasSlots
{
public:
	ListBox();  // 0x491ea0 (pizzafrenzy)

	// 0x491590 (pizzafrenzy)
	void setSize(float theWidth, float theHeight) { mSize = Point(theWidth, theHeight); }
	// Each colour with its getter (the layout reader reads its attribute with the colour it has as the default)
	void setBackgroundColor(const Color4& theColor) { mBackgroundColor = theColor; }   // 0x4915e0 (pizzafrenzy)
	const Color4& getBackgroundColor() const { return mBackgroundColor; }              // 0x491610 (pizzafrenzy)
	void setTextColor(const Color4& theColor) { mTextColor = theColor; }               // 0x4915a0 (pizzafrenzy)
	const Color4& getTextColor() const { return mTextColor; }                          // 0x4915d0 (pizzafrenzy)
	void setHighlightColor(const Color4& theColor) { mHighlightColor = theColor; }     // 0x491620 (pizzafrenzy)
	const Color4& getHighlightColor() const { return mHighlightColor; }                // 0x491650 (pizzafrenzy)
	// 0x491660 (pizzafrenzy)
	void setSelectionBackgroundColor(const Color4& theColor) { mSelectionBackgroundColor = theColor; }
	// 0x491690 (pizzafrenzy)
	const Color4& getSelectionBackgroundColor() const { return mSelectionBackgroundColor; }
	// 0x4916a0 (pizzafrenzy)
	void setSelectionTextColor(const Color4& theColor) { mSelectionTextColor = theColor; }
	// 0x4916d0 (pizzafrenzy)
	void setSelectionHighlightColor(const Color4& theColor) { mSelectionHighlightColor = theColor; }
	// 0x491700 (pizzafrenzy)
	void setIndent(int theIndent) { mIndent = theIndent; }
	// 0x491760 (pizzafrenzy): the sounds for the mouse entering a row and pressing one, loaded by their names (an empty
	// name is none)
	void setSounds(const std::string& theEnterSound, const std::string& thePressSound);

	// 0x90: 0x4919d0 (pizzafrenzy), not translated
	void draw(Graphics* g) override {}
	// 0x98: 0x491720 (pizzafrenzy), not translated
	void layout() override {}

	Point mSize;                       // +0x128
	Color4 mBackgroundColor;           // +0x130
	Color4 mTextColor;                 // +0x140
	Color4 mHighlightColor;            // +0x150
	Color4 mSelectionBackgroundColor;  // +0x160
	Color4 mSelectionTextColor;        // +0x170
	Color4 mSelectionHighlightColor;   // +0x180
	int mIndent;                       // +0x190
	int mScrollOffset;                 // +0x194: set to 0 by the constructor, nothing translated reads it
	int mSelectedIndex;                // +0x198: -1 by the constructor, nothing translated reads it
	int mHoverIndex;                   // +0x19c: -1 by the constructor, nothing translated reads it
	Sound* mEnterSound;                // +0x1a0
	Sound* mPressSound;                // +0x1a4
};

} // namespace engine

#endif
