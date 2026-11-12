#ifndef __BONNIES_WIDGET_H__
#define __BONNIES_WIDGET_H__

// The widgets (0x42b7a0-0x42f700): the base (the vtable 0x459940) and its seven types, which a screen deletes by
// mType: 0 a text field, 1 a button, 2 a picture, 3 a check box, 4 a slider, 5 a list, 6 a plain area. A widget's
// position is relative to its screen (mOwner). They talk to the game through the event queue (Events.h).

#include "Str.h"

namespace bonnies
{

struct Event;
struct Font;
struct FontDesc;
struct Image;
struct Sample;
class Screen;

// mFlags
enum
{
	WIDGET_HIDDEN = 1, WIDGET_PRESSED = 2, WIDGET_FOCUS = 4, WIDGET_INACTIVE = 8, WIDGET_BEVEL = 0x10,
	WIDGET_PANEL = 0x20, WIDGET_IMAGE = 0x40, WIDGET_FOCUS_TEXT = 0x80, WIDGET_FOCUS_PANEL = 0x100,
	WIDGET_FLAT_PRESS = 0x200
};

class Widget
{
public:
	// 0x42c240
	Widget();
	// 0x42c290
	virtual ~Widget();

	// slot 0, 0x42c2f0
	virtual void init();
	// slot 1, 0x42c370: loses the focus and frees the image, font and sounds
	virtual void shutdown();
	// slot 2, 0x42c670: its screen is visible and it is not hidden
	virtual int isVisible();
	// slot 3, 0x42c6a0: visible and not inactive
	virtual int isActive();
	// slot 4, 0x42c6c0
	virtual int hasFocus();
	// slots 5, 6, 0x42b9a0, 0x42b980
	virtual int contains(float theX, float theY);
	virtual int contains(float theX, float theY, int theCheck);
	// slot 7, 0x42c490: whether the point is in the widget (its hit rectangle when it has one), theMargin around it;
	// with theCheck only buttons, lists, check boxes and sliders answer
	virtual int contains(float theX, float theY, int theCheck, float theMargin);
	// slot 8, 0x42c5c0: the debug rectangles
	virtual void draw();
	// slot 9, 0x41f560
	virtual void update();
	// slot 10, 0x42c6d0: the focus follows the mouse, and a click is a press and a release; 0 when consumed
	virtual int handleEvent(Event* theEvent);
	// slots 11, 12, 0x42c830, 0x42c890: the focus, and the tooltip on the status line
	virtual void focus();
	virtual void unfocus();
	// slot 14, 0x42c3e0
	virtual void setFont(FontDesc* theDesc);

	// 0x42c7b0: the sound of a press (0), release (1) or toggle (2)
	void setSound(int theIndex, const Str& theName);

	int mId;                  // +0x04
	float mX;                 // +0x08
	float mY;                 // +0x0c
	float mWidth;             // +0x10
	float mHeight;            // +0x14
	float mHitX;              // +0x18: the hit rectangle, relative to mX, mY (none when mHitX and mHitY are 0)
	float mHitY;              // +0x1c
	float mHitWidth;          // +0x20
	float mHitHeight;         // +0x24
	uint32_t mFlags;          // +0x28
	uint32_t mTextFlags;      // +0x2c
	uint32_t mColor;          // +0x30
	uint32_t mColor2;         // +0x34
	uint32_t mFocusColor;     // +0x38 (0: mColor brightened)
	uint32_t mFocusColor2;    // +0x3c
	int mPriority;            // +0x40
	int mKeys[4];             // +0x44: the keys that press it
	Sample* mSounds[3];       // +0x54
	int mType;                // +0x60
	Str mText;                // +0x64
	Str mTooltip;             // +0x6c
	Image* mImage;            // +0x74
	Font* mFont;              // +0x78
	Widget* mNext;            // +0x7c
	Widget* mPrev;            // +0x80
	Screen* mOwner;           // +0x84
};

// Type 0 (0x42c900, the vtable 0x45997c): a line of text edited in place
class TextField : public Widget
{
public:
	// 0x42c900
	TextField();
	// 0x42c960
	~TextField() override;

	// slot 0, 0x42c9d0
	void init() override;
	// slot 5, 0x42ca60: only when editable, with a margin of 0.015
	int contains(float theX, float theY) override;
	// slot 8, 0x42ca90
	void draw() override;
	// slot 9, 0x42d6a0
	void update() override;
	// slot 10, 0x42ce90: a click begins editing; while editing, the keys edit
	int handleEvent(Event* theEvent) override;
	using Widget::contains;

	// 0x42cbd0: the text shown, with a blinking '|' at the cursor (theIgnored is never read)
	void refreshDisplay(int theIgnored = 0);
	// 0x42cd10
	void beginEdit();
	// 0x42cd70: deletes the selected text; whether there was any
	int deleteSelection();

	int mEditing;             // +0x88
	int mEscapeCancels;       // +0x8c
	int mTabEnds;             // +0x90
	int mEditable;            // +0x94
	int mNamesOnly;           // +0x98: letters, digits, blanks and '_' only
	Str mOriginal;            // +0x9c: the text before editing
	Str mDisplay;             // +0xa4
	int mCursor;              // +0xac
	int mMaxLength;           // +0xb0 (30)
	uint32_t mPanelColor;     // +0xb4
	uint32_t mBlinkTime;      // +0xb8
	int mBlinkOn;             // +0xbc
	int mAnchor;              // +0xc0: the selection's other end
	int mSelecting;           // +0xc4
};

// Type 1 (0x42b7a0, the vtable 0x459900)
class Button : public Widget
{
public:
	// 0x42b7a0
	Button();
	// 0x42b810
	~Button() override;

	// slot 0, 0x42b890
	void init() override;
	// slot 1, 0x42b930
	void shutdown() override;
	// slot 8, 0x42b9c0
	void draw() override;
	// slot 9, 0x42be60: a key press ends when no key is down, a mouse press when the button is up
	void update() override;
	// slot 10, 0x42bec0
	int handleEvent(Event* theEvent) override;

	// 0x42bd90, 0x42bdf0
	void press();
	void release();
	// 0x42c000, 0x42c0c0, 0x42c180: the images and their frames when up, in focus and pressed
	void setUpImage(const Str& theName, int theFrame);
	void setFocusImage(const Str& theName, int theFrame);
	void setDownImage(const Str& theName, int theFrame);

	Str mUpName;              // +0x88
	Str mFocusName;           // +0x90
	Str mDownName;            // +0x98
	Image* mUpImage;          // +0xa0
	Image* mFocusImage;       // +0xa4
	Image* mDownImage;        // +0xa8
	int mUpFrame;             // +0xac
	int mFocusFrame;          // +0xb0
	int mDownFrame;           // +0xb4
	uint32_t mPanelColor;     // +0xb8
	uint32_t mEdgeColor;      // +0xbc
	uint32_t mFocusPanelColor;  // +0xc0 (0: mPanelColor brightened)
	float mTextX;             // +0xc4
	float mTextY;             // +0xc8
	float mTextWidth;         // +0xcc: the text's width when not 0
	float mTextHeight;        // +0xd0
	int mKeyPressed;          // +0xd4
	int mMousePressed;        // +0xd8
};

// Type 3 (0x42d6c0, the vtable 0x4599b8)
class CheckBox : public Widget
{
public:
	// 0x42d6c0
	CheckBox();
	// 0x42d6e0
	~CheckBox() override;

	// slot 0, 0x42d6f0
	void init() override;
	// slot 8, 0x42d7a0
	void draw() override;
	// slot 9, 0x42d750
	void update() override;
	// slot 10, 0x42d980
	int handleEvent(Event* theEvent) override;

	// 0x42d720
	void toggle();
	// 0x42d940
	void press();

	float mBoxWidth;          // +0x88 (0.1)
	float mBoxHeight;         // +0x8c (0.1)
	int mChecked;             // +0x90
};

// Type 4 (0x42d9f0, the vtable 0x459a04)
class Slider : public Widget
{
public:
	// 0x42d9f0
	Slider();
	// 0x42da40
	~Slider() override;

	// slot 0, 0x42daa0
	void init() override;
	// slot 1, 0x42db00
	void shutdown() override;
	// slot 8, 0x42db50
	void draw() override;
	// slot 9, 0x42db20
	void update() override;
	// slot 10, 0x42dfd0
	int handleEvent(Event* theEvent) override;

	// 0x42dd00: follows the mouse (in mSteps steps, at most mMaxStep at a time unless the mouse moved fast)
	void track();
	// 0x42df10: the knob's image
	void setKnob(const Str& theName, int theFrame);

	float mValue;             // +0x88: 0 to 1
	uint32_t mTrackColor;     // +0x8c
	uint32_t mKnobColor;      // +0x90
	int mSteps;               // +0x94
	int mMaxStep;             // +0x98
	Str mKnobName;            // +0x9c
	Image* mKnobImage;        // +0xa4
	int mKnobFrame;           // +0xa8
};

// A list's item (0x10 bytes, 0x42f660)
struct ListItem
{
	ListItem();
	ListItem& operator=(const ListItem& theItem);  // 0x42f6d0

	Str mText;                // +0x00
	int mData;                // +0x08
	int mSpare;               // +0x0c: copied with the item; nothing reads it
};

// Type 5 (0x42e070, the vtable 0x459a4c)
class ListBox : public Widget
{
public:
	// 0x42e070
	ListBox();
	// 0x42e090
	~ListBox() override;

	// slot 0, 0x42e0a0
	void init() override;
	// slot 1, 0x42e170
	void shutdown() override;
	// slot 8, 0x42e690
	void draw() override;
	// slot 9, 0x42e7e0: scrolls while the mouse is held above or below
	void update() override;
	// slot 10, 0x42e2a0
	int handleEvent(Event* theEvent) override;

	// 0x42e1b0
	void scroll(int theRows);
	// 0x42e200: selects theIndex and scrolls to it
	void select(int theIndex);
	// 0x42e3d0, 0x42e510: inserts an item before theIndex (at the end when past it)
	void insert(const ListItem& theItem, int theIndex);
	void insert(const Str& theText, int theData, int theIndex);
	// 0x42e5b0
	void remove(int theIndex);
	// 0x42e660
	void clear();

	ListItem* mItems;         // +0x88
	int mRows;                // +0x8c (4): the rows shown
	int mCount;               // +0x90
	int mTop;                 // +0x94
	int mSelected;            // +0x98 (-1)
	float mListX;             // +0x9c
	float mListY;             // +0xa0
	float mRowWidth;          // +0xa4
	float mRowHeight;         // +0xa8
	uint32_t mClickTime;      // +0xac
	int mTracking;            // +0xb0
	uint32_t mScrollTime;     // +0xb4
};

// Type 6 (0x42e910, the vtable 0x459a90): an area that only takes the focus and clicks
class Area : public Widget
{
public:
	// 0x42e910
	Area();
	// 0x42e930
	~Area() override;

	// slot 0, 0x42e940
	void init() override;
};

} // namespace bonnies

#endif
