#ifndef __TYPERSHARK_TABPANEL_H__
#define __TYPERSHARK_TABPANEL_H__

// TabPanel.cpp: one page of the TabbedWidget on the stats screen. It is a widget that owns a list of child widgets
// (the 2004 framework has no child widgets: the children are ordinary widgets of the manager, and the panel brings
// them to the front, shows and hides them together with itself). The base of AdventurePanel, AbyssPanel and
// TutorialPanel; the title is the text on the panel's tab.
// Original: class Sexy::TabPanel : public Widget, vftable 0x52f6cc, sizeof 0x8c (constructor 0x450cd0, destructor body
// 0x444690 with its thunk 0x450e70, deleting destructor 0x444700; the three subclasses have deleting destructors of
// their own, 0x450dc0 for the abyss and adventure panels and 0x450a50 for the tutorial panel).
//
// Not translated: the allocator code of the vector<Widget*> (0x4511c0 push_back shared with the TabbedWidget's
// vector<TabPanel*>), the destructor's thunk and deleting destructors (the C++ destructor chain).

#include "Widget.h"
#include <vector>

namespace Sexy
{

class WinWSApp;

class TabPanel : public Widget
{
public:
	std::vector<Widget*>	mChildren;		// +0x58 (the vector object; first element pointer at +0x5c) widgets that belong to the panel
	WinWSApp*				mApp;			// +0x68 constructor argument
	bool					mShown;			// +0x6c ctor true; SetShown stores it last
	SexyString				mTitle;			// +0x70 the text on the tab, set by the subclass constructor

public:
	TabPanel(WinWSApp* theApp);												// 0x450cd0
	virtual ~TabPanel() override;											// 0x444690 (deleting 0x444700), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x43fbd0 slot 1 (calls the base only)
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x43fbe0 slot 2 (calls the base only)

	virtual void			SetShown(bool theShow);							// 0x442a10 slot 47 the TabbedWidget calls it on a tab change
	virtual void			AddChild(Widget* theChild);						// 0x4516a0 slot 48
	virtual Widget*			GetChild(int theIndex);							// 0x4446f0 slot 49

	void					BringChildrenToFront();							// 0x442aa0 the TabbedWidget calls it after adding the panels
};

}

#endif
