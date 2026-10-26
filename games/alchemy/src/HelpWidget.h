#ifndef __ALCHEMY_HELPWIDGET_H__
#define __ALCHEMY_HELPWIDGET_H__

// The help screen (widget class 0x4e3b88 + listener 0x4e3b6c, ctor 0x408a8a, size 0x68, Draw 0x408cd7): the rules over the
// board's backdrop, and two links: the tutorial and the way back.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class AlchemyApp;
class HyperlinkWidget;

class HelpWidget : public Widget, public ButtonListener
{
public:
	AlchemyApp*			mApp;               // +0x5c
	HyperlinkWidget*	mTutorialLink;      // +0x60 id 0
	HyperlinkWidget*	mReturnLink;        // +0x64 id 1: back to the game, or to the selector when there is no game

public:
	HelpWidget(AlchemyApp* theApp);         // 0x408a8a
	virtual ~HelpWidget();                  // 0x408a6e

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;       // 0x4089fa
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x408a21
	virtual void		Draw(Graphics* g) override;                                     // 0x408cd7
	virtual void		ButtonDepress(int theId) override;                              // 0x408a4d
};

}

#endif
