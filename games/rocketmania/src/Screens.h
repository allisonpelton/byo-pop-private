#ifndef __ROCKETMANIA_SCREENS_H__
#define __ROCKETMANIA_SCREENS_H__

// The full-screen widgets of Rocket Mania Deluxe: LoadingScreen (RocketManiaGame 0x4104b0-0x411860) and the
// game's other screens. Each is a top-level widget (the 2003 framework has no child widgets) that routes its
// buttons to itself.

#include "Widget.h"
#include "ButtonListener.h"
#include "HyperlinkWidget.h"
#include <string>

namespace Sexy
{

class RocketApp;

// 0x4104b0: the loading picture, a bar that fills as the game's files load, and the link that starts the game.
// port: the shareware branches (the trial counters, the register link, "You must register online now") are not
// ported; the Steam build's partner.xml says NoReg and the screen always takes the registered path.
class LoadingScreen : public Widget, public ButtonListener
{
public:
	RocketApp*			mApp;                   // +0x5c
	double				mProgress;              // +0x60 the bar's fill, 0..1
	bool				mLinkShown;             // +0x70 the play link has been shown
	HyperlinkWidget*	mPlayLink;              // +0x94 id 1 "click here to play"

public:
	LoadingScreen(RocketApp* theApp);
	virtual ~LoadingScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
};

}

#endif
