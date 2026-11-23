#ifndef __SPROUT2_EDITBOX_H__
#define __SPROUT2_EDITBOX_H__

// A line of text the player types (EditBox, 0x138 bytes, vtable 0x55daf4 (feedingfrenzy2) with 70 slots; its
// constructor 0x48f3d0 (feedingfrenzy2)): a container of a background image, the text and a blinking cursor image. While
// enabled it takes the engine's keys: printable characters go in at the cursor up to the length allowed, through the
// box's filter and only where its font has a glyph; Backspace and Delete take one out, Home, End and the arrows move
// the cursor, and Ctrl+V pastes the clipboard's text. Any of those keys shows the cursor and starts its blink again.

#include "Container.h"
#include "Font.h"
#include "Image.h"
#include "TextItem.h"
#include "sigslot.h"

#include <string>

namespace engine
{

class EditBox : public Container, public sigslot::HasSlots
{
public:
	// What the box's filter makes of a character typed or pasted: the character to put in, or 0 to refuse it
	typedef char (*CharFilter)(char theChar);

	// 0x48f3d0 (feedingfrenzy2): empty, with nothing to draw until setLook
	EditBox();

	// 0x68 (0x52031e (feedingfrenzy2))
	std::string getClassName() override { return "EditBox"; }
	// 0x90 (0x49ad60 (feedingfrenzy2)): the cursor shown and its blink begun, the keys taken (each connection made
	// afresh) and the box updated every frame. (The children are not enabled: they are plain images and text)
	void enable() override;
	// 0x94 (0x48cc70 (feedingfrenzy2)): the cursor hidden, the keys given up, no more updates
	void disable() override;
	// 0xa4 (0x48f1c0 (feedingfrenzy2)): the cursor's blink (shown for good when the blink time is 0); the children are
	// not updated
	void update(UpdateContext& theContext) override;

	// 0x48e900 (feedingfrenzy2): the background, the text item (in the font, left aligned) and the cursor added in
	// that order, and the font kept for the glyphs and the cursor's place
	void setLook(Surface* theBackground, Surface* theCursor, Font* theFont);
	// 0x508fbf (feedingfrenzy2): the text's colour, multiplied in
	void setTextColor(const Color& theColor);
	// The cursor's place with no text before it, and the cursor moved there and on by the text before it (0x407e10
	// (feedingfrenzy2))
	void setCursorOffset(const Vector2f& theOffset)
	{
		mCursorOffset = theOffset;
		updateCursor();
	}
	// The seconds the cursor stays shown or hidden, 0 for always shown (the reader sets both fields)
	void setBlinkTime(float theSeconds)
	{
		mBlinkTime = theSeconds;
		mBlinkLeft = theSeconds;
	}
	// 0x49fb60 (feedingfrenzy2): the text item's position
	void setTextOffset(const Vector2f& theOffset);
	void setMaxLength(int theLength) { mMaxLength = theLength; }
	// 0x49d5e0 (feedingfrenzy2): the text cut to the longest allowed, the cursor put after it
	void setText(const std::string& theText);
	// The action named by the screen's file (+0x110; nothing the export shows reads it)
	void setAction(const std::string& theAction) { mAction = theAction; }
	void setFilter(CharFilter theFilter) { mFilter = theFilter; }
	// 0x499ab0 (feedingfrenzy2)
	const std::string& getText() const { return mText; }

private:
	// 0x49dfd0 (feedingfrenzy2): the engine's key down
	void keyDown(int theKey);
	// 0x53bdf5 (feedingfrenzy2): a key held, as many key downs as the repeat's count
	void keyRepeat(int theKey, int theCount);
	// 0x48ba10 (feedingfrenzy2): the engine's character typed
	void charTyped(char theChar);
	// 0x53aa03 (feedingfrenzy2): Backspace, the character before the cursor taken out
	void deleteBack();
	// 0x53aa86 (feedingfrenzy2): Delete, the character after it
	void deleteForward();
	// 0x53a83d (feedingfrenzy2): Ctrl+V, the clipboard's text put in at the cursor, as much as there is room for
	void paste();
	// 0x494450 (feedingfrenzy2): the text item given the text, and the cursor moved
	void textChanged();
	// 0x407e10 (feedingfrenzy2): the cursor at its offset moved right by the width of the text before it
	void updateCursor();
	// The cursor shown and its blink started again (inline in each key's handling)
	void restartBlink();

	std::string mText;            // +0xc8
	int mMaxLength;               // +0xe4: 12 unless the screen's file says
	Ref<Image> mCursor;           // +0xe8
	Ref<TextItem> mTextItem;      // +0xec
	Ref<Font> mFont;              // +0xf0
	int mCursorIndex;             // +0xf4: the characters before the cursor
	Vector2f mCursorOffset;       // +0xf8
	float mBlinkTime;             // +0x100
	float mBlinkLeft;             // +0x104: until the cursor is shown or hidden again
	// +0x108 and +0x10c: zeroed by the constructor; nothing the export shows reads them
	std::string mAction;          // +0x110
	CharFilter mFilter;           // +0x12c: null for none
};

} // namespace engine

#endif
