#ifndef __BIGMONEY_HELPWIDGET_H__
#define __BIGMONEY_HELPWIDGET_H__

// HelpWidget.cpp: the full-screen "How to play Big Money: <mode>" page shown before the first game of a
// mode (unless "Don't Show This Screen Again" was clicked for it) and from the Options dialog.
// Original: class Sexy::HelpWidget : public Widget, public ButtonListener, sizeof 0xa0 (new in
// WinBMApp::ShowHelp 0x41b6f0), vftable 0x4f5d94 (54 slots), ButtonListener vftable 0x4f5d74.
// Offsets are absolute (Widget is 0x80 bytes in 1.2). Behaviour spec: re/bigmoney/analysis/screens.md.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class WinBMApp;
class Graphics;
class WidgetManager;
class HyperlinkWidget;

class HelpWidget : public Widget, public ButtonListener
{
public:
	enum
	{
		START_LINK_ID = 0,          // CLICK_HERE_TO_START_GAME / CLICK_HERE_TO_CONTINUE_GAME
		MORE_LINK_ID = 1,           // CLICK_HERE_FOR_MORE_INSTRUCTIONS (never sized: inert)
		DONT_SHOW_LINK_ID = 2       // DONT_SHOW_THIS_SCREEN_AGAIN (hidden when a board exists)
	};

	// +0x080 ButtonListener vfptr
	WinBMApp*		mApp;                   // +0x084
	// +0x088..+0x08f unused
	HyperlinkWidget* mStartLink;            // +0x090 mFont1, (0,249,183), Rect(406 - (w+4)/2, 411, w+4, 26)
	HyperlinkWidget* mMoreLink;             // +0x094 mFont1Underlined, (217,33,153), no rect
	HyperlinkWidget* mDontShowLink;         // +0x098 mFont1, (98,98,98), over (212,212,212), Rect(406 - (w+4)/2, 446, w+4, 26)
	int				mPage;                  // +0x09c 0; nothing sets it (the page-1 branch draws nothing)

public:
	HelpWidget(WinBMApp* theApp);                                   // 0x410f20
	virtual ~HelpWidget();                                          // 0x410d80 (scalar deleting dtor 0x410f00) slot 0
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;  // 0x410df0 slot 7
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x410e30 slot 8
	virtual void	Draw(Graphics* g) override;                              // 0x411590 slot 13
	virtual void	OrderInManagerChanged() override;                        // 0x410e70 slot 15
	virtual void	ButtonPress(int theId) override;                         // 0x410eb0 ButtonListener slot 1 (mouse down)

	using ButtonListener::ButtonPress;
};

}

#endif
