#ifndef __MENUTALKER_H__
#define __MENUTALKER_H__

// The large Inchy on the main menu (vtable 0x4fb7f4, 0x1c bytes; 0x422d90-0x423010): he pops up after a moment and
// says his welcome (mode 1, or 2 for the game over words; mode 3 stays silent).

namespace Sexy
{

class FossilizedApp;
class TalkSequence;
class Widget;

class MenuTalker
{
public:
	FossilizedApp*		mApp;			// +4
	TalkSequence*		mTalk;			// +8
	Widget*				mOwner;			// +0xc
	int					mFrame;			// +0x10 his picture: 1 hidden, 0 up
	int					mCountdown;		// +0x14
	int					mMode;			// +0x18

	MenuTalker(FossilizedApp* theApp, Widget* theOwner, int theMode);	// 0x422d90
	virtual ~MenuTalker();												// 0x422e10
	virtual void		Draw();											// 0x422fb0
	virtual void		Update();										// 0x422e30
};

}

#endif
