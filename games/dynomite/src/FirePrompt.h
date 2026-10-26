#ifndef __FIREPROMPT_H__
#define __FIREPROMPT_H__

// The board's prompt at +0x60 (vtable 0x4fc828, 0x5c bytes; 0x431e50-0x432330): two lines under the launcher
// asking for the first egg, pulsing; every five pulses they swap with two lines of the board's own when it has
// them, and they fade out once the board's +0x16c is set.

#include <string>

namespace Sexy
{

class Dynomite;
class FossilizedApp;

class FirePrompt
{
public:
	FossilizedApp*		mApp;				// +4
	Dynomite*			mBoard;				// +8
	int					mField0c;			// +0xc (saved)
	float				mPulses;			// +0x10 pulses left before the lines change (4)
	int					mAlpha;				// +0x14 (255)
	int					mFade;				// +0x18 the alpha's step a quarter at a time (-1)
	bool				mField1c;			// +0x1c (saved)
	std::string			mOwnLine1;			// +0x20 the board's own lines ("" none)
	std::string			mOwnLine2;			// +0x3c
	bool				mShowOwn;			// +0x58 the board's own lines, when it has them, rather than the first-egg ones

	FirePrompt(FossilizedApp* theApp, Dynomite* theBoard);	// 0x431fc0
	virtual ~FirePrompt();									// 0x431f80

	void				Update();							// 0x431e50
	void				Reset();							// 0x431ee0: fades in from nothing
	void				Draw();								// 0x432090
	// TODO: the save and load (0x431ef0, 0x431f40) come with the saved games
};

}

#endif
