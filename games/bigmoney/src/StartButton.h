#ifndef __BIGMONEY_STARTBUTTON_H__
#define __BIGMONEY_STARTBUTTON_H__

// StartButton.cpp: the "START" bar at the bottom of the game selector (cross-fading text and chasing
// arrows).
// Original: class Sexy::StartButton : public ButtonWidget, sizeof 0x138 (new in the GameSelector ctor),
// vftable 0x4fbe04 (57 slots). Offsets are absolute (ButtonWidget is 0x118 bytes in 1.2).
// Behaviour spec: re/bigmoney/analysis/screens.md.

#include "ButtonWidget.h"

namespace Sexy
{

class WinBMApp;
class Graphics;

class StartButton : public ButtonWidget
{
public:
	WinBMApp*		mApp;                   // +0x118
	double			mHiliteAlpha;           // +0x120 0..1 cross-fade to the highlighted text cell, +-0.1 per update
	double			mHiliteSpeed;           // +0x128 0.05; never read (Update uses a literal 0.1)
	int				mUnused130;             // +0x130 zeroed by Update when the highlight rises, never read (port: initialised)
	int				mArrowFrame;            // +0x134 0..7, +1 every 16 idle updates; 5 lights all arrows (forced while hovered)

public:
	StartButton(WinBMApp* theApp, int theId, ButtonListener* theListener); // 0x419260
	virtual ~StartButton();                                         // 0x4192c0 (scalar deleting dtor 0x4193d0) slot 0
	virtual void	Update() override;                                       // 0x4192d0 slot 9
	virtual void	Draw(Graphics* g) override;                              // 0x4193f0 slot 13
};

}

#endif
