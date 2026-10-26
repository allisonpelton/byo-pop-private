#ifndef __KRONEY_H__
#define __KRONEY_H__

// DynomitePanic's +0x248 (vtable 0x4fc1f8, only its destructor; 0x2c bytes; 0x42b3a0-0x42bb50): Kroney, right of
// the board, with his hourglass. While the game runs the sand falls for mPeriod seconds; Kroney calls out two seconds
// before the end, the siren sounds at three, the clock ticks from five to three, and when the sand is out he flips
// the hourglass and the whirleys come (Update returns true; not after the first flip, which starts the game).

namespace Sexy
{

class Dynomite;
class FossilizedApp;

class Kroney
{
public:
	Dynomite*			mBoard;				// +4
	FossilizedApp*		mApp;				// +8
	float				mFrame;				// +0xc his sheet's frame (0-8; the flip's 0-14)
	float				mY;					// +0x10 his middle (-100 above the board, down to 120)
	float				mSandFraction;		// +0x14 the share of the sand that has fallen
	float				mTimeLeft;			// +0x18 seconds
	float				mCallTime;			// +0x1c seconds to his call (two before the end)
	float				mPeriod;			// +0x20 seconds
	int					mState;				// +0x24 0 the sand falls, 1 he flips the hourglass
	bool				mTalking;			// +0x28 his balloon shows
	bool				mFirstFlip;			// +0x29 the flip that starts the game brings no whirleys
	bool				mCallAlt;			// +0x2a which of his two calls is next
	bool				mHurry;				// +0x2b he calls that time is running out instead

	Kroney();								// 0x42b3a0
	virtual ~Kroney();						// 0x42b3d0

	void				Start(float thePeriod);	// 0x42b3e0: a flip first, then thePeriod seconds
	bool				Update();				// 0x42b420: true when a flip has ended and the whirleys come
	void				Draw();					// 0x42b7c0
	// TODO: the save and load (0x42b630, 0x42b6c0) come with the saved games
};

}

#endif
