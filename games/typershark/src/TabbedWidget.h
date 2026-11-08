#ifndef __TYPERSHARK_TABBEDWIDGET_H__
#define __TYPERSHARK_TABBEDWIDGET_H__

// TabbedWidget.cpp: the tab strip and the panel frame of the stats screen. It draws the background, the panel image
// below the tabs and a tab (three slices of the tab image stretched to the width of the title) for every TabPanel it
// holds; a click on another tab hides the shown panel and shows that one. The panels are added to the manager with the
// widget and sit in front of it (each is a widget of the manager). The StatsScreen makes it and sets the fields below
// after the constructor.
// Original: class Sexy::TabbedWidget : public Widget, vftable 0x5308e4, sizeof 0xc4 (constructor 0x451090, destructor
// 0x451170, deleting destructor 0x451140).
//
// Not translated: the vector<Rect> instances (0x451230 push_back, 0x450c20, 0x44c9c0, 0x44cb63, 0x44cb6f, 0x443410,
// 0x443a40, 0x4420c0, 0x4423b0: std::vector<Rect> here) and the shared vector<T*> push_back 0x4511c0 (std::vector).

#include "Widget.h"
#include "Rect.h"
#include "Color.h"
#include <vector>

namespace Sexy
{

class Font;
class Image;
class TabPanel;
class WinWSApp;

class TabbedWidget : public Widget
{
public:
	std::vector<TabPanel*>	mPanels;			// +0x58 (first pointer at +0x5c) the pages, in tab order; not owned (the StatsScreen deletes them)
	std::vector<Rect>		mTabRects;			// +0x68 (first at +0x6c) one rectangle per tab, x/y/width/height inside the widget, set by AddTab
	int						mCurrentTab;		// +0x78 the shown page; ctor 0
	WinWSApp*				mApp;				// +0x7c constructor argument 1
	Color					mTextColor;			// +0x80 the tab titles' colour (set by the StatsScreen: black)
	Font*					mFont;				// +0x90 the tab titles' font (set by the StatsScreen: Rockwell 13)
	Image*					mPanelImage;		// +0x94 constructor argument 2: drawn below the tabs (at y = height of the tab image)
	Image*					mBackground;		// +0x98 ctor NULL; drawn first at 0, 0 (the StatsScreen sets the water image)
	Image*					mTabImage;			// +0x9c constructor argument 3: three equal slices, left, middle, right
	Image*					mInactiveTabImage;	// +0xa0 ctor NULL; the tab image of the tabs that are not shown (NULL: the same image)
	int						mTabPadding;		// +0xa4 added on each side of a title for the tab width (the StatsScreen sets 10; ctor leaves it unset)
	int						mTextOffsetY;		// +0xa8 added to the font's ascent for the title's baseline (the StatsScreen sets 10; ctor leaves it unset)
	int						mFirstTabX;			// +0xac x of the first tab; ctor 0 (the StatsScreen sets 0x10)
	int						mTabY;				// +0xb0 y of every tab; ctor 0 (the StatsScreen sets 8)
	Color					mDividerColor;		// +0xb4 the line under a tab that is not shown (the StatsScreen sets black)

public:
	TabbedWidget(WinWSApp* theApp, Image* thePanelImage, Image* theTabImage);	// 0x451090
	virtual ~TabbedWidget() override;										// 0x451170 (deleting 0x451140), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x442f30 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x442f90 slot 2
	virtual void			Draw(Graphics* g) override;						// 0x442c00 slot 16
	virtual void			MouseLeave() override;							// 0x4414b0 slot 24
	virtual void			MouseMove(int x, int y) override;				// 0x442b80 slot 25
	virtual void			MouseDown(int x, int y, int theClickCount) override;	// 0x442ae0 slot 27 (any button)

	using Widget::MouseDown;
	using Widget::MouseMove;

	void					AddTab(TabPanel* thePanel);						// 0x453ac0 the tab is as wide as the title (at least two thirds of a tab image) plus the padding
	void					SetCurrentTab(int theTab);						// 0x442fd0 hides the shown page and shows that one (no repaint)
};

}

#endif
