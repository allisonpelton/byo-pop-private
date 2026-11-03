#ifndef __PIXELUS_CHARACTER_H__
#define __PIXELUS_CHARACTER_H__

// Character.cpp: the figure of a god (or of Claudius) with its moods (Pixelus.exe 0x42e110 constructor, 0x42df40 Update,
// 0x42de40 Poke; the speech bubble it may carry is 0x42c4a0, updated by 0x42c590). It stands in a temple, speaks in a
// story and, in the tutorial, teaches beside the board. Its head swings on a spring, its face goes through short
// sequences of poses (a table at 0x5105e0: a pose and how long it stays, in half-updates) and the figure of Jupiter (9)
// blinks. All of this is moved by draws of the two generators, which is what makes the draws of a whole screen add up:
// the generator of the application (the original's `*(app + 0x670)`) and the game's own (0x579360).
//
// port: the speech bubble's text and picture are not drawn (its state, which draws numbers, is kept).

namespace Sexy
{

class PixelusApp;
class Widget;

// 0x42c4a0: the hint that comes up beside a temple's god
class SpeechBubble
{
public:
	PixelusApp*		mApp;
	int				mGod;			// +0x4
	Widget*			mOwner;			// +0x8 the widget that is redrawn when the bubble changes
	int				mTip;			// +0xc the text shown, -1 for none
	int				mNextTip;		// +0x10
	int				mPhase;			// +0x14
	int				mPeriod;		// +0x18 how long a text stays
	int				mDelay;			// +0x1c updates before the first text
	int				mAlpha;			// +0x20
	bool			mLeaving;		// +0x24

public:
	SpeechBubble(PixelusApp* theApp, int theGod, Widget* theOwner, int theMode);
	void			Update();		// 0x42c590
	void			Hurry();		// 0x42c550
};

class Character
{
public:
	enum
	{
		NUM_ANIMATIONS = 5,
		ANIMATION_ENTRIES = 50,
		NO_ANIMATION = -1
	};

	PixelusApp*		mApp;
	int				mTick;			// +0x4
	SpeechBubble*	mBubble;		// +0x8
	double			mAngle;			// +0x10 the head's swing
	double			mSpeed;			// +0x18
	int				mGod;			// +0x20
	int				mAnimation;		// +0x24 the sequence that plays, or -1
	int				mAnimTick;		// +0x28 half-updates in the entry
	int				mAnimEntry;		// +0x2c
	int				mPose;			// +0x30 the head shown (0 to 3)
	int				mFade;			// +0x34 how much of it shows over the pose before (0 to 255)
	int				mPrevPose;		// +0x38
	int				mBlink;			// +0x3c updates into a blink, -1 for none
	double			mBlinkAngle;	// +0x40

public:
	Character(PixelusApp* theApp, int theGod, Widget* theOwner, int theMode);
	~Character();

	// theWidget is marked dirty when there is one
	void			Update(Widget* theWidget);
	void			Poke();			// 0x42de40
};

}

#endif
