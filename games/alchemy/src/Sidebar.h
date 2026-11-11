#ifndef __ALCHEMY_SIDEPANEL_H__
#define __ALCHEMY_SIDEPANEL_H__

// The left 150 pixels of the screen (class 0x4e4b40, ctor 0x40f987, size 0x90): the forge gauge (a tank of red
// liquid with a wave on top and rising bubbles), the score and the skill and mode labels. The buttons below are the
// app's widgets (WinAlchApp::LoadingCompleted).

#include "Widget.h"
#include <list>

namespace Sexy
{

class WinAlchApp;

struct Bubble
{
	int			mX;         // +8 0..0x3d across the tank
	int			mY;         // +0xc rises from 0x5b (the bottom) to the surface
	double		mAccum;     // +0x10
	double		mSpeed;     // +0x18 steps per update
	bool		mBurst;     // +0x20 it reached the surface and pops
	int			mFrame;     // +0x24 the pop's frame, 0..7

	Bubble() : mX(0), mY(0), mAccum(0), mSpeed(0), mBurst(false), mFrame(0) {}
};

class Sidebar : public Widget
{
public:
	WinAlchApp*			mApp;           // +0x58
	int					mFill;          // +0x60 the gauge's level in pixels, 0..0x5b
	int					mHold;          // +0x64 updates the gauge waits before it moves
	int					mBubbleScroll;  // +0x68 the liquid's picture scrolls up (mod 0x67)
	int					mWaveFrame;     // +0x70 the wave's frame (mod 7)
	int					mWaveX;         // +0x74 the wave's scroll (mod 0x6a)
	double				mScoreAnim;     // +0x78 the displayed score
	std::list<Bubble>	mBubbles;       // +0x80
	bool				mFlashing;      // +0x8c
	bool				mFlashOn;       // +0x8d
	bool				mOnTarget;      // +0x8e the gauge shows the board's level

	Sidebar(WinAlchApp* theApp);              // 0x40f987

	void				FlashLava(bool isFlashing);       // 0x40f8bc
	virtual void		MouseDown(int x, int y, int theClickCount) override;    // 0x40f8d3
	virtual void		Draw(Graphics* g) override;         // 0x40fa93
	virtual void		Update() override;                  // 0x40fead

	using Widget::MouseDown;
};

}

#endif
