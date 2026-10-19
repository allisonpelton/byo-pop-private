#ifndef __WIDGETMANAGER_H__
#define __WIDGETMANAGER_H__

// port(variant): headless shim of framework/WidgetManager.h for the verifier
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the original game. It runs no widget: the verifier
// updates its boards itself (the replay player updates a playback board once per frame in the game too, with
// the widget manager's updates of it switched off), and there is never a modal dialog.

#include "Widget.h"

namespace Sexy
{

enum
{
	WIDGETFLAGS_UPDATE = 1
};

class WidgetManager
{
public:
	Widget*				mBaseModalWidget;
	Widget*				mFocusWidget;

	WidgetManager() : mBaseModalWidget(NULL), mFocusWidget(NULL) {}

	void				AddWidget(Widget* theWidget) {}
	void				RemoveWidget(Widget* theWidget)
	{
		if (mFocusWidget == theWidget)
			mFocusWidget = NULL;
	}
	void				BringToBack(Widget* theWidget) {}
	void				SetFocus(Widget* theWidget) { mFocusWidget = theWidget; }
};

}

#endif
