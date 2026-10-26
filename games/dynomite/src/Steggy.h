#ifndef __STEGGY_H__
#define __STEGGY_H__

// The board's +0x84 (vtable 0x4fcd60, 0x3c bytes; 0x437aa0-0x438010): Steggy, left of the slingshot, holding the
// next egg on a bob; when an egg is fired he tosses the next one into the slingshot, ducks down for another (the
// board's slot 45) and comes back up. He looks scared, with an alert over him, while the eggs are past the line.

namespace Sexy
{

class Dynomite;
class Egg;
class FossilizedApp;

class Steggy
{
public:
	Dynomite*			mBoard;				// +4
	FossilizedApp*		mApp;				// +8
	float				mBob;				// +0xc up to 5 either way
	float				mDrop;				// +0x10 how far he has ducked (0 to past 160)
	int					mBobAngle;			// +0x14 degrees, 5 a frame
	float				mTossVX;			// +0x18
	float				mTossVY;			// +0x1c 0.75 a frame down
	Egg*				mNext;				// +0x20 the egg he holds
	Egg*				mToss;				// +0x24 the egg on its way to the slingshot
	bool				mField28;			// +0x28 (true; saved)
	int					mField2c;			// +0x2c (2000; 1000 once he is up)
	bool				mAlerting;			// +0x30 the alert blinks every 35 frames with its sound
	int					mBlinkCount;		// +0x34
	bool				mBlink;				// +0x38 the alert shows
	bool				mWantEgg;			// +0x39 he takes his first egg on his next update (true)
	bool				mDropEgg;			// +0x3a he drops the one he holds when he next ducks

	Steggy(FossilizedApp* theApp, Dynomite* theBoard);	// 0x437aa0
	virtual ~Steggy();									// 0x437af0

	void				Toss();								// 0x437b20
	void				Draw();								// 0x437ea0
	void				Update();							// 0x438010
	// TODO: the save and load (0x437b50, 0x437c90) come with the saved games
};

}

#endif
