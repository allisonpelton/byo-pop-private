#ifndef __TANK_H__
#define __TANK_H__

// The board's +0x88 (vtable 0x4fcd80, 0x78 bytes; 0x438400-0x4388d0): Tank, right of the slingshot. He looks about
// at random, flinches when the grid breaks or blasts eggs, and after five or more he calls out one of four phrases
// (a word in a balloon, its voice, and his mouth moving); he sweats while the eggs are past the line.

namespace Sexy
{

class Cell;
class Dynomite;
class FossilizedApp;

// one mouth shape and its length in updates
struct TankMouth
{
	int					mFrame;
	int					mFrames;
};

class Tank
{
public:
	Dynomite*			mBoard;				// +4
	FossilizedApp*		mApp;				// +8
	int					mEye;				// +0xc where he looks (the eye sheet's cell)
	int					mEyeTime;			// +0x10 updates until he looks elsewhere
	int					mState;				// +0x14 0 idle, 1 flinching, 2 calling out
	int					mTime;				// +0x18 the flinch's updates, or the mouth shape he is at
	int					mPhrase;			// +0x1c 0-3
	TankMouth			mMouth[10];			// +0x20
	Cell*				mWord;				// +0x70 the phrase's word
	int					mMouthCount;		// +0x74

	Tank(FossilizedApp* theApp, Dynomite* theBoard);	// 0x438400
	virtual ~Tank();									// 0x438430

	void				Update();							// 0x438440
	void				Flinch(int theCount);				// 0x4384d0: 100 is a blast
	void				Draw();								// 0x438690
};

}

#endif
