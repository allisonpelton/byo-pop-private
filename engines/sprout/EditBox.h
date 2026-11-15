#ifndef __SPROUT_EDITBOX_H__
#define __SPROUT_EDITBOX_H__

// The signal build's text entry (Pizza Frenzy's; Iggle Pop's Mac build names it EditBox: a container with slots; 0x1a8
// bytes, constructor 0x472ea0 (pizzafrenzy), vtable 0x5025a0 (pizzafrenzy)): a background image, the text, and a cursor
// image that follows the end of the text before the cursor, blinking. Only what the layout reader reaches is here: its
// constructor and setters. Its connection to the application's key and update signals (connect 0x88:
// 0x473060 (pizzafrenzy), disconnect 0x8c: 0x4728c0 (pizzafrenzy), update 0x94: 0x471fd0 (pizzafrenzy), the key
// handlers) are not translated, nothing reaches them yet.

#include "Font.h"
#include "ImageNode.h"
#include "Node.h"
#include "Signal.h"
#include "TextItem.h"
#include <string>

namespace engine
{

class EditBox : public Container, public HasSlots
{
public:
	EditBox();  // 0x472ea0 (pizzafrenzy)

	// 0x471db0 (pizzafrenzy): the background image, the text (left aligned, in the font) and the cursor image, in that
	// order, each a child; the images and the font come from the archive by their names
	void setImages(const std::string& theBackgroundImage, const std::string& theCursorImage, const std::string& theFont);
	// 0x472110 (pizzafrenzy): the action the box tells when it is done
	void setAction(const std::string& theAction) { mAction = theAction; }
	// 0x472220 (pizzafrenzy): where the cursor is from the end of the text before it
	void setCursorOffset(const Point& theOffset);
	// 0x471c60 (pizzafrenzy): the seconds the cursor takes to blink, which also starts the count again
	void setCursorBlinkTime(float theSeconds)
	{
		mBlinkTime = theSeconds;
		mBlinkTimeLeft = theSeconds;
	}
	// 0x471fc0 (pizzafrenzy): the text's position in the box
	void setTextOffset(const Point& theOffset);
	// 0x471c50 (pizzafrenzy): the most characters it takes
	void setMaxLength(int theLength) { mMaxLength = theLength; }

private:
	// 0x472130 (pizzafrenzy): the cursor image after the width of the text up to the cursor, plus its offset
	void updateCursor();

	std::string mText;              // +0x138
	int mMaxLength;                 // +0x154
	Ref<ImageNode> mCursor;         // +0x158
	Ref<TextItem> mTextItem;        // +0x15c
	Ref<Font> mFont;                // +0x160
	int mCursorIndex;               // +0x164: how many characters are before the cursor
	Point mCursorOffset;            // +0x168
	float mBlinkTime;               // +0x170
	float mBlinkTimeLeft;           // +0x174
	Point mTextOffset;              // +0x178: only the constructor sets it; the text's position is set by setTextOffset
	std::string mAction;            // +0x180
	// +0x19c: set to 0 by the constructor, nothing translated reads it (the blink's state?)
	int mCursorShown;
};

} // namespace engine

#endif
