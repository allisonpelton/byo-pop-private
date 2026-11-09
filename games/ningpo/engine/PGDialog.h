#ifndef __NINGPO_PGDIALOG_H__
#define __NINGPO_PGDIALOG_H__

// PGDialog (vtable 0x462c60, 0x411480..0x411e00) and its items (0x411f50..0x413100). A dialog is a background sprite
// with a list of items (buttons, lines of text, images, sliders), run by doModal(): its own event loop, which draws
// the game screen under the dialogs (PGApp::renderDirty) and every open dialog over it (0x411da0) until an item
// click or the timeout ends it. The open dialogs are a stack (0x4c1d60); what they need redrawn is a list of dirty
// rectangles of their own (0x4c1d80) on top of the app's.

#include "Pg.h"

namespace pg
{

class HDisplay;
struct HFont;
class HInput;
class PGApp;
class PGDialog;
class PGSprite;

// The item base (vtable 0x462ca8, 0x138 bytes)
class PGDialogItem
{
public:
	int mRefCount;          // +0x08 (HObject)
	PGRect mRect;           // +0x0c slot 0x2c; the item's subclasses keep it in screen coordinates
	PGDialog* mDialog;      // +0x1c
	PGDialogItem* mNext;    // +0x20
	bool mVisible;          // +0x24 slots 0x20/0x24 (a hidden button shows its disabled sprite)
	char mName[0x100];      // +0x25 slots 0x3c/0x40
	int mUserData;          // +0x128 slots 0x34/0x38
	PGPoint mPosition;      // +0x12c relative to the dialog (but for images)

	PGDialogItem();         // 0x411f50
	// 0x412d70
	virtual ~PGDialogItem() {}

	bool isVisible() const { return mVisible; }               // 0x408950 (slot 0x20)
	void setVisible(bool theVisible) { mVisible = theVisible; }  // 0x408960 (slot 0x24)
	// slot 0x28: Tab can move the focus to it
	virtual bool canFocus() const { return false; }           // 0x4234f0
	const PGRect* getRect() const { return &mRect; }          // 0x408970 (slot 0x2c)
	// slot 0x30 (0x412480)
	virtual void setPosition(const PGPoint* thePosition);
	int getUserData() const { return mUserData; }             // 0x411fc0 (slot 0x34)
	void setUserData(int theData) { mUserData = theData; }    // 0x411fd0 (slot 0x38)
	void setName(const char* theName);                        // 0x411fa0 (slot 0x3c)
	const char* getName() const { return mName; }             // 0x411f90 (slot 0x40)
	// slot 0x44: the item's part inside theClip
	virtual void draw(HDisplay* theDisplay, const PGRect* theClip) {}  // 0x408980
	// slot 0x48: true when the event is for this item (a click on it, or Enter)
	virtual bool handleEvent(const PGEvent* theEvent) { return false; }  // 0x4088e0
	// slot 0x4c: mRect from the position
	virtual void updateRect() {}                              // 0x425eb0
	// 0x412d20 (slot 0x50): the dialog redraws theRect
	void invalidate(const PGRect* theRect);
	// 0x411ff0 (slot 0x54)
	void setOwner(PGDialog* theDialog);
	void setNext(PGDialogItem* theNext) { mNext = theNext; }  // 0x412020 (slot 0x58)
	PGDialogItem* getNext() const { return mNext; }           // 0x411fe0 (slot 0x5c)
};

// A button (vtable 0x463380, 0x144 bytes)
class PGDialogButton : public PGDialogItem
{
public:
	bool mHover;            // +0x134
	PGSprite* mNormal;      // +0x138
	PGSprite* mLit;         // +0x13c
	PGSprite* mDisabled;    // +0x140

	PGDialogButton();       // 0x412030

	void draw(HDisplay* theDisplay, const PGRect* theClip) override;  // 0x412240
	bool handleEvent(const PGEvent* theEvent) override;              // 0x412080
	void updateRect() override;                                      // 0x412190
	// 0x412150 (slot 0x60)
	void setSprites(PGSprite* theNormal, PGSprite* theLit, PGSprite* theDisabled);
};

// A line of text, editable or not (vtable 0x462d98, 0x560 bytes)
class PGDialogText : public PGDialogItem
{
public:
	int mAlign;             // +0x134 slot 0x6c: HDisplay::drawText's
	float mColor[4];        // +0x138 slot 0x68
	bool mEditable;         // +0x148 slot 0x60
	int mMaxLength;         // +0x14c slot 0x70
	int mVisibleLength;     // +0x150 slots 0x70/0x74: how many of the last characters show
	char mText[0x400];      // +0x154
	int mLength;            // +0x554
	HFont* mFont;           // +0x558 slot 0x64
	int mFlags;             // +0x55c slots 0x78/0x7c: TEXT_BLINK

	enum
	{
		TEXT_BLINK = 1,
	};

	PGDialogText();         // 0x4124a0

	bool canFocus() const override { return mEditable; }             // 0x40a9f0
	void draw(HDisplay* theDisplay, const PGRect* theClip) override;  // 0x412910
	bool handleEvent(const PGEvent* theEvent) override;              // 0x412680
	void setEditable(bool theEditable) { mEditable = theEditable; }  // 0x40aa00
	void setFont(HFont* theFont) { mFont = theFont; }                // 0x412670
	void setColor(const float* theColor);                            // 0x412590
	void setAlign(int theAlign) { mAlign = theAlign; }               // 0x40aa10
	void setMaxLength(int theLength) { mMaxLength = theLength; mVisibleLength = theLength; }  // 0x412560
	void setVisibleLength(int theLength) { mVisibleLength = theLength; }  // 0x412580
	void setFlags(int theFlags) { mFlags = theFlags; }               // 0x412900
	int getFlags() const { return mFlags; }                          // 0x40aa20
	// 0x4125c0
	void setText(const char* theText);
	const char* getText() const { return mText; }
};

// An image, shown in part as a progress bar (vtable 0x4633e8, 0x13c bytes); its position is the screen's
class PGDialogImage : public PGDialogItem
{
public:
	float mProgress;        // +0x134 the part of the width shown
	PGSprite* mSprite;      // +0x138

	PGDialogImage();        // 0x412a50

	void setPosition(const PGPoint* thePosition) override;           // 0x412af0
	void draw(HDisplay* theDisplay, const PGRect* theClip) override;  // 0x412ba0
	void setSprite(PGSprite* theSprite);                             // 0x412a90 (slot 0x60)
	// 0x412b40 (slot 0x64)
	void setProgress(float theProgress);
	float getProgress() const { return mProgress; }                  // 0x412ae0 (slot 0x68)
};

// A slider: a knob dragged along a line (vtable 0x463458, 0x148 bytes)
class PGDialogSlider : public PGDialogItem
{
public:
	bool mDragging;         // +0x134
	int mLength;            // +0x138 in pixels
	int mVertical;          // +0x13c
	float mValue;           // +0x140 0..1
	PGSprite* mKnob;        // +0x144

	PGDialogSlider();       // 0x412d30

	void draw(HDisplay* theDisplay, const PGRect* theClip) override;  // 0x412ee0
	bool handleEvent(const PGEvent* theEvent) override;              // 0x412da0
	void updateRect() override;                                      // 0x413100
	// 0x412d80 (slot 0x60)
	void setRange(int theVertical, int theLength) { mLength = theLength; mVertical = theVertical; }
	// 0x4131b0 (slot 0x64)
	virtual void setValue(float theValue);
	float getValue() const { return mValue; }                        // 0x40bd30 (slot 0x68)
	// 0x4130e0 (slot 0x6c)
	void setKnob(PGSprite* theKnob);
};

class PGDialog;
// port: the open dialog on top, or nullptr when none is open (the test commands, src/TestCommands.cpp)
PGDialog* TopDialog();

class PGDialog
{
public:
	int mRefCount;              // +0x08 (HObject)
	PGApp* mApp;                // +0x0c
	int mParam;                 // +0x10 doModal's last argument
	PGPoint mPosition;          // +0x14
	PGRect mRect;               // +0x1c the background's
	PGPoint mMouse;             // +0x2c
	PGDialogItem* mFirstItem;   // +0x34
	PGDialogItem* mFocus;       // +0x38
	PGDialogItem* mDefault;     // +0x3c Enter clicks it
	PGSprite* mBackground;      // +0x40

	// What doModal() returns when no item ended it
	static PGDialogItem* const TIMED_OUT;   // -1
	static PGDialogItem* const CLOSED;      // -2: the window was closed

	PGDialog();                 // 0x411480
	virtual ~PGDialog() {}

	// 0x411cc0 (slot 0x20)
	void setBackground(PGSprite* theBackground);
	// 0x4116e0 PGDialog::doModal (slot 0x24): the item clicked, or TIMED_OUT after theTimeout milliseconds when
	// onTimeout() agrees, or CLOSED
	PGDialogItem* doModal(PGApp* theApp, HDisplay* theDisplay, HInput* theInput, int theTimeout, int theParam);
	// slot 0x28: true ends doModal()
	virtual bool onItemClicked(PGDialogItem* theItem) { return false; }  // 0x4088e0
	// slot 0x2c: true ends doModal() once the timeout has passed (it is asked again at every pass after that)
	virtual bool onTimeout() { return false; }                           // 0x4234f0
	// 0x411510 (slot 0x30)
	void addItem(PGDialogItem* theItem);
	// 0x4114e0 (slot 0x34): the items and the background, forgotten (the game owns them)
	void removeAll();
	void setDefault(PGDialogItem* theItem) { mDefault = theItem; }       // 0x4114d0 (slot 0x38)
	// 0x411e00 (slot 0x3c)
	void invalidate(const PGRect* theRect);
	// 0x411580 (slot 0x40): false when the window was closed
	virtual bool handleEvent(const PGEvent* theEvent);
	// 0x411990 (slot 0x44)
	virtual void draw(HDisplay* theDisplay);
	// 0x411ca0
	void setPosition(const PGPoint* thePosition);
	// 0x411d20: the focus to theItem, or to the first item
	void setFocus(PGDialogItem* theItem);

protected:
	// 0x411cd0
	void updateRect();
	// 0x411920: the item the event is for
	PGDialogItem* hitTest(const PGEvent* theEvent);
};

// 0x411dd0: theRect is redrawn, under the dialogs and in them
void InvalidateDialogRect(const PGRect* theRect);
// 0x411da0: every open dialog, the lowest first
void DrawDialogs(HDisplay* theDisplay);

} // namespace pg

#endif
