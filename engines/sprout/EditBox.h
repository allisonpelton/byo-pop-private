#ifndef __SPROUT_EDITBOX_H__
#define __SPROUT_EDITBOX_H__

// The signal build's text entry (Pizza Frenzy's; Iggle Pop's Mac build names it EditBox: a container with slots; 0x1a8
// bytes, constructor 0x472ea0 (pizzafrenzy), vtable 0x5025a0 (pizzafrenzy)): a background image, the text, and a cursor
// image that follows the end of the text before the cursor, blinking. While it is connected (connect 0x88:
// 0x473060 (pizzafrenzy), disconnect 0x8c: 0x4728c0 (pizzafrenzy)) it takes the application's key and character
// signals.

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
	void setImages(const std::string& theBackgroundImage, const std::string& theCursorImage,
	               const std::string& theFont);
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
	// 0x471c80 (pizzafrenzy): a function that says which characters it takes, returning the character or 0
	void setCharFilter(char (*theFilter)(char)) { mCharFilter = theFilter; }
	// 0x4727f0 (pizzafrenzy): the text (cut to the most characters), the cursor after it
	void setText(const std::string& theText);
	const std::string& getText() const { return mText; }

	// 0x88: 0x473060 (pizzafrenzy): the cursor shows, and the box takes the key, repeat and character signals
	void connect() override;
	// 0x8c: 0x4728c0 (pizzafrenzy)
	void disconnect() override;
	// 0x94: 0x471fd0 (pizzafrenzy): the cursor blinks
	void update(UpdateInfo& theInfo) override;

private:
	// 0x472920 (pizzafrenzy): the cursor's keys, the delete keys and the paste
	void keyDown(int theKey);
	// 0x472a70 (pizzafrenzy): the key as many times as its count
	void keyRepeat(int theKey, int theCount);
	// 0x472240 (pizzafrenzy): a character goes in at the cursor when the box takes it
	void charTyped(int theChar);
	void eraseBeforeCursor();  // 0x4724d0 (pizzafrenzy)
	void eraseAfterCursor();   // 0x4723c0 (pizzafrenzy)
	void paste();              // 0x4725e0 (pizzafrenzy)
	void showCursor();         // the cursor shows and its blink starts again
	void textChanged();        // the text item shows the text, and the cursor follows it

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
	char (*mCharFilter)(char);      // +0x19c: null takes every character
};

} // namespace engine

#endif
