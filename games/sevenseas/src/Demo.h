#ifndef __SEVENSEAS_DEMO_H__
#define __SEVENSEAS_DEMO_H__

// The beginner's demo (WinSS.exe 0x4132b0): a first game on the easy level starts with a board that plays itself,
// a fake cursor clicking its way through the rules, and a button "Click Here to Start!" that begins the real game.

#include "Widget.h"
#include "ButtonListener.h"
#include "Button2003.h"
#include "Board.h"
#include <list>

namespace Sexy
{

class SevenSeasApp;

// 0x50 bytes in the original for the player's ship (the fields start at +0), 0x48 for the others (they start at +4)
struct DemoShip
{
	int					mX;             // the cell
	int					mY;
	int					mFrame;         // the heading, 0 north to 14 northwest in steps of two
	int					mTargetX;       // the cell it sails to
	int					mTargetY;
	int					mTimer;         // 0 .. 0x28 while it sails
	int					mAnim;          // counts up with the updates; picks the sail frame (a random start)
};

// 0x4132b0
class DemoWidget : public Widget, public ButtonListener
{
public:
	SevenSeasApp*		mApp;                   // +0x5c
	DialogButton*		mButton;                // +0x60 id 1
	DemoShip			mPlayer;                // +0x64
	DemoShip			mPirates[4];            // +0x68 .. +0x74
	DemoShip			mBoss;                  // +0x78
	int					mAct;                   // +0x7c the part of the show, 0 the fade in, 9 the fade out
	int					mStep;                  // +0x80 the step in a part
	int					mFade;                  // +0x84 the black cover's alpha
	int					mShotCnt;               // +0x88 counts the cannon ball's flight
	int					mWaitCnt;               // +0x8c
	int					mTurnCnt;               // +0x90
	int					mCursorX;               // +0x94 the fake cursor, in pixels
	int					mCursorY;               // +0x98
	double				mScrollX;               // +0xa0 the water
	double				mScrollY;               // +0xa8
	double				mSpeedX;                // +0xb0
	double				mSpeedY;                // +0xb8
	std::list<AnimInstance*>	mAnims;         // +0xc4 the explosions and splashes

public:
	DemoWidget(SevenSeasApp* theApp);
	virtual ~DemoWidget();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		MouseMove(int x, int y) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;

private:
	void				InitShips();
	void				UpdateAnims();
	void				AddAnim(const char* theName, int theX, int theY);
	void				DrawAnims(Graphics* g);
	void				UpdateAct();
	void				UpdateIntroStep();
};

}

#endif
