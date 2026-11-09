#ifndef __TYPERSHARK_GAMEOBJECT_H__
#define __TYPERSHARK_GAMEOBJECT_H__

// GameObject.cpp: the base of every sprite object of the game board (sharks, piranhas, bubbles, the player's cannon...).
// A position and a velocity in doubles, a state, a frame counter for the 20-frame sprite sheets, and the last six
// position changes. Original: class Sexy::GameObject (vftable 0x529394, 12 virtuals; the object part is 0xf0 bytes: the
// constructor is 0x4611b0). The entity classes inherit it as a *virtual* base with Enemy; the port uses plain single
// inheritance (docs/ENTITIES.md). Field comments give the offset inside the original's part.

namespace Sexy
{

class Graphics;

class GameObject
{
public:
	double					mDXHistory[6];			// +0x08 the last six changes of x, written by RecordHistory
	int						mDXIndex;				// +0x38 where the next one goes (0..5)
	double					mPrevX;					// +0x48 x at the previous RecordHistory
	double					mDYHistory[6];			// +0x50 the last six changes of y
	int						mDYIndex;				// +0x80
	double					mPrevY;					// +0x90
	double					mX;						// +0x98 the position (slot 5, 6 set it)
	double					mY;						// +0xa0
	double					mLastX;					// +0xa8 the position at the end of the last RecordHistory
	double					mLastY;					// +0xb0
	int						mState;					// +0xb8 the creature's state (slot 7); 0 = swimming, see the entity classes
	double					mVelX;					// +0xc0 (slot 9)
	double					mVelY;					// +0xc8 (slot 10)
	int						mFrameWidth;			// +0xd0 one frame of the sprite sheet (image width / 20 for the sharks); the subclass constructor sets it
	int						mFrameHeight;			// +0xd4
	int						mFrame;					// +0xd8 the frame drawn (0..mNumFrames-1); SetVisible resets it
	int						mFrameDelay;			// +0xdc ticks per frame (slot 11)
	int						mNumFrames;				// +0xe0 20 for the sharks and piranhas
	bool					mVisible;				// +0xe4 (slot 8)
	int						mTick;					// +0xe8 counts Update calls (the helper 0x401220)

public:
	GameObject();																// 0x4611b0
	virtual ~GameObject();														// slot 0 (deleting 0x461e80 and the like)

	virtual void			Update() = 0;										// slot 1 (_purecall): one tick
	virtual void			Draw(Graphics* g);									// slot 2 0x461270: only RecordHistory; subclasses draw first
	virtual int				GetFrameWidth() { return mFrameWidth; }				// slot 3 0x461500
	virtual int				GetFrameHeight() { return mFrameHeight; }			// slot 4 0x461510
	virtual void			SetX(double x) { mX = x; }							// slot 5 0x4010a0
	virtual void			SetY(double y) { mY = y; }							// slot 6 0x4010c0
	virtual void			SetState(int theState) { mState = theState; }		// slot 7 0x4010e0
	virtual void			SetVisible(bool theVisible)							// slot 8 0x4010f0
	{
		mVisible = theVisible;
		mFrame = 0;
	}
	virtual void			SetVelX(double v) { mVelX = v; }					// slot 9 0x401110
	virtual void			SetVelY(double v) { mVelY = v; }					// slot 10 0x401130
	virtual void			SetFrameDelay(int theDelay) { mFrameDelay = theDelay; }	// slot 11 0x401150

	void					RecordHistory();									// the body of 0x461270
	void					IncTick() { mTick++; }								// 0x401220
};

}

#endif
