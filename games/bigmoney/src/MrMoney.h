#ifndef __BIGMONEY_MRMONEY_H__
#define __BIGMONEY_MRMONEY_H__

// MrMoney.cpp: Mr. Moneybags, the talking head on a spring at the left of the screen. One instance,
// WinBMApp::mMrMoney, made by WinBMApp::TitleScreenIsFinished (Resize(0, 0, 184, 136), Move(-200, 0))
// and added to the WidgetManager. Board, GameSelector and WinBMApp move it by writing mTargetX/mTargetY;
// everyone makes it talk with PlayAnim.
// Original: class Sexy::MrMoney : public Widget, sizeof 0xd8 (new in 0x4215c0), vftable 0x4fb63c
// (54 slots; overrides 0 ~Widget, 9 Update, 13 Draw, 26 Move, 44 IsPointVisible = the shared
// `return true` stub). Offsets are absolute (Widget is 0x80 bytes in 1.2).
// Behaviour spec: re/bigmoney/analysis/board_draw.md section 3.

#include "Widget.h"

namespace Sexy
{

class WinBMApp;
class Graphics;

// MrMoney::PlayAnim ids. Each caller plays the speech sample of the same name next to the call
// (Board, GameSelector, HighScoreWidget); 24..49 are silent idle animations started by Update.
enum MrMoneyAnim
{
	MRANIM_ACTION = 0,              // GameSelector: PlayAnim(gameType) with SOUND_ACTION + gameType
	MRANIM_STRATEGY = 1,
	MRANIM_PUZZLE = 2,
	MRANIM_ANEWHIGHSCORE = 3,       // HighScoreWidget
	MRANIM_THREE = 4,               // Board countdowns: 4 + (3 - n)
	MRANIM_TWO = 5,
	MRANIM_ONE = 6,
	MRANIM_AWESOME = 7,
	MRANIM_BIGMONEY = 8,
	MRANIM_LOVEIT = 9,              // GameSelector
	MRANIM_BONUS = 10,
	MRANIM_EXCELLENT = 11,
	MRANIM_GAMEOVER = 12,
	MRANIM_GETREADY = 13,
	MRANIM_GO = 14,
	MRANIM_GOODJOB = 15,
	MRANIM_INCREDIBLE = 16,
	MRANIM_LEVELUP = 17,
	MRANIM_UNUSED_18 = 18,          // no caller (probably "megamoney")
	MRANIM_SPEEDBONUS = 19,
	MRANIM_SUPERBONUS = 20,
	MRANIM_UNBELIEVABLE = 21,
	MRANIM_WAKEUP = 22,             // Update: 12000 updates without input, with SOUND_WAKEUP
	MRANIM_WELCOME = 23,            // GameSelector
	MRANIM_BLINK = 24,              // Update: CrtRand() % 3000 == 0
	MRANIM_IDLE_FIRST = 25,         // Update: CrtRand() % 6000 == 0 starts 25 + CrtRand() % 25
	NUM_MRMONEY_ANIMS = 50
};

// One step of an animation: show mFrame (WinBMApp::mMrMoneyImages index 0..5) for mDuration*2 updates.
// mFrame -1 ends the animation.
struct MrMoneyAnimStep
{
	int				mFrame;
	int				mDuration;
};

class MrMoney : public Widget
{
public:
	enum
	{
		NUM_ANIM_STEPS = 50,        // steps per animation in the table (0x190 bytes)
		IDLE_WAKE_TICKS = 12000     // WidgetManager updates since the last input before the wake-up
	};

	WinBMApp*		mApp;                   // +0x80 ctor arg
	// +0x84 pad
	double			mRot;                   // +0x88 rotation in radians; drawn rotated when |mRot| >= 0.005
	double			mRotVel;                // +0x90 angular velocity (ctor 0.004)
	int				mTargetX;               // +0x98 spring target; Move sets it, Board / WinBMApp write it directly
	int				mTargetY;               // +0x9c
	double			mPosX;                  // +0xa0 spring position; Update moves the widget to (int)mPosX, (int)mPosY
	double			mPosY;                  // +0xa8
	double			mVelX;                  // +0xb0
	double			mVelY;                  // +0xb8
	int				mAnim;                  // +0xc0 MrMoneyAnim playing, -1 none (GameSelector reads it)
	int				mAnimTick;              // +0xc4 updates spent in the current step
	int				mAnimStep;              // +0xc8
	int				mFrame;                 // +0xcc WinBMApp::mMrMoneyImages index 0..5
	bool			mIdleWoke;              // +0xd0 the idle wake-up has fired (ctor true)
	// +0xd1..+0xd7 pad

public:
	MrMoney(WinBMApp* theApp);                                      // 0x415140
	virtual ~MrMoney();                                             // 0x4151f0 (scalar deleting dtor 0x415540) slot 0

	void			PlayAnim(int theAnim);                          // 0x415200
	virtual void	Move(int theNewX, int theNewY);                 // 0x415240 slot 26
	virtual void	Update();                                       // 0x415290 slot 9
	virtual void	Draw(Graphics* g);                              // 0x4154d0 slot 13
};

}

#endif
