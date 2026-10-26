#ifndef __NINGPO_PGCONTROL_H__
#define __NINGPO_PGCONTROL_H__

// PGControl (0x30 bytes, vtable 0x463328, 0x411030..0x4113c0): a control of the game screen, drawn from up to three
// sprites (normal, lit while the mouse is over it, disabled). The app keeps its controls in a list (PGApp+0x6904),
// draws them after the screen in every dirty rectangle and hands them the events (PGApp::dispatchToControls). The
// game's tile buttons and icons are subclasses (vtables 0x462590, 0x4625e8).

#include "Pg.h"

namespace pg
{

class PGApp;
class PGSprite;

class PGControl
{
public:
	int mRefCount;              // +0x08 (HObject)
	bool mShownUnderDialogs;    // +0x0c slots 0x20/0x24: drawn while a dialog is up (PGApp::drawControls)
	bool mHighlighted;          // +0x0d slot 0x48: the mouse is over it
	bool mEnabled;              // +0x0e slots 0x28/0x2c: gets events; a disabled control shows its disabled sprite
	PGSprite* mLit;             // +0x10
	PGSprite* mDisabled;        // +0x14
	PGSprite* mNormal;          // +0x18
	PGApp* mOwner;              // +0x1c
	PGRect mRect;               // +0x20 slot 0x44

	PGControl();                // 0x411030
	virtual ~PGControl() {}

	bool isShownUnderDialogs() const { return mShownUnderDialogs; }   // 0x407bf0 (slot 0x20)
	void setShownUnderDialogs(bool theShown) { mShownUnderDialogs = theShown; }  // 0x4113c0 (slot 0x24)
	void setEnabled(bool theEnabled) { mEnabled = theEnabled; }       // 0x407c00 (slot 0x28)
	bool isEnabled() const { return mEnabled; }                       // 0x407c10 (slot 0x2c)
	// slot 0x30: the control's part inside theClip
	virtual void draw(const PGRect* theClip);
	// 0x4112a0 (slot 0x34)
	void setSprites(PGSprite* theNormal, PGSprite* theLit, PGSprite* theDisabled);
	void setOwner(PGApp* theOwner) { mOwner = theOwner; }             // 0x411290 (slot 0x38)
	// slot 0x3c: true when the control took the event
	virtual bool handleEvent(const PGEvent* theEvent);
	// 0x4112d0 (slot 0x40)
	void setPosition(const PGPoint* thePosition);
	const PGRect* getRect() const { return &mRect; }                  // 0x407c20 (slot 0x44)
	bool isHighlighted() const { return mHighlighted; }               // 0x407c30 (slot 0x48)
	// 0x411070 (slot 0x4c)
	void setHighlight(bool theHighlighted);
	// 0x411370 (slot 0x50): a left click inside the control
	bool isClick(const PGEvent* theEvent);
	// slot 0x54 of the game's buttons (vtable 0x4625e8)
	virtual void onClick() {}

	// HObject (0x407a40, 0x407a80, 0x407a90)
	int refCount() const { return mRefCount; }
	void addRef() { mRefCount++; }
	void releaseRef() { mRefCount--; }

private:
	// 0x4112f0: the rectangle from the position and the sprites' size
	void updateRect();
};

// The clipping that the original repeats inline wherever it draws a sprite at a rectangle (PGControl::draw 0x4110a0,
// the dialogs 0x411990, the loading screen and the cursor): theRect clipped to theClip, and theSrc (the sprite's
// whole rectangle on entry) cut by as much. False when they do not meet.
bool ClipToRect(PGRect* theRect, PGRect* theSrc, const PGRect* theClip);

} // namespace pg

#endif
