#ifndef __ALCHEMY_WIZARD_H__
#define __ALCHEMY_WIZARD_H__

// The wizard's head (class 0x4e58b0, ctor 0x414887, size 0x90): a floating head that fades in and moves its mouth
// while a message is "spoken". The selector screen has one and the board has one (the tutorial and the hints).
// The mouth follows one of four fixed lip-sync tracks (the table at 0x510340).

#include "Widget.h"

namespace Sexy
{

class AlchemyApp;

class WizardBox : public Widget
{
public:
	AlchemyApp*			mApp;           // +0x58
	double				mAlpha;         // +0x60 0..1
	int					mFadeDir;       // +0x68 1: fading in, -1: out, 0: still (the ctor starts it fading in)
	double				mBobX;          // +0x70 phase of the sideways float
	double				mBobY;          // +0x78 phase of the up and down float
	bool				mTalking;       // +0x80
	int					mTrack;         // +0x84 the lip-sync track (0..3)
	int					mStep;          // +0x88 the step in the track
	int					mTimer;         // +0x8c updates left in the step

	WizardBox(AlchemyApp* theApp);      // 0x414887

	void				Talk(int theTrack);         // 0x4148ce
	void				StopTalking();              // 0x414903
	virtual void		Update() override;          // 0x414912
	virtual void		Draw(Graphics* g) override; // 0x414a7d
};

}

#endif
