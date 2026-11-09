#ifndef __TYPERSHARK_STATSSCREEN_H__
#define __TYPERSHARK_STATSSCREEN_H__

// StatsScreen.cpp: the statistics screen (the menu's "Stats" button, and after a lesson with theInitialTab 2): a
// TabbedWidget over the water background with three pages, "Adventure Mode" (AdventurePanel), "Abyss Mode"
// (AbyssPanel) and "Tutorial Mode" (TutorialPanel). The app makes it (WinWSApp::ShowStatsScreen: new StatsScreen(this,
// theWhich), SetUser(mUser), Resize to the window, AddWidget) and owns the Close button, which it keeps in front of the
// pages (WinWSApp::BringStatsCloseButtonToFront, which TabPanel::SetShown calls).
// Original: class Sexy::StatsScreen : public Widget, public ButtonListener (the listener sub-object is at +0x58 with its
// own table 0x52f1f0), vftable 0x52f20c, constructor 0x45baa0, destructor body 0x4412a0 (deleting 0x441ee0).
//
// Not translated: the ButtonListener's slot-2 forwarder 0x449de0 (the base class ButtonListener::ButtonPress(id, count)
// calls ButtonPress(id) in the port).

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class AbyssPanel;
class AdventurePanel;
class TabbedWidget;
class TutorialPanel;
class UserProfile;
class WinWSApp;

class StatsScreen : public Widget, public ButtonListener
{
public:
	// the Widget part ends at +0x58, the ButtonListener part is +0x58 (its virtual table pointer)
	WinWSApp*				mApp;				// +0x5c constructor argument 1
	TabbedWidget*			mTabs;				// +0x60 owned; the tab strip with the three pages
	AdventurePanel*			mAdventurePanel;	// +0x64 owned (page 0)
	AbyssPanel*				mAbyssPanel;		// +0x68 owned (page 1)
	TutorialPanel*			mTutorialPanel;		// +0x6c owned (page 2)

public:
	StatsScreen(WinWSApp* theApp, int theInitialTab);						// 0x45baa0 the page shown first: 0 adventure, 1 abyss, 2 tutorial
	virtual ~StatsScreen() override;										// 0x4412a0 (deleting 0x441ee0), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x441330 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x441360 slot 2
	virtual void			ButtonPress(int theId) override;				// 0x441320 the listener's slot 3: the button-click sound

	using ButtonListener::ButtonPress;

	void					SetUser(UserProfile* theUser);					// 0x451050 gives the three pages the profile and sets the tutorial page to its last lesson
};

}

#endif
