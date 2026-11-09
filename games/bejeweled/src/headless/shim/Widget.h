#ifndef __WIDGET_H__
#define __WIDGET_H__

// port(variant): headless shim of framework/Widget.h and WidgetContainer.h for the verifier
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the original game. Only what Board and the replay
// player use: the board tick mUpdateCnt, incremented exactly as WidgetContainer::Update does (Board::Update
// calls Widget::Update after its pause checks), the geometry Resize sets (UpdateLevelTransition reads mHeight), the
// widget flags, and the virtual input handlers Board overrides, with the framework's dispatch between the
// MouseDown/MouseUp overloads. Nothing is drawn and no widget manager runs them.

#include "Common.h"
#include "Color.h"
#include "Rect.h"
#include "Flags.h"
#include "Graphics.h"

namespace Sexy
{

class WidgetManager;
class Widget;

typedef std::list<Widget*> WidgetList;

class WidgetContainer
{
public:
	WidgetManager*		mWidgetManager;
	WidgetContainer*	mParent;
	int					mUpdateCnt;
	bool				mDirty;
	int					mX;
	int					mY;
	int					mWidth;
	int					mHeight;
	FlagsMod			mWidgetFlagsMod;
	int					mPriority;
	int					mZOrder;

	WidgetContainer() : mWidgetManager(NULL), mParent(NULL), mUpdateCnt(0), mDirty(false), mX(0), mY(0), mWidth(0), mHeight(0),
		mPriority(0), mZOrder(0) {}
	virtual ~WidgetContainer() {}

	virtual void		MarkDirty() { mDirty = true; }
	virtual void		Update() { mUpdateCnt++; }   // WidgetContainer::Update
	virtual void		Draw(Graphics* g) {}
};

class Widget : public WidgetContainer
{
public:
	bool				mVisible;
	bool				mMouseVisible;
	bool				mDisabled;
	bool				mHasFocus;
	bool				mIsDown;
	bool				mIsOver;
	bool				mHasTransparencies;
	bool				mDoFinger;
	bool				mWantsFocus;

	Widget() : mVisible(true), mMouseVisible(true), mDisabled(false), mHasFocus(false), mIsDown(false), mIsOver(false),
		mHasTransparencies(false), mDoFinger(false), mWantsFocus(false) {}
	virtual ~Widget() {}

	virtual void		Resize(int theX, int theY, int theWidth, int theHeight)
	{
		mX = theX;
		mY = theY;
		mWidth = theWidth;
		mHeight = theHeight;
		MarkDirty();
	}
	virtual void		Resize(const Rect& theRect) { Resize(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight); }
	virtual void		Draw(Graphics* g) {}
	virtual void		Update() { WidgetContainer::Update(); }   // Widget::Update
	virtual void		GotFocus() { mHasFocus = true; }
	virtual void		LostFocus() { mHasFocus = false; }
	virtual void		KeyChar(SexyChar theChar) {}
	virtual void		MouseEnter() {}
	virtual void		MouseLeave() {}
	virtual void		MouseMove(int x, int y) {}
	// The framework's dispatch (Widget.cpp): a click count below 0 is the right button, 3 the middle one
	virtual void		MouseDown(int x, int y, int theClickCount)
	{
		if (theClickCount == 3)
			MouseDown(x, y, 2, 1);
		else if (theClickCount >= 0)
			MouseDown(x, y, 0, theClickCount);
		else
			MouseDown(x, y, 1, -theClickCount);
	}
	virtual void		MouseDown(int x, int y, int theBtnNum, int theClickCount) {}
	virtual void		MouseUp(int x, int y) {}
	virtual void		MouseUp(int x, int y, int theLastDownButtonId)
	{
		MouseUp(x, y);
		if (theLastDownButtonId == 3)
			MouseUp(x, y, 2, 1);
		else if (theLastDownButtonId >= 0)
			MouseUp(x, y, 0, theLastDownButtonId);
		else
			MouseUp(x, y, 1, -theLastDownButtonId);
	}
	virtual void		MouseUp(int x, int y, int theBtnNum, int theClickCount) {}
	virtual void		MouseDrag(int x, int y) {}
};

}

#endif
