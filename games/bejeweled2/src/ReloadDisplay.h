#ifndef __RELOADDISPLAY_H__
#define __RELOADDISPLAY_H__

// ReloadDisplay.h: the screen that shows while the game's pictures are loaded again after the display changed
// (Sexy::ReloadDisplay, 0x5d3e12; the object is 0x8c bytes).

#include "Widget.h"

namespace Sexy
{

class WinDMApp;

class ReloadDisplay : public Widget
{
public:
	WinDMApp*		mApp;               // +0x88
	bool			mDrawn;             // port: the display has been drawn once, so the reload can start

public:
	ReloadDisplay(WinDMApp* theApp);

	virtual void	Update() override;                              // 0x5d3e12
	virtual void	Draw(Graphics* g) override;                     // 0x5d3ea8
};

}

#endif
