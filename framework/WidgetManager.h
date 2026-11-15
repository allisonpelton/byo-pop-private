#ifndef __WIDGETMANAGER_H__
#define __WIDGETMANAGER_H__

#include "Common.h"
#include "KeyCodes.h"
#include "WidgetContainer.h"

namespace Sexy
{

class Widget;
class Image;
class MemoryImage;
class SexyAppBase;
class Graphics;

typedef std::list<Widget*> WidgetList;

enum
{
	WIDGETFLAGS_UPDATE			= 1,
	WIDGETFLAGS_MARK_DIRTY		= 2,
	WIDGETFLAGS_DRAW			= 4,
	WIDGETFLAGS_CLIP			= 8,	
	WIDGETFLAGS_ALLOW_MOUSE		= 16,
	WIDGETFLAGS_ALLOW_FOCUS		= 32,	
};

class PreModalInfo
{
public:
	Widget*					mBaseModalWidget;
	Widget*					mPrevBaseModalWidget;
	Widget*					mPrevFocusWidget;
	FlagsMod				mPrevBelowModalFlagsMod;
};

typedef std::list<PreModalInfo> PreModalInfoList;

typedef std::vector<std::pair<Widget*, int> > DeferredOverlayVector;

// port: off (the 1.x framework's WidgetManager draws each widget with a Graphics whose fast stretch is on in a 2D run, so
// every stretch a widget draws samples the source in 16.16 steps). On, the widgets' Graphics keep their default (fast stretch
// off), as the 2004 frameworks' WidgetManager has it (no SetFastStretch anywhere in its drawing: AstroPop's only caller of
// Graphics::SetFastStretch 0x4af630 is the level-up text's own draw), so a stretch averages unless the game asks for the fast
// one. A game sets it once, before it draws.
extern bool gWidgetsDrawWithoutFastStretch;

class WidgetManager : public WidgetContainer
{
public:	
	Widget*					mDefaultTab;
	
	Graphics*				mCurG;
	SexyAppBase*			mApp;
	MemoryImage*			mImage;	
	MemoryImage*			mTransientImage;
	bool					mLastHadTransients;	
	Widget*					mPopupCommandWidget;	
	DeferredOverlayVector	mDeferredOverlayWidgets;
	int						mMinDeferredOverlayPriority;
	
	bool					mHasFocus;
	Widget*					mFocusWidget;
	Widget*					mLastDownWidget;
	Widget*					mOverWidget;
	Widget*					mBaseModalWidget;
	FlagsMod				mLostFocusFlagsMod;
	FlagsMod				mBelowModalFlagsMod;
	FlagsMod				mDefaultBelowModalFlagsMod;
	PreModalInfoList		mPreModalInfoList;
	Rect					mMouseDestRect;
	Rect					mMouseSourceRect;
	bool					mMouseIn;
	int						mLastMouseX;
	int						mLastMouseY;
	int						mDownButtons;
	int						mActualDownButtons;
	int						mLastInputUpdateCnt;
	// port: the 2004 framework's last-press time (+0x168 there; docs/FRAMEWORK_2004.md): the app clock
	// (SexyAppBase::GetClockTime) at the last MouseDown, 0 before the first. BookWorm's idle test reads it.
	DWORD					mLastMouseDownTick;
	
	bool					mKeyDown[0xFF];
	int						mLastDownButtonId;

	int						mWidgetFlags;

	// port: one widget may take every mouse and key event for itself (SetInputCapture): the mod menu while it is open
	// (SexyAppBase::OpenModMenu). The events then reach it alone, and what a game reads between them stays as it was:
	// mLastMouseX, mLastMouseY, mMouseIn, mDownButtons, mActualDownButtons, mKeyDown, mLastDownButtonId,
	// mLastInputUpdateCnt, mLastMouseDownTick, mOverWidget and mLastDownWidget. A game that polls the buttons itself
	// (Venice fires on mDownButtons, docs/MOD_MENU_DESIGN.md 5.4) sees none held. The capture keeps its own pointer,
	// buttons and keys (IsInputKeyDown).
	Widget*					mCaptureWidget;
	int						mCaptureMouseX;
	int						mCaptureMouseY;
	bool					mCaptureMouseIn;
	int						mCaptureDownButtons;		// as mDownButtons: 1 left, 2 right, 4 middle
	bool					mCaptureKeyDown[0xFF];
	// a key held when the capture ended: its press went to the capture's widget, and its repeats reach no other (its
	// release does)
	bool					mHeldPastCapture[0xFF];

protected:
	int						GetWidgetFlags();
	void					MouseEnter(Widget* theWidget);
	void					MouseLeave(Widget* theWidget);
	void					CaptureMouse(int x, int y);

protected:
	void					SetBaseModal(Widget* theWidget, const FlagsMod& theBelowFlagsMod);

public:
	WidgetManager(SexyAppBase* theApplet);
	virtual ~WidgetManager();
	
	void					FreeResources();		
	void					AddBaseModal(Widget* theWidget, const FlagsMod& theBelowFlagsMod);
	void					AddBaseModal(Widget* theWidget);
	void					RemoveBaseModal(Widget* theWidget);
	void					Resize(const Rect& theMouseDestRect, const Rect& theMouseSourceRect);
	void					DisableWidget(Widget* theWidget);	
	Widget*					GetAnyWidgetAt(int x, int y, int* theWidgetX, int* theWidgetY);
	Widget*					GetWidgetAt(int x, int y, int* theWidgetX, int* theWidgetY);
	void					SetFocus(Widget* aWidget);
	void					GotFocus();	
	void					LostFocus();	
	void					InitModalFlags(ModalFlags* theModalFlags);
	void					DrawWidgetsTo(Graphics* g);
	void					DoMouseUps(Widget* theWidget, ulong theDownCode);	
	void					DoMouseUps();
	void					DeferOverlay(Widget* theWidget, int thePriority);
	void					FlushDeferredOverlayWidgets(int theMaxPriority);
	
	bool					DrawScreen();
	bool					UpdateFrame();				
	bool					UpdateFrameF(float theFrac);
	void					SetPopupCommandWidget(Widget* theList);
	void					RemovePopupCommandWidget();	
	void					MousePosition(int x, int y);	
	void					RehupMouse();
	void					RemapMouse(int& theX, int& theY);
	bool					MouseUp(int x, int y, int theClickCount);
	bool					MouseDown(int x, int y, int theClickCount);
	bool					MouseMove(int x, int y);
	bool					MouseDrag(int x, int y);
	bool					MouseExit(int x, int y);
	void					MouseWheel(int theDelta);
	bool					KeyChar(SexyChar theChar);
	bool					KeyDown(KeyCode key);
	bool					KeyUp(KeyCode key);

	bool					IsLeftButtonDown();
	bool					IsMiddleButtonDown();
	bool					IsRightButtonDown();

	// port: theWidget takes every event from now on (see mCaptureWidget), NULL gives them back. Taking them lets go of
	// what the widgets had: the one under the pointer is left, the buttons come up and the keys come up, as a lost
	// focus does; the keys held stay held for the capture. Giving them back leaves the pointer where the capture saw it
	// last; RehupMouse then tells the widgets under it.
	void					SetInputCapture(Widget* theWidget);
	// port: whether a key is held for the widget that gets the keys: the capture's while there is one
	bool					IsInputKeyDown(KeyCode theKey);
};

}

#endif // __WIDGETMANAGER_H__