#ifndef __HAMMERHEADS_GNOMEFLYING_H__
#define __HAMMERHEADS_GNOMEFLYING_H__

// The aviator (0x4394d0-0x43a450): a gnome in a plane that crosses the screen from one side to the other. If it gets across
// untouched it takes coins out of the player's counter, one at a time, up to twenty, on its way out. If it is hit it falls
// and drops ten things, a heart and a coin in turn, one every 20 steps. It is an Object of its own, not one of a cubby's
// creatures.

#include <string>

#include "Object.h"

namespace Sexy
{

class Graphics;
class SoundInstance;

// What a saved game keeps of an aviator (0x43a370 copies it out, 0x4397e0 puts it back)
struct FlyingState
{
	std::string		mName;
	bool			mActive;
	double			mFrame;
	double			mSpeedX;
	double			mSpeedY;
	int				mUnusedInt;
	Rect			mBounds;
	double			mX;
	double			mY;
	double			mRotation;
	double			mRotationSpeed;
	bool			mHit;
	bool			mLeft;
	int				mCount;
	bool			mCalled;
	int				mHoldCount;
	int				mState;
};

class GnomeFlying : public SimpleObject
{
public:
	enum
	{
		STATE_FLYING = 0,		// the propeller turns (the frame goes up by 0.3 a step)
		STATE_HIT = 1,			// the frame stops
		STATE_DONE = 2			// never reached: it needs the frame above 4 in the hit state, and the frame stands still there
	};

	// +0x80 to +0x8f: the rectangle of one cel at the plane's position (SimpleObject's bounds); +0x90 and +0x98: its position
	double			mFrame;						// +0xa0: the cel of the propeller, 0.3 a step, its whole part eight at a time round
	double			mSpeedX;					// +0xa8: 800 to 1050 pixels a time unit, to the right or left
	double			mSpeedY;					// +0xb0: 0 until it is hit, then 250 (it falls)
	int				mUnusedInt;					// +0xb8: set to 0 and never used
	double			mRotation;					// +0xc0: the angle of its picture; never changed (the speed below is 0)
	double			mRotationSpeed;				// +0xc8: slows by 2 percent a step
	bool			mHit;						// +0xd0: it has been hit
	bool			mLeft;						// +0xd1: it left the screen without being hit; the coin taking starts
	int				mCount;						// +0xd4: the things dropped (hit) or the coins taken (left); the count of both
	bool			mCalled;					// +0xd8: it has said its "yoohoo" on the way in
	int				mHoldCount;					// +0xdc: a hit takes 1 off; it is only taken off the screen once this is below 1
	int				mState;						// +0xe0
	SoundInstance*	mEngine;					// +0xe4: the engine's noise, played until it is hit

public:
	GnomeFlying();								// 0x4398b0
	virtual ~GnomeFlying();						// 0x4394d0

	OBJECT_TYPE(GnomeFlying)

	virtual void	FixedUpdate() override;						// 0x43a110
	virtual void	Update(float theFrac) override;				// 0x439b80
	virtual void	Draw(Graphics* g) override;					// 0x439530
	virtual bool	Hit(int theX, int theY, const std::vector<Point>& theMask, int theWidth, int theHeight) override;	// 0x439cf0
	virtual void	Strike(double theX, double theY) override;	// 0x43a240

	void			SaveState(FlyingState& theState) const;		// 0x43a370
	void			LoadState(const FlyingState& theState);		// 0x4397e0

private:
	void			TakeCoin();									// 0x439db0
	void			DropItem();									// 0x439ec0
	void			UpdateBounds();
};

// 0x439b10: makes an aviator and gives it the name
GnomeFlying* MakeGnomeFlying(const std::string& theName);

// 0x439770: the aviator at the middle of a level's introduction (theCel 0 to 7, which one of its cels is shown)
void DrawFlyingIntro(Graphics* g, double theX, double theY, int theCel);

}

#endif
