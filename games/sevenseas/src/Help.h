#ifndef __SEVENSEAS_HELP_H__
#define __SEVENSEAS_HELP_H__

// The help screen (WinSS.exe 0x40d330, from Options -> Help): "How to play" over the sea, the ship with the arrows
// around it, one of each kind of ship and island with a few words, and two buttons: play the tutorial, return to game.

#include "Widget.h"
#include "ButtonListener.h"
#include "ButtonWidget.h"

namespace Sexy
{

class SevenSeasApp;

class HelpWidget : public Widget, public ButtonListener
{
public:
	SevenSeasApp*		mApp;                   // +0x5c
	ButtonWidget*		mTutorialButton;        // +0x60 id 0
	ButtonWidget*		mReturnButton;          // +0x64 id 1
	double				mScrollX;               // +0x68 the water
	double				mScrollY;               // +0x70
	double				mSpeedX;                // +0x78
	double				mSpeedY;                // +0x80

public:
	HelpWidget(SevenSeasApp* theApp);
	virtual ~HelpWidget();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;

private:
	void				DrawCellArrow(Graphics* g, int theX, int theY);
};

}

#endif
