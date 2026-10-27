#ifndef __INCHYSMALL_H__
#define __INCHYSMALL_H__

// The small Inchy of the side bar (0x84 bytes, vtable 0x4fb8ac; InchySmall.cpp in the original's link order,
// 0x423010-0x423d00): he rises behind the side bar's frame, blinks, and calls out combos with a speech bubble (a
// widget of its own, vtable 0x4fb800) and a voice whose mouth shapes a TalkSequence plays (0x42be60).

#include "PtrList.h"
#include "Widget.h"
#include <string>

namespace Sexy
{

class FossilizedApp;
class InchySmall;

// 0x42be60 (vtable 0x4fc248, 0x2c bytes): a list of mouth shapes with their lengths in updates, and a sound
struct TalkStep
{
	int					mMouth;
	int					mFrames;
};

class TalkSequence
{
public:
	PtrList<TalkStep>	mSteps;			// +4
	bool				mPlaying;		// +0x1c
	TalkStep*			mCurrent;		// +0x20
	int					mCountdown;		// +0x24
	bool				mChanged;		// +0x28 the mouth changed (the owner redraws and clears it)
	bool				mDone;			// +0x29 the last step ended (the owner clears it)

	TalkSequence();						// 0x42be60
	virtual ~TalkSequence();			// 0x42bfc0

	void				Reset();								// 0x42bec0
	void				AddStep(int theMouth, int theFrames);	// 0x42bf20
	void				Play(class GameSound* theSound);		// 0x42bf50
	void				Update();								// 0x42bf80
};

// 0x423090 (vtable 0x4fb800, 0x5c bytes): the speech bubble
class InchyBubble : public Widget
{
public:
	InchySmall*			mInchy;			// +0x58

	InchyBubble(InchySmall* theInchy);	// 0x423090
	virtual ~InchyBubble();				// 0x4230b0
	virtual void		Draw(Graphics* g) override;	// 0x4230c0
};

struct InchyEvent
{
	int					mDelay;
	int					mAction;		// 0 rise, 1 sink, 2-8 the combo calls
};

class InchySmall
{
public:
	Widget*				mOwner;			// +4 the side bar
	FossilizedApp*		mApp;			// +8
	int					mState;			// +0xc 0 down, 1-2 rising, 3-4 sinking, 5 up
	int					mCounter;		// +0x10
	InchyBubble*		mBubble;		// +0x14
	PtrList<InchyEvent>	mEvents;		// +0x18
	TalkSequence		mTalk;			// +0x30 (its +0x28/+0x29 are read as +0x58/+0x59)
	std::string			mText;			// +0x5c
	int					mSpeech;		// +0x78 the bubble's text, -1 none
	int					mBlinkCounter;	// +0x7c
	bool				mBlink;			// +0x80

	InchySmall(FossilizedApp* theApp, Widget* theOwner);	// 0x423800
	virtual ~InchySmall();										// 0x4238a0

	void				Rise(int theDelay);						// 0x423010
	void				Sink(int theDelay);						// 0x423040
	void				HideBubble();							// 0x423070
	void				CreateBubble();							// 0x423120
	void				Draw();									// 0x423190
	void				DoAction(int theAction);				// 0x423250
	void				Update();								// 0x423650
	void				Combo(int theDelay, int theLevel);		// 0x423990
};

}

#endif
